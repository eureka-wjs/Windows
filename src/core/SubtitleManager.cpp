#include "SubtitleManager.h"
#include <QFileInfo>
#include <QDir>
#include <QThread>
#include <QCoreApplication>

bool SubtitleManager::processVideo(const QString& videoPath, bool force)
{
    VideoFile video(videoPath);
    return downloadSubtitle(video, force);
}

ScanResult SubtitleManager::scanFolder(const QString& folderPath, bool force)
{
    ScanResult result;
    result.folder = folderPath;
    result.startTime = QDateTime::currentDateTime();
    
    m_stopRequested = false;
    
    if (m_logger) {
        m_logger->info(QString("开始扫描：%1").arg(folderPath));
    }
    
    // 检查配额
    m_api->checkQuota();
    
    // 扫描视频文件
    QStringList videoFiles = FileUtils::scanVideoFiles(
        folderPath, 
        m_config.videoExtensions(),
        true,  // 递归扫描
        m_config.minFileSize()
    );
    
    int totalFiles = videoFiles.size();
    int processed = 0;
    
    for (int i = 0; i < totalFiles && !m_stopRequested; ++i) {
        const QString& filePath = videoFiles[i];
        
        // 快速检查：同目录是否存在字幕
        QStringList existingSubs = FileUtils::findExistingSubtitles(filePath);
        if (!existingSubs.isEmpty() && !force) {
            if (m_logger) {
                m_logger->info(QString("跳过（已有字幕）：%1").arg(filePath));
            }
            result.skipped++;
            result.apiCallsSaved++;
            continue;
        }
        
        // 检查历史记录
        if (!force && m_history->isProcessed(filePath, m_config.useHashMatching())) {
            if (m_logger) {
                m_logger->info(QString("跳过（已处理）：%1").arg(filePath));
            }
            result.skipped++;
            result.apiCallsSaved++;
            continue;
        }
        
        // 处理视频
        processed++;
        emit progressUpdated(i + 1, totalFiles);
        
        bool success = processVideo(filePath, force);
        if (success) {
            result.success++;
        } else {
            result.failed++;
            FailedRecord record;
            record.filePath = filePath;
            record.reason = "下载失败";  // 具体原因在 downloadSubtitle 中记录
            result.failedFiles.append(record);
        }
        
        // 批量处理延迟
        if (m_config.batchDelay() > 0) {
            QThread::msleep(m_config.batchDelay() * 1000);
        }
    }
    
    result.totalScanned = processed + result.filtered;
    result.endTime = QDateTime::currentDateTime();
    
    // 统计报告
    if (m_logger) {
        m_logger->info("");
        m_logger->info("============================================================");
        m_logger->info("批量下载统计报告");
        m_logger->info("============================================================");
        m_logger->info(QString("扫描目录：%1").arg(folderPath));
        
        qint64 duration = result.startTime.secsTo(result.endTime);
        m_logger->info(QString("总耗时：%1 秒 (%2 分钟)").arg(duration).arg(duration / 60.0, 0, 'f', 1));
        m_logger->info("");
        m_logger->info("文件统计:");
        m_logger->info(QString("  总计扫描：%1 个文件").arg(videoFiles.size()));
        m_logger->info(QString("  过滤跳过：%1 个").arg(result.filtered));
        m_logger->info(QString("  处理视频：%1 个").arg(processed));
        m_logger->info("");
        m_logger->info("处理结果:");
        m_logger->info(QString("  成功：%1 个").arg(result.success));
        m_logger->info(QString("  失败：%1 个").arg(result.failed));
        m_logger->info(QString("  跳过：%1 个").arg(result.skipped));
        if (processed > 0) {
            m_logger->info(QString("  成功率：%1%").arg(result.success * 100.0 / processed, 0, 'f', 1));
        }
        m_logger->info("");
        m_logger->info("配额统计:");
        m_logger->info(QString("  节省 API 调用：%1 次").arg(result.apiCallsSaved));
        m_logger->info(QString("  当前配额：%1/分钟").arg(m_api->quota()));
        
        if (!result.failedFiles.isEmpty()) {
            m_logger->info("");
            m_logger->info("失败文件:");
            for (int i = 0; i < result.failedFiles.size(); ++i) {
                const FailedRecord& record = result.failedFiles[i];
                QFileInfo fileInfo(record.filePath);
                m_logger->info(QString("  %1. %2 - %3")
                    .arg(i + 1)
                    .arg(fileInfo.fileName())
                    .arg(record.reason));
            }
        }
        
        if (result.success == processed && result.failed == 0) {
            m_logger->info("");
            m_logger->success("所有文件处理成功！");
        }
        
        m_logger->info("============================================================");
    }
    
    emit statsUpdated(result);
    emit processingComplete(result);
    
    return result;
}

bool SubtitleManager::downloadSubtitle(const VideoFile& video, bool force)
{
    QString filename = video.name();
    QString folder = video.folder();
    
    if (m_logger) {
        m_logger->info(QString("开始处理：%1").arg(filename));
    }
    
    // 检查 1：快速检查同目录是否存在字幕
    if (!force) {
        QStringList existing = FileUtils::findExistingSubtitles(video.path());
        if (!existing.isEmpty()) {
            if (m_logger) {
                m_logger->success(QString("已存在字幕：%1").arg(existing[0]));
            }
            return true;
        }
    }
    
    // 检查 2：检查历史记录
    if (!force && m_history->isProcessed(video.path(), m_config.useHashMatching())) {
        if (m_logger) {
            m_logger->info("已处理过，跳过");
        }
        return true;
    }
    
    // 提取名称
    QString movieName = NameParser::extractMovieName(filename);
    if (m_logger) {
        m_logger->info(QString("识别名称：%1").arg(movieName));
    }
    
    if (movieName.isEmpty() || movieName.length() < 3) {
        if (m_logger) {
            m_logger->error("无法识别电影名称（长度至少 3 个字符）");
        }
        return false;
    }
    
    // 多策略搜索
    QList<SubtitleInfo> subs;
    QString searchStrategy;
    
    // 策略 1: 使用完整文件名搜索
    if (m_logger) {
        m_logger->debug("策略 1: 完整文件名搜索");
    }
    subs = m_api->search(filename, true);
    if (!subs.isEmpty()) {
        searchStrategy = "完整文件名 (is_file=1)";
    }
    
    // 策略 2: 使用优化提取的名称搜索
    if (subs.isEmpty()) {
        if (m_logger) {
            m_logger->debug("策略 2: 优化名称搜索");
        }
        subs = m_api->search(movieName, false);
        if (!subs.isEmpty()) {
            searchStrategy = QString("优化名称：%1").arg(movieName);
        }
    }
    
    // 策略 3: 使用剧集标准名称搜索
    if (subs.isEmpty()) {
        QString showName = NameParser::extractShowName(filename);
        if (showName != movieName) {
            if (m_logger) {
                m_logger->debug(QString("策略 3: 剧集名称搜索：%1").arg(showName));
            }
            subs = m_api->search(showName, false);
            if (!subs.isEmpty()) {
                searchStrategy = QString("剧集名称：%1").arg(showName);
            }
        }
    }
    
    // 策略 4: 使用多种搜索变体
    if (subs.isEmpty()) {
        if (m_logger) {
            m_logger->debug("策略 4: 多变体搜索");
        }
        QStringList variants = NameParser::generateSearchVariants(movieName);
        for (const QString& variant : variants) {
            if (variant != movieName) {
                if (m_logger) {
                    m_logger->debug(QString("  尝试变体：%1").arg(variant));
                }
                subs = m_api->search(variant, false);
                if (!subs.isEmpty()) {
                    searchStrategy = QString("搜索变体：%1").arg(variant);
                    break;
                }
            }
        }
    }
    
    // 策略 5: 简化名称
    if (subs.isEmpty()) {
        QString simpleName = NameParser::simplifyName(movieName);
        if (simpleName != movieName) {
            if (m_logger) {
                m_logger->debug(QString("策略 5: 简化名称搜索：%1").arg(simpleName));
            }
            subs = m_api->search(simpleName, false);
            if (!subs.isEmpty()) {
                searchStrategy = QString("简化名称：%1").arg(simpleName);
            }
        }
    }
    
    if (!subs.isEmpty()) {
        if (m_logger) {
            m_logger->success(QString("搜索成功 (策略：%1)").arg(searchStrategy));
        }
    }
    
    if (subs.isEmpty()) {
        if (m_logger) {
            m_logger->error("未找到字幕");
        }
        m_history->markFailed(video.path(), "未找到字幕");
        return false;
    }
    
    // 过滤中文字幕
    QList<SubtitleInfo> chineseSubs = filterChineseSubs(subs);
    
    // 显示搜索结果
    if (m_logger) {
        m_logger->info(QString("找到 %1 个字幕:").arg(chineseSubs.size()));
        for (int i = 0; i < qMin(5, chineseSubs.size()); ++i) {
            const SubtitleInfo& sub = chineseSubs[i];
            m_logger->info(QString("  %1. [%2] %3 (%4) [%5]")
                .arg(i + 1)
                .arg(sub.voteScore(), 3)
                .arg(sub.nativeName())
                .arg(sub.languageDesc())
                .arg(sub.subtype()));
        }
    }
    
    // 选择最佳字幕
    SubtitleInfo bestSub = selectBestSubtitle(chineseSubs);
    
    if (bestSub.id() == 0) {
        if (m_logger) {
            m_logger->error("未找到合适的字幕");
        }
        m_history->markFailed(video.path(), "未找到合适的字幕");
        return false;
    }
    
    // 下载字幕
    QString subPath = bestSub.getSavePath(video.path());
    bool success = m_api->download(bestSub.id(), subPath, video.path());
    
    if (success) {
        if (m_logger) {
            m_logger->success(QString("下载完成：%1").arg(filename));
        }
        m_history->markProcessed(video.path(), subPath, m_config.useHashMatching());
        return true;
    } else {
        if (m_logger) {
            m_logger->error(QString("下载失败：%1").arg(filename));
        }
        m_history->markFailed(video.path(), "下载失败");
        return false;
    }
}

SubtitleInfo SubtitleManager::selectBestSubtitle(const QList<SubtitleInfo>& subs)
{
    if (subs.isEmpty()) {
        return SubtitleInfo();
    }
    
    // 按投票分数排序
    QList<SubtitleInfo> sorted = subs;
    std::sort(sorted.begin(), sorted.end(), [](const SubtitleInfo& a, const SubtitleInfo& b) {
        return a.voteScore() > b.voteScore();
    });
    
    return sorted[0];
}

QList<SubtitleInfo> SubtitleManager::filterChineseSubs(const QList<SubtitleInfo>& subs)
{
    QList<SubtitleInfo> chineseSubs;
    
    for (const SubtitleInfo& sub : subs) {
        QString desc = sub.languageDesc().toLower();
        if (desc.contains("chinese") || desc.contains("chi") || desc.contains("zh")) {
            chineseSubs.append(sub);
        }
    }
    
    return chineseSubs.isEmpty() ? subs : chineseSubs;
}

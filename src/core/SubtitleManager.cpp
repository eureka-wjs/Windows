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
    
    bool isDebugMode = m_config.debugMode();
    
    if (isDebugMode) {
        m_logger->debug("═══════════════════════════════════════════════════════");
        m_logger->debug("【扫描文件夹】开始");
        m_logger->debug(QString("  扫描路径：%1").arg(folderPath));
        m_logger->debug(QString("  强制模式：%1").arg(force ? "是" : "否"));
        m_logger->debug(QString("  最小文件大小：%1 字节").arg(m_config.minFileSize()));
        m_logger->debug(QString("  视频扩展名：%1").arg(m_config.videoExtensions().join(", ")));
    }
    
    // 稳定性检查：确保已正确初始化
    if (!m_api) {
        if (m_logger) {
            m_logger->error("字幕管理器未初始化：m_api 为空");
        }
        emit processingComplete(result);
        return result;
    }
    
    if (!m_history) {
        if (m_logger) {
            m_logger->error("字幕管理器未初始化：m_history 为空");
        }
        emit processingComplete(result);
        return result;
    }
    
    if (m_logger) {
        m_logger->info(QString("开始扫描：%1").arg(folderPath));
    }
    
    // 检查配额
    if (isDebugMode) {
        m_logger->debug(QString("检查 API 配额，当前配额：%1/分钟").arg(m_api->quota()));
    }
    if (!m_api->checkQuota()) {
        if (m_logger) {
            m_logger->error("API 配额不足或检查失败");
            m_logger->error(QString("  当前配额：%1/分钟").arg(m_api->quota()));
        }
        emit processingComplete(result);
        return result;
    }
    
    // 扫描视频文件
    if (isDebugMode) {
        m_logger->debug("扫描视频文件...");
    }
    QStringList videoFiles = FileUtils::scanVideoFiles(
        folderPath,
        m_config.videoExtensions(),
        true,  // 递归扫描
        m_config.minFileSize()
    );
    
    int totalFiles = videoFiles.size();
    int processed = 0;
    
    if (isDebugMode) {
        m_logger->debug(QString("扫描完成，找到 %1 个视频文件").arg(totalFiles));
        if (totalFiles > 0) {
            m_logger->debug("视频文件列表:");
            for (int i = 0; i < totalFiles; ++i) {
                m_logger->debug(QString("  %1. %2").arg(i + 1).arg(videoFiles[i]));
            }
        }
    }
    
    for (int i = 0; i < totalFiles && !m_stopRequested; ++i) {
        const QString& filePath = videoFiles[i];
        
        // 非调试模式下也显示处理进度
        if (m_logger) {
            m_logger->info(QString("═══════════════════════════════════════════════════════"));
            m_logger->info(QString("处理文件 %1/%2: %3").arg(i + 1).arg(totalFiles).arg(QFileInfo(filePath).fileName()));
        }
        
        if (isDebugMode) {
            m_logger->debug("═══════════════════════════════════════════════════════");
            m_logger->debug(QString("【处理文件 %1/%2】%3").arg(i + 1).arg(totalFiles).arg(filePath));
        }
        
        // 快速检查：同目录是否存在字幕
        if (isDebugMode) {
            m_logger->debug("检查同目录字幕...");
        }
        QStringList existingSubs = FileUtils::findExistingSubtitles(filePath);
        if (!existingSubs.isEmpty() && !force) {
            if (m_logger) {
                m_logger->info(QString("跳过（已有字幕）：%1").arg(filePath));
                if (isDebugMode) {
                    m_logger->debug(QString("  已存在的字幕：%1").arg(existingSubs.join(", ")));
                }
            }
            result.skipped++;
            result.apiCallsSaved++;
            // 实时更新统计
            emit statsUpdated(result);
            if (isDebugMode) {
                m_logger->debug(QString("  节省 API 调用次数：%1").arg(result.apiCallsSaved));
            }
            continue;
        }
        if (isDebugMode) {
            m_logger->debug("  同目录无字幕文件");
        }
        
        // 检查历史记录
        if (isDebugMode) {
            m_logger->debug("检查历史记录...");
            m_logger->debug(QString("  使用哈希匹配：%1").arg(m_config.useHashMatching() ? "是" : "否"));
        }
        if (!force && m_history->isProcessed(filePath, m_config.useHashMatching())) {
            if (m_logger) {
                m_logger->info(QString("跳过（已处理）：%1").arg(filePath));
            }
            result.skipped++;
            result.apiCallsSaved++;
            // 实时更新统计
            emit statsUpdated(result);
            continue;
        }
        if (isDebugMode) {
            m_logger->debug("  历史记录中无此文件");
        }
        
        // 处理视频
        processed++;
        emit progressUpdated(i + 1, totalFiles, filePath);
        
        if (isDebugMode) {
            m_logger->debug("开始处理视频...");
        }
        bool success = processVideo(filePath, force);
        if (success) {
            result.success++;
            if (m_logger) {
                m_logger->success(QString("✓ 文件处理成功：%1").arg(QFileInfo(filePath).fileName()));
            }
        } else {
            result.failed++;
            FailedRecord record;
            record.filePath = filePath;
            record.reason = "下载失败";  // 具体原因在 downloadSubtitle 中记录
            result.failedFiles.append(record);
            if (m_logger) {
                m_logger->error(QString("✗ 文件处理失败：%1").arg(QFileInfo(filePath).fileName()));
            }
        }
        // 实时更新统计
        emit statsUpdated(result);
        
        // 批量处理延迟
        if (m_config.batchDelay() > 0) {
            if (isDebugMode) {
                m_logger->debug(QString("等待 %1 秒...").arg(m_config.batchDelay()));
            }
            QThread::msleep(m_config.batchDelay() * 1000);
        }
    }
    
    result.totalScanned = videoFiles.size();
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
    QString failureReason = "未知错误";
    bool isDebugMode = m_config.debugMode();
    
    if (isDebugMode) {
        m_logger->debug("═══════════════════════════════════════════════════════");
        m_logger->debug("【下载字幕】开始");
        m_logger->debug(QString("  视频名称：%1").arg(filename));
        m_logger->debug(QString("  视频目录：%1").arg(folder));
        m_logger->debug(QString("  完整路径：%1").arg(video.path()));
        m_logger->debug(QString("  文件大小：%1 字节").arg(QFileInfo(video.path()).size()));
        m_logger->debug(QString("  强制模式：%1").arg(force ? "是" : "否"));
        m_logger->debug(QString("  使用哈希匹配：%1").arg(m_config.useHashMatching() ? "是" : "否"));
    }
    
    if (m_logger) {
        m_logger->info(QString("═══════════════════════════════════════════════════════"));
        m_logger->info(QString("开始处理：%1").arg(filename));
        m_logger->info(QString("  完整路径：%1").arg(video.path()));
        m_logger->info(QString("  文件大小：%1 字节").arg(QFileInfo(video.path()).size()));
    }
    
    // 检查 1：快速检查同目录是否存在字幕
    if (!force) {
        if (isDebugMode) {
            m_logger->debug("【检查 1】检查同目录字幕...");
        }
        QStringList existing = FileUtils::findExistingSubtitles(video.path());
        if (!existing.isEmpty()) {
            if (m_logger) {
                m_logger->success(QString("已存在字幕：%1").arg(existing[0]));
                if (isDebugMode) {
                    m_logger->debug(QString("  已存在的字幕文件：%1").arg(existing.join(", ")));
                }
            }
            return true;
        }
        if (isDebugMode) {
            m_logger->debug("  同目录不存在字幕文件");
        }
    }
    
    // 检查 2：检查历史记录
    if (isDebugMode) {
        m_logger->debug("【检查 2】检查历史记录...");
    }
    if (!force && m_history->isProcessed(video.path(), m_config.useHashMatching())) {
        if (m_logger) {
            m_logger->info("已处理过，跳过");
        }
        return true;
    }
    if (isDebugMode) {
        m_logger->debug("  历史记录中无此文件");
    }
    
    // 提取名称
    if (isDebugMode) {
        m_logger->debug("【名称解析】开始解析文件名...");
        m_logger->debug(QString("  原始文件名：%1").arg(filename));
    }
    QString movieName = NameParser::extractMovieName(filename);
    if (m_logger) {
        m_logger->info(QString("识别名称：%1").arg(movieName));
    }
    if (isDebugMode) {
        m_logger->debug(QString("  解析结果：%1").arg(movieName));
        m_logger->debug(QString("  结果长度：%1").arg(movieName.length()));
    }
    
    if (movieName.isEmpty() || movieName.length() < 3) {
        failureReason = "无法识别电影名称（长度至少 3 个字符）";
        if (m_logger) {
            m_logger->error(QString("❌ 失败原因：%1").arg(failureReason));
            if (isDebugMode) {
                m_logger->debug(QString("  原始文件名：%1").arg(filename));
                m_logger->debug(QString("  提取结果：'%1'").arg(movieName));
                m_logger->debug(QString("  结果长度：%1").arg(movieName.length()));
                m_logger->debug("  名称为空：" + QString(movieName.isEmpty() ? "是" : "否"));
                m_logger->debug("  长度小于 3：" + QString(movieName.length() < 3 ? "是" : "否"));
            }
        }
        m_history->markFailed(video.path(), failureReason);
        return false;
    }
    
    if (isDebugMode) {
        m_logger->debug(QString("名称解析成功：%1").arg(movieName));
    }
    
    // 多策略搜索
    QList<SubtitleInfo> subs;
    QString searchStrategy;
    QString showName;  // 提前声明，用于错误总结
    
    if (isDebugMode) {
        m_logger->debug("═══════════════════════════════════════════════════════");
        m_logger->debug("【多策略搜索】开始");
        m_logger->debug(QString("  当前配额：%1/分钟").arg(m_api->quota()));
    }
    
    // 策略 1: 使用完整文件名搜索
    if (isDebugMode) {
        m_logger->debug("【策略 1】完整文件名搜索 (is_file=1)");
        m_logger->debug(QString("  搜索关键词：%1").arg(filename));
        m_logger->debug("  调用 API 搜索...");
    }
    subs = m_api->search(filename, true);
    if (isDebugMode) {
        m_logger->debug(QString("  API 返回结果数：%1").arg(subs.size()));
    }
    if (!subs.isEmpty()) {
        searchStrategy = "完整文件名 (is_file=1)";
        if (isDebugMode) {
            m_logger->debug(QString("  ✓ 找到 %1 个结果").arg(subs.size()));
            m_logger->debug("  结果列表:");
            for (int i = 0; i < qMin(3, subs.size()); ++i) {
                const SubtitleInfo& sub = subs[i];
                m_logger->debug(QString("    %1. ID=%2, 评分=%3, 名称=%4, 语言=%5")
                    .arg(i + 1)
                    .arg(sub.id())
                    .arg(sub.voteScore())
                    .arg(sub.nativeName())
                    .arg(sub.languageDesc()));
            }
            if (subs.size() > 3) {
                m_logger->debug(QString("    ... 还有 %1 个结果").arg(subs.size() - 3));
            }
        }
    } else {
        if (isDebugMode) {
            m_logger->debug("  ✗ 未找到结果");
        }
    }
    
    // 策略 2: 使用优化提取的名称搜索
    if (subs.isEmpty()) {
        if (isDebugMode) {
            m_logger->debug("【策略 2】优化名称搜索");
            m_logger->debug(QString("  搜索关键词：%1").arg(movieName));
            m_logger->debug("  调用 API 搜索...");
        }
        subs = m_api->search(movieName, false);
        if (isDebugMode) {
            m_logger->debug(QString("  API 返回结果数：%1").arg(subs.size()));
        }
        if (!subs.isEmpty()) {
            searchStrategy = QString("优化名称：%1").arg(movieName);
            if (isDebugMode) {
                m_logger->debug(QString("  ✓ 找到 %1 个结果").arg(subs.size()));
                m_logger->debug("  结果列表:");
                for (int i = 0; i < qMin(3, subs.size()); ++i) {
                    const SubtitleInfo& sub = subs[i];
                    m_logger->debug(QString("    %1. ID=%2, 评分=%3, 名称=%4")
                        .arg(i + 1)
                        .arg(sub.id())
                        .arg(sub.voteScore())
                        .arg(sub.nativeName()));
                }
            }
        } else {
            if (isDebugMode) {
                m_logger->debug("  ✗ 未找到结果");
            }
        }
    }
    
    // 策略 3: 使用剧集标准名称搜索
    if (subs.isEmpty()) {
        if (isDebugMode) {
            m_logger->debug("【策略 3】剧集名称搜索");
        }
        showName = NameParser::extractShowName(filename);
        if (isDebugMode) {
            m_logger->debug(QString("  解析得到的剧集名称：%1").arg(showName));
        }
        if (showName != movieName && !showName.isEmpty()) {
            if (isDebugMode) {
                m_logger->debug(QString("  搜索关键词：%1").arg(showName));
                m_logger->debug("  调用 API 搜索...");
            }
            subs = m_api->search(showName, false);
            if (isDebugMode) {
                m_logger->debug(QString("  API 返回结果数：%1").arg(subs.size()));
            }
            if (!subs.isEmpty()) {
                searchStrategy = QString("剧集名称：%1").arg(showName);
                if (isDebugMode) {
                    m_logger->debug(QString("  ✓ 找到 %1 个结果").arg(subs.size()));
                }
            } else {
                if (isDebugMode) {
                    m_logger->debug("  ✗ 未找到结果");
                }
            }
        } else {
            if (isDebugMode) {
                m_logger->debug("  跳过（剧集名称与电影名称相同或为空）");
            }
        }
    }
    
    // 策略 4: 使用多种搜索变体
    if (subs.isEmpty()) {
        if (isDebugMode) {
            m_logger->debug("【策略 4】多变体搜索");
        }
        QStringList variants = NameParser::generateSearchVariants(movieName);
        if (isDebugMode) {
            m_logger->debug(QString("  生成的变体数量：%1").arg(variants.size()));
            m_logger->debug(QString("  变体列表：%1").arg(variants.join(", ")));
        }
        int variantCount = 0;
        int apiCallCount = 0;
        for (const QString& variant : variants) {
            if (variant != movieName) {
                variantCount++;
                if (isDebugMode) {
                    m_logger->debug(QString("  尝试变体 %1/%2: %3").arg(variantCount).arg(variants.size() - 1).arg(variant));
                }
                subs = m_api->search(variant, false);
                apiCallCount++;
                if (isDebugMode) {
                    m_logger->debug(QString("    API 返回结果数：%1").arg(subs.size()));
                }
                if (!subs.isEmpty()) {
                    searchStrategy = QString("搜索变体：%1").arg(variant);
                    if (isDebugMode) {
                        m_logger->debug(QString("    ✓ 找到 %1 个结果").arg(subs.size()));
                        m_logger->debug(QString("    累计 API 调用次数：%1").arg(apiCallCount));
                    }
                    break;
                } else {
                    if (isDebugMode) {
                        m_logger->debug("    ✗ 未找到结果");
                    }
                }
            }
        }
        if (isDebugMode) {
            if (variantCount == 0) {
                m_logger->debug("  没有可用的搜索变体");
            } else {
                m_logger->debug(QString("  变体搜索结束，共调用 API %1 次").arg(apiCallCount));
            }
        }
    }
    
    // 策略 5: 简化名称
    if (subs.isEmpty()) {
        if (isDebugMode) {
            m_logger->debug("【策略 5】简化名称搜索");
        }
        QString simpleName = NameParser::simplifyName(movieName);
        if (isDebugMode) {
            m_logger->debug(QString("  原始名称：%1").arg(movieName));
            m_logger->debug(QString("  简化名称：%1").arg(simpleName));
        }
        if (simpleName != movieName && !simpleName.isEmpty()) {
            if (isDebugMode) {
                m_logger->debug(QString("  搜索关键词：%1").arg(simpleName));
                m_logger->debug("  调用 API 搜索...");
            }
            subs = m_api->search(simpleName, false);
            if (isDebugMode) {
                m_logger->debug(QString("  API 返回结果数：%1").arg(subs.size()));
            }
            if (!subs.isEmpty()) {
                searchStrategy = QString("简化名称：%1").arg(simpleName);
                if (isDebugMode) {
                    m_logger->debug(QString("  ✓ 找到 %1 个结果").arg(subs.size()));
                }
            } else {
                if (isDebugMode) {
                    m_logger->debug("  ✗ 未找到结果");
                }
            }
        } else {
            if (isDebugMode) {
                m_logger->debug("  跳过（简化名称与原始名称相同或为空）");
            }
        }
    }
    
    if (isDebugMode) {
        m_logger->debug("【多策略搜索】结束");
        m_logger->debug(QString("  最终结果：找到 %1 个字幕").arg(subs.size()));
        m_logger->debug(QString("  使用策略：%1").arg(searchStrategy.isEmpty() ? "无" : searchStrategy));
    }
    
    // 搜索结果总结
    if (!subs.isEmpty()) {
        if (m_logger) {
            m_logger->success(QString("✓ 搜索成功 (策略：%1)").arg(searchStrategy));
            if (isDebugMode) {
                m_logger->debug(QString("  共找到 %1 个字幕").arg(subs.size()));
            }
        }
    } else {
        QString failureReason = "所有搜索策略均未找到字幕";
        if (m_logger) {
            m_logger->error(QString("❌ 失败原因：%1").arg(failureReason));
            // 始终显示已尝试的搜索策略（不仅仅是调试模式）
            m_logger->info("已尝试的搜索策略:");
            m_logger->info("  1. 完整文件名搜索 (is_file=1) - 失败");
            m_logger->info(QString("  2. 优化名称搜索 (%1) - 失败").arg(movieName));
            m_logger->info(QString("  3. 剧集名称搜索 - %1").arg(showName != movieName && !showName.isEmpty() ? "失败" : "跳过"));
            m_logger->info("  4. 多变体搜索 - 失败");
            m_logger->info("  5. 简化名称搜索 - 失败");
            if (isDebugMode) {
                m_logger->debug("详细调试信息:");
                m_logger->debug(QString("  原始文件名：%1").arg(filename));
                m_logger->debug(QString("  识别名称：%1").arg(movieName));
                m_logger->debug(QString("  剧集名称：%1").arg(showName));
            }
        }
        m_history->markFailed(video.path(), failureReason);
        return false;
    }
    
    // 过滤中文字幕
    QList<SubtitleInfo> chineseSubs = filterChineseSubs(subs);
    
    if (m_logger) {
        m_logger->info(QString("字幕过滤：原始 %1 个，中文字幕 %2 个")
            .arg(subs.size()).arg(chineseSubs.size()));
        if (chineseSubs.isEmpty()) {
            m_logger->warning("警告：未找到中文字幕，将使用所有结果");
        }
    }
    
    if (isDebugMode) {
        m_logger->debug(QString("过滤前：%1 个字幕，过滤后：%2 个中文字幕")
            .arg(subs.size()).arg(chineseSubs.size()));
        if (chineseSubs.isEmpty()) {
            m_logger->debug("警告：未找到中文字幕，将使用所有结果");
        }
    }
    
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
        if (chineseSubs.size() > 5) {
            m_logger->info(QString("  ... 还有 %1 个字幕").arg(chineseSubs.size() - 5));
        }
    }
    
    // 选择最佳字幕
    SubtitleInfo bestSub = selectBestSubtitle(chineseSubs);
    
    if (bestSub.id() == 0) {
        QString failureReason = "未找到合适的字幕（所有字幕评分均为 0）";
        if (m_logger) {
            m_logger->error(QString("❌ 失败原因：%1").arg(failureReason));
            // 始终显示字幕列表（不仅仅是调试模式）
            if (!chineseSubs.isEmpty()) {
                m_logger->info("字幕列表:");
                for (int i = 0; i < chineseSubs.size(); ++i) {
                    const SubtitleInfo& sub = chineseSubs[i];
                    m_logger->info(QString("  %1. ID=%2, 评分=%3, 名称=%4")
                        .arg(i + 1)
                        .arg(sub.id())
                        .arg(sub.voteScore())
                        .arg(sub.nativeName()));
                }
            } else {
                m_logger->info("字幕列表为空（过滤后无中文字幕）");
            }
            if (isDebugMode) {
                m_logger->debug("详细调试信息：");
                m_logger->debug(QString("  过滤前字幕数：%1").arg(subs.size()));
                m_logger->debug(QString("  过滤后字幕数：%1").arg(chineseSubs.size()));
            }
        }
        m_history->markFailed(video.path(), failureReason);
        return false;
    }
    
    if (isDebugMode) {
        m_logger->debug(QString("选择最佳字幕：ID=%1, 评分=%2, 名称=%3")
            .arg(bestSub.id())
            .arg(bestSub.voteScore())
            .arg(bestSub.nativeName()));
    }
    
    // 下载字幕
    QString subPath = bestSub.getSavePath(video.path());
    if (m_logger) {
        m_logger->info(QString("准备下载：字幕 ID=%1, 保存路径=%2").arg(bestSub.id()).arg(subPath));
    }
    if (isDebugMode) {
        m_logger->debug(QString("详细：原始视频路径=%1").arg(video.path()));
    }
    
    bool success = m_api->download(bestSub.id(), subPath, video.path());
    
    if (success) {
        if (m_logger) {
            m_logger->success(QString("✓ 下载完成：%1").arg(filename));
            m_logger->info(QString("  保存位置：%1").arg(subPath));
        }
        m_history->markProcessed(video.path(), subPath, m_config.useHashMatching());
        return true;
    } else {
        QString failureReason = "API 下载失败（可能是网络问题或配额不足）";
        if (m_logger) {
            m_logger->error(QString("❌ 失败原因：%1").arg(failureReason));
            // 始终显示下载详情（不仅仅是调试模式）
            m_logger->info("下载详情:");
            m_logger->info(QString("  字幕 ID: %1").arg(bestSub.id()));
            m_logger->info(QString("  字幕名称：%1").arg(bestSub.nativeName()));
            m_logger->info(QString("  保存路径：%1").arg(subPath));
            m_logger->info(QString("  当前配额：%1/分钟").arg(m_api->quota()));
            if (isDebugMode) {
                m_logger->debug(QString("  原始视频路径：%1").arg(video.path()));
            }
        }
        m_history->markFailed(video.path(), failureReason);
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

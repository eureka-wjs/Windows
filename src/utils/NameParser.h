#ifndef NAMEPARSER_H
#define NAMEPARSER_H

#include <QString>
#include <QStringList>
#include <QRegularExpression>
#include <QFileInfo>

/**
 * @brief 视频名称解析工具类
 * 
 * 从视频文件名中提取影视作品名称，支持电影和剧集
 */
class NameParser
{
public:
    NameParser() = default;
    
    /**
     * @brief 提取电影名称（优化版）
     * 
     * 移除分辨率、来源、编码、音频、语言、版本等标签
     * 
     * @param filename 视频文件名
     * @return 提取的电影名称
     */
    static QString extractMovieName(const QString& filename) {
        QFileInfo fileInfo(filename);
        QString name = fileInfo.completeBaseName();
        
        // 移除常见标签（按优先级排序）
        QStringList patterns = {
            // 分辨率标签
            R"(\.(4K|2160p|1080p|720p|480p|\d{3,4}p))",
            // 来源标签
            R"(\.(BluRay|WEB-DL|WEBRip|HDTV|DVDRip|BDRip|HDRip))",
            // 编码标签
            R"(\.(x264|x265|HEVC|H264|AVC|VP9|AV1))",
            // 音频标签
            R"(\.(AAC|DTS|DD5\.1|TrueHD|FLAC|AC3|EAC3|MP3))",
            // 语言/音轨标签
            R"(\.(Dub|Dual|Multi.*Audio|CHS|CHT|ENG|CN|US|JP|KR))",
            // 版本标签
            R"(\.(REMASTERED|EXTENDED|THEATRICAL|UNCUT|UNRATED|DIRECTOR))",
            // 其他标签
            R"(\.(10bit|8bit|Hi10p|Hi444p|Main10))",
            // 方括号内容
            R"(\[.*?\])",
            // 圆括号内容
            R"(\(.*?\))",
            // 网站链接
            R"(-\s*www\.[^\s]+)",
            R"(\.cn\s*$)",
            R"(\.com\s*$)",
        };
        
        for (const QString& pattern : patterns) {
            QRegularExpression re(pattern, QRegularExpression::CaseInsensitiveOption);
            name.replace(re, "");
        }
        
        // 清理并标准化
        name = name.replace(".", " ").replace("_", " ").trimmed();
        QRegularExpression spaceRe(R"(\s+)");
        name = name.replace(spaceRe, " ").trimmed();
        
        // 保留年份（如果有）
        QRegularExpression yearRe(R"(\b(19|20)\d{2}\b)");
        QRegularExpressionMatch match = yearRe.match(name);
        if (match.hasMatch()) {
            name = name.left(match.capturedEnd());
        }
        
        return name.trimmed();
    }
    
    /**
     * @brief 提取剧集标准名称
     * 
     * 仅保留剧名 + SXXEXX 季集信息
     * 
     * @param filename 视频文件名
     * @return 提取的剧集名称
     */
    static QString extractShowName(const QString& filename) {
        QFileInfo fileInfo(filename);
        QString name = fileInfo.completeBaseName();
        
        // 移除常见标签（与 extractMovieName 类似）
        QStringList patterns = {
            // 分辨率标签
            R"(\.(4K|2160p|1080p|720p|480p|\d{3,4}p))",
            // 来源标签
            R"(\.(BluRay|WEB-DL|WEBRip|HDTV|DVDRip|BDRip|HDRip))",
            // 编码标签
            R"(\.(x264|x265|HEVC|H264|AVC|VP9|AV1))",
            // 音频标签
            R"(\.(AAC|DTS|DD5\.1|TrueHD|FLAC|AC3|EAC3|MP3))",
            // 语言/音轨标签
            R"(\.(Dub|Dual|Multi.*Audio|CHS|CHT|ENG|CN|US|JP|KR))",
            // 版本标签
            R"(\.(REMASTERED|EXTENDED|THEATRICAL|UNCUT|UNRATED|DIRECTOR))",
            // 方括号内容
            R"(\[.*?\])",
            // 圆括号内容
            R"(\(.*?\))",
            // 网站链接
            R"(-\s*www\.[^\s]+)",
            R"(\.cn\s*$)",
            R"(\.com\s*$)",
        };
        
        for (const QString& pattern : patterns) {
            QRegularExpression re(pattern, QRegularExpression::CaseInsensitiveOption);
            name.replace(re, "");
        }
        
        // 清理并标准化
        name = name.replace(".", " ").replace("_", " ").trimmed();
        QRegularExpression spaceRe(R"(\s+)");
        name = name.replace(spaceRe, " ").trimmed();
        
        // 尝试提取 SXXEXX 格式
        QRegularExpression seasonEpisodeRe(R"(S\d+E\d+)", QRegularExpression::CaseInsensitiveOption);
        QRegularExpressionMatch match = seasonEpisodeRe.match(name);
        if (match.hasMatch()) {
            // 保留剧名 + 季集信息
            int endPos = match.capturedEnd();
            name = name.left(endPos).trimmed();
        }
        
        return name.trimmed();
    }
    
    /**
     * @brief 生成多种搜索变体
     * 
     * 生成多种命名格式变体，提高匹配成功率
     * 
     * @param query 原始查询字符串
     * @return 搜索变体列表
     */
    static QStringList generateSearchVariants(const QString& query) {
        QStringList variants;
        variants.append(query);
        
        // 变体 1: 移除 Dub/Dual 等音轨标签
        QRegularExpression noDubRe(R"(\s*(Dub|Dual|Multi.*Audio)\s*)", QRegularExpression::CaseInsensitiveOption);
        QString noDub = query.replace(noDubRe, " ").trimmed();
        QRegularExpression spaceRe(R"(\s+)");
        noDub = noDub.replace(spaceRe, " ").trimmed();
        if (!noDub.isEmpty() && noDub != query) {
            variants.append(noDub);
        }
        
        // 变体 2: 使用破折号格式（S01E01 -> S01-E01）
        QRegularExpression dashRe(R"(S(\d+)E(\d+))", QRegularExpression::CaseInsensitiveOption);
        QString dashFormat = query;
        // 手动替换以支持捕获组
        QString result;
        int lastPos = 0;
        QRegularExpressionMatchIterator it = dashRe.globalMatch(query);
        while (it.hasNext()) {
            QRegularExpressionMatch match = it.next();
            result += query.mid(lastPos, match.capturedStart() - lastPos);
            result += QString("S%1-E%2").arg(match.captured(1)).arg(match.captured(2));
            lastPos = match.capturedEnd();
        }
        result += query.mid(lastPos);
        dashFormat = result;
        if (dashFormat != query) {
            variants.append(dashFormat);
        }
        
        // 变体 3: 使用点号分隔（S01E01 -> S01.E01）
        result.clear();
        lastPos = 0;
        it = dashRe.globalMatch(query);
        while (it.hasNext()) {
            QRegularExpressionMatch match = it.next();
            result += query.mid(lastPos, match.capturedStart() - lastPos);
            result += QString("S%1.E%2").arg(match.captured(1)).arg(match.captured(2));
            lastPos = match.capturedEnd();
        }
        result += query.mid(lastPos);
        if (result != query) {
            variants.append(result);
        }
        
        // 变体 4: 仅保留剧名 + 季集（如果有）
        QRegularExpression showRe(R"((.+?\s*S\d+E\d+))", QRegularExpression::CaseInsensitiveOption);
        QRegularExpressionMatch showMatch = showRe.match(query);
        if (showMatch.hasMatch()) {
            QString showName = showMatch.captured(1).trimmed();
            if (showName != query) {
                variants.append(showName);
            }
        }
        
        // 去重并保持顺序
        QStringList unique;
        QSet<QString> seen;
        for (const QString& v : variants) {
            if (!v.isEmpty() && !seen.contains(v)) {
                seen.insert(v);
                unique.append(v);
            }
        }
        
        return unique;
    }
    
    /**
     * @brief 简化名称（移除括号内容）
     * 
     * @param name 原始名称
     * @return 简化后的名称
     */
    static QString simplifyName(const QString& name) {
        QRegularExpression bracketRe(R"(\s+\(.*?\))");
        QString simplified = name.replace(bracketRe, "").trimmed();
        return simplified;
    }
};

#endif // NAMEPARSER_H

#ifndef SUBTITLEINFO_H
#define SUBTITLEINFO_H

#include <QString>
#include <QJsonObject>
#include <QJsonArray>
#include <QDateTime>
#include <QFileInfo>

/**
 * @brief 字幕信息数据模型
 * 
 * 封装从 API 获取的字幕信息
 */
class SubtitleInfo
{
public:
    SubtitleInfo() 
        : m_id(0)
        , m_voteScore(0)
    {}
    
    // Getter 和 Setter
    int id() const { return m_id; }
    void setId(int id) { m_id = id; }
    
    QString nativeName() const { return m_nativeName; }
    void setNativeName(const QString& name) { m_nativeName = name; }
    
    QString title() const { return m_title; }
    void setTitle(const QString& title) { m_title = title; }
    
    int voteScore() const { return m_voteScore; }
    void setVoteScore(int score) { m_voteScore = score; }
    
    QString language() const { return m_language; }
    void setLanguage(const QString& lang) { m_language = lang; }
    
    QString languageDesc() const { return m_languageDesc; }
    void setLanguageDesc(const QString& desc) { m_languageDesc = desc; }
    
    QString subtype() const { return m_subtype; }
    void setSubtype(const QString& type) { m_subtype = type; }
    
    QString downloadUrl() const { return m_downloadUrl; }
    void setDownloadUrl(const QString& url) { m_downloadUrl = url; }
    
    QString fileName() const { return m_fileName; }
    void setFileName(const QString& name) { m_fileName = name; }
    
    /**
     * @brief 是否为中文字幕
     * @return 是否包含中文
     */
    bool isChinese() const {
        QString desc = m_languageDesc.toLower();
        return desc.contains("chinese") || 
               desc.contains("chi") || 
               desc.contains("zh") ||
               desc.contains("cn") ||
               desc.contains("chs") ||
               desc.contains("cht");
    }
    
    /**
     * @brief 从 API 响应的 JSON 对象创建
     * @param obj JSON 对象
     * @return SubtitleInfo 对象
     */
    static SubtitleInfo fromJson(const QJsonObject& obj) {
        SubtitleInfo sub;
        sub.m_id = obj["id"].toInt();
        sub.m_nativeName = obj["native_name"].toString();
        sub.m_title = obj["title"].toString();
        sub.m_voteScore = obj["vote_score"].toInt();
        sub.m_subtype = obj["subtype"].toString();
        
        // 解析语言信息
        QJsonObject langObj = obj["lang"].toObject();
        sub.m_languageDesc = langObj["desc"].toString();
        
        // 解析文件列表
        QJsonArray filelist = obj["filelist"].toArray();
        if (!filelist.isEmpty()) {
            QJsonObject fileObj = filelist[0].toObject();
            sub.m_downloadUrl = fileObj["url"].toString();
            sub.m_fileName = fileObj["f"].toString("subtitle.srt");
        }
        
        return sub;
    }
    
    /**
     * @brief 转换为 JSON 对象
     * @return JSON 对象
     */
    QJsonObject toJson() const {
        QJsonObject obj;
        obj["id"] = m_id;
        obj["native_name"] = m_nativeName;
        obj["title"] = m_title;
        obj["vote_score"] = m_voteScore;
        obj["language"] = m_language;
        obj["language_desc"] = m_languageDesc;
        obj["subtype"] = m_subtype;
        obj["download_url"] = m_downloadUrl;
        obj["file_name"] = m_fileName;
        return obj;
    }
    
    /**
     * @brief 获取字幕保存路径
     * @param videoPath 视频文件路径
     * @param suffix 语言后缀
     * @return 字幕文件保存路径
     */
    QString getSavePath(const QString& videoPath, const QString& suffix = ".zh-CN") const {
        QFileInfo fileInfo(videoPath);
        QString baseName = fileInfo.completeBaseName();
        QString folder = fileInfo.absolutePath();
        
        // 根据实际文件扩展名决定字幕扩展名
        QString ext = ".srt";
        if (m_fileName.toLower().contains(".ass")) {
            ext = ".ass";
        } else if (m_fileName.toLower().contains(".ssa")) {
            ext = ".ssa";
        } else if (m_fileName.toLower().contains(".vtt")) {
            ext = ".vtt";
        }
        
        return QString("%1/%2%3%4").arg(folder, baseName, suffix, ext);
    }

private:
    int m_id;                    ///< 字幕 ID
    QString m_nativeName;        ///< 原始文件名
    QString m_title;             ///< 字幕标题
    int m_voteScore;             ///< 投票分数
    QString m_language;          ///< 语言代码
    QString m_languageDesc;      ///< 语言描述
    QString m_subtype;           ///< 字幕类型（字幕/特效等）
    QString m_downloadUrl;       ///< 下载地址
    QString m_fileName;          ///< 字幕文件名
};

#endif // SUBTITLEINFO_H

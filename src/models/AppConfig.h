#ifndef APPCONFIG_H
#define APPCONFIG_H

#include <QString>
#include <QStringList>
#include <QJsonObject>
#include <QJsonArray>
#include <QSettings>
#include <QStandardPaths>

/**
 * @brief 应用配置数据模型
 * 
 * 封装应用的所有配置项
 */
class AppConfig
{
public:
    AppConfig()
        : m_apiKey("")
        , m_languages({"chi", "zh", "zh-tw"})
        , m_videoExtensions({".mp4", ".mkv", ".avi", ".mov", ".wmv", ".flv"})
        , m_minScore(0)
        , m_timeout(30)
        , m_maxRetries(2)
        , m_batchDelay(0)
        , m_minFileSize(10 * 1024 * 1024)  // 10MB
        , m_useHashMatching(false)
        , m_workingDirectory("")
    {}
    
    // Getter 和 Setter
    QString apiKey() const { return m_apiKey; }
    void setApiKey(const QString& key) { m_apiKey = key; }
    
    QStringList languages() const { return m_languages; }
    void setLanguages(const QStringList& langs) { m_languages = langs; }
    
    QStringList videoExtensions() const { return m_videoExtensions; }
    void setVideoExtensions(const QStringList& exts) { m_videoExtensions = exts; }
    
    int minScore() const { return m_minScore; }
    void setMinScore(int score) { m_minScore = score; }
    
    int timeout() const { return m_timeout; }
    void setTimeout(int secs) { m_timeout = secs; }
    
    int maxRetries() const { return m_maxRetries; }
    void setMaxRetries(int retries) { m_maxRetries = retries; }
    
    int batchDelay() const { return m_batchDelay; }
    void setBatchDelay(int secs) { m_batchDelay = secs; }
    
    qint64 minFileSize() const { return m_minFileSize; }
    void setMinFileSize(qint64 size) { m_minFileSize = size; }
    
    bool useHashMatching() const { return m_useHashMatching; }
    void setUseHashMatching(bool use) { m_useHashMatching = use; }
    
    QString workingDirectory() const { return m_workingDirectory; }
    void setWorkingDirectory(const QString& dir) { m_workingDirectory = dir; }
    
    /**
     * @brief 获取已处理记录文件路径
     * @return 文件路径
     */
    static QString processedLogPath() {
        return QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + 
               "/subtitle_processed.json";
    }
    
    /**
     * @brief 获取下载日志文件路径
     * @return 文件路径
     */
    static QString downloadLogPath() {
        return QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + 
               "/subtitle_download.log";
    }
    
    /**
     * @brief 获取失败记录文件路径
     * @return 文件路径
     */
    static QString failedLogPath() {
        return QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + 
               "/subtitle_failed.json";
    }
    
    /**
     * @brief 从 QSettings 加载配置
     * @param settings QSettings 对象
     */
    void loadFromSettings(QSettings* settings) {
        settings->beginGroup("Subtitle");
        m_apiKey = settings->value("ApiKey", "").toString();
        m_languages = settings->value("Languages", QStringList({"chi", "zh", "zh-tw"})).toStringList();
        m_videoExtensions = settings->value("VideoExtensions", 
            QStringList({".mp4", ".mkv", ".avi", ".mov", ".wmv", ".flv"})).toStringList();
        m_minScore = settings->value("MinScore", 0).toInt();
        m_timeout = settings->value("Timeout", 30).toInt();
        m_maxRetries = settings->value("MaxRetries", 2).toInt();
        m_batchDelay = settings->value("BatchDelay", 0).toInt();
        m_minFileSize = settings->value("MinFileSize", 10 * 1024 * 1024).toLongLong();
        m_useHashMatching = settings->value("UseHashMatching", false).toBool();
        m_workingDirectory = settings->value("WorkingDirectory", "").toString();
        settings->endGroup();
        
        settings->beginGroup("Window");
        m_windowGeometry = settings->value("Geometry").toByteArray();
        m_windowState = settings->value("State").toByteArray();
        settings->endGroup();
    }
    
    /**
     * @brief 保存配置到 QSettings
     * @param settings QSettings 对象
     */
    void saveToSettings(QSettings* settings) {
        settings->beginGroup("Subtitle");
        settings->setValue("ApiKey", m_apiKey);
        settings->setValue("Languages", m_languages);
        settings->setValue("VideoExtensions", m_videoExtensions);
        settings->setValue("MinScore", m_minScore);
        settings->setValue("Timeout", m_timeout);
        settings->setValue("MaxRetries", m_maxRetries);
        settings->setValue("BatchDelay", m_batchDelay);
        settings->setValue("MinFileSize", m_minFileSize);
        settings->setValue("UseHashMatching", m_useHashMatching);
        settings->setValue("WorkingDirectory", m_workingDirectory);
        settings->endGroup();
        
        settings->beginGroup("Window");
        settings->setValue("Geometry", m_windowGeometry);
        settings->setValue("State", m_windowState);
        settings->endGroup();
    }
    
    /**
     * @brief 从 JSON 对象加载配置
     * @param obj JSON 对象
     */
    void loadFromJson(const QJsonObject& obj) {
        m_apiKey = obj["api_key"].toString();
        
        QJsonArray langArray = obj["languages"].toArray();
        m_languages.clear();
        for (const QJsonValue& val : langArray) {
            m_languages.append(val.toString());
        }
        
        QJsonArray extArray = obj["video_extensions"].toArray();
        m_videoExtensions.clear();
        for (const QJsonValue& val : extArray) {
            m_videoExtensions.append(val.toString());
        }
        
        m_minScore = obj["min_score"].toInt(0);
        m_timeout = obj["timeout"].toInt(30);
        m_maxRetries = obj["max_retries"].toInt(2);
        m_batchDelay = obj["batch_delay"].toInt(0);
        m_minFileSize = obj["min_file_size"].toVariant().toLongLong();
        m_useHashMatching = obj["use_hash_matching"].toBool(false);
        m_workingDirectory = obj["working_directory"].toString();
    }
    
    /**
     * @brief 转换为 JSON 对象
     * @return JSON 对象
     */
    QJsonObject toJson() const {
        QJsonObject obj;
        obj["api_key"] = m_apiKey;
        
        QJsonArray langArray;
        for (const QString& lang : m_languages) {
            langArray.append(lang);
        }
        obj["languages"] = langArray;
        
        QJsonArray extArray;
        for (const QString& ext : m_videoExtensions) {
            extArray.append(ext);
        }
        obj["video_extensions"] = extArray;
        
        obj["min_score"] = m_minScore;
        obj["timeout"] = m_timeout;
        obj["max_retries"] = m_maxRetries;
        obj["batch_delay"] = m_batchDelay;
        obj["min_file_size"] = m_minFileSize;
        obj["use_hash_matching"] = m_useHashMatching;
        obj["working_directory"] = m_workingDirectory;
        
        return obj;
    }
    
    /**
     * @brief 窗口几何形状
     */
    QByteArray windowGeometry() const { return m_windowGeometry; }
    void setWindowGeometry(const QByteArray& geom) { m_windowGeometry = geom; }
    
    /**
     * @brief 窗口状态
     */
    QByteArray windowState() const { return m_windowState; }
    void setWindowState(const QByteArray& state) { m_windowState = state; }

private:
    QString m_apiKey;              ///< API Token
    QStringList m_languages;       ///< 目标语言列表
    QStringList m_videoExtensions; ///< 视频文件扩展名
    int m_minScore;                ///< 最小匹配分数
    int m_timeout;                 ///< 请求超时（秒）
    int m_maxRetries;              ///< 最大重试次数
    int m_batchDelay;              ///< 批量处理延迟（秒）
    qint64 m_minFileSize;          ///< 最小文件大小
    bool m_useHashMatching;        ///< 是否使用哈希匹配
    QString m_workingDirectory;    ///< 工作目录
    
    // 窗口状态（不序列化）
    QByteArray m_windowGeometry;   ///< 窗口几何形状
    QByteArray m_windowState;      ///< 窗口状态
};

#endif // APPCONFIG_H

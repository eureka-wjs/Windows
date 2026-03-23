#ifndef CONFIGMANAGER_H
#define CONFIGMANAGER_H

#include <QObject>
#include <QSettings>
#include <QStandardPaths>
#include <QDir>
#include "src/models/AppConfig.h"

/**
 * @brief 配置管理类
 * 
 * 管理应用配置的加载和保存
 */
class ConfigManager : public QObject
{
    Q_OBJECT

public:
    explicit ConfigManager(QObject* parent = nullptr)
        : QObject(parent)
        , m_settings(nullptr)
    {
        // 初始化 QSettings
        QCoreApplication::setOrganizationName("AutoDownloadSubtitle");
        QCoreApplication::setApplicationName("AutoDownloadSubtitle");
        
        m_settings = new QSettings(this);
        
        // 确保目录存在
        QString dataPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
        QDir dir(dataPath);
        if (!dir.exists()) {
            dir.mkpath(".");
        }
    }
    
    ~ConfigManager() {
        if (m_settings) {
            m_settings->sync();
        }
    }
    
    /**
     * @brief 加载配置
     * @return 应用配置对象
     */
    AppConfig loadConfig() {
        AppConfig config;
        if (m_settings) {
            config.loadFromSettings(m_settings);
        }
        return config;
    }
    
    /**
     * @brief 保存配置
     * @param config 应用配置对象
     */
    void saveConfig(const AppConfig& config) {
        if (m_settings) {
            config.saveToSettings(m_settings);
            m_settings->sync();
        }
    }
    
    /**
     * @brief 获取 API Key
     * @return API Key
     */
    QString apiKey() const {
        if (!m_settings) return QString();
        return m_settings->value("Subtitle/ApiKey", "").toString();
    }
    
    /**
     * @brief 设置 API Key
     * @param key API Key
     */
    void setApiKey(const QString& key) {
        if (m_settings) {
            m_settings->setValue("Subtitle/ApiKey", key);
            m_settings->sync();
        }
    }
    
    /**
     * @brief 获取工作目录
     * @return 工作目录
     */
    QString workingDirectory() const {
        if (!m_settings) return QString();
        return m_settings->value("Subtitle/WorkingDirectory", "").toString();
    }
    
    /**
     * @brief 设置工作目录
     * @param dir 工作目录
     */
    void setWorkingDirectory(const QString& dir) {
        if (m_settings) {
            m_settings->setValue("Subtitle/WorkingDirectory", dir);
            m_settings->sync();
        }
    }
    
    /**
     * @brief 获取是否使用哈希匹配
     * @return 是否使用哈希
     */
    bool useHashMatching() const {
        if (!m_settings) return false;
        return m_settings->value("Subtitle/UseHashMatching", false).toBool();
    }
    
    /**
     * @brief 设置是否使用哈希匹配
     * @param use 是否使用
     */
    void setUseHashMatching(bool use) {
        if (m_settings) {
            m_settings->setValue("Subtitle/UseHashMatching", use);
            m_settings->sync();
        }
    }
    
    /**
     * @brief 获取窗口几何形状
     * @return 窗口几何形状
     */
    QByteArray windowGeometry() const {
        if (!m_settings) return QByteArray();
        return m_settings->value("Window/Geometry").toByteArray();
    }
    
    /**
     * @brief 设置窗口几何形状
     * @param geom 窗口几何形状
     */
    void setWindowGeometry(const QByteArray& geom) {
        if (m_settings) {
            m_settings->setValue("Window/Geometry", geom);
            m_settings->sync();
        }
    }
    
    /**
     * @brief 获取窗口状态
     * @return 窗口状态
     */
    QByteArray windowState() const {
        if (!m_settings) return QByteArray();
        return m_settings->value("Window/State").toByteArray();
    }
    
    /**
     * @brief 设置窗口状态
     * @param state 窗口状态
     */
    void setWindowState(const QByteArray& state) {
        if (m_settings) {
            m_settings->setValue("Window/State", state);
            m_settings->sync();
        }
    }
    
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

private:
    QSettings* m_settings;    ///< QSettings 对象
};

#endif // CONFIGMANAGER_H

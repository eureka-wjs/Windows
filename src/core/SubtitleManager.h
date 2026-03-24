#ifndef SUBTITLEMANAGER_H
#define SUBTITLEMANAGER_H

#include <QObject>
#include <QList>
#include <QMap>
#include <QTimer>
#include "core/AssrtAPI.h"
#include "core/HistoryManager.h"
#include "core/ConfigManager.h"
#include "models/VideoFile.h"
#include "models/SubtitleInfo.h"
#include "models/AppConfig.h"
#include "utils/NameParser.h"
#include "utils/FileUtils.h"
#include "utils/Logger.h"

/**
 * @brief 扫描结果结构体
 */
struct ScanResult {
    int totalScanned = 0;       ///< 总扫描文件数
    int success = 0;            ///< 成功数量
    int failed = 0;             ///< 失败数量
    int skipped = 0;            ///< 跳过数量
    int filtered = 0;           ///< 过滤数量
    int apiCallsSaved = 0;      ///< 节省 API 调用次数
    QList<FailedRecord> failedFiles;  ///< 失败文件列表
    QDateTime startTime;        ///< 开始时间
    QDateTime endTime;          ///< 结束时间
    QString folder;             ///< 扫描目录
};

/**
 * @brief 字幕管理类
 * 
 * 协调 API、历史记录和工具类，完成字幕下载任务
 */
class SubtitleManager : public QObject
{
    Q_OBJECT

public:
    explicit SubtitleManager(ConfigManager* configManager, QObject* parent = nullptr)
        : QObject(parent)
        , m_configManager(configManager)
        , m_api(nullptr)
        , m_history(nullptr)
        , m_logger(nullptr)
        , m_stopRequested(false)
    {
        m_config = m_configManager->loadConfig();
    }
    
    /**
     * @brief 初始化
     */
    bool initialize() {
        // 加载配置
        m_config = m_configManager->loadConfig();
        
        // 检查 API Key
        if (m_config.apiKey().isEmpty()) {
            return false;
        }
        
        // 创建日志对象
        m_logger = new Logger(ConfigManager::downloadLogPath(), this);
        
        // 连接 Logger 的 logMessage 信号到 SubtitleManager 的 logMessage 信号
        connect(m_logger, &Logger::logMessage, this, &SubtitleManager::logMessage);
        
        // 创建历史记录管理器
        m_history = new HistoryManager(
            ConfigManager::processedLogPath(),
            ConfigManager::failedLogPath(),
            this
        );
        // 设置 HistoryManager 的 logger
        m_history->setLogger(m_logger);
        
        // 创建 API 对象
        m_api = new AssrtAPI(this);
        
        // 初始化 API
        if (!m_api->initialize(m_config.apiKey(), m_logger)) {
            return false;
        }
        
        return true;
    }
    
    /**
     * @brief 处理单个视频文件
     * @param videoPath 视频文件路径
     * @param force 强制重新下载
     * @return 是否成功
     */
    bool processVideo(const QString& videoPath, bool force = false);
    
    /**
     * @brief 扫描文件夹并批量处理
     * @param folderPath 文件夹路径
     * @param force 强制重新下载
     * @return 扫描结果
     */
    ScanResult scanFolder(const QString& folderPath, bool force = false);
    
    /**
     * @brief 重新加载配置
     */
    void reloadConfig() {
        m_config = m_configManager->loadConfig();
        // 【诊断日志】记录 reloadConfig 后的调试模式值
        if (m_logger) {
            m_logger->debug(QString("[诊断] reloadConfig 完成：debugMode=%1")
                .arg(m_config.debugMode() ? "true" : "false"));
        }
    }
    
    /**
     * @brief 停止处理
     */
    void stop() {
        m_stopRequested = true;
    }
    
    /**
     * @brief 是否正在处理
     * @return 是否正在处理
     */
    bool isProcessing() const { return !m_stopRequested; }
    
    /**
     * @brief 获取日志对象
     * @return 日志对象
     */
    Logger* logger() const { return m_logger; }
    
    /**
     * @brief 获取 API 对象
     * @return API 对象
     */
    AssrtAPI* api() const { return m_api; }
    
    /**
     * @brief 获取历史记录管理器
     * @return 历史记录管理器
     */
    HistoryManager* history() const { return m_history; }

signals:
    /**
     * @brief 进度更新信号
     * @param current 当前文件索引
     * @param total 总文件数
     * @param filePath 当前处理的文件路径
     */
    void progressUpdated(int current, int total, const QString& filePath);
    
    /**
     * @brief 统计更新信号
     * @param result 扫描结果
     */
    void statsUpdated(const ScanResult& result);
    
    /**
     * @brief 日志信号
     * @param level 日志级别
     * @param message 日志消息
     */
    void logMessage(Logger::Level level, const QString& message);
    
    /**
     * @brief 处理完成信号
     * @param result 扫描结果
     */
    void processingComplete(const ScanResult& result);

private:
    /**
     * @brief 下载单个视频的字幕（多策略搜索）
     * @param video 视频文件
     * @param force 强制重新下载
     * @return 是否成功
     */
    bool downloadSubtitle(const VideoFile& video, bool force = false);
    
    /**
     * @brief 选择最佳字幕
     * @param subs 字幕列表
     * @return 最佳字幕
     */
    SubtitleInfo selectBestSubtitle(const QList<SubtitleInfo>& subs);
    
    /**
     * @brief 过滤中文字幕
     * @param subs 字幕列表
     * @return 中文字幕列表
     */
    QList<SubtitleInfo> filterChineseSubs(const QList<SubtitleInfo>& subs);

private:
    ConfigManager* m_configManager;     ///< 配置管理器
    AssrtAPI* m_api;                    ///< API 客户端
    HistoryManager* m_history;          ///< 历史记录管理器
    Logger* m_logger;                   ///< 日志对象
    AppConfig m_config;                 ///< 当前配置
    
    bool m_stopRequested;               ///< 是否请求停止
};

#endif // SUBTITLEMANAGER_H

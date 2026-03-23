#ifndef ASSRTAPI_H
#define ASSRTAPI_H

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QEventLoop>
#include <QTimer>
#include <QMutex>
#include "models/SubtitleInfo.h"
#include "utils/Logger.h"

/**
 * @brief 射手网 API 客户端
 * 
 * 封装射手网字幕 API 的所有操作
 */
class AssrtAPI : public QObject
{
    Q_OBJECT

public:
    explicit AssrtAPI(QObject* parent = nullptr)
        : QObject(parent)
        , m_networkManager(nullptr)
        , m_quota(0)
        , m_initialized(false)
    {}
    
    /**
     * @brief 初始化 API
     * @param token API Token
     * @param logger 日志对象
     * @return 是否成功
     */
    bool initialize(const QString& token, Logger* logger);
    
    /**
     * @brief 是否已初始化
     * @return 是否已初始化
     */
    bool isInitialized() const { return m_initialized; }
    
    /**
     * @brief 获取当前配额
     * @return 配额数量
     */
    int quota() const { return m_quota; }
    
    /**
     * @brief 搜索字幕
     * @param query 搜索关键词
     * @param isFile 是否为文件名搜索
     * @return 字幕列表
     */
    QList<SubtitleInfo> search(const QString& query, bool isFile = true);
    
    /**
     * @brief 获取字幕详情
     * @param subId 字幕 ID
     * @return 字幕信息
     */
    SubtitleInfo getDetail(int subId);
    
    /**
     * @brief 下载字幕
     * @param subId 字幕 ID
     * @param savePath 保存路径
     * @param videoPath 视频文件路径
     * @return 是否成功
     */
    bool download(int subId, const QString& savePath, const QString& videoPath);
    
    /**
     * @brief 验证 Token
     * @return 是否有效
     */
    bool verifyToken();
    
    /**
     * @brief 检查配额
     * @param skipLog 是否跳过日志
     * @return 是否有配额
     */
    bool checkQuota(bool skipLog = false);

signals:
    /**
     * @brief 配额变化信号
     * @param quota 新配额
     */
    void quotaChanged(int quota);
    
    /**
     * @brief 进度信号
     * @param current 当前进度
     * @param total 总进度
     */
    void progress(int current, int total);

private:
   /**
    * @brief 等待重试
    * @param seconds 等待秒数
    */
   void waitAndRetry(int seconds = 60);
    
    /**
     * @brief 发送 GET 请求
     * @param url 请求 URL
     * @param params 请求参数
     * @return 响应 JSON
     */
    QJsonDocument sendGetRequest(const QString& url, const QMap<QString, QString>& params);

private:
    QString m_token;                          ///< API Token
    QString m_apiUrl;                         ///< API 基础 URL
    QNetworkAccessManager* m_networkManager;  ///< 网络管理器
    Logger* m_logger;                         ///< 日志对象
    int m_quota;                              ///< 当前配额
    bool m_initialized;                       ///< 是否已初始化
    QMutex m_mutex;                           ///< 互斥锁
    
    static const int MAX_RETRIES = 2;         ///< 最大重试次数
    static const int TIMEOUT = 30;            ///< 超时时间（秒）
};

#endif // ASSRTAPI_H

#ifndef HISTORYMANAGER_H
#define HISTORYMANAGER_H

#include <QObject>
#include <QString>
#include <QList>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>
#include <QFile>
#include <QDir>
#include <QDateTime>
#include <QMutex>
#include "models/VideoFile.h"
#include "utils/Logger.h"

/**
 * @brief 失败记录结构体
 */
struct FailedRecord {
    QString filePath;
    QString reason;
    QDateTime failedTime;
    
    QJsonObject toJson() const {
        QJsonObject obj;
        obj["video_path"] = filePath;
        obj["reason"] = reason;
        obj["failed_time"] = failedTime.toString(Qt::ISODate);
        return obj;
    }
    
    static FailedRecord fromJson(const QJsonObject& obj) {
        FailedRecord record;
        record.filePath = obj["video_path"].toString();
        record.reason = obj["reason"].toString();
        record.failedTime = QDateTime::fromString(obj["failed_time"].toString(), Qt::ISODate);
        return record;
    }
};

/**
 * @brief 已处理记录结构体
 */
struct ProcessedRecord {
    QString videoPath;
    QString subtitlePath;
    QDateTime processedTime;
    
    QJsonObject toJson() const {
        QJsonObject obj;
        obj["video_path"] = videoPath;
        obj["subtitle_path"] = subtitlePath;
        obj["processed_time"] = processedTime.toString(Qt::ISODate);
        return obj;
    }
    
    static ProcessedRecord fromJson(const QJsonObject& obj) {
        ProcessedRecord record;
        record.videoPath = obj["video_path"].toString();
        record.subtitlePath = obj["subtitle_path"].toString();
        record.processedTime = QDateTime::fromString(obj["processed_time"].toString(), Qt::ISODate);
        return record;
    }
};

/**
 * @brief 历史记录管理类
 * 
 * 管理已处理记录和失败记录
 */
class HistoryManager : public QObject
{
    Q_OBJECT

public:
    explicit HistoryManager(const QString& processedLogPath,
                           const QString& failedLogPath,
                           QObject* parent = nullptr)
        : QObject(parent)
        , m_processedLogPath(processedLogPath)
        , m_failedLogPath(failedLogPath)
        , m_logger(nullptr)
    {
        // 确保目录存在
        ensureDirectory(m_processedLogPath);
        ensureDirectory(m_failedLogPath);
    }
    
    /**
     * @brief 设置日志记录器
     * @param logger 日志记录器指针
     */
    void setLogger(Logger* logger) {
        m_logger = logger;
    }
    
    /**
     * @brief 检查文件是否已处理
     * @param videoPath 视频文件路径
     * @param useHash 是否使用哈希匹配
     * @return 是否已处理
     */
    bool isProcessed(const QString& videoPath, bool useHash = false) {
        QMutexLocker locker(&m_mutex);
        loadProcessedData();
        
        QString fileKey = getFileKey(videoPath, useHash);
        if (fileKey.isEmpty()) {
            return false;
        }
        
        return m_processedData.contains(fileKey);
    }
    
    /**
     * @brief 标记文件已处理
     * @param videoPath 视频文件路径
     * @param subtitlePath 字幕文件路径
     * @param useHash 是否使用哈希
     */
    void markProcessed(const QString& videoPath, const QString& subtitlePath, bool useHash = false) {
        QMutexLocker locker(&m_mutex);
        loadProcessedData();
        
        QString fileKey = getFileKey(videoPath, useHash);
        if (fileKey.isEmpty()) {
            return;
        }
        
        ProcessedRecord record;
        record.videoPath = videoPath;
        record.subtitlePath = subtitlePath;
        record.processedTime = QDateTime::currentDateTime();
        
        m_processedData[fileKey] = record.toJson();
        saveProcessedData();
    }
    
    /**
     * @brief 标记文件处理失败
     * @param videoPath 视频文件路径
     * @param reason 失败原因
     */
    void markFailed(const QString& videoPath, const QString& reason) {
        QMutexLocker locker(&m_mutex);
        loadFailedData();
        
        FailedRecord record;
        record.filePath = videoPath;
        record.reason = reason;
        record.failedTime = QDateTime::currentDateTime();
        
        m_failedData.append(record.toJson());
        saveFailedData();
    }
    
    /**
     * @brief 获取所有失败记录
     * @return 失败记录列表
     */
    QList<FailedRecord> getFailedRecords() {
        QMutexLocker locker(&m_mutex);
        loadFailedData();
        
        QList<FailedRecord> records;
        for (const QJsonValue& val : m_failedData) {
            records.append(FailedRecord::fromJson(val.toObject()));
        }
        return records;
    }
    
    /**
     * @brief 清除已处理记录
     */
    void clearProcessed() {
        QMutexLocker locker(&m_mutex);
        QFile file(m_processedLogPath);
        if (file.exists()) {
            file.remove();
        }
        m_processedData.clear();
    }
    
    /**
     * @brief 清除失败记录
     */
    void clearFailed() {
        QMutexLocker locker(&m_mutex);
        QFile file(m_failedLogPath);
        if (file.exists()) {
            file.remove();
        }
        m_failedData = QJsonArray();
    }
    
    /**
     * @brief 获取已处理记录数量
     * @return 记录数量
     */
    int processedCount() {
        QMutexLocker locker(&m_mutex);
        loadProcessedData();
        return m_processedData.size();
    }
    
    /**
     * @brief 获取失败记录数量
     * @return 记录数量
     */
    int failedCount() {
        QMutexLocker locker(&m_mutex);
        loadFailedData();
        return m_failedData.size();
    }

private:
    /**
     * @brief 获取文件键（哈希或文件名）
     */
    QString getFileKey(const QString& videoPath, bool useHash) {
        if (useHash) {
            VideoFile video(videoPath);
            return video.calculateHash(false);
        } else {
            QFileInfo fileInfo(videoPath);
            return fileInfo.fileName();
        }
    }
    
    /**
     * @brief 确保目录存在
     */
    void ensureDirectory(const QString& filePath) {
        QFileInfo fileInfo(filePath);
        QDir dir(fileInfo.absolutePath());
        if (!dir.exists()) {
            dir.mkpath(".");
        }
    }
    
    /**
     * @brief 加载已处理数据
     */
    void loadProcessedData() {
        if (m_processedDataLoaded) {
            return;
        }
        
        qint64 startTime = QDateTime::currentMSecsSinceEpoch();
        
        QFile file(m_processedLogPath);
        if (!file.exists()) {
            m_processedDataLoaded = true;
            return;
        }
        
        if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QByteArray data = file.readAll();
            QJsonDocument doc = QJsonDocument::fromJson(data);
            if (!doc.isNull()) {
                m_processedData = doc.object().toVariantMap();
            }
            file.close();
        }
        m_processedDataLoaded = true;
        
        qint64 elapsed = QDateTime::currentMSecsSinceEpoch() - startTime;
        if (m_logger) {
            m_logger->debug(QString("[HistoryManager] 加载已处理数据完成，耗时 %1ms").arg(elapsed));
        }
    }
    
    /**
     * @brief 保存已处理数据
     */
    void saveProcessedData() {
        qint64 startTime = QDateTime::currentMSecsSinceEpoch();
        
        QFile file(m_processedLogPath);
        if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QJsonDocument doc(QJsonObject::fromVariantMap(m_processedData));
            file.write(doc.toJson(QJsonDocument::Indented));
            file.close();
            
            qint64 elapsed = QDateTime::currentMSecsSinceEpoch() - startTime;
            if (m_logger) {
                m_logger->debug(QString("[HistoryManager] 保存已处理数据完成，耗时 %1ms，数据大小：%2 字节").arg(elapsed).arg(file.size()));
            }
        }
    }
    
    /**
     * @brief 加载失败数据
     */
    void loadFailedData() {
        if (m_failedDataLoaded) {
            return;
        }
        
        qint64 startTime = QDateTime::currentMSecsSinceEpoch();
        
        QFile file(m_failedLogPath);
        if (!file.exists()) {
            m_failedDataLoaded = true;
            return;
        }
        
        if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QByteArray data = file.readAll();
            QJsonDocument doc = QJsonDocument::fromJson(data);
            if (!doc.isNull() && doc.isArray()) {
                m_failedData = doc.array();
            }
            file.close();
        }
        m_failedDataLoaded = true;
        
        qint64 elapsed = QDateTime::currentMSecsSinceEpoch() - startTime;
        if (m_logger) {
            m_logger->debug(QString("[HistoryManager] 加载失败数据完成，耗时 %1ms").arg(elapsed));
        }
    }
    
    /**
     * @brief 保存失败数据
     */
    void saveFailedData() {
        qint64 startTime = QDateTime::currentMSecsSinceEpoch();
        
        QFile file(m_failedLogPath);
        if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QJsonDocument doc(m_failedData);
            file.write(doc.toJson(QJsonDocument::Indented));
            file.close();
            
            qint64 elapsed = QDateTime::currentMSecsSinceEpoch() - startTime;
            if (m_logger) {
                m_logger->debug(QString("[HistoryManager] 保存失败数据完成，耗时 %1ms").arg(elapsed));
            }
        }
    }

private:
    QString m_processedLogPath;     ///< 已处理记录文件路径
    QString m_failedLogPath;        ///< 失败记录文件路径
    
    QVariantMap m_processedData;    ///< 已处理数据
    QJsonArray m_failedData;        ///< 失败数据
    
    bool m_processedDataLoaded = false;  ///< 已处理数据是否已加载
    bool m_failedDataLoaded = false;     ///< 失败数据是否已加载
    
    QMutex m_mutex;                 ///< 互斥锁
    Logger* m_logger;               ///< 日志记录器指针
};

#endif // HISTORYMANAGER_H

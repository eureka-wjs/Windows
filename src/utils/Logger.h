#ifndef LOGGER_H
#define LOGGER_H

#include <QObject>
#include <QString>
#include <QStringList>
#include <QFile>
#include <QTextStream>
#include <QDateTime>
#include <QMutex>
#include <QDir>
#include <QStringConverter>

/**
 * @brief 日志工具类
 * 
 * 支持分级日志输出到文件和控制台
 * 
 * 日志级别：
 * - INFO    : 一般信息
 * - SUCCESS : 成功信息
 * - WARNING : 警告信息
 * - ERROR   : 错误信息
 * - DEBUG   : 调试信息
 */
class Logger : public QObject
{
    Q_OBJECT

public:
    enum Level {
        Debug,
        Info,
        Success,
        Warning,
        Error
    };
    Q_ENUM(Level)

    explicit Logger(const QString& logFile, QObject* parent = nullptr)
        : QObject(parent)
        , m_logFile(logFile)
    {
        // 确保日志目录存在
        QFileInfo fileInfo(m_logFile);
        QDir dir(fileInfo.absolutePath());
        if (!dir.exists()) {
            dir.mkpath(".");
        }
    }
    
    /**
     * @brief 记录日志（INFO 级别）
     */
    void log(const QString& message) {
        logLevel(Info, message);
    }
    
    /**
     * @brief 记录信息日志
     */
    void info(const QString& message) {
        logLevel(Info, message);
    }
    
    /**
     * @brief 记录调试日志
     */
    void debug(const QString& message) {
        logLevel(Debug, message);
    }
    
    /**
     * @brief 记录成功日志
     */
    void success(const QString& message) {
        logLevel(Success, message);
    }
    
    /**
     * @brief 记录警告日志
     */
    void warning(const QString& message) {
        logLevel(Warning, message);
    }
    
    /**
     * @brief 记录错误日志
     */
    void error(const QString& message) {
        logLevel(Error, message);
    }
    
    /**
     * @brief 清空日志文件
     */
    void clear() {
        QMutexLocker locker(&m_mutex);
        QFile file(m_logFile);
        if (file.exists()) {
            file.remove();
        }
    }

signals:
    /**
     * @brief 日志信号
     * @param level 日志级别
     * @param message 日志消息
     */
    void logMessage(Logger::Level level, const QString& message);

private:
    /**
     * @brief 记录指定级别的日志
     */
    void logLevel(Level level, const QString& message) {
        QMutexLocker locker(&m_mutex);
        
        QString timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss");
        QString levelStr = levelToString(level);
        QString logLine = QString("[%1] [%2] %3\n").arg(timestamp, levelStr, message);
        
        // 写入文件
        QFile file(m_logFile);
        if (file.open(QIODevice::Append | QIODevice::Text)) {
            QTextStream out(&file);
            out.setEncoding(QStringConverter::Utf8);
            out << logLine;
            file.close();
        }
        
        // 发射信号
        emit logMessage(level, message);
    }
    
    /**
     * @brief 将日志级别转换为字符串
     */
    QString levelToString(Level level) const {
        switch (level) {
            case Debug:   return "DEBUG";
            case Info:    return "INFO";
            case Success: return "SUCCESS";
            case Warning: return "WARNING";
            case Error:   return "ERROR";
            default:      return "UNKNOWN";
        }
    }

private:
    QString m_logFile;     ///< 日志文件路径
    QMutex m_mutex;        ///< 互斥锁
};

#endif // LOGGER_H

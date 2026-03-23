#include "AssrtAPI.h"
#include <QUrl>
#include <QUrlQuery>
#include <QThread>
#include <QCoreApplication>

const QString API_URL = "https://api.assrt.net/v1";

bool AssrtAPI::initialize(const QString& token, Logger* logger)
{
    QMutexLocker locker(&m_mutex);
    
    m_token = token;
    m_logger = logger;
    m_apiUrl = API_URL;
    
    if (m_networkManager == nullptr) {
        m_networkManager = new QNetworkAccessManager(this);
    }
    
    // 验证 Token
    if (verifyToken()) {
        m_initialized = true;
        return true;
    }
    
    return false;
}

bool AssrtAPI::verifyToken()
{
    if (m_token.isEmpty()) {
        if (m_logger) m_logger->error("Token 为空");
        return false;
    }
    
    // 使用重试机制验证 Token
    int maxAttempts = 5;
    for (int attempt = 1; attempt <= maxAttempts; ++attempt) {
        if (m_logger) {
            m_logger->info(QString("验证 Token... (%1/%2)").arg(attempt).arg(maxAttempts));
        }
        
        QMap<QString, QString> params;
        params["token"] = m_token;
        
        QJsonDocument doc = sendGetRequest(m_apiUrl + "/user/quota", params);
        
        if (!doc.isNull()) {
            QJsonObject obj = doc.object();
            int status = obj["status"].toInt(-1);
            
            if (status == 0) {
                // Token 有效
                QJsonObject user = obj["user"].toObject();
                m_quota = user["quota"].toInt(0);
                if (m_logger) {
                    m_logger->success(QString("Token 有效，剩余配额：%1/分钟").arg(m_quota));
                }
                emit quotaChanged(m_quota);
                return true;
            } else if (status == 30900) {
                // 配额已用尽
                if (m_logger) {
                    m_logger->warning("配额已用尽（30900），等待 60 秒后重试...");
                }
                waitAndRetry(60);
                continue;
            } else {
                if (m_logger) {
                    m_logger->error(QString("Token 验证失败：%1").arg(status));
                }
                return false;
            }
        }
        
        // 请求失败，等待重试
        if (attempt < maxAttempts) {
            waitAndRetry(60);
        }
    }
    
    if (m_logger) {
        m_logger->error("多次重试后 Token 验证仍然失败");
    }
    return false;
}

QList<SubtitleInfo> AssrtAPI::search(const QString& query, bool isFile)
{
    QList<SubtitleInfo> results;
    
    if (!m_initialized) {
        if (m_logger) m_logger->error("API 未初始化，跳过搜索");
        return results;
    }
    
    if (m_logger) {
        m_logger->info(QString("搜索：%1").arg(query));
    }
    
    for (int attempt = 0; attempt < MAX_RETRIES; ++attempt) {
        QMap<QString, QString> params;
        params["token"] = m_token;
        params["q"] = query;
        params["cnt"] = "15";
        params["pos"] = "0";
        if (isFile) {
            params["is_file"] = "1";
        }
        
        QJsonDocument doc = sendGetRequest(m_apiUrl + "/sub/search", params);
        
        if (!doc.isNull()) {
            QJsonObject obj = doc.object();
            int status = obj["status"].toInt(-1);
            
            if (status == 0) {
                // 搜索成功
                QJsonObject subObj = obj["sub"].toObject();
                QJsonArray subs = subObj["subs"].toArray();
                
                for (const QJsonValue& val : subs) {
                    results.append(SubtitleInfo::fromJson(val.toObject()));
                }
                
                if (m_logger) {
                    m_logger->success(QString("搜索到 %1 个字幕").arg(results.size()));
                }
                return results;
            } else if (status == 30900) {
                // 配额已用尽
                if (m_logger) {
                    m_logger->warning("配额已用尽（30900），等待 60 秒后重试...");
                }
                waitAndRetry(60);
                continue;
            }
        }
        
        // 请求失败
        if (attempt < MAX_RETRIES - 1) {
            waitAndRetry(60);
        }
    }
    
    if (m_logger) {
        m_logger->error("搜索失败（已达最大重试次数）");
    }
    return results;
}

SubtitleInfo AssrtAPI::getDetail(int subId)
{
    SubtitleInfo result;
    
    if (!m_initialized) {
        return result;
    }
    
    QMap<QString, QString> params;
    params["token"] = m_token;
    params["id"] = QString::number(subId);
    
    QJsonDocument doc = sendGetRequest(m_apiUrl + "/sub/detail", params);
    
    if (!doc.isNull()) {
        QJsonObject obj = doc.object();
        if (obj["status"].toInt(-1) == 0) {
            QJsonObject subObj = obj["sub"].toObject();
            QJsonArray subs = subObj["subs"].toArray();
            if (!subs.isEmpty()) {
                result = SubtitleInfo::fromJson(subs[0].toObject());
            }
        }
    }
    
    return result;
}

bool AssrtAPI::download(int subId, const QString& savePath, const QString& videoPath)
{
    Q_UNUSED(videoPath);
    
    if (!m_initialized) {
        if (m_logger) m_logger->error("API 未初始化");
        return false;
    }
    
    if (m_logger) {
        m_logger->info("开始下载字幕...");
    }
    
    // 检查配额
    if (!checkQuota(true)) {
        if (m_logger) {
            m_logger->warning("API 配额检查失败，尝试恢复...");
        }
        if (!checkQuota()) {
            return false;
        }
    }
    
    int consecutive493 = 0;
    
    for (int attempt = 0; attempt < MAX_RETRIES; ++attempt) {
        // 获取详情
        SubtitleInfo sub = getDetail(subId);
        if (sub.downloadUrl().isEmpty()) {
            if (m_logger) {
                m_logger->error("无法获取字幕详情或下载地址");
            }
            if (attempt < MAX_RETRIES - 1) {
                waitAndRetry(60);
                continue;
            }
            return false;
        }
        
        // 下载文件
        if (m_logger) {
            m_logger->info("下载中...");
        }
        
        QUrl url(sub.downloadUrl());
        QNetworkRequest request(url);
        request.setHeader(QNetworkRequest::UserAgentHeader, 
            "Mozilla/5.0 (Macintosh; Intel Mac OS X 10_15_7) AppleWebKit/537.36");
        
        QNetworkReply* reply = m_networkManager->get(request);
        
        // 等待下载完成
        QEventLoop loop;
        QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
        QTimer::timeout([&]() {
            reply->abort();
            loop.quit();
        }, TIMEOUT * 1000);
        loop.exec();
        
        if (reply->error() != QNetworkReply::NoError) {
            int httpStatus = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
            
            if (m_logger) {
                m_logger->error(QString("下载失败：%1").arg(reply->errorString()));
            }
            
            // 493 错误特殊处理
            if (httpStatus == 493) {
                consecutive493++;
                if (m_logger) {
                    m_logger->warning(QString("493 错误（连续第 %1 次）").arg(consecutive493));
                }
                
                if (consecutive493 >= 2) {
                    if (m_logger) {
                        m_logger->warning("连续 2 次 493 错误，判断为下载配额不足，等待 120 秒...");
                    }
                    if (attempt < MAX_RETRIES - 1) {
                        waitAndRetry(120);
                        continue;
                    }
                } else {
                    if (attempt < MAX_RETRIES - 1) {
                        waitAndRetry(60);
                        continue;
                    }
                }
            } else if (httpStatus >= 500) {
                // 服务器错误，重试
                if (attempt < MAX_RETRIES - 1) {
                    waitAndRetry(60);
                    continue;
                }
            }
            
            reply->deleteLater();
            return false;
        }
        
        // 保存文件
        QByteArray data = reply->readAll();
        reply->deleteLater();
        
        // 确定文件扩展名
        QString ext = ".srt";
        QString fileName = sub.fileName().toLower();
        if (fileName.contains(".ass")) {
            ext = ".ass";
        } else if (fileName.contains(".ssa")) {
            ext = ".ssa";
        } else if (fileName.contains(".vtt")) {
            ext = ".vtt";
        }
        
        QString finalPath = savePath;
        if (!finalPath.endsWith(ext)) {
            finalPath = savePath + ext;
        }
        
        QFile file(finalPath);
        if (file.open(QIODevice::WriteOnly)) {
            file.write(data);
            file.close();
            
            if (m_logger) {
                m_logger->success(QString("字幕已保存：%1").arg(finalPath));
            }
            return true;
        } else {
            if (m_logger) {
                m_logger->error(QString("无法保存文件：%1").arg(finalPath));
            }
            return false;
        }
    }
    
    return false;
}

bool AssrtAPI::checkQuota(bool skipLog)
{
    if (!m_initialized) {
        if (!skipLog && m_logger) {
            m_logger->error("配额检查失败：API 未初始化");
        }
        return false;
    }
    
    QMap<QString, QString> params;
    params["token"] = m_token;
    
    QJsonDocument doc = sendGetRequest(m_apiUrl + "/user/quota", params);
    
    if (!doc.isNull()) {
        QJsonObject obj = doc.object();
        int status = obj["status"].toInt(-1);
        
        if (status == 30900) {
            if (!skipLog && m_logger) {
                m_logger->warning("配额已用尽（30900），等待 60 秒后重试...");
            }
            waitAndRetry(60);
            return checkQuota(skipLog);
        }
        
        if (status == 0) {
            QJsonObject user = obj["user"].toObject();
            m_quota = user["quota"].toInt(0);
            if (!skipLog && m_logger) {
                m_logger->info(QString("当前配额：%1 次/分钟").arg(m_quota));
            }
            emit quotaChanged(m_quota);
            
            if (m_quota <= 0) {
                if (!skipLog && m_logger) {
                    m_logger->warning("配额已用尽，等待 60 秒后重试...");
                }
                waitAndRetry(60);
                return checkQuota(skipLog);
            }
            return true;
        }
    }
    
    return false;
}

void AssrtAPI::waitAndRetry(int seconds)
{
    if (m_logger) {
        m_logger->log(QString("开始等待 %1 秒...").arg(seconds));
    }
    
    for (int i = seconds; i > 0; --i) {
        QCoreApplication::processEvents();
        QThread::msleep(1000);
    }
    
    if (m_logger) {
        m_logger->log("等待完成");
    }
}

QJsonDocument AssrtAPI::sendGetRequest(const QString& url, const QMap<QString, QString>& params)
{
    QUrl fullUrl(url);
    QUrlQuery query;
    
    for (auto it = params.begin(); it != params.end(); ++it) {
        query.addQueryItem(it.key(), it.value());
    }
    fullUrl.setQuery(query);
    
    QNetworkRequest request(fullUrl);
    request.setHeader(QNetworkRequest::UserAgentHeader,
        "Mozilla/5.0 (Macintosh; Intel Mac OS X 10_15_7) AppleWebKit/537.36");
    
    QNetworkReply* reply = m_networkManager->get(request);
    
    // 等待响应
    QEventLoop loop;
    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    QTimer::timeout([&]() {
        reply->abort();
        loop.quit();
    }, TIMEOUT * 1000);
    loop.exec();
    
    QJsonDocument result;
    
    if (reply->error() == QNetworkReply::NoError) {
        QByteArray data = reply->readAll();
        result = QJsonDocument::fromJson(data);
    }
    
    reply->deleteLater();
    return result;
}

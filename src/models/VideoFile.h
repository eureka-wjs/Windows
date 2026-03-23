#ifndef VIDEOFILE_H
#define VIDEOFILE_H

#include <QString>
#include <QFileInfo>
#include <QCryptographicHash>
#include <QFile>
#include <QIODevice>
#include <QJsonObject>

/**
 * @brief 视频文件数据模型
 * 
 * 封装视频文件的基本信息和操作
 */
class VideoFile
{
public:
    VideoFile() : m_size(0) {}
    
    explicit VideoFile(const QString& path) 
        : m_path(path)
    {
        QFileInfo fileInfo(path);
        m_name = fileInfo.fileName();
        m_folder = fileInfo.absolutePath();
        m_size = fileInfo.size();
    }
    
    // Getter 和 Setter
    QString path() const { return m_path; }
    void setPath(const QString& path) { 
        m_path = path; 
        QFileInfo fileInfo(path);
        m_name = fileInfo.fileName();
        m_folder = fileInfo.absolutePath();
        m_size = fileInfo.size();
    }
    
    QString name() const { return m_name; }
    QString folder() const { return m_folder; }
    qint64 size() const { return m_size; }
    
    /**
     * @brief 获取不带扩展名的文件名
     * @return 文件名（不含扩展名）
     */
    QString baseName() const {
        QFileInfo fileInfo(m_path);
        return fileInfo.completeBaseName();
    }
    
    /**
     * @brief 计算文件 MD5 哈希
     * @param quick 快速模式，只读取前 1MB
     * @return MD5 哈希字符串
     */
    QString calculateHash(bool quick = false) const {
        QFile file(m_path);
        if (!file.open(QIODevice::ReadOnly)) {
            return QString();
        }
        
        QCryptographicHash hash(QCryptographicHash::Md5);
        
        if (quick) {
            // 快速模式：只读取前 1MB
            QByteArray chunk = file.read(1024 * 1024);
            hash.addData(chunk);
        } else {
            // 完整模式：读取整个文件
            while (!file.atEnd()) {
                QByteArray chunk = file.read(4096);
                hash.addData(chunk);
            }
        }
        
        file.close();
        return hash.result().toHex();
    }
    
    /**
     * @brief 检查是否为有效的视频文件
     * @param extensions 视频扩展名列表
     * @param minSize 最小文件大小
     * @return 是否为有效视频文件
     */
    bool isVideoFile(const QStringList& extensions, qint64 minSize = 10 * 1024 * 1024) const {
        // 检查隐藏文件
        if (m_name.startsWith(".") || m_name.startsWith("._")) {
            return false;
        }
        
        // 检查扩展名
        bool hasValidExtension = false;
        for (const QString& ext : extensions) {
            if (m_name.toLower().endsWith(ext)) {
                hasValidExtension = true;
                break;
            }
        }
        if (!hasValidExtension) {
            return false;
        }
        
        // 检查文件大小
        if (m_size < minSize) {
            return false;
        }
        
        return true;
    }
    
    /**
     * @brief 转换为 JSON 对象
     * @return JSON 对象
     */
    QJsonObject toJson() const {
        QJsonObject obj;
        obj["path"] = m_path;
        obj["name"] = m_name;
        obj["folder"] = m_folder;
        obj["size"] = m_size;
        return obj;
    }
    
    /**
     * @brief 从 JSON 对象创建
     * @param obj JSON 对象
     * @return VideoFile 对象
     */
    static VideoFile fromJson(const QJsonObject& obj) {
        VideoFile video;
        video.m_path = obj["path"].toString();
        video.m_name = obj["name"].toString();
        video.m_folder = obj["folder"].toString();
        video.m_size = obj["size"].toInt();
        return video;
    }

private:
    QString m_path;      ///< 文件完整路径
    QString m_name;      ///< 文件名（含扩展名）
    QString m_folder;    ///< 所在文件夹
    qint64 m_size;       ///< 文件大小（字节）
};

#endif // VIDEOFILE_H

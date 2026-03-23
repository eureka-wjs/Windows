#ifndef FILEUTILS_H
#define FILEUTILS_H

#include <QString>
#include <QStringList>
#include <QFileInfo>
#include <QDir>
#include <QDateTime>

/**
 * @brief 文件操作工具类
 * 
 * 提供文件查找、检查等实用功能
 */
class FileUtils
{
public:
    FileUtils() = default;
    
    /**
     * @brief 检查是否为有效的视频文件
     * 
     * @param filePath 文件路径
     * @param extensions 视频扩展名列表
     * @param minSize 最小文件大小（字节）
     * @return 是否为有效视频文件
     */
    static bool isVideoFile(const QString& filePath, 
                            const QStringList& extensions, 
                            qint64 minSize = 10 * 1024 * 1024) {
        QFileInfo fileInfo(filePath);
        QString fileName = fileInfo.fileName();
        
        // 检查隐藏文件
        if (fileName.startsWith(".") || fileName.startsWith("._")) {
            return false;
        }
        
        // 检查扩展名
        bool hasValidExtension = false;
        QString lowerFileName = fileName.toLower();
        for (const QString& ext : extensions) {
            if (lowerFileName.endsWith(ext)) {
                hasValidExtension = true;
                break;
            }
        }
        if (!hasValidExtension) {
            return false;
        }
        
        // 检查文件大小
        if (fileInfo.size() < minSize) {
            return false;
        }
        
        return true;
    }
    
    /**
     * @brief 查找视频同目录下已存在的字幕文件
     * 
     * @param videoPath 视频文件路径
     * @param subExtensions 字幕文件扩展名
     * @return 已存在的字幕文件路径列表
     */
    static QStringList findExistingSubtitles(const QString& videoPath,
                                             const QStringList& subExtensions = {".srt", ".ass", ".ssa", ".vtt"}) {
        QStringList existing;
        QFileInfo fileInfo(videoPath);
        QString folder = fileInfo.absolutePath();
        QString baseName = fileInfo.completeBaseName();
        
        // 可能的字幕文件名
        QStringList possibleNames = {
            baseName,
            baseName + ".zh-CN",
            baseName + ".zh-tw",
            baseName + ".zh",
            baseName + ".en",
            baseName + ".chi",
            baseName + ".chs",
            baseName + ".cht"
        };
        
        for (const QString& name : possibleNames) {
            for (const QString& ext : subExtensions) {
                QString subPath = QString("%1/%2%3").arg(folder, name, ext);
                if (QFile::exists(subPath)) {
                    existing.append(subPath);
                }
            }
        }
        
        return existing;
    }
    
    /**
     * @brief 确保目录存在
     * 
     * @param path 目录路径
     * @return 是否成功
     */
    static bool ensureDirectory(const QString& path) {
        QDir dir(path);
        return dir.exists() || dir.mkpath(".");
    }
    
    /**
     * @brief 获取文件扩展名（小写）
     * 
     * @param filePath 文件路径
     * @return 文件扩展名
     */
    static QString getExtension(const QString& filePath) {
        QFileInfo fileInfo(filePath);
        return fileInfo.suffix().toLower();
    }
    
    /**
     * @brief 获取不带扩展名的文件名
     * 
     * @param filePath 文件路径
     * @return 文件名
     */
    static QString getBaseName(const QString& filePath) {
        QFileInfo fileInfo(filePath);
        return fileInfo.completeBaseName();
    }
    
    /**
     * @brief 获取文件大小
     * 
     * @param filePath 文件路径
     * @return 文件大小（字节）
     */
    static qint64 getFileSize(const QString& filePath) {
        QFileInfo fileInfo(filePath);
        return fileInfo.size();
    }
    
    /**
     * @brief 获取文件修改时间
     * 
     * @param filePath 文件路径
     * @return 修改时间
     */
    static QDateTime getModifiedTime(const QString& filePath) {
        QFileInfo fileInfo(filePath);
        return fileInfo.lastModified();
    }
    
    /**
     * @brief 扫描目录下的所有视频文件
     * 
     * @param dirPath 目录路径
     * @param extensions 视频扩展名列表
     * @param recursive 是否递归扫描子目录
     * @param minSize 最小文件大小
     * @return 视频文件路径列表
     */
    static QStringList scanVideoFiles(const QString& dirPath,
                                      const QStringList& extensions,
                                      bool recursive = true,
                                      qint64 minSize = 10 * 1024 * 1024) {
        QStringList videoFiles;
        QDir dir(dirPath);
        
        if (!dir.exists()) {
            return videoFiles;
        }
        
        // 设置过滤器
        QDir::Filters filters = QDir::Files | QDir::NoDotAndDotDot;
        if (recursive) {
            filters |= QDir::Dirs;
        }
        
        QFileInfoList entries = dir.entryInfoList(filters, QDir::DirsFirst);
        
        for (const QFileInfo& entry : entries) {
            if (entry.isDir() && recursive) {
                // 跳过隐藏目录
                if (entry.fileName().startsWith(".")) {
                    continue;
                }
                // 递归扫描子目录
                QStringList subFiles = scanVideoFiles(entry.absoluteFilePath(), extensions, true, minSize);
                videoFiles.append(subFiles);
            } else if (entry.isFile()) {
                if (isVideoFile(entry.absoluteFilePath(), extensions, minSize)) {
                    videoFiles.append(entry.absoluteFilePath());
                }
            }
        }
        
        return videoFiles;
    }
};

#endif // FILEUTILS_H

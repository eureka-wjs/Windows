#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QMimeData>
#include <QFileDialog>
#include <QMessageBox>
#include <QThread>
#include <QTimer>
#include "core/ConfigManager.h"
#include "core/SubtitleManager.h"
#include "ui/LogWidget.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindowClass; }
QT_END_NAMESPACE

/**
 * @brief 主窗口类
 * 
 * 应用程序的主界面
 */
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
   // UI 槽函数
   void onApiTokenChanged();
   void onSaveTokenClicked();
   void onVerifyTokenClicked();
   void onDebugModeToggled();
   void onBrowseButtonClicked();
   void onStartButtonClicked();
   void onStopButtonClicked();
    
    // 处理信号
    void onProgressUpdated(int current, int total, const QString& filePath);
    void onStatsUpdated(const ScanResult& result);
    void onLogMessage(Logger::Level level, const QString& message);
    void onProcessingComplete(const ScanResult& result);
    void onQuotaChanged(int quota);

protected:
   // 拖放事件处理
   void dragEnterEvent(QDragEnterEvent* event) override;
   void dropEvent(QDropEvent* event) override;

private:
   /**
    * @brief 设置 UI
    */
   void setupUI();
    
    /**
     * @brief 连接信号槽
     */
    void setupConnections();
    
    /**
     * @brief 加载配置
     */
    void loadConfig();
    
    /**
     * @brief 保存配置
     */
    void saveConfig();
    
    /**
     * @brief 更新界面状态
     */
    void updateUIState();
    
    /**
     * @brief 设置处理状态
     * @param processing 是否正在处理
     */
    void setProcessingState(bool processing);

private:
    Ui::MainWindowClass* m_ui;        ///< UI 对象
    ConfigManager* m_configManager;   ///< 配置管理器
    SubtitleManager* m_subtitleManager; ///< 字幕管理器
    LogWidget* m_logWidget;           ///< 日志控件
    QThread* m_workerThread;          ///< 工作线程
    bool m_processing;                ///< 是否正在处理
    
    ScanResult m_currentResult;       ///< 当前扫描结果
};

#endif // MAINWINDOW_H

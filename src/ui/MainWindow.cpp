#include "MainWindow.h"
#include "ui_MainWindow.h"
#include <QDir>
#include <QUrl>
#include <QStyle>
#include <QApplication>
#include <QPointer>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , m_ui(new Ui::MainWindowClass)
    , m_configManager(new ConfigManager(this))
    , m_subtitleManager(nullptr)
    , m_logWidget(nullptr)
    , m_workerThread(new QThread(this))
    , m_processing(false)
{
    m_ui->setupUi(this);
    setupUI();
    setupConnections();
    loadConfig();
    updateUIState();
}

MainWindow::~MainWindow()
{
    delete m_ui;
}

void MainWindow::setupUI()
{
    // 创建日志控件
    m_logWidget = new LogWidget(this);
    m_ui->logContainer->layout()->addWidget(m_logWidget);
    
    // 设置拖放区域样式
    m_ui->dropFrame->setAcceptDrops(true);
    m_ui->dropFrame->setStyleSheet(
        "QFrame#dropFrame { "
        "  border: 2px dashed #aaa; "
        "  border-radius: 10px; "
        "  background-color: #f9f9f9; "
        "}"
        "QFrame#dropFrame:hover { "
        "  border-color: #0078d4; "
        "  background-color: #e5f3ff; "
        "}"
    );
    
    // 设置开始按钮样式
    m_ui->startButton->setStyleSheet(
        "QPushButton { "
        "  background-color: #4ec9b0; "
        "  color: white; "
        "  border: none; "
        "  border-radius: 5px; "
        "  font-weight: bold; "
        "}"
        "QPushButton:hover { "
        "  background-color: #3db89f; "
        "}"
        "QPushButton:pressed { "
        "  background-color: #2ca78e; "
        "}"
        "QPushButton:disabled { "
        "  background-color: #888; "
        "}"
    );
    
    // 设置停止按钮样式
    m_ui->stopButton->setStyleSheet(
        "QPushButton { "
        "  background-color: #f44747; "
        "  color: white; "
        "  border: none; "
        "  border-radius: 5px; "
        "  font-weight: bold; "
        "}"
        "QPushButton:hover { "
        "  background-color: #e03636; "
        "}"
        "QPushButton:pressed { "
        "  background-color: #cc2525; "
        "}"
        "QPushButton:disabled { "
        "  background-color: #888; "
        "}"
    );
}

void MainWindow::setupConnections()
{
    // API Token 变更
    connect(m_ui->apiTokenEdit, &QLineEdit::textChanged,
            this, &MainWindow::onApiTokenChanged);
    
    // 保存 Token 按钮
    connect(m_ui->saveTokenButton, &QPushButton::clicked,
            this, &MainWindow::onSaveTokenClicked);
    
    // 验证 Token 按钮
    connect(m_ui->verifyTokenButton, &QPushButton::clicked,
            this, &MainWindow::onVerifyTokenClicked);
    
    // 调试模式复选框
    connect(m_ui->debugModeCheckBox, &QCheckBox::checkStateChanged,
            this, &MainWindow::onDebugModeToggled);
    
    // 浏览按钮
    connect(m_ui->browseButton, &QPushButton::clicked,
            this, &MainWindow::onBrowseButtonClicked);
    
    // 开始按钮
    connect(m_ui->startButton, &QPushButton::clicked,
            this, &MainWindow::onStartButtonClicked);
    
    // 停止按钮
    connect(m_ui->stopButton, &QPushButton::clicked,
            this, &MainWindow::onStopButtonClicked);
    
    // 字幕管理器信号
    // （在 initialize 之后连接）
}

void MainWindow::loadConfig()
{
    AppConfig config = m_configManager->loadConfig();
    
    // 加载 API Token
    m_ui->apiTokenEdit->setText(config.apiKey());
    
    // 加载工作目录
    if (!config.workingDirectory().isEmpty()) {
        m_ui->workingDirEdit->setText(config.workingDirectory());
    }
    
    // 加载调试模式
    m_ui->debugModeCheckBox->setChecked(config.debugMode());
    
    // 恢复窗口状态
    if (!config.windowGeometry().isEmpty()) {
        restoreGeometry(config.windowGeometry());
    }
    if (!config.windowState().isEmpty()) {
        restoreState(config.windowState());
    }
}

void MainWindow::saveConfig()
{
    AppConfig config = m_configManager->loadConfig();
    
    // 保存 API Token
    config.setApiKey(m_ui->apiTokenEdit->text());
    
    // 保存工作目录
    config.setWorkingDirectory(m_ui->workingDirEdit->text());
    
    // 保存调试模式
    config.setDebugMode(m_ui->debugModeCheckBox->isChecked());
    
    // 保存窗口状态
    config.setWindowGeometry(saveGeometry());
    config.setWindowState(saveState());
    
    m_configManager->saveConfig(config);
}

void MainWindow::updateUIState()
{
    // 检查 API Token 是否有效
    bool hasToken = !m_ui->apiTokenEdit->text().isEmpty();
    bool hasDir = !m_ui->workingDirEdit->text().isEmpty();
    
    m_ui->saveTokenButton->setEnabled(hasToken);
    m_ui->startButton->setEnabled(hasToken && hasDir && !m_processing);
}

void MainWindow::setProcessingState(bool processing)
{
    m_processing = processing;
    
    m_ui->startButton->setEnabled(!processing);
    m_ui->stopButton->setEnabled(processing);
    m_ui->apiTokenEdit->setEnabled(!processing);
    m_ui->browseButton->setEnabled(!processing);
    
    if (processing) {
        m_ui->progressBar->setValue(0);
        m_ui->progressBar->setFormat("处理中...");
    } else {
        m_ui->progressBar->setFormat("就绪");
    }
}

void MainWindow::onApiTokenChanged()
{
    updateUIState();
}

void MainWindow::onDebugModeToggled()
{
    // 保存配置
    saveConfig();
    
    // 在日志中显示调试模式状态
    if (m_logWidget && m_ui->debugModeCheckBox->isChecked()) {
        m_logWidget->appendLog(Logger::Info, "调试模式已开启，将显示详细的处理信息");
    }
}

void MainWindow::onSaveTokenClicked()
{
    QString token = m_ui->apiTokenEdit->text();
    if (token.isEmpty()) {
        QMessageBox::warning(this, "警告", "请输入 API Token");
        return;
    }
    
    m_configManager->setApiKey(token);
    QMessageBox::information(this, "成功", "API Token 已保存");
}

void MainWindow::onVerifyTokenClicked()
{
    QString token = m_ui->apiTokenEdit->text();
    if (token.isEmpty()) {
        QMessageBox::warning(this, "警告", "请输入 API Token");
        return;
    }
    
    // 禁用按钮，防止重复点击
    m_ui->verifyTokenButton->setEnabled(false);
    m_ui->verifyTokenButton->setText("验证中...");
    
    // 创建临时 API 对象进行验证
    AssrtAPI* api = new AssrtAPI(this);
    Logger* tempLogger = new Logger(ConfigManager::downloadLogPath(), this);
    
    if (!api->initialize(token, tempLogger)) {
        QMessageBox::critical(this, "错误", "API 初始化失败，请检查 Token 格式");
        m_ui->verifyTokenButton->setEnabled(true);
        m_ui->verifyTokenButton->setText("验证");
        return;
    }
    
    // 验证 Token（通过检查配额）
    bool isValid = api->checkQuota(true);
    int quota = api->quota();
    
    // 清理
    delete tempLogger;
    delete api;
    
    // 恢复按钮状态
    m_ui->verifyTokenButton->setEnabled(true);
    m_ui->verifyTokenButton->setText("验证");
    
    // 显示结果
    if (isValid) {
        QMessageBox::information(this, "验证成功",
            QString("API Token 有效！\n当前配额：%1 次/分钟").arg(quota));
        if (m_logWidget) {
            m_logWidget->appendLog(Logger::Success,
                QString("API Token 验证成功，配额：%1 次/分钟").arg(quota));
        }
    } else {
        QMessageBox::warning(this, "验证失败",
            "API Token 无效或配额不足，请检查：\n"
            "1. Token 是否正确（32 位）\n"
            "2. 网络连接是否正常\n"
            "3. API 配额是否已用完");
        if (m_logWidget) {
            m_logWidget->appendLog(Logger::Error, "API Token 验证失败");
        }
    }
}

void MainWindow::onBrowseButtonClicked()
{
    QString dir = QFileDialog::getExistingDirectory(
        this, 
        "选择工作目录",
        m_ui->workingDirEdit->text()
    );
    
    if (!dir.isEmpty()) {
        m_ui->workingDirEdit->setText(dir);
        m_configManager->setWorkingDirectory(dir);
        updateUIState();
    }
}

void MainWindow::dragEnterEvent(QDragEnterEvent* event)
{
    if (event->mimeData()->hasUrls()) {
        event->acceptProposedAction();
    }
}

void MainWindow::dropEvent(QDropEvent* event)
{
    QList<QUrl> urls = event->mimeData()->urls();
    if (urls.isEmpty()) {
        return;
    }
    
    QString path = urls.first().toLocalFile();
    if (path.isEmpty()) {
        return;
    }
    
    QFileInfo fileInfo(path);
    if (fileInfo.isDir()) {
        m_ui->workingDirEdit->setText(path);
        m_configManager->setWorkingDirectory(path);
        updateUIState();
        
        m_logWidget->appendLog(Logger::Info, QString("工作目录：%1").arg(path));
    } else {
        m_logWidget->appendLog(Logger::Warning, "请拖放文件夹，而不是文件");
    }
}

void MainWindow::onStartButtonClicked()
{
    // 调试日志：检查指针有效性
    if (m_logWidget == nullptr) {
        QMessageBox::critical(this, "错误", "日志控件未初始化，请重启程序");
        return;
    }
    
    if (m_ui == nullptr) {
        QMessageBox::critical(this, "错误", "UI 未初始化，请重启程序");
        return;
    }
    
    QString dir = m_ui->workingDirEdit->text();
    if (dir.isEmpty()) {
        QMessageBox::warning(this, "警告", "请先选择工作目录");
        return;
    }
    
    if (!QDir(dir).exists()) {
        QMessageBox::warning(this, "警告", "工作目录不存在");
        return;
    }
    
    // 检查 API Token
    QString token = m_ui->apiTokenEdit->text();
    if (token.isEmpty()) {
        QMessageBox::warning(this, "警告", "请先输入 API Token");
        return;
    }
    
    // 初始化字幕管理器
    if (m_subtitleManager == nullptr) {
        m_subtitleManager = new SubtitleManager(m_configManager);
        
        if (!m_subtitleManager->initialize()) {
            QMessageBox::critical(this, "错误",
                "字幕管理器初始化失败，请检查：\n"
                "1. API Token 是否正确\n"
                "2. 网络连接是否正常\n"
                "3. 日志文件路径是否可写");
            return;
        }
        
        // 验证 API
        AssrtAPI* api = m_subtitleManager->api();
        if (!api) {
            QMessageBox::critical(this, "错误", "API 客户端初始化失败");
            return;
        }
        
        // 连接信号
        connect(m_subtitleManager, &SubtitleManager::progressUpdated,
                this, &MainWindow::onProgressUpdated);
        connect(m_subtitleManager, &SubtitleManager::statsUpdated,
                this, &MainWindow::onStatsUpdated);
        connect(m_subtitleManager, &SubtitleManager::logMessage,
                this, &MainWindow::onLogMessage);
        connect(m_subtitleManager, &SubtitleManager::processingComplete,
                this, &MainWindow::onProcessingComplete);
        connect(api, &AssrtAPI::quotaChanged,
                this, &MainWindow::onQuotaChanged);
    }
    
    // 保存配置
    saveConfig();
    
    // 开始处理
    setProcessingState(true);
    m_logWidget->clear();
    m_logWidget->appendLog(Logger::Info, QString("开始处理目录：%1").arg(dir));
    
    // 在工作线程中处理
    // 先清理旧的线程
    if (m_workerThread != nullptr && m_workerThread->isRunning()) {
        m_workerThread->quit();
        m_workerThread->wait(3000);
    }
    
    m_workerThread = new QThread(this);
    m_subtitleManager->moveToThread(m_workerThread);
    
    // 使用 QPointer 防止悬空指针
    QPointer<SubtitleManager> subManager(m_subtitleManager);
    connect(m_workerThread, &QThread::started, [subManager, dir]() {
        if (subManager) {
            // 重新加载配置，确保调试模式等设置生效
            subManager->reloadConfig();
            subManager->scanFolder(dir);
        }
    });
    connect(m_workerThread, &QThread::finished, m_workerThread, &QThread::deleteLater);
    
    m_workerThread->start();
}

void MainWindow::onStopButtonClicked()
{
    if (m_subtitleManager) {
        m_subtitleManager->stop();
        m_logWidget->appendLog(Logger::Warning, "正在停止处理...");
    }
}

void MainWindow::onProgressUpdated(int current, int total, const QString& filePath)
{
    m_ui->progressBar->setValue(current);
    m_ui->progressBar->setFormat(QString("%1 / %2").arg(current).arg(total));
    
    // 更新当前文件标签
    QFileInfo fileInfo(filePath);
    m_ui->currentFileLabel->setText(QString("当前文件：%1").arg(fileInfo.fileName()));
}

void MainWindow::onStatsUpdated(const ScanResult& result)
{
    m_currentResult = result;
    
    QString stats = QString("总文件：%1 | 成功：%2 | 失败：%3 | 跳过：%4")
        .arg(result.totalScanned)
        .arg(result.success)
        .arg(result.failed)
        .arg(result.skipped);
    
    m_ui->statsLabel->setText(stats);
}

void MainWindow::onLogMessage(Logger::Level level, const QString& message)
{
    m_logWidget->appendLog(level, message);
}

void MainWindow::onProcessingComplete(const ScanResult& result)
{
    setProcessingState(false);
    
    // 清理工作线程
    if (m_workerThread) {
        m_workerThread->quit();
        m_workerThread->wait();
        delete m_workerThread;
        m_workerThread = nullptr;
    }
    
    // 显示完成消息
    if (result.failed == 0 && result.success > 0) {
        m_logWidget->appendLog(Logger::Success, "所有文件处理完成！");
    } else if (result.success > 0) {
        m_logWidget->appendLog(Logger::Warning, QString("处理完成，%1 个失败").arg(result.failed));
    } else {
        m_logWidget->appendLog(Logger::Error, "处理完成，但没有成功下载任何字幕");
    }
}

void MainWindow::onQuotaChanged(int quota)
{
    m_ui->quotaLabel->setText(QString("配额：%1/分钟").arg(quota));
}

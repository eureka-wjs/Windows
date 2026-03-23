#include <QApplication>
#include <QStyleFactory>
#include <QFont>
#include "ui/MainWindow.h"

int main(int argc, char *argv[])
{
    // Qt6 中高 DPI 支持默认启用
    QApplication::setHighDpiScaleFactorRoundingPolicy(Qt::HighDpiScaleFactorRoundingPolicy::PassThrough);
    
    QApplication app(argc, argv);
    
    // 设置应用程序信息
    QApplication::setOrganizationName("AutoDownloadSubtitle");
    QApplication::setApplicationName("AutoDownloadSubtitle");
    QApplication::setApplicationVersion("1.0.0");
    
    // 设置样式
    QApplication::setStyle(QStyleFactory::create("Fusion"));
    
    // 设置字体
    QFont font("Segoe UI", 9);
    QApplication::setFont(font);
    
    // 创建主窗口
    MainWindow w;
    w.show();
    
    return QApplication::exec();
}

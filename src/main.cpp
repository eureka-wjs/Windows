#include <QApplication>
#include <QStyleFactory>
#include <QFont>
#include "src/ui/MainWindow.h"

int main(int argc, char *argv[])
{
    // 启用高 DPI 支持
    QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
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

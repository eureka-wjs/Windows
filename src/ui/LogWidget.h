#ifndef LOGWIDGET_H
#define LOGWIDGET_H

#include <QWidget>
#include <QTextEdit>
#include <QScrollBar>
#include <QVBoxLayout>
#include <QLabel>
#include <QFont>
#include "utils/Logger.h"

/**
 * @brief 日志显示控件
 * 
 * 用于在界面上显示分级日志
 */
class LogWidget : public QWidget
{
    Q_OBJECT

public:
    explicit LogWidget(QWidget* parent = nullptr)
        : QWidget(parent)
    {
        setupUI();
    }
    
    /**
     * @brief 添加日志
     * @param level 日志级别
     * @param message 日志消息
     */
    void appendLog(Logger::Level level, const QString& message) {
        QString formatted = formatLog(level, message);
        m_textEdit->append(formatted);
        scrollToBottom();
    }
    
    /**
     * @brief 清空日志
     */
    void clear() {
        m_textEdit->clear();
    }
    
    /**
     * @brief 滚动到底部
     */
    void scrollToBottom() {
        QScrollBar* scrollbar = m_textEdit->verticalScrollBar();
        if (scrollbar) {
            scrollbar->setValue(scrollbar->maximum());
        }
    }

public slots:
    /**
     * @brief 槽函数：处理日志信号
     * @param level 日志级别
     * @param message 日志消息
     */
    void onLogMessage(Logger::Level level, const QString& message) {
        appendLog(level, message);
    }

private:
    /**
     * @brief 设置 UI
     */
    void setupUI() {
        QVBoxLayout* layout = new QVBoxLayout(this);
        layout->setContentsMargins(0, 0, 0, 0);
        layout->setSpacing(0);
        
        // 日志标题
        QLabel* titleLabel = new QLabel("处理日志");
        titleLabel->setStyleSheet("font-weight: bold; padding: 5px;");
        layout->addWidget(titleLabel);
        
        // 日志文本框
        m_textEdit = new QTextEdit();
        m_textEdit->setReadOnly(true);
        m_textEdit->setLineWrapMode(QTextEdit::WidgetWidth);
        m_textEdit->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
        m_textEdit->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
        m_textEdit->setFont(QFont("Consolas", 9));
        m_textEdit->setStyleSheet(
            "QTextEdit { "
            "  background-color: #1e1e1e; "
            "  color: #d4d4d4; "
            "  border: 1px solid #3c3c3c; "
            "  padding: 5px; "
            "}"
        );
        layout->addWidget(m_textEdit);
        
        setLayout(layout);
    }
    
    /**
     * @brief 格式化日志
     * @param level 日志级别
     * @param message 日志消息
     * @return 格式化后的 HTML
     */
    QString formatLog(Logger::Level level, const QString& message) {
        QString icon;
        QString color;
        
        switch (level) {
            case Logger::Debug:
                icon = "🔍";
                color = "#808080";  // 灰色
                break;
            case Logger::Info:
                icon = "ℹ️";
                color = "#d4d4d4";  // 白色
                break;
            case Logger::Success:
                icon = "✅";
                color = "#4ec9b0";  // 绿色
                break;
            case Logger::Warning:
                icon = "⚠️";
                color = "#ce9178";  // 橙色
                break;
            case Logger::Error:
                icon = "❌";
                color = "#f44747";  // 红色
                break;
            default:
                icon = "•";
                color = "#d4d4d4";
        }
        
        return QString("<span style=\"color: %1;\">%2 %3</span>")
            .arg(color, icon, message.toHtmlEscaped());
    }

private:
    QTextEdit* m_textEdit;    ///< 文本编辑控件
};

#endif // LOGWIDGET_H

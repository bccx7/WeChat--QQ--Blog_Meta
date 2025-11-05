#include "QQvs.h"
#include <QtWidgets/QApplication>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    // 简易 QQ 风格样式（主色 #12B7F5）
    QString qss = R"(QMainWindow{background:#f5f7fa;border-radius:8px;}\n
QPushButton,QToolButton{background:#ffffff;border:1px solid #d0d5db;border-radius:6px;padding:6px 12px;font-size:14px;}\n
QPushButton:hover,QToolButton:hover{background:#12B7F5;color:#ffffff;}\n
QPushButton:pressed,QToolButton:pressed{background:#0fa0d8;color:#ffffff;}\n
QListWidget{background:#ffffff;border:1px solid #d0d5db;border-radius:6px;}\n
QListWidget::item{padding:4px;}\n
QScrollBar:vertical{background:transparent;width:8px;margin:0;}\n
QScrollBar::handle:vertical{background:#c1c7cd;border-radius:4px;}\n
QScrollBar::handle:vertical:hover{background:#12B7F5;}\n
QLabel#HeaderLabel{font-size:16px;font-weight:bold;color:#222;}\n
QFrame#Separator{background:#d0d5db;height:1px;}\n)";
    app.setStyleSheet(qss);

    QQvs window;
    window.show();
    return app.exec();
}

/********************************************************************************
** Form generated from reading UI file 'QQvs.ui'
**
** Created by: Qt User Interface Compiler version 6.5.3
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_QQVS_H
#define UI_QQVS_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QMenuBar>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QStatusBar>
#include <QtWidgets/QToolBar>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_QQvsClass
{
public:
    QWidget *centralWidget;
    QPushButton *loginButton;
    QLineEdit *id;
    QLineEdit *passward;
    QLabel *label;
    QMenuBar *menuBar;
    QToolBar *mainToolBar;
    QStatusBar *statusBar;

    void setupUi(QMainWindow *QQvsClass)
    {
        if (QQvsClass->objectName().isEmpty())
            QQvsClass->setObjectName("QQvsClass");
        QQvsClass->resize(554, 536);
        centralWidget = new QWidget(QQvsClass);
        centralWidget->setObjectName("centralWidget");
        loginButton = new QPushButton(centralWidget);
        loginButton->setObjectName("loginButton");
        loginButton->setGeometry(QRect(220, 370, 80, 24));
        id = new QLineEdit(centralWidget);
        id->setObjectName("id");
        id->setGeometry(QRect(170, 170, 181, 23));
        passward = new QLineEdit(centralWidget);
        passward->setObjectName("passward");
        passward->setGeometry(QRect(170, 270, 181, 23));
        label = new QLabel(centralWidget);
        label->setObjectName("label");
        label->setGeometry(QRect(210, 70, 101, 41));
        label->setStyleSheet(QString::fromUtf8("font: 9pt \"Mistral\";\n"
"font: 9pt \"Modern No. 20\";"));
        QQvsClass->setCentralWidget(centralWidget);
        menuBar = new QMenuBar(QQvsClass);
        menuBar->setObjectName("menuBar");
        menuBar->setGeometry(QRect(0, 0, 554, 21));
        QQvsClass->setMenuBar(menuBar);
        mainToolBar = new QToolBar(QQvsClass);
        mainToolBar->setObjectName("mainToolBar");
        QQvsClass->addToolBar(Qt::TopToolBarArea, mainToolBar);
        statusBar = new QStatusBar(QQvsClass);
        statusBar->setObjectName("statusBar");
        QQvsClass->setStatusBar(statusBar);

        retranslateUi(QQvsClass);

        QMetaObject::connectSlotsByName(QQvsClass);
    } // setupUi

    void retranslateUi(QMainWindow *QQvsClass)
    {
        QQvsClass->setWindowTitle(QCoreApplication::translate("QQvsClass", "QQvs", nullptr));
        loginButton->setText(QCoreApplication::translate("QQvsClass", "\347\231\273\345\275\225", nullptr));
        label->setText(QCoreApplication::translate("QQvsClass", "<html><head/><body><p><span style=\" font-size:28pt;\">Meta</span></p></body></html>", nullptr));
    } // retranslateUi

};

namespace Ui {
    class QQvsClass: public Ui_QQvsClass {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_QQVS_H

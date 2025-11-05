#pragma once

#include <QtWidgets/QMainWindow>
#include <QStackedWidget>
#include "PlatformBcc.h"
#include "ui_QQvs.h"

class QLabel;

/**
 * @class QQvs
 * @brief 应用程序的主窗口类，负责管理UI、用户交互和页面切换。
 *
 * 该类处理用户登录、服务选择、主界面展示以及与后端平台（PlatformBcc）的交互。
 * 它使用 QStackedWidget 来管理不同的视图，如登录页、服务选择页和主服务页。
 */
class QQvs : public QMainWindow
{
    Q_OBJECT

public:
    /**
     * @brief 构造一个新的 QQvs 主窗口对象。
     * @param parent 父QWidget，默认为nullptr。
     */
    explicit QQvs(QWidget* parent = nullptr);
    
    /**
     * @brief 析构函数，清理资源。
     */
    ~QQvs();

private slots:
    /**
     * @brief 处理登录按钮点击事件。
     *
     * 验证用户凭据，处理新用户注册，并引导至服务选择或主界面。
     */
    void onLoginClicked();

    /**
     * @brief 显示登录页面。
     *
     * 切换 QStackedWidget 到登录视图，并清理之前的会话状态。
     */
    void showLoginPage();

    /**
     * @brief 显示用户的主页面（已弃用，重定向到 showServiceMain）。
     * @param userId 登录用户的ID。
     */
    void showMainPage(const QString& userId);

    /**
     * @brief 显示指定服务的主界面。
     * @param userId 登录用户的ID。
     * @param service 要显示的服务名称（如 "QQ", "WeChat"）。
     */
    void showServiceMain(const QString& userId, const QString& service);

private:
    /**
     * @brief 为新用户显示开通服务的选择界面。
     * @param userId 用户的ID。
     */
    void showOpenServiceChooser(const QString& userId);

    /**
     * @brief 显示服务选择界面，让用户选择进入哪个已开通的服务。
     * @param userId 用户的ID。
     */
    void showServiceSelector(const QString& userId);

    /**
     * @brief 事件过滤器，用于捕获特定控件的事件，如头像点击。
     * @param watched 被监视的对象。
     * @param event 发生的事件。
     * @return 如果事件被处理，则返回 true，否则返回 false。
     */
    bool eventFilter(QObject* watched, QEvent* event) override;

    Ui::QQvsClass ui; ///< Qt Designer 生成的UI类实例。
    PlatformBcc* m_platformPtr = nullptr; ///< 指向平台单例的指针。
    
    /**
     * @brief 获取平台单例的引用，如果不存在则创建。
     * @return PlatformBcc 的引用。
     */
    PlatformBcc& m_platformRef();

    QStackedWidget* m_stack = nullptr; ///< 用于管理UI页面的堆栈窗口。
    QWidget* m_mainPage = nullptr; ///< 指向当前主页面（服务选择或服务主界面）的指针。
    QLabel* m_avatarSelf = nullptr; ///< 指向用户自己头像的QLabel控件。
    QString m_currentUserId; ///< 当前登录用户的ID。
    QString m_currentService; ///< 用户当前正在使用的服务名称。
};


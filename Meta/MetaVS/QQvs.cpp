#include "QQvs.h"
#include <QMessageBox>
#include <QWidget>
#include <QFile>
#include <QTextStream>
#include "PlatformBcc.h"
#include <QDir>
#include <QMenu>
#include <QAction>
#include <QListWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QDialog>
#include <QPushButton>
#include <QStackedWidget>
#include <QInputDialog>
#include <QFormLayout>
#include <QLineEdit>
#include <QDate>
#include <QCheckBox>
#include <QScrollArea>
#include <QTimer>
#include <QSplitter>
#include <QDateTime>
#include <functional>
#include <QDateTime>
#include <QTextEdit>
#include <QScrollBar>
#include <QDateEdit>
#include <QMouseEvent>

namespace {
  
}

/**
 * @brief QQvs 类的构造函数。
 *
 * 初始化UI，设置页面堆栈 (QStackedWidget)，并将登录页面作为初始页面。
 * @param parent 父QWidget指针。
 */
QQvs::QQvs(QWidget *parent)
    : QMainWindow(parent)
{
    ui.setupUi(this); 

    // 安全取出当前 central widget（登录页），避免稍后 setCentralWidget 删除它
    QWidget* loginWidget = this->takeCentralWidget();

    m_stack = new QStackedWidget(this);
    m_stack->addWidget(loginWidget); // index 0 登录界面
    setCentralWidget(m_stack);       // 设置堆栈为新的 central widget

    // 连接登录按钮的点击信号到 onLoginClicked 槽函数
    connect(ui.loginButton, &QPushButton::clicked, this, &QQvs::onLoginClicked);
}

/**
 * @brief QQvs 类的析构函数。
 *
 * 清理平台实例指针，防止内存泄漏。
 */
QQvs::~QQvs()
{
    if (m_platformPtr) {
        delete m_platformPtr;
        m_platformPtr = nullptr;
    }
}

/**
 * @brief 处理登录按钮点击事件的槽函数。
 *
 * 负责整个登录和注册流程：
 * 1. 从 "account.txt" 同步所有用户数据到内存。
 * 2. 验证用户输入的账号和密码。
 * 3. 如果用户不存在，则启动新用户注册流程，收集信息并保存。
 * 4. 如果用户存在且密码正确，则继续登录。
 * 5. 登录成功后，加载用户数据，同步不同服务间的好友列表。
 * 6. 执行一些平台初始化操作，如恢复群组结构、清理无效群等。
 * 7. 根据用户是否已开通服务，导航到服务开通页面或服务选择页面。
 */
void QQvs::onLoginClicked()
{
    // 自动同步account.txt到内存用户表，并加载每个用户的本地数据文件
    QFile syncFile("C:/Users/bcc54/Desktop/Code/QQvs/QQvs/account.txt");
    if (syncFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream in(&syncFile);
        auto& m_platform = m_platformRef();
        while (!in.atEnd()) {
            QString line = in.readLine();
            QStringList parts = line.split(' ');
            if (parts.size() == 2) {
                QString uid = parts[0];
                m_platform.ensureUserExists(uid);
                // 尝试加载该用户的 .dat 文件（如果存在）以恢复昵称/地点/注册日期/好友等
                UserBcc* u = m_platform.getUser(uid);
                if (u) {
                    QString userDataPath = QString("C:/Users/bcc54/Desktop/Code/QQvs/QQvs/%1.dat").arg(uid);
                    QFile f(userDataPath);
                    if (f.exists()) {
                        u->loadFromFile(userDataPath);
                    }
                }
            }
        }
        syncFile.close();
    }

    QString userId = ui.id->text().trimmed();
    QString password = ui.passward->text();

    if (userId.isEmpty() || password.isEmpty()) {
        QMessageBox::warning(this, "提示", "账号和密码不能为空！");
        return;
    }

    // 验证账户和密码
    QFile file("C:/Users/bcc54/Desktop/Code/QQvs/QQvs/account.txt");
    bool found = false;
    bool passwordCorrect = false;
    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream in(&file);
        while (!in.atEnd()) {
            QString line = in.readLine();
            QStringList parts = line.split(' ');
            if (parts.size() == 2) {
                if (parts[0] == userId) {
                    found = true;
                    if (parts[1] == password) {
                        passwordCorrect = true;
                    }
                    break;
                }
            }
        }
        file.close();
    }

    auto& m_platform = m_platformRef();

    if (found) {
        if (!passwordCorrect) {
            QMessageBox::warning(this, "提示", "密码错误！");
            return;
        }
        // 现有用户：继续登录流程
    }
    else {
        // 新用户注册流程
        QDialog dlg(this);
        dlg.setWindowTitle("新用户信息 - 完成注册");
        QFormLayout* form = new QFormLayout(&dlg);
        QLineEdit* nickEdit = new QLineEdit();
        QLineEdit* locEdit = new QLineEdit();
        QDateEdit* birthEdit = new QDateEdit(); birthEdit->setCalendarPopup(true); birthEdit->setDisplayFormat("yyyy-MM-dd"); birthEdit->setDate(QDate::currentDate());
        form->addRow("昵称：", nickEdit);
        form->addRow("地点：", locEdit);
        form->addRow("出生日期：", birthEdit);

        QHBoxLayout* hb = new QHBoxLayout();
        QPushButton* okBtn = new QPushButton("确定");
        QPushButton* cancelBtn = new QPushButton("取消");
        hb->addWidget(okBtn);
        hb->addWidget(cancelBtn);
        form->addRow(hb);

        connect(okBtn, &QPushButton::clicked, &dlg, &QDialog::accept);
        connect(cancelBtn, &QPushButton::clicked, &dlg, &QDialog::reject);

        if (dlg.exec() != QDialog::Accepted) {
            QMessageBox::information(this, "提示", "已取消注册。");
            return;
        }

        QString nickname = nickEdit->text().trimmed();
        QString location = locEdit->text().trimmed();
        QDate birthDate = birthEdit->date();

        if (nickname.isEmpty() || location.isEmpty()) {
            QMessageBox::warning(this, "提示", "昵称和地点不能为空，注册未完成。");
            return;
        }

        // 写入新账户信息
        if (file.open(QIODevice::Append | QIODevice::Text)) {
            QTextStream out(&file);
            out << userId << " " << password << "\n";
            file.close();
        } else {
            QMessageBox::warning(this, "错误", "无法写入账户文件！");
            return;
        }

        // 创建用户并保存信息
        m_platform.ensureUserExists(userId);
        UserBcc* newUser = m_platform.getUser(userId);
        if (newUser) {
            newUser->setNickname(nickname);
            newUser->setLocation(location);
            newUser->setRegisterDate(QDate::currentDate());
            if(birthDate.isValid()) newUser->setBirthDate(birthDate); else newUser->setBirthDate(QDate::currentDate());
            QString userDataPath = "C:/Users/bcc54/Desktop/Code/QQvs/QQvs/" + userId + ".dat";
            newUser->saveToFile(userDataPath);
        }

        QMessageBox::information(this, "提示", "新用户注册成功，已自动登录！");
    }

    // 平台登录与数据同步
    m_platform.login(userId, "QQ");
    auto* user = m_platform.getUser(userId);
    if (user) {
        QString userDataPath = "C:/Users/bcc54/Desktop/Code/QQvs/QQvs/" + userId + ".dat";
        user->loadFromFile(userDataPath);

        // 同步QQ和微信的好友信息
        if (user->openedServices().contains("QQ") && user->openedServices().contains("WeChat")) {
            QSet<QString> qqFriends = user->getFriendsOfService("QQ");
            for (const QString& friendId : qqFriends) {
                user->addFriendForService("WeChat", friendId);
            }

            QSet<QString> wechatFriends = user->getFriendsOfService("WeChat");
            for (const QString& friendId : wechatFriends) {
                user->addFriendForService("QQ", friendId);
            }
            user->saveToFile(userDataPath); // 保存同步后的结果
        }
    }

    // 演示功能：切换群类型
    ServiceBcc* qqService = m_platform.getService("QQ");
    if (qqService) {
        auto g = qqService->getGroup("1001");
        if (g) {
            g->addMember(userId);
            m_platform.switchGroupTypePreserveMembers(qqService, "1001", GroupTypeBcc::WechatGroup);
        }
    }

    // 全局数据恢复与清理
    m_platform.reconstructGroupsFromUserData();
    m_platform.loadAllGroups("C:/Users/bcc54/Desktop/Code/QQvs/QQvs");

    // 清理当前用户无效的群组信息
    if (user) {
        QSet<QString> toRemove;
        QSet<QString> groups = user->getGroupsOfService("QQ");
        ServiceBcc* svc = m_platform.getService("QQ");
        for (const QString& gid : groups) {
            if (!svc) continue;
            auto g = svc->getGroup(gid);
            if (!g || !g->getMembers().contains(userId)) {
                toRemove.insert(gid);
            }
        }
        for (const QString& gid : toRemove) {
            user->leaveGroup("QQ", gid);
        }
        if (!toRemove.isEmpty()) {
            user->saveToFile("C:/Users/bcc54/Desktop/Code/QQvs/QQvs/" + userId + ".dat");
        }
    }

    m_currentUserId = userId;
    
    // 根据用户状态决定下一页面
    UserBcc* currentUser = m_platform.getUser(userId);
    if (currentUser && currentUser->openedServices().isEmpty()) {
        showOpenServiceChooser(userId);
    } else {
        showServiceSelector(userId);
    }
}

/**
 * @brief 为新用户显示开通服务的选择界面。
 *
 * 动态创建UI，包含QQ、微信、微博等服务选项，用户选择后保存并进入服务选择页面。
 * @param userId 用户的ID。
 */
void QQvs::showOpenServiceChooser(const QString& userId)
{
    if (m_mainPage) {
        m_stack->removeWidget(m_mainPage);
        delete m_mainPage;
        m_mainPage = nullptr;
    }

    QWidget* page = new QWidget();
    QVBoxLayout* v = new QVBoxLayout(page);
    v->setContentsMargins(20, 20, 20, 20);
    v->setSpacing(20);

    QLabel* tip = new QLabel("选择你要开通的服务：");
    tip->setStyleSheet("font-size:18px; font-weight:bold;");
    v->addWidget(tip, 0, Qt::AlignCenter);

    QHBoxLayout* hl = new QHBoxLayout();
    hl->setSpacing(30);

    struct ServiceItem {
        QString name;
        QString label;
        QColor color;
    };
    ServiceItem items[] = {
        {"QQ", "QQ", QColor("#12B7F5")},
        {"WeChat", "微信", QColor("#07C160")},
        {"Weibo", "微博", QColor("#F56C6C")}
    };

    QList<QCheckBox*> checks;
    for (const auto& it : items) {
        QWidget* itemWidget = new QWidget();
        QVBoxLayout* one = new QVBoxLayout(itemWidget);
        one->setSpacing(10);
        QLabel* circle = new QLabel(it.label);
        circle->setFixedSize(80, 80);
        circle->setStyleSheet(QString("border-radius:40px; background:%1; color:#fff; font:bold 18px 'Segoe UI'; qproperty-alignment:AlignCenter;").arg(it.color.name()));
        QCheckBox* cb = new QCheckBox("开通");
        cb->setProperty("svc", it.name);
        one->addWidget(circle, 0, Qt::AlignHCenter);
        one->addWidget(cb, 0, Qt::AlignHCenter);
        hl->addWidget(itemWidget);
        checks.append(cb);
    }
    v->addLayout(hl);

    QPushButton* ok = new QPushButton("确定");
    ok->setMinimumHeight(40);
    ok->setStyleSheet("font-size:16px;");
    v->addWidget(ok);
    v->addStretch();

    connect(ok, &QPushButton::clicked, this, [this, userId, checks]() {
        PlatformBcc& plat = m_platformRef();
        UserBcc* u = plat.getUser(userId);
        if (!u) {
            showLoginPage();
            return;
        }
        QSet<QString> opened;
        for (auto* cb : checks) {
            if (cb->isChecked()) {
                opened.insert(cb->property("svc").toString());
            }
        }
        if (opened.isEmpty()) {
            QMessageBox::warning(this, "提示", "请至少选择一个服务");
            return;
        }
        for (const QString& s : opened) {
            u->addOpenedService(s);
        }
        u->saveToFile("C:/Users/bcc54/Desktop/Code/QQvs/QQvs/" + userId + ".dat");
        showServiceSelector(userId);
    });

    m_stack->addWidget(page);
    m_mainPage = page;
    m_stack->setCurrentWidget(page);
}

/**
 * @brief 显示服务选择界面。
 *
 * 用户可以在此页面选择进入已开通的任一服务，或退出登录。
 * @param userId 用户的ID。
 */
void QQvs::showServiceSelector(const QString& userId)
{
    if (m_mainPage) {
        m_stack->removeWidget(m_mainPage);
        delete m_mainPage;
        m_mainPage = nullptr;
    }

    PlatformBcc& plat = m_platformRef();
    UserBcc* u = plat.getUser(userId);
    if (!u) {
        showLoginPage();
        return;
    }

    QWidget* page = new QWidget();
    QVBoxLayout* v = new QVBoxLayout(page);
    v->setContentsMargins(20, 20, 20, 20);
    v->setSpacing(20);

    QLabel* tip = new QLabel("选择要进入的服务：");
    tip->setStyleSheet("font-size:18px; font-weight:bold;");
    v->addWidget(tip, 0, Qt::AlignCenter);

    QHBoxLayout* hl = new QHBoxLayout();
    hl->setSpacing(30);

    auto createServiceButton = [this, userId](const QString& svc, const QString& text, const QColor& c) {
        QPushButton* b = new QPushButton(text);
        b->setFixedSize(100, 100);
        b->setStyleSheet(QString("border-radius:50px; background:%1; color:#fff; font:bold 16px 'Segoe UI';").arg(c.name()));
        connect(b, &QPushButton::clicked, this, [this, userId, svc]() {
            m_currentService = svc;
            showServiceMain(userId, svc);
        });
        return b;
    };

    if (u->openedServices().contains("QQ")) {
        hl->addWidget(createServiceButton("QQ", "QQ", QColor("#12B7F5")));
    }
    if (u->openedServices().contains("WeChat")) {
        hl->addWidget(createServiceButton("WeChat", "微信", QColor("#07C160")));
    }
    if (u->openedServices().contains("Weibo")) {
        hl->addWidget(createServiceButton("Weibo", "微博", QColor("#F56C6C")));
    }
    v->addLayout(hl);
    v->addStretch();

    QPushButton* logout = new QPushButton("退出登录");
    logout->setMinimumHeight(40);
    logout->setStyleSheet("font-size:16px;");
    v->addWidget(logout);
    connect(logout, &QPushButton::clicked, this, &QQvs::showLoginPage);

    m_stack->addWidget(page);
    m_mainPage = page;
    m_stack->setCurrentWidget(page);
}

/**
 * @brief 显示指定服务的主界面。
 *
 * 这是应用的核心UI，根据服务类型（QQ, WeChat, Weibo）动态构建界面。
 * - 对于微博，显示一个全局的公共聊天广场。
 * - 对于QQ和微信，显示好友和群组列表，并提供加好友、建群、聊天、管理等功能。
 * - 包含大量动态UI创建和信号槽连接，用于实现复杂的交互逻辑。
 * @param userId 登录用户的ID。
 * @param service 要显示的服务名称。
 */
void QQvs::showServiceMain(const QString& userId, const QString& service)
{
    if (m_mainPage) {
        m_stack->removeWidget(m_mainPage);
        delete m_mainPage;
        m_mainPage = nullptr;
    }

    // 微博服务的特殊UI分支
    if (service == "Weibo") {
        QWidget* mainWidget = new QWidget();
        mainWidget->setWindowTitle(service + " - 主界面");
        mainWidget->resize(700, 600);
        QHBoxLayout* rootLayout = new QHBoxLayout(mainWidget); rootLayout->setContentsMargins(0,0,0,0); rootLayout->setSpacing(0);
        
        // 左侧工具列
        QWidget* toolCol = new QWidget(); toolCol->setMinimumWidth(90); toolCol->setMaximumWidth(110);
        QVBoxLayout* toolLayout = new QVBoxLayout(toolCol); toolLayout->setContentsMargins(8,8,8,8); toolLayout->setSpacing(10);
        QLabel* avatarSelf = new QLabel(userId.right(2)); m_avatarSelf = avatarSelf; avatarSelf->installEventFilter(this);
        avatarSelf->setFixedSize(60,60);
        avatarSelf->setStyleSheet("border-radius:30px;background:#F56C6C;color:#fff;font:bold 20px 'Segoe UI';qproperty-alignment:AlignCenter;");
        toolLayout->addWidget(avatarSelf,0,Qt::AlignHCenter);
        QPushButton* logoutBtn = new QPushButton("退出"); logoutBtn->setFixedHeight(32); logoutBtn->setStyleSheet("font-size:12px;");
        toolLayout->addWidget(logoutBtn); connect(logoutBtn,&QPushButton::clicked,this,&QQvs::showLoginPage);
        toolLayout->addStretch();
        QPushButton* switchSvcBtn = new QPushButton("切换服务"); switchSvcBtn->setFixedHeight(32); toolLayout->addWidget(switchSvcBtn);
        connect(switchSvcBtn,&QPushButton::clicked,this,[this,userId](){ showServiceSelector(userId); });
        QLabel* currentSvcLbl = new QLabel("当前服务: "+service); currentSvcLbl->setStyleSheet("color:#666;font-size:11px;"); currentSvcLbl->setAlignment(Qt::AlignHCenter);
        toolLayout->addWidget(currentSvcLbl);
        
        // 右侧微博广场
        QWidget* plaza = new QWidget(); QVBoxLayout* plazaLayout = new QVBoxLayout(plaza);
        plazaLayout->setContentsMargins(8,8,8,8); plazaLayout->setSpacing(8);
        QLabel* title = new QLabel("微博"); title->setStyleSheet("font-weight:bold;font-size:14px;color:#F56C6C;");
        plazaLayout->addWidget(title);
        QScrollArea* scroll = new QScrollArea(); scroll->setWidgetResizable(true);
        QWidget* msgContainer = new QWidget(); QVBoxLayout* msgLayout = new QVBoxLayout(msgContainer);
        msgLayout->setContentsMargins(12,12,12,12); msgLayout->setSpacing(10);
        scroll->setWidget(msgContainer); plazaLayout->addWidget(scroll,1);
        QHBoxLayout* inputBar = new QHBoxLayout(); QTextEdit* input = new QTextEdit(); input->setPlaceholderText("发布内容..."); input->setFixedHeight(100);
        QPushButton* sendBtn = new QPushButton("发送"); QPushButton* refreshBtn = new QPushButton("刷新");
        inputBar->addWidget(input,1); QVBoxLayout* side = new QVBoxLayout(); side->addWidget(sendBtn); side->addWidget(refreshBtn); side->addStretch();
        inputBar->addLayout(side); plazaLayout->addLayout(inputBar);
        
        const QString chatFile = "C:/Users/bcc54/Desktop/Code/QQvs/QQvs/chat_weibo_global.log";
        auto esc = [](QString s){ return s.replace("\n","\\n"); };
        auto unesc = [](QString s){ return s.replace("\\n","\n"); };
        
        // 消息气泡UI创建
        auto addBubble = [scroll,msgLayout,userId](const QString& from,const QString& text,const QDateTime& ts){
            bool mine = (from==userId);
            QWidget* wrap = new QWidget(); QHBoxLayout* hl = new QHBoxLayout(wrap); hl->setContentsMargins(0,0,0,0); hl->setSpacing(8);
            QLabel* avatar = new QLabel(from.right(2)); avatar->setFixedSize(36,36);
            avatar->setStyleSheet(QString("border-radius:18px;%1color:#fff;font:bold 12px 'Segoe UI';qproperty-alignment:AlignCenter;").arg(mine?"background:#F56C6C;":"background:#999;"));
            QString rich = QString("<div style='font-size:11px;color:#666;margin-bottom:2px;'>%1 %2</div>%3")
                           .arg(ts.toString("HH:mm:ss"), from, text.toHtmlEscaped().replace("\n","<br/>"));
            QLabel* bubble = new QLabel(rich); bubble->setTextFormat(Qt::RichText); bubble->setWordWrap(true);
            bubble->setStyleSheet(QString("background:%1;border-radius:6px;padding:6px 12px;font-size:12px;color:#222;max-width:480px;line-height:1.25;")
                                  .arg(mine?QColor("#F56C6C").lighter(150).name():"#EDEDED"));
            if(mine){ hl->addStretch(); hl->addWidget(bubble); hl->addWidget(avatar);} else { hl->addWidget(avatar); hl->addWidget(bubble); hl->addStretch(); }
            msgLayout->addWidget(wrap);
            QTimer::singleShot(0,[scroll](){ if(auto sb=scroll->verticalScrollBar()) sb->setValue(sb->maximum()); });
        };
        
        // 消息加载与发送逻辑
        auto loadMessages = [msgLayout,scroll,chatFile,unesc,addBubble](){
            while(QLayoutItem* child=msgLayout->takeAt(0)){ if(child->widget()) child->widget()->deleteLater(); delete child; }
            QFile f(chatFile); if(!f.exists()||!f.open(QIODevice::ReadOnly|QIODevice::Text)) return; QTextStream in(&f);
            while(!in.atEnd()){ QString line=in.readLine(); if(line.trimmed().isEmpty()) continue; QStringList parts=line.split('\t'); if(parts.size()<3) continue; QDateTime ts=QDateTime::fromString(parts[0],Qt::ISODate); addBubble(parts[1], unesc(line.section('\t',2)), ts.isValid()?ts:QDateTime::currentDateTime()); }
            f.close();
        };
        auto appendMessage = [chatFile,esc](const QString& from,const QString& text){ QFile f(chatFile); if(!f.open(QIODevice::Append|QIODevice::Text)) return false; QTextStream out(&f); out<<QDateTime::currentDateTime().toString(Qt::ISODate)<<'\t'<<from<<'\t'<<esc(text)<<'\n'; f.close(); return true; };
        
        loadMessages();
        connect(refreshBtn,&QPushButton::clicked,this,[loadMessages](){ loadMessages(); });
        connect(sendBtn,&QPushButton::clicked,this,[appendMessage,addBubble,input,userId](){ QString txt=input->toPlainText(); if(txt.trimmed().isEmpty()) return; if(!appendMessage(userId,txt)){ QMessageBox::warning(nullptr,"提示","写入失败"); return; } input->clear(); addBubble(userId,txt,QDateTime::currentDateTime()); });
        
        rootLayout->addWidget(toolCol); rootLayout->addWidget(plaza,1);
        m_stack->addWidget(mainWidget); m_mainPage = mainWidget; m_stack->setCurrentWidget(mainWidget);
        return;
    }

    // QQ 和 WeChat 服务的通用UI
    QWidget* mainWidget = new QWidget();
    mainWidget->setWindowTitle(service + " - 主界面");
    mainWidget->resize(700, 600);

    QHBoxLayout* rootLayout = new QHBoxLayout(mainWidget);
    rootLayout->setContentsMargins(0, 0, 0, 0);
    rootLayout->setSpacing(0);

    // 左侧工具列
    QWidget* toolCol = new QWidget();
    toolCol->setMinimumWidth(90);
    toolCol->setMaximumWidth(110);
    QVBoxLayout* toolLayout = new QVBoxLayout(toolCol);
    toolLayout->setContentsMargins(8, 8, 8, 8);
    toolLayout->setSpacing(10);

    QLabel* avatarSelf = new QLabel(userId.right(2));
    m_avatarSelf = avatarSelf;
    avatarSelf->installEventFilter(this);
    avatarSelf->setFixedSize(60, 60);
    QString bg = (service == "WeChat") ? "#07C160" : ((service == "Weibo") ? "#F56C6C" : "#12B7F5");
    avatarSelf->setStyleSheet(QString("border-radius:30px; background:%1; color:#fff; font:bold 20px 'Segoe UI'; qproperty-alignment:AlignCenter;").arg(bg));
    toolLayout->addWidget(avatarSelf, 0, Qt::AlignHCenter);

    // 功能按钮
    QPushButton* addFriendBtn = new QPushButton("加好友");
    QPushButton* queryFriendBtn = new QPushButton("查询");
    QPushButton* msgCenterBtn = new QPushButton("消息");
    QPushButton* startGroupBtn = new QPushButton("建群");
    QPushButton* logoutBtn = new QPushButton("退出");
    
    QList<QPushButton*> buttons = { addFriendBtn, queryFriendBtn, msgCenterBtn, startGroupBtn };
    QPushButton* joinGroupBtn = nullptr;
    if (service == "QQ") {
        joinGroupBtn = new QPushButton("加群");
        buttons.insert(1, joinGroupBtn);
    }
    buttons.append(logoutBtn);

    for (QPushButton* b : buttons) {
        b->setFixedHeight(32);
        b->setStyleSheet("font-size:12px;");
        toolLayout->addWidget(b);
    }
    if (joinGroupBtn) {
        connect(joinGroupBtn, &QPushButton::clicked, this, [this, userId, service, msgCenterBtn]() {
            PlatformBcc& plat = m_platformRef(); ServiceBcc* svc = plat.getService(service);
            if (!svc) { QMessageBox::warning(this, "提示", "服务不可用"); return; }
            bool ok = false; QString gid = QInputDialog::getText(this, "申请加入群聊", "请输入群聊ID：", QLineEdit::Normal, "", &ok).trimmed();
            if (!ok || gid.isEmpty()) return;
            QSharedPointer<GroupBcc> g = svc->getGroup(gid);
            if (!g) { QMessageBox::warning(this, "提示", "群不存在"); return; }
            if (g->getJoinRequests().contains(userId)) { QMessageBox::information(this, "提示", "已提交申请，等待审核"); return; }
            if (g->getMembers().contains(userId)) { QMessageBox::information(this, "提示", "您已在该群中"); return; }
            if (g->addJoinRequest(userId)) {
                plat.saveAllGroups("C:/Users/bcc54/Desktop/Code/QQvs/QQvs");
                QMessageBox::information(this, "提示", "申请已提交，等待群主/管理员审核");
            } else QMessageBox::warning(this, "提示", "提交申请失败");
        });
    }
    toolLayout->addStretch();

    QPushButton* switchSvcBtn = new QPushButton("切换服务");
    switchSvcBtn->setFixedHeight(32);
    toolLayout->addWidget(switchSvcBtn);
    connect(switchSvcBtn, &QPushButton::clicked, this, [this, userId]() {
        showServiceSelector(userId);
    });

    QLabel* currentSvcLbl = new QLabel("当前服务: " + service);
    currentSvcLbl->setStyleSheet("color:#666; font-size:11px;");
    currentSvcLbl->setAlignment(Qt::AlignHCenter);
    toolLayout->addWidget(currentSvcLbl);

    // 消息中心未读角标
    QLabel* msgBadge = new QLabel(msgCenterBtn);
    msgBadge->setMinimumSize(16, 16);
    msgBadge->setStyleSheet("background:#ff3b30;color:#fff;border-radius:8px;font-size:10px;padding:0 3px;");
    msgBadge->setAlignment(Qt::AlignCenter);
    msgBadge->hide();
    auto updateMsgBadge = [this, userId, msgBadge, msgCenterBtn, service]() {
        PlatformBcc& plat = m_platformRef();
        int unread = 0;
        if (UserBcc* u = plat.getUser(userId)) {
            unread += u->incomingFriendRequests().size();
            unread += u->rejectedNotices().size();
            if (service == "QQ") { // 仅QQ显示群申请
                ServiceBcc* svc = plat.getService(service);
                if (svc) {
                    const QMap<QString, QSharedPointer<GroupBcc>>& all = svc->groups();
                    for (auto git = all.begin(); git != all.end(); ++git) {
                        QSharedPointer<GroupBcc> g = git.value(); if (!g) continue;
                        if (g->isOwner(userId) || g->isAdmin(userId) || g->getOwner().isEmpty()) {
                            unread += g->getJoinRequests().size();
                        }
                    }
                }
            }
        }
        if (unread > 0) { msgBadge->setText(QString::number(unread)); msgBadge->show(); msgBadge->move(msgCenterBtn->width() - msgBadge->width(), 0); }
        else msgBadge->hide();
    };
    QTimer::singleShot(0, this, [updateMsgBadge]() { updateMsgBadge(); });

    // 中间列表列（好友和群组）
    QWidget* listCol = new QWidget();
    listCol->setMinimumWidth(230);
    listCol->setMaximumWidth(260);
    QVBoxLayout* listLayout = new QVBoxLayout(listCol);
    listLayout->setContentsMargins(8, 8, 8, 8);
    listLayout->setSpacing(6);
    QListWidget* list = new QListWidget();
    list->setSelectionMode(QAbstractItemView::SingleSelection);
    list->setContextMenuPolicy(Qt::CustomContextMenu);
    listLayout->addWidget(list);

    rootLayout->addWidget(toolCol);
    rootLayout->addWidget(listCol);

    // 列表刷新逻辑
    auto refreshList = [this, list, userId, service, bg]() {
        list->clear();
        PlatformBcc& plat = m_platformRef();
        if (UserBcc* u = plat.getUser(userId)) {
            // 刷新好友列表
            for (const QString& fid : u->getFriendsOfService(service)) {
                UserBcc* fu = plat.getUser(fid);
                QString nick = fu && !fu->getNickname().isEmpty() ? fu->getNickname() : fid;
                QWidget* cell = new QWidget(); QHBoxLayout* hl = new QHBoxLayout(cell); hl->setContentsMargins(6, 4, 6, 4);
                QLabel* avatar = new QLabel(fid.right(2)); avatar->setFixedSize(30, 30);
                avatar->setStyleSheet(QString("border-radius:15px;background:%1;color:#fff;font:bold 12px 'Segoe UI';qproperty-alignment:AlignCenter;").arg(bg));
                QVBoxLayout* vinfo = new QVBoxLayout(); vinfo->setContentsMargins(0, 0, 0, 0); vinfo->setSpacing(1);
                QLabel* nickLbl = new QLabel(nick); nickLbl->setStyleSheet("font-weight:bold;color:#222;font-size:13px;");
                QLabel* idLbl = new QLabel(QString("ID: %1").arg(fid)); idLbl->setStyleSheet("color:#666;font-size:11px;");
                vinfo->addWidget(nickLbl); vinfo->addWidget(idLbl);
                hl->addWidget(avatar); hl->addLayout(vinfo); hl->addStretch();
                QListWidgetItem* it = new QListWidgetItem(list); it->setSizeHint(QSize(0, 44)); it->setData(Qt::UserRole, 0); it->setData(Qt::UserRole + 1, fid); list->addItem(it); list->setItemWidget(it, cell);
            }
            // 刷新群组列表
            ServiceBcc* svc = plat.getService(service);
            for (const QString& gid : u->getGroupsOfService(service)) {
                QSharedPointer<GroupBcc> g = svc ? svc->getGroup(gid) : QSharedPointer<GroupBcc>();
                QString gname = g ? g->getName() : gid;
                QWidget* cell = new QWidget(); QHBoxLayout* hl = new QHBoxLayout(cell); hl->setContentsMargins(6, 4, 6, 4);
                QLabel* avatar = new QLabel("群"); avatar->setFixedSize(30, 30);
                avatar->setStyleSheet(QString("border-radius:8px;background:%1;color:#fff;font:bold 11px 'Segoe UI';qproperty-alignment:AlignCenter;").arg(QColor(bg).darker(120).name()));
                QVBoxLayout* vinfo = new QVBoxLayout(); vinfo->setContentsMargins(0, 0, 0, 0); vinfo->setSpacing(1);
                QLabel* nameLbl = new QLabel(gname); nameLbl->setStyleSheet("font-weight:bold;color:#222;font-size:13px;");
                QLabel* idLbl = new QLabel(QString("GID: %1").arg(gid)); idLbl->setStyleSheet("color:#666;font-size:11px;");
                vinfo->addWidget(nameLbl); vinfo->addWidget(idLbl);
                hl->addWidget(avatar); hl->addLayout(vinfo); hl->addStretch();
                QListWidgetItem* it = new QListWidgetItem(list); it->setSizeHint(QSize(0, 44)); it->setData(Qt::UserRole, 1); it->setData(Qt::UserRole + 1, gid); list->addItem(it); list->setItemWidget(it, cell);
            }
        }
    };
    refreshList();

    // 连接功能按钮的信号槽
    connect(logoutBtn, &QPushButton::clicked, this, &QQvs::showLoginPage);

    connect(addFriendBtn, &QPushButton::clicked, this, [this, userId, updateMsgBadge, service]() {
        // 添加好友逻辑
        PlatformBcc& plat = m_platformRef(); bool ok = false; QString targetId = QInputDialog::getText(this, "添加好友", "请输入好友ID：", QLineEdit::Normal, "", &ok); if (!ok || targetId.trimmed().isEmpty()) return; targetId = targetId.trimmed(); if (targetId == userId) { QMessageBox::warning(this, "提示", "不能添加自己！"); return; }
        UserBcc* self = plat.getUser(userId); UserBcc* other = plat.getUser(targetId); if (!other) { QMessageBox::warning(this, "提示", "该用户不存在！"); return; }
        if (self->hasFriendInService(service, targetId)) { QMessageBox::information(this, "提示", "已是好友。"); return; }
        if (self->hasPendingRequestWith(targetId) || other->hasPendingRequestWith(userId)) { QMessageBox::information(this, "提示", "已存在待处理的好友申请。"); return; }
        self->addFriendRequestOutgoing(targetId); other->addFriendRequestIncoming(userId); self->saveToFile("C:/Users/bcc54/Desktop/Code/QQvs/QQvs/" + userId + ".dat"); other->saveToFile("C:/Users/bcc54/Desktop/Code/QQvs/QQvs/" + targetId + ".dat"); QMessageBox::information(this, "提示", "好友申请已发送。"); updateMsgBadge();
    });

    connect(queryFriendBtn, &QPushButton::clicked, this, [this, userId, service]() {
        // 查询好友信息逻辑
        PlatformBcc& plat = m_platformRef(); bool ok = false; QString targetId = QInputDialog::getText(this, "查询好友", "请输入好友ID：", QLineEdit::Normal, "", &ok); if (!ok || targetId.trimmed().isEmpty()) return; targetId = targetId.trimmed(); UserBcc* self = plat.getUser(userId); if (!self || !self->hasFriendInService(service, targetId)) { QMessageBox::warning(this, "提示", QString("该用户不是您的%1好友！").arg(service)); return; } UserBcc* f = plat.getUser(targetId); if (!f) { QMessageBox::warning(this, "提示", "未找到该好友信息！"); return; } QDialog dlg(this); dlg.setWindowTitle(QString("好友信息 - %1").arg(targetId)); QVBoxLayout* v = new QVBoxLayout(&dlg); QString nick = f->getNickname().isEmpty() ? targetId : f->getNickname(); v->addWidget(new QLabel(QString("账号：%1\n昵称：%2\n地点：%3").arg(targetId, nick, f->getLocation()))); if (f->getRegisterDate().isValid()) { int days = f->getRegisterDate().daysTo(QDate::currentDate()); v->addWidget(new QLabel(QString("注册天数：%1 天").arg(days))); } QPushButton* close = new QPushButton("关闭"); v->addWidget(close); connect(close, &QPushButton::clicked, &dlg, &QDialog::accept); dlg.exec();
    });

    connect(msgCenterBtn, &QPushButton::clicked, this, [this, userId, updateMsgBadge, refreshList, service]() {
        // 消息中心逻辑
        PlatformBcc& plat = m_platformRef(); UserBcc* self = plat.getUser(userId); if (!self) return; QDialog dlg(this); dlg.setWindowTitle("消息中心"); dlg.resize(520, 600); QVBoxLayout* root = new QVBoxLayout(&dlg);
        root->addWidget(new QLabel("收到的好友申请：")); QListWidget* reqList = new QListWidget(); root->addWidget(reqList, 4);
        for (const QString& fromId : self->incomingFriendRequests()) {
            UserBcc* fu = plat.getUser(fromId); QString nick = (fu && !fu->getNickname().isEmpty()) ? fu->getNickname() : fromId; QWidget* rowW = new QWidget(); QHBoxLayout* h = new QHBoxLayout(rowW); h->setContentsMargins(4, 2, 4, 2); QLabel* lbl = new QLabel(QString("%1 (%2)").arg(nick, fromId)); QPushButton* okBtn = new QPushButton("通过"); QPushButton* rejBtn = new QPushButton("拒绝"); h->addWidget(lbl); h->addStretch(); h->addWidget(okBtn); h->addWidget(rejBtn); QListWidgetItem* item = new QListWidgetItem(); item->setSizeHint(QSize(0, 32)); reqList->addItem(item); reqList->setItemWidget(item, rowW);
            connect(okBtn, &QPushButton::clicked, this, [this, fromId, userId, &dlg, refreshList, updateMsgBadge, service]() {
                PlatformBcc& plat = m_platformRef();
                UserBcc* self = plat.getUser(userId);
                UserBcc* other = plat.getUser(fromId);
                if (!self || !other) return;
                self->removeIncomingRequest(fromId);
                other->removeOutgoingRequest(userId);
                self->addFriendForService(service, fromId);
                other->addFriendForService(service, userId);
                self->saveToFile("C:/Users/bcc54/Desktop/Code/QQvs/QQvs/" + userId + ".dat");
                other->saveToFile("C:/Users/bcc54/Desktop/Code/QQvs/QQvs/" + fromId + ".dat");
                dlg.close();
                refreshList();
                updateMsgBadge();
            });
            connect(rejBtn, &QPushButton::clicked, this, [this, fromId, userId, &dlg, updateMsgBadge]() {
                PlatformBcc& plat = m_platformRef();
                UserBcc* self = plat.getUser(userId);
                UserBcc* other = plat.getUser(fromId);
                if (!self || !other) return;
                self->removeIncomingRequest(fromId);
                other->removeOutgoingRequest(userId);
                other->addRejectedNotice(userId);
                self->saveToFile("C:/Users/bcc54/Desktop/Code/QQvs/QQvs/" + userId + ".dat");
                other->saveToFile("C:/Users/bcc54/Desktop/Code/QQvs/QQvs/" + fromId + ".dat");
                dlg.close();
                updateMsgBadge();
            });
        }
        if (service == "QQ") {
            root->addWidget(new QLabel("群加入申请：")); QListWidget* groupReqList = new QListWidget(); root->addWidget(groupReqList, 4);
            ServiceBcc* svc = plat.getService(service); if (svc) {
                const QMap<QString, QSharedPointer<GroupBcc>>& all = svc->groups(); for (auto it = all.begin(); it != all.end(); ++it) {
                    QSharedPointer<GroupBcc> g = it.value(); if (!g) continue; if (!(g->isOwner(userId) || g->isAdmin(userId) || g->getOwner().isEmpty())) continue; for (const QString& applier : g->getJoinRequests()) {
                        QWidget* rowW = new QWidget(); QHBoxLayout* h = new QHBoxLayout(rowW); h->setContentsMargins(4, 2, 4, 2); QLabel* lbl = new QLabel(QString("[%1] 申请加入群 %2").arg(applier, g->getId())); QPushButton* agree = new QPushButton("同意"); QPushButton* reject = new QPushButton("拒绝"); h->addWidget(lbl); h->addStretch(); h->addWidget(agree); h->addWidget(reject); QListWidgetItem* item = new QListWidgetItem(); item->setSizeHint(QSize(0, 34)); groupReqList->addItem(item); groupReqList->setItemWidget(item, rowW);
                        connect(agree, &QPushButton::clicked, this, [this, g, applier, service, &dlg, refreshList, updateMsgBadge]() {
                            if (!g->getJoinRequests().contains(applier)) return;
                            g->removeJoinRequest(applier);
                            g->addMember(applier);
                            if (UserBcc* u = m_platformRef().getUser(applier)) {
                                u->joinGroup(service, g->getId());
                                u->saveToFile("C:/Users/bcc54/Desktop/Code/QQvs/QQvs/" + applier + ".dat");
                            }
                            m_platformRef().saveAllGroups("C:/Users/bcc54/Desktop/Code/QQvs/QQvs");
                            refreshList();
                            dlg.close();
                            QMessageBox::information(this, "提示", "已同意加入");
                            updateMsgBadge();
                        });
                        connect(reject, &QPushButton::clicked, this, [this, g, applier, &dlg, updateMsgBadge]() {
                            if (!g->getJoinRequests().contains(applier)) return;
                            g->removeJoinRequest(applier);
                            m_platformRef().saveAllGroups("C:/Users/bcc54/Desktop/Code/QQvs/QQvs");
                            dlg.close();
                            QMessageBox::information(this, "提示", "已拒绝申请");
                            updateMsgBadge();
                        });
                    }
                }
            }
        }
        root->addWidget(new QLabel("我被拒绝的好友申请：")); QListWidget* rejList = new QListWidget(); root->addWidget(rejList, 2); for (const QString& rid : self->rejectedNotices()) { UserBcc* ru = plat.getUser(rid); QString nick = (ru && !ru->getNickname().isEmpty()) ? ru->getNickname() : rid; rejList->addItem(new QListWidgetItem(QString("%1 (%2) - 已拒绝").arg(nick, rid))); }
        QPushButton* close = new QPushButton("关闭"); root->addWidget(close); connect(close, &QPushButton::clicked, &dlg, &QDialog::accept); dlg.exec(); updateMsgBadge();
    });

    // 创建临时子群和邀请好友逻辑
    connect(startGroupBtn, &QPushButton::clicked, this, [this, userId, refreshList, service]() {
        PlatformBcc& plat = m_platformRef(); UserBcc* self = plat.getUser(userId); if (!self) { QMessageBox::warning(this, "提示", "用户不存在"); return; } QSet<QString> friends = self->getFriendsOfService(service); if (friends.isEmpty()) { QMessageBox::information(this, "提示", "暂无好友，无法建群"); return; } QDialog dlg(this); dlg.setWindowTitle("发起群聊 - 选择成员"); dlg.resize(420, 520); QVBoxLayout* root = new QVBoxLayout(&dlg); QHBoxLayout* nameLay = new QHBoxLayout(); nameLay->addWidget(new QLabel("群名称：")); QLineEdit* nameEdit = new QLineEdit(); nameLay->addWidget(nameEdit); root->addLayout(nameLay); QScrollArea* scroll = new QScrollArea(); scroll->setWidgetResizable(true); QWidget* container = new QWidget(); QVBoxLayout* vlist = new QVBoxLayout(container); QList<QCheckBox*> boxes; for (const QString& fid : friends) { UserBcc* fu = plat.getUser(fid); QString nick = (fu && !fu->getNickname().isEmpty()) ? fu->getNickname() : fid; QCheckBox* cb = new QCheckBox(QString("%1 (%2)").arg(nick, fid)); cb->setProperty("uid", fid); boxes << cb; vlist->addWidget(cb); } vlist->addStretch(); scroll->setWidget(container); root->addWidget(new QLabel("请选择要加入的好友：")); root->addWidget(scroll, 1); QHBoxLayout* btnLay = new QHBoxLayout(); QPushButton* okBtn = new QPushButton("完成"); QPushButton* cancelBtn = new QPushButton("取消"); btnLay->addStretch(); btnLay->addWidget(okBtn); btnLay->addWidget(cancelBtn); root->addLayout(btnLay); connect(cancelBtn, &QPushButton::clicked, &dlg, &QDialog::reject); connect(okBtn, &QPushButton::clicked, &dlg, &QDialog::accept); if (dlg.exec() != QDialog::Accepted) return; QStringList members; for (QCheckBox* cb : boxes) if (cb->isChecked()) members << cb->property("uid").toString(); if (members.isEmpty()) { QMessageBox::warning(this, "提示", "请至少选择一位好友"); return; } QString gname = nameEdit->text().trimmed(); if (gname.isEmpty()) gname = QString("临时群%1").arg(QDateTime::currentDateTime().toString("MMddhhmmss")); ServiceBcc* svc = plat.getService(service); if (!svc) { QMessageBox::warning(this, "提示", "服务不可用"); return; } int gidNum = 3000; while (svc->hasGroup(QString::number(gidNum))) ++gidNum; QString newGid = QString::number(gidNum); QSharedPointer<GroupBcc> newGroup(new QQGroupBcc(newGid, gname)); newGroup->setOwner(userId); newGroup->setTemporary(true); newGroup->addMember(userId); if(UserBcc* su=m_platformRef().getUser(userId)) su->joinGroup(service, newGid); for (const QString& mid : members) { newGroup->addMember(mid); if (UserBcc* u = plat.getUser(mid)) u->joinGroup(service, newGid); } svc->addGroup(newGroup); QString base = "C:/Users/bcc54/Desktop/Code/QQvs/QQvs/"; self->saveToFile(base + userId + ".dat"); for (const QString& mid : members) { if (UserBcc* u = plat.getUser(mid)) u->saveToFile(base + mid + ".dat"); } plat.saveAllGroups("C:/Users/bcc54/Desktop/Code/QQvs/QQvs"); QMessageBox::information(this, "提示", QString("临时子群已创建：%1").arg(gname)); refreshList();
    });

    // 右键菜单逻辑
    connect(list, &QListWidget::customContextMenuRequested, this, [this, list, userId, refreshList, service, bg](const QPoint& pos) {
        QListWidgetItem* it = list->itemAt(pos);
        if (!it) return;
        int type = it->data(Qt::UserRole).toInt();
        QString id = it->data(Qt::UserRole + 1).toString();
        PlatformBcc& plat = m_platformRef();
        if (type == 0) { // 好友管理
            QDialog dlg(this); dlg.setWindowTitle(QString("好友管理 - %1").arg(id)); dlg.resize(340, 260); QVBoxLayout* v = new QVBoxLayout(&dlg);
            UserBcc* f = plat.getUser(id);
            if (f) {
                QDate b = f->getBirthDate(); QDate r = f->getRegisterDate();
                QString birthStr = b.isValid() ? b.toString("yyyy-MM-dd") : (r.isValid() ? r.toString("yyyy-MM-dd") : "未登记");
                QLabel* info = new QLabel(QString("账号：%1\n昵称：%2\n地点：%3\n出生日期：%4").arg(id, f->getNickname().isEmpty() ? id : f->getNickname(), f->getLocation(), birthStr));
                info->setStyleSheet("font-size:13px;color:#444;"); v->addWidget(info);
            }
            QPushButton* del = new QPushButton("删除好友"); del->setStyleSheet("background:#ff4d4f;color:#fff;border:0;border-radius:6px;padding:6px 14px;font-size:13px;"); v->addWidget(del);
            connect(del, &QPushButton::clicked, this, [this, &dlg, id, userId, refreshList, service]() {
                PlatformBcc& plat = m_platformRef();
                if (UserBcc* su = plat.getUser(userId)) {
                    su->removeFriendForService(service, id);
                    su->saveToFile("C:/Users/bcc54/Desktop/Code/QQvs/QQvs/" + userId + ".dat");
                }
                if (UserBcc* fu = plat.getUser(id)) {
                    fu->removeFriendForService(service, userId);
                    fu->saveToFile("C:/Users/bcc54/Desktop/Code/QQvs/QQvs/" + id + ".dat");
                }
                refreshList();
                dlg.accept();
            });
            v->addStretch();
            dlg.exec();
        }
        else if (type == 1) { // 群管理
            QDialog dlg(this); dlg.setWindowTitle(QString("群管理 - %1").arg(id)); dlg.resize(420, 420); QVBoxLayout* v = new QVBoxLayout(&dlg);
            ServiceBcc* svc = plat.getService(service); QSharedPointer<GroupBcc> gptr = svc ? svc->getGroup(id) : QSharedPointer<GroupBcc>();
            if (!gptr) { QLabel* nf = new QLabel("群不存在"); nf->setStyleSheet("color:#666;"); v->addWidget(nf); dlg.exec(); return; }
            QLabel* meta = new QLabel(QString("群主：%1 管理员数：%2 类型：%3").arg(gptr->getOwner()).arg(gptr->getAdmins().size()).arg(gptr->isTemporary() ? "临时群" : "正式群")); meta->setStyleSheet("color:#444;font-size:13px;"); v->addWidget(meta);
            QListWidget* members = new QListWidget(); members->setMinimumHeight(180);
            for (const QString& mid : gptr->getMembers()) { // 使用 accessor
                QString role = gptr->isOwner(mid) ? "[群主]" : (gptr->isAdmin(mid) ? "[管理员]" : "");
                UserBcc* mu = plat.getUser(mid); QString nick = (mu && !mu->getNickname().isEmpty()) ? mu->getNickname() : "";
                QListWidgetItem* mIt = new QListWidgetItem(role + mid + (nick.isEmpty()?"":(" ("+nick+")")));
                mIt->setData(Qt::UserRole, mid); members->addItem(mIt);
            }
            v->addWidget(members);
            if (service == "WeChat") {
             
                QHBoxLayout* btnLayout = new QHBoxLayout();
                QPushButton* inviteBtn = new QPushButton("邀请好友");
                QPushButton* quitBtn = new QPushButton("退出群聊");
                QPushButton* kickBtn = new QPushButton("移除成员");
                QPushButton* dissolveBtn = new QPushButton("解散群聊");
                btnLayout->addWidget(inviteBtn); btnLayout->addWidget(quitBtn); btnLayout->addWidget(kickBtn); btnLayout->addWidget(dissolveBtn); v->addLayout(btnLayout);
                bool isOwner = gptr->isOwner(userId); quitBtn->setEnabled(!isOwner); kickBtn->setEnabled(isOwner); dissolveBtn->setEnabled(isOwner);
                connect(inviteBtn, &QPushButton::clicked, this, [this, gptr, id, userId, members, service]() { PlatformBcc& platRef = m_platformRef(); UserBcc* self = platRef.getUser(userId); if (!self) return; QSet<QString> friends = self->getFriendsOfService(service); QSet<QString> already = gptr->getMembers(); QDialog dlg2(this); dlg2.setWindowTitle("邀请好友入群"); QVBoxLayout* root = new QVBoxLayout(&dlg2); QList<QCheckBox*> boxes; for (const QString& fid : friends) { if (already.contains(fid)) continue; UserBcc* fu = platRef.getUser(fid); QString nick = (fu && !fu->getNickname().isEmpty()) ? fu->getNickname() : ""; QCheckBox* cb = new QCheckBox(fid + (nick.isEmpty() ? "" : (" (" + nick + ")"))); cb->setProperty("uid", fid); boxes << cb; root->addWidget(cb); } root->addStretch(); QHBoxLayout* hb = new QHBoxLayout(); QPushButton* ok = new QPushButton("完成"); QPushButton* cancel = new QPushButton("取消"); hb->addStretch(); hb->addWidget(ok); hb->addWidget(cancel); root->addLayout(hb); connect(cancel, &QPushButton::clicked, &dlg2, &QDialog::reject); connect(ok, &QPushButton::clicked, &dlg2, &QDialog::accept); if (dlg2.exec() != QDialog::Accepted) return; QStringList toAdd; for (auto* cb : boxes) if (cb->isChecked()) toAdd << cb->property("uid").toString(); if (toAdd.isEmpty()) return; for (const QString& fid : toAdd) { gptr->addMember(fid); if (UserBcc* fu = platRef.getUser(fid)) { fu->joinGroup(service, id); fu->saveToFile("C:/Users/bcc54/Desktop/Code/QQvs/QQvs/" + fid + ".dat"); } QListWidgetItem* mIt = new QListWidgetItem(fid); mIt->setData(Qt::UserRole, fid); members->addItem(mIt); } platRef.saveAllGroups("C:/Users/bcc54/Desktop/Code/QQvs/QQvs"); });
                connect(quitBtn, &QPushButton::clicked, this, [this, gptr, userId, id, service]() { if(!gptr->getMembers().contains(userId)) return; if(gptr->isOwner(userId)){ QMessageBox::warning(this,"提示","群主不能直接退出，请解散群聊。"); return; } gptr->removeMember(userId); if(UserBcc* self=m_platformRef().getUser(userId)){ self->leaveGroup(service,id); self->saveToFile("C:/Users/bcc54/Desktop/Code/QQvs/QQvs/"+userId+".dat"); } m_platformRef().saveAllGroups("C:/Users/bcc54/Desktop/Code/QQvs/QQvs"); QMessageBox::information(this,"提示","已退出群聊。"); });
                connect(kickBtn, &QPushButton::clicked, this, [this, gptr, userId, members, id, service]() { if(!gptr->isOwner(userId)) return; QListWidgetItem* cur = members->currentItem(); if(!cur){ QMessageBox::warning(this,"提示","请选择要移除的成员。"); return; } QString target = cur->data(Qt::UserRole).toString(); if(target==userId){ QMessageBox::warning(this,"提示","不能移除自己。"); return; } if(gptr->isOwner(target)){ QMessageBox::warning(this,"提示","不能移除群主。"); return; } if(gptr->removeMember(target)){ if(UserBcc* tu=m_platformRef().getUser(target)){ tu->leaveGroup(service,id); tu->saveToFile("C:/Users/bcc54/Desktop/Code/QQvs/QQvs/"+target+".dat"); } delete members->takeItem(members->row(cur)); m_platformRef().saveAllGroups("C:/Users/bcc54/Desktop/Code/QQvs/QQvs"); QMessageBox::information(this,"提示","成员已移除。"); } });
                connect(dissolveBtn, &QPushButton::clicked, this, [this, gptr, userId, id, service, v]() { if(!gptr->isOwner(userId)) return; if(QMessageBox::question(this,"解散群聊","确定要解散该群?",QMessageBox::Yes|QMessageBox::No)!=QMessageBox::Yes) return; for(const QString& mid: gptr->getMembers()){ if(UserBcc* mu=m_platformRef().getUser(mid)){ mu->leaveGroup(service,id); mu->saveToFile("C:/Users/bcc54/Desktop/Code/QQvs/QQvs/"+mid+".dat"); } } if(ServiceBcc* svc2=m_platformRef().getService(service)) svc2->removeGroup(id); QFile::remove("C:/Users/bcc54/Desktop/Code/QQvs/QQvs/group_"+service+"_"+id+".grp"); m_platformRef().saveAllGroups("C:/Users/bcc54/Desktop/Code/QQvs/QQvs"); if(v&&v->parentWidget()) v->parentWidget()->close(); QMessageBox::information(this,"提示","该群已解散。"); });
            } else if (service == "QQ") {
               
                QGridLayout* grid = new QGridLayout(); grid->setSpacing(8); grid->setContentsMargins(0,0,0,0);
                QPushButton* setAdminBtn = new QPushButton("设为管理员");
                QPushButton* removeAdminBtn = new QPushButton("取消管理员");
                QPushButton* kickBtn = new QPushButton("移除成员");
                QPushButton* subGroupBtn = new QPushButton("建立临时子群");
                QPushButton* inviteBtn = new QPushButton("邀请好友");
                QPushButton* quitBtn = new QPushButton("退出群聊");
                QPushButton* dissolveBtn = new QPushButton("解散群聊");
                QList<QPushButton*> all{setAdminBtn,removeAdminBtn,kickBtn,subGroupBtn,inviteBtn,quitBtn,dissolveBtn};
                for(auto* b: all) b->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
                grid->addWidget(setAdminBtn,0,0); grid->addWidget(removeAdminBtn,0,1); grid->addWidget(kickBtn,0,2);
                grid->addWidget(subGroupBtn,1,0); grid->addWidget(inviteBtn,1,1); grid->addWidget(quitBtn,1,2);
                grid->addWidget(dissolveBtn,2,0,1,3);
                v->addLayout(grid);
                auto canManage = [&](const QString& uid){ return gptr->canManage(uid); };
                bool selfMgr = canManage(userId);
                if (gptr->isTemporary()) {
                    setAdminBtn->hide(); removeAdminBtn->hide(); kickBtn->hide(); subGroupBtn->hide(); inviteBtn->hide();
                    quitBtn->setEnabled(!gptr->isOwner(userId)); dissolveBtn->setEnabled(gptr->isOwner(userId));
                } else {
                    setAdminBtn->setEnabled(selfMgr); removeAdminBtn->setEnabled(selfMgr); kickBtn->setEnabled(selfMgr);
                    subGroupBtn->setEnabled(selfMgr && gptr->canCreateSubgroup()); inviteBtn->setEnabled(true);
                    quitBtn->setEnabled(!gptr->isOwner(userId)); dissolveBtn->setEnabled(gptr->isOwner(userId));
                }
                connect(setAdminBtn,&QPushButton::clicked,this,[this,gptr,selfMgr,members](){ if(!selfMgr||gptr->isTemporary()) return; QListWidgetItem* cur=members->currentItem(); if(!cur) return; QString target=cur->data(Qt::UserRole).toString(); if(gptr->isOwner(target)||gptr->isAdmin(target)) return; if(gptr->addAdmin(target)){ cur->setText("[管理员]"+cur->text()); m_platformRef().saveAllGroups("C:/Users/bcc54/Desktop/Code/QQvs/QQvs"); } });
                connect(removeAdminBtn,&QPushButton::clicked,this,[this,gptr,selfMgr,members](){ if(!selfMgr||gptr->isTemporary()) return; QListWidgetItem* cur=members->currentItem(); if(!cur) return; QString target=cur->data(Qt::UserRole).toString(); if(gptr->isOwner(target)||!gptr->isAdmin(target)) return; if(gptr->removeAdmin(target)){ cur->setText(target); m_platformRef().saveAllGroups("C:/Users/bcc54/Desktop/Code/QQvs/QQvs"); } });
                connect(kickBtn,&QPushButton::clicked,this,[this,gptr,selfMgr,members,id,service](){ if(!selfMgr||gptr->isTemporary()) return; QListWidgetItem* cur=members->currentItem(); if(!cur) return; QString target=cur->data(Qt::UserRole).toString(); if(gptr->isOwner(target)) return; if(!gptr->removeMember(target)) return; if(UserBcc* tu=m_platformRef().getUser(target)){ tu->leaveGroup(service,id); tu->saveToFile("C:/Users/bcc54/Desktop/Code/QQvs/QQvs/"+target+".dat"); } delete members->takeItem(members->row(cur)); m_platformRef().saveAllGroups("C:/Users/bcc54/Desktop/Code/QQvs/QQvs"); });
                connect(subGroupBtn,&QPushButton::clicked,this,[this,gptr,userId,id,refreshList,service](){ if(gptr->isTemporary()||!gptr->isOwner(userId)) return; QDialog sel(this); sel.setWindowTitle("建立临时子群"); sel.resize(360,480); QVBoxLayout* root=new QVBoxLayout(&sel); root->addWidget(new QLabel("请选择成员(至少1人)：")); QList<QCheckBox*> boxes; for(const QString& mid: gptr->getMembers()){ if(mid==userId) continue; UserBcc* mu=m_platformRef().getUser(mid); QString nick=(mu && !mu->getNickname().isEmpty())?mu->getNickname():""; QCheckBox* cb=new QCheckBox(mid+(nick.isEmpty()?"":(" ("+nick+")"))); cb->setProperty("uid",mid); boxes<<cb; root->addWidget(cb);} root->addStretch(); QHBoxLayout* hb=new QHBoxLayout(); QPushButton* ok=new QPushButton("完成"); QPushButton* cancel=new QPushButton("取消"); hb->addStretch(); hb->addWidget(ok); hb->addWidget(cancel); root->addLayout(hb); connect(cancel,&QPushButton::clicked,&sel,&QDialog::reject); connect(ok,&QPushButton::clicked,&sel,&QDialog::accept); if(sel.exec()!=QDialog::Accepted) return; QStringList selMembers; for(auto* cb: boxes) if(cb->isChecked()) selMembers<<cb->property("uid").toString(); if(selMembers.isEmpty()){ QMessageBox::warning(this,"提示","请选择至少一名成员。"); return; } ServiceBcc* svc2=m_platformRef().getService(service); if(!svc2) return; int gidNum=5000; while(svc2->hasGroup(QString::number(gidNum))) ++gidNum; QString newId=QString::number(gidNum); QSharedPointer<GroupBcc> sub(new QQGroupBcc(newId, QString("临时讨论%1").arg(newId))); sub->setOwner(userId); sub->setTemporary(true); sub->addMember(userId); if(UserBcc* su=m_platformRef().getUser(userId)) su->joinGroup(service, newId); for(const QString& m: selMembers){ sub->addMember(m); if(UserBcc* u=m_platformRef().getUser(m)) u->joinGroup(service, newId); } svc2->addGroup(sub); m_platformRef().saveAllGroups("C:/Users/bcc54/Desktop/Code/QQvs/QQvs"); QString base="C:/Users/bcc54/Desktop/Code/QQvs/QQvs/"; if(UserBcc* su=m_platformRef().getUser(userId)) su->saveToFile(base+userId+".dat"); for(const QString& m: selMembers){ if(UserBcc* u=m_platformRef().getUser(m)) u->saveToFile(base+m+".dat"); } QMessageBox::information(this,"提示","临时子群已创建。"); refreshList(); });
                connect(quitBtn,&QPushButton::clicked,this,[this,gptr,userId,id,service](){ if(!gptr->getMembers().contains(userId)) return; if(gptr->isOwner(userId)){ QMessageBox::warning(this,"提示","群主不能直接退出，请解散群聊。"); return; } gptr->removeMember(userId); if(UserBcc* self=m_platformRef().getUser(userId)){ self->leaveGroup(service,id); self->saveToFile("C:/Users/bcc54/Desktop/Code/QQvs/QQvs/"+userId+".dat"); } m_platformRef().saveAllGroups("C:/Users/bcc54/Desktop/Code/QQvs/QQvs"); QMessageBox::information(this,"提示","已退出群聊。"); });
                connect(dissolveBtn,&QPushButton::clicked,this,[this,gptr,userId,id,service,v](){ if(!gptr->isOwner(userId)) return; if(QMessageBox::question(this,"解散群聊","确定要解散该群?",QMessageBox::Yes|QMessageBox::No)!=QMessageBox::Yes) return; for(const QString& mid: gptr->getMembers()){ if(UserBcc* mu=m_platformRef().getUser(mid)){ mu->leaveGroup(service,id); mu->saveToFile("C:/Users/bcc54/Desktop/Code/QQvs/QQvs/"+mid+".dat"); } } if(ServiceBcc* svc2=m_platformRef().getService(service)) svc2->removeGroup(id); QFile::remove("C:/Users/bcc54/Desktop/Code/QQvs/QQvs/group_"+service+"_"+id+".grp"); m_platformRef().saveAllGroups("C:/Users/bcc54/Desktop/Code/QQvs/QQvs"); if(v&&v->parentWidget()) v->parentWidget()->close(); QMessageBox::information(this,"提示","该群已解散。"); });
            }
            v->addStretch();
            dlg.exec();
        }
    });

    // 聊天功能
    connect(list, &QListWidget::itemClicked, this, [this, userId, service, bg](QListWidgetItem* it) {
        if (!it) return;
        int type = it->data(Qt::UserRole).toInt();
        QString targetId = it->data(Qt::UserRole + 1).toString();
        PlatformBcc& plat = m_platformRef();
        bool isFriend = (type == 0);
        bool isGroup  = (type == 1);
        QString chatFile;
        if (isFriend){
            if(!plat.getUser(targetId)) { QMessageBox::warning(this,"提示","好友不存在"); return; }
            QString a=userId,b=targetId; if(a>b) std::swap(a,b); // 排序保证双方文件名一致
            chatFile = QString("C:/Users/bcc54/Desktop/Code/QQvs/QQvs/chat_%1_%2_%3.log").arg(service,a,b);
        } else if(isGroup){
            ServiceBcc* svc = plat.getService(service);
            if(!svc || !svc->getGroup(targetId)) { QMessageBox::warning(this,"提示","群不存在"); return; }
            chatFile = QString("C:/Users/bcc54/Desktop/Code/QQvs/QQvs/chat_group_%1_%2.log").arg(service,targetId);
        } else return;

        QDialog dlg(this);
        dlg.setWindowTitle(isFriend ? QString("聊天 - %1").arg(targetId) : QString("群聊 - %1").arg(targetId));
        dlg.resize(560,640);
        QVBoxLayout* root = new QVBoxLayout(&dlg);
        QScrollArea* scroll = new QScrollArea();
        scroll->setWidgetResizable(true);
        scroll->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
        QWidget* msgContainer = new QWidget();
        QVBoxLayout* msgLayout = new QVBoxLayout(msgContainer);
        msgLayout->setContentsMargins(12,12,12,12);
        msgLayout->setSpacing(10);
        scroll->setWidget(msgContainer);
        root->addWidget(scroll,1);

        QHBoxLayout* inputBar = new QHBoxLayout();
        QTextEdit* input = new QTextEdit(); input->setPlaceholderText("输入消息..."); input->setFixedHeight(90);
        QPushButton* sendBtn = new QPushButton("发送");
        QPushButton* refreshBtn = new QPushButton("刷新");
        inputBar->addWidget(input,1);
        QVBoxLayout* side = new QVBoxLayout(); side->addWidget(sendBtn); side->addWidget(refreshBtn); side->addStretch();
        inputBar->addLayout(side);
        root->addLayout(inputBar);

        auto esc = [](QString s){ return s.replace("\n","\\n"); };
        auto unesc = [](QString s){ return s.replace("\\n","\n"); };
        // 捕获 bg 颜色避免未捕获错误
        auto addBubble = [scroll,msgLayout,userId,bg](const QString& from,const QString& text,const QDateTime& ts){
            bool mine = (from==userId);
            QWidget* wrap = new QWidget(); QHBoxLayout* hl = new QHBoxLayout(wrap); hl->setContentsMargins(0,0,0,0); hl->setSpacing(8);
            QLabel* avatar = new QLabel(from.right(2)); avatar->setFixedSize(36,36);
            avatar->setStyleSheet(QString("border-radius:18px;%1color:#fff;font:bold 12px 'Segoe UI';qproperty-alignment:AlignCenter;").arg(mine?QString("background:%1;").arg(bg):"background:#999;"));
            QString rich = QString("<div style='font-size:11px;color:#666;margin-bottom:2px;'>%1</div>%2")
                           .arg(ts.toString("HH:mm:ss"), text.toHtmlEscaped().replace("\n","<br/>"));
            QLabel* bubble = new QLabel(rich); bubble->setTextFormat(Qt::RichText); bubble->setWordWrap(true);
            bubble->setStyleSheet(QString("background:%1;border-radius:6px;padding:4px 10px;font-size:12px;color:#222;max-width:460px;line-height:1.25;")
                                  .arg(mine?QColor(bg).lighter(140).name():"#EDEDED"));
            if(mine){ hl->addStretch(); hl->addWidget(bubble); hl->addWidget(avatar);} else { hl->addWidget(avatar); hl->addWidget(bubble); hl->addStretch(); }
            msgLayout->addWidget(wrap);
            QTimer::singleShot(0,[scroll](){ if(auto sb=scroll->verticalScrollBar()) sb->setValue(sb->maximum()); });
        };

        auto loadMessages = [msgLayout,scroll,chatFile,unesc,addBubble](){
            while(QLayoutItem* child = msgLayout->takeAt(0)) { if(child->widget()) child->widget()->deleteLater(); delete child; }
            QFile f(chatFile); if(!f.exists()||!f.open(QIODevice::ReadOnly|QIODevice::Text)) return; QTextStream in(&f);
            while(!in.atEnd()){ QString line=in.readLine(); if(line.trimmed().isEmpty()) continue; QStringList parts=line.split('\t'); if(parts.size()<3) continue; QDateTime ts=QDateTime::fromString(parts[0],Qt::ISODate); addBubble(parts[1], unesc(line.section('\t',2)), ts.isValid()?ts:QDateTime::currentDateTime()); }
            f.close();
        };
        auto appendMessage = [chatFile,esc](const QString& from,const QString& text){ QFile f(chatFile); if(!f.open(QIODevice::Append|QIODevice::Text)) return false; QTextStream out(&f); out<<QDateTime::currentDateTime().toString(Qt::ISODate)<<'\t'<<from<<'\t'<<esc(text)<<'\n'; f.close(); return true; };

        loadMessages();
        connect(refreshBtn,&QPushButton::clicked,this,[loadMessages](){ loadMessages(); });
        connect(sendBtn,&QPushButton::clicked,this,[appendMessage,addBubble,input,userId](){ QString txt=input->toPlainText(); if(txt.trimmed().isEmpty()) return; if(!appendMessage(userId,txt)){ QMessageBox::warning(nullptr,"提示","写入失败"); return;} input->clear(); addBubble(userId,txt,QDateTime::currentDateTime()); });

        dlg.exec();
    });

    m_stack->addWidget(mainWidget);
    m_mainPage = mainWidget;
    m_stack->setCurrentWidget(mainWidget);
}


void QQvs::showMainPage(const QString& userId)
{
    if (m_mainPage) {
        m_stack->removeWidget(m_mainPage);
        delete m_mainPage;
        m_mainPage = nullptr;
    }
    showServiceMain(userId, "QQ");
}

/**
 * @brief 显示登录页面。
 *
 * 清理当前主页面，并将堆栈窗口切换回索引为0的登录页面。
 */
void QQvs::showLoginPage()
{
    if (m_mainPage) {
        m_stack->removeWidget(m_mainPage);
        delete m_mainPage;
        m_mainPage = nullptr;
    }
    m_stack->setCurrentIndex(0); // 切换到登录界面
}

/**
 * @brief 获取平台单例的引用。
 *
 * 如果平台实例 (m_platformPtr) 尚未创建，则创建它并加载演示数据。
 * 这是一个简化的单例模式实现。
 * @return 对 PlatformBcc 实例的引用。
 */
PlatformBcc& QQvs::m_platformRef()
{
    if (!m_platformPtr) {
        m_platformPtr = new PlatformBcc();
        m_platformPtr->loadDemo();
    }
    return *m_platformPtr;
}

/**
 * @brief 事件过滤器，用于处理特定UI组件的事件。
 *
 * 目前主要用于捕获用户头像 (m_avatarSelf) 的点击事件，以显示个人信息对话框。
 * @param watched 被监视的对象。
 * @param event 发生的事件。
 * @return 如果事件被处理则返回 true，否则调用基类的实现。
 */
bool QQvs::eventFilter(QObject* watched, QEvent* event)
{
    if (watched == m_avatarSelf && event->type() == QEvent::MouseButtonPress) {
        QMouseEvent* me = static_cast<QMouseEvent*>(event);
        if (me->button() == Qt::LeftButton) {
            // 显示个人信息对话框
            PlatformBcc& plat = m_platformRef();
            UserBcc* u = plat.getUser(m_currentUserId);
            if (!u) return false;
            QDialog dlg(this);
            dlg.setWindowTitle("个人信息");
            dlg.resize(360, 340);
            QVBoxLayout* root = new QVBoxLayout(&dlg);
            QString nick = u->getNickname().isEmpty()?u->getId():u->getNickname();
            QFormLayout* form = new QFormLayout();
            QLineEdit* nickEdit = new QLineEdit(nick);
            QLabel* idLbl = new QLabel(u->getId());
            QLabel* locLbl = new QLabel(u->getLocation());
            QLabel* regLbl = new QLabel(u->getRegisterDate().isValid()?u->getRegisterDate().toString("yyyy-MM-dd"):"-");
            QLabel* birthLbl = new QLabel(u->getBirthDate().isValid()?u->getBirthDate().toString("yyyy-MM-dd"):regLbl->text());
            form->addRow("账号：", idLbl);
            form->addRow("昵称：", nickEdit);
            form->addRow("地点：", locLbl);
            form->addRow("注册日期：", regLbl);
            form->addRow("出生日期：", birthLbl);
            // 统计好友与群数量
            QSet<QString> friends = u->getFriendsOfService(m_currentService);
            QSet<QString> groups = u->getGroupsOfService(m_currentService);
            QLabel* statLbl = new QLabel(QString("好友数：%1  群数：%2").arg(friends.size()).arg(groups.size()));
            root->addLayout(form);
            root->addWidget(statLbl);
            QHBoxLayout* btns = new QHBoxLayout();
            btns->addStretch();
            QPushButton* saveBtn = new QPushButton("保存");
            QPushButton* closeBtn = new QPushButton("关闭");
            btns->addWidget(saveBtn);
            btns->addWidget(closeBtn);
            root->addLayout(btns);
            connect(closeBtn,&QPushButton::clicked,&dlg,&QDialog::reject);
            connect(saveBtn,&QPushButton::clicked,&dlg,[&,nickEdit,u](){
                QString newNick = nickEdit->text().trimmed();
                if(newNick.isEmpty()) {
                    QMessageBox::warning(&dlg, "提示", "昵称不能为空");
                    return;
                }
                if(newNick != u->getNickname()) {
                    u->setNickname(newNick);
                    u->saveToFile("C:/Users/bcc54/Desktop/Code/QQvs/QQvs/"+u->getId()+".dat");
                    if(m_avatarSelf) m_avatarSelf->setText(u->getId().right(2));
                }
                dlg.accept();
            });
            dlg.exec();
            return true; // 事件已处理
        }
    }
    return QMainWindow::eventFilter(watched, event);
}

#include "UserBcc.h"
#include <QFile>
#include <QTextStream>

// UserBcc: 面向对象设计说明
//  - 封装: 用户属性(昵称/地点/日期/好友/群/申请状态)通过访问器与操作方法集中管理，外部不能直接操作内部映射结构。
//  - 组合: 使用 QMap<QString,QSet<QString>> 将不同服务下的好友与群集合聚合在一个对象内，体现按服务维度的关系分组。
//  - 职责单一: 仅负责“用户自身状态”的持久化与修改，不处理 UI 展示或跨用户策略。
//  - 不变量维护: addFriendForService 建立好友即清理申请/拒绝记录，保持状态一致；save/load 使内存与文件格式对应。
//  - 可扩展点: 未来可加入权限验证、事件回调、统计缓存等而不影响外部调用接口。

UserBcc::UserBcc(const QString& userId)
    : m_userId(userId)
{
}

// 基础只读 ID (身份不允许被外部直接修改, load 可覆盖来自文件)
const QString& UserBcc::getId() const { return m_userId; }

// 昵称封装访问(未来可加入合法性校验)
const QString& UserBcc::getNickname() const { return m_nickname; }
void UserBcc::setNickname(const QString& nickname) { m_nickname = nickname; }

// 生日/注册日期封装(统一入口保证格式/有效性扩展空间)
const QDate& UserBcc::getBirthDate() const { return m_birthDate; }
void UserBcc::setBirthDate(const QDate& date) { m_birthDate = date; }

const QDate& UserBcc::getRegisterDate() const { return m_registerDate; }
void UserBcc::setRegisterDate(const QDate& date) { m_registerDate = date; }

// 地点信息封装
const QString& UserBcc::getLocation() const { return m_location; }
void UserBcc::setLocation(const QString& location) { m_location = location; }

// 好友集合的只读(返回副本, 防止外部直接改内部结构; 若需性能可提供 const 引用 + 迭代器接口)
QSet<QString> UserBcc::getFriendsOfService(const QString& serviceName) const
{
    return m_serviceToFriends.value(serviceName);
}

// 添加好友: 同时维护申请状态一致性(封装 + 不变量维护)
void UserBcc::addFriendForService(const QString& serviceName, const QString& friendUserId)
{
    m_serviceToFriends[serviceName].insert(friendUserId);
    // 成为好友后清理可能存在的申请/拒绝记录
    m_incomingFriendRequests.remove(friendUserId);
    m_outgoingFriendRequests.remove(friendUserId);
    m_rejectedNotices.remove(friendUserId);
}

// 删除好友: 当某服务下好友集合为空时移除该服务键(保持映射精简)
void UserBcc::removeFriendForService(const QString& serviceName, const QString& friendUserId)
{
    if (m_serviceToFriends.contains(serviceName)) {
        m_serviceToFriends[serviceName].remove(friendUserId);
        if (m_serviceToFriends[serviceName].isEmpty()) {
            m_serviceToFriends.remove(serviceName);
        }
    }
}

// 查询好友存在性(封装集合操作)
bool UserBcc::hasFriendInService(const QString& serviceName, const QString& friendUserId) const
{
    return m_serviceToFriends.value(serviceName).contains(friendUserId);
}

// 群集合访问(返回副本, 保护内部结构)
QSet<QString> UserBcc::getGroupsOfService(const QString& serviceName) const
{
    return m_serviceToGroups.value(serviceName);
}

// 加入群: 简单插入(未来可加入权限/验证)
void UserBcc::joinGroup(const QString& serviceName, const QString& groupId)
{
    m_serviceToGroups[serviceName].insert(groupId);
}

// 离开群: 空集合清理键(与好友逻辑一致)
void UserBcc::leaveGroup(const QString& serviceName, const QString& groupId)
{
    if (m_serviceToGroups.contains(serviceName)) {
        m_serviceToGroups[serviceName].remove(groupId);
        if (m_serviceToGroups[serviceName].isEmpty()) {
            m_serviceToGroups.remove(serviceName);
        }
    }
}

// 开通服务: 简单集合插入(未来可添加状态/时间戳)
void UserBcc::addOpenedService(const QString& serviceName)
{
    m_openedServices.insert(serviceName);
}

// 返回已开通服务集合引用(只读)
const QSet<QString>& UserBcc::openedServices() const
{
    return m_openedServices;
}

// 添加收到的好友申请: 校验避免重复/自申请/已是好友; 互相申请时留待外部确认建立好友
bool UserBcc::addFriendRequestIncoming(const QString& fromUserId)
{
    if (fromUserId == m_userId) return false;
    if (m_incomingFriendRequests.contains(fromUserId)) return false;
    if (m_serviceToFriends.value("QQ").contains(fromUserId)) return false; // 示例: 目前只检查 QQ 好友(可扩展为所有服务)
    m_incomingFriendRequests.insert(fromUserId);
    if (m_outgoingFriendRequests.contains(fromUserId)) {
        // 互相申请 -> 外部逻辑决定是否直接转换为好友
        m_outgoingFriendRequests.remove(fromUserId);
    }
    return true;
}

// 添加发出的好友申请: 同样校验; 未来可加入节流/频率限制
bool UserBcc::addFriendRequestOutgoing(const QString& toUserId)
{
    if (toUserId == m_userId) return false;
    if (m_outgoingFriendRequests.contains(toUserId)) return false;
    if (m_serviceToFriends.value("QQ").contains(toUserId)) return false;
    m_outgoingFriendRequests.insert(toUserId);
    return true;
}

// 申请集合移除封装
void UserBcc::removeIncomingRequest(const QString& fromUserId)
{
    m_incomingFriendRequests.remove(fromUserId);
}

void UserBcc::removeOutgoingRequest(const QString& toUserId)
{
    m_outgoingFriendRequests.remove(toUserId);
}

// 被拒绝通知: 加入并清理对应 outgoing 申请
void UserBcc::addRejectedNotice(const QString& userId)
{
    if (userId != m_userId) {
        m_rejectedNotices.insert(userId);
        m_outgoingFriendRequests.remove(userId);
    }
}

void UserBcc::removeRejectedNotice(const QString& userId)
{
    m_rejectedNotices.remove(userId);
}

// 是否存在与对方的待处理申请(双向任一)
bool UserBcc::hasPendingRequestWith(const QString& otherId) const
{
    return m_incomingFriendRequests.contains(otherId) || m_outgoingFriendRequests.contains(otherId);
}

// 持久化: saveToFile 封装序列化逻辑(开放扩展为 JSON/二进制); 成员顺序与 load 对应, 保持可读性
bool UserBcc::saveToFile(const QString& filePath) const
{
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
        return false;

    QTextStream out(&file);
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    out.setCodec("UTF-8");
#else
    out.setEncoding(QStringConverter::Utf8);
#endif

    // 基本信息字段
    out << "userid:" << m_userId << "\n";
    out << "nickname:" << m_nickname << "\n";
    out << "birthdate:" << m_birthDate.toString(Qt::ISODate) << "\n";
    out << "registerdate:" << m_registerDate.toString(Qt::ISODate) << "\n";
    out << "location:" << m_location << "\n";

    // 好友: 按服务序列化
    for (auto it = m_serviceToFriends.constBegin(); it != m_serviceToFriends.constEnd(); ++it) {
        out << "friends:" << it.key() << ":";
        const QSet<QString>& set = it.value();
        bool first = true;
        for (const QString& fid : set) {
            if (!first) out << ",";
            out << fid;
            first = false;
        }
        out << "\n";
    }

    // 群: 按服务序列化
    for (auto it = m_serviceToGroups.constBegin(); it != m_serviceToGroups.constEnd(); ++it) {
        out << "groups:" << it.key() << ":";
        const QSet<QString>& set = it.value();
        bool first = true;
        for (const QString& gid : set) {
            if (!first) out << ",";
            out << gid;
            first = false;
        }
        out << "\n";
    }

    // 已开通服务
    if (!m_openedServices.isEmpty()) {
        out << "opened_services:";
        bool first = true;
        for (const QString& s : m_openedServices) {
            if (!first) out << ",";
            out << s;
            first = false;
        }
        out << "\n";
    }

    // 申请与拒绝集合(增量信息, 用于恢复界面状态)
    if (!m_incomingFriendRequests.isEmpty()) {
        out << "incoming_requests:";
        bool first = true;
        for (const QString& id : m_incomingFriendRequests) {
            if (!first) out << ","; out << id; first = false;
        }
        out << "\n";
    }
    if (!m_outgoingFriendRequests.isEmpty()) {
        out << "outgoing_requests:";
        bool first = true;
        for (const QString& id : m_outgoingFriendRequests) {
            if (!first) out << ","; out << id; first = false;
        }
        out << "\n";
    }
    if (!m_rejectedNotices.isEmpty()) {
        out << "rejected_notices:";
        bool first = true;
        for (const QString& id : m_rejectedNotices) {
            if (!first) out << ","; out << id; first = false;
        }
        out << "\n";
    }

    file.close();
    return true;
}

// 反序列化: loadFromFile 清理旧状态后逐行解析(保证一致性); 使用键前缀匹配提高可维护性
bool UserBcc::loadFromFile(const QString& filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return false;

    QTextStream in(&file);
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    in.setCodec("UTF-8");
#else
    in.setEncoding(QStringConverter::Utf8);
#endif

    // 清理旧状态(避免残留数据)
    m_serviceToFriends.clear();
    m_serviceToGroups.clear();
    m_openedServices.clear();
    m_incomingFriendRequests.clear();
    m_outgoingFriendRequests.clear();
    m_rejectedNotices.clear();

    while (!in.atEnd()) {
        QString line = in.readLine();
        if (line.startsWith("userid:")) {
            m_userId = line.mid(7);
        } else if (line.startsWith("nickname:")) {
            m_nickname = line.mid(9);
        } else if (line.startsWith("birthdate:")) {
            m_birthDate = QDate::fromString(line.mid(10), Qt::ISODate);
        } else if (line.startsWith("registerdate:")) {
            m_registerDate = QDate::fromString(line.mid(13), Qt::ISODate);
        } else if (line.startsWith("location:")) {
            m_location = line.mid(9);
        } else if (line.startsWith("friends:")) {
            QString rest = line.mid(8);
            int p = rest.indexOf(':');
            if (p > 0) {
                QString svc = rest.left(p);
                QString list = rest.mid(p+1);
                if (!list.isEmpty()) {
                    QStringList items = list.split(',', Qt::SkipEmptyParts);
                    for (const QString& it : items) m_serviceToFriends[svc].insert(it);
                }
            }
        } else if (line.startsWith("groups:")) {
            QString rest = line.mid(7);
            int p = rest.indexOf(':');
            if (p > 0) {
                QString svc = rest.left(p);
                QString list = rest.mid(p+1);
                if (!list.isEmpty()) {
                    QStringList items = list.split(',', Qt::SkipEmptyParts);
                    for (const QString& it : items) m_serviceToGroups[svc].insert(it);
                }
            }
        } else if (line.startsWith("opened_services:")) {
            QString list = line.mid(QString("opened_services:").length());
            if (!list.isEmpty()) {
                QStringList items = list.split(',', Qt::SkipEmptyParts);
                for (const QString& it : items) m_openedServices.insert(it);
            }
        } else if (line.startsWith("incoming_requests:")) {
            QString list = line.mid(QString("incoming_requests:").length());
            if (!list.isEmpty()) {
                QStringList items = list.split(',', Qt::SkipEmptyParts);
                for (const QString& it : items) m_incomingFriendRequests.insert(it);
            }
        } else if (line.startsWith("outgoing_requests:")) {
            QString list = line.mid(QString("outgoing_requests:").length());
            if (!list.isEmpty()) {
                QStringList items = list.split(',', Qt::SkipEmptyParts);
                for (const QString& it : items) m_outgoingFriendRequests.insert(it);
            }
        } else if (line.startsWith("rejected_notices:")) {
            QString list = line.mid(QString("rejected_notices:").length());
            if (!list.isEmpty()) {
                QStringList items = list.split(',', Qt::SkipEmptyParts);
                for (const QString& it : items) m_rejectedNotices.insert(it);
            }
        }
    }

    file.close();
    return true;
}



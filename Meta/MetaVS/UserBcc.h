#pragma once

#include <QString>
#include <QDate>
#include <QSet>
#include <QMap>
#include <QVector>

class GroupBcc;

/**
 * @class UserBcc
 * 面向对象设计说明:
 *  - 封装: 用户的基础属性与社交关系通过访问器与操作方法暴露, 外部无法直接修改内部映射与集合。
 *  - 组合: 使用 QMap<QString,QSet<QString>> 聚合按服务划分的好友与群集合, 一个 UserBcc 实例组合多个服务维度数据。
 *  - 职责单一: 仅负责“用户自身状态 + 持久化”逻辑, 不处理跨用户或 UI 逻辑 (这些在 PlatformBcc / QQvs 中)。
 *  - 不变量维护: addFriendForService 在建立好友时同步清理申请/拒绝集合, 保持状态一致; 删除好友/退群时清理空服务键保持映射整洁。
 *  - 可扩展(开放封闭): 通过方法接口留出扩展校验/事件钩子空间, 无需修改调用方 (例如未来添加权限或回调)。
 */
class UserBcc
{
public:
    /** 显式构造防止隐式 QString -> UserBcc 转换 */
    explicit UserBcc(const QString& userId = QString());

    // 基础属性访问 (封装: 只读 + 受控写入)
    const QString& getId() const;
    const QString& getNickname() const;
    void setNickname(const QString& nickname);

    const QDate& getBirthDate() const;
    void setBirthDate(const QDate& date);

    const QDate& getRegisterDate() const;
    void setRegisterDate(const QDate& date);

    const QString& getLocation() const;
    void setLocation(const QString& location);

    // 好友按服务分类维护: 通过方法操作集合 (避免调用方直接写内部结构)封装：提供接口来操作内部数据
    QSet<QString> getFriendsOfService(const QString& serviceName) const; // 返回副本, 保护内部
    void addFriendForService(const QString& serviceName, const QString& friendUserId); // 建立好友并清理申请状态
    void removeFriendForService(const QString& serviceName, const QString& friendUserId); // 移除并可能收缩服务键
    bool hasFriendInService(const QString& serviceName, const QString& friendUserId) const;

    // 群按服务分类维护: 与好友逻辑一致的封装
    QSet<QString> getGroupsOfService(const QString& serviceName) const; // 返回副本
    void joinGroup(const QString& serviceName, const QString& groupId);
    void leaveGroup(const QString& serviceName, const QString& groupId);

    // 服务开通集合: 表示哪些服务已启用 (可用于 UI 构建与功能限制)
    void addOpenedService(const QString& serviceName);
    const QSet<QString>& openedServices() const; // 只读引用

    // 好友申请 / 消息状态 (当前简化为 QQ 服务驱动, 但结构具备扩展性)
    const QSet<QString>& incomingFriendRequests() const { return m_incomingFriendRequests; }
    const QSet<QString>& outgoingFriendRequests() const { return m_outgoingFriendRequests; }
    const QSet<QString>& rejectedNotices() const { return m_rejectedNotices; }

    bool addFriendRequestIncoming(const QString& fromUserId);    // 去重/自申请检查 + 互相申请逻辑
    bool addFriendRequestOutgoing(const QString& toUserId);      // 去重/自申请检查
    void removeIncomingRequest(const QString& fromUserId);       // 状态清理封装
    void removeOutgoingRequest(const QString& toUserId);
    void addRejectedNotice(const QString& userId);               // 维护拒绝通知与申请一致性
    void removeRejectedNotice(const QString& userId);
    bool hasPendingRequestWith(const QString& otherId) const;    // 双向查询封装

    // 持久化: 将内部状态序列化/反序列化 (格式与 UserBcc.cpp 中实现对应)
    bool saveToFile(const QString& filePath) const;
    bool loadFromFile(const QString& filePath);

private:
    // 基础资料 (封装属性, loadFromFile 可覆写, set 方法受控修改)
    QString m_userId;
    QString m_nickname;
    QDate   m_birthDate;
    QDate   m_registerDate; // 注册/申请时间
    QString m_location;

    // 组合结构: 服务 -> 好友集合 / 群集合
    QMap<QString, QSet<QString>> m_serviceToFriends;
    QMap<QString, QSet<QString>> m_serviceToGroups;
    QSet<QString> m_openedServices;

    // 申请状态集合: 通过方法保证不变量
    QSet<QString> m_incomingFriendRequests; // 收到待处理申请
    QSet<QString> m_outgoingFriendRequests; // 发出的待处理申请
    QSet<QString> m_rejectedNotices;        // 被拒绝的申请通知
};




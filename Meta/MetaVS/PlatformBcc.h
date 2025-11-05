#pragma once

#include <QString>
#include <QMap>
#include <QSet>
#include <QSharedPointer>
#include "UserBcc.h"
#include "ServiceBcc.h"

/**
 * @class PlatformBcc
 * 面向对象设计说明：
 *  - 作为“平台”协调者(Facade/Coordinator)，封装用户(UserBcc)与服务(ServiceBcc)集合的管理。
 *  - 提供行为接口：登录、跨服务好友同步、群类型切换、群批量持久化与重建。
 *  - 通过组合 (Composition) 持有用户与服务映射；不暴露内部容器，外部只能使用公开方法（封装）。
 *  - switchGroupTypePreserveMembers 展示运行时多态：替换 GroupBcc 派生实例但保持成员集合。
 *  - reconstructGroupsFromUserData 将用户的群 ID 数据反向生成缺失群对象，体现数据修复职责。
 *  - 登录状态 m_loggedIn 使用 serviceName -> set(userId) 映射，隔离权限/状态管理。
 *  - 绑定关系 m_wechatToQQ 演示跨账号聚合，不直接耦合 UserBcc，自主存储简化关系。
 *  单一职责：本类不直接处理 UI，只负责核心业务数据与跨对象协作。
 */
class PlatformBcc
{
public:
    PlatformBcc();

    /** 加载演示数据：为初始运行填充测试用户/服务/群。 */
    void loadDemo();

    /** 确保用户存在（若不存在则创建），返回是否已存在或创建成功。 */
    bool ensureUserExists(const QString& userId);
    /** 获取用户指针（若不存在返回 nullptr）。 */
    UserBcc* getUser(const QString& userId);

    /** 获取服务对象指针（若不存在返回 nullptr）。 */
    ServiceBcc* getService(const QString& serviceName);

    /** 建立微信账号与 QQ 账号绑定关系（封装关系映射）。 */
    void bindWeChatToQQ(const QString& wechatId, const QString& qqId);
    /** 查询微信账号对应绑定的 QQ 账号（若无绑定返回空字符串）。 */
    QString getBoundQQ(const QString& wechatId) const;

    /**
     * 登录逻辑：某用户登录一个服务时记录状态，可派生策略扩展。
     * 目前视为开通该服务（与 UI 逻辑配合）。
     */
    void login(const QString& userId, const QString& serviceName);
    bool isLoggedIn(const QString& userId, const QString& serviceName) const;

    /**
     * 跨服务添加好友：根据一个服务的好友关系在另一服务中建立镜像（展示服务间协作）。
     */
    void addFriendAcrossServices(const QString& fromService, const QString& toService,
                                 const QString& userId, const QString& friendId);

    /** 计算两个服务下用户的共同好友集合。 */
    QSet<QString> commonFriends(const QString& serviceA, const QString& serviceB,
                                const QString& userId) const;

    /**
     * 动态切换群类型而保留成员：体现运行时多态与对象替换。
     * 根据目标类型创建新派生实例(如 QQGroupBcc -> WechatGroupBcc)，复制成员/管理员/元数据。
     */
    QSharedPointer<GroupBcc> switchGroupTypePreserveMembers(ServiceBcc* service,
                                                            const QString& groupId,
                                                            GroupTypeBcc targetType,
                                                            const QString& newNameOptional = QString());

    /**
     * 根据用户加入的群 ID 重建缺失的群对象：数据一致性修复。
     * 遍历所有用户，对应服务缺失的 groupId 创建默认群(最小信息)，避免悬空引用。
     */
    void reconstructGroupsFromUserData();

    /** 批量保存所有服务的群到目录（持久化 Facade）。 */
    void saveAllGroups(const QString& dirPath) const;
    /** 从目录批量加载群文件进各服务（反序列化恢复）。 */
    void loadAllGroups(const QString& dirPath);

private:
    // 用户表：封装用户对象集合（组合）。键为 userId，值为实体对象（按值存储，便捷内存管理）。
    QMap<QString, UserBcc> m_users;
    // 服务表：封装服务对象集合（组合）。键为 serviceName。
    QMap<QString, ServiceBcc> m_services;
    // 账号绑定表：微信账号 -> QQ账号（轻量关联，未深度耦合 UserBcc）。
    QMap<QString, QString> m_wechatToQQ;
    // 登录状态：serviceName -> 已登录用户集合（状态映射）。
    QMap<QString, QSet<QString>> m_loggedIn;
};



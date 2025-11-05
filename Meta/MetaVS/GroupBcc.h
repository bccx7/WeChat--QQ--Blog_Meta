#pragma once

#include <QString>
#include <QSet>
#include <QSharedPointer>
#include <QVector>

/**
 * @enum GroupTypeBcc
 * 面向对象: 作为多态区分枚举，配合 GroupBcc::getType() 虚函数实现运行时类型识别。
 */
enum class GroupTypeBcc
{
    QQGroup,
    WechatGroup
};

/**
 * @class GroupBcc
 * 基类 (抽象类)：封装所有群的公共属性与行为，体现抽象(virtual纯虚函数) + 封装(私有/受保护成员经访问器暴露)。
 * 派生类通过重写 featureDescription/canCreateSubgroup/joinPolicyDescription 实现多态。
 *  - 虚析构确保通过基类指针删除派生对象时资源正确释放。
 *  - 成员集合/管理员集合等通过只读 const 引用暴露，修改统一走 add/remove 函数(封装)。
 *  - canManage / isOwner / isAdmin 构成角色权限判定接口，实现职责分离，UI 层不直接接触底层集合。
 */
class GroupBcc
{
public:
    explicit GroupBcc(const QString& id, const QString& name);
    virtual ~GroupBcc(); // 多态析构

    // 基本信息访问器（封装）。
    const QString& getId() const;
    const QString& getName() const;
    void setName(const QString& name);

    // 成员管理：集合只读；增删通过方法（便于后续加权限/事件）。
    const QSet<QString>& getMembers() const;
    bool addMember(const QString& userId);
    bool removeMember(const QString& userId);

    // 角色管理: 封装群主/管理员概念。管理员操作会受临时群标志限制。
    const QString& getOwner() const { return m_owner; }
    void setOwner(const QString& owner) { m_owner = owner; addMember(owner); }
    const QSet<QString>& getAdmins() const { return m_admins; }
    bool addAdmin(const QString& userId) { if (!m_members.contains(userId) || m_isTemporary) return false; m_admins.insert(userId); return true; }
    bool removeAdmin(const QString& userId) { if (m_isTemporary) return false; return m_admins.remove(userId) > 0; }
    bool isOwner(const QString& userId) const { return m_owner == userId; }
    bool isAdmin(const QString& userId) const { return m_admins.contains(userId); }
    bool canManage(const QString& userId) const { return !m_isTemporary && (isOwner(userId) || isAdmin(userId)); }

    // 临时群属性: 体现简单状态策略。设为临时群自动清空管理员，防止滥用。
    bool isTemporary() const { return m_isTemporary; }
    void setTemporary(bool t) { m_isTemporary = t; if (t) { m_admins.clear(); } }

    // 子群管理: 仅非临时群可添加。逻辑由派生类型的 canCreateSubgroup 进一步限制（策略多态）。
    const QSet<QString>& getSubgroups() const { return m_subgroups; }
    bool addSubgroup(const QString& subId) { if (m_isTemporary) return false; int b = m_subgroups.size(); m_subgroups.insert(subId); return m_subgroups.size() > b; }

    // 加入申请: 申请集合封装，避免 UI 直接操作内部结构。
    const QSet<QString>& getJoinRequests() const { return m_joinRequests; }
    bool addJoinRequest(const QString& userId){ if(m_members.contains(userId)) return false; int b=m_joinRequests.size(); m_joinRequests.insert(userId); return m_joinRequests.size()>b; }
    bool removeJoinRequest(const QString& userId){ return m_joinRequests.remove(userId)>0; }

    // 纯虚函数: 定义可变行为接口（多态实现差异化）。
    virtual GroupTypeBcc getType() const = 0;              // 运行时类型
    virtual QString featureDescription() const = 0;        // 功能说明（用于 UI 展示）
    virtual bool canCreateSubgroup() const = 0;            // 能否创建子群（策略差异）
    virtual QString joinPolicyDescription() const = 0;     // 加入策略文案

protected:
    // 受保护成员：派生类可访问；外部只能经公共接口（封装）。
    QString m_id;
    QString m_name;
    QSet<QString> m_members;
    QString m_owner;
    QSet<QString> m_admins;
    QSet<QString> m_subgroups;
    QSet<QString> m_joinRequests; // 待审核加入申请（成员账号）
    bool m_isTemporary = false;   // 是否临时群
};

/**
 * @class QQGroupBcc
 * 派生类：实现 QQ 群策略。final 防止继续继承（封闭具体实现）。
 * 特性：支持管理员制度 + 加入申请审核 + 可创建临时讨论组 / 子群（非临时时）。
 */
class QQGroupBcc final : public GroupBcc
{
public:
    QQGroupBcc(const QString& id, const QString& name);
    GroupTypeBcc getType() const override;           // 多态类型
    QString featureDescription() const override;     // QQ 群特性
    bool canCreateSubgroup() const override;         // 仅非临时群
    QString joinPolicyDescription() const override;  // 申请+审核模式
};

/**
 * @class WechatGroupBcc
 * 派生类：实现 微信群策略。final 防止再深入继承。
 * 特性：不支持管理员与临时讨论组，邀请制加入。
 */
class WechatGroupBcc final : public GroupBcc
{
public:
    WechatGroupBcc(const QString& id, const QString& name);
    GroupTypeBcc getType() const override;           // 多态类型
    QString featureDescription() const override;     // 微信群特性
    bool canCreateSubgroup() const override;         // 始终 false
    QString joinPolicyDescription() const override;  // 邀请制
};




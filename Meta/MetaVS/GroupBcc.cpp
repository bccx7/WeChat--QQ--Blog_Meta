#include "GroupBcc.h"

/**
 * Base class GroupBcc: 封装群的通用数据与操作（封装/抽象）。
 * 通过虚函数 getType()/featureDescription()/canCreateSubgroup()/joinPolicyDescription() 支持多态，
 * 使调用方可用基类指针统一处理不同群类型的行为。
 */
GroupBcc::GroupBcc(const QString& id, const QString& name)
    : m_id(id), m_name(name)
{
}

// 虚析构：允许通过基类指针安全析构派生对象（多态清理）
GroupBcc::~GroupBcc() = default;

// 封装：只读访问群ID与名称
const QString& GroupBcc::getId() const { return m_id; }
const QString& GroupBcc::getName() const { return m_name; }
// 封装：修改名称通过公开接口，提高不变量控制点
void GroupBcc::setName(const QString& name) { m_name = name; }

// 成员集合只读暴露，外部不能直接插入/删除（封装）
const QSet<QString>& GroupBcc::getMembers() const { return m_members; }

// 封装成员添加：返回是否真的增加（逻辑集中便于以后添加校验/事件触发）
bool GroupBcc::addMember(const QString& userId)
{
    int before = m_members.size();
    m_members.insert(userId);
    return m_members.size() > before;
}

// 封装成员移除：统一出口，可后续扩展权限/钩子
bool GroupBcc::removeMember(const QString& userId)
{
    return m_members.remove(userId) > 0;
}

/**
 * QQGroupBcc 派生类：体现继承 & 多态。
 * 重写特性相关虚函数，提供 QQ 群的专属描述、子群能力与加入策略。
 */
QQGroupBcc::QQGroupBcc(const QString& id, const QString& name)
    : GroupBcc(id, name)
{
}

// 多态类型标识
GroupTypeBcc QQGroupBcc::getType() const { return GroupTypeBcc::QQGroup; }

// 多态描述：用于 UI 显示差异化功能
QString QQGroupBcc::featureDescription() const
{
    return QString::fromUtf8("QQ群：支持申请加入与管理员制度，可创建临时讨论组");
}

// 行为差异：是否允许创建子群（演示策略多态）
bool QQGroupBcc::canCreateSubgroup() const { return !m_isTemporary; }

// 加入策略描述（策略展示）
QString QQGroupBcc::joinPolicyDescription() const
{
    return QString::fromUtf8("申请加入，经管理员/群主审核");
}

/**
 * WechatGroupBcc 派生类：不同策略实现，展示多态。
 * 不支持管理员体系与临时讨论组。
 */
WechatGroupBcc::WechatGroupBcc(const QString& id, const QString& name)
    : GroupBcc(id, name)
{
}

GroupTypeBcc WechatGroupBcc::getType() const { return GroupTypeBcc::WechatGroup; }

QString WechatGroupBcc::featureDescription() const
{
    return QString::fromUtf8("微信群：仅群主特权，邀请制加入，不支持临时讨论组");
}

// 微信群永远不支持子群
bool WechatGroupBcc::canCreateSubgroup() const { return false; }

QString WechatGroupBcc::joinPolicyDescription() const
{
    return QString::fromUtf8("仅可被群成员/群主邀请加入");
}

// 注：通过基类指针使用派生对象时，调用这些重写方法即可实现运行时多态。



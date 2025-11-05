#include "ServiceBcc.h"

/**
 * ServiceBcc: 面向对象要点
 *  - 封装(Encapsulation): 通过 getGroup/hasGroup/addGroup/removeGroup 暴露最小群管理接口，外部不可直接修改 m_groups。
 *  - 组合(Composition): 持有 QMap<groupId, QSharedPointer<GroupBcc>>，管理多个群对象生命周期。
 *  - 多态(Polymorphism): QSharedPointer<GroupBcc> 可指向 QQGroupBcc / WechatGroupBcc 等派生类；调用端使用基类接口无需关心具体类型。
 *  - 责任分离: ServiceBcc 只聚焦“某一服务下的群集合”管理，不处理用户/跨服务逻辑。
 */

/**
 * @brief 构造一个新的 ServiceBcc 对象。
 * @param name 服务的名称。
 */
ServiceBcc::ServiceBcc(const QString& name)
    : m_name(name)
{
}

/** 封装：只读访问服务名称 */
const QString& ServiceBcc::getName() const { return m_name; }

/** 封装：查询群是否存在（不暴露内部容器结构） */
bool ServiceBcc::hasGroup(const QString& groupId) const
{
    return m_groups.contains(groupId);
}

/**
 * 多态入口：返回基类指针，调用者可通过虚函数获取派生行为
 */
QSharedPointer<GroupBcc> ServiceBcc::getGroup(const QString& groupId) const
{
    return m_groups.value(groupId);
}

/**
 * 添加群：使用群对象的 getId() 作为键；若已有同名键将覆盖（可后续扩展冲突策略）。
 * 通过统一入口保持封装，便于以后添加校验/事件触发。
 */
void ServiceBcc::addGroup(const QSharedPointer<GroupBcc>& group)
{
    if (!group) return;
    m_groups.insert(group->getId(), group);
}

/** 封装：移除群，返回是否真正删除（>0 表示映射发生变更） */
bool ServiceBcc::removeGroup(const QString& groupId)
{
    return m_groups.remove(groupId) > 0;
}

/**
 * 返回常量引用：只读访问集合；防止外部通过非常量引用直接改动。
 * 调用者需使用类公开方法做修改，保障不变量。
 */
const QMap<QString, QSharedPointer<GroupBcc>>& ServiceBcc::groups() const
{
    return m_groups;
}

/**
 * 返回可变引用：仅在内部或受信代码需要批量操作时使用。
 * 若开放此接口要谨慎（可能破坏封装），保留同时仍提供高层方法方便安全操作。
 */
QMap<QString, QSharedPointer<GroupBcc>>& ServiceBcc::groupsMutable()
{
    return m_groups;
}



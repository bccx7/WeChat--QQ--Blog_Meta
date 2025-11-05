#pragma once

#include <QString>
#include <QMap>
#include <QSharedPointer>
#include "GroupBcc.h"

/**
 * @class ServiceBcc
 * @brief 代表一个服务（如 QQ、WeChat），管理该服务下的所有群组。
 *
 * 提供了对群组的增、删、查、改等基本操作。
 */
class ServiceBcc
{
public:
    /**
     * @brief 构造一个新的 ServiceBcc 对象。
     * @param name 服务的名称。
     */
    explicit ServiceBcc(const QString& name = QString());

    /**
     * @brief 获取服务的名称。
     * @return 服务的名称。
     */
    const QString& getName() const; 

    /**
     * @brief 检查是否存在指定 ID 的群组。
     * @param groupId 要检查的群组 ID。
     * @return 如果存在则返回 true，否则返回 false。
     */
    bool hasGroup(const QString& groupId) const;

    /**
     * @brief 获取指定 ID 的群组。
     * @param groupId 要获取的群组 ID。
     * @return 返回群组的共享指针，如果不存在则返回空指针。
     */
    QSharedPointer<GroupBcc> getGroup(const QString& groupId) const;

    /**
     * @brief 添加一个群组到服务中。
     * @param group 要添加的群组的共享指针。
     */
    void addGroup(const QSharedPointer<GroupBcc>& group);

    /**
     * @brief 从服务中移除一个群组。
     * @param groupId 要移除的群组 ID。
     * @return 如果成功移除则返回 true，否则返回 false。
     */
    bool removeGroup(const QString& groupId); 

    /**
     * @brief 获取所有群组的常量引用。
     * @return 返回群组映射的常量引用。
     */
    const QMap<QString, QSharedPointer<GroupBcc>>& groups() const;

    /**
     * @brief 获取所有群组的可变引用。
     * @return 返回群组映射的可变引用。
     */
    QMap<QString, QSharedPointer<GroupBcc>>& groupsMutable();

private:
    QString m_name; ///< 服务的名称。
    QMap<QString, QSharedPointer<GroupBcc>> m_groups; ///< 存储群组的映射，键为群组 ID，值为群组对象的共享指针。
};




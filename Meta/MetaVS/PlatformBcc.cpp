#include "PlatformBcc.h"
#include <QFile>
#include <QTextStream>
#include <QDir>

// PlatformBcc: 平台协调类的实现文件。演示面向对象的组合(持有用户/服务集合)、封装(仅公开必要接口)、
PlatformBcc::PlatformBcc() = default; // 构造：保持成员映射为空（RAII 自动管理）

/**
 * loadDemo: 初始化演示数据
 * OOP: 通过组合向 m_services 中插入 ServiceBcc 对象；利用派生类 QQGroupBcc / WechatGroupBcc 构造群（多态）。
 */
void PlatformBcc::loadDemo()
{
    // 服务对象插入：封装 service 管理，不暴露内部容器给外部直接操作
    m_services.insert("QQ", ServiceBcc("QQ"));
    m_services.insert("WeChat", ServiceBcc("WeChat"));
    m_services.insert("Weibo", ServiceBcc("Weibo"));

    // 预置群：根据服务类型选择不同派生类（运行时类型构建）
    for (const QString& svc : { QString("QQ"), QString("WeChat") }) {
        auto& service = m_services[svc];
        for (int i = 1001; i <= 1006; ++i) {
            QString gid = QString::number(i);
            QSharedPointer<GroupBcc> g = (svc == "QQ")
                ? QSharedPointer<GroupBcc>(new QQGroupBcc(gid, svc + "群" + gid))
                : QSharedPointer<GroupBcc>(new WechatGroupBcc(gid, svc + "群" + gid));
            service.addGroup(g); // 封装：通过 addGroup 而不是直接访问内部映射
        }
    }

    // 用户与绑定关系：组合 + 责任分离
    ensureUserExists("10001");
    ensureUserExists("20002");
    bindWeChatToQQ("wx_10001", "10001");

    // 初始 QQ 好友（使用封装的用户对象方法）
    m_users["10001"].addFriendForService("QQ", "20002");
}

/** ensureUserExists: 工厂/保障式创建，集中用户对象生命周期管理 */
bool PlatformBcc::ensureUserExists(const QString& userId)
{
    if (!m_users.contains(userId)) {
        m_users.insert(userId, UserBcc(userId)); // 按值存储：简单所有权，体现组合
        return true;
    }
    return false;
}

/** getUser: 受控访问内部用户映射（封装） */
UserBcc* PlatformBcc::getUser(const QString& userId)
{
    return m_users.contains(userId) ? &m_users[userId] : nullptr;
}

/** getService: 受控访问服务对象（封装） */
ServiceBcc* PlatformBcc::getService(const QString& serviceName)
{
    return m_services.contains(serviceName) ? &m_services[serviceName] : nullptr;
}

/** bindWeChatToQQ: 维护跨账号关系（聚合），不与 UserBcc 强耦合 */
void PlatformBcc::bindWeChatToQQ(const QString& wechatId, const QString& qqId)
{
    m_wechatToQQ[wechatId] = qqId;
}

/** getBoundQQ: 只读查询绑定关系 */
QString PlatformBcc::getBoundQQ(const QString& wechatId) const
{
    return m_wechatToQQ.value(wechatId);
}

/**
 * login: 登录任一服务即记录在所有服务的登录集合中。
 * 体现职责：PlatformBcc 统一维护登录状态，UI 不直接操作 m_loggedIn（封装）。
 */
void PlatformBcc::login(const QString& userId, const QString& serviceName)
{
    Q_UNUSED(serviceName); // 当前策略忽略具体服务名称（可扩展）
    ensureUserExists(userId);
    for (auto it = m_services.begin(); it != m_services.end(); ++it) {
        m_loggedIn[it.key()].insert(userId);
    }
}

/** isLoggedIn: 状态查询接口 */
bool PlatformBcc::isLoggedIn(const QString& userId, const QString& serviceName) const
{
    auto it = m_loggedIn.find(serviceName);
    if (it == m_loggedIn.end()) return false;
    return it.value().contains(userId);
}

/** addFriendAcrossServices: 跨服务好友同步示例（平台协调角色） */
void PlatformBcc::addFriendAcrossServices(const QString& fromService, const QString& toService,
                                           const QString& userId, const QString& friendId)
{
    Q_UNUSED(fromService); // 当前实现不使用来源服务，仅示例
    ensureUserExists(userId);
    ensureUserExists(friendId);
    m_users[userId].addFriendForService(toService, friendId);
}

/** commonFriends: 组合数据运算（不暴露底层容器结构） */
QSet<QString> PlatformBcc::commonFriends(const QString& serviceA, const QString& serviceB,
                                         const QString& userId) const
{
    QSet<QString> a = m_users.value(userId).getFriendsOfService(serviceA);
    QSet<QString> b = m_users.value(userId).getFriendsOfService(serviceB);
    return a.intersect(b); // 使用 QSet 接口保持封装
}

/**
 * switchGroupTypePreserveMembers:
 * 运行时多态核心示例——根据 targetType 创建新的派生群对象并迁移成员。
 * 调用端仅依赖基类指针 QSharedPointer<GroupBcc>，无需了解派生具体实现，体现开放封闭原则。
 */
QSharedPointer<GroupBcc> PlatformBcc::switchGroupTypePreserveMembers(ServiceBcc* service,
                                                                     const QString& groupId,
                                                                     GroupTypeBcc targetType,
                                                                     const QString& newNameOptional)
{
    if (!service) return {};
    QSharedPointer<GroupBcc> oldGroup = service->getGroup(groupId);
    if (!oldGroup) return {};

    QString newName = newNameOptional.isEmpty() ? oldGroup->getName() : newNameOptional;

    QSharedPointer<GroupBcc> newGroup;
    if (targetType == GroupTypeBcc::QQGroup) {
        newGroup.reset(new QQGroupBcc(groupId, newName));
    } else {
        newGroup.reset(new WechatGroupBcc(groupId, newName));
    }
    // 保留成员：迁移旧群的成员集合（封装：只用 addMember，不直接操作内部结构）
    for (const QString& uid : oldGroup->getMembers()) {
        newGroup->addMember(uid);
    }
    // 覆盖：简单添加，让 ServiceBcc 内部映射更新（若存在旧指针可在 ServiceBcc 层做替换策略扩展）
    service->addGroup(newGroup);
    return newGroup;
}

/**
 * reconstructGroupsFromUserData:
 * 数据一致性修复。依据用户记录的 QQ 群 ID 反向补全缺失群对象。
 * OOP: 平台级遍历 + 在服务内集中创建 QQGroupBcc，占位并填充成员。
 */
void PlatformBcc::reconstructGroupsFromUserData()
{
    ServiceBcc* qqSvc = getService("QQ");
    if (!qqSvc) return;
    QMap<QString, QSet<QString>> groupMembers; // 临时收集结构
    for (auto it = m_users.begin(); it != m_users.end(); ++it) {
        const QString& uid = it.key();
        QSet<QString> groups = it.value().getGroupsOfService("QQ");
        for (const QString& gid : groups) {
            groupMembers[gid].insert(uid);
        }
    }
    for (auto it = groupMembers.begin(); it != groupMembers.end(); ++it) {
        const QString& gid = it.key();
        if (!qqSvc->hasGroup(gid)) {
            // 创建占位群（派生 QQGroupBcc）
            QSharedPointer<GroupBcc> g(new QQGroupBcc(gid, QString("临时群") + gid));
            for (const QString& member : it.value()) {
                g->addMember(member);
            }
            qqSvc->addGroup(g);
        } else {
            auto existing = qqSvc->getGroup(gid);
            if (existing) {
                for (const QString& member : it.value()) existing->addMember(member);
            }
        }
    }
}

/** saveAllGroups: 群持久化 Facade，序列化所有服务的群数据到文件 */
void PlatformBcc::saveAllGroups(const QString& dirPath) const
{
    QDir().mkpath(dirPath); // 简单目录确保（职责内）
    for (auto svcIt = m_services.begin(); svcIt != m_services.end(); ++svcIt) {
        const QString& svcName = svcIt.key();
        const ServiceBcc& svc = svcIt.value();
        for (auto gIt = svc.groups().begin(); gIt != svc.groups().end(); ++gIt) {
            const QSharedPointer<GroupBcc>& g = gIt.value();
            if (!g) continue;
            QString filePath = dirPath + QString("/group_%1_%2.grp").arg(svcName, g->getId());
            QFile f(filePath);
            if (!f.open(QIODevice::WriteOnly | QIODevice::Text)) continue; // 封装：失败直接跳过
            QTextStream out(&f);
#if QT_VERSION < QT_VERSION_CHECK(6,0,0)
            out.setCodec("UTF-8");
#else
            out.setEncoding(QStringConverter::Utf8);
#endif
            // 文本格式：简洁键值对；后续可扩展为 JSON/二进制
            out << "id:" << g->getId() << "\n";
            out << "name:" << g->getName() << "\n";
            out << "type:" << (g->getType() == GroupTypeBcc::QQGroup ? "qq" : "wechat") << "\n";
            out << "owner:" << g->getOwner() << "\n";
            out << "tmp:" << (g->isTemporary() ? "1" : "0") << "\n";
            out << "members:";
            bool first = true;
            for (const QString& m : g->getMembers()) { if (!first) out << ","; out << m; first = false; }
            out << "\nadmins:"; first = true;
            for (const QString& a : g->getAdmins()) { if (!first) out << ","; out << a; first = false; }
            out << "\nsubgroups:"; first = true;
            for (const QString& s : g->getSubgroups()) { if (!first) out << ","; out << s; first = false; }
            out << "\njoinreqs:"; first = true; // 持久化加入申请（不影响成员集合）
            for (const QString& jr : g->getJoinRequests()) { if (!first) out << ","; out << jr; first = false; }
            out << "\n";
            f.close();
        }
    }
}

/** loadAllGroups: 反序列化群对象；根据 type 字段恢复派生类型（运行时多态） */
void PlatformBcc::loadAllGroups(const QString& dirPath)
{
    QDir dir(dirPath);
    if (!dir.exists()) return;
    QStringList files = dir.entryList(QStringList() << "group_*.grp", QDir::Files);
    QSet<QString> usersNeedingSave; // 延迟保存标记
    for (const QString& fn : files) {
        QFile f(dir.absoluteFilePath(fn));
        if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) continue;
        QTextStream in(&f);
#if QT_VERSION < QT_VERSION_CHECK(6,0,0)
        in.setCodec("UTF-8");
#else
        in.setEncoding(QStringConverter::Utf8);
#endif
        // 暂存解析字段
        QString id, name, type, owner; QSet<QString> members, admins, subgroups; bool tmpFlag=false; QSet<QString> joinreqs;
        while (!in.atEnd()) {
            QString line = in.readLine();
            if (line.startsWith("id:")) id = line.mid(3);
            else if (line.startsWith("name:")) name = line.mid(5);
            else if (line.startsWith("type:")) type = line.mid(5);
            else if (line.startsWith("owner:")) owner = line.mid(6);
            else if (line.startsWith("tmp:")) tmpFlag = (line.mid(4)=="1");
            else if (line.startsWith("members:")) { QString list = line.mid(8); if (!list.isEmpty()) for (const QString& v : list.split(',', Qt::SkipEmptyParts)) members.insert(v); }
            else if (line.startsWith("admins:")) { QString list = line.mid(7); if (!list.isEmpty()) for (const QString& v : list.split(',', Qt::SkipEmptyParts)) admins.insert(v); }
            else if (line.startsWith("subgroups:")) { QString list = line.mid(10); if (!list.isEmpty()) for (const QString& v : list.split(',', Qt::SkipEmptyParts)) subgroups.insert(v); }
            else if (line.startsWith("joinreqs:")) { QString list = line.mid(9); if (!list.isEmpty()) for (const QString& v : list.split(',', Qt::SkipEmptyParts)) joinreqs.insert(v); }
        }
        f.close();
        if (id.isEmpty()) continue; // 基本健壮性
        QString svcName = fn.section('_', 1, 1); // 文件名解析 serviceName
        ServiceBcc* svc = getService(svcName);
        if (!svc) continue;
        // 根据类型创建派生实例（运行时多态）
        QSharedPointer<GroupBcc> g;
        if (type == "wechat") g.reset(new WechatGroupBcc(id, name)); else g.reset(new QQGroupBcc(id, name));
        g->setTemporary(tmpFlag);
        for (const QString& m : members) g->addMember(m);
        if (!owner.isEmpty()) g->setOwner(owner);
        if (!g->isTemporary()) { for (const QString& a : admins) g->addAdmin(a); for (const QString& s : subgroups) g->addSubgroup(s); }
        for (const QString& jr : joinreqs) { if (!members.contains(jr)) g->addJoinRequest(jr); }
        svc->addGroup(g);
        // 反向更新用户的群集合（保持数据双向一致）
        for (const QString& m : members) {
            ensureUserExists(m);
            UserBcc* u = getUser(m);
            if (u) {
                QSet<QString> existing = u->getGroupsOfService(svcName);
                if (!existing.contains(id)) { u->joinGroup(svcName, id); usersNeedingSave.insert(m); }
            }
        }
    }
    // 保存被修正的用户文件
    for (const QString& uid : usersNeedingSave) {
        UserBcc* u = getUser(uid);
        if (u) {
            QString userFile = dirPath + "/" + uid + ".dat";
            u->saveToFile(userFile);
        }
    }
}



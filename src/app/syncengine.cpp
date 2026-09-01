#include "syncengine.h"

#include "../core/pairing.h"
#include "../net/crypto.h"

#include <nlohmann/json.hpp>
#include <QDateTime>
#include <QJsonArray>
#include <QJsonDocument>
#include <QUuid>
#include <QDebug>

using json = nlohmann::json;

namespace app {

SyncEngine::SyncEngine(QObject* parent)
    : QObject(parent)
    , m_debounce(new QTimer(this))
{
    m_debounce->setSingleShot(true);
    m_debounce->setInterval(300);
    connect(m_debounce, &QTimer::timeout, this, &SyncEngine::onDebounce);
}

SyncEngine::~SyncEngine() { shutdown(); }

void SyncEngine::init(store::Database* db, net::RelayPool* pool,
                      const QString& deviceId, const QString& displayName)
{
    m_db = db;
    m_pool = pool;
    m_deviceId = deviceId;
    m_displayName = displayName;

    connect(pool, &net::RelayPool::eventReceived, this, &SyncEngine::onRelayEvent);
    connect(pool, &net::RelayPool::onlineChanged, this, &SyncEngine::onRelayOnline);
    pool->setRelays(net::RelayPool::defaultRelays());
    pool->connectAll();
    subscribeAll();
    flushOutbox();
}

bool SyncEngine::isOnline() const
{
    return m_pool && m_pool->isOnline();
}

void SyncEngine::shutdown()
{
    if (m_debounce)
        m_debounce->stop();
    if (m_pool)
        m_pool->disconnectAll();
}

void SyncEngine::createWorkspace(const QString& title)
{
    if (!m_db)
        return;
    core::WorkspaceMeta ws;
    ws.workspaceId = QUuid::createUuid().toString(QUuid::WithoutBraces).toStdString();
    ws.key = net::generateListKey();
    ws.title = title.toStdString();
    ws.created = QDateTime::currentMSecsSinceEpoch();
    ws.titleVer = {1, m_deviceId.toStdString()};
    m_db->createWorkspace(ws);
    m_db->setSetting("syncWorkspaceId", ws.workspaceId);
    subscribeAll();
    onLocalChange();
}

QString SyncEngine::joinUri() const
{
    const auto wsId = workspaceId();
    const auto key = workspaceKey();
    if (!wsId || !key)
        return {};
    auto ws = m_db->getWorkspace(*wsId);
    const std::string title = ws ? ws->title : "Punch Clock";
    return QString::fromStdString(core::buildJoinUri(*wsId, *key, title));
}

void SyncEngine::handleJoinUrl(const QUrl& url)
{
    QString s = url.toString();
    s.replace(QStringLiteral("openpunchclock://"), QStringLiteral("colocourse://"));
    const auto info = core::parseJoinUri(s.toStdString());
    if (!info)
        return;
    core::WorkspaceMeta ws;
    ws.workspaceId = info->listId;
    ws.key = info->key;
    ws.title = info->title;
    ws.created = QDateTime::currentMSecsSinceEpoch();
    m_db->createWorkspace(ws);
    m_db->setSetting("syncWorkspaceId", ws.workspaceId);
    subscribeAll();
    catchUpOnForeground();
}

void SyncEngine::onLocalChange()
{
    m_debounce->start();
}

void SyncEngine::catchUpOnForeground()
{
    flushOutbox();
    subscribeAll();
}

void SyncEngine::onDebounce()
{
    publishDelta();
}

std::optional<std::string> SyncEngine::workspaceId() const
{
    if (!m_db)
        return std::nullopt;
    if (auto s = m_db->getSetting("syncWorkspaceId"))
        return *s;
    const auto all = m_db->getWorkspaces();
    return all.empty() ? std::nullopt : std::optional(all.front().workspaceId);
}

std::optional<std::vector<uint8_t>> SyncEngine::workspaceKey() const
{
    const auto id = workspaceId();
    if (!id)
        return std::nullopt;
    const auto ws = m_db->getWorkspace(*id);
    if (!ws || ws->key.size() != 32)
        return std::nullopt;
    return ws->key;
}

void SyncEngine::subscribeAll()
{
    const auto key = workspaceKey();
    if (!key || !m_pool)
        return;
    const std::string tag = net::deriveChannelTag(*key);
    m_pool->subscribeAll(QString::fromStdString(tag), 0);
    m_subscribed = true;
}

std::string SyncEngine::buildPayloadJson() const
{
    json root;
    root["v"] = 1;
    root["t"] = "punch";
    root["by"] = m_deviceId.toStdString();
    json entries = json::array();
    for (const auto& e : m_db->getTimeEntries()) {
        entries.push_back({
            {"id", e.entryId},
            {"project", e.projectId},
            {"start", e.startMs},
            {"end", e.endMs},
            {"break", e.breakMs},
            {"notes", e.notes},
            {"tags", e.tags},
            {"reimburse", e.reimburse},
            {"deduct", e.deduct},
            {"source", e.source},
            {"touched", e.touched},
            {"del", e.del},
        });
    }
    root["entries"] = entries;
    json projects = json::array();
    for (const auto& p : m_db->getProjects(true)) {
        projects.push_back({
            {"id", p.projectId},
            {"name", p.name},
            {"rate", p.hourlyRate},
            {"color", p.color},
            {"default", p.isDefault},
            {"del", p.del},
            {"touched", p.touched},
        });
    }
    root["projects"] = projects;
    return root.dump();
}

void SyncEngine::applyPayloadJson(const std::string& plain)
{
    json root;
    try {
        root = json::parse(plain);
    } catch (...) {
        return;
    }
    if (root.value("t", "") != "punch")
        return;

    for (const auto& pj : root.value("projects", json::array())) {
        core::Project p;
        p.projectId = pj.value("id", "");
        p.name = pj.value("name", "");
        p.hourlyRate = pj.value("rate", 0.0);
        p.color = pj.value("color", "#2E7D32");
        p.isDefault = pj.value("default", false);
        p.del = pj.value("del", false);
        p.touched = pj.value("touched", int64_t(0));
        auto existing = m_db->getProject(p.projectId);
        if (!existing || p.touched >= existing->touched)
            m_db->upsertProject(p);
    }

    for (const auto& ej : root.value("entries", json::array())) {
        core::TimeEntry e;
        e.entryId = ej.value("id", "");
        e.projectId = ej.value("project", "");
        e.startMs = ej.value("start", int64_t(0));
        e.endMs = ej.value("end", int64_t(0));
        e.breakMs = ej.value("break", int64_t(0));
        e.notes = ej.value("notes", "");
        e.tags = ej.value("tags", "");
        e.reimburse = ej.value("reimburse", 0.0);
        e.deduct = ej.value("deduct", 0.0);
        e.source = ej.value("source", "sync");
        e.touched = ej.value("touched", int64_t(0));
        e.del = ej.value("del", false);
        auto existing = m_db->getTimeEntry(e.entryId);
        if (!existing || e.touched >= existing->touched)
            m_db->upsertTimeEntry(e);
    }
    emit remoteChanges();
}

void SyncEngine::publishDelta()
{
    const auto wsId = workspaceId();
    const auto key = workspaceKey();
    if (!wsId || !key || !m_pool)
        return;

    const std::string plain = buildPayloadJson();
    const std::string tag = net::deriveChannelTag(*key);
    const std::string cipher = net::encryptPayload(*key, tag, plain);
    if (cipher.empty())
        return;

    net::NostrEvent ev;
    ev.kind = 4545;
    ev.created_at = QDateTime::currentSecsSinceEpoch();
    ev.content = QString::fromStdString(cipher);
    QJsonArray tagT;
    tagT.append(QStringLiteral("t"));
    tagT.append(QString::fromStdString(tag));
    ev.tags.append(tagT);

    const auto seed = net::deriveNostrSeed(*key);
    if (!net::signEvent(ev, seed))
        return;

    const QJsonDocument doc(ev.toJson());
    m_db->outboxPush(*wsId, doc.toJson(QJsonDocument::Compact).toStdString());
    emit outboxChanged();

    if (m_pool->isOnline()) {
        m_pool->publishToAll(ev);
        const auto pending = m_db->outboxPeekAll(*wsId);
        if (!pending.empty())
            m_db->outboxRemove(pending.front().first);
        emit outboxChanged();
    }
}

void SyncEngine::flushOutbox()
{
    publishDelta();
}

void SyncEngine::onRelayOnline(bool online)
{
    emit onlineChanged(online);
    if (online)
        flushOutbox();
}

void SyncEngine::onRelayEvent(const net::NostrEvent& ev)
{
    if (ev.kind != 4545)
        return;
    if (m_db->isEventSeen(ev.id.toStdString()))
        return;
    m_db->markEventSeen(ev.id.toStdString());

    const auto key = workspaceKey();
    if (!key)
        return;

    std::string tag;
    for (const auto& tagVal : ev.tags) {
        if (tagVal.isArray() && tagVal.toArray().size() >= 2
            && tagVal.toArray().at(0).toString() == QLatin1String("t")) {
            tag = tagVal.toArray().at(1).toString().toStdString();
            break;
        }
    }
    if (tag.empty())
        tag = net::deriveChannelTag(*key);

    const auto plain = net::decryptPayload(*key, tag, ev.content.toStdString());
    if (!plain)
        return;
    applyPayloadJson(*plain);
}

} // namespace app

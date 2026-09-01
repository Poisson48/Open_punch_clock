#include "database.h"

#include <QDateTime>
#include <QDebug>
#include <QSqlError>
#include <QSqlQuery>
#include <QUuid>

namespace store {

namespace {
QString qs(const std::string& s) { return QString::fromStdString(s); }
std::string ss(const QString& s) { return s.toStdString(); }
qlonglong ll(int64_t v) { return static_cast<qlonglong>(v); }
} // namespace

Database::~Database() { close(); }

bool Database::open(const QString& path)
{
    m_connectionName = QStringLiteral("openpunch_") +
                       QUuid::createUuid().toString(QUuid::WithoutBraces);
    m_db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), m_connectionName);
    m_db.setDatabaseName(path);
    if (!m_db.open()) {
        qWarning() << "Database::open failed:" << m_db.lastError().text();
        return false;
    }
    QSqlQuery q(m_db);
    q.exec(QStringLiteral("PRAGMA journal_mode=WAL"));
    q.exec(QStringLiteral("PRAGMA foreign_keys=ON"));
    return createSchema();
}

void Database::close()
{
    if (m_db.isOpen())
        m_db.close();
    if (!m_connectionName.isEmpty()) {
        QSqlDatabase::removeDatabase(m_connectionName);
        m_connectionName.clear();
    }
}

bool Database::createSchema()
{
    QSqlQuery q(m_db);
    const QStringList ddl = {
        QStringLiteral(
            "CREATE TABLE IF NOT EXISTS projects ("
            "  project_id TEXT PRIMARY KEY,"
            "  name TEXT, hourly_rate REAL, color TEXT,"
            "  is_default INT, created INT, del INT,"
            "  name_l INT, name_d TEXT, rate_l INT, rate_d TEXT,"
            "  del_l INT, del_d TEXT, touched INT"
            ")"),
        QStringLiteral(
            "CREATE TABLE IF NOT EXISTS time_entries ("
            "  entry_id TEXT PRIMARY KEY,"
            "  project_id TEXT, start_ms INT, end_ms INT, break_ms INT,"
            "  notes TEXT, tags TEXT, reimburse REAL, deduct REAL,"
            "  source TEXT, lat REAL, lon REAL,"
            "  created INT, touched INT, del INT, del_l INT, del_d TEXT"
            ")"),
        QStringLiteral(
            "CREATE TABLE IF NOT EXISTS punch_state ("
            "  id INT PRIMARY KEY CHECK (id = 1),"
            "  clocked_in INT, project_id TEXT, clock_in_ms INT,"
            "  break_start_ms INT, accumulated_break_ms INT, active_entry_id TEXT"
            ")"),
        QStringLiteral(
            "CREATE TABLE IF NOT EXISTS settings (key TEXT PRIMARY KEY, value TEXT)"),
        QStringLiteral(
            "CREATE TABLE IF NOT EXISTS history ("
            "  rowid INTEGER PRIMARY KEY AUTOINCREMENT,"
            "  entry_id TEXT, action TEXT, detail TEXT, at_ms INT"
            ")"),
        QStringLiteral(
            "CREATE TABLE IF NOT EXISTS workspaces ("
            "  workspace_id TEXT PRIMARY KEY, key BLOB, title TEXT,"
            "  title_ver_l INT, title_ver_d TEXT, lamport INT,"
            "  last_sync INT, created INT"
            ")"),
        QStringLiteral(
            "CREATE TABLE IF NOT EXISTS outbox ("
            "  rowid INTEGER PRIMARY KEY AUTOINCREMENT,"
            "  workspace_id TEXT, event_json TEXT, created INT"
            ")"),
        QStringLiteral(
            "CREATE TABLE IF NOT EXISTS seen_events ("
            "  event_id TEXT PRIMARY KEY, seen_at INT"
            ")"),
        QStringLiteral(
            "CREATE TABLE IF NOT EXISTS members ("
            "  workspace_id TEXT, device_id TEXT, name TEXT,"
            "  ver_l INT, ver_d TEXT,"
            "  PRIMARY KEY(workspace_id, device_id)"
            ")"),
    };
    for (const auto& sql : ddl) {
        if (!q.exec(sql)) {
            qWarning() << "DDL failed:" << q.lastError().text() << sql;
            return false;
        }
    }
    q.exec(QStringLiteral(
        "INSERT OR IGNORE INTO punch_state (id, clocked_in, project_id, clock_in_ms,"
        " break_start_ms, accumulated_break_ms, active_entry_id)"
        " VALUES (1, 0, '', 0, 0, 0, '')"));
    return migrateSchema();
}

bool Database::migrateSchema() { return true; }

bool Database::upsertProject(const core::Project& p)
{
    QSqlQuery q(m_db);
    q.prepare(QStringLiteral(
        "INSERT INTO projects (project_id, name, hourly_rate, color, is_default, created,"
        " del, name_l, name_d, rate_l, rate_d, del_l, del_d, touched)"
        " VALUES (?,?,?,?,?,?,?,?,?,?,?,?,?,?)"
        " ON CONFLICT(project_id) DO UPDATE SET"
        " name=excluded.name, hourly_rate=excluded.hourly_rate, color=excluded.color,"
        " is_default=excluded.is_default, del=excluded.del,"
        " name_l=excluded.name_l, name_d=excluded.name_d,"
        " rate_l=excluded.rate_l, rate_d=excluded.rate_d,"
        " del_l=excluded.del_l, del_d=excluded.del_d, touched=excluded.touched"));
    q.addBindValue(qs(p.projectId));
    q.addBindValue(qs(p.name));
    q.addBindValue(p.hourlyRate);
    q.addBindValue(qs(p.color));
    q.addBindValue(p.isDefault ? 1 : 0);
    q.addBindValue(ll(p.created));
    q.addBindValue(p.del ? 1 : 0);
    q.addBindValue(ll(p.nameVer.lamport));
    q.addBindValue(qs(p.nameVer.deviceId));
    q.addBindValue(ll(p.rateVer.lamport));
    q.addBindValue(qs(p.rateVer.deviceId));
    q.addBindValue(ll(p.delVer.lamport));
    q.addBindValue(qs(p.delVer.deviceId));
    q.addBindValue(ll(p.touched));
    return q.exec();
}

std::vector<core::Project> Database::getProjects(bool includeDeleted) const
{
    std::vector<core::Project> out;
    QSqlQuery q(m_db);
    q.exec(QStringLiteral("SELECT * FROM projects ORDER BY is_default DESC, name"));
    while (q.next()) {
        core::Project p;
        p.projectId = ss(q.value(0).toString());
        p.name = ss(q.value(1).toString());
        p.hourlyRate = q.value(2).toDouble();
        p.color = ss(q.value(3).toString());
        p.isDefault = q.value(4).toInt() != 0;
        p.created = q.value(5).toLongLong();
        p.del = q.value(6).toInt() != 0;
        p.nameVer = {q.value(7).toLongLong(), ss(q.value(8).toString())};
        p.rateVer = {q.value(9).toLongLong(), ss(q.value(10).toString())};
        p.delVer = {q.value(11).toLongLong(), ss(q.value(12).toString())};
        p.touched = q.value(13).toLongLong();
        if (!includeDeleted && p.del)
            continue;
        out.push_back(std::move(p));
    }
    return out;
}

std::optional<core::Project> Database::getProject(const std::string& projectId) const
{
    QSqlQuery q(m_db);
    q.prepare(QStringLiteral("SELECT * FROM projects WHERE project_id=?"));
    q.addBindValue(qs(projectId));
    if (!q.exec() || !q.next())
        return std::nullopt;
    core::Project p;
    p.projectId = ss(q.value(0).toString());
    p.name = ss(q.value(1).toString());
    p.hourlyRate = q.value(2).toDouble();
    p.color = ss(q.value(3).toString());
    p.isDefault = q.value(4).toInt() != 0;
    p.created = q.value(5).toLongLong();
    p.del = q.value(6).toInt() != 0;
    p.nameVer = {q.value(7).toLongLong(), ss(q.value(8).toString())};
    p.rateVer = {q.value(9).toLongLong(), ss(q.value(10).toString())};
    p.delVer = {q.value(11).toLongLong(), ss(q.value(12).toString())};
    p.touched = q.value(13).toLongLong();
    return p;
}

bool Database::deleteProject(const std::string& projectId)
{
    QSqlQuery q(m_db);
    q.prepare(QStringLiteral("UPDATE projects SET del=1, touched=? WHERE project_id=?"));
    q.addBindValue(ll(QDateTime::currentMSecsSinceEpoch()));
    q.addBindValue(qs(projectId));
    return q.exec();
}

bool Database::setDefaultProject(const std::string& projectId)
{
    QSqlQuery q(m_db);
    q.exec(QStringLiteral("UPDATE projects SET is_default=0"));
    q.prepare(QStringLiteral("UPDATE projects SET is_default=1 WHERE project_id=?"));
    q.addBindValue(qs(projectId));
    return q.exec();
}

bool Database::upsertTimeEntry(const core::TimeEntry& e)
{
    QSqlQuery q(m_db);
    q.prepare(QStringLiteral(
        "INSERT INTO time_entries (entry_id, project_id, start_ms, end_ms, break_ms,"
        " notes, tags, reimburse, deduct, source, lat, lon, created, touched, del, del_l, del_d)"
        " VALUES (?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?)"
        " ON CONFLICT(entry_id) DO UPDATE SET"
        " project_id=excluded.project_id, start_ms=excluded.start_ms, end_ms=excluded.end_ms,"
        " break_ms=excluded.break_ms, notes=excluded.notes, tags=excluded.tags,"
        " reimburse=excluded.reimburse, deduct=excluded.deduct, source=excluded.source,"
        " lat=excluded.lat, lon=excluded.lon, touched=excluded.touched,"
        " del=excluded.del, del_l=excluded.del_l, del_d=excluded.del_d"));
    q.addBindValue(qs(e.entryId));
    q.addBindValue(qs(e.projectId));
    q.addBindValue(ll(e.startMs));
    q.addBindValue(ll(e.endMs));
    q.addBindValue(ll(e.breakMs));
    q.addBindValue(qs(e.notes));
    q.addBindValue(qs(e.tags));
    q.addBindValue(e.reimburse);
    q.addBindValue(e.deduct);
    q.addBindValue(qs(e.source));
    q.addBindValue(e.lat);
    q.addBindValue(e.lon);
    q.addBindValue(ll(e.created));
    q.addBindValue(ll(e.touched));
    q.addBindValue(e.del ? 1 : 0);
    q.addBindValue(ll(e.delVer.lamport));
    q.addBindValue(qs(e.delVer.deviceId));
    return q.exec();
}

std::vector<core::TimeEntry> Database::getTimeEntries(int64_t fromMs, int64_t toMs) const
{
    std::vector<core::TimeEntry> out;
    QSqlQuery q(m_db);
    QString sql = QStringLiteral(
        "SELECT entry_id, project_id, start_ms, end_ms, break_ms, notes, tags,"
        " reimburse, deduct, source, lat, lon, created, touched, del, del_l, del_d"
        " FROM time_entries WHERE del=0");
    if (fromMs > 0)
        sql += QStringLiteral(" AND start_ms >= ") + QString::number(fromMs);
    if (toMs > 0)
        sql += QStringLiteral(" AND (end_ms=0 OR end_ms <= ") + QString::number(toMs) + QLatin1Char(')');
    sql += QStringLiteral(" ORDER BY start_ms DESC");
    q.exec(sql);
    while (q.next()) {
        core::TimeEntry e;
        e.entryId = ss(q.value(0).toString());
        e.projectId = ss(q.value(1).toString());
        e.startMs = q.value(2).toLongLong();
        e.endMs = q.value(3).toLongLong();
        e.breakMs = q.value(4).toLongLong();
        e.notes = ss(q.value(5).toString());
        e.tags = ss(q.value(6).toString());
        e.reimburse = q.value(7).toDouble();
        e.deduct = q.value(8).toDouble();
        e.source = ss(q.value(9).toString());
        e.lat = q.value(10).toDouble();
        e.lon = q.value(11).toDouble();
        e.created = q.value(12).toLongLong();
        e.touched = q.value(13).toLongLong();
        e.del = q.value(14).toInt() != 0;
        e.delVer = {q.value(15).toLongLong(), ss(q.value(16).toString())};
        out.push_back(std::move(e));
    }
    return out;
}

std::optional<core::TimeEntry> Database::getTimeEntry(const std::string& entryId) const
{
    QSqlQuery q(m_db);
    q.prepare(QStringLiteral("SELECT * FROM time_entries WHERE entry_id=?"));
    q.addBindValue(qs(entryId));
    if (!q.exec() || !q.next())
        return std::nullopt;
    core::TimeEntry e;
    e.entryId = ss(q.value(0).toString());
    e.projectId = ss(q.value(1).toString());
    e.startMs = q.value(2).toLongLong();
    e.endMs = q.value(3).toLongLong();
    e.breakMs = q.value(4).toLongLong();
    e.notes = ss(q.value(5).toString());
    e.tags = ss(q.value(6).toString());
    e.reimburse = q.value(7).toDouble();
    e.deduct = q.value(8).toDouble();
    e.source = ss(q.value(9).toString());
    e.lat = q.value(10).toDouble();
    e.lon = q.value(11).toDouble();
    e.created = q.value(12).toLongLong();
    e.touched = q.value(13).toLongLong();
    e.del = q.value(14).toInt() != 0;
    e.delVer = {q.value(15).toLongLong(), ss(q.value(16).toString())};
    return e;
}

bool Database::deleteTimeEntry(const std::string& entryId)
{
    QSqlQuery q(m_db);
    q.prepare(QStringLiteral("UPDATE time_entries SET del=1, touched=? WHERE entry_id=?"));
    q.addBindValue(ll(QDateTime::currentMSecsSinceEpoch()));
    q.addBindValue(qs(entryId));
    return q.exec();
}

core::PunchState Database::getPunchState() const
{
    core::PunchState s;
    QSqlQuery q(m_db);
    q.exec(QStringLiteral("SELECT * FROM punch_state WHERE id=1"));
    if (q.next()) {
        s.clockedIn = q.value(1).toInt() != 0;
        s.projectId = ss(q.value(2).toString());
        s.clockInMs = q.value(3).toLongLong();
        s.breakStartMs = q.value(4).toLongLong();
        s.accumulatedBreakMs = q.value(5).toLongLong();
        s.activeEntryId = ss(q.value(6).toString());
    }
    return s;
}

bool Database::savePunchState(const core::PunchState& s)
{
    QSqlQuery q(m_db);
    q.prepare(QStringLiteral(
        "UPDATE punch_state SET clocked_in=?, project_id=?, clock_in_ms=?,"
        " break_start_ms=?, accumulated_break_ms=?, active_entry_id=? WHERE id=1"));
    q.addBindValue(s.clockedIn ? 1 : 0);
    q.addBindValue(qs(s.projectId));
    q.addBindValue(ll(s.clockInMs));
    q.addBindValue(ll(s.breakStartMs));
    q.addBindValue(ll(s.accumulatedBreakMs));
    q.addBindValue(qs(s.activeEntryId));
    return q.exec();
}

bool Database::appendAudit(const AuditEntry& e)
{
    QSqlQuery q(m_db);
    q.prepare(QStringLiteral(
        "INSERT INTO history (entry_id, action, detail, at_ms) VALUES (?,?,?,?)"));
    q.addBindValue(qs(e.entryId));
    q.addBindValue(qs(e.action));
    q.addBindValue(qs(e.detail));
    q.addBindValue(ll(e.atMs));
    return q.exec();
}

std::vector<Database::AuditEntry> Database::getAudit(int limit) const
{
    std::vector<AuditEntry> out;
    QSqlQuery q(m_db);
    q.prepare(QStringLiteral(
        "SELECT entry_id, action, detail, at_ms FROM history ORDER BY at_ms DESC LIMIT ?"));
    q.addBindValue(limit);
    q.exec();
    while (q.next()) {
        out.push_back({ss(q.value(0).toString()), ss(q.value(1).toString()),
                       ss(q.value(2).toString()), q.value(3).toLongLong()});
    }
    return out;
}

std::optional<std::string> Database::getSetting(const std::string& key) const
{
    QSqlQuery q(m_db);
    q.prepare(QStringLiteral("SELECT value FROM settings WHERE key=?"));
    q.addBindValue(qs(key));
    if (!q.exec() || !q.next())
        return std::nullopt;
    return ss(q.value(0).toString());
}

bool Database::setSetting(const std::string& key, const std::string& value)
{
    QSqlQuery q(m_db);
    q.prepare(QStringLiteral(
        "INSERT INTO settings (key, value) VALUES (?,?)"
        " ON CONFLICT(key) DO UPDATE SET value=excluded.value"));
    q.addBindValue(qs(key));
    q.addBindValue(qs(value));
    return q.exec();
}

bool Database::createWorkspace(const core::WorkspaceMeta& ws)
{
    QSqlQuery q(m_db);
    q.prepare(QStringLiteral(
        "INSERT OR REPLACE INTO workspaces (workspace_id, key, title, title_ver_l,"
        " title_ver_d, lamport, last_sync, created) VALUES (?,?,?,?,?,?,?,?)"));
    q.addBindValue(qs(ws.workspaceId));
    q.addBindValue(QByteArray(reinterpret_cast<const char*>(ws.key.data()),
                              static_cast<int>(ws.key.size())));
    q.addBindValue(qs(ws.title));
    q.addBindValue(ll(ws.titleVer.lamport));
    q.addBindValue(qs(ws.titleVer.deviceId));
    q.addBindValue(ll(ws.lamport));
    q.addBindValue(ll(ws.lastSync));
    q.addBindValue(ll(ws.created));
    return q.exec();
}

std::vector<core::WorkspaceMeta> Database::getWorkspaces() const
{
    std::vector<core::WorkspaceMeta> out;
    QSqlQuery q(m_db);
    q.exec(QStringLiteral("SELECT * FROM workspaces"));
    while (q.next()) {
        core::WorkspaceMeta ws;
        ws.workspaceId = ss(q.value(0).toString());
        const QByteArray key = q.value(1).toByteArray();
        ws.key.assign(key.begin(), key.end());
        ws.title = ss(q.value(2).toString());
        ws.titleVer = {q.value(3).toLongLong(), ss(q.value(4).toString())};
        ws.lamport = q.value(5).toLongLong();
        ws.lastSync = q.value(6).toLongLong();
        ws.created = q.value(7).toLongLong();
        out.push_back(std::move(ws));
    }
    return out;
}

std::optional<core::WorkspaceMeta> Database::getWorkspace(const std::string& workspaceId) const
{
    QSqlQuery q(m_db);
    q.prepare(QStringLiteral("SELECT * FROM workspaces WHERE workspace_id=?"));
    q.addBindValue(qs(workspaceId));
    if (!q.exec() || !q.next())
        return std::nullopt;
    core::WorkspaceMeta ws;
    ws.workspaceId = ss(q.value(0).toString());
    const QByteArray key = q.value(1).toByteArray();
    ws.key.assign(key.begin(), key.end());
    ws.title = ss(q.value(2).toString());
    ws.titleVer = {q.value(3).toLongLong(), ss(q.value(4).toString())};
    ws.lamport = q.value(5).toLongLong();
    ws.lastSync = q.value(6).toLongLong();
    ws.created = q.value(7).toLongLong();
    return ws;
}

bool Database::updateLastSync(const std::string& workspaceId, int64_t ms)
{
    QSqlQuery q(m_db);
    q.prepare(QStringLiteral(
        "UPDATE workspaces SET last_sync=? WHERE workspace_id=? AND last_sync < ?"));
    q.addBindValue(ll(ms));
    q.addBindValue(qs(workspaceId));
    q.addBindValue(ll(ms));
    return q.exec();
}

int64_t Database::bumpLamport(const std::string& workspaceId, int64_t atLeast)
{
    QSqlQuery q(m_db);
    q.prepare(QStringLiteral("SELECT lamport FROM workspaces WHERE workspace_id=?"));
    q.addBindValue(qs(workspaceId));
    if (!q.exec() || !q.next())
        return -1;
    const int64_t cur = q.value(0).toLongLong();
    const int64_t next = std::max(cur + 1, atLeast);
    QSqlQuery u(m_db);
    u.prepare(QStringLiteral("UPDATE workspaces SET lamport=? WHERE workspace_id=?"));
    u.addBindValue(ll(next));
    u.addBindValue(qs(workspaceId));
    if (!u.exec())
        return -1;
    return next;
}

bool Database::outboxPush(const std::string& workspaceId, const std::string& eventJson)
{
    QSqlQuery q(m_db);
    q.prepare(QStringLiteral(
        "INSERT INTO outbox (workspace_id, event_json, created) VALUES (?,?,?)"));
    q.addBindValue(qs(workspaceId));
    q.addBindValue(qs(eventJson));
    q.addBindValue(ll(QDateTime::currentMSecsSinceEpoch()));
    return q.exec();
}

bool Database::outboxRemove(int64_t rowid)
{
    QSqlQuery q(m_db);
    q.prepare(QStringLiteral("DELETE FROM outbox WHERE rowid=?"));
    q.addBindValue(static_cast<qlonglong>(rowid));
    return q.exec();
}

std::vector<std::pair<int64_t, std::string>> Database::outboxPeekAll(const std::string& workspaceId)
{
    std::vector<std::pair<int64_t, std::string>> out;
    QSqlQuery q(m_db);
    q.prepare(QStringLiteral(
        "SELECT rowid, event_json FROM outbox WHERE workspace_id=? ORDER BY rowid"));
    q.addBindValue(qs(workspaceId));
    q.exec();
    while (q.next())
        out.emplace_back(q.value(0).toLongLong(), ss(q.value(1).toString()));
    return out;
}

int Database::outboxCount() const
{
    QSqlQuery q(m_db);
    q.exec(QStringLiteral("SELECT COUNT(*) FROM outbox"));
    return q.next() ? q.value(0).toInt() : 0;
}

bool Database::markEventSeen(const std::string& eventId)
{
    QSqlQuery q(m_db);
    q.prepare(QStringLiteral(
        "INSERT OR IGNORE INTO seen_events (event_id, seen_at) VALUES (?,?)"));
    q.addBindValue(qs(eventId));
    q.addBindValue(ll(QDateTime::currentMSecsSinceEpoch()));
    return q.exec();
}

bool Database::isEventSeen(const std::string& eventId) const
{
    QSqlQuery q(m_db);
    q.prepare(QStringLiteral("SELECT 1 FROM seen_events WHERE event_id=?"));
    q.addBindValue(qs(eventId));
    return q.exec() && q.next();
}

bool Database::upsertMember(const std::string& workspaceId, const std::string& deviceId,
                            const std::string& name, const core::Ver& ver)
{
    QSqlQuery q(m_db);
    q.prepare(QStringLiteral(
        "INSERT INTO members (workspace_id, device_id, name, ver_l, ver_d)"
        " VALUES (?,?,?,?,?) ON CONFLICT(workspace_id, device_id) DO UPDATE SET"
        " name=excluded.name, ver_l=excluded.ver_l, ver_d=excluded.ver_d"));
    q.addBindValue(qs(workspaceId));
    q.addBindValue(qs(deviceId));
    q.addBindValue(qs(name));
    q.addBindValue(ll(ver.lamport));
    q.addBindValue(qs(ver.deviceId));
    return q.exec();
}

std::vector<std::pair<std::string, std::string>> Database::getMembers(const std::string& workspaceId)
{
    std::vector<std::pair<std::string, std::string>> out;
    QSqlQuery q(m_db);
    q.prepare(QStringLiteral("SELECT device_id, name FROM members WHERE workspace_id=?"));
    q.addBindValue(qs(workspaceId));
    q.exec();
    while (q.next())
        out.emplace_back(ss(q.value(0).toString()), ss(q.value(1).toString()));
    return out;
}

} // namespace store

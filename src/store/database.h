#pragma once

#include "../core/types.h"

#include <QSqlDatabase>
#include <QString>
#include <optional>
#include <string>
#include <vector>

namespace store {

class Database
{
public:
    Database() = default;
    ~Database();

    bool open(const QString& path);
    void close();
    bool isOpen() const { return m_db.isOpen(); }

    // --- Projects ---
    bool upsertProject(const core::Project& p);
    std::vector<core::Project> getProjects(bool includeDeleted = false) const;
    std::optional<core::Project> getProject(const std::string& projectId) const;
    bool deleteProject(const std::string& projectId);
    bool setDefaultProject(const std::string& projectId);

    // --- Time entries ---
    bool upsertTimeEntry(const core::TimeEntry& e);
    std::vector<core::TimeEntry> getTimeEntries(int64_t fromMs = 0, int64_t toMs = 0) const;
    std::optional<core::TimeEntry> getTimeEntry(const std::string& entryId) const;
    bool deleteTimeEntry(const std::string& entryId);

    // --- Punch state (singleton) ---
    core::PunchState getPunchState() const;
    bool savePunchState(const core::PunchState& s);

    // --- History audit (local) ---
    struct AuditEntry {
        std::string entryId;
        std::string action;
        std::string detail;
        int64_t atMs = 0;
    };
    bool appendAudit(const AuditEntry& e);
    std::vector<AuditEntry> getAudit(int limit = 200) const;

    // --- Settings ---
    std::optional<std::string> getSetting(const std::string& key) const;
    bool setSetting(const std::string& key, const std::string& value);

    // --- Sync tables (Colo Course pattern) ---
    bool createWorkspace(const core::WorkspaceMeta& ws);
    std::vector<core::WorkspaceMeta> getWorkspaces() const;
    std::optional<core::WorkspaceMeta> getWorkspace(const std::string& workspaceId) const;
    bool updateLastSync(const std::string& workspaceId, int64_t ms);
    int64_t bumpLamport(const std::string& workspaceId, int64_t atLeast = 0);

    bool outboxPush(const std::string& workspaceId, const std::string& eventJson);
    bool outboxRemove(int64_t rowid);
    std::vector<std::pair<int64_t, std::string>> outboxPeekAll(const std::string& workspaceId);
    int outboxCount() const;

    bool markEventSeen(const std::string& eventId);
    bool isEventSeen(const std::string& eventId) const;

    bool upsertMember(const std::string& workspaceId, const std::string& deviceId,
                      const std::string& name, const core::Ver& ver);
    std::vector<std::pair<std::string, std::string>> getMembers(const std::string& workspaceId);

private:
    bool createSchema();
    bool migrateSchema();

    QSqlDatabase m_db;
    QString      m_connectionName;
};

} // namespace store

#include "punchengine.h"

#include <QDateTime>
#include <QUuid>

namespace app {

PunchEngine::PunchEngine(store::Database* db)
    : m_db(db)
{
    reload();
    if (auto id = m_db->getSetting("deviceId"))
        m_deviceId = *id;
}

void PunchEngine::reload()
{
    if (m_db)
        m_state = m_db->getPunchState();
}

void PunchEngine::persist()
{
    if (m_db)
        m_db->savePunchState(m_state);
}

void PunchEngine::audit(const std::string& entryId, const std::string& action,
                        const std::string& detail)
{
    if (!m_db)
        return;
    m_db->appendAudit({entryId, action, detail, QDateTime::currentMSecsSinceEpoch()});
}

bool PunchEngine::clockIn(const std::string& projectId, double lat, double lon)
{
    if (m_state.clockedIn || !m_db || projectId.empty())
        return false;

    const int64_t now = QDateTime::currentMSecsSinceEpoch();
    core::TimeEntry e;
    e.entryId = QUuid::createUuid().toString(QUuid::WithoutBraces).toStdString();
    e.projectId = projectId;
    e.startMs = now;
    e.endMs = 0;
    e.source = "punch";
    e.created = now;
    e.touched = now;
    e.lat = lat;
    e.lon = lon;
    if (!m_db->upsertTimeEntry(e))
        return false;

    m_state.clockedIn = true;
    m_state.projectId = projectId;
    m_state.clockInMs = now;
    m_state.breakStartMs = 0;
    m_state.accumulatedBreakMs = 0;
    m_state.activeEntryId = e.entryId;
    persist();
    audit(e.entryId, "clock_in", projectId);
    return true;
}

bool PunchEngine::clockOut(double lat, double lon)
{
    if (!m_state.clockedIn || !m_db)
        return false;

    const int64_t now = QDateTime::currentMSecsSinceEpoch();
    if (m_state.breakStartMs > 0)
        endBreak();

    auto entry = m_db->getTimeEntry(m_state.activeEntryId);
    if (!entry)
        return false;

    entry->endMs = now;
    entry->breakMs = m_state.accumulatedBreakMs;
    entry->touched = now;
    if (lat != 0.0 || lon != 0.0) {
        entry->lat = lat;
        entry->lon = lon;
    }
    if (!m_db->upsertTimeEntry(*entry))
        return false;

    m_state = {};
    persist();
    audit(entry->entryId, "clock_out", "");
    return true;
}

bool PunchEngine::startBreak()
{
    if (!m_state.clockedIn || m_state.breakStartMs > 0)
        return false;
    m_state.breakStartMs = QDateTime::currentMSecsSinceEpoch();
    persist();
    audit(m_state.activeEntryId, "break_start", "");
    return true;
}

bool PunchEngine::endBreak()
{
    if (m_state.breakStartMs <= 0)
        return false;
    const int64_t now = QDateTime::currentMSecsSinceEpoch();
    m_state.accumulatedBreakMs += now - m_state.breakStartMs;
    m_state.breakStartMs = 0;
    persist();
    audit(m_state.activeEntryId, "break_end", "");
    return true;
}

int64_t PunchEngine::liveElapsedMs(int64_t nowMs) const
{
    if (!m_state.clockedIn)
        return 0;
    return nowMs - m_state.clockInMs;
}

int64_t PunchEngine::liveBreakMs(int64_t nowMs) const
{
    int64_t total = m_state.accumulatedBreakMs;
    if (m_state.breakStartMs > 0)
        total += nowMs - m_state.breakStartMs;
    return total;
}

} // namespace app

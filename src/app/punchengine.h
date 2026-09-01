#pragma once

#include "../core/types.h"
#include "../store/database.h"

#include <QString>

namespace app {

class PunchEngine
{
public:
    explicit PunchEngine(store::Database* db);

    core::PunchState state() const { return m_state; }
    void reload();

    bool clockIn(const std::string& projectId, double lat = 0, double lon = 0);
    bool clockOut(double lat = 0, double lon = 0);
    bool startBreak();
    bool endBreak();

    int64_t liveElapsedMs(int64_t nowMs) const;
    int64_t liveBreakMs(int64_t nowMs) const;

private:
    void persist();
    void audit(const std::string& entryId, const std::string& action,
               const std::string& detail);

    store::Database*     m_db = nullptr;
    core::PunchState     m_state;
    std::string          m_deviceId;
};

} // namespace app

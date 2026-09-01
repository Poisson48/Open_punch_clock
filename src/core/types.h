#pragma once

#include <cstdint>
#include <string>
#include <compare>
#include <vector>

namespace core {

struct Ver {
    int64_t lamport = 0;
    std::string deviceId;

    auto operator<=>(const Ver& o) const noexcept {
        if (lamport != o.lamport)
            return lamport <=> o.lamport;
        return deviceId <=> o.deviceId;
    }
    bool operator==(const Ver& o) const noexcept = default;
};

struct Project {
    std::string projectId;
    std::string name;
    double      hourlyRate = 0.0;
    std::string color;       // #RRGGBB
    bool        isDefault = false;
    int64_t     created = 0;
    bool        del = false;
    Ver         nameVer;
    Ver         rateVer;
    Ver         delVer;
    int64_t     touched = 0;
};

struct TimeEntry {
    std::string entryId;
    std::string projectId;
    int64_t     startMs = 0;
    int64_t     endMs = 0;       // 0 = en cours
    int64_t     breakMs = 0;
    std::string notes;
    std::string tags;
    double      reimburse = 0.0;
    double      deduct = 0.0;
    std::string source;          // punch | manual
    double      lat = 0.0;
    double      lon = 0.0;
    int64_t     created = 0;
    int64_t     touched = 0;
    bool        del = false;
    Ver         delVer;
};

struct PunchState {
    bool        clockedIn = false;
    std::string projectId;
    int64_t     clockInMs = 0;
    int64_t     breakStartMs = 0;
    int64_t     accumulatedBreakMs = 0;
    std::string activeEntryId;
};

struct WorkspaceMeta {
    std::string workspaceId;
    std::vector<uint8_t> key;
    std::string title;
    Ver         titleVer;
    int64_t     lamport = 0;
    int64_t     lastSync = 0;
    int64_t     created = 0;
};

} // namespace core

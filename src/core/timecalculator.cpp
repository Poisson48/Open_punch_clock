#include "timecalculator.h"

#include <algorithm>
#include <cmath>

namespace core {

DurationResult computeDuration(const TimeEntry& entry, double hourlyRate,
                               int64_t nowMs)
{
    DurationResult r;
    if (entry.startMs <= 0)
        return r;

    const int64_t end = entry.endMs > 0 ? entry.endMs : nowMs;
    r.grossMs = std::max(int64_t(0), end - entry.startMs);
    r.netMs = std::max(int64_t(0), r.grossMs - entry.breakMs);
    r.hours = static_cast<double>(r.netMs) / 3600000.0;
    r.earnings = r.hours * hourlyRate + entry.reimburse - entry.deduct;
    return r;
}

double totalHoursInRange(const std::vector<TimeEntry>& entries,
                         int64_t fromMs, int64_t toMs,
                         const std::string& projectId)
{
    double total = 0.0;
    for (const auto& e : entries) {
        if (e.del)
            continue;
        if (!projectId.empty() && e.projectId != projectId)
            continue;
        const int64_t end = e.endMs > 0 ? e.endMs : toMs;
        const int64_t start = std::max(e.startMs, fromMs);
        const int64_t finish = std::min(end, toMs);
        if (finish <= start)
            continue;
        const int64_t gross = finish - start;
        const int64_t breakPart = std::min(e.breakMs, gross);
        total += static_cast<double>(gross - breakPart) / 3600000.0;
    }
    return total;
}

double overtimeHours(double totalHours, double thresholdHours)
{
    if (thresholdHours <= 0.0)
        return 0.0;
    return std::max(0.0, totalHours - thresholdHours);
}

} // namespace core

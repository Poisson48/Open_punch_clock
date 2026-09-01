#pragma once

#include "types.h"

#include <cstdint>

namespace core {

struct DurationResult {
    int64_t grossMs = 0;
    int64_t netMs = 0;
    double  hours = 0.0;
    double  earnings = 0.0;
};

// Calcule durée et gains pour une entrée terminée ou en cours.
DurationResult computeDuration(const TimeEntry& entry, double hourlyRate,
                               int64_t nowMs);

// Heures travaillées dans [fromMs, toMs) pour une liste d'entrées.
double totalHoursInRange(const std::vector<TimeEntry>& entries,
                         int64_t fromMs, int64_t toMs,
                         const std::string& projectId = "");

// Heures au-delà du seuil heures sup sur la période.
double overtimeHours(double totalHours, double thresholdHours);

} // namespace core

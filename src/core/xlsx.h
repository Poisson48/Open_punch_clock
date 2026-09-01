#pragma once

#include <string>
#include <vector>

namespace core {

// Génère un fichier XLSX minimal (OOXML dans ZIP store-only).
std::string xlsxWrite(const std::vector<std::vector<std::string>>& rows);

} // namespace core

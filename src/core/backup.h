#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace core {

// Archive chiffrée (libsodium secretbox) contenant un ZIP store-only :
// openpunchclock.db + manifest.json
std::optional<std::vector<uint8_t>> backupEncrypt(const std::string& dbPath,
                                                  const std::string& keyMaterial);

std::optional<std::string> backupDecryptToZip(const std::vector<uint8_t>& blob,
                                              const std::string& keyMaterial);

} // namespace core

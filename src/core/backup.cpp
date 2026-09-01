#include "backup.h"
#include "zip.h"

#include <nlohmann/json.hpp>
#include <sodium.h>

#include <QDateTime>
#include <QFile>

#include <cstring>

namespace core {

namespace {

constexpr char kMagic[] = "OPCBK1";
constexpr char kDomain[] = "openpunchclock/backup/v1";

std::vector<uint8_t> deriveKey(const std::string& keyMaterial)
{
    std::vector<uint8_t> key(crypto_secretbox_KEYBYTES);
    crypto_generichash(key.data(), key.size(),
                       reinterpret_cast<const unsigned char*>(keyMaterial.data()),
                       keyMaterial.size(),
                       reinterpret_cast<const unsigned char*>(kDomain),
                       std::strlen(kDomain));
    return key;
}

std::optional<std::vector<uint8_t>> readFile(const std::string& path)
{
    QFile f(QString::fromStdString(path));
    if (!f.open(QIODevice::ReadOnly))
        return std::nullopt;
    const QByteArray data = f.readAll();
    return std::vector<uint8_t>(data.begin(), data.end());
}

} // namespace

std::optional<std::vector<uint8_t>> backupEncrypt(const std::string& dbPath,
                                                   const std::string& keyMaterial)
{
    if (sodium_init() < 0)
        return std::nullopt;

    auto dbBytes = readFile(dbPath);
    if (!dbBytes || dbBytes->empty())
        return std::nullopt;

    nlohmann::json manifest;
    manifest["version"] = 1;
    manifest["exportedAt"] = QDateTime::currentDateTimeUtc().toString(Qt::ISODate).toStdString();
    const std::string manifestStr = manifest.dump();

    std::vector<ZipEntry> entries;
    entries.push_back({ "openpunchclock.db",
                        std::string(dbBytes->begin(), dbBytes->end()) });
    entries.push_back({ "manifest.json", manifestStr });
    const std::string zipData = zipWrite(entries);

    const auto key = deriveKey(keyMaterial);
    std::vector<uint8_t> nonce(crypto_secretbox_NONCEBYTES);
    randombytes_buf(nonce.data(), nonce.size());

    std::vector<uint8_t> cipher(zipData.size() + crypto_secretbox_MACBYTES);
    if (crypto_secretbox_easy(cipher.data(),
                              reinterpret_cast<const unsigned char*>(zipData.data()),
                              zipData.size(),
                              nonce.data(),
                              key.data()) != 0) {
        return std::nullopt;
    }

    std::vector<uint8_t> out;
    out.insert(out.end(), kMagic, kMagic + std::strlen(kMagic));
    out.insert(out.end(), nonce.begin(), nonce.end());
    out.insert(out.end(), cipher.begin(), cipher.end());
    return out;
}

std::optional<std::string> backupDecryptToZip(const std::vector<uint8_t>& blob,
                                              const std::string& keyMaterial)
{
    if (sodium_init() < 0)
        return std::nullopt;

    const size_t header = std::strlen(kMagic) + crypto_secretbox_NONCEBYTES;
    if (blob.size() < header + crypto_secretbox_MACBYTES)
        return std::nullopt;
    if (std::memcmp(blob.data(), kMagic, std::strlen(kMagic)) != 0)
        return std::nullopt;

    const auto key = deriveKey(keyMaterial);
    const unsigned char* nonce = blob.data() + std::strlen(kMagic);
    const unsigned char* cipher = nonce + crypto_secretbox_NONCEBYTES;
    const size_t cipherLen = blob.size() - header;

    std::vector<unsigned char> plain(cipherLen - crypto_secretbox_MACBYTES);
    if (crypto_secretbox_open_easy(plain.data(), cipher, cipherLen, nonce, key.data()) != 0)
        return std::nullopt;

    return std::string(plain.begin(), plain.end());
}

} // namespace core

#pragma once

#include "../store/database.h"
#include "../net/relaypool.h"
#include "../net/nostr.h"

#include <QObject>
#include <QTimer>
#include <QUrl>
#include <QString>

namespace app {

class SyncEngine : public QObject
{
    Q_OBJECT

public:
    explicit SyncEngine(QObject* parent = nullptr);
    ~SyncEngine() override;

    void init(store::Database* db, net::RelayPool* pool,
              const QString& deviceId, const QString& displayName);

    bool isOnline() const;
    void onLocalChange();
    void catchUpOnForeground();
    void shutdown();

    void createWorkspace(const QString& title);
    QString joinUri() const;
    void handleJoinUrl(const QUrl& url);

signals:
    void onlineChanged(bool online);
    void outboxChanged();
    void remoteChanges();

private slots:
    void onRelayEvent(const net::NostrEvent& ev);
    void onRelayOnline(bool online);
    void onDebounce();

private:
    void subscribeAll();
    void publishDelta();
    void flushOutbox();
    std::string buildPayloadJson() const;
    void applyPayloadJson(const std::string& json);
    std::optional<std::string> workspaceId() const;
    std::optional<std::vector<uint8_t>> workspaceKey() const;

    store::Database* m_db = nullptr;
    net::RelayPool*  m_pool = nullptr;
    QString          m_deviceId;
    QString          m_displayName;
    QTimer*          m_debounce = nullptr;
    bool             m_subscribed = false;
};

} // namespace app

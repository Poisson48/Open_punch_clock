#pragma once

#include "../store/database.h"
#include "punchengine.h"
#include "projectmodel.h"
#include "timeentrymodel.h"

#include <QObject>
#include <QTimer>
#include <QString>
#include <QVariantList>

namespace net { class RelayPool; }

namespace app {

class SyncEngine;

class AppController : public QObject
{
    Q_OBJECT

    Q_PROPERTY(bool clockedIn READ clockedIn NOTIFY punchChanged)
    Q_PROPERTY(bool onBreak READ onBreak NOTIFY punchChanged)
    Q_PROPERTY(QString activeProjectId READ activeProjectId NOTIFY punchChanged)
    Q_PROPERTY(qint64 liveElapsedMs READ liveElapsedMs NOTIFY tick)
    Q_PROPERTY(qint64 liveBreakMs READ liveBreakMs NOTIFY tick)
    Q_PROPERTY(double liveEarnings READ liveEarnings NOTIFY tick)
    Q_PROPERTY(QString deviceId READ deviceId CONSTANT)
    Q_PROPERTY(bool online READ online NOTIFY onlineChanged)
    Q_PROPERTY(int pendingChanges READ pendingChanges NOTIFY outboxChanged)
    Q_PROPERTY(int reminderMinutes READ reminderMinutes WRITE setReminderMinutes NOTIFY settingsChanged)
    Q_PROPERTY(int payPeriodDays READ payPeriodDays WRITE setPayPeriodDays NOTIFY settingsChanged)
    Q_PROPERTY(double overtimeThreshold READ overtimeThreshold WRITE setOvertimeThreshold NOTIFY settingsChanged)
    Q_PROPERTY(bool gpsEnabled READ gpsEnabled WRITE setGpsEnabled NOTIFY settingsChanged)
    Q_PROPERTY(bool syncEnabled READ syncEnabled NOTIFY settingsChanged)
    Q_PROPERTY(QString locale READ locale WRITE setLocale NOTIFY localeChanged)
    Q_PROPERTY(QVariantList availableLocales READ availableLocales CONSTANT)
    Q_PROPERTY(QString appVersion READ appVersion CONSTANT)
    Q_PROPERTY(QString toastMessage READ toastMessage NOTIFY toastChanged)
    Q_PROPERTY(bool keepScreenOn READ keepScreenOn WRITE setKeepScreenOn NOTIFY settingsChanged)

    Q_PROPERTY(ProjectModel* projects READ projects CONSTANT)
    Q_PROPERTY(TimeEntryModel* entries READ entries CONSTANT)

public:
    explicit AppController(QObject* parent = nullptr);
    ~AppController() override;

    bool init();
    void shutdown();

    static QString databasePath();

    bool clockedIn() const;
    bool onBreak() const;
    QString activeProjectId() const;
    qint64 liveElapsedMs() const;
    qint64 liveBreakMs() const;
    double liveEarnings() const;
    QString deviceId() const { return m_deviceId; }
    bool online() const;
    int pendingChanges() const;
    int reminderMinutes() const;
    int payPeriodDays() const;
    double overtimeThreshold() const;
    bool gpsEnabled() const;
    bool syncEnabled() const;
    QString locale() const { return m_locale; }
    QVariantList availableLocales() const;
    QString appVersion() const;
    QString toastMessage() const { return m_toastMessage; }
    bool keepScreenOn() const { return m_keepScreenOn; }

    ProjectModel* projects() { return &m_projects; }
    TimeEntryModel* entries() { return &m_entries; }

    Q_INVOKABLE bool punchIn(const QString& projectId);
    Q_INVOKABLE bool punchOut();
    Q_INVOKABLE bool startBreak();
    Q_INVOKABLE bool endBreak();

    Q_INVOKABLE void setReminderMinutes(int m);
    Q_INVOKABLE void setPayPeriodDays(int d);
    Q_INVOKABLE void setOvertimeThreshold(double h);
    Q_INVOKABLE void setGpsEnabled(bool on);
    Q_INVOKABLE void setLocale(const QString& code);

    Q_INVOKABLE QString exportCsv(qint64 fromMs, qint64 toMs);
    Q_INVOKABLE bool writeCsvFile(const QString& path, qint64 fromMs, qint64 toMs);
    Q_INVOKABLE bool writeXlsxFile(const QString& path, qint64 fromMs, qint64 toMs);

    Q_INVOKABLE QString suggestedExportPath(const QString& ext) const;

    Q_INVOKABLE bool shareCsvWeek();
    Q_INVOKABLE bool shareXlsxWeek();
    Q_INVOKABLE bool shareCsvMonth();
    Q_INVOKABLE bool shareXlsxMonth();

    Q_INVOKABLE bool exportBackup(const QString& path, const QString& passphrase);
    Q_INVOKABLE bool importBackup(const QString& path, const QString& passphrase);
    Q_INVOKABLE bool shareBackup(const QString& passphrase);

    Q_INVOKABLE QString suggestedBackupPath() const;

    Q_INVOKABLE void processLaunchIntent();

    Q_INVOKABLE bool copyToClipboard(const QString& text);
    Q_INVOKABLE void showToast(const QString& message);
    Q_INVOKABLE void clearToast();
    Q_INVOKABLE void setKeepScreenOn(bool on);

    Q_INVOKABLE QVariantMap weekReport();
    Q_INVOKABLE QVariantMap monthReport();
    Q_INVOKABLE QVariantList weekReportByProject();

    Q_INVOKABLE QVariantList auditLog(int limit = 100);

    Q_INVOKABLE void enableSync(const QString& title);
    Q_INVOKABLE QString syncJoinUri() const;

    Q_INVOKABLE void onApplicationStateChanged(Qt::ApplicationState state);

public slots:
    void handleJoinUrl(const QUrl& url);

signals:
    void punchChanged();
    void tick();
    void onlineChanged(bool online);
    void outboxChanged();
    void settingsChanged();
    void localeChanged();
    void retranslateRequested();
    void reminderTriggered();
    void toastChanged();

private slots:
    void onTick();
    void checkReminder();

private:
    void captureGps(double* lat, double* lon);
    void refreshWidget();
    QString formatDuration(qint64 ms) const;
    std::vector<std::vector<std::string>> buildExportRows(qint64 fromMs, qint64 toMs) const;

    store::Database  m_db;
    PunchEngine      m_punch;
    ProjectModel     m_projects;
    TimeEntryModel   m_entries;
    SyncEngine*      m_sync = nullptr;
    net::RelayPool*  m_pool = nullptr;
    QTimer           m_tickTimer;
    QTimer           m_reminderTimer;
    QString          m_deviceId;
    int              m_reminderMinutes = 480;
    int              m_payPeriodDays = 14;
    double           m_overtimeThreshold = 35.0;
    bool             m_gpsEnabled = false;
    bool             m_keepScreenOn = false;
    bool             m_reminderNotified = false;
    QString          m_locale;
    QString          m_toastMessage;
};

} // namespace app

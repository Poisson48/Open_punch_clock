#include "appcontroller.h"
#include "i18n.h"
#include "syncengine.h"

#include "../core/backup.h"
#include "../core/csv.h"
#include "../core/timecalculator.h"
#include "../core/xlsx.h"
#include "../core/zip.h"
#include "../net/relaypool.h"
#include "platform.h"

#include <QDate>
#include <QDir>
#include <QHash>
#include <QFile>
#include <QStandardPaths>
#include <QTime>
#include <QUrl>
#include <QUuid>
#include <QVariantMap>

#ifdef PUNCH_HAS_GPS
#  include <QGeoPositionInfoSource>
#endif

namespace app {

QString AppController::databasePath()
{
    return QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)
        + QStringLiteral("/openpunchclock.db");
}

AppController::AppController(QObject* parent)
    : QObject(parent)
    , m_punch(&m_db)
    , m_projects(&m_db)
    , m_entries(&m_db, &m_projects)
{
    connect(&m_tickTimer, &QTimer::timeout, this, &AppController::onTick);
    m_tickTimer.setInterval(1000);
    connect(&m_reminderTimer, &QTimer::timeout, this, &AppController::checkReminder);
    m_reminderTimer.setInterval(60000);
}

AppController::~AppController()
{
    shutdown();
}

bool AppController::init()
{
    QDir().mkpath(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation));
    if (!m_db.open(databasePath()))
        return false;

    if (auto id = m_db.getSetting("deviceId")) {
        m_deviceId = QString::fromStdString(*id);
    } else {
        m_deviceId = QUuid::createUuid().toString(QUuid::WithoutBraces);
        m_db.setSetting("deviceId", m_deviceId.toStdString());
    }

    if (auto r = m_db.getSetting("reminderMinutes"))
        m_reminderMinutes = std::stoi(*r);
    if (auto p = m_db.getSetting("payPeriodDays"))
        m_payPeriodDays = std::stoi(*p);
    if (auto o = m_db.getSetting("overtimeThreshold"))
        m_overtimeThreshold = std::stod(*o);
    if (auto g = m_db.getSetting("gpsEnabled"))
        m_gpsEnabled = *g == "1";
    if (auto k = m_db.getSetting("keepScreenOn"))
        m_keepScreenOn = *k == "1";
    if (auto loc = m_db.getSetting("locale"))
        m_locale = I18n::normalizeLocale(QString::fromStdString(*loc));
    else
        m_locale = I18n::systemLocale();

    m_punch.reload();
    m_projects.reload();
    m_entries.reload();
    if (m_punch.state().clockedIn)
        m_tickTimer.start();
    m_reminderTimer.start();

    m_pool = new net::RelayPool(this);
    m_sync = new SyncEngine(this);
    m_sync->init(&m_db, m_pool, m_deviceId, QStringLiteral("User"));
    connect(m_sync, &SyncEngine::onlineChanged, this, &AppController::onlineChanged);
    connect(m_sync, &SyncEngine::outboxChanged, this, &AppController::outboxChanged);
    connect(m_sync, &SyncEngine::remoteChanges, this, [this]() {
        m_entries.reload();
        m_projects.reload();
    });

    emit settingsChanged();
    platformKeepScreenOn(m_keepScreenOn);
    refreshWidget();
    return true;
}

void AppController::shutdown()
{
    m_tickTimer.stop();
    if (m_sync)
        m_sync->shutdown();
}

bool AppController::clockedIn() const { return m_punch.state().clockedIn; }
bool AppController::onBreak() const { return m_punch.state().breakStartMs > 0; }
QString AppController::activeProjectId() const {
    return QString::fromStdString(m_punch.state().projectId);
}

qint64 AppController::liveElapsedMs() const
{
    return m_punch.liveElapsedMs(QDateTime::currentMSecsSinceEpoch());
}

qint64 AppController::liveBreakMs() const
{
    return m_punch.liveBreakMs(QDateTime::currentMSecsSinceEpoch());
}

double AppController::liveEarnings() const
{
    if (!clockedIn())
        return 0.0;
    core::TimeEntry e;
    e.startMs = m_punch.state().clockInMs;
    e.endMs = 0;
    e.breakMs = liveBreakMs();
    e.reimburse = 0;
    e.deduct = 0;
    const double rate = m_projects.hourlyRateFor(activeProjectId());
    return core::computeDuration(e, rate, QDateTime::currentMSecsSinceEpoch()).earnings;
}

bool AppController::online() const { return m_sync && m_sync->isOnline(); }
int AppController::pendingChanges() const { return m_db.outboxCount(); }
int AppController::reminderMinutes() const { return m_reminderMinutes; }
int AppController::payPeriodDays() const { return m_payPeriodDays; }
double AppController::overtimeThreshold() const { return m_overtimeThreshold; }
bool AppController::gpsEnabled() const { return m_gpsEnabled; }
bool AppController::syncEnabled() const { return !m_db.getWorkspaces().empty(); }

QString AppController::appVersion() const
{
#ifndef PUNCH_APP_VERSION
    return QStringLiteral("0.0.0");
#else
    return QStringLiteral(PUNCH_APP_VERSION);
#endif
}

QString AppController::formatDuration(qint64 ms) const
{
    if (ms <= 0)
        return QStringLiteral("00:00:00");
    const qint64 s = ms / 1000;
    const qint64 h = s / 3600;
    const qint64 m = (s % 3600) / 60;
    const qint64 sec = s % 60;
    return QStringLiteral("%1:%2:%3")
        .arg(h, 2, 10, QChar('0'))
        .arg(m, 2, 10, QChar('0'))
        .arg(sec, 2, 10, QChar('0'));
}

void AppController::refreshWidget()
{
    const qint64 elapsed = liveElapsedMs() - liveBreakMs();
    QString status;
    if (onBreak())
        status = tr("En pause");
    else if (clockedIn())
        status = tr("En service");
    else
        status = tr("Prêt à pointer");

    platformUpdateWidget(clockedIn(), onBreak(), formatDuration(elapsed), status);
}

void AppController::captureGps(double* lat, double* lon)
{
    *lat = *lon = 0.0;
#ifdef PUNCH_HAS_GPS
    if (!m_gpsEnabled)
        return;
    if (auto* src = QGeoPositionInfoSource::createDefaultSource(this)) {
        const QGeoPositionInfo info = src->lastKnownPosition();
        if (info.isValid()) {
            *lat = info.coordinate().latitude();
            *lon = info.coordinate().longitude();
        }
        delete src;
    }
#else
    Q_UNUSED(lat);
    Q_UNUSED(lon);
#endif
}

bool AppController::punchIn(const QString& projectId)
{
    double lat = 0, lon = 0;
    captureGps(&lat, &lon);
    if (!m_punch.clockIn(projectId.toStdString(), lat, lon))
        return false;
    m_reminderNotified = false;
    m_tickTimer.start();
    platformVibrate(40);
    if (m_sync)
        m_sync->onLocalChange();
    emit punchChanged();
    refreshWidget();
    return true;
}

bool AppController::punchOut()
{
    double lat = 0, lon = 0;
    captureGps(&lat, &lon);
    if (!m_punch.clockOut(lat, lon))
        return false;
    m_tickTimer.stop();
    m_entries.reload();
    platformVibrate(60);
    if (m_sync)
        m_sync->onLocalChange();
    emit punchChanged();
    refreshWidget();
    return true;
}

bool AppController::startBreak()
{
    if (!m_punch.startBreak())
        return false;
    emit punchChanged();
    refreshWidget();
    return true;
}

bool AppController::endBreak()
{
    if (!m_punch.endBreak())
        return false;
    emit punchChanged();
    refreshWidget();
    return true;
}

void AppController::setReminderMinutes(int m)
{
    m_reminderMinutes = m;
    m_db.setSetting("reminderMinutes", std::to_string(m));
    emit settingsChanged();
}

void AppController::setPayPeriodDays(int d)
{
    m_payPeriodDays = d;
    m_db.setSetting("payPeriodDays", std::to_string(d));
    emit settingsChanged();
}

void AppController::setOvertimeThreshold(double h)
{
    m_overtimeThreshold = h;
    m_db.setSetting("overtimeThreshold", std::to_string(h));
    emit settingsChanged();
}

void AppController::setGpsEnabled(bool on)
{
    m_gpsEnabled = on;
    m_db.setSetting("gpsEnabled", on ? "1" : "0");
    emit settingsChanged();
}

void AppController::setKeepScreenOn(bool on)
{
    m_keepScreenOn = on;
    m_db.setSetting("keepScreenOn", on ? "1" : "0");
    platformKeepScreenOn(on);
    emit settingsChanged();
}

void AppController::showToast(const QString& message)
{
    if (message.isEmpty())
        return;
    m_toastMessage = message;
    emit toastChanged();
    platformShowToast(message);
}

void AppController::clearToast()
{
    if (m_toastMessage.isEmpty())
        return;
    m_toastMessage.clear();
    emit toastChanged();
}

QVariantList AppController::availableLocales() const
{
    return I18n::availableLocales();
}

void AppController::setLocale(const QString& code)
{
    const QString norm = I18n::normalizeLocale(code);
    if (m_locale == norm)
        return;
    m_locale = norm;
    m_db.setSetting("locale", m_locale.toStdString());
    m_entries.reload();
    emit localeChanged();
    emit retranslateRequested();
}

void AppController::onTick()
{
    emit tick();
    refreshWidget();
}

void AppController::checkReminder()
{
    if (!clockedIn() || onBreak()) {
        m_reminderNotified = false;
        return;
    }
    const qint64 elapsedMin = liveElapsedMs() / 60000;
    if (elapsedMin >= m_reminderMinutes && !m_reminderNotified) {
        m_reminderNotified = true;
        platformNotify(tr("Open Punch Clock"),
                       tr("N'oubliez pas de pointer la sortie"),
                       QDateTime::currentMSecsSinceEpoch());
        emit reminderTriggered();
    }
}

std::vector<std::vector<std::string>> AppController::buildExportRows(qint64 fromMs,
                                                                   qint64 toMs) const
{
    std::vector<std::vector<std::string>> rows;
    rows.push_back({
        tr("Date").toStdString(),
        tr("Projet").toStdString(),
        tr("Début").toStdString(),
        tr("Fin").toStdString(),
        tr("Pause (h)").toStdString(),
        tr("Net (h)").toStdString(),
        tr("Taux").toStdString(),
        tr("Gains").toStdString(),
        tr("Notes").toStdString(),
        tr("Tags").toStdString(),
    });
    const int64_t now = QDateTime::currentMSecsSinceEpoch();
    for (const auto& e : m_db.getTimeEntries(fromMs, toMs > 0 ? toMs : 0)) {
        const double rate = m_projects.hourlyRateFor(QString::fromStdString(e.projectId));
        const auto d = core::computeDuration(e, rate, now);
        const QDateTime start = QDateTime::fromMSecsSinceEpoch(e.startMs);
        const QDateTime end = e.endMs > 0 ? QDateTime::fromMSecsSinceEpoch(e.endMs)
                                          : QDateTime();
        rows.push_back({
            start.toString(QStringLiteral("yyyy-MM-dd")).toStdString(),
            m_projects.nameFor(QString::fromStdString(e.projectId)).toStdString(),
            start.toString(QStringLiteral("HH:mm")).toStdString(),
            end.isValid() ? end.toString(QStringLiteral("HH:mm")).toStdString() : "—",
            QString::number(static_cast<double>(e.breakMs) / 3600000.0, 'f', 2).toStdString(),
            QString::number(d.hours, 'f', 2).toStdString(),
            QString::number(rate, 'f', 2).toStdString(),
            QString::number(d.earnings, 'f', 2).toStdString(),
            e.notes,
            e.tags,
        });
    }
    return rows;
}

QString AppController::exportCsv(qint64 fromMs, qint64 toMs)
{
    return QString::fromStdString(core::csvWrite(buildExportRows(fromMs, toMs)));
}

bool AppController::writeCsvFile(const QString& path, qint64 fromMs, qint64 toMs)
{
    QFile f(path);
    if (!f.open(QIODevice::WriteOnly))
        return false;
    f.write(exportCsv(fromMs, toMs).toUtf8());
    return true;
}

bool AppController::writeXlsxFile(const QString& path, qint64 fromMs, qint64 toMs)
{
    const std::string data = core::xlsxWrite(buildExportRows(fromMs, toMs));
    QFile f(path);
    if (!f.open(QIODevice::WriteOnly))
        return false;
    f.write(QByteArray(data.data(), static_cast<int>(data.size())));
    return true;
}

QString AppController::suggestedExportPath(const QString& ext) const
{
    const QString dir = QStandardPaths::writableLocation(QStandardPaths::CacheLocation);
    const QString name = QStringLiteral("timesheet_")
        + QDateTime::currentDateTime().toString(QStringLiteral("yyyyMMdd_HHmm"))
        + QLatin1Char('.') + ext;
    return dir + QLatin1Char('/') + name;
}

bool AppController::shareCsvWeek()
{
    const auto w = weekReport();
    const QString path = suggestedExportPath(QStringLiteral("csv"));
    if (!writeCsvFile(path, w.value(QStringLiteral("fromMs")).toLongLong(),
                      w.value(QStringLiteral("toMs")).toLongLong()))
        return false;
    return platformShareFile(path, QStringLiteral("text/csv"));
}

bool AppController::shareXlsxWeek()
{
    const auto w = weekReport();
    const QString path = suggestedExportPath(QStringLiteral("xlsx"));
    if (!writeXlsxFile(path, w.value(QStringLiteral("fromMs")).toLongLong(),
                       w.value(QStringLiteral("toMs")).toLongLong()))
        return false;
    return platformShareFile(path,
        QStringLiteral("application/vnd.openxmlformats-officedocument.spreadsheetml.sheet"));
}

bool AppController::shareCsvMonth()
{
    const auto m = monthReport();
    const QString path = suggestedExportPath(QStringLiteral("csv"));
    if (!writeCsvFile(path, m.value(QStringLiteral("fromMs")).toLongLong(),
                      m.value(QStringLiteral("toMs")).toLongLong()))
        return false;
    return platformShareFile(path, QStringLiteral("text/csv"));
}

bool AppController::shareXlsxMonth()
{
    const auto m = monthReport();
    const QString path = suggestedExportPath(QStringLiteral("xlsx"));
    if (!writeXlsxFile(path, m.value(QStringLiteral("fromMs")).toLongLong(),
                       m.value(QStringLiteral("toMs")).toLongLong()))
        return false;
    return platformShareFile(path,
        QStringLiteral("application/vnd.openxmlformats-officedocument.spreadsheetml.sheet"));
}

bool AppController::shareBackup(const QString& passphrase)
{
    const QString path = suggestedBackupPath();
    if (!exportBackup(path, passphrase))
        return false;
    return platformShareFile(path, QStringLiteral("application/octet-stream"));
}

bool AppController::exportBackup(const QString& path, const QString& passphrase)
{
    if (passphrase.trimmed().isEmpty())
        return false;

    const auto blob = core::backupEncrypt(databasePath().toStdString(),
                                          passphrase.toStdString());
    if (!blob)
        return false;

    QFile f(path);
    if (!f.open(QIODevice::WriteOnly))
        return false;
    f.write(reinterpret_cast<const char*>(blob->data()),
            static_cast<qint64>(blob->size()));
    return true;
}

bool AppController::importBackup(const QString& path, const QString& passphrase)
{
    if (passphrase.trimmed().isEmpty())
        return false;

    QFile f(path);
    if (!f.open(QIODevice::ReadOnly))
        return false;
    const QByteArray raw = f.readAll();
    const std::vector<uint8_t> blob(raw.begin(), raw.end());

    const auto zipOpt = core::backupDecryptToZip(blob, passphrase.toStdString());
    if (!zipOpt)
        return false;

    const auto entries = core::zipRead(*zipOpt);
    if (!entries)
        return false;

    std::string dbBytes;
    for (const auto& e : *entries) {
        if (e.name == "openpunchclock.db")
            dbBytes = e.data;
    }
    if (dbBytes.empty())
        return false;

    shutdown();
    const QString dbPath = databasePath();
    QFile::remove(dbPath);
    QFile out(dbPath);
    if (!out.open(QIODevice::WriteOnly))
        return false;
    out.write(dbBytes.data(), static_cast<qint64>(dbBytes.size()));
    out.close();

    if (!init())
        return false;

    emit punchChanged();
    refreshWidget();
    return true;
}

void AppController::processLaunchIntent()
{
    const QString action = platformConsumeLaunchPunchAction();
    if (action == QLatin1String("in")) {
        const QString pid = m_projects.defaultProjectId();
        if (!pid.isEmpty() && punchIn(pid))
            showToast(tr("Entrée (widget)"));
    } else if (action == QLatin1String("out")) {
        if (punchOut())
            showToast(tr("Sortie (widget)"));
    }
}

QString AppController::suggestedBackupPath() const
{
    const QString dir = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    const QString name = QStringLiteral("openpunchclock_backup_")
        + QDateTime::currentDateTime().toString(QStringLiteral("yyyyMMdd_HHmm"))
        + QStringLiteral(".opcbk");
    return dir + QLatin1Char('/') + name;
}

bool AppController::copyToClipboard(const QString& text)
{
    return platformSetClipboard(text);
}

QVariantMap AppController::weekReport()
{
    const QDate today = QDate::currentDate();
    const int dow = today.dayOfWeek();
    const QDate monday = today.addDays(1 - dow);
    const int64_t from = QDateTime(monday, QTime(0, 0)).toMSecsSinceEpoch();
    const int64_t to = QDateTime::currentDateTime().toMSecsSinceEpoch();
    const auto entries = m_db.getTimeEntries(from, to);
    const double hours = core::totalHoursInRange(entries, from, to);
    QVariantMap m;
    m.insert(QStringLiteral("hours"), hours);
    m.insert(QStringLiteral("overtime"), core::overtimeHours(hours, m_overtimeThreshold));
    m.insert(QStringLiteral("fromMs"), static_cast<qint64>(from));
    m.insert(QStringLiteral("toMs"), static_cast<qint64>(to));
    return m;
}

QVariantMap AppController::monthReport()
{
    const QDate today = QDate::currentDate();
    const QDate first(today.year(), today.month(), 1);
    const int64_t from = QDateTime(first, QTime(0, 0)).toMSecsSinceEpoch();
    const int64_t to = QDateTime::currentDateTime().toMSecsSinceEpoch();
    const auto entries = m_db.getTimeEntries(from, to);
    const double hours = core::totalHoursInRange(entries, from, to);
    QVariantMap m;
    m.insert(QStringLiteral("hours"), hours);
    m.insert(QStringLiteral("overtime"), core::overtimeHours(hours, m_overtimeThreshold));
    m.insert(QStringLiteral("fromMs"), static_cast<qint64>(from));
    m.insert(QStringLiteral("toMs"), static_cast<qint64>(to));
    return m;
}

QVariantList AppController::weekReportByProject()
{
    const auto w = weekReport();
    const int64_t from = w.value(QStringLiteral("fromMs")).toLongLong();
    const int64_t to = w.value(QStringLiteral("toMs")).toLongLong();
    const int64_t now = QDateTime::currentMSecsSinceEpoch();

    QHash<QString, double> hoursByProject;
    for (const auto& e : m_db.getTimeEntries(from, to)) {
        const double rate = m_projects.hourlyRateFor(QString::fromStdString(e.projectId));
        const auto d = core::computeDuration(e, rate, now);
        const QString pid = QString::fromStdString(e.projectId);
        hoursByProject[pid] += d.hours;
    }

    QVariantList list;
    for (auto it = hoursByProject.constBegin(); it != hoursByProject.constEnd(); ++it) {
        QVariantMap row;
        row.insert(QStringLiteral("projectId"), it.key());
        row.insert(QStringLiteral("projectName"), m_projects.nameFor(it.key()));
        row.insert(QStringLiteral("hours"), it.value());
        list.append(row);
    }
    return list;
}

QVariantList AppController::auditLog(int limit)
{
    QVariantList list;
    for (const auto& a : m_db.getAudit(limit)) {
        QVariantMap m;
        m.insert(QStringLiteral("entryId"), QString::fromStdString(a.entryId));
        m.insert(QStringLiteral("action"), QString::fromStdString(a.action));
        m.insert(QStringLiteral("detail"), QString::fromStdString(a.detail));
        m.insert(QStringLiteral("atMs"), static_cast<qint64>(a.atMs));
        list.append(m);
    }
    return list;
}

void AppController::enableSync(const QString& title)
{
    if (!m_sync)
        return;
    m_sync->createWorkspace(title);
    emit settingsChanged();
}

QString AppController::syncJoinUri() const
{
    return m_sync ? m_sync->joinUri() : QString{};
}

void AppController::handleJoinUrl(const QUrl& url)
{
    if (m_sync)
        m_sync->handleJoinUrl(url);
}

void AppController::onApplicationStateChanged(Qt::ApplicationState state)
{
    if (state == Qt::ApplicationActive) {
        processLaunchIntent();
        if (m_sync)
            m_sync->catchUpOnForeground();
    }
}

} // namespace app

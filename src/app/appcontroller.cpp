#include "appcontroller.h"
#include "i18n.h"
#include "syncengine.h"

#include "../core/csv.h"
#include "../core/timecalculator.h"
#include "../core/xlsx.h"
#include "../net/relaypool.h"
#include "platform.h"

#include <QDate>
#include <QDir>
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
    m_tickTimer.start();
    if (m_sync)
        m_sync->onLocalChange();
    emit punchChanged();
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
    if (m_sync)
        m_sync->onLocalChange();
    emit punchChanged();
    return true;
}

bool AppController::startBreak()
{
    if (!m_punch.startBreak())
        return false;
    emit punchChanged();
    return true;
}

bool AppController::endBreak()
{
    if (!m_punch.endBreak())
        return false;
    emit punchChanged();
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
}

void AppController::checkReminder()
{
    if (!clockedIn() || onBreak())
        return;
    const qint64 elapsedMin = liveElapsedMs() / 60000;
    if (elapsedMin >= m_reminderMinutes) {
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
    const QString dir = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    const QString name = QStringLiteral("timesheet_")
        + QDateTime::currentDateTime().toString(QStringLiteral("yyyyMMdd_HHmm"))
        + QLatin1Char('.') + ext;
    return dir + QLatin1Char('/') + name;
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
    if (state == Qt::ApplicationActive && m_sync)
        m_sync->catchUpOnForeground();
}

} // namespace app

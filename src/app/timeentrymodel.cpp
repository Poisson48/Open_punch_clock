#include "timeentrymodel.h"
#include "projectmodel.h"

#include "../core/timecalculator.h"

#include <QDateTime>
#include <QUuid>

namespace app {

TimeEntryModel::TimeEntryModel(store::Database* db, ProjectModel* projects,
                               QObject* parent)
    : QAbstractListModel(parent)
    , m_db(db)
    , m_projects(projects)
{
}

void TimeEntryModel::reload(int64_t fromMs, int64_t toMs)
{
    m_fromMs = fromMs;
    m_toMs = toMs;
    beginResetModel();
    m_rows.clear();
    if (!m_db)
        return;

    const int64_t now = QDateTime::currentMSecsSinceEpoch();
    const auto entries = m_db->getTimeEntries(fromMs, toMs > 0 ? toMs : 0);
    for (const auto& e : entries) {
        Row r;
        r.entry = e;
        const double rate = m_projects ? m_projects->hourlyRateFor(
            QString::fromStdString(e.projectId)) : 0.0;
        const auto d = core::computeDuration(e, rate, now);
        r.netHours = d.hours;
        r.earnings = d.earnings;
        r.projectName = m_projects ? m_projects->nameFor(QString::fromStdString(e.projectId))
                                   : QString::fromStdString(e.projectId);
        const QDateTime dt = QDateTime::fromMSecsSinceEpoch(e.startMs);
        const QDate today = QDate::currentDate();
        if (dt.date() == today)
            r.day = QStringLiteral("Aujourd'hui");
        else if (dt.date() == today.addDays(-1))
            r.day = QStringLiteral("Hier");
        else
            r.day = dt.toString(QStringLiteral("dddd d MMMM"));
        m_rows.push_back(std::move(r));
    }
    endResetModel();
    emit refreshed();
}

bool TimeEntryModel::saveManualEntry(const QString& entryId, const QString& projectId,
                                     qint64 startMs, qint64 endMs, qint64 breakMs,
                                     const QString& notes, const QString& tags,
                                     double reimburse, double deduct)
{
    core::TimeEntry e;
    const int64_t now = QDateTime::currentMSecsSinceEpoch();
    e.entryId = entryId.isEmpty()
        ? QUuid::createUuid().toString(QUuid::WithoutBraces).toStdString()
        : entryId.toStdString();
    e.projectId = projectId.toStdString();
    e.startMs = startMs;
    e.endMs = endMs;
    e.breakMs = breakMs;
    e.notes = notes.toStdString();
    e.tags = tags.toStdString();
    e.reimburse = reimburse;
    e.deduct = deduct;
    e.source = "manual";
    e.created = now;
    e.touched = now;
    if (!m_db->upsertTimeEntry(e))
        return false;
    m_db->appendAudit({e.entryId, "manual_save", e.notes, now});
    reload(m_fromMs, m_toMs);
    return true;
}

bool TimeEntryModel::deleteEntry(const QString& entryId)
{
    if (!m_db->deleteTimeEntry(entryId.toStdString()))
        return false;
    reload(m_fromMs, m_toMs);
    return true;
}

int TimeEntryModel::rowCount(const QModelIndex& parent) const
{
    return parent.isValid() ? 0 : static_cast<int>(m_rows.size());
}

QVariant TimeEntryModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || index.row() >= rowCount())
        return {};
    const Row& r = m_rows.at(index.row());
    const auto& e = r.entry;
    switch (role) {
    case EntryIdRole: return QString::fromStdString(e.entryId);
    case ProjectIdRole: return QString::fromStdString(e.projectId);
    case ProjectNameRole: return r.projectName;
    case StartMsRole: return static_cast<qint64>(e.startMs);
    case EndMsRole: return static_cast<qint64>(e.endMs);
    case BreakMsRole: return static_cast<qint64>(e.breakMs);
    case NotesRole: return QString::fromStdString(e.notes);
    case TagsRole: return QString::fromStdString(e.tags);
    case ReimburseRole: return e.reimburse;
    case DeductRole: return e.deduct;
    case SourceRole: return QString::fromStdString(e.source);
    case NetHoursRole: return r.netHours;
    case EarningsRole: return r.earnings;
    case DayRole: return r.day;
    default: return {};
    }
}

QHash<int, QByteArray> TimeEntryModel::roleNames() const
{
    return {
        {EntryIdRole, "entryId"},
        {ProjectIdRole, "projectId"},
        {ProjectNameRole, "projectName"},
        {StartMsRole, "startMs"},
        {EndMsRole, "endMs"},
        {BreakMsRole, "breakMs"},
        {NotesRole, "notes"},
        {TagsRole, "tags"},
        {ReimburseRole, "reimburse"},
        {DeductRole, "deduct"},
        {SourceRole, "source"},
        {NetHoursRole, "netHours"},
        {EarningsRole, "earnings"},
        {DayRole, "day"},
    };
}

} // namespace app

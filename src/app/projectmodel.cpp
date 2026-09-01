#include "projectmodel.h"

#include <QDateTime>
#include <QUuid>

namespace app {

ProjectModel::ProjectModel(store::Database* db, QObject* parent)
    : QAbstractListModel(parent)
    , m_db(db)
{
}

void ProjectModel::reload()
{
    if (auto id = m_db->getSetting("deviceId"))
        m_deviceId = *id;
    beginResetModel();
    m_items = m_db ? m_db->getProjects() : std::vector<core::Project>{};
    if (m_items.empty() && m_db) {
        core::Project p;
        p.projectId = QUuid::createUuid().toString(QUuid::WithoutBraces).toStdString();
        p.name = "Default";
        p.hourlyRate = 15.0;
        p.color = "#2E7D32";
        p.isDefault = true;
        p.created = QDateTime::currentMSecsSinceEpoch();
        m_db->upsertProject(p);
        m_items = m_db->getProjects();
    }
    endResetModel();
    emit refreshed();
}

QString ProjectModel::createProject(const QString& name, double hourlyRate,
                                    const QString& color)
{
    core::Project p;
    p.projectId = QUuid::createUuid().toString(QUuid::WithoutBraces).toStdString();
    p.name = name.toStdString();
    p.hourlyRate = hourlyRate;
    p.color = color.toStdString();
    p.created = QDateTime::currentMSecsSinceEpoch();
    p.touched = p.created;
    p.nameVer = {1, m_deviceId};
    p.rateVer = {1, m_deviceId};
    if (!m_db->upsertProject(p))
        return {};
    reload();
    return QString::fromStdString(p.projectId);
}

bool ProjectModel::updateProject(const QString& projectId, const QString& name,
                                 double hourlyRate, const QString& color)
{
    auto p = m_db->getProject(projectId.toStdString());
    if (!p)
        return false;
    p->name = name.toStdString();
    p->hourlyRate = hourlyRate;
    p->color = color.toStdString();
    p->touched = QDateTime::currentMSecsSinceEpoch();
    const bool ok = m_db->upsertProject(*p);
    if (ok)
        reload();
    return ok;
}

bool ProjectModel::deleteProject(const QString& projectId)
{
    const bool ok = m_db->deleteProject(projectId.toStdString());
    if (ok)
        reload();
    return ok;
}

bool ProjectModel::setDefault(const QString& projectId)
{
    const bool ok = m_db->setDefaultProject(projectId.toStdString());
    if (ok)
        reload();
    return ok;
}

QString ProjectModel::defaultProjectId() const
{
    for (const auto& p : m_items) {
        if (p.isDefault)
            return QString::fromStdString(p.projectId);
    }
    return m_items.empty() ? QString{} : QString::fromStdString(m_items.front().projectId);
}

double ProjectModel::hourlyRateFor(const QString& projectId) const
{
    for (const auto& p : m_items) {
        if (p.projectId == projectId.toStdString())
            return p.hourlyRate;
    }
    return 0.0;
}

QString ProjectModel::nameFor(const QString& projectId) const
{
    for (const auto& p : m_items) {
        if (p.projectId == projectId.toStdString())
            return QString::fromStdString(p.name);
    }
    return {};
}

QString ProjectModel::projectIdAt(int index) const
{
    if (index < 0 || index >= static_cast<int>(m_items.size()))
        return {};
    return QString::fromStdString(m_items.at(index).projectId);
}

int ProjectModel::rowCount(const QModelIndex& parent) const
{
    return parent.isValid() ? 0 : static_cast<int>(m_items.size());
}

QVariant ProjectModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || index.row() >= rowCount())
        return {};
    const auto& p = m_items.at(index.row());
    switch (role) {
    case ProjectIdRole: return QString::fromStdString(p.projectId);
    case NameRole: return QString::fromStdString(p.name);
    case HourlyRateRole: return p.hourlyRate;
    case ColorRole: return QString::fromStdString(p.color);
    case IsDefaultRole: return p.isDefault;
    default: return {};
    }
}

QHash<int, QByteArray> ProjectModel::roleNames() const
{
    return {
        {ProjectIdRole, "projectId"},
        {NameRole, "name"},
        {HourlyRateRole, "hourlyRate"},
        {ColorRole, "color"},
        {IsDefaultRole, "isDefault"},
    };
}

} // namespace app

#pragma once

#include "../core/types.h"
#include "../store/database.h"

#include <QAbstractListModel>
#include <QString>

namespace app {

class ProjectModel : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(int count READ rowCount NOTIFY refreshed)

public:
    enum Roles {
        ProjectIdRole = Qt::UserRole + 1,
        NameRole,
        HourlyRateRole,
        ColorRole,
        IsDefaultRole,
    };
    Q_ENUM(Roles)

    explicit ProjectModel(store::Database* db, QObject* parent = nullptr);

    Q_INVOKABLE void reload();
    Q_INVOKABLE QString createProject(const QString& name, double hourlyRate,
                                      const QString& color);
    Q_INVOKABLE bool updateProject(const QString& projectId, const QString& name,
                                   double hourlyRate, const QString& color);
    Q_INVOKABLE bool deleteProject(const QString& projectId);
    Q_INVOKABLE bool setDefault(const QString& projectId);
    Q_INVOKABLE QString defaultProjectId() const;
    Q_INVOKABLE double hourlyRateFor(const QString& projectId) const;
    Q_INVOKABLE QString nameFor(const QString& projectId) const;
    Q_INVOKABLE QString projectIdAt(int index) const;

    int rowCount(const QModelIndex& parent = {}) const override;
    QVariant data(const QModelIndex& index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

signals:
    void refreshed();

private:
    store::Database*           m_db = nullptr;
    std::vector<core::Project> m_items;
    std::string                m_deviceId;
};

} // namespace app

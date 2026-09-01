#pragma once

#include "../core/types.h"
#include "../store/database.h"

#include <QAbstractListModel>
#include <QString>

namespace app {

class ProjectModel;

class TimeEntryModel : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(int count READ rowCount NOTIFY refreshed)

public:
    enum Roles {
        EntryIdRole = Qt::UserRole + 1,
        ProjectIdRole,
        ProjectNameRole,
        StartMsRole,
        EndMsRole,
        BreakMsRole,
        NotesRole,
        TagsRole,
        ReimburseRole,
        DeductRole,
        SourceRole,
        NetHoursRole,
        EarningsRole,
        DayRole,
    };
    Q_ENUM(Roles)

    explicit TimeEntryModel(store::Database* db, ProjectModel* projects,
                            QObject* parent = nullptr);

    Q_INVOKABLE void reload(int64_t fromMs = 0, int64_t toMs = 0);
    Q_INVOKABLE bool saveManualEntry(const QString& entryId, const QString& projectId,
                                     qint64 startMs, qint64 endMs, qint64 breakMs,
                                     const QString& notes, const QString& tags,
                                     double reimburse, double deduct);
    Q_INVOKABLE bool deleteEntry(const QString& entryId);

    int rowCount(const QModelIndex& parent = {}) const override;
    QVariant data(const QModelIndex& index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

signals:
    void refreshed();

private:
    struct Row {
        core::TimeEntry entry;
        double netHours = 0;
        double earnings = 0;
        QString projectName;
        QString day;
    };

    store::Database*    m_db = nullptr;
    ProjectModel*       m_projects = nullptr;
    std::vector<Row>    m_rows;
    int64_t             m_fromMs = 0;
    int64_t             m_toMs = 0;
};

} // namespace app

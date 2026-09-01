#include "../src/store/database.h"

#include <QtTest>
#include <QTemporaryDir>
#include <QUuid>

class DatabaseTest : public QObject
{
    Q_OBJECT
private slots:
    void projectAndEntry()
    {
        QTemporaryDir tmp;
        store::Database db;
        QVERIFY(db.open(tmp.path() + "/t.db"));

        core::Project p;
        p.projectId = "p1";
        p.name = "Client A";
        p.hourlyRate = 20.0;
        p.color = "#2E7D32";
        p.isDefault = true;
        p.created = 1000;
        QVERIFY(db.upsertProject(p));

        core::TimeEntry e;
        e.entryId = "e1";
        e.projectId = "p1";
        e.startMs = 1000;
        e.endMs = 5000;
        e.breakMs = 500;
        e.source = "manual";
        e.created = 1000;
        e.touched = 1000;
        QVERIFY(db.upsertTimeEntry(e));

        QCOMPARE(db.getProjects().size(), 1u);
        QCOMPARE(db.getTimeEntries().size(), 1u);

        core::PunchState s;
        s.clockedIn = true;
        s.projectId = "p1";
        s.clockInMs = 9000;
        s.activeEntryId = "e2";
        QVERIFY(db.savePunchState(s));
        QCOMPARE(db.getPunchState().clockedIn, true);
    }
};
QTEST_MAIN(DatabaseTest)
#include "tst_database.moc"

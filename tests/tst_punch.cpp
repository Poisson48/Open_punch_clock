#include "../src/app/punchengine.h"
#include "../src/store/database.h"

#include <QtTest>
#include <QTemporaryDir>
#include <QDateTime>

class PunchTest : public QObject
{
    Q_OBJECT
private slots:
    void clockInOutSurvivesReload()
    {
        QTemporaryDir tmp;
        store::Database db;
        QVERIFY(db.open(tmp.path() + "/t.db"));
        db.setSetting("deviceId", "dev1");

        core::Project p;
        p.projectId = "p1";
        p.name = "Job";
        p.hourlyRate = 10;
        p.created = QDateTime::currentMSecsSinceEpoch();
        db.upsertProject(p);

        app::PunchEngine engine(&db);
        QVERIFY(engine.clockIn("p1"));
        QVERIFY(engine.state().clockedIn);

        app::PunchEngine engine2(&db);
        engine2.reload();
        QVERIFY(engine2.state().clockedIn);

        QVERIFY(engine2.clockOut());
        QVERIFY(!engine2.state().clockedIn);
        QCOMPARE(db.getTimeEntries().size(), 1u);
    }
};
QTEST_MAIN(PunchTest)
#include "tst_punch.moc"

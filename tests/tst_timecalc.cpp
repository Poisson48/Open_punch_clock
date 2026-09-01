#include "../src/core/timecalculator.h"

#include <QtTest>

class TimeCalcTest : public QObject
{
    Q_OBJECT
private slots:
    void netDuration()
    {
        core::TimeEntry e;
        e.startMs = 1000;
        e.endMs = 3600000 + 1000;
        e.breakMs = 900000;
        const auto r = core::computeDuration(e, 20.0, e.endMs);
        QCOMPARE(r.netMs, static_cast<int64_t>(2700000));
        QCOMPARE(r.hours, 0.75);
        QCOMPARE(r.earnings, 15.0);
    }

    void overtime()
    {
        QCOMPARE(core::overtimeHours(40.0, 35.0), 5.0);
        QCOMPARE(core::overtimeHours(30.0, 35.0), 0.0);
    }
};
QTEST_MAIN(TimeCalcTest)
#include "tst_timecalc.moc"

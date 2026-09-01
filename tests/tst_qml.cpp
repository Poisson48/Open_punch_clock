#include "../src/app/appcontroller.h"

#include <QtTest>

class QmlTest : public QObject
{
    Q_OBJECT
private slots:
    void controllerInit()
    {
        app::AppController controller;
        QVERIFY(controller.init());
        QVERIFY(controller.projects()->rowCount() >= 1);
        QVERIFY(!controller.projects()->defaultProjectId().isEmpty());
    }
};
QTEST_MAIN(QmlTest)
#include "tst_qml.moc"

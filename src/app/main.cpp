#include <QCoreApplication>
#include <QDesktopServices>
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickStyle>
#include <QTranslator>

#include "appcontroller.h"
#include "i18n.h"
#include "permissions.h"
#include "platform.h"
#include "theme.h"
#include "updater.h"

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);
    app.setOrganizationName("OpenPunchClock");
    app.setApplicationName("OpenPunchClock");

    QQuickStyle::setStyle(QStringLiteral("Material"));
    app::initNotifications();

    app::AppController controller;
    if (!controller.init()) {
        qCritical("AppController::init() failed");
        return 1;
    }

    QTranslator translator;
    app::I18n::installTranslator(&app, &translator, controller.locale());

    QDesktopServices::setUrlHandler(QStringLiteral("openpunchclock"),
                                    &controller, "handleJoinUrl");
    QDesktopServices::setUrlHandler(QStringLiteral("colocourse"),
                                    &controller, "handleJoinUrl");

    QObject::connect(&app, &QGuiApplication::applicationStateChanged,
                     &controller, &app::AppController::onApplicationStateChanged);
    QObject::connect(&app, &QCoreApplication::aboutToQuit,
                     &controller, &app::AppController::shutdown);

    app::Permissions permissions;
    qmlRegisterSingletonInstance("OpenPunchClock", 1, 0, "Permissions", &permissions);

    app::Theme theme;

    app::Updater updater;
    updater.check();
    QObject::connect(&app, &QGuiApplication::applicationStateChanged,
                     &updater, [&updater](Qt::ApplicationState state) {
        if (state == Qt::ApplicationActive)
            updater.check();
    });

    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty(QStringLiteral("AppController"), &controller);
    engine.rootContext()->setContextProperty(QStringLiteral("Theme"), &theme);
    engine.rootContext()->setContextProperty(QStringLiteral("Updater"), &updater);

    QObject::connect(&controller, &app::AppController::retranslateRequested, &engine,
                     [&app, &translator, &controller, &engine]() {
        app::I18n::installTranslator(&app, &translator, controller.locale());
        engine.retranslate();
    });

    const QUrl url(QStringLiteral("qrc:/OpenPunchClock/qml/Main.qml"));
    QObject::connect(&engine, &QQmlApplicationEngine::objectCreated,
                     &app, [url](QObject *obj, const QUrl &objUrl) {
        if (!obj && url == objUrl)
            QCoreApplication::exit(-1);
    }, Qt::QueuedConnection);

    engine.load(url);
    return app.exec();
}

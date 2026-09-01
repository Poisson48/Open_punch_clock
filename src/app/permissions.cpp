#include "permissions.h"

#include <QCoreApplication>

#if QT_VERSION >= QT_VERSION_CHECK(6, 5, 0)
#  include <QPermissions>
#endif

namespace app {

Permissions::Permissions(QObject* parent)
    : QObject(parent)
{
#if QT_VERSION < QT_VERSION_CHECK(6, 5, 0)
    m_cameraGranted = true;
    m_locationGranted = true;
#endif
}

void Permissions::setLocationGranted(bool granted)
{
    if (m_locationGranted == granted)
        return;
    m_locationGranted = granted;
    emit locationGrantedChanged();
}

void Permissions::setCameraGranted(bool granted)
{
    if (m_cameraGranted == granted)
        return;
    m_cameraGranted = granted;
    emit cameraGrantedChanged();
}

void Permissions::requestCamera()
{
#if QT_VERSION >= QT_VERSION_CHECK(6, 5, 0)
    const QCameraPermission permission;

    switch (qApp->checkPermission(permission)) {
    case Qt::PermissionStatus::Granted:
        setCameraGranted(true);
        return;
    case Qt::PermissionStatus::Denied:
        // Refus définitif : c'est à l'utilisateur de rouvrir les réglages système.
        setCameraGranted(false);
        return;
    case Qt::PermissionStatus::Undetermined:
        qApp->requestPermission(permission, this, [this](const QPermission& result) {
            setCameraGranted(result.status() == Qt::PermissionStatus::Granted);
        });
        return;
    }
#else
    setCameraGranted(true);
#endif
}

void Permissions::requestLocation()
{
#if QT_VERSION >= QT_VERSION_CHECK(6, 5, 0)
    const QLocationPermission permission;
    permission.setAccuracy(QLocationPermission::Precise);

    switch (qApp->checkPermission(permission)) {
    case Qt::PermissionStatus::Granted:
        setLocationGranted(true);
        return;
    case Qt::PermissionStatus::Denied:
        setLocationGranted(false);
        return;
    case Qt::PermissionStatus::Undetermined:
        qApp->requestPermission(permission, this, [this](const QPermission& result) {
            setLocationGranted(result.status() == Qt::PermissionStatus::Granted);
        });
        return;
    }
#else
    setLocationGranted(true);
#endif
}

} // namespace app

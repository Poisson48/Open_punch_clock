#pragma once

#include <QObject>

namespace app {

// Permissions runtime, exposé à QML comme singleton `Permissions`.
//
// L'API QPermission n'existe qu'à partir de Qt 6.5 (Android est en 6.8, le desktop
// Linux packagé est en 6.4) : sous 6.5, et sur les plateformes sans permissions
// runtime, on considère l'accès accordé — c'est le cas de Linux.
class Permissions : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool cameraGranted READ cameraGranted NOTIFY cameraGrantedChanged)
    Q_PROPERTY(bool locationGranted READ locationGranted NOTIFY locationGrantedChanged)

public:
    explicit Permissions(QObject* parent = nullptr);

    bool cameraGranted() const { return m_cameraGranted; }
    bool locationGranted() const { return m_locationGranted; }

    Q_INVOKABLE void requestCamera();
    Q_INVOKABLE void requestLocation();

signals:
    void cameraGrantedChanged();
    void locationGrantedChanged();

private:
    void setCameraGranted(bool granted);
    void setLocationGranted(bool granted);

    bool m_cameraGranted = false;
    bool m_locationGranted = false;
};

} // namespace app

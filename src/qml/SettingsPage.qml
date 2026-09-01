import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import OpenPunchClock

Item {
    id: root
    readonly property string pageTitle: qsTr("Réglages")

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: Theme.pad
        spacing: Theme.gap

        Label { text: qsTr("Langue"); color: Theme.textDim }
        ComboBox {
            id: localeBox
            Layout.fillWidth: true
            implicitHeight: Theme.touchTarget
            textRole: "name"
            model: AppController.availableLocales
            Component.onCompleted: syncLocale()
            function syncLocale() {
                for (let i = 0; i < count; ++i) {
                    if (model[i].code === AppController.locale) {
                        currentIndex = i
                        return
                    }
                }
            }
            onActivated: {
                if (currentIndex >= 0 && model[currentIndex].code !== AppController.locale)
                    AppController.setLocale(model[currentIndex].code)
            }
        }

        Connections {
            target: AppController
            function onLocaleChanged() { localeBox.syncLocale() }
        }

        Label { text: qsTr("Rappel clock-out (minutes)"); color: Theme.textDim }
        SpinBox {
            from: 60; to: 960; stepSize: 30
            value: AppController.reminderMinutes
            onValueModified: AppController.setReminderMinutes(value)
        }

        Label { text: qsTr("Période de paie (jours)"); color: Theme.textDim }
        SpinBox {
            from: 7; to: 31
            value: AppController.payPeriodDays
            onValueModified: AppController.setPayPeriodDays(value)
        }

        Label { text: qsTr("Seuil heures sup (h/semaine)"); color: Theme.textDim }
        SpinBox {
            from: 0; to: 60
            value: AppController.overtimeThreshold
            onValueModified: AppController.setOvertimeThreshold(value)
        }

        Switch {
            text: qsTr("GPS au punch (opt-in)")
            checked: AppController.gpsEnabled
            onToggled: {
                if (checked)
                    Permissions.requestLocation()
                AppController.setGpsEnabled(checked)
            }
        }

        GroupBox {
            Layout.fillWidth: true
            title: qsTr("Sauvegarde chiffrée")
            ColumnLayout {
                width: parent.width
                ColoTextField {
                    id: backupPass
                    Layout.fillWidth: true
                    placeholderText: qsTr("Phrase secrète")
                    echoMode: TextInput.Password
                }
                Button {
                    Layout.fillWidth: true
                    text: qsTr("Exporter la sauvegarde")
                    onClicked: {
                        const path = AppController.suggestedBackupPath()
                        if (AppController.exportBackup(path, backupPass.text))
                            console.log("Backup:", path)
                    }
                }
            }
        }

        GroupBox {
            Layout.fillWidth: true
            title: qsTr("Sync multi-appareils")
            ColumnLayout {
                width: parent.width
                Label {
                    text: AppController.syncEnabled ? qsTr("Sync activée") : qsTr("Sync désactivée")
                    color: Theme.textDim
                }
                Button {
                    text: qsTr("Activer la sync")
                    visible: !AppController.syncEnabled
                    onClicked: AppController.enableSync(qsTr("Mon punch clock"))
                }
                Button {
                    text: qsTr("Copier lien d'invitation")
                    visible: AppController.syncEnabled
                    onClicked: {
                        const uri = AppController.syncJoinUri()
                        if (uri.length > 0)
                            AppController.copyToClipboard(uri)
                    }
                }
            }
        }

        Label {
            Layout.fillWidth: true
            text: qsTr("Open Punch Clock v%1 — GPLv3").arg(AppController.appVersion)
            color: Theme.textDim
            font.pixelSize: 12
        }

        Item { Layout.fillHeight: true }
    }
}

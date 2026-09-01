import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import OpenPunchClock

Item {
    id: root
    readonly property string pageTitle: "Réglages"

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: Theme.pad
        spacing: Theme.gap

        Label { text: "Rappel clock-out (minutes)"; color: Theme.textDim }
        SpinBox {
            from: 60; to: 960; stepSize: 30
            value: AppController.reminderMinutes
            onValueModified: AppController.setReminderMinutes(value)
        }

        Label { text: "Période de paie (jours)"; color: Theme.textDim }
        SpinBox {
            from: 7; to: 31
            value: AppController.payPeriodDays
            onValueModified: AppController.setPayPeriodDays(value)
        }

        Label { text: "Seuil heures sup (h/semaine)"; color: Theme.textDim }
        SpinBox {
            from: 0; to: 60
            value: AppController.overtimeThreshold
            onValueModified: AppController.setOvertimeThreshold(value)
        }

        Switch {
            text: "GPS au punch (opt-in)"
            checked: AppController.gpsEnabled
            onToggled: AppController.setGpsEnabled(checked)
        }

        GroupBox {
            Layout.fillWidth: true
            title: "Sync multi-appareils"
            ColumnLayout {
                width: parent.width
                Label {
                    text: AppController.syncEnabled ? "Sync activée" : "Sync désactivée"
                    color: Theme.textDim
                }
                Button {
                    text: "Activer la sync"
                    visible: !AppController.syncEnabled
                    onClicked: AppController.enableSync("Mon punch clock")
                }
                Button {
                    text: "Copier lien d'invitation"
                    visible: AppController.syncEnabled
                    onClicked: {
                        const uri = AppController.syncJoinUri()
                        if (uri.length > 0)
                            console.log("Join URI:", uri)
                    }
                }
            }
        }

        Label {
            Layout.fillWidth: true
            text: "Open Punch Clock v0.1 — GPLv3"
            color: Theme.textDim
            font.pixelSize: 12
        }

        Item { Layout.fillHeight: true }
    }
}

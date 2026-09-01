import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts
import OpenPunchClock

Item {
    id: root
    readonly property string pageTitle: qsTr("Projets")

    property Component actions: Row {
        ToolButton {
            width: Theme.touchTarget
            height: Theme.touchTarget
            text: "+"
            font.pixelSize: 22
            onClicked: createDialog.open()
        }
    }

    ListView {
        anchors.fill: parent
        anchors.margins: Theme.pad
        spacing: Theme.gap
        clip: true
        model: AppController.projects

        delegate: Rectangle {
            width: ListView.view.width
            height: 64
            radius: Theme.radius
            color: Theme.surface
            border.color: Theme.outline

            RowLayout {
                anchors.fill: parent
                anchors.margins: Theme.gap
                Rectangle {
                    Layout.preferredWidth: 12
                    Layout.preferredHeight: 12
                    radius: 6
                    color: model.color || Theme.accent
                }
                Label {
                    Layout.fillWidth: true
                    text: model.name + (model.isDefault ? " ★" : "")
                    color: Theme.text
                }
                Label {
                    text: model.hourlyRate.toFixed(2) + " €/h"
                    color: Theme.textDim
                }
            }

            MouseArea {
                anchors.fill: parent
                onClicked: editDialog.openFor(model.projectId, model.name,
                                              model.hourlyRate, model.color, model.isDefault)
            }
        }
    }

    ColoDialog {
        id: createDialog
        title: qsTr("Nouveau projet")
        ColumnLayout {
            width: parent.width
            ColoTextField { id: newName; hint: qsTr("Nom du client"); Layout.fillWidth: true }
            ColoTextField { id: newRate; hint: qsTr("Taux horaire"); text: "15"; Layout.fillWidth: true }
        }
        onAccepted: {
            const id = AppController.projects.createProject(newName.text, parseFloat(newRate.text) || 0, "#43A047")
            if (id.length > 0)
                AppController.showToast(qsTr("Projet créé"))
        }
    }

    ColoDialog {
        id: editDialog
        title: qsTr("Modifier le projet")
        property string pid: ""
        property bool isDefault: false
        ColumnLayout {
            width: parent.width
            ColoTextField { id: editName; Layout.fillWidth: true }
            ColoTextField { id: editRate; Layout.fillWidth: true }
            Button {
                Layout.fillWidth: true
                text: qsTr("Définir par défaut")
                visible: !editDialog.isDefault
                onClicked: {
                    AppController.projects.setDefault(editDialog.pid)
                    AppController.showToast(qsTr("Projet par défaut mis à jour"))
                }
            }
            Button {
                Layout.fillWidth: true
                text: qsTr("Supprimer le projet")
                Material.foreground: Theme.danger
                onClicked: deleteConfirm.open()
            }
        }
        function openFor(id, name, rate, color, def) {
            pid = id; editName.text = name; editRate.text = String(rate)
            isDefault = def
            open()
        }
        onAccepted: {
            if (AppController.projects.updateProject(pid, editName.text, parseFloat(editRate.text) || 0, "#43A047"))
                AppController.showToast(qsTr("Projet mis à jour"))
        }
    }

    ColoDialog {
        id: deleteConfirm
        title: qsTr("Supprimer ce projet ?")
        destructive: true
        acceptText: qsTr("Supprimer")
        showCancel: true
        onAccepted: {
            if (AppController.projects.deleteProject(editDialog.pid)) {
                AppController.showToast(qsTr("Projet supprimé"))
                editDialog.close()
            }
        }
    }
}

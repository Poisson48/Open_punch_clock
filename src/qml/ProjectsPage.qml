import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import OpenPunchClock

Item {
    id: root
    readonly property string pageTitle: "Projets"

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
            height: 56
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
                                              model.hourlyRate, model.color)
            }
        }
    }

    ColoDialog {
        id: createDialog
        title: "Nouveau projet"
        ColumnLayout {
            width: parent.width
            ColoTextField { id: newName; placeholderText: "Nom du client"; Layout.fillWidth: true }
            ColoTextField { id: newRate; placeholderText: "Taux horaire"; text: "15"; Layout.fillWidth: true }
        }
        onAccepted: {
            AppController.projects.createProject(newName.text, parseFloat(newRate.text) || 0, "#2E7D32")
        }
    }

    ColoDialog {
        id: editDialog
        title: "Modifier le projet"
        property string pid: ""
        ColumnLayout {
            width: parent.width
            ColoTextField { id: editName; Layout.fillWidth: true }
            ColoTextField { id: editRate; Layout.fillWidth: true }
        }
        function openFor(id, name, rate, color) {
            pid = id; editName.text = name; editRate.text = String(rate)
            open()
        }
        onAccepted: {
            AppController.projects.updateProject(pid, editName.text, parseFloat(editRate.text) || 0, "#2E7D32")
        }
    }
}

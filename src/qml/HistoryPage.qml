import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts
import OpenPunchClock

Item {
    id: root
    readonly property string pageTitle: qsTr("Historique")

    property Component actions: Row {
        ToolButton {
            width: Theme.touchTarget
            height: Theme.touchTarget
            text: "+"
            onClicked: editor.openNew()
        }
    }

    Component.onCompleted: AppController.entries.reload()

    Connections {
        target: AppController
        function onPunchChanged() { AppController.entries.reload() }
    }

    ListView {
        anchors.fill: parent
        anchors.margins: Theme.pad
        spacing: 6
        clip: true
        model: AppController.entries

        section.property: "day"
        section.delegate: Label {
            width: ListView.view.width
            text: section
            font.bold: true
            color: Theme.textDim
            topPadding: 8
        }

        delegate: Rectangle {
            width: ListView.view.width
            height: 72
            radius: Theme.radius
            color: Theme.surface
            border.color: Theme.outline

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: Theme.gap
                Label {
                    text: model.projectName + " · " + TimeUtils.formatHours(model.netHours)
                    font.bold: true
                    color: Theme.text
                }
                Label {
                    text: TimeUtils.formatStamp(model.startMs)
                        + (model.endMs > 0 ? " → " + TimeUtils.formatStamp(model.endMs)
                                             : (" " + qsTr("(en cours)")))
                    color: Theme.textDim
                    font.pixelSize: 13
                }
                Label {
                    visible: model.notes && model.notes.length > 0
                    text: model.notes
                    color: Theme.textDim
                    font.pixelSize: 12
                    elide: Text.ElideRight
                    Layout.fillWidth: true
                }
            }

            MouseArea {
                anchors.fill: parent
                onClicked: editor.openEdit(model.entryId, model.projectId, model.startMs,
                                           model.endMs, model.breakMs, model.notes, model.tags,
                                           model.reimburse, model.deduct)
            }
        }
    }

    TimeCardEditor {
        id: editor
        onSaved: AppController.entries.reload()
    }
}

import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts
import OpenPunchClock

Item {
    id: root
    readonly property string pageTitle: qsTr("Historique")

    Component.onCompleted: AppController.entries.reload()

    Connections {
        target: AppController
        function onPunchChanged() { AppController.entries.reload() }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: Theme.pad
        spacing: Theme.gap

        Button {
            Layout.fillWidth: true
            Layout.preferredHeight: Theme.touchTarget
            text: qsTr("+ Ajouter une fiche")
            Material.background: Theme.accent
            Material.foreground: Theme.onAccent
            onClicked: editor.openNew()
        }

        ListView {
            Layout.fillWidth: true
            Layout.fillHeight: true
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

            delegate: ItemDelegate {
                width: ListView.view.width
                height: 72
                background: Rectangle {
                    radius: Theme.radius
                    color: Theme.surface
                    border.color: Theme.outline
                }
                contentItem: ColumnLayout {
                    spacing: 2
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
                }
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

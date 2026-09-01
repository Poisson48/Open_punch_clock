import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts
import OpenPunchClock

Item {
    id: root
    readonly property string pageTitle: qsTr("Rapports")

    property var week: AppController.weekReport()
    property var month: AppController.monthReport()
    property var byProject: AppController.weekReportByProject()

    Component.onCompleted: refresh()
    function refresh() {
        week = AppController.weekReport()
        month = AppController.monthReport()
        byProject = AppController.weekReportByProject()
    }

    property Component actions: Row {
        ToolButton {
            width: Theme.touchTarget
            height: Theme.touchTarget
            text: "↓"
            onClicked: exportMenu.popup()
        }
    }

    ColoMenu {
        id: exportMenu
        MenuItem {
            text: qsTr("Partager CSV (semaine)")
            onTriggered: shareOrToast(AppController.shareCsvWeek())
        }
        MenuItem {
            text: qsTr("Partager XLSX (semaine)")
            onTriggered: shareOrToast(AppController.shareXlsxWeek())
        }
        MenuItem {
            text: qsTr("Partager CSV (mois)")
            onTriggered: shareOrToast(AppController.shareCsvMonth())
        }
        MenuItem {
            text: qsTr("Partager XLSX (mois)")
            onTriggered: shareOrToast(AppController.shareXlsxMonth())
        }
    }

    function shareOrToast(ok) {
        AppController.showToast(ok ? qsTr("Partage ouvert") : qsTr("Export impossible"))
    }

    ScrollView {
        anchors.fill: parent
        clip: true

        ColumnLayout {
            width: root.width
            anchors.margins: Theme.pad
            spacing: Theme.gap

            GroupBox {
                Layout.fillWidth: true
                title: qsTr("Cette semaine")
                Label {
                    text: TimeUtils.formatHours(week.hours) + qsTr(" · HS: ") + week.overtime.toFixed(2) + qsTr(" h")
                }
            }

            GroupBox {
                Layout.fillWidth: true
                title: qsTr("Ce mois")
                Label {
                    text: TimeUtils.formatHours(month.hours) + qsTr(" · HS: ") + month.overtime.toFixed(2) + qsTr(" h")
                }
            }

            GroupBox {
                Layout.fillWidth: true
                title: qsTr("Par projet (semaine)")
                visible: byProject.length > 0
                ColumnLayout {
                    width: parent.width
                    Repeater {
                        model: byProject
                        Label {
                            Layout.fillWidth: true
                            text: modelData.projectName + ": " + TimeUtils.formatHours(modelData.hours)
                            color: Theme.text
                        }
                    }
                }
            }

            Button {
                Layout.fillWidth: true
                Layout.preferredHeight: Theme.touchTarget
                text: qsTr("Actualiser")
                onClicked: refresh()
            }

            Item { Layout.preferredHeight: Theme.pad + 40 }
        }
    }
}

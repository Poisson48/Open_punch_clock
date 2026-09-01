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

    function shareOrToast(ok) {
        AppController.showToast(ok ? qsTr("Partage ouvert") : qsTr("Export impossible"))
    }

    ScrollView {
        anchors.fill: parent
        clip: true

        ColumnLayout {
            width: root.width - Theme.pad * 2
            x: Theme.pad
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

            Label { text: qsTr("Exporter et partager"); color: Theme.textDim; Layout.fillWidth: true }

            Button {
                Layout.fillWidth: true
                Layout.preferredHeight: Theme.touchTarget
                text: qsTr("CSV — semaine")
                onClicked: shareOrToast(AppController.shareCsvWeek())
            }
            Button {
                Layout.fillWidth: true
                Layout.preferredHeight: Theme.touchTarget
                text: qsTr("XLSX — semaine")
                onClicked: shareOrToast(AppController.shareXlsxWeek())
            }
            Button {
                Layout.fillWidth: true
                Layout.preferredHeight: Theme.touchTarget
                text: qsTr("CSV — mois")
                onClicked: shareOrToast(AppController.shareCsvMonth())
            }
            Button {
                Layout.fillWidth: true
                Layout.preferredHeight: Theme.touchTarget
                text: qsTr("XLSX — mois")
                onClicked: shareOrToast(AppController.shareXlsxMonth())
            }

            Button {
                Layout.fillWidth: true
                Layout.preferredHeight: Theme.touchTarget
                text: qsTr("Actualiser")
                onClicked: refresh()
            }

            Item { Layout.preferredHeight: Theme.pad }
        }
    }
}

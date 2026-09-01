import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import OpenPunchClock

Item {
    id: root
    readonly property string pageTitle: qsTr("Rapports")

    property var week: AppController.weekReport()
    property var month: AppController.monthReport()

    Component.onCompleted: refresh()
    function refresh() {
        week = AppController.weekReport()
        month = AppController.monthReport()
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
            onTriggered: AppController.shareCsvWeek()
        }
        MenuItem {
            text: qsTr("Partager XLSX (semaine)")
            onTriggered: AppController.shareXlsxWeek()
        }
    }

    ColumnLayout {
        anchors.fill: parent
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

        Button {
            Layout.fillWidth: true
            text: qsTr("Actualiser")
            onClicked: refresh()
        }

        Item { Layout.fillHeight: true }
    }
}

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import OpenPunchClock

Item {
    id: root
    readonly property string pageTitle: "Rapports"

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
            text: "Export CSV (semaine)"
            onTriggered: {
                const path = AppController.suggestedExportPath("csv")
                AppController.writeCsvFile(path, week.fromMs, week.toMs)
            }
        }
        MenuItem {
            text: "Export XLSX (semaine)"
            onTriggered: {
                const path = AppController.suggestedExportPath("xlsx")
                AppController.writeXlsxFile(path, week.fromMs, week.toMs)
            }
        }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: Theme.pad
        spacing: Theme.gap

        GroupBox {
            Layout.fillWidth: true
            title: "Cette semaine"
            Label { text: TimeUtils.formatHours(week.hours) + " · HS: " + week.overtime.toFixed(2) + " h" }
        }

        GroupBox {
            Layout.fillWidth: true
            title: "Ce mois"
            Label { text: TimeUtils.formatHours(month.hours) + " · HS: " + month.overtime.toFixed(2) + " h" }
        }

        Button {
            Layout.fillWidth: true
            text: "Actualiser"
            onClicked: refresh()
        }

        Item { Layout.fillHeight: true }
    }
}

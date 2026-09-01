import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import OpenPunchClock

Item {
    id: root
    readonly property string pageTitle: "Pointeuse"

    property string selectedProjectId: AppController.projects.defaultProjectId()

    function handleBack() { return false }

    Connections {
        target: AppController.projects
        function onRefreshed() {
            if (!selectedProjectId)
                selectedProjectId = AppController.projects.defaultProjectId()
        }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: Theme.pad
        spacing: Theme.gap

        ComboBox {
            id: projectBox
            Layout.fillWidth: true
            implicitHeight: Theme.touchTarget
            model: AppController.projects
            textRole: "name"
            onActivated: root.selectedProjectId = AppController.projects.projectIdAt(currentIndex)
            Component.onCompleted: {
                for (let i = 0; i < AppController.projects.count; ++i) {
                    if (AppController.projects.data(AppController.projects.index(i, 0), 261)) {
                        currentIndex = i
                        break
                    }
                }
                root.selectedProjectId = AppController.projects.projectIdAt(currentIndex)
            }
        }

        Item { Layout.fillHeight: true }

        Label {
            Layout.alignment: Qt.AlignHCenter
            text: TimeUtils.formatDuration(AppController.liveElapsedMs - AppController.liveBreakMs)
            font.pixelSize: 48
            font.bold: true
            color: Theme.text
        }

        Label {
            Layout.alignment: Qt.AlignHCenter
            text: AppController.clockedIn
                  ? ("~ " + AppController.liveEarnings.toFixed(2) + " €")
                  : "Prêt à pointer"
            font.pixelSize: 20
            color: Theme.textDim
        }

        Item { Layout.preferredHeight: Theme.gap }

        // Zone boutons fixe (G10)
        ColumnLayout {
            Layout.fillWidth: true
            spacing: 16

            Button {
                Layout.fillWidth: true
                Layout.preferredHeight: 72
                visible: !AppController.clockedIn
                text: "PUNCH IN"
                Material.background: Theme.accent
                Material.foreground: Theme.onAccent
                onClicked: {
                    AppController.punchIn(root.selectedProjectId)
                }
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: 16
                visible: AppController.clockedIn

                Button {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 64
                    text: AppController.onBreak ? "FIN PAUSE" : "BREAK"
                    enabled: AppController.clockedIn
                    Material.background: Theme.surfaceHigh
                    Material.foreground: Theme.text
                    onClicked: {
                        if (AppController.onBreak)
                            AppController.endBreak()
                        else
                            AppController.startBreak()
                    }
                }

                Button {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 64
                    text: "PUNCH OUT"
                    Material.background: Theme.danger
                    Material.foreground: "white"
                    onClicked: AppController.punchOut()
                }
            }
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: Theme.gap

            Button {
                Layout.fillWidth: true
                text: "Historique"
                flat: true
                onClicked: StackView.view.push(historyPage)
            }
            Button {
                Layout.fillWidth: true
                text: "Projets"
                flat: true
                onClicked: StackView.view.push(projectsPage)
            }
            Button {
                Layout.fillWidth: true
                text: "Rapports"
                flat: true
                onClicked: StackView.view.push(reportsPage)
            }
            Button {
                Layout.preferredWidth: Theme.touchTarget
                text: "⚙"
                flat: true
                onClicked: StackView.view.push(settingsPage)
            }
        }
    }

    Component { id: historyPage; HistoryPage {} }
    Component { id: projectsPage; ProjectsPage {} }
    Component { id: reportsPage; ReportsPage {} }
    Component { id: settingsPage; SettingsPage {} }
}

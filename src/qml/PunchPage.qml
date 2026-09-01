import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts
import OpenPunchClock

Item {
    id: root
    readonly property string pageTitle: qsTr("Pointeuse")

    property string selectedProjectId: AppController.projects.defaultProjectId()

    function handleBack() { return false }

    function syncProjectBox() {
        const pid = root.selectedProjectId || AppController.projects.defaultProjectId()
        for (let i = 0; i < AppController.projects.count; ++i) {
            if (AppController.projects.projectIdAt(i) === pid) {
                projectBox.currentIndex = i
                root.selectedProjectId = pid
                return
            }
        }
        if (AppController.projects.count > 0) {
            projectBox.currentIndex = 0
            root.selectedProjectId = AppController.projects.projectIdAt(0)
        }
    }

    Connections {
        target: AppController.projects
        function onRefreshed() { root.syncProjectBox() }
    }

    Component.onCompleted: syncProjectBox()

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: Theme.pad
        anchors.bottomMargin: Theme.pad + 32
        spacing: Theme.gap

        ComboBox {
            id: projectBox
            Layout.fillWidth: true
            implicitHeight: Theme.touchTarget
            model: AppController.projects
            textRole: "name"
            onActivated: root.selectedProjectId = AppController.projects.projectIdAt(currentIndex)
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
                  : qsTr("Prêt à pointer")
            font.pixelSize: 20
            color: Theme.textDim
        }

        Item { Layout.preferredHeight: Theme.gap }

        ColumnLayout {
            Layout.fillWidth: true
            spacing: 16

            Button {
                Layout.fillWidth: true
                Layout.preferredHeight: 72
                visible: !AppController.clockedIn
                text: qsTr("PUNCH IN")
                Material.background: Theme.accent
                Material.foreground: Theme.onAccent
                onClicked: {
                    if (!root.selectedProjectId) {
                        AppController.showToast(qsTr("Choisissez un projet"))
                        return
                    }
                    if (!AppController.punchIn(root.selectedProjectId))
                        AppController.showToast(qsTr("Impossible de pointer l'entrée"))
                    else
                        AppController.showToast(qsTr("Entrée enregistrée"))
                }
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: 16
                visible: AppController.clockedIn

                Button {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 64
                    text: AppController.onBreak ? qsTr("FIN PAUSE") : qsTr("BREAK")
                    Material.background: Theme.surfaceHigh
                    Material.foreground: Theme.text
                    onClicked: {
                        if (AppController.onBreak) {
                            if (AppController.endBreak())
                                AppController.showToast(qsTr("Pause terminée"))
                        } else if (AppController.startBreak()) {
                            AppController.showToast(qsTr("Pause démarrée"))
                        }
                    }
                }

                Button {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 64
                    text: qsTr("PUNCH OUT")
                    Material.background: Theme.danger
                    Material.foreground: "#FFFFFF"
                    onClicked: {
                        if (!AppController.punchOut())
                            AppController.showToast(qsTr("Impossible de pointer la sortie"))
                        else
                            AppController.showToast(qsTr("Sortie enregistrée"))
                    }
                }
            }
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: Theme.gap

            Button {
                Layout.fillWidth: true
                Layout.preferredHeight: Theme.touchTarget
                text: qsTr("Historique")
                onClicked: Navigation.push(historyPage)
            }
            Button {
                Layout.fillWidth: true
                Layout.preferredHeight: Theme.touchTarget
                text: qsTr("Projets")
                onClicked: Navigation.push(projectsPage)
            }
            Button {
                Layout.fillWidth: true
                Layout.preferredHeight: Theme.touchTarget
                text: qsTr("Rapports")
                onClicked: Navigation.push(reportsPage)
            }
            Button {
                Layout.preferredWidth: Theme.touchTarget
                Layout.preferredHeight: Theme.touchTarget
                text: "⚙"
                onClicked: Navigation.push(settingsPage)
            }
        }
    }

    Component { id: historyPage; HistoryPage {} }
    Component { id: projectsPage; ProjectsPage {} }
    Component { id: reportsPage; ReportsPage {} }
    Component { id: settingsPage; SettingsPage {} }
}

import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts
import OpenPunchClock

ApplicationWindow {
    id: window
    visible: true
    title: qsTr("Open Punch Clock")
    width: 400
    height: 780
    color: Theme.background

    Material.theme: Theme.dark ? Material.Dark : Material.Light
    Material.background: Theme.background
    Material.foreground: Theme.text
    Material.accent: Theme.accent

    readonly property bool offline: !AppController.online
    readonly property bool pending: AppController.pendingChanges > 0

    property int pageIndex: 0

    function handleSystemBack() {
        if (closeTopOverlay())
            return true
        if (pageIndex !== 0) {
            pageIndex = 0
            return true
        }
        return false
    }

    onClosing: function (close) {
        close.accepted = !handleSystemBack()
    }

    Shortcut {
        sequences: ["Back", "Escape"]
        context: Qt.ApplicationShortcut
        onActivated: handleSystemBack()
    }

    function closeTopOverlay() {
        if (!window.Overlay || !window.Overlay.overlay)
            return false
        const overlay = window.Overlay.overlay
        for (let i = overlay.children.length - 1; i >= 0; --i) {
            const child = overlay.children[i]
            if (child && child.opened === true && typeof child.close === "function") {
                child.close()
                return true
            }
        }
        return false
    }

    function pageTitleFor(index) {
        switch (index) {
        case 0: return qsTr("Pointeuse")
        case 1: return qsTr("Historique")
        case 2: return qsTr("Projets")
        case 3: return qsTr("Rapports")
        case 4: return qsTr("Réglages")
        default: return qsTr("Open Punch Clock")
        }
    }

    header: Rectangle {
        color: Theme.surface
        implicitHeight: Math.max(56, headerRow.implicitHeight + 8)

        RowLayout {
            id: headerRow
            anchors.fill: parent
            anchors.leftMargin: 4
            anchors.rightMargin: 4

            ToolButton {
                Layout.preferredWidth: Theme.touchTarget
                Layout.preferredHeight: Theme.touchTarget
                visible: window.pageIndex !== 0
                text: "←"
                font.pixelSize: 20
                onClicked: window.pageIndex = 0
            }

            Label {
                Layout.fillWidth: true
                text: pageLoader.item && pageLoader.item.pageTitle
                      ? pageLoader.item.pageTitle : pageTitleFor(window.pageIndex)
                font.pixelSize: 18
                font.bold: true
                color: Theme.text
                elide: Text.ElideRight
            }

            Loader {
                id: actionsLoader
                Layout.preferredWidth: item ? Math.max(item.implicitWidth, Theme.touchTarget) : 0
                Layout.preferredHeight: Theme.touchTarget
                sourceComponent: pageLoader.item ? pageLoader.item.actions : null
            }
        }

        Rectangle { anchors.bottom: parent.bottom; width: parent.width; height: 1; color: Theme.outline }
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        Loader {
            id: pageLoader
            Layout.fillWidth: true
            Layout.fillHeight: true
            sourceComponent: [
                punchPage, historyPage, projectsPage, reportsPage, settingsPage
            ][window.pageIndex]
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: toastBarInner.visible ? 40 : 0
            color: Theme.surfaceHigh
            visible: toastBarInner.visible

            Label {
                id: toastBarInner
                anchors.centerIn: parent
                visible: AppController.toastMessage.length > 0
                text: AppController.toastMessage
                color: Theme.text
                font.pixelSize: 14
            }

            Timer {
                interval: 2500
                running: toastBarInner.visible
                onTriggered: AppController.clearToast()
            }
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: statusBannerInner.visible ? 28 : 0
            color: offline ? Theme.warning : Theme.accentSoft
            visible: statusBannerInner.visible

            Label {
                id: statusBannerInner
                anchors.centerIn: parent
                visible: offline || pending
                text: offline ? qsTr("Hors ligne") : qsTr("Sync en attente…")
                color: Theme.text
                font.pixelSize: 12
            }
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 56
            color: Theme.surface
            border.color: Theme.outline

            RowLayout {
                anchors.fill: parent
                anchors.margins: 4
                spacing: 2

                Repeater {
                    model: [
                        { icon: "⏱", label: qsTr("Pointeuse") },
                        { icon: "📋", label: qsTr("Hist.") },
                        { icon: "👤", label: qsTr("Projets") },
                        { icon: "📊", label: qsTr("Rapports") },
                        { icon: "⚙", label: qsTr("Régl.") }
                    ]
                    delegate: ToolButton {
                        id: tabBtn
                        Layout.fillWidth: true
                        Layout.preferredHeight: 48
                        checkable: true
                        checked: window.pageIndex === index
                        onClicked: window.pageIndex = index

                        contentItem: ColumnLayout {
                            spacing: 0
                            Label {
                                Layout.alignment: Qt.AlignHCenter
                                text: modelData.icon
                                font.pixelSize: 18
                                color: tabBtn.checked ? Theme.accent : Theme.textDim
                            }
                            Label {
                                Layout.alignment: Qt.AlignHCenter
                                text: modelData.label
                                font.pixelSize: 10
                                color: tabBtn.checked ? Theme.accent : Theme.textDim
                            }
                        }

                        background: Rectangle {
                            color: tabBtn.checked ? Theme.accentSoft : "transparent"
                            radius: 8
                        }
                    }
                }
            }
        }
    }

    Component { id: punchPage; PunchPage {} }
    Component { id: historyPage; HistoryPage {} }
    Component { id: projectsPage; ProjectsPage {} }
    Component { id: reportsPage; ReportsPage {} }
    Component { id: settingsPage; SettingsPage {} }
}

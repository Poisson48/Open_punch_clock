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

    Component.onCompleted: Navigation.stack = stack

    readonly property bool offline: !AppController.online
    readonly property bool pending: AppController.pendingChanges > 0

    function handleSystemBack() {
        if (closeTopOverlay())
            return true
        const page = stack.currentItem
        if (page && typeof page.handleBack === "function" && page.handleBack())
            return true
        if (stack.depth > 1) {
            stack.pop()
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
                visible: stack.depth > 1
                text: "←"
                font.pixelSize: 20
                onClicked: handleSystemBack()
            }

            Label {
                Layout.fillWidth: true
                text: stack.currentItem && stack.currentItem.pageTitle
                      ? stack.currentItem.pageTitle : qsTr("Open Punch Clock")
                font.pixelSize: 18
                font.bold: true
                color: Theme.text
                elide: Text.ElideRight
            }

            Loader {
                id: actionsLoader
                Layout.preferredWidth: item ? item.implicitWidth : 0
                Layout.preferredHeight: Theme.touchTarget
                sourceComponent: stack.currentItem ? stack.currentItem.actions : null
            }
        }

        Rectangle { anchors.bottom: parent.bottom; width: parent.width; height: 1; color: Theme.outline }
    }

    StackView {
        id: stack
        anchors.fill: parent
        anchors.bottomMargin: (statusBanner.visible ? 28 : 0) + (toastBar.visible ? 40 : 0)
        initialItem: punchPage
    }

    Component { id: punchPage; PunchPage {} }

    Rectangle {
        id: toastBar
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: statusBanner.top
        height: visible ? 40 : 0
        color: Theme.surfaceHigh
        z: 11
        visible: AppController.toastMessage.length > 0
        opacity: visible ? 1 : 0
        Behavior on opacity { NumberAnimation { duration: 200 } }

        Label {
            anchors.centerIn: parent
            text: AppController.toastMessage
            color: Theme.text
            font.pixelSize: 14
        }

        Timer {
            id: toastTimer
            interval: 2500
            onTriggered: AppController.clearToast()
        }

        Connections {
            target: AppController
            function onToastChanged() {
                if (AppController.toastMessage.length > 0)
                    toastTimer.restart()
            }
        }
    }

    Rectangle {
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        height: statusBanner.visible ? 28 : 0
        color: offline ? Theme.warning : (pending ? Theme.accentSoft : Theme.surfaceHigh)
        visible: statusBanner.visible
        z: 10

        Label {
            id: statusBanner
            anchors.centerIn: parent
            visible: offline || pending
            text: offline ? qsTr("Hors ligne") : qsTr("Sync en attente…")
            color: Theme.text
            font.pixelSize: 12
        }
    }
}

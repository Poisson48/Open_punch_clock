import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ColoDialog {
    id: dlg

    property string mode: "history"

    title: mode === "pending" ? qsTr("Nouveautés — %1").arg(Updater.latestVersion)
         : mode === "whatsNew" ? qsTr("Quoi de neuf — %1").arg(Updater.currentVersion)
         : qsTr("Notes de version")
    acceptText: mode === "pending" ? qsTr("Télécharger")
              : mode === "whatsNew" ? qsTr("Compris")
              : qsTr("Fermer")
    showCancel: mode === "pending"
    destructive: false

    readonly property string bodyText: {
        if (mode === "pending")
            return Updater.releaseNotes
        if (mode === "whatsNew")
            return Updater.whatsNewNotes
        let blocks = []
        for (let i = 0; i < Updater.changelog.length; ++i) {
            const e = Updater.changelog[i]
            const ver = e.version || ""
            const notes = (e.notes || "").trim()
            if (!ver)
                continue
            blocks.push(notes.length > 0
                        ? qsTr("Version %1\n\n%2").arg(ver).arg(notes)
                        : qsTr("Version %1").arg(ver))
        }
        return blocks.join("\n\n————————————\n\n")
    }

    function openPending()  { mode = "pending";  open() }
    function openWhatsNew() { mode = "whatsNew"; open() }
    function openHistory()  { mode = "history";  open() }

    Label {
        Layout.fillWidth: true
        visible: dlg.bodyText.length === 0
        text: mode === "history"
              ? qsTr("Aucune note de version pour l'instant.")
              : qsTr("Corrections et améliorations.")
        color: Theme.textDim
        font.pixelSize: 14
        wrapMode: Text.WordWrap
    }

    Flickable {
        Layout.fillWidth: true
        Layout.preferredHeight: visible
            ? Math.min(Math.max(notes.implicitHeight, 80), 360) : 0
        visible: dlg.bodyText.length > 0
        contentHeight: notes.implicitHeight
        clip: true
        boundsBehavior: Flickable.StopAtBounds
        ScrollIndicator.vertical: ScrollIndicator {}

        Label {
            id: notes
            width: parent.width
            text: dlg.bodyText
            color: Theme.textDim
            font.pixelSize: 14
            wrapMode: Text.WordWrap
            lineHeight: 1.3
        }
    }

    onAccepted: {
        if (mode === "pending")
            Updater.download()
        else if (mode === "whatsNew")
            Updater.acknowledgeNotes()
    }
}

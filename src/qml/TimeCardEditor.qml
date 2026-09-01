import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts
import OpenPunchClock

ColoDialog {
    id: dlg
    title: qsTr("Fiche horaire")
    property var onSaved: function() {}

    property string entryId: ""
    property string projectId: ""

    function openNew() {
        entryId = ""
        projectId = AppController.projects.defaultProjectId()
        syncProjectBox()
        const now = Date.now()
        startField.text = Qt.formatDateTime(new Date(now), "yyyy-MM-dd HH:mm")
        endField.text = Qt.formatDateTime(new Date(now + 3600000), "yyyy-MM-dd HH:mm")
        breakField.text = "0"
        notesField.text = ""
        tagsField.text = ""
        reimbField.text = "0"
        deductField.text = "0"
        open()
    }

    function openEdit(eid, pid, startMs, endMs, breakMs, notes, tags, reimb, deduct) {
        entryId = eid
        projectId = pid
        syncProjectBox()
        startField.text = Qt.formatDateTime(new Date(startMs), "yyyy-MM-dd HH:mm")
        endField.text = endMs > 0 ? Qt.formatDateTime(new Date(endMs), "yyyy-MM-dd HH:mm") : ""
        breakField.text = String(Math.round(breakMs / 60000))
        notesField.text = notes || ""
        tagsField.text = tags || ""
        reimbField.text = String(reimb || 0)
        deductField.text = String(deduct || 0)
        open()
    }

    function syncProjectBox() {
        for (let i = 0; i < AppController.projects.count; ++i) {
            if (AppController.projects.projectIdAt(i) === dlg.projectId) {
                projectBox.currentIndex = i
                return
            }
        }
    }

    function parseMs(text) {
        const d = new Date(text.replace(" ", "T"))
        return isNaN(d.getTime()) ? 0 : d.getTime()
    }

    ColumnLayout {
        width: parent.width
        ComboBox {
            id: projectBox
            Layout.fillWidth: true
            implicitHeight: Theme.touchTarget
            model: AppController.projects
            textRole: "name"
            onActivated: dlg.projectId = AppController.projects.projectIdAt(currentIndex)
        }
        ColoTextField { id: startField; hint: qsTr("Début (yyyy-MM-dd HH:mm)"); Layout.fillWidth: true }
        ColoTextField { id: endField; hint: qsTr("Fin"); Layout.fillWidth: true }
        ColoTextField { id: breakField; hint: qsTr("Pause (minutes)"); Layout.fillWidth: true }
        ColoTextField { id: notesField; hint: qsTr("Notes"); Layout.fillWidth: true }
        ColoTextField { id: tagsField; hint: qsTr("Tags"); Layout.fillWidth: true }
        ColoTextField { id: reimbField; hint: qsTr("Remboursement"); Layout.fillWidth: true }
        ColoTextField { id: deductField; hint: qsTr("Déduction"); Layout.fillWidth: true }
        Button {
            Layout.fillWidth: true
            visible: dlg.entryId.length > 0
            text: qsTr("Supprimer cette entrée")
            Material.foreground: Theme.danger
            onClicked: deleteConfirm.open()
        }
    }

    ColoDialog {
        id: deleteConfirm
        title: qsTr("Confirmer la suppression ?")
        destructive: true
        acceptText: qsTr("Supprimer")
        parent: Overlay.overlay
        onAccepted: {
            if (AppController.entries.deleteEntry(dlg.entryId)) {
                dlg.onSaved()
                AppController.showToast(qsTr("Entrée supprimée"))
                dlg.close()
            }
        }
    }

    onAccepted: {
        if (!dlg.projectId) {
            AppController.showToast(qsTr("Choisissez un projet"))
            return
        }
        const ok = AppController.entries.saveManualEntry(
            dlg.entryId, dlg.projectId,
            parseMs(startField.text), parseMs(endField.text),
            (parseInt(breakField.text) || 0) * 60000,
            notesField.text, tagsField.text,
            parseFloat(reimbField.text) || 0, parseFloat(deductField.text) || 0)
        if (ok) {
            dlg.onSaved()
            AppController.showToast(qsTr("Fiche enregistrée"))
        } else {
            AppController.showToast(qsTr("Enregistrement impossible"))
        }
    }
}

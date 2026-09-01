import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import OpenPunchClock

ColoDialog {
    id: dlg
    title: "Time card"
    property var onSaved: function() {}

    property string entryId: ""
    property string projectId: ""

    function openNew() {
        entryId = ""
        projectId = AppController.projects.defaultProjectId()
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
        startField.text = Qt.formatDateTime(new Date(startMs), "yyyy-MM-dd HH:mm")
        endField.text = endMs > 0 ? Qt.formatDateTime(new Date(endMs), "yyyy-MM-dd HH:mm") : ""
        breakField.text = String(Math.round(breakMs / 60000))
        notesField.text = notes || ""
        tagsField.text = tags || ""
        reimbField.text = String(reimb || 0)
        deductField.text = String(deduct || 0)
        open()
    }

    function parseMs(text) {
        const d = new Date(text.replace(" ", "T"))
        return isNaN(d.getTime()) ? 0 : d.getTime()
    }

    ColumnLayout {
        width: parent.width
        ColoTextField { id: startField; placeholderText: "Début (yyyy-MM-dd HH:mm)"; Layout.fillWidth: true }
        ColoTextField { id: endField; placeholderText: "Fin"; Layout.fillWidth: true }
        ColoTextField { id: breakField; placeholderText: "Pause (minutes)"; Layout.fillWidth: true }
        ColoTextField { id: notesField; placeholderText: "Notes"; Layout.fillWidth: true }
        ColoTextField { id: tagsField; placeholderText: "Tags"; Layout.fillWidth: true }
        ColoTextField { id: reimbField; placeholderText: "Remboursement"; Layout.fillWidth: true }
        ColoTextField { id: deductField; placeholderText: "Déduction"; Layout.fillWidth: true }
    }

    onAccepted: {
        const ok = AppController.entries.saveManualEntry(
            dlg.entryId, dlg.projectId,
            parseMs(startField.text), parseMs(endField.text),
            (parseInt(breakField.text) || 0) * 60000,
            notesField.text, tagsField.text,
            parseFloat(reimbField.text) || 0, parseFloat(deductField.text) || 0)
        if (ok)
            dlg.onSaved()
    }
}

pragma Singleton
import QtQuick

QtObject {
    function formatStamp(ms) {
        if (!ms || ms <= 0)
            return "—"
        const d = new Date(ms)
        const time = Qt.formatDateTime(d, "HH:mm")
        const now = new Date()
        if (d.toDateString() === now.toDateString())
            return qsTr("aujourd'hui à ") + time
        const yesterday = new Date(now.getTime() - 86400000)
        if (d.toDateString() === yesterday.toDateString())
            return qsTr("hier à ") + time
        return Qt.formatDateTime(d, qsTr("d MMM 'à' HH:mm"))
    }

    function formatDuration(ms) {
        if (!ms || ms <= 0)
            return "00:00:00"
        const s = Math.floor(ms / 1000)
        const h = Math.floor(s / 3600)
        const m = Math.floor((s % 3600) / 60)
        const sec = s % 60
        return [h, m, sec].map(v => String(v).padStart(2, "0")).join(":")
    }

    function formatHours(h) {
        return h.toFixed(2) + qsTr(" h")
    }

    function dayLabel(ms) {
        const d = new Date(ms)
        const now = new Date()
        if (d.toDateString() === now.toDateString())
            return qsTr("Aujourd'hui")
        if (new Date(now.getTime() - 86400000).toDateString() === d.toDateString())
            return qsTr("Hier")
        return Qt.formatDate(d, qsTr("dddd d MMMM"))
    }
}

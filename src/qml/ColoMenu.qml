import QtQuick
import QtQuick.Controls

// Menu qui s'élargit à son plus long libellé.
Menu {
    parent: ApplicationWindow.window ? ApplicationWindow.window.contentItem : undefined
    implicitWidth: {
        let w = 0
        for (let i = 0; i < count; ++i) {
            const it = itemAt(i)
            if (it)
                w = Math.max(w, it.implicitWidth)
        }
        return w
    }
}

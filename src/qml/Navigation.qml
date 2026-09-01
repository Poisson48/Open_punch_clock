pragma Singleton
import QtQuick

// Référence vers le StackView principal — évite StackView.view (parfois indisponible).
QtObject {
    property var stack: null

    function push(component) {
        if (stack)
            stack.push(component)
    }

    function pop() {
        if (stack && stack.depth > 1)
            stack.pop()
    }
}

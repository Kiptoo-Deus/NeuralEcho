import QtQuick
import QtQuick.Controls
import QtQuick.Dialogs
import NeuralEcho 1.0
import "components"

ApplicationWindow {
    id:      root
    width:   1280
    height:  820
    visible: true
    title:   "NeuralEcho — Acoustic Echo Cancellation Platform v" + appVersion
    color:   ThemeConstants.bgPrimary

    // Single AppController instance shared across all pages
    AppController { id: controller }

    // ── Error dialog ──────────────────────────────────────────────────────────
    MessageDialog {
        id: errorDialog
        buttons: MessageDialog.Ok
    }
    Connections {
        target: controller
        function onErrorOccurred(msg) {
            errorDialog.text    = msg
            errorDialog.title   = "NeuralEcho Error"
            errorDialog.open()
        }
    }

    // ── Main layout: sidebar + page stack ─────────────────────────────────────
    Row {
        anchors { top: parent.top; bottom: statusBar.top; left: parent.left; right: parent.right }

        SidebarNav {
            id: sidebar
            width:  180
            height: parent.height
            onPageSelected: (idx) => stack.currentIndex = idx
        }

        StackLayout {
            id:     stack
            width:  parent.width - sidebar.width
            height: parent.height
            currentIndex: 0

            DashboardPage      { controller: controller }
            SpectrogramPage    { controller: controller }
            MetricsPage        { controller: controller }
            ExperimentsPage    {}
            ModelInspectorPage {}
        }
    }

    // ── Global status bar ─────────────────────────────────────────────────────
    StatusBar {
        id: statusBar
        anchors { left: parent.left; right: parent.right; bottom: parent.bottom }
        height: 28
        erleDb:     controller.erleDb
        rtf:        controller.rtf
        running:    controller.running
        deviceName: controller.inputDevice
    }
}

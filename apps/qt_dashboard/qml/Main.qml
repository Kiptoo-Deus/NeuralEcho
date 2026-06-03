import QtQuick
import QtQuick.Controls
import NeuralEcho 1.0
import "components"

ApplicationWindow {
    id: root
    width:  1280
    height: 800
    visible: true
    title:   "NeuralEcho — Acoustic Echo Cancellation Platform"
    color:   ThemeConstants.bgPrimary

    AppController { id: controller }

    // ── Layout: sidebar + content ──────────────────────────────────────────────
    Row {
        anchors.fill: parent

        SidebarNav {
            id:     sidebar
            width:  180
            height: parent.height
            onPageSelected: (idx) => stack.currentIndex = idx
        }

        StackLayout {
            id:     stack
            width:  parent.width - sidebar.width
            height: parent.height
            currentIndex: 0

            DashboardPage     { controller: controller }
            SpectrogramPage   { controller: controller }
            MetricsPage       { controller: controller }
            ExperimentsPage   {}
            ModelInspectorPage{}
        }
    }

    // ── Global status bar ──────────────────────────────────────────────────────
    StatusBar {
        anchors { left: parent.left; right: parent.right; bottom: parent.bottom }
        height: 28
        erleDb:    controller.erleDb
        rtf:       controller.rtf
        running:   controller.running
        deviceName: controller.deviceName
    }
}

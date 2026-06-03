import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "components"

Rectangle {
    color: ThemeConstants.bgPrimary
    property var controller

    ColumnLayout {
        anchors { fill: parent; margins: ThemeConstants.spacingM }
        spacing: ThemeConstants.spacingM

        // ── Header row ────────────────────────────────────────────────────────
        RowLayout {
            spacing: ThemeConstants.spacingM

            Text {
                text:  "Live Dashboard"
                color: ThemeConstants.textPrimary
                font { family: ThemeConstants.fontFamily
                       pixelSize: ThemeConstants.fontSizeTitle; bold: true }
            }
            Item { Layout.fillWidth: true }

            // Neural toggle
            RowLayout {
                spacing: 6
                Text { text: "Neural AEC"; color: ThemeConstants.textMuted
                       font.pixelSize: ThemeConstants.fontSizeSmall }
                Switch {
                    id: neuralSwitch
                    checked: controller ? controller.neuralEnabled : true
                    onToggled: if (controller) controller.neuralEnabled = checked
                    indicator: Rectangle {
                        implicitWidth: 40; implicitHeight: 20; radius: 10
                        color: neuralSwitch.checked
                               ? ThemeConstants.accentGreen : ThemeConstants.border
                        Rectangle {
                            x: neuralSwitch.checked ? parent.width - width - 2 : 2
                            y: 2; width: 16; height: 16; radius: 8
                            color: ThemeConstants.textPrimary
                            Behavior on x { NumberAnimation { duration: 120 } }
                        }
                    }
                }
            }

            // Start / Stop
            Rectangle {
                width: 120; height: 36; radius: ThemeConstants.radius
                color: controller && controller.running
                       ? ThemeConstants.accentRed   + "22"
                       : ThemeConstants.accentGreen + "22"
                border.color: controller && controller.running
                              ? ThemeConstants.accentRed : ThemeConstants.accentGreen
                Text {
                    anchors.centerIn: parent
                    text: controller && controller.running ? "⏹  Stop" : "▶  Start"
                    color: controller && controller.running
                           ? ThemeConstants.accentRed : ThemeConstants.accentGreen
                    font { family: ThemeConstants.fontFamily
                           pixelSize: ThemeConstants.fontSizeBase; bold: true }
                }
                MouseArea {
                    anchors.fill: parent; cursorShape: Qt.PointingHandCursor
                    onClicked: {
                        if (!controller) return
                        controller.running ? controller.stopSession()
                                           : controller.startSession()
                    }
                }
            }
        }

        // ── Device selectors ──────────────────────────────────────────────────
        RowLayout {
            spacing: ThemeConstants.spacingM
            Layout.fillWidth: true

            Repeater {
                model: [
                    { label: "Input Device",  prop: "input"  },
                    { label: "Output Device", prop: "output" }
                ]
                delegate: RowLayout {
                    spacing: 6
                    Text { text: modelData.label; color: ThemeConstants.textMuted
                           font.pixelSize: ThemeConstants.fontSizeSmall }
                    ComboBox {
                        id: devCombo
                        Layout.preferredWidth: 200
                        model: modelData.prop === "input"
                               ? (controller ? controller.availableInputDevices()  : [])
                               : (controller ? controller.availableOutputDevices() : [])
                        background: Rectangle {
                            color: ThemeConstants.bgCard; radius: ThemeConstants.radius
                            border.color: ThemeConstants.border
                        }
                        contentItem: Text {
                            leftPadding: 8; text: parent.displayText
                            color: ThemeConstants.textPrimary
                            font.pixelSize: ThemeConstants.fontSizeSmall
                            verticalAlignment: Text.AlignVCenter
                            elide: Text.ElideRight
                        }
                        onActivated: {
                            if (!controller) return
                            modelData.prop === "input"
                                ? controller.selectInputDevice(currentIndex)
                                : controller.selectOutputDevice(currentIndex)
                        }
                    }
                }
            }

            Item { Layout.fillWidth: true }

            // Refresh devices
            Rectangle {
                width: 90; height: 28; radius: ThemeConstants.radius
                color: ThemeConstants.bgCard; border.color: ThemeConstants.border
                Text { anchors.centerIn: parent; text: "⟳ Refresh"
                       color: ThemeConstants.textMuted
                       font.pixelSize: ThemeConstants.fontSizeSmall }
                MouseArea {
                    anchors.fill: parent; cursorShape: Qt.PointingHandCursor
                    onClicked: if (controller) controller.refreshDevices()
                }
            }
        }

        // ── Metric cards ──────────────────────────────────────────────────────
        RowLayout {
            spacing: ThemeConstants.spacingM
            MetricCard {
                label: "ERLE"
                value: controller ? controller.erleDb.toFixed(1) : "0.0"
                unit:  "dB"; accent: ThemeConstants.accentBlue
            }
            MetricCard {
                label: "RTF"
                value: controller ? controller.rtf.toFixed(3) : "0.000"
                unit:  ""; accent: ThemeConstants.accentGreen
            }
            MetricCard {
                label: "CPU"
                value: controller ? controller.cpuPercent.toFixed(1) : "0.0"
                unit:  "%"; accent: ThemeConstants.accentOrange
            }
            MetricCard {
                label: "Frames"
                value: controller ? (controller.framesProcessed || 0).toString() : "0"
                unit:  ""; accent: ThemeConstants.accentPurple
            }
        }

        // ── Waveforms ─────────────────────────────────────────────────────────
        WaveformPlot {
            Layout.fillWidth: true; Layout.preferredHeight: 90
            label: "Microphone Input"
            samples: controller && controller.running ? controller.inputWaveform() : []
            waveColor: ThemeConstants.accentBlue
        }
        WaveformPlot {
            Layout.fillWidth: true; Layout.preferredHeight: 90
            label: "Processed Output (AEC)"
            samples: controller && controller.running ? controller.outputWaveform() : []
            waveColor: ThemeConstants.accentGreen
        }

        // Refresh waveforms at 25 Hz via frameReady signal
        Connections {
            target: controller
            function onFrameReady() { /* bindings auto-update */ }
        }

        Item { Layout.fillHeight: true }
    }
}

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

        // Header row
        RowLayout {
            spacing: ThemeConstants.spacingM

            Text {
                text:  "Live Dashboard"
                color: ThemeConstants.textPrimary
                font { family: ThemeConstants.fontFamily
                       pixelSize: ThemeConstants.fontSizeTitle; bold: true }
            }
            Item { Layout.fillWidth: true }

            // Start / Stop button
            Rectangle {
                width: 110; height: 36
                radius: ThemeConstants.radius
                color: controller.running
                       ? ThemeConstants.accentRed + "33"
                       : ThemeConstants.accentGreen + "33"
                border.color: controller.running
                              ? ThemeConstants.accentRed
                              : ThemeConstants.accentGreen

                Text {
                    anchors.centerIn: parent
                    text:  controller.running ? "⏹  Stop" : "▶  Start"
                    color: controller.running
                           ? ThemeConstants.accentRed
                           : ThemeConstants.accentGreen
                    font { family: ThemeConstants.fontFamily
                           pixelSize: ThemeConstants.fontSizeBase; bold: true }
                }
                MouseArea {
                    anchors.fill: parent
                    cursorShape:  Qt.PointingHandCursor
                    onClicked: controller.running
                               ? controller.stopSession()
                               : controller.startSession()
                }
            }
        }

        // Metric cards
        RowLayout {
            spacing: ThemeConstants.spacingM
            MetricCard {
                label: "ERLE"
                value: controller.erleDb.toFixed(1)
                unit:  "dB"
                accent: ThemeConstants.accentBlue
            }
            MetricCard {
                label: "RTF"
                value: controller.rtf.toFixed(3)
                unit:  ""
                accent: ThemeConstants.accentGreen
            }
            MetricCard {
                label: "CPU"
                value: controller.cpuPercent.toFixed(1)
                unit:  "%"
                accent: ThemeConstants.accentOrange
            }
        }

        // Waveforms
        WaveformPlot {
            Layout.fillWidth: true
            Layout.preferredHeight: 100
            label:     "Input (Mic)"
            samples:   controller.running ? controller.inputWaveform() : []
            waveColor: ThemeConstants.accentBlue
        }

        WaveformPlot {
            Layout.fillWidth: true
            Layout.preferredHeight: 100
            label:     "Output (Processed)"
            samples:   controller.running ? controller.outputWaveform() : []
            waveColor: ThemeConstants.accentGreen
        }

        // Refresh waveforms on frame signal
        Connections {
            target: controller
            function onFrameReady() {
                // Waveform bindings auto-update via property access;
                // requestPaint() is handled inside WaveformPlot.
            }
        }

        Item { Layout.fillHeight: true }
    }
}

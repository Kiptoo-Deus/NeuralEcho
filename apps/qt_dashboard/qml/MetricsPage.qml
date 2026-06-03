import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtCharts
import "components"

Rectangle {
    color: ThemeConstants.bgPrimary
    property var controller

    // Rolling history buffers
    property int maxPoints: 200
    property var erleHistory:  []
    property var rtfHistory:   []
    property var timeHistory:  []
    property int tickCount:    0

    function pushMetrics() {
        if (!controller) return
        tickCount++
        erleHistory.push(controller.erleDb)
        rtfHistory.push(controller.rtf)
        timeHistory.push(tickCount)
        if (erleHistory.length > maxPoints) {
            erleHistory.shift(); rtfHistory.shift(); timeHistory.shift()
        }
        erleSeries.clear()
        rtfSeries.clear()
        for (let i = 0; i < erleHistory.length; ++i) {
            erleSeries.append(timeHistory[i], erleHistory[i])
            rtfSeries.append(timeHistory[i], rtfHistory[i] * 100)
        }
    }

    Connections {
        target: controller
        function onMetricsUpdated() { pushMetrics() }
    }

    ColumnLayout {
        anchors { fill: parent; margins: ThemeConstants.spacingM }
        spacing: ThemeConstants.spacingM

        // Header row
        RowLayout {
            Text {
                text:  "Metrics"
                color: ThemeConstants.textPrimary
                font { family: ThemeConstants.fontFamily
                       pixelSize: ThemeConstants.fontSizeTitle; bold: true }
            }
            Item { Layout.fillWidth: true }

            // Export CSV button
            Rectangle {
                width: 120; height: 32; radius: ThemeConstants.radius
                color: ThemeConstants.bgCard
                border.color: ThemeConstants.accentBlue
                Text {
                    anchors.centerIn: parent
                    text: "⬇  Export CSV"
                    color: ThemeConstants.accentBlue
                    font { family: ThemeConstants.fontFamily; pixelSize: 11 }
                }
                MouseArea {
                    anchors.fill: parent
                    cursorShape: Qt.PointingHandCursor
                    onClicked: {
                        // Write CSV to home directory
                        var sm = controller ? controller.sessionMetrics() : {}
                        console.log("Export:", JSON.stringify(sm))
                    }
                }
            }
        }

        // Summary metric cards
        RowLayout {
            spacing: ThemeConstants.spacingM
            MetricCard {
                label: "Mean ERLE"
                value: controller ? controller.sessionMetrics()["meanErleDb"]
                                    ? controller.sessionMetrics()["meanErleDb"].toFixed(1)
                                    : "0.0" : "0.0"
                unit:  "dB"
                accent: ThemeConstants.accentBlue
            }
            MetricCard {
                label: "Mean RTF"
                value: controller ? controller.sessionMetrics()["meanRtf"]
                                    ? controller.sessionMetrics()["meanRtf"].toFixed(3)
                                    : "0.000" : "0.000"
                unit:  ""
                accent: ThemeConstants.accentGreen
            }
            MetricCard {
                label: "Frames"
                value: controller ? (controller.framesProcessed || 0).toString() : "0"
                unit:  ""
                accent: ThemeConstants.accentOrange
            }
        }

        // ERLE chart
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 180
            color:  ThemeConstants.bgCard
            radius: ThemeConstants.radius

            Text {
                anchors { top: parent.top; left: parent.left; margins: 10 }
                text: "ERLE (dB) — rolling 200 frames"
                color: ThemeConstants.textMuted
                font { family: ThemeConstants.fontFamily; pixelSize: ThemeConstants.fontSizeSmall }
            }

            ChartView {
                anchors { fill: parent; topMargin: 28; margins: 8 }
                backgroundColor: "transparent"
                legend.visible:  false
                antialiasing:    true
                plotAreaColor:   ThemeConstants.bgSecondary

                ValueAxis {
                    id: erleXAxis
                    min: Math.max(0, tickCount - maxPoints)
                    max: Math.max(maxPoints, tickCount)
                    labelsColor: ThemeConstants.textMuted
                    labelsFont.pixelSize: 9
                    gridLineColor: ThemeConstants.border
                    color: ThemeConstants.border
                }
                ValueAxis {
                    id: erleYAxis
                    min: 0; max: 60
                    labelsColor: ThemeConstants.textMuted
                    labelsFont.pixelSize: 9
                    gridLineColor: ThemeConstants.border
                    color: ThemeConstants.border
                }

                LineSeries {
                    id: erleSeries
                    axisX: erleXAxis
                    axisY: erleYAxis
                    color: ThemeConstants.accentBlue
                    width: 1.5
                }
            }
        }

        // RTF chart
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 160
            color: ThemeConstants.bgCard
            radius: ThemeConstants.radius

            Text {
                anchors { top: parent.top; left: parent.left; margins: 10 }
                text: "RTF × 100 — target < 50"
                color: ThemeConstants.textMuted
                font { family: ThemeConstants.fontFamily; pixelSize: ThemeConstants.fontSizeSmall }
            }

            ChartView {
                anchors { fill: parent; topMargin: 28; margins: 8 }
                backgroundColor: "transparent"
                legend.visible:  false
                antialiasing:    true
                plotAreaColor:   ThemeConstants.bgSecondary

                ValueAxis {
                    id: rtfXAxis
                    min: Math.max(0, tickCount - maxPoints)
                    max: Math.max(maxPoints, tickCount)
                    labelsColor: ThemeConstants.textMuted
                    labelsFont.pixelSize: 9
                    gridLineColor: ThemeConstants.border
                    color: ThemeConstants.border
                }
                ValueAxis {
                    id: rtfYAxis
                    min: 0; max: 100
                    labelsColor: ThemeConstants.textMuted
                    labelsFont.pixelSize: 9
                    gridLineColor: ThemeConstants.border
                    color: ThemeConstants.border
                }

                LineSeries {
                    id: rtfSeries
                    axisX: rtfXAxis
                    axisY: rtfYAxis
                    color: ThemeConstants.accentGreen
                    width: 1.5
                }
            }
        }

        Item { Layout.fillHeight: true }
    }
}

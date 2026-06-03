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

        // Header
        RowLayout {
            Text {
                text:  "Spectrogram"
                color: ThemeConstants.textPrimary
                font { family: ThemeConstants.fontFamily
                       pixelSize: ThemeConstants.fontSizeTitle; bold: true }
            }
            Item { Layout.fillWidth: true }
            Text {
                text:  controller && controller.running ? "● LIVE" : "○ STOPPED"
                color: controller && controller.running
                       ? ThemeConstants.accentGreen : ThemeConstants.textMuted
                font { family: ThemeConstants.fontFamily
                       pixelSize: ThemeConstants.fontSizeSmall; bold: true }
            }
        }

        // Three spectrograms side by side
        RowLayout {
            Layout.fillWidth:  true
            Layout.fillHeight: true
            spacing: ThemeConstants.spacingS

            Repeater {
                model: [
                    { label: "Mic Input",      color: "#1a3a5c" },
                    { label: "AEC Output",     color: "#1a5c2a" },
                    { label: "Residual Echo",  color: "#5c2a1a" }
                ]

                delegate: Rectangle {
                    Layout.fillWidth:  true
                    Layout.fillHeight: true
                    color:  modelData.color
                    radius: ThemeConstants.radius
                    clip:   true

                    // Label
                    Text {
                        anchors { top: parent.top; horizontalCenter: parent.horizontalCenter }
                        anchors.topMargin: 8
                        text:  modelData.label
                        color: ThemeConstants.textPrimary
                        font { family: ThemeConstants.fontFamily
                               pixelSize: ThemeConstants.fontSizeSmall; bold: true }
                    }

                    // Scrolling spectrogram canvas
                    Canvas {
                        id: spectCanvas
                        anchors { fill: parent; topMargin: 28; margins: 4 }
                        property int colIndex: 0

                        // ImageData ring-buffer for scrolling effect
                        property var imgData: null

                        Component.onCompleted: requestPaint()

                        onPaint: {
                            const ctx = getContext("2d")
                            if (!imgData) {
                                imgData = ctx.createImageData(width, height)
                                // Fill black
                                for (let i = 0; i < imgData.data.length; i += 4) {
                                    imgData.data[i]   = 26
                                    imgData.data[i+1] = 26
                                    imgData.data[i+2] = 26
                                    imgData.data[i+3] = 255
                                }
                            }
                            ctx.putImageData(imgData, 0, 0)
                        }

                        // Scroll one column per frame
                        function pushColumn(magnitudes) {
                            if (!imgData) return
                            const h = height
                            const w = width
                            // Shift image left by 1px
                            const d = imgData.data
                            for (let y = 0; y < h; ++y) {
                                for (let x = 0; x < w - 1; ++x) {
                                    const dst = (y * w + x) * 4
                                    const src = (y * w + x + 1) * 4
                                    d[dst]   = d[src]
                                    d[dst+1] = d[src+1]
                                    d[dst+2] = d[src+2]
                                    d[dst+3] = 255
                                }
                                // Write new rightmost column
                                const binIdx = Math.floor((1.0 - y / h) * magnitudes.length)
                                const val    = Math.max(0, Math.min(1,
                                    magnitudes[Math.min(binIdx, magnitudes.length - 1)] || 0))
                                const px = (y * w + w - 1) * 4
                                // Heat-map: cold=blue → warm=red
                                d[px]   = Math.round(255 * val)
                                d[px+1] = Math.round(100 * val * (1 - val) * 4)
                                d[px+2] = Math.round(255 * (1 - val))
                                d[px+3] = 255
                            }
                            requestPaint()
                        }
                    }
                }
            }
        }

        // Frequency axis label
        RowLayout {
            Layout.fillWidth: true
            Text { text: "0 Hz";    color: ThemeConstants.textMuted; font.pixelSize: 9 }
            Item { Layout.fillWidth: true }
            Text { text: "4000 Hz"; color: ThemeConstants.textMuted; font.pixelSize: 9 }
            Item { Layout.fillWidth: true }
            Text { text: "8000 Hz"; color: ThemeConstants.textMuted; font.pixelSize: 9 }
        }
    }
}

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "components"

Rectangle {
    color: ThemeConstants.bgPrimary

    // Simulated ONNX layer data — replaced with real data from C++ backend
    property var layers: [
        { name: "input",         type: "Input",    params: 0,    time_us: 0 },
        { name: "encoder/conv0", type: "Conv1d",   params: 896,  time_us: 42 },
        { name: "encoder/prelu0",type: "PReLU",    params: 32,   time_us: 5  },
        { name: "encoder/conv1", type: "Conv1d",   params: 18496,time_us: 88 },
        { name: "encoder/prelu1",type: "PReLU",    params: 64,   time_us: 6  },
        { name: "encoder/conv2", type: "Conv1d",   params: 73856,time_us: 143},
        { name: "encoder/prelu2",type: "PReLU",    params: 128,  time_us: 7  },
        { name: "gru",           type: "GRU",      params: 591360,time_us:1420},
        { name: "decoder/conv0", type: "Conv1d",   params: 98432,time_us: 198},
        { name: "decoder/prelu0",type: "PReLU",    params: 128,  time_us: 7  },
        { name: "decoder/conv1", type: "Conv1d",   params: 24608,time_us: 72 },
        { name: "decoder/prelu1",type: "PReLU",    params: 32,   time_us: 5  },
        { name: "decoder/conv2", type: "Conv1d",   params: 33,   time_us: 12 },
        { name: "sigmoid",       type: "Sigmoid",  params: 0,    time_us: 8  },
        { name: "output",        type: "Output",   params: 0,    time_us: 0  }
    ]

    property int selectedLayer: 7   // GRU selected by default

    ColumnLayout {
        anchors { fill: parent; margins: ThemeConstants.spacingM }
        spacing: ThemeConstants.spacingM

        // Header
        RowLayout {
            Text {
                text:  "Model Inspector"
                color: ThemeConstants.textPrimary
                font { family: ThemeConstants.fontFamily
                       pixelSize: ThemeConstants.fontSizeTitle; bold: true }
            }
            Item { Layout.fillWidth: true }
            // Summary badge
            Rectangle {
                height: 28; width: summaryText.width + 20
                color: ThemeConstants.bgCard; radius: ThemeConstants.radius
                border.color: ThemeConstants.border
                Text {
                    id: summaryText
                    anchors.centerIn: parent
                    text: "~800 K params  ·  INT8  ·  ~200 KB"
                    color: ThemeConstants.textMuted
                    font.pixelSize: ThemeConstants.fontSizeSmall
                }
            }
        }

        // Main two-column layout
        RowLayout {
            Layout.fillWidth:  true
            Layout.fillHeight: true
            spacing: ThemeConstants.spacingM

            // ── Left: layer tree ──────────────────────────────────────────────
            Rectangle {
                Layout.preferredWidth: 240
                Layout.fillHeight: true
                color:  ThemeConstants.bgCard
                radius: ThemeConstants.radius
                clip:   true

                Column {
                    anchors.fill: parent

                    Rectangle {
                        width: parent.width; height: 32
                        color: ThemeConstants.bgSecondary
                        Text {
                            anchors { left: parent.left; verticalCenter: parent.verticalCenter }
                            anchors.leftMargin: 12
                            text: "Layers"
                            color: ThemeConstants.textMuted
                            font { pixelSize: ThemeConstants.fontSizeSmall; bold: true }
                        }
                    }

                    ListView {
                        width:  parent.width
                        height: parent.height - 32
                        model:  layers
                        clip:   true

                        delegate: Rectangle {
                            width:  ListView.view.width; height: 36
                            color:  selectedLayer === index
                                    ? ThemeConstants.accentBlue + "33" : "transparent"

                            Rectangle {
                                visible: selectedLayer === index
                                width: 3; height: parent.height
                                color: ThemeConstants.accentBlue
                            }

                            Row {
                                anchors { left: parent.left; verticalCenter: parent.verticalCenter }
                                anchors.leftMargin: 12
                                spacing: 8

                                // Type colour dot
                                Rectangle {
                                    width: 8; height: 8; radius: 4
                                    anchors.verticalCenter: parent.verticalCenter
                                    color: {
                                        switch(modelData.type) {
                                        case "Conv1d":  return ThemeConstants.accentBlue
                                        case "GRU":     return ThemeConstants.accentPurple
                                        case "PReLU":   return ThemeConstants.accentGreen
                                        case "Sigmoid": return ThemeConstants.accentOrange
                                        default:        return ThemeConstants.textMuted
                                        }
                                    }
                                }

                                Column {
                                    Text {
                                        text:  modelData.name
                                        color: selectedLayer === index
                                               ? ThemeConstants.textPrimary
                                               : ThemeConstants.textMuted
                                        font { pixelSize: 10; family: "Courier" }
                                    }
                                    Text {
                                        text:  modelData.type + "  ·  "
                                               + (modelData.params > 0
                                                  ? modelData.params.toLocaleString() + " p"
                                                  : "—")
                                        color: ThemeConstants.textMuted
                                        font.pixelSize: 9
                                    }
                                }
                            }

                            MouseArea {
                                anchors.fill: parent
                                cursorShape: Qt.PointingHandCursor
                                onClicked: selectedLayer = index
                            }
                        }
                    }
                }
            }

            // ── Right: detail panel ───────────────────────────────────────────
            ColumnLayout {
                Layout.fillWidth:  true
                Layout.fillHeight: true
                spacing: ThemeConstants.spacingM

                // Layer detail card
                Rectangle {
                    Layout.fillWidth: true
                    height: 130
                    color:  ThemeConstants.bgCard
                    radius: ThemeConstants.radius

                    GridLayout {
                        anchors { fill: parent; margins: ThemeConstants.spacingM }
                        columns: 4; rowSpacing: 8; columnSpacing: ThemeConstants.spacingM

                        Repeater {
                            model: [
                                { k: "Name",       v: layers[selectedLayer].name },
                                { k: "Type",       v: layers[selectedLayer].type },
                                { k: "Parameters", v: layers[selectedLayer].params.toLocaleString() },
                                { k: "Infer (µs)", v: layers[selectedLayer].time_us.toString() }
                            ]
                            delegate: Column {
                                spacing: 2
                                Text {
                                    text:  modelData.k
                                    color: ThemeConstants.textMuted
                                    font.pixelSize: ThemeConstants.fontSizeSmall
                                }
                                Text {
                                    text:  modelData.v
                                    color: ThemeConstants.accentBlue
                                    font { pixelSize: ThemeConstants.fontSizeLarge
                                           bold: true; family: "Courier" }
                                }
                            }
                        }
                    }
                }

                // Timing breakdown bar chart
                Rectangle {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    color:  ThemeConstants.bgCard
                    radius: ThemeConstants.radius
                    clip:   true

                    Text {
                        anchors { top: parent.top; left: parent.left; margins: 12 }
                        text: "Per-layer inference time (µs)"
                        color: ThemeConstants.textMuted
                        font { pixelSize: ThemeConstants.fontSizeSmall }
                    }

                    Canvas {
                        anchors { fill: parent; topMargin: 32; margins: 12 }
                        property var layerData: layers

                        onLayerDataChanged: requestPaint()
                        Component.onCompleted: requestPaint()

                        onPaint: {
                            const ctx = getContext("2d")
                            ctx.clearRect(0, 0, width, height)

                            const maxT = Math.max.apply(null, layerData.map(l => l.time_us)) || 1
                            const barH = Math.max(4, (height - 4) / layerData.length - 2)
                            const colors = {
                                "Conv1d": "#58A6FF", "GRU": "#BC8CFF",
                                "PReLU":  "#3FB950", "Sigmoid": "#D29922"
                            }

                            layerData.forEach((layer, i) => {
                                const y   = i * (barH + 2)
                                const bw  = (layer.time_us / maxT) * (width - 60)
                                const col = colors[layer.type] || "#8B949E"

                                ctx.fillStyle = col + "44"
                                ctx.fillRect(0, y, width - 60, barH)

                                ctx.fillStyle = col
                                ctx.fillRect(0, y, bw, barH)

                                ctx.fillStyle = "#E6EDF3"
                                ctx.font = "9px Courier"
                                if (layer.time_us > 0)
                                    ctx.fillText(layer.time_us + "µs", bw + 4, y + barH - 2)
                            })
                        }
                    }
                }
            }
        }
    }
}

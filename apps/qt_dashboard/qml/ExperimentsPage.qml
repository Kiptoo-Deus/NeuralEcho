import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs
import "components"

Rectangle {
    color: ThemeConstants.bgPrimary

    property string datasetPath: ""
    property bool   running:     false
    property var    results:     []
    property int    progress:    0
    property int    totalFiles:  0

    ColumnLayout {
        anchors { fill: parent; margins: ThemeConstants.spacingM }
        spacing: ThemeConstants.spacingM

        // Header
        Text {
            text:  "Experiments"
            color: ThemeConstants.textPrimary
            font { family: ThemeConstants.fontFamily
                   pixelSize: ThemeConstants.fontSizeTitle; bold: true }
        }
        Text {
            text:  "Batch evaluation against AEC Challenge / DNS datasets"
            color: ThemeConstants.textMuted
            font { family: ThemeConstants.fontFamily; pixelSize: ThemeConstants.fontSizeBase }
        }

        // ── Config row ────────────────────────────────────────────────────────
        Rectangle {
            Layout.fillWidth: true
            height: 110
            color:  ThemeConstants.bgCard
            radius: ThemeConstants.radius

            GridLayout {
                anchors { fill: parent; margins: ThemeConstants.spacingM }
                columns: 3
                rowSpacing: ThemeConstants.spacingS
                columnSpacing: ThemeConstants.spacingM

                // Dataset path
                Text { text: "Dataset Directory"; color: ThemeConstants.textMuted
                       font.pixelSize: ThemeConstants.fontSizeSmall }
                Rectangle {
                    Layout.fillWidth: true
                    height: 32; color: ThemeConstants.bgSecondary
                    radius: ThemeConstants.radius
                    border.color: ThemeConstants.border
                    Text {
                        anchors { left: parent.left; verticalCenter: parent.verticalCenter }
                        anchors.leftMargin: 8
                        text: datasetPath.length > 0 ? datasetPath : "Not selected…"
                        color: datasetPath.length > 0
                               ? ThemeConstants.textPrimary : ThemeConstants.textMuted
                        font.pixelSize: ThemeConstants.fontSizeSmall
                        elide: Text.ElideLeft; width: parent.width - 16
                    }
                }
                Rectangle {
                    width: 90; height: 32; radius: ThemeConstants.radius
                    color: ThemeConstants.accentBlue + "22"
                    border.color: ThemeConstants.accentBlue
                    Text {
                        anchors.centerIn: parent
                        text: "Browse…"
                        color: ThemeConstants.accentBlue
                        font.pixelSize: ThemeConstants.fontSizeSmall
                    }
                    MouseArea {
                        anchors.fill: parent
                        cursorShape: Qt.PointingHandCursor
                        onClicked: folderDialog.open()
                    }
                }

                // Model
                Text { text: "Model"; color: ThemeConstants.textMuted
                       font.pixelSize: ThemeConstants.fontSizeSmall }
                ComboBox {
                    Layout.fillWidth: true
                    model: ["neural_echo_net_int8.onnx", "neural_echo_net_fp32.onnx"]
                    background: Rectangle { color: ThemeConstants.bgSecondary
                                            radius: ThemeConstants.radius
                                            border.color: ThemeConstants.border }
                    contentItem: Text {
                        leftPadding: 8; text: parent.displayText
                        color: ThemeConstants.textPrimary
                        font.pixelSize: ThemeConstants.fontSizeSmall
                        verticalAlignment: Text.AlignVCenter
                    }
                }
                Item {}
            }
        }

        // ── Run / progress row ────────────────────────────────────────────────
        RowLayout {
            spacing: ThemeConstants.spacingM

            Rectangle {
                width: 140; height: 38; radius: ThemeConstants.radius
                color: running ? ThemeConstants.accentRed + "22"
                               : ThemeConstants.accentGreen + "22"
                border.color: running ? ThemeConstants.accentRed
                                      : ThemeConstants.accentGreen
                enabled: datasetPath.length > 0

                Text {
                    anchors.centerIn: parent
                    text:  running ? "⏹  Stop" : "▶  Run Evaluation"
                    color: running ? ThemeConstants.accentRed
                                   : ThemeConstants.accentGreen
                    font { family: ThemeConstants.fontFamily
                           pixelSize: ThemeConstants.fontSizeSmall; bold: true }
                }
                MouseArea {
                    anchors.fill: parent; cursorShape: Qt.PointingHandCursor
                    onClicked: running = !running
                }
            }

            // Progress bar
            Rectangle {
                Layout.fillWidth: true; height: 38
                color: ThemeConstants.bgCard; radius: ThemeConstants.radius
                visible: running || progress > 0

                Rectangle {
                    width:  totalFiles > 0
                            ? parent.width * progress / totalFiles : 0
                    height: parent.height
                    color:  ThemeConstants.accentBlue
                    radius: ThemeConstants.radius
                    Behavior on width { NumberAnimation { duration: 150 } }
                }
                Text {
                    anchors.centerIn: parent
                    text: totalFiles > 0
                          ? progress + " / " + totalFiles + " files"
                          : "Waiting…"
                    color: ThemeConstants.textPrimary
                    font.pixelSize: ThemeConstants.fontSizeSmall
                }
            }
        }

        // ── Results table ─────────────────────────────────────────────────────
        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            color:  ThemeConstants.bgCard
            radius: ThemeConstants.radius
            clip:   true

            Column {
                anchors.fill: parent

                // Table header
                Rectangle {
                    width: parent.width; height: 34
                    color: ThemeConstants.bgSecondary
                    radius: ThemeConstants.radius

                    RowLayout {
                        anchors { fill: parent; leftMargin: 12; rightMargin: 12 }
                        Repeater {
                            model: ["File", "ERLE (dB)", "PESQ", "STOI", "RTF"]
                            Text {
                                Layout.fillWidth: true
                                text:  modelData
                                color: ThemeConstants.textMuted
                                font { family: ThemeConstants.fontFamily
                                       pixelSize: ThemeConstants.fontSizeSmall; bold: true }
                            }
                        }
                    }
                }

                ListView {
                    width:  parent.width
                    height: parent.height - 34
                    model:  results
                    clip:   true

                    delegate: Rectangle {
                        width:  ListView.view.width
                        height: 32
                        color:  index % 2 === 0
                                ? ThemeConstants.bgCard : ThemeConstants.bgSecondary

                        RowLayout {
                            anchors { fill: parent; leftMargin: 12; rightMargin: 12 }
                            Repeater {
                                model: [
                                    modelData.file   || "",
                                    modelData.erle   ? modelData.erle.toFixed(1) : "—",
                                    modelData.pesq   ? modelData.pesq.toFixed(2) : "—",
                                    modelData.stoi   ? modelData.stoi.toFixed(3) : "—",
                                    modelData.rtf    ? modelData.rtf.toFixed(3)  : "—"
                                ]
                                Text {
                                    Layout.fillWidth: true
                                    text:  modelData
                                    color: ThemeConstants.textPrimary
                                    font.pixelSize: ThemeConstants.fontSizeSmall
                                    elide: Text.ElideRight
                                }
                            }
                        }
                    }

                    // Empty state
                    Text {
                        anchors.centerIn: parent
                        visible: results.length === 0
                        text: "No results yet. Select a dataset and run evaluation."
                        color: ThemeConstants.textMuted
                        font.pixelSize: ThemeConstants.fontSizeBase
                    }
                }
            }
        }
    }

    // Folder picker dialog
    FolderDialog {
        id: folderDialog
        title: "Select Dataset Directory"
        onAccepted: datasetPath = selectedFolder.toString().replace("file://", "")
    }
}

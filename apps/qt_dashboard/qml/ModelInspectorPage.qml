import QtQuick
import "components"
Rectangle {
    color: ThemeConstants.bgPrimary
    Column {
        anchors { fill: parent; margins: ThemeConstants.spacingM }
        spacing: ThemeConstants.spacingM
        Text { text: "Model Inspector"; color: ThemeConstants.textPrimary
               font { family: ThemeConstants.fontFamily; pixelSize: ThemeConstants.fontSizeTitle; bold: true } }
        Text { text: "ONNX layer tree, activation heatmap, per-layer timing.\nImplementation: Week 19–20."
               color: ThemeConstants.textMuted; font.pixelSize: ThemeConstants.fontSizeBase }
    }
}

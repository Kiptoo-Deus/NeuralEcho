import QtQuick
import "components"
Rectangle {
    color: ThemeConstants.bgPrimary
    Column {
        anchors { fill: parent; margins: ThemeConstants.spacingM }
        spacing: ThemeConstants.spacingM
        Text { text: "Experiments"; color: ThemeConstants.textPrimary
               font { family: ThemeConstants.fontFamily; pixelSize: ThemeConstants.fontSizeTitle; bold: true } }
        Text { text: "Batch dataset evaluation and auto-report generation.\nImplementation: Week 19–20."
               color: ThemeConstants.textMuted; font.pixelSize: ThemeConstants.fontSizeBase }
    }
}

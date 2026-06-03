import QtQuick
import "components"
Rectangle {
    color: ThemeConstants.bgPrimary
    property var controller
    Column {
        anchors { fill: parent; margins: ThemeConstants.spacingM }
        spacing: ThemeConstants.spacingM
        Text { text: "Metrics"; color: ThemeConstants.textPrimary
               font { family: ThemeConstants.fontFamily; pixelSize: ThemeConstants.fontSizeTitle; bold: true } }
        Text { text: "Session ERLE / PESQ / STOI charts and CSV export.\nImplementation: Week 17–18."
               color: ThemeConstants.textMuted; font.pixelSize: ThemeConstants.fontSizeBase }
    }
}

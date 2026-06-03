import QtQuick
import QtQuick.Controls
import "components"

Rectangle {
    color: ThemeConstants.bgPrimary
    property var controller

    Column {
        anchors { fill: parent; margins: ThemeConstants.spacingM }
        spacing: ThemeConstants.spacingM

        Text {
            text: "Spectrogram"
            color: ThemeConstants.textPrimary
            font { family: ThemeConstants.fontFamily
                   pixelSize: ThemeConstants.fontSizeTitle; bold: true }
        }
        Text {
            text: "Real-time before/after spectrogram visualisation.\nImplementation: Week 17–18."
            color: ThemeConstants.textMuted
            font.pixelSize: ThemeConstants.fontSizeBase
        }
    }
}

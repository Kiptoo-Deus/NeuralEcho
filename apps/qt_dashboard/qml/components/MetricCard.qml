import QtQuick
import "."

Rectangle {
    id: root
    width:  160
    height: 90
    color:  ThemeConstants.bgCard
    radius: ThemeConstants.radiusLg

    property string label:    "ERLE"
    property string value:    "0.0"
    property string unit:     "dB"
    property color  accent:   ThemeConstants.accentBlue

    // Top accent strip
    Rectangle {
        width: parent.width; height: 3
        radius: ThemeConstants.radiusLg
        color: root.accent
    }

    Column {
        anchors.centerIn: parent
        spacing: 4

        Text {
            anchors.horizontalCenter: parent.horizontalCenter
            text:  root.label
            color: ThemeConstants.textMuted
            font { family: ThemeConstants.fontFamily
                   pixelSize: ThemeConstants.fontSizeSmall }
        }

        Row {
            anchors.horizontalCenter: parent.horizontalCenter
            spacing: 3
            Text {
                text:  root.value
                color: root.accent
                font { family: ThemeConstants.fontFamily
                       pixelSize: 26; bold: true }
            }
            Text {
                text:  root.unit
                color: ThemeConstants.textMuted
                font.pixelSize: ThemeConstants.fontSizeSmall
                anchors.bottom: parent.bottom
                anchors.bottomMargin: 4
            }
        }
    }
}

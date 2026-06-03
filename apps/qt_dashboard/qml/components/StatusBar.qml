import QtQuick
import QtQuick.Controls
import "."

Rectangle {
    color: ThemeConstants.bgSecondary

    property bool   running:    false
    property real   erleDb:     0
    property real   rtf:        0
    property string deviceName: "No device"

    Rectangle {
        width: parent.width; height: 1; color: ThemeConstants.border
    }

    Row {
        anchors { left: parent.left; verticalCenter: parent.verticalCenter }
        anchors.leftMargin: 16
        spacing: 24

        // Status dot
        Row {
            spacing: 6
            anchors.verticalCenter: parent.verticalCenter
            Rectangle {
                width: 8; height: 8; radius: 4
                anchors.verticalCenter: parent.verticalCenter
                color: running ? ThemeConstants.accentGreen : ThemeConstants.textMuted
            }
            Text {
                text:  running ? "Running" : "Stopped"
                color: running ? ThemeConstants.accentGreen : ThemeConstants.textMuted
                font { family: ThemeConstants.fontFamily
                       pixelSize: ThemeConstants.fontSizeSmall }
            }
        }

        Text {
            text:  deviceName
            color: ThemeConstants.textMuted
            font.pixelSize: ThemeConstants.fontSizeSmall
        }
    }

    Row {
        anchors { right: parent.right; verticalCenter: parent.verticalCenter }
        anchors.rightMargin: 16
        spacing: 20

        Text {
            text:  "ERLE: " + erleDb.toFixed(1) + " dB"
            color: erleDb > 30 ? ThemeConstants.accentGreen : ThemeConstants.accentOrange
            font { family: ThemeConstants.fontFamily
                   pixelSize: ThemeConstants.fontSizeSmall; bold: true }
        }
        Text {
            text:  "RTF: " + rtf.toFixed(3)
            color: rtf < 0.5 ? ThemeConstants.accentGreen : ThemeConstants.accentRed
            font { family: ThemeConstants.fontFamily
                   pixelSize: ThemeConstants.fontSizeSmall }
        }
        Text {
            text:  "v" + appVersion
            color: ThemeConstants.textMuted
            font.pixelSize: ThemeConstants.fontSizeSmall
        }
    }
}

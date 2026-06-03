import QtQuick
import QtQuick.Controls
import "."

Rectangle {
    id: root
    color: "#0D1117"
    signal pageSelected(int index)

    property int selectedIndex: 0

    // Logo / title area
    Column {
        anchors { top: parent.top; left: parent.left; right: parent.right }
        anchors.topMargin: 20

        Text {
            anchors.horizontalCenter: parent.horizontalCenter
            text:  "NeuralEcho"
            font { family: ThemeConstants.fontFamily; pixelSize: 18; bold: true }
            color: ThemeConstants.accentBlue
        }
        Text {
            anchors.horizontalCenter: parent.horizontalCenter
            text:  "v1.0"
            font.pixelSize: ThemeConstants.fontSizeSmall
            color: ThemeConstants.textMuted
        }
    }

    // Nav items
    readonly property var pages: [
        { label: "Dashboard",   icon: "⊞" },
        { label: "Spectrogram", icon: "◈" },
        { label: "Metrics",     icon: "◉" },
        { label: "Experiments", icon: "⊡" },
        { label: "Inspector",   icon: "⊙" },
    ]

    Column {
        anchors { top: parent.top; left: parent.left; right: parent.right }
        anchors.topMargin: 80
        spacing: 2

        Repeater {
            model: root.pages
            delegate: Rectangle {
                width:  root.width
                height: 44
                color: root.selectedIndex === index
                       ? ThemeConstants.bgCard
                       : "transparent"
                radius: ThemeConstants.radius

                // Active indicator bar
                Rectangle {
                    visible: root.selectedIndex === index
                    width: 3; height: parent.height
                    color: ThemeConstants.accentBlue
                    radius: 2
                }

                Row {
                    anchors { left: parent.left; verticalCenter: parent.verticalCenter }
                    anchors.leftMargin: 16
                    spacing: 10

                    Text {
                        text:  modelData.icon
                        color: root.selectedIndex === index
                               ? ThemeConstants.accentBlue
                               : ThemeConstants.textMuted
                        font.pixelSize: 16
                    }
                    Text {
                        text:  modelData.label
                        color: root.selectedIndex === index
                               ? ThemeConstants.textPrimary
                               : ThemeConstants.textMuted
                        font { family: ThemeConstants.fontFamily
                               pixelSize: ThemeConstants.fontSizeBase }
                    }
                }

                MouseArea {
                    anchors.fill: parent
                    cursorShape:  Qt.PointingHandCursor
                    onClicked: {
                        root.selectedIndex = index
                        root.pageSelected(index)
                    }
                }
            }
        }
    }

    // Bottom: version
    Text {
        anchors { bottom: parent.bottom; horizontalCenter: parent.horizontalCenter }
        anchors.bottomMargin: 12
        text:  "© 2024 Joel Kiptoo"
        color: ThemeConstants.textMuted
        font.pixelSize: 9
    }
}

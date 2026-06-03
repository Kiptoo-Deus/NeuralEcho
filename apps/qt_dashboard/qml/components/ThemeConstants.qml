pragma Singleton
import QtQuick

QtObject {
    // Backgrounds
    readonly property color bgPrimary:   "#0D1117"
    readonly property color bgSecondary: "#161B22"
    readonly property color bgCard:      "#1F2937"
    readonly property color border:      "#30363D"

    // Accent colours
    readonly property color accentBlue:   "#58A6FF"
    readonly property color accentGreen:  "#3FB950"
    readonly property color accentOrange: "#D29922"
    readonly property color accentRed:    "#F78166"
    readonly property color accentPurple: "#BC8CFF"

    // Text
    readonly property color textPrimary:  "#E6EDF3"
    readonly property color textMuted:    "#8B949E"

    // Typography
    readonly property int fontSizeSmall:  11
    readonly property int fontSizeBase:   13
    readonly property int fontSizeLarge:  16
    readonly property int fontSizeTitle:  20

    readonly property string fontFamily: Qt.platform.os === "windows"
                                         ? "Segoe UI" : "Inter"

    // Spacing
    readonly property int spacingXS: 4
    readonly property int spacingS:  8
    readonly property int spacingM:  16
    readonly property int spacingL:  24
    readonly property int spacingXL: 40

    // Radius
    readonly property int radius:    6
    readonly property int radiusLg: 10
}

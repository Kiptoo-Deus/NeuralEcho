import QtQuick
import "."
Canvas {
    property var powerData: []
    property color hotColor:  "#FF6B35"
    property color coldColor: ThemeConstants.bgSecondary
    onPowerDataChanged: requestPaint()
    onPaint: {
        const ctx = getContext("2d")
        ctx.clearRect(0, 0, width, height)
        ctx.fillStyle = ThemeConstants.bgSecondary
        ctx.fillRect(0, 0, width, height)
        if (!powerData || powerData.length === 0) return
        const N  = powerData.length
        const bw = width / N
        for (let i = 0; i < N; ++i) {
            const v  = Math.max(0, Math.min(1, powerData[i]))
            const r  = Math.round(255 * v)
            const g  = Math.round(107 * v)
            const b  = Math.round(53  * (1 - v))
            ctx.fillStyle = `rgb(${r},${g},${b})`
            ctx.fillRect(i * bw, 0, bw, height)
        }
    }
}

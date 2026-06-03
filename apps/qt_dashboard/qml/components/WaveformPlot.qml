import QtQuick
import "."

/// Draws a scrolling waveform from a list of float samples.
Canvas {
    id: root
    property string label:       "Waveform"
    property var    samples:     []
    property color  waveColor:   ThemeConstants.accentBlue
    property real   lineWidth:   1.2
    property real   amplitudeScale: 0.4   // fraction of half-height

    // Repaint whenever samples change
    onSamplesChanged: requestPaint()

    onPaint: {
        const ctx = getContext("2d")
        ctx.clearRect(0, 0, width, height)

        // Background
        ctx.fillStyle = ThemeConstants.bgSecondary
        ctx.fillRect(0, 0, width, height)

        // Centre line
        ctx.strokeStyle = ThemeConstants.border
        ctx.lineWidth   = 0.5
        ctx.beginPath()
        ctx.moveTo(0, height / 2)
        ctx.lineTo(width, height / 2)
        ctx.stroke()

        if (!samples || samples.length < 2) {
            drawLabel(ctx)
            return
        }

        // Waveform
        ctx.strokeStyle = root.waveColor
        ctx.lineWidth   = root.lineWidth
        ctx.beginPath()

        const N  = samples.length
        const dx = width / (N - 1)
        const cy = height / 2
        const amp = cy * root.amplitudeScale

        ctx.moveTo(0, cy - samples[0] * amp)
        for (let i = 1; i < N; ++i)
            ctx.lineTo(i * dx, cy - samples[i] * amp)

        ctx.stroke()
        drawLabel(ctx)
    }

    function drawLabel(ctx) {
        ctx.fillStyle = ThemeConstants.textMuted
        ctx.font      = "11px " + ThemeConstants.fontFamily
        ctx.fillText(root.label, 8, 16)
    }
}

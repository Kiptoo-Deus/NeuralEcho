//! Spectral feature construction for NeuralEchoNet input.

/// Normalise a magnitude spectrum to unit maximum.
pub fn normalise_spectrum(spectrum: &mut [f32]) {
    let max = spectrum.iter().cloned().fold(0.0f32, f32::max);
    if max > 1e-8 {
        spectrum.iter_mut().for_each(|v| *v /= max);
    }
}

/// Stack three spectra into a flat [3 × freq_bins] feature vector.
pub fn stack_spectra(
    far:      &[f32],
    mic:      &[f32],
    residual: &[f32],
    out:      &mut Vec<f32>,
) {
    out.clear();
    out.extend_from_slice(far);
    out.extend_from_slice(mic);
    out.extend_from_slice(residual);
}

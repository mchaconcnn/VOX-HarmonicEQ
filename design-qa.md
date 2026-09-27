# VOX HarmonicEQ Design QA

- Source visual truth: `work/vox-vibrato-reference.png` for the plug-in shell and `work/regular-eq-reference.png` for the signal display
- Implementation screenshot: `work/vox-harmoniceq-regular-eq.png`
- Full-view comparison: `work/vox-ui-comparison-regular-eq.png`
- Focused EQ comparison: `work/regular-eq-focused-comparison.png`
- Source dimensions: 1461 × 1077 px VOX shell; 1158 × 347 px regular-EQ display
- Implementation dimensions: 1182 × 958 px, including the JUCE standalone host chrome
- Comparison normalization: source resized proportionally to 1300 × 958 px; implementation retained at native 1182 × 958 px; both placed in one 2482 × 958 px comparison image
- State: dark desktop plug-in UI, manual A3 target, +8 dB F0, +4.5 dB x2, -5 dB x3; standalone audio input muted

## Findings

No actionable P0, P1, or P2 differences remain.

The implementation preserves the reference's defining visual language: a black textured hardware chassis, inset silver frame, four corner screws, large condensed white VOX product title, subdued subtitle, recessed green-on-black display, green illuminated control accents, metallic rotary knobs, uppercase control labels, and a centered muted footer link.

The denser top control bank and two horizontal CRT panes are intentional product adaptations. HarmonicEQ has nine global controls and up to 41 automatable EQ bands, while the source VOX Vibrato design has three primary controls and one waveform. Separating signal analysis from gain editing keeps the conventional EQ response and live spectrum legible without visually competing with the 41 sliders.

## Required Fidelity Surfaces

- Fonts and typography: Avenir Next Condensed provides the narrow hardware-label character of the reference. The large `VOX HARMONIC EQ` title, subdued subtitle, uppercase section labels, and compact numeric readouts preserve the hierarchy without clipping.
- Spacing and layout rhythm: outer chassis margins, inset frames, header divider, control bank, two recessed displays, and footer divider follow the same vertical rhythm. All visible controls remain separated and readable at the verified 1180 × 900 editor size.
- Colors and visual tokens: near-black chassis, dark metal gradients, silver borders and pointers, green phosphor accents, dimmed inactive bands, and a translucent steel-blue EQ fill combine the VOX shell palette with the supplied regular-EQ reference.
- Image quality and asset fidelity: the reference contains no standalone photography or illustration. Material depth and control surfaces are rendered natively at the current scale, avoiding stretched or blurry raster assets.
- Copy and content: product naming reads `rco.cr > VOX HARMONIC EQ`; the subtitle describes the actual effect; section names map to the plug-in's behavior; the footer exposes `rco.cr/vox/harmonicEQ` as a functional link.
- Icons: the malformed external-link glyph from the first pass was removed. Hardware screws, knob pointers, and the combo disclosure remain sharp and aligned at native resolution.
- States and interactions: detected/manual pitch source, note entry, harmonic counts, bypass, disabled out-of-range bands, scrolling gain pane, and the website hyperlink remain functional native controls. Native QA verified that changing the manual target from G0 to A3 moved every response node and the complete curve; +8 dB at F0, +4.5 dB at x2, and -5 dB at x3 produced aligned peaks and a notch at 220, 440, and 660 Hz. The standalone capture has no spectrum because its input is explicitly muted; in Logic the plug-in transforms the processed output into a smoothed 2048-point FFT trace.
- Accessibility: controls retain JUCE accessibility roles, readable parameter names, keyboard/host automation access, and sufficient high-contrast labels. Disabled bands are communicated through both disabled control state and reduced opacity.

## Comparison History

### Pass 1

- P2: `VOX` appeared only in the small breadcrumb rather than in the primary product title, weakening the family resemblance and requested name.
- P2: the footer external-link arrow rendered as invalid glyphs in the native font.

Fixes:

- Changed the primary title to `VOX HARMONIC EQ` and reduced the breadcrumb to `rco.cr >`.
- Removed the unsupported glyph while retaining the full functional URL and tooltip.

Post-fix evidence: `work/vox-ui-comparison-final.png` shows the revised title hierarchy and clean footer link.

### Pass 2

- Added a 20 Hz–20 kHz logarithmic frequency scale behind the harmonic faders.
- Added a filled, glowing combined-response curve calculated from the same peaking-filter coefficients as the audio engine.
- Added live harmonic nodes, with the fundamental distinguished in white and upper/subharmonic nodes in phosphor green.
- Verified the graph in the native standalone UI at A3/220 Hz with non-zero gain values and confirmed that all curve features move with the target note.

### Pass 3

- P2: the applied-response curve and the gain sliders shared one visual field, so the signal graph competed with the editable controls.
- P2: there was no visual representation of the processed audio waveform.

Fixes:

- Split the lower interface into a dedicated `APPLIED EQ + WAVEFORM` pane and a separate `HARMONIC GAINS` pane.
- Added a dark-green time-domain trace sourced from the actual post-EQ/post-mix output through a lock-free decimated monitor.
- Increased the default editor height to 1180 × 900 so both panes remain readable without shrinking the VOX hardware controls.

Post-fix evidence: `work/vox-ui-comparison-split-pane.png` shows the clear two-pane hierarchy, the brighter applied-EQ response, and the uncluttered slider-only panel.

### Pass 4

- P2: the signal trace was a time-domain oscilloscope waveform, while the supplied regular-EQ reference uses a frequency-domain analyzer aligned to the EQ axis.
- P2: the applied response was a narrow glow around the zero line instead of the broad filled area expected from a conventional EQ display.

Fixes:

- Replaced the oscilloscope trace with a smoothed 2048-point Hann-windowed FFT of the actual post-EQ/post-mix audio.
- Mapped the analyzer and applied response to the same logarithmic 20 Hz–20 kHz axis.
- Filled the complete area beneath the EQ response with translucent steel blue, moved frequency labels onto the 0 dB axis, and retained dark green for the live spectrum.
- Renamed the pane `APPLIED EQ + SPECTRUM` so its frequency-domain behavior is explicit.

Post-fix evidence: `work/regular-eq-focused-comparison.png` shows the shared frequency axis, filled response area, fine grid, and distinct analyzer/response layers against the supplied reference.

## Focused Region Comparison

The 1158 × 347 regular-EQ reference was normalized to 714 × 214 and placed beside the 1088 × 214 implementation crop. The comparison confirms the conventional EQ structure: a logarithmic grid, 0 dB frequency labels, broad filled response area, and a separate frequency-analyzer trace. The implementation intentionally retains narrower harmonic peaks and the VOX green accent because those communicate this product's pitch-tracked band model.

## Follow-up Polish

- P3: a future asset pass could add a subtle photographic metal grain. The current procedural texture is clean, resolution-independent, and visually consistent, so this does not block the match.
- P3: Logic's host window will replace the standalone title/audio-device chrome visible in the implementation capture.

## Implementation Checklist

- [x] VOX family title and hierarchy
- [x] Black metal chassis and silver hardware treatment
- [x] Green recessed harmonic display
- [x] Live EQ response follows detected or automated pitch
- [x] Actual post-EQ spectrum appears with the response in dark green
- [x] Conventional filled EQ response and shared logarithmic frequency axis
- [x] Signal analysis and harmonic sliders use separate recessed panes
- [x] Functional native controls and automation bindings retained
- [x] Product URL added as a clickable link
- [x] Final side-by-side comparison completed

final result: passed

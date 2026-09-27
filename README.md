# rco.cr > VOX HarmonicEQ

VOX HarmonicEQ is a real-time, monophonic pitch-tracking equalizer for Logic Pro.
It detects the fundamental of an incoming signal and moves a bank of peaking
filters to the selected subharmonic, fundamental, and upper-harmonic
frequencies. The detected pitch can be replaced by a Logic-automatable manual
note and fine-tune value.

## Formats

- Audio Unit (`.component`) for Logic Pro
- Standalone application for development and audio-device testing

Logic Pro uses the Audio Unit build. The standalone build is a convenient test
host and is not required after installation.

## Controls

- **Pitch source:** Detected or Manual; automatable in Logic.
- **Manual note:** MIDI note 0–127, shown as note names; automatable.
- **Fine tune:** ±100 cents.
- **Glide:** Smoothing time for automatic or automated pitch changes.
- **Below:** Number of subharmonic EQ bands, from `F0/2` through `F0/9`.
- **Above:** Number of upper harmonic bands, from `2×F0` through `33×F0`.
- **Q:** Shared bandwidth of the harmonic filters.
- **Band gains:** Independent ±12 dB controls for all 41 bands.
- **Signal display:** A dedicated upper pane presents a conventional logarithmic
  EQ view. The applied response is a translucent filled shape with a bright
  boundary and harmonic nodes; the actual post-EQ audio appears as a dark-green
  FFT spectrum on the same 20 Hz–20 kHz frequency axis.
- **Harmonic gains:** A separate lower pane contains only the scrollable gain
  sliders and their live centre-frequency readouts.
- **Mix, output, and bypass:** Standard output controls.

Subharmonic bands EQ audio that already exists below the fundamental; they do
not synthesize new low-frequency content.

## Build and test

Requirements: macOS, Xcode, CMake, and internet access for CMake's pinned JUCE
9.0.2 dependency on the first configure.

```sh
./script/build_and_run.sh
```

This configures and builds the AU and standalone products, runs the DSP tests,
and launches the standalone application.

To install the AU for the current user:

```sh
./script/build_and_run.sh --install
```

Then validate and rescan it:

```sh
auval -v aufx Heq1 RCOc
```

In Logic Pro, open **Logic Pro > Settings > Plug-in Manager**, find **rco.cr >
VOX HarmonicEQ**, and choose **Reset & Rescan Selection** if necessary.

Product page: [rco.cr/vox/harmonicEQ](https://rco.cr/vox/harmonicEQ)

## Real-time design

All filters, detector buffers, and spectrum-monitor storage are allocated during preparation. Pitch
analysis uses a downsampled normalized-autocorrelation detector, confidence
gating, a 200 ms dropout hold, and parameter smoothing. The plug-in reports no
added latency because it analyzes preceding samples while the audio path remains
live; raw post-EQ samples reach the editor through a lock-free monitor and are
windowed into a 2048-point spectrum on the UI thread. Pitch-driven EQ movement
necessarily follows a new note by a short tracking interval.

# rco.cr > VOX HarmonicEQ 0.3.1

VOX HarmonicEQ is installed for the current user at:

`~/Library/Audio/Plug-Ins/Components/VOX HarmonicEQ.component`

In Logic Pro, insert it from **Audio FX > Audio Units > rco.cr > VOX HarmonicEQ**.
If it does not appear immediately, open **Logic Pro > Settings > Plug-in Manager**,
select VOX HarmonicEQ, and choose **Reset & Rescan Selection**.

Product page: [rco.cr/vox/harmonicEQ](https://rco.cr/vox/harmonicEQ)

The plug-in supports detected or manually automated notes, ±100-cent fine tuning,
up to eight EQ bands below the fundamental (`F0/2` through `F0/9`), the fundamental,
and up to 32 upper harmonics (`2×F0` through `33×F0`). All band gains and global
controls are available to Logic automation.

The upper signal pane uses a conventional EQ presentation: a translucent filled
20 Hz–20 kHz response shape with the actual post-EQ FFT spectrum in dark green.
Its curve and frequency nodes follow the detected pitch or manually automated
note. A separate lower pane contains only the harmonic gain sliders and
frequency readouts.

The supplied AU build is for Apple silicon (`arm64`) and is ad-hoc signed for local
use. Public distribution would require a Developer ID signature and notarization.

Validation command:

```sh
auval -v aufx Heq1 RCOc
```

This build passed Apple's full Audio Unit validation on September 26, 2026.

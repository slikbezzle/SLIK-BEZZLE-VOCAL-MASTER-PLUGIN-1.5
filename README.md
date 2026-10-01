# SLIK BEZZLE VOCAL MASTER V1.3

A JUCE/CMake VST3 + Standalone vocal processor project matching the Slik Bezzle black / white / neon green / turquoise concept. VST3 is the primary plugin format and is intended for DAWs that support VST3.

## Included
- VST3 plugin target
- Standalone app target for testing without a DAW
- Resizable custom GUI
- Vocal Master macro
- Auto Tune controls: on/off, tune amount, retune speed, humanize
- Auto Key controls: on/off, key and scale selectors
- Compression DSP
- De-esser-style high-frequency control
- Saturation / soft clipping
- Air high-shelf enhancement
- Limiter DSP
- Input/output gain
- Dry/wet mix
- DAW automation via AudioProcessorValueTreeState
- Plugin state saving/restoring

## Important
This build is a real VST3 project with an original pitch-correction foundation. It includes pitch detection, scale-aware note targeting, automatic key estimation, retune smoothing, humanize control, and a low-latency pitch-shift stage. It still needs extensive real-world vocal testing and optimization before commercial release.

## Build
1. Install a C++ compiler and CMake.
2. Download JUCE from https://juce.com/.
3. Configure with `-DJUCE_DIR=/path/to/JUCE`.
4. Build the `SlikBezzleVocalMaster` target.
5. The VST3 is produced by JUCE in the build output and can be copied/installed to your DAW's VST3 folder.

Example (macOS/Linux shell):

cmake -B build -DJUCE_DIR=/path/to/JUCE
cmake --build build --config Release

Windows PowerShell example:

cmake -B build -DJUCE_DIR="C:/JUCE"
cmake --build build --config Release

VST3 is the current VST format. VST2 is legacy and is not included in this project because the old VST2 SDK has separate licensing/distribution requirements.


## V1.2 Auto Tune DSP

This build adds a lightweight monophonic vocal pitch-correction foundation. It performs autocorrelation pitch detection, automatic key/scale estimation from recent pitch-class activity, scale-aware target-note selection, retune-speed smoothing, humanize control, and a low-latency granular pitch-shift stage.

Tune modes: Natural, Modern, and Hard Tune. Auto Key can be bypassed by selecting Key and Scale manually.

This is an original implementation and does not copy Antares Auto-Tune code or assets. For commercial release, test extensively on real vocals and across sample rates/buffer sizes before shipping.


## V1.3 UI Upgrade
- Premium black / white / neon green / turquoise interface
- Live pitch-tracking display with detected note and frequency
- Central Vocal Master control and animated waveform-style visualization
- Auto Tune, Auto Key, Natural / Modern / Hard Tune controls
- Input/output meters and processor strip
- Included `SLIK_BEZZLE_VOCAL_MASTER_MOCKUP.png` visual reference

## VST3 compatibility
The project builds a standard VST3 plug-in with JUCE. Any DAW must support VST3 to load it; no plugin format can guarantee compatibility with literally every DAW. On Windows, macOS, or Linux, build the VST3 target with the appropriate compiler and JUCE installation.

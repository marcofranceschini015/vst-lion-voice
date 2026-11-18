# Lion Voice

A vocal channel strip plugin (VST3 / AU) that performs a complete vocal mix in one place, styled like an SSL console:
brushed-steel faceplate, SSL-style knobs and a small Brescia lioness engraved in the steel.

> **Status:** MVP 1 in progress. The full interface is being built, but only the input/output gain and the
> subtractive EQ process audio. The other modules are visible but inactive.

## Signal chain

```
IN gain → Subtractive EQ → Leveler (LA-2A style) → Character comp (FET style)
        → Main EQ → Saturation → De-esser → OUT gain
```

The module order is fixed. Every module except IN/OUT gain has its own bypass switch, and a chain panel lights up
the modules that are currently active.

## Modules

| Module | What it does | MVP 1 |
|---|---|---|
| IN / OUT gain | Faders with peak/RMS meters and clip LEDs. Always active. | ✅ |
| Subtractive EQ | High-pass filter (12/18 dB/oct) and four cut bands: mud (200–300 Hz), boxiness (400–600 Hz), nasality (800 Hz–1 kHz), harshness (2–4 kHz). | ✅ |
| Leveler | Slow, smooth optical-style compression (LA-2A inspired) to even out the performance. | planned |
| Character comp | Fast FET-style compression (1176 inspired) to control peaks and add attitude. | planned |
| Main EQ | Body (Pultec-style low shelf), Presence (3–6 kHz) and Air (soft high shelf). | planned |
| Saturation | Harmonic colour. | planned |
| De-esser | Dynamic-EQ de-esser with a listen mode to find the sibilance. | planned |

## Requirements

- macOS (Apple Silicon or Intel)
- Xcode Command Line Tools (`xcode-select --install`)
- [CMake](https://cmake.org) ≥ 3.25 and [Ninja](https://ninja-build.org): `brew install cmake ninja`
- Optional: [pluginval](https://github.com/Tracktion/pluginval) for validation
- Optional editor setup: VS Code with the CMake Tools, clangd and CodeLLDB extensions

## Getting the code

JUCE is included as a git submodule, so clone recursively:

```sh
git clone --recursive https://github.com/marcofranceschini015/vst-lion-voice.git
cd vst-lion-voice
```

If you already cloned without `--recursive`:

```sh
git submodule update --init --recursive
```

## Building

| Task | Command |
|---|---|
| Configure (debug, native arch) | `cmake --preset debug` |
| Build | `cmake --build --preset debug` |
| Run unit tests | `ctest --preset debug` |
| Release build (universal arm64 + x86_64) | `cmake --preset release && cmake --build --preset release` |
| Validate the plugin | `scripts/validate.sh` |

In VS Code the CMake Tools extension picks up the presets automatically: build with `F7`, and start the standalone
app under the debugger with `F5`.

Every build copies the plugin to:

- `~/Library/Audio/Plug-Ins/VST3/Lion Voice.vst3`
- `~/Library/Audio/Plug-Ins/Components/Lion Voice.component` (Audio Unit)

A standalone app is also built, which is handy for quick tests without a DAW.

## Using it in a DAW

- **REAPER:** Options → Preferences → Plug-ins → VST → *Re-scan*, then insert *Lion Voice (Marco Franceschini)* on a vocal track.
- **Logic Pro / GarageBand:** the Audio Unit appears under *Audio Units → Marco Franceschini → Lion Voice*.
  If a new build does not show up, run `killall -9 AudioComponentRegistrar` and restart the host.

Suggested workflow: set the input gain so the vocal peaks around −12 to −10 dBFS, clean up with the subtractive EQ,
then compare with the plugin bypassed at matched loudness.

## Project layout

```
CMakeLists.txt, CMakePresets.json   build configuration
libs/JUCE/                          JUCE framework (git submodule)
src/                                plugin processor, editor, parameters
src/dsp/                            DSP code, independent of JUCE and the GUI
src/gui/                            look-and-feel, panels, meters, logo
tests/                              DSP unit tests (Catch2)
scripts/                            helper scripts (validation)
```

## Roadmap

1. **MVP 1:** complete interface, IN/OUT gain, subtractive EQ.
2. Leveler and character compressors with gain-reduction meters.
3. Main EQ, saturation and de-esser.
4. Later ideas: automatic input analysis, macro controls, an assistant that suggests a full mix.

Reverb, delay and stereo effects are out of scope and will live in a separate plugin.

## License

Lion Voice is free software, released under the **GNU Affero General Public License v3.0**. See [LICENSE](LICENSE).

It is built with [JUCE](https://juce.com), used under its AGPLv3 licence, and the
[VST3 SDK](https://github.com/steinbergmedia/vst3sdk) (MIT). VST is a registered trademark of Steinberg Media
Technologies GmbH.

## Author

Marco Franceschini

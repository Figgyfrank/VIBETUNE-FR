# VibeTune (auto-pitch VST3 for Ableton Live, Windows)

Trap-focused auto-tune with a live pitch display, 20 presets, tooltips on every control, a resizable window, and a Low Latency mode.

## Get the installer without installing a compiler
1. Make a free account at github.com and create a new repo.
2. Upload everything in this folder (including the hidden `.github` folder) to the repo.
3. Open the repo's **Actions** tab. The build starts by itself and takes about 10 minutes.
4. When it's green, open the run and download **VibeTune-Setup**. Unzip it and run `VibeTune-Setup.exe`.
   (Or download **VibeTune-VST3** if you only want the plugin file.)

## Use in Ableton Live
1. Live 11 or newer is needed for VST3.
2. Preferences -> Plug-ins -> turn on "Use VST3 System Folders" -> Rescan.
3. Plug-ins -> Audio Effects -> drop **VibeTune** on your vocal track.
4. Set **Key** to your beat's key, pick a preset, and hover over any knob for help.

## Low Latency mode
Cuts the delay from about 14.5 ms to about 8.5 ms. Voices below about 120 Hz pass through untuned in this mode. Switch it before you record, because it changes the latency Ableton compensates for.

## The 20 presets
Each preset sets scale, retune speed, amount, humanize, transpose, fine tune, mix and output. They never change your Key or Low Latency setting.

01 Hard Tune Classic, 02 Robot Snap, 03 Melodic Trap, 04 ATL Glide, 05 Future Haze, 06 Pentatonic Pain, 07 Dark Minor Mood, 08 Drill Cold, 09 Auto Crooner, 10 Mumble Melody, 11 Sing-Song Flow, 12 Ghost Vocal, 13 Chipmunk Lift, 14 Deep Demon, 15 Subtle Polish, 16 Natural Fix, 17 Wide Doubler, 18 Blues Cry, 19 Harmonic Drama, 20 Phrygian Menace

## Icons
`Assets/vibetune.ico` is the desktop and app icon. `Assets/setup.ico` is the installer icon (same art with a download badge). The installer adds a desktop shortcut.

## Developer notes
- `Source/DSP/AutoTuneEngine.h` is plain C++ with no JUCE dependency. `Tests/test_engine.cpp` checks it with synthetic voices: `g++ -O2 -std=c++17 Tests/test_engine.cpp -o t && ./t`.
- JUCE is AGPLv3 / commercial licensed. Fine for personal use. Check JUCE's licensing before selling or distributing.

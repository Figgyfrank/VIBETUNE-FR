#pragma once

namespace vt
{
// scale index: 0 Chromatic, 1 Major, 2 Minor, 3 Harmonic Minor, 4 Pentatonic Minor,
//              5 Pentatonic Major, 6 Dorian, 7 Phrygian, 8 Blues
// Presets never touch Key or Low Latency - set those for your song / session.
struct Preset
{
    const char* name;
    int   scale;
    float speedMs, amountPct, humanizePct, transposeSt, fineCents, mixPct, gainDb;
};

inline constexpr int kNumPresets = 20;

inline const Preset kPresets[kNumPresets] =
{
    //  name                    scale spd   amt   hum  trn  fine mix   gain
    { "01 Hard Tune Classic",     2,    0,  100,    0,   0,    0, 100,  0.0f },
    { "02 Robot Snap",            0,    0,  100,    0,   0,    0, 100,  0.0f },
    { "03 Melodic Trap",          2,   12,  100,   10,   0,    0, 100,  0.0f },
    { "04 ATL Glide",             4,   28,  100,   15,   0,    0, 100,  0.0f },
    { "05 Future Haze",           4,    6,  100,    5,   0,    0,  90,  0.0f },
    { "06 Pentatonic Pain",       4,   18,  100,   20,   0,    0, 100,  0.0f },
    { "07 Dark Minor Mood",       2,   22,   95,   25,   0,    0, 100,  0.0f },
    { "08 Drill Cold",            7,    8,  100,    0,   0,    0, 100,  0.0f },
    { "09 Auto Crooner",          2,   45,   90,   35,   0,    0, 100,  0.0f },
    { "10 Mumble Melody",         4,   35,  100,   30,   0,   -8,  85,  0.0f },
    { "11 Sing-Song Flow",        1,   20,  100,   20,   0,    0, 100,  0.0f },
    { "12 Ghost Vocal",           2,   70,   80,   40,   0,    0,  60,  0.0f },
    { "13 Chipmunk Lift",         4,   10,  100,   10,  12,    0, 100, -1.0f },
    { "14 Deep Demon",            2,   15,   85,   15,  -7,    0, 100,  0.0f },
    { "15 Subtle Polish",         0,   60,   70,   45,   0,    0, 100,  0.0f },
    { "16 Natural Fix",           0,  110,   55,   50,   0,    0, 100,  0.0f },
    { "17 Wide Doubler",          2,   20,  100,   15,   0,   14,  50,  0.0f },
    { "18 Blues Cry",             8,   25,  100,   20,   0,    0, 100,  0.0f },
    { "19 Harmonic Drama",        3,   10,  100,   10,   0,    0, 100,  0.0f },
    { "20 Phrygian Menace",       7,    4,  100,    5,  -5,    0, 100,  0.0f },
};
} // namespace vt

#pragma once

// VibeTune DSP core. Plain C++17, no JUCE dependency (so it can be unit-tested anywhere).
//
//  1. Pitch detection : YIN on a decimated mono copy of the input (cheap + accurate).
//  2. Scale snapping  : nearest allowed note, with hysteresis, humanize dead-zone,
//                       amount and retune-speed smoothing.
//  3. Pitch shifting  : two crossfaded delay-line taps that are always exactly ONE
//                       detected pitch period apart, so the two copies add in phase
//                       (no comb filtering) and the voice keeps its character.
//
//  Latency is fixed (reported to the host) and depends on the mode:
//    Normal      : lowest voiced pitch 70 Hz  -> about 14.5 ms @ 48 kHz
//    Low latency : lowest voiced pitch 120 Hz -> about  8.5 ms @ 48 kHz

#include <algorithm>
#include <array>
#include <atomic>
#include <cmath>
#include <vector>

namespace vt
{

constexpr double kPi = 3.14159265358979323846;

struct TuneParams
{
    std::array<bool, 12> allowed { true, true, true, true, true, true,
                                   true, true, true, true, true, true }; // C..B
    float speedMs        = 20.0f;  // retune speed, 0 = instant "hard tune"
    float amount         = 1.0f;   // 0..1 how much of the correction is applied
    float humanizeCents  = 0.0f;   // dead-zone: natural drift inside this is left alone
    float transposeSemis = 0.0f;   // transpose + fine tune, in semitones
    float mix            = 1.0f;   // 0 dry .. 1 wet
    float outGain        = 1.0f;   // linear
};

//==============================================================================
class PitchDetector
{
public:
    void allocate (double sampleRate, float lowestHz)
    {
        decim = std::max (1, (int) std::lround (sampleRate / 24000.0));
        fs = sampleRate / decim;
        maxTau = (int) std::ceil (fs / lowestHz) + 2;
        ring.assign ((size_t) (2 * maxTau + 8), 0.0f);
        frame.assign (ring.size(), 0.0f);
        df.assign ((size_t) maxTau + 2, 0.0f);
        cmnd.assign ((size_t) maxTau + 2, 0.0f);
        configure (lowestHz, 1000.0f);
    }

    // No allocation here, so it is safe to call from the audio thread.
    void configure (float lowestHz, float highestHz)
    {
        tauMax = std::min (maxTau, (int) std::ceil (fs / lowestHz) + 1);
        tauMin = std::max (2, (int) std::floor (fs / highestHz));
        win = tauMax;
        frameLen = win + tauMax;
        std::fill (ring.begin(), ring.end(), 0.0f);
        writePos = 0;
        acc = 0.0f;
        accCount = 0;
    }

    void push (float x)
    {
        acc += x;
        if (++accCount >= decim)
        {
            ring[(size_t) writePos] = acc / (float) decim;
            writePos = (writePos + 1) % frameLen;
            acc = 0.0f;
            accCount = 0;
        }
    }

    // Returns the fundamental in Hz, or 0 when the signal is not clearly pitched.
    float detect()
    {
        for (int i = 0; i < frameLen; ++i)
            frame[(size_t) i] = ring[(size_t) ((writePos + i) % frameLen)];

        // energy gate on the newest `win` samples
        double e = 0.0;
        for (int i = tauMax; i < frameLen; ++i)
            e += (double) frame[(size_t) i] * frame[(size_t) i];
        if (std::sqrt (e / win) < 0.0015)
            return 0.0f;

        const float* x = frame.data();

        for (int tau = 1; tau < tauMax; ++tau)
        {
            float d = 0.0f;
            for (int j = 0; j < win; ++j)
            {
                const float t = x[j] - x[j + tau];
                d += t * t;
            }
            df[(size_t) tau] = d;
        }

        cmnd[0] = 1.0f;
        float running = 0.0f;
        for (int tau = 1; tau < tauMax; ++tau)
        {
            running += df[(size_t) tau];
            cmnd[(size_t) tau] = running > 1.0e-12f ? df[(size_t) tau] * (float) tau / running : 1.0f;
        }

        const int last = tauMax - 2;
        int best = -1;

        for (int tau = tauMin; tau <= last; ++tau)
        {
            if (cmnd[(size_t) tau] < 0.15f)
            {
                while (tau + 1 <= last && cmnd[(size_t) tau + 1] < cmnd[(size_t) tau])
                    ++tau;
                best = tau;
                break;
            }
        }

        if (best < 0)
        {
            float lowest = 1.0e9f;
            for (int tau = tauMin; tau <= last; ++tau)
                if (cmnd[(size_t) tau] < lowest) { lowest = cmnd[(size_t) tau]; best = tau; }

            if (best < 0 || lowest > 0.3f)
                return 0.0f;
        }

        const float a = cmnd[(size_t) best - 1], b = cmnd[(size_t) best], c = cmnd[(size_t) best + 1];
        const float den = a - 2.0f * b + c;
        float shift = den != 0.0f ? 0.5f * (a - c) / den : 0.0f;
        shift = std::max (-1.0f, std::min (1.0f, shift));

        return (float) (fs / ((double) best + shift));
    }

private:
    int decim = 2, maxTau = 0, tauMin = 2, tauMax = 0, win = 0, frameLen = 0;
    int writePos = 0, accCount = 0;
    float acc = 0.0f;
    double fs = 24000.0;
    std::vector<float> ring, frame, df, cmnd;
};

//==============================================================================
class AutoTuneEngine
{
public:
    static constexpr float kFloorNormal = 70.0f;
    static constexpr float kFloorLow    = 120.0f;
    static constexpr float kCeil        = 1000.0f;

    static int latencyFor (double sr, bool lowLatency)
    {
        return (int) std::ceil (sr / (lowLatency ? kFloorLow : kFloorNormal)) + 8;
    }

    void prepare (double sr)
    {
        sampleRate = sr;
        int size = 1;
        while (size < 2 * latencyFor (sr, false) + 16)
            size <<= 1;
        mask = size - 1;

        for (auto& b : buf)
            b.assign ((size_t) size, 0.0f);

        detector.allocate (sr, kFloorNormal);

        ratioCoef = 1.0 - std::exp (-1.0 / (0.002 * sr));
        t0Coef    = 1.0 - std::exp (-1.0 / (0.015 * sr));
        vAtk      = 1.0 - std::exp (-1.0 / (0.003 * sr));
        vRel      = 1.0 - std::exp (-1.0 / (0.020 * sr));

        setLowLatency (false);
    }

    // Audio-thread safe (no allocation). Clears all state.
    void setLowLatency (bool ll)
    {
        lowLatency = ll;
        floorHz = ll ? kFloorLow : kFloorNormal;
        latency = latencyFor (sampleRate, ll);
        hop = ll ? 64 : 128;
        detector.configure (floorHz, kCeil);
        reset();
    }

    void reset()
    {
        for (auto& b : buf)
            std::fill (b.begin(), b.end(), 0.0f);

        w = 0;
        d1 = (double) latency;
        t0s = t0Target = std::min ((double) latency - 6.0, sampleRate / 200.0);
        ratio = ratioTarget = 1.0;
        shift = 0.0;
        vSm = 0.0;
        voicedTarget = 0.0;
        hopCount = 0;
        histCount = voicedRun = unvoicedRun = 0;
        lastTarget = -1;
        meterDetected.store (-1.0f);
        meterTarget.store (-1.0f);
    }

    int getLatencySamples() const { return latency; }
    float getDetectedMidi() const { return meterDetected.load(); } // -1 = unvoiced
    float getTargetMidi()   const { return meterTarget.load(); }   // -1 = none

    void process (float* const* ch, int numCh, int numSamples, const TuneParams& p)
    {
        numCh = std::max (1, std::min (numCh, 2));

        for (int n = 0; n < numSamples; ++n)
        {
            float mono = 0.0f;
            for (int c = 0; c < numCh; ++c)
                mono += ch[c][n];
            mono /= (float) numCh;
            detector.push (mono);

            w = (w + 1) & mask;
            for (int c = 0; c < numCh; ++c)
                buf[(size_t) c][(size_t) w] = ch[c][n];

            if (++hopCount >= hop)
            {
                hopCount = 0;
                updateControl (p);
            }

            ratio += ratioCoef * (ratioTarget - ratio);
            t0s   += t0Coef * (t0Target - t0s);
            vSm   += (voicedTarget > 0.5 ? vAtk : vRel) * (voicedTarget - vSm);

            // Sweep the read head. Pitch up (ratio > 1) = delay shrinking.
            const double halfW = t0s;
            const double lo = (double) latency - halfW, hi = (double) latency + halfW;
            d1 += 1.0 - ratio;
            while (d1 > hi) d1 -= 2.0 * halfW;
            while (d1 < lo) d1 += 2.0 * halfW;

            const double phase = (d1 - lo) / (2.0 * halfW);
            const float g1 = (float) (0.5 - 0.5 * std::cos (2.0 * kPi * phase));
            const float g2 = 1.0f - g1;
            const double d2 = phase < 0.5 ? d1 + halfW : d1 - halfW; // exactly one period apart

            for (int c = 0; c < numCh; ++c)
            {
                const auto& b = buf[(size_t) c];
                const float wet = g1 * readCubic (b, d1) + g2 * readCubic (b, d2);
                const float dry = b[(size_t) ((w - latency) & mask)];
                const float tuned = dry + (float) vSm * (wet - dry);
                ch[c][n] = (dry + p.mix * (tuned - dry)) * p.outGain;
            }
        }
    }

private:
    float readCubic (const std::vector<float>& b, double delay) const
    {
        const double pos = (double) w - delay;
        const double fl = std::floor (pos);
        const int i1 = (int) fl;
        const float f = (float) (pos - fl);

        const float ym1 = b[(size_t) ((i1 - 1) & mask)];
        const float y0  = b[(size_t) (i1 & mask)];
        const float y1  = b[(size_t) ((i1 + 1) & mask)];
        const float y2  = b[(size_t) ((i1 + 2) & mask)];

        const float c1 = 0.5f * (y1 - ym1);
        const float c2 = ym1 - 2.5f * y0 + 2.0f * y1 - 0.5f * y2;
        const float c3 = 0.5f * (y2 - ym1) + 1.5f * (y0 - y1);
        return ((c3 * f + c2) * f + c1) * f + y0;
    }

    static float median3 (float a, float b, float c)
    {
        return std::max (std::min (a, b), std::min (std::max (a, b), c));
    }

    int pickTarget (float m, const std::array<bool, 12>& allowed) const
    {
        bool any = false;
        for (bool a : allowed) any = any || a;
        if (! any)
            return (int) std::lround (m);

        const int base = (int) std::floor (m);
        int best = base;
        float bestD = 1.0e9f;

        for (int k = -7; k <= 8; ++k)
        {
            const int n = base + k;
            if (! allowed[(size_t) (((n % 12) + 12) % 12)])
                continue;
            const float d = std::fabs ((float) n - m);
            if (d < bestD) { bestD = d; best = n; }
        }

        // hysteresis: don't flip between two notes when sitting on the boundary
        if (lastTarget >= 0 && allowed[(size_t) (((lastTarget % 12) + 12) % 12)]
            && std::fabs ((float) lastTarget - m) <= bestD + 0.12f)
            return lastTarget;

        return best;
    }

    void updateControl (const TuneParams& p)
    {
        const float f = detector.detect();
        const double hopSec = (double) hop / sampleRate;

        if (f > 0.0f)
        {
            ++voicedRun;
            unvoicedRun = 0;

            const float midi = 69.0f + 12.0f * std::log2 (f / 440.0f);
            midiHist[2] = midiHist[1];
            midiHist[1] = midiHist[0];
            midiHist[0] = midi;
            if (histCount < 3) ++histCount;

            const float m = histCount >= 3 ? median3 (midiHist[0], midiHist[1], midiHist[2]) : midi;

            const double fm = 440.0 * std::exp2 ((double) (m - 69.0f) / 12.0);
            t0Target = std::max (sampleRate / (double) kCeil, std::min ((double) latency - 6.0, sampleRate / fm));

            const int target = pickTarget (m, p.allowed);
            lastTarget = target;

            float dev = (float) target - m;
            const float dz = p.humanizeCents * 0.01f;
            dev = std::fabs (dev) <= dz ? 0.0f : dev - std::copysign (dz, dev);
            dev *= p.amount;

            const double alpha = p.speedMs < 1.0f ? 1.0
                                                  : 1.0 - std::exp (-hopSec / ((double) p.speedMs * 0.001));
            shift += alpha * ((double) dev - shift);

            if (voicedRun >= 2) voicedTarget = 1.0;
            meterDetected.store (m);
            meterTarget.store ((float) target);
        }
        else
        {
            ++unvoicedRun;
            voicedRun = 0;
            histCount = 0;
            lastTarget = -1;
            if (unvoicedRun >= 2) voicedTarget = 0.0;
            shift *= std::exp (-hopSec / 0.04);
            meterDetected.store (-1.0f);
            meterTarget.store (-1.0f);
        }

        ratioTarget = std::exp2 (((double) shift + (double) p.transposeSemis) / 12.0);
    }

    double sampleRate = 48000.0;
    bool lowLatency = false;
    float floorHz = kFloorNormal;
    int latency = 700, hop = 128, hopCount = 0, mask = 0, w = 0;

    std::array<std::vector<float>, 2> buf;
    PitchDetector detector;

    double d1 = 0.0, t0s = 200.0, t0Target = 200.0;
    double ratio = 1.0, ratioTarget = 1.0, shift = 0.0;
    double vSm = 0.0, voicedTarget = 0.0;
    double ratioCoef = 0.01, t0Coef = 0.001, vAtk = 0.01, vRel = 0.001;

    float midiHist[3] { 0, 0, 0 };
    int histCount = 0, voicedRun = 0, unvoicedRun = 0, lastTarget = -1;

    std::atomic<float> meterDetected { -1.0f }, meterTarget { -1.0f };
};

} // namespace vt

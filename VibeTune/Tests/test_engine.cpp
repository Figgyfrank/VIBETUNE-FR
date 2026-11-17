// Build & run (any OS with a C++17 compiler):
//   g++ -O2 -std=c++17 test_engine.cpp -o test_engine && ./test_engine
#include "../Source/DSP/AutoTuneEngine.h"
#include <cstdio>
#include <functional>
#include <random>

using namespace vt;

static std::vector<float> makeVoice (double sr, double secs, std::function<double (double)> f0ofT)
{
    std::vector<float> x ((size_t) (sr * secs));
    double phase = 0.0;
    for (size_t n = 0; n < x.size(); ++n)
    {
        const double t = (double) n / sr;
        const double f = f0ofT (t);
        phase += f / sr;
        double s = 0.0;
        for (int k = 1; k <= 30 && f * k < sr * 0.45; ++k)
            s += std::sin (2.0 * kPi * k * phase) / (double) k;
        x[n] = (float) (0.25 * s);
    }
    return x;
}

static double measureHz (const std::vector<float>& x, size_t start, size_t len, double sr)
{
    const int tmin = (int) (sr / 1000.0), tmax = (int) (sr / 60.0);
    double bestV = -1e9; int bestT = tmin;
    std::vector<double> r ((size_t) tmax + 2, 0.0);
    for (int t = tmin; t <= tmax; ++t)
    {
        double s = 0.0, e1 = 0.0, e2 = 0.0;
        for (size_t i = start; i + (size_t) t < start + len; ++i)
        { s += x[i] * x[i + (size_t) t]; e1 += x[i] * x[i]; e2 += x[i + (size_t) t] * x[i + (size_t) t]; }
        r[(size_t) t] = s / std::sqrt (e1 * e2 + 1e-12);
        if (r[(size_t) t] > bestV * 1.0 && r[(size_t) t] > 0.0) { }
    }
    // first strong peak (avoid sub-harmonics)
    for (int t = tmin + 1; t < tmax; ++t)
        if (r[(size_t) t] > 0.9 && r[(size_t) t] >= r[(size_t) t - 1] && r[(size_t) t] >= r[(size_t) t + 1])
        { bestT = t; bestV = r[(size_t) t]; break; }
    const double a = r[(size_t) bestT - 1], b = r[(size_t) bestT], c = r[(size_t) bestT + 1];
    const double den = a - 2 * b + c;
    const double sh = den != 0 ? 0.5 * (a - c) / den : 0.0;
    return sr / ((double) bestT + sh);
}

static std::vector<float> run (const std::vector<float>& in, double sr, bool ll, const TuneParams& p, int block = 256)
{
    AutoTuneEngine eng;
    eng.prepare (sr);
    eng.setLowLatency (ll);
    std::vector<float> out = in;
    for (size_t pos = 0; pos < out.size(); pos += (size_t) block)
    {
        const int n = (int) std::min<size_t> ((size_t) block, out.size() - pos);
        float* ch[1] = { out.data() + pos };
        eng.process (ch, 1, n, p);
    }
    return out;
}

static double cents (double f, double ref) { return 1200.0 * std::log2 (f / ref); }

int main()
{
    int fails = 0;
    auto check = [&] (const char* name, bool ok, double v, const char* unit)
    { std::printf ("%-52s %s  (%.2f %s)\n", name, ok ? "PASS" : "FAIL", v, unit); if (! ok) ++fails; };

    for (double sr : { 44100.0, 48000.0 })
    for (bool ll : { false, true })
    {
        std::printf ("\n--- sr=%.0f  mode=%s  latency=%d samples (%.1f ms) ---\n", sr, ll ? "LOW-LATENCY" : "NORMAL",
                     AutoTuneEngine::latencyFor (sr, ll), 1000.0 * AutoTuneEngine::latencyFor (sr, ll) / sr);

        TuneParams hard; hard.speedMs = 0.0f; hard.amount = 1.0f; hard.humanizeCents = 0.0f;

        { // 225 Hz is ~+22 cents sharp of A3 (220 Hz): expect 220
            auto in = makeVoice (sr, 2.5, [] (double) { return 225.0; });
            auto out = run (in, sr, ll, hard);
            double f = measureHz (out, (size_t) (sr * 1.5), (size_t) (sr * 0.8), sr);
            check ("225 Hz -> snaps to A3 (220 Hz)", std::fabs (cents (f, 220.0)) < 8.0, cents (f, 220.0), "cents err");
        }
        { // 300 Hz ~ -? : D4=293.66, expect 293.66
            auto in = makeVoice (sr, 2.5, [] (double) { return 300.0; });
            auto out = run (in, sr, ll, hard);
            double f = measureHz (out, (size_t) (sr * 1.5), (size_t) (sr * 0.8), sr);
            check ("300 Hz -> snaps to D4 (293.66 Hz)", std::fabs (cents (f, 293.66)) < 8.0, cents (f, 293.66), "cents err");
        }
        { // C major: input between C#4 (277) and D4: 280 Hz should snap to D4 (293.66)
            TuneParams cmaj = hard;
            cmaj.allowed = { true, false, true, false, true, true, false, true, false, true, false, true };
            auto in = makeVoice (sr, 2.5, [] (double) { return 285.0; });
            auto out = run (in, sr, ll, cmaj);
            double f = measureHz (out, (size_t) (sr * 1.5), (size_t) (sr * 0.8), sr);
            check ("C major: 285 Hz -> D4 (293.66 Hz)", std::fabs (cents (f, 293.66)) < 10.0, cents (f, 293.66), "cents err");
        }
        { // vibrato +/-35 cents @5.5 Hz around 220 Hz: output should be nearly flat
            auto in = makeVoice (sr, 3.0, [] (double t) { return 220.0 * std::pow (2.0, 0.35 * std::sin (2 * kPi * 5.5 * t) / 12.0); });
            auto out = run (in, sr, ll, hard);
            double lo = 1e9, hi = -1e9;
            for (double t0 = 1.2; t0 < 2.6; t0 += 0.1)
            { double f = measureHz (out, (size_t) (sr * t0), (size_t) (sr * 0.1), sr); lo = std::min (lo, f); hi = std::max (hi, f); }
            check ("vibrato +/-35c flattened by hard tune (spread)", cents (hi, lo) < 30.0, cents (hi, lo), "cents p-p");
        }
        { // amount=0 / mix=0 must equal the input delayed by the reported latency
            TuneParams off = hard; off.amount = 0.0f;
            auto in = makeVoice (sr, 1.0, [] (double) { return 233.0; });
            auto out = run (in, sr, ll, off);
            const int L = AutoTuneEngine::latencyFor (sr, ll);
            double maxErr = 0.0;
            for (size_t n = (size_t) (sr * 0.5); n < in.size(); ++n)
                maxErr = std::max (maxErr, (double) std::fabs (out[n] - in[n - (size_t) L]));
            check ("amount=0 -> bit-accurate delayed passthrough", maxErr < 0.02, maxErr, "max abs err");
        }
        { // level must stay sane while tuning
            auto in = makeVoice (sr, 2.0, [] (double) { return 225.0; });
            auto out = run (in, sr, ll, hard);
            double ri = 0, ro = 0; size_t a = (size_t) (sr * 1.0);
            bool bad = false;
            for (size_t n = a; n < in.size(); ++n) { ri += in[n] * in[n]; ro += out[n] * out[n]; if (! std::isfinite (out[n]) || std::fabs (out[n]) > 2.0f) bad = true; }
            double dB = 10 * std::log10 (ro / ri);
            check ("output level within +/-1.5 dB of input, no NaN", ! bad && std::fabs (dB) < 1.5, dB, "dB");
        }
        { // noise (unvoiced) should pass through at about unity
            std::mt19937 rng (1); std::uniform_real_distribution<float> d (-0.2f, 0.2f);
            std::vector<float> in ((size_t) (sr * 1.0)); for (auto& s : in) s = d (rng);
            auto out = run (in, sr, ll, hard);
            double ri = 0, ro = 0;
            for (size_t n = (size_t) (sr * 0.5); n < in.size(); ++n) { ri += in[n] * in[n]; ro += out[n] * out[n]; }
            double dB = 10 * std::log10 (ro / ri);
            check ("noise passes ~unity", std::fabs (dB) < 1.5, dB, "dB");
        }
    }

    std::printf ("\n%s (%d failures)\n", fails == 0 ? "ALL TESTS PASSED" : "SOME TESTS FAILED", fails);
    return fails;
}

// SarangiEngine.h
// A self-contained (no JUCE) physical model of a Nepali sarangi.
//
//   * Bowed string: digital waveguide with a friction "bow table"
//     (after Smith / Cook's STK Bowed model).
//   * Meend: smooth pitch glides between legato notes.
//   * Gamak: vibrato that blooms in after each note starts.
//   * Taraf: sympathetic strings tuned to a Sa (tonic) scale that ring
//     along with whatever you play.
//   * Skin body: nasal formant filtering of the goat-skin resonator.
//   * Space: a small Freeverb-style room.
#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <vector>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace sarangi
{

// ---------------------------------------------------------------- helpers
struct FracDelay
{
    std::vector<float> buf;
    int writePos = 0;
    float delay = 1.0f;
    float last = 0.0f;

    void init (int size) { buf.assign ((size_t) size, 0.0f); writePos = 0; last = 0.0f; }
    void clear()         { std::fill (buf.begin(), buf.end(), 0.0f); last = 0.0f; }
    void setDelay (float d) { delay = std::clamp (d, 1.0f, (float) buf.size() - 3.0f); }

    float read() const
    {
        const int size = (int) buf.size();
        float rp = (float) writePos - delay;
        while (rp < 0.0f) rp += (float) size;
        const int i0 = (int) rp;
        const int i1 = (i0 + 1) % size;
        const float frac = rp - (float) i0;
        return buf[(size_t) i0] + frac * (buf[(size_t) i1] - buf[(size_t) i0]);
    }
    void write (float x)
    {
        buf[(size_t) writePos] = x;
        if (++writePos >= (int) buf.size()) writePos = 0;
    }
    float tick (float in) { last = read(); write (in); return last; }
};

struct OnePole
{
    float b0 = 1.0f, a1 = 0.0f, gain = 1.0f, y1 = 0.0f;
    void setPole (float p) { b0 = (p > 0.0f) ? 1.0f - p : 1.0f + p; a1 = -p; }
    float tick (float x) { y1 = gain * b0 * x - a1 * y1; return y1; }
    void clear() { y1 = 0.0f; }
};

struct Biquad
{
    double b0 = 1, b1 = 0, b2 = 0, a1 = 0, a2 = 0;
    double z1 = 0, z2 = 0;

    void clear() { z1 = z2 = 0; }
    float tick (float xin)
    {
        const double x = xin;
        const double y = b0 * x + z1;
        z1 = b1 * x - a1 * y + z2;
        z2 = b2 * x - a2 * y;
        return (float) y;
    }
    void setNorm (double nb0, double nb1, double nb2, double na0, double na1, double na2)
    {
        b0 = nb0 / na0; b1 = nb1 / na0; b2 = nb2 / na0; a1 = na1 / na0; a2 = na2 / na0;
    }
    void peaking (double fs, double f, double q, double dB)
    {
        const double A = std::pow (10.0, dB / 40.0), w = 2.0 * M_PI * f / fs;
        const double al = std::sin (w) / (2.0 * q), c = std::cos (w);
        setNorm (1 + al * A, -2 * c, 1 - al * A, 1 + al / A, -2 * c, 1 - al / A);
    }
    void highpass (double fs, double f, double q)
    {
        const double w = 2.0 * M_PI * f / fs, al = std::sin (w) / (2.0 * q), c = std::cos (w);
        setNorm ((1 + c) / 2, -(1 + c), (1 + c) / 2, 1 + al, -2 * c, 1 - al);
    }
    void lowpass (double fs, double f, double q)
    {
        const double w = 2.0 * M_PI * f / fs, al = std::sin (w) / (2.0 * q), c = std::cos (w);
        setNorm ((1 - c) / 2, 1 - c, (1 - c) / 2, 1 + al, -2 * c, 1 - al);
    }
};

// ------------------------------------------------------------- reverb
class SmallRoom
{
public:
    void prepare (double fs)
    {
        static const int combL[8] = { 1116, 1188, 1277, 1356, 1422, 1491, 1557, 1617 };
        static const int apL[4]   = { 556, 441, 341, 225 };
        const double s = fs / 44100.0;
        for (int ch = 0; ch < 2; ++ch)
        {
            const int spread = ch * 23;
            for (int i = 0; i < 8; ++i) { combs[ch][i].assign ((size_t) std::max (1, (int) ((combL[i] + spread) * s)), 0.0f); combPos[ch][i] = 0; combLp[ch][i] = 0; }
            for (int i = 0; i < 4; ++i) { aps[ch][i].assign ((size_t) std::max (1, (int) ((apL[i] + spread) * s)), 0.0f); apPos[ch][i] = 0; }
        }
    }
    void process (float in, float& outL, float& outR)
    {
        const float feedback = 0.84f, damp = 0.35f;
        const float x = in * 0.03f;
        float out[2];
        for (int ch = 0; ch < 2; ++ch)
        {
            float acc = 0.0f;
            for (int i = 0; i < 8; ++i)
            {
                auto& b = combs[ch][i];
                int& p = combPos[ch][i];
                const float y = b[(size_t) p];
                combLp[ch][i] = y * (1.0f - damp) + combLp[ch][i] * damp;
                b[(size_t) p] = x + combLp[ch][i] * feedback;
                if (++p >= (int) b.size()) p = 0;
                acc += y;
            }
            for (int i = 0; i < 4; ++i)
            {
                auto& b = aps[ch][i];
                int& p = apPos[ch][i];
                const float bo = b[(size_t) p];
                b[(size_t) p] = acc + bo * 0.5f;
                acc = bo - acc;
                if (++p >= (int) b.size()) p = 0;
            }
            out[ch] = acc;
        }
        outL = out[0];
        outR = out[1];
    }

private:
    std::vector<float> combs[2][8], aps[2][4];
    int combPos[2][8] {}, apPos[2][4] {};
    float combLp[2][8] {};
};

// -------------------------------------------------------------- engine
struct Params
{
    float bow   = 0.55f; // bow pressure / grit        0..1
    float meend = 0.45f; // glide time                 0..1
    float gamak = 0.35f; // vibrato depth              0..1
    float taraf = 0.50f; // sympathetic strings level  0..1
    float space = 0.30f; // room amount                0..1
    float volume = 0.75f;// output                     0..1
    int   sa = 0;        // tonic: 0 = C ... 11 = B
};

class Engine
{
public:
    void prepare (double sampleRate)
    {
        fs = (float) sampleRate;
        const int maxDelay = (int) (fs / 30.0f) + 8;
        neck.init (maxDelay);
        bridge.init (maxDelay);

        stringFilter.setPole (0.75f - (0.2f * 22050.0f / fs));
        stringFilter.gain = 0.95f;

        // nasal, skin-covered body
        body[0].highpass (fs, 160.0, 0.7);
        body[1].peaking  (fs, 520.0, 1.3, 5.0);
        body[2].peaking  (fs, 1350.0, 2.0, 4.0);
        body[3].peaking  (fs, 2900.0, 2.6, 7.5);
        body[4].lowpass  (fs, 7500.0, 0.7);

        for (auto& s : sym) s.d.init ((int) (fs / 40.0f) + 8);
        retuneSympathetic (-1);

        room.prepare (fs);
        reset();
    }

    void reset()
    {
        neck.clear(); bridge.clear(); stringFilter.clear();
        for (auto& b : body) b.clear();
        for (auto& s : sym) { s.d.clear(); s.lp = 0.0f; }
        heldNotes.clear();
        env = 0.0f; envTarget = 0.0f; bowDown = false;
        vibPhase = 0.0f; vibOnset = 0.0f;
        logFreq = logTarget = std::log (220.0f);
        level = 0.0f;
    }

    void setParams (const Params& p)
    {
        if (p.sa != params.sa) retuneSympathetic (p.sa);
        params = p;
    }

    void noteOn (int note, float velocity)
    {
        heldNotes.erase (std::remove (heldNotes.begin(), heldNotes.end(), note), heldNotes.end());
        const bool legato = ! heldNotes.empty();
        heldNotes.push_back (note);

        logTarget = std::log (midiToHz ((float) note));
        maxVelocity = 0.03f + 0.2f * std::clamp (velocity, 0.05f, 1.0f);

        if (! legato)
        {
            // A fresh bow stroke. If the string is silent, start right on pitch;
            // if it is still ringing, slide from where it was (meend into the note).
            if (env < 0.02f) logFreq = logTarget;
            vibOnset = 0.0f;
        }
        else
        {
            vibOnset *= 0.4f; // re-bloom the gamak a little on each new legato note
        }
        bowDown = true;
        envTarget = 1.0f;
    }

    void noteOff (int note)
    {
        heldNotes.erase (std::remove (heldNotes.begin(), heldNotes.end(), note), heldNotes.end());
        if (! heldNotes.empty())
            logTarget = std::log (midiToHz ((float) heldNotes.back()));
        else
            envTarget = 0.0f;
    }

    void allNotesOff() { heldNotes.clear(); envTarget = 0.0f; }

    // Renders numSamples of stereo audio (overwrites the buffers).
    void render (float* outL, float* outR, int numSamples)
    {
        const float glideSec = 0.012f + params.meend * params.meend * 0.55f;
        const float glideCoef = 1.0f - std::exp (-1.0f / (glideSec * fs));
        const float attackCoef  = 1.0f - std::exp (-1.0f / (0.07f * fs));
        const float releaseCoef = 1.0f - std::exp (-1.0f / (0.12f * fs));
        const float bowP = 0.15f + 0.7f * params.bow;
        const float slope = 5.0f - 4.0f * bowP;
        const float gamakDepthSemis = params.gamak * params.gamak * 0.9f;
        const float vibInc = 5.3f / fs;
        const float onsetInc = 1.0f / (0.45f * fs);
        const float symIn = 0.06f;
        const float symOut = 0.012f * params.taraf;
        const float outGain = 2.2f * params.volume * params.volume;

        for (int i = 0; i < numSamples; ++i)
        {
            // envelope (bow speed)
            env += (envTarget - env) * (envTarget > env ? attackCoef : releaseCoef);
            if (envTarget == 0.0f && env < 1.0e-4f) bowDown = false;

            // pitch: meend glide + gamak vibrato (+ tiny human drift)
            logFreq += (logTarget - logFreq) * glideCoef;
            vibPhase += vibInc;
            if (vibPhase >= 1.0f) vibPhase -= 1.0f;
            vibOnset = std::min (1.0f, vibOnset + onsetInc);
            const float vib = std::sin (2.0f * (float) M_PI * vibPhase) * gamakDepthSemis * vibOnset * vibOnset;
            drift += (noise() * 0.5f - drift) * 0.0005f;
            const float freq = std::exp (logFreq) * std::pow (2.0f, (vib + drift * 0.06f) / 12.0f);

            const float baseDelay = fs / freq - 4.0f;
            neck.setDelay (baseDelay * (1.0f - betaRatio));
            bridge.setDelay (baseDelay * betaRatio);

            // bowed waveguide
            const float bowNoise = 1.0f + noise() * (0.04f + 0.18f * params.bow);
            const float bowVelocity = maxVelocity * env * bowNoise;
            const float bridgeRefl = -stringFilter.tick (bridge.last);
            const float nutRefl = -neck.last;
            const float stringVel = bridgeRefl + nutRefl;
            const float dv = bowVelocity - stringVel;
            float newVel = 0.0f;
            if (bowDown) newVel = dv * bowTable (dv, slope);
            neck.tick (bridgeRefl + newVel);
            bridge.tick (nutRefl + newVel);

            // skin body
            float s = bridge.last;
            for (auto& b : body) s = b.tick (s);
            s *= 0.25f;

            // sympathetic (taraf) strings
            float symSum = 0.0f;
            for (auto& st : sym)
            {
                const float y = st.d.read();
                st.lp = 0.55f * y + 0.45f * st.lp;
                const float fb = st.lp * st.feedback;
                st.d.write (s * symIn + fb);
                symSum += y;
            }
            const float mono = s + symSum * symOut;

            float rl = 0.0f, rr = 0.0f;
            room.process (mono, rl, rr);
            const float wet = params.space * 1.2f;
            float l = (mono + rl * wet) * outGain;
            float r = (mono + rr * wet) * outGain;

            if (! std::isfinite (l) || ! std::isfinite (r)) { reset(); l = r = 0.0f; }
            l = softClip (l);
            r = softClip (r);

            outL[i] = l;
            outR[i] = r;
            const float a = std::fabs (l);
            level = std::max (a, level * 0.9995f);
        }
    }

    // For the UI: smoothed output level, current pitch and whether a note is held.
    float getLevel() const     { return level; }
    float getCurrentHz() const { return std::exp (logFreq); }
    bool isBowing() const      { return ! heldNotes.empty(); }

private:
    static float midiToHz (float n) { return 440.0f * std::pow (2.0f, (n - 69.0f) / 12.0f); }
    static float softClip (float x) { return std::tanh (x); }

    static float bowTable (float input, float slope)
    {
        float s = (input + 0.001f) * slope;
        s = std::fabs (s) + 0.75f;
        s = std::pow (s, -4.0f);
        return std::clamp (s, 0.01f, 0.98f);
    }

    float noise()
    {
        rng = rng * 1664525u + 1013904223u;
        return (float) (rng >> 8) * (2.0f / 16777216.0f) - 1.0f;
    }

    void retuneSympathetic (int saOverride)
    {
        const int sa = saOverride >= 0 ? saOverride : params.sa;
        // Sa Re Ga Ma Pa Dha Ni Sa' + low Pa + low Sa (Bilawal / major scale)
        static const int degrees[10] = { 0, 2, 4, 5, 7, 9, 11, 12, -5, -12 };
        for (size_t i = 0; i < sym.size(); ++i)
        {
            const float hz = midiToHz ((float) (60 + sa + degrees[i]));
            sym[i].d.setDelay (fs / hz);
            sym[i].feedback = 0.994f;
            sym[i].d.clear();
            sym[i].lp = 0.0f;
        }
    }

    struct SymString { FracDelay d; float lp = 0.0f; float feedback = 0.994f; };

    float fs = 44100.0f;
    Params params;
    FracDelay neck, bridge;
    OnePole stringFilter;
    std::array<Biquad, 5> body;
    std::array<SymString, 10> sym;
    SmallRoom room;

    std::vector<int> heldNotes;
    float env = 0.0f, envTarget = 0.0f, maxVelocity = 0.15f;
    bool bowDown = false;
    float logFreq = 0.0f, logTarget = 0.0f;
    float vibPhase = 0.0f, vibOnset = 0.0f, drift = 0.0f;
    float level = 0.0f;
    uint32_t rng = 22222u;
    static constexpr float betaRatio = 0.127236f;
};

} // namespace sarangi

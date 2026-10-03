// Offline test: renders a short sarangi phrase to a WAV file.
// Build: g++ -O2 -std=c++17 -I../Source render_demo.cpp -o render_demo
#include "SarangiEngine.h"
#include <cstdio>
#include <cstring>
#include <vector>

struct Ev { double t; int note; bool on; float vel; };

static void writeWav (const char* path, const std::vector<float>& L, const std::vector<float>& R, int fs)
{
    FILE* f = std::fopen (path, "wb");
    const uint32_t n = (uint32_t) L.size(), dataBytes = n * 4, riff = 36 + dataBytes, fmtLen = 16, rate = (uint32_t) fs, byteRate = rate * 4;
    const uint16_t pcm = 1, ch = 2, align = 4, bits = 16;
    std::fwrite ("RIFF", 1, 4, f); std::fwrite (&riff, 4, 1, f); std::fwrite ("WAVEfmt ", 1, 8, f);
    std::fwrite (&fmtLen, 4, 1, f); std::fwrite (&pcm, 2, 1, f); std::fwrite (&ch, 2, 1, f);
    std::fwrite (&rate, 4, 1, f); std::fwrite (&byteRate, 4, 1, f); std::fwrite (&align, 2, 1, f); std::fwrite (&bits, 2, 1, f);
    std::fwrite ("data", 1, 4, f); std::fwrite (&dataBytes, 4, 1, f);
    for (uint32_t i = 0; i < n; ++i)
    {
        int16_t s[2] = { (int16_t) (std::clamp (L[i], -1.0f, 1.0f) * 32767), (int16_t) (std::clamp (R[i], -1.0f, 1.0f) * 32767) };
        std::fwrite (s, 2, 2, f);
    }
    std::fclose (f);
}

int main (int argc, char** argv)
{
    const int fs = 48000;
    sarangi::Engine e;
    e.prepare (fs);
    sarangi::Params p;
    p.sa = 2; // D
    e.setParams (p);

    // Sa = D4 (62). A slow alaap-like phrase with legato overlaps (meend).
    const int Sa = 62;
    std::vector<Ev> ev;
    double t = 0.2;
    auto note = [&] (int deg, double dur, double overlap = 0.0, float vel = 0.8f)
    {
        ev.push_back ({ t, Sa + deg, true, vel });
        ev.push_back ({ t + dur + overlap, Sa + deg, false, 0 });
        t += dur;
    };
    note (0, 1.4, 0.08);  note (2, 0.7, 0.08); note (4, 0.7, 0.08); note (5, 0.5, 0.08);
    note (7, 1.8, 0.0);   t += 0.35;
    note (9, 0.6, 0.08);  note (7, 0.6, 0.08); note (5, 0.6, 0.08); note (4, 0.9, 0.08);
    note (2, 0.6, 0.08);  note (0, 2.4, 0.0);  t += 0.4;
    note (12, 0.9, 0.12, 1.0f); note (11, 0.5, 0.1); note (9, 0.5, 0.1); note (7, 2.6, 0.0);
    const double total = t + 3.5;

    std::sort (ev.begin(), ev.end(), [] (const Ev& a, const Ev& b) { return a.t < b.t || (a.t == b.t && ! a.on && b.on); });

    const int N = (int) (total * fs);
    std::vector<float> L ((size_t) N), R ((size_t) N);
    size_t k = 0;
    const int block = 64;
    for (int pos = 0; pos < N; pos += block)
    {
        while (k < ev.size() && ev[k].t * fs <= pos)
        {
            if (ev[k].on) e.noteOn (ev[k].note, ev[k].vel); else e.noteOff (ev[k].note);
            ++k;
        }
        const int n = std::min (block, N - pos);
        e.render (&L[(size_t) pos], &R[(size_t) pos], n);
    }

    double peak = 0, sum = 0;
    for (int i = 0; i < N; ++i) { peak = std::max (peak, (double) std::fabs (L[i])); sum += L[i] * L[i]; }
    std::printf ("peak %.3f  rms %.4f  (%.1f s)\n", peak, std::sqrt (sum / N), total);

    writeWav (argc > 1 ? argv[1] : "sarangi_demo.wav", L, R, fs);
    return 0;
}

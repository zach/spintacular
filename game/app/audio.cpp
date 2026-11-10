#include "audio.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <memory>
#include <vector>

#include <miniaudio.h>

namespace audio {

namespace {

constexpr int kRate = 44100;
constexpr float kTau = 6.28318530718f;
constexpr int kLayers = 7;

using Buf = std::vector<float>;

struct Noise {
    std::uint32_t s = 0x1234ABCD;
    float next() {
        s ^= s << 13;
        s ^= s >> 17;
        s ^= s << 5;
        return static_cast<float>(s & 0xFFFFFF) / 8388608.0f - 1.0f;
    }
};

float midi(float note) { return 440.0f * std::pow(2.0f, (note - 69.0f) / 12.0f); }

void ensure(Buf& b, float end_seconds) {
    std::size_t n = static_cast<std::size_t>(end_seconds * kRate) + 1;
    if (b.size() < n) b.resize(n, 0.0f);
}

// A decaying or sustained tone made of harmonics, added into the buffer at `at`.
// shape: amplitudes of harmonics 1.., attack/decay in seconds; sustain holds the level until `dur`.
void tone(Buf& b, float at, float dur, float freq, float amp, std::initializer_list<float> harmonics, float attack,
          float decay, bool sustain = false, float vibrato = 0, float glide_to = 0, bool wrap = false) {
    if (!wrap) ensure(b, at + dur);
    std::size_t start = static_cast<std::size_t>(at * kRate), n = static_cast<std::size_t>(dur * kRate);
    float phase = 0;
    for (std::size_t i = 0; i < n; ++i) {
        float t = static_cast<float>(i) / kRate;
        float f = glide_to > 0 ? freq + (glide_to - freq) * (t / dur) : freq;
        f *= 1.0f + vibrato * std::sin(kTau * 5.5f * t);
        phase += kTau * f / kRate;
        float env = std::min(1.0f, t / std::max(attack, 1e-4f));
        if (sustain)
            env *= std::min(1.0f, (dur - t) / 0.03f);
        else
            env *= std::exp(-t / decay);
        float v = 0;
        int h = 1;
        for (float a : harmonics) {
            if (f * h < kRate * 0.45f) v += a * std::sin(phase * h);
            ++h;
        }
        std::size_t k = start + i;
        if (wrap) k %= b.size();
        if (k < b.size()) b[k] += v * env * amp;
    }
}

// Filtered noise: a one-pole low-pass at `cutoff` Hz, high-passed when `high` is set.
void noise(Buf& b, float at, float dur, float amp, float cutoff, float attack, float decay, bool high = false,
           bool wrap = false, std::uint32_t seed = 7) {
    if (!wrap) ensure(b, at + dur);
    std::size_t start = static_cast<std::size_t>(at * kRate), n = static_cast<std::size_t>(dur * kRate);
    Noise rng;
    rng.s = seed * 2654435761u + 99;
    float lp = 0, k = 1.0f - std::exp(-kTau * cutoff / kRate);
    for (std::size_t i = 0; i < n; ++i) {
        float t = static_cast<float>(i) / kRate;
        float x = rng.next();
        lp += k * (x - lp);
        float v = high ? x - lp : lp;
        float env = std::min(1.0f, t / std::max(attack, 1e-4f)) * std::exp(-t / decay) * std::min(1.0f, (dur - t) / 0.004f);
        std::size_t j = start + i;
        if (wrap) j %= b.size();
        if (j < b.size()) b[j] += v * env * amp;
    }
}

void finish(Buf& b, float gain = 1.0f) {
    float peak = 0.001f;
    for (float v : b) peak = std::max(peak, std::fabs(v));
    float g = gain * 0.9f / peak;
    for (float& v : b) v *= g;
    // A short fade so no buffer ends on a click.
    std::size_t fade = std::min<std::size_t>(b.size(), 200);
    for (std::size_t i = 0; i < fade; ++i) b[b.size() - 1 - i] *= static_cast<float>(i) / fade;
}

const std::initializer_list<float> kBrass = {1.0f, 0.7f, 0.5f, 0.35f, 0.22f, 0.12f, 0.06f};
const std::initializer_list<float> kPipe = {1.0f, 0.0f, 0.42f, 0.0f, 0.16f, 0.0f, 0.06f};
const std::initializer_list<float> kReed = {1.0f, 0.55f, 0.4f, 0.3f, 0.2f, 0.12f};
const std::initializer_list<float> kPure = {1.0f};
const std::initializer_list<float> kBellish = {1.0f, 0.0f, 0.0f, 0.35f};

void bell(Buf& b, float at, float freq, float amp, float decay) {
    const float ratios[] = {1.0f, 2.76f, 5.4f, 8.93f};
    const float amps[] = {1.0f, 0.55f, 0.3f, 0.15f};
    for (int i = 0; i < 4; ++i) tone(b, at, decay * 3.2f, freq * ratios[i], amp * amps[i], kPure, 0.001f, decay / (1 + i * 0.6f));
}

Buf make_sfx(Sfx id) {
    Buf b;
    switch (id) {
    case Sfx::Click:
        tone(b, 0, 0.05f, 1250, 0.6f, kPure, 0.001f, 0.012f);
        noise(b, 0, 0.012f, 0.4f, 6000, 0.0005f, 0.004f, true);
        break;
    case Sfx::Refuse:
        tone(b, 0, 0.16f, 110, 0.6f, kReed, 0.004f, 0.09f);
        tone(b, 0, 0.16f, 117, 0.6f, kReed, 0.004f, 0.09f);
        break;
    case Sfx::Tick: // the flapper clicking past a peg
        noise(b, 0, 0.012f, 0.9f, 5000, 0.0003f, 0.003f, true);
        tone(b, 0, 0.05f, 820, 0.7f, {1.0f, 0.4f}, 0.0005f, 0.012f);
        tone(b, 0, 0.04f, 1730, 0.3f, kPure, 0.0005f, 0.006f);
        break;
    case Sfx::LandCargo: // a prize landing on the wagon: a bright wooden note
        tone(b, 0, 0.5f, 660, 0.8f, {1.0f, 0.0f, 0.0f, 0.4f}, 0.001f, 0.11f);
        tone(b, 0, 0.3f, 1320, 0.25f, kPure, 0.001f, 0.05f);
        noise(b, 0, 0.02f, 0.5f, 2500, 0.0005f, 0.006f);
        break;
    case Sfx::LandTarp:
        noise(b, 0, 0.3f, 0.9f, 700, 0.01f, 0.07f);
        noise(b, 0.05f, 0.2f, 0.4f, 2400, 0.02f, 0.05f, false, false, 11);
        break;
    case Sfx::LandCrate:
        tone(b, 0, 0.3f, 170, 0.9f, {1.0f, 0.5f, 0.2f}, 0.001f, 0.05f, false, 0, 120);
        noise(b, 0, 0.03f, 0.8f, 3000, 0.0005f, 0.01f);
        break;
    case Sfx::LandBlank:
        tone(b, 0, 0.12f, 300, 0.5f, {1.0f, 0.3f}, 0.001f, 0.03f);
        break;
    case Sfx::Save: // a cymbal catch and a gasp from the crowd
        noise(b, 0, 0.32f, 0.9f, 4500, 0.001f, 0.07f, true);
        tone(b, 0, 0.3f, 3150, 0.15f, kPure, 0.001f, 0.05f);
        noise(b, 0.12f, 0.6f, 0.55f, 1300, 0.22f, 0.16f, false, false, 21);
        noise(b, 0.12f, 0.6f, 0.3f, 3200, 0.25f, 0.14f, true, false, 23);
        break;
    case Sfx::Spoil: // a sad trombone, cut short
        tone(b, 0.00f, 0.34f, midi(58), 0.8f, kBrass, 0.03f, 1, true, 0.012f, midi(57.6f));
        tone(b, 0.36f, 0.36f, midi(57), 0.8f, kBrass, 0.03f, 1, true, 0.014f, midi(56.4f));
        tone(b, 0.74f, 0.22f, midi(56), 0.8f, kBrass, 0.03f, 1, true, 0.02f, midi(54.5f));
        break;
    case Sfx::QuotaBell:
        bell(b, 0, 1046, 0.8f, 0.42f);
        bell(b, 0.09f, 1568, 0.5f, 0.36f);
        break;
    case Sfx::Whistle: // a slide whistle, rising
        tone(b, 0, 0.2f, 700, 0.6f, {1.0f, 0.12f}, 0.02f, 1, true, 0.01f, 930);
        noise(b, 0, 0.2f, 0.06f, 5000, 0.02f, 0.3f, true);
        break;
    case Sfx::Step: // the ticket machine rattling out a number
        for (int i = 0; i < 3; ++i) noise(b, i * 0.022f, 0.012f, 0.7f, 5200, 0.0004f, 0.003f, true, false, 5 + i);
        tone(b, 0.05f, 0.22f, 880, 0.55f, kBellish, 0.001f, 0.06f);
        break;
    case Sfx::Total:
        bell(b, 0, 1318, 0.7f, 0.3f);
        bell(b, 0.07f, 1760, 0.7f, 0.34f);
        for (int i = 0; i < 6; ++i) noise(b, 0.02f + i * 0.03f, 0.02f, 0.3f, 7000, 0.0005f, 0.006f, true, false, 40 + i);
        break;
    case Sfx::FullTruck: {
        const float notes[] = {60, 64, 67, 72, 76, 79};
        for (int i = 0; i < 6; ++i) {
            float at = i * 0.085f;
            tone(b, at, i == 5 ? 0.7f : 0.14f, midi(notes[i]), 0.6f, kBrass, 0.012f, 1, true, i == 5 ? 0.008f : 0);
            tone(b, at, i == 5 ? 0.7f : 0.14f, midi(notes[i] - 12), 0.35f, kBrass, 0.012f, 1, true);
        }
        noise(b, 0.42f, 0.9f, 0.5f, 5000, 0.002f, 0.28f, true);
        break;
    }
    case Sfx::Buy:
        tone(b, 0, 0.25f, 2093, 0.5f, kPure, 0.0005f, 0.05f);
        tone(b, 0.06f, 0.3f, 2637, 0.5f, kPure, 0.0005f, 0.07f);
        noise(b, 0, 0.01f, 0.3f, 8000, 0.0003f, 0.003f, true);
        break;
    case Sfx::Sell:
        tone(b, 0, 0.2f, 2637, 0.45f, kPure, 0.0005f, 0.04f);
        tone(b, 0.06f, 0.3f, 1760, 0.45f, kPure, 0.0005f, 0.07f);
        break;
    case Sfx::Reroll:
        for (int i = 0; i < 6; ++i) {
            noise(b, i * 0.045f, 0.014f, 0.7f, 4800, 0.0004f, 0.004f, true, false, 60 + i);
            tone(b, i * 0.045f, 0.04f, 900 - i * 60.0f, 0.4f, kPure, 0.0005f, 0.01f);
        }
        break;
    case Sfx::RoundWon: {
        const float notes[] = {67, 72, 76, 79};
        for (int i = 0; i < 4; ++i) tone(b, i * 0.11f, i == 3 ? 0.55f : 0.13f, midi(notes[i]), 0.6f, kBrass, 0.012f, 1, true, i == 3 ? 0.008f : 0);
        bell(b, 0.33f, 1568, 0.35f, 0.3f);
        break;
    }
    case Sfx::RunWon: {
        const float notes[] = {60, 64, 67, 72, 67, 72, 76, 84};
        const float starts[] = {0, 0.13f, 0.26f, 0.39f, 0.65f, 0.78f, 0.91f, 1.04f};
        for (int i = 0; i < 8; ++i) {
            float dur = i == 3 ? 0.24f : i == 7 ? 1.1f : 0.12f;
            tone(b, starts[i], dur, midi(notes[i]), 0.55f, kBrass, 0.012f, 1, true, i == 7 ? 0.008f : 0);
            tone(b, starts[i], dur, midi(notes[i] - 8), 0.35f, kBrass, 0.012f, 1, true);
        }
        noise(b, 1.04f, 1.2f, 0.45f, 5000, 0.002f, 0.4f, true);
        bell(b, 1.04f, 2093, 0.3f, 0.5f);
        break;
    }
    case Sfx::RunLost: {
        const float notes[] = {58, 57, 56, 55};
        for (int i = 0; i < 4; ++i)
            tone(b, i * 0.36f, i == 3 ? 0.9f : 0.34f, midi(notes[i]), 0.75f, kBrass, 0.03f, 1, true, i == 3 ? 0.03f : 0.012f,
                 midi(notes[i] - (i == 3 ? 1.5f : 0.4f)));
        break;
    }
    case Sfx::Unlock: {
        const float notes[] = {84, 88, 91, 96, 100};
        for (int i = 0; i < 5; ++i) tone(b, i * 0.07f, 0.5f, midi(notes[i]), 0.5f, kBellish, 0.001f, 0.12f);
        break;
    }
    default: break;
    }
    if (b.empty()) b.resize(64, 0.0f);
    finish(b);
    return b;
}

// ---- the band: a 16-bar waltz in C, one loop per player
constexpr float kBeat = 0.4f;
constexpr int kBars = 16;
constexpr float kLoop = kBars * 3 * kBeat;

// Chord roots per bar (midi), and whether the bar is a dominant seventh.
const int kRoot[kBars] = {48, 48, 43, 43, 43, 43, 48, 48, 53, 53, 48, 48, 43, 43, 48, 48};
const bool kDom[kBars] = {false, false, true, true, true, true, false, false, false, false, false, false, true, true, false, false};
// The tune: three notes a bar; 0 holds the note before.
const int kTune[kBars][3] = {{76, 79, 84}, {83, 81, 79}, {74, 77, 83}, {81, 79, 77}, {74, 77, 83}, {81, 79, 77},
                             {76, 79, 84}, {79, 0, 0},   {81, 84, 89}, {88, 86, 84}, {79, 84, 88}, {86, 84, 79},
                             {77, 83, 86}, {86, 83, 79}, {84, 79, 76}, {72, 0, 0}};

void chord_notes(int bar, int out[3]) {
    int r = kRoot[bar] + 12;
    out[0] = r + (kDom[bar] ? 4 : 4);
    out[1] = r + 7;
    out[2] = r + (kDom[bar] ? 10 : 12);
}

Buf make_layer(int layer) {
    Buf b(static_cast<std::size_t>(kLoop * kRate), 0.0f);
    for (int bar = 0; bar < kBars; ++bar) {
        float t0 = bar * 3 * kBeat;
        int ch[3];
        chord_notes(bar, ch);
        switch (layer) {
        case 0: // the calliope carries the tune
            for (int n = 0; n < 3; ++n) {
                if (kTune[bar][n] == 0) continue;
                int len = 1;
                while (n + len < 3 && kTune[bar][n + len] == 0) ++len;
                tone(b, t0 + n * kBeat, len * kBeat * 0.94f, midi(static_cast<float>(kTune[bar][n])), 0.5f, kPipe, 0.025f, 1, true,
                     0.006f, 0, true);
                noise(b, t0 + n * kBeat, 0.05f, 0.05f, 3000, 0.005f, 0.02f, true, true, 100 + bar * 3 + n);
            }
            break;
        case 1: // tuba: the oom
            tone(b, t0, kBeat * 0.85f, midi(static_cast<float>(kRoot[bar] - 12 + (bar % 2 == 1 ? 7 : 0))), 0.9f, {1.0f, 0.6f, 0.3f, 0.12f},
                 0.02f, 0.3f, false, 0, 0, true);
            break;
        case 2: // accordion: the pah-pah
            for (int beat = 1; beat < 3; ++beat)
                for (int k = 0; k < 3; ++k)
                    tone(b, t0 + beat * kBeat, kBeat * 0.45f, midi(static_cast<float>(ch[k])), 0.3f, kReed, 0.008f, 1, true, 0, 0, true);
            break;
        case 3: // snare brushes on two and three, a tap on one
            noise(b, t0, 0.05f, 0.35f, 6000, 0.001f, 0.012f, true, true, 300 + bar);
            for (int beat = 1; beat < 3; ++beat) {
                noise(b, t0 + beat * kBeat, 0.11f, 0.6f, 3500, 0.001f, 0.035f, true, true, 310 + bar * 2 + beat);
                tone(b, t0 + beat * kBeat, 0.08f, 190, 0.25f, kPure, 0.001f, 0.025f, false, 0, 0, true);
            }
            break;
        case 4: // glockenspiel: the chord, broken into eighths
            for (int e = 0; e < 6; ++e) {
                const int order[6] = {0, 1, 2, 1, 2, 1};
                tone(b, t0 + e * kBeat * 0.5f, 0.5f, midi(static_cast<float>(ch[order[e]] + 24)), 0.28f, kBellish, 0.001f, 0.09f, false, 0,
                     0, true);
            }
            break;
        case 5: // brass: the tune a sixth below, held
            for (int n = 0; n < 3; ++n) {
                if (kTune[bar][n] == 0) continue;
                int len = 1;
                while (n + len < 3 && kTune[bar][n + len] == 0) ++len;
                int note = kTune[bar][n] - 12;
                // Snap to the nearest chord tone at or below, so the harmony stays in the chord.
                int best = note - 9;
                for (int k = 0; k < 3; ++k)
                    for (int oct = -12; oct <= 12; oct += 12) {
                        int c = ch[k] + oct;
                        if (c <= note - 3 && c > best) best = c;
                    }
                tone(b, t0 + n * kBeat, len * kBeat * 0.9f, midi(static_cast<float>(best)), 0.42f, kBrass, 0.03f, 1, true, 0.004f, 0, true);
            }
            break;
        case 6: // bass drum on one, a cymbal every fourth bar
            tone(b, t0, 0.22f, 95, 1.0f, kPure, 0.001f, 0.06f, false, 0, 48, true);
            noise(b, t0, 0.03f, 0.3f, 900, 0.0005f, 0.012f, false, true, 400 + bar);
            if (bar % 4 == 0) noise(b, t0, 1.1f, 0.4f, 5200, 0.002f, 0.3f, true, true, 500 + bar);
            break;
        default: break;
        }
    }
    float peak = 0.001f;
    for (float v : b) peak = std::max(peak, std::fabs(v));
    const float level[kLayers] = {0.5f, 0.6f, 0.38f, 0.33f, 0.3f, 0.36f, 0.55f};
    for (float& v : b) v *= level[layer] / peak;
    return b;
}

Buf make_roll() { // a snare roll, one second, looping
    Buf b(kRate, 0.0f);
    for (int i = 0; i < 28; ++i) {
        float at = i / 28.0f;
        noise(b, at, 0.034f, i % 2 ? 0.55f : 0.8f, 3800, 0.0008f, 0.011f, true, true, 700 + i);
        tone(b, at, 0.03f, 210, 0.2f, kPure, 0.0005f, 0.008f, false, 0, 0, true);
    }
    float peak = 0.001f;
    for (float v : b) peak = std::max(peak, std::fabs(v));
    for (float& v : b) v *= 0.8f / peak;
    return b;
}

struct Voice {
    ma_audio_buffer buffer;
    ma_sound sound;
    bool ok = false;
};

struct State {
    ma_engine* engine = nullptr;
    ma_sound_group sfx_group, music_group;
    bool groups = false;
    std::vector<Buf> data;                                // every buffer the voices play; never resized after init
    std::vector<std::vector<std::unique_ptr<Voice>>> sfx; // a few voices per effect
    std::vector<int> next;
    std::unique_ptr<Voice> layers[kLayers];
    std::unique_ptr<Voice> roll;
    int layer_target = -1;
    bool quiet = false;
    float risk = -1;
} s;

std::unique_ptr<Voice> make_voice(const Buf& data, ma_sound_group* group, bool loop) {
    auto v = std::make_unique<Voice>();
    ma_audio_buffer_config cfg = ma_audio_buffer_config_init(ma_format_f32, 1, data.size(), data.data(), nullptr);
    cfg.sampleRate = kRate;
    if (ma_audio_buffer_init(&cfg, &v->buffer) != MA_SUCCESS) return nullptr;
    if (ma_sound_init_from_data_source(s.engine, &v->buffer, MA_SOUND_FLAG_NO_SPATIALIZATION, group, &v->sound) != MA_SUCCESS) {
        ma_audio_buffer_uninit(&v->buffer);
        return nullptr;
    }
    ma_sound_set_looping(&v->sound, loop ? MA_TRUE : MA_FALSE);
    v->ok = true;
    return v;
}

void free_voice(std::unique_ptr<Voice>& v) {
    if (v && v->ok) {
        ma_sound_uninit(&v->sound);
        ma_audio_buffer_uninit(&v->buffer);
    }
    v.reset();
}

} // namespace

bool init(ma_engine* engine) {
    s.engine = engine;
    if (!engine) return false;
    if (ma_sound_group_init(engine, 0, nullptr, &s.sfx_group) != MA_SUCCESS) return false;
    if (ma_sound_group_init(engine, 0, nullptr, &s.music_group) != MA_SUCCESS) {
        ma_sound_group_uninit(&s.sfx_group);
        return false;
    }
    s.groups = true;
    const int count = static_cast<int>(Sfx::Count);
    s.data.reserve(count + kLayers + 1);
    s.sfx.resize(count);
    s.next.assign(count, 0);
    for (int i = 0; i < count; ++i) {
        s.data.push_back(make_sfx(static_cast<Sfx>(i)));
        int voices = static_cast<Sfx>(i) == Sfx::Tick ? 6 : 3;
        for (int v = 0; v < voices; ++v) s.sfx[i].push_back(make_voice(s.data.back(), &s.sfx_group, false));
    }
    // The band starts together and loops for good; players join by fading in.
    ma_uint64 start = ma_engine_get_time_in_pcm_frames(engine) + ma_engine_get_sample_rate(engine) / 4;
    for (int l = 0; l < kLayers; ++l) {
        s.data.push_back(make_layer(l));
        s.layers[l] = make_voice(s.data.back(), &s.music_group, true);
        if (s.layers[l]) {
            ma_sound_set_volume(&s.layers[l]->sound, 0.0f);
            ma_sound_set_start_time_in_pcm_frames(&s.layers[l]->sound, start);
            ma_sound_start(&s.layers[l]->sound);
        }
    }
    s.data.push_back(make_roll());
    s.roll = make_voice(s.data.back(), &s.sfx_group, true);
    if (s.roll) {
        ma_sound_set_volume(&s.roll->sound, 0.0f);
        ma_sound_start(&s.roll->sound);
    }
    return true;
}

void shutdown() {
    if (!s.engine) return;
    for (auto& voices : s.sfx)
        for (auto& v : voices) free_voice(v);
    for (auto& l : s.layers) free_voice(l);
    free_voice(s.roll);
    if (s.groups) {
        ma_sound_group_uninit(&s.sfx_group);
        ma_sound_group_uninit(&s.music_group);
    }
    s = State{};
}

void play(Sfx sfx, float volume, float pitch) {
    if (!s.engine) return;
    int i = static_cast<int>(sfx);
    if (i < 0 || i >= static_cast<int>(s.sfx.size()) || s.sfx[i].empty()) return;
    auto& v = s.sfx[i][s.next[i]++ % s.sfx[i].size()];
    if (!v) return;
    ma_sound_stop(&v->sound);
    ma_sound_seek_to_pcm_frame(&v->sound, 0);
    ma_sound_set_volume(&v->sound, volume);
    ma_sound_set_pitch(&v->sound, std::clamp(pitch, 0.25f, 4.0f));
    ma_sound_start(&v->sound);
}

void music(int layers, bool quiet) {
    if (!s.engine) return;
    layers = std::clamp(layers, 0, kLayers - 1);
    if (layers == s.layer_target && quiet == s.quiet) return;
    s.layer_target = layers;
    s.quiet = quiet;
    for (int l = 0; l < kLayers; ++l) {
        if (!s.layers[l]) continue;
        float target = l <= layers ? (quiet ? 0.55f : 1.0f) : 0.0f;
        ma_sound_set_fade_in_milliseconds(&s.layers[l]->sound, -1, target, l <= layers ? 350 : 700);
    }
}

void risk(float chance) {
    if (!s.engine || !s.roll) return;
    chance = std::clamp(chance, 0.0f, 1.0f);
    if (std::fabs(chance - s.risk) < 0.01f) return;
    s.risk = chance;
    ma_sound_set_fade_in_milliseconds(&s.roll->sound, -1, chance <= 0 ? 0.0f : 0.12f + 0.5f * chance, 120);
    ma_sound_set_pitch(&s.roll->sound, 0.85f + 0.6f * chance);
}

void volumes(float master, float music_volume, float sfx_volume) {
    if (!s.engine) return;
    ma_engine_set_volume(s.engine, master);
    ma_sound_group_set_volume(&s.music_group, music_volume * 0.5f);
    ma_sound_group_set_volume(&s.sfx_group, sfx_volume);
}

} // namespace audio

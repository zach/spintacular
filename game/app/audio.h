// Sound, on the composition's miniaudio engine. Every sound is synthesized
// at startup: the game ships no audio files. docs/design/art-and-audio.md.
#pragma once

struct ma_engine;

namespace audio {

enum class Sfx {
    Click, Refuse, Tick, LandCargo, LandTarp, LandCrate, LandBlank, Save, Spoil, QuotaBell, Whistle, Step, Total,
    FullTruck, Buy, Sell, Reroll, RoundWon, RunWon, RunLost, Unlock, Count
};

bool init(ma_engine* engine);
void shutdown();
void play(Sfx sfx, float volume = 1.0f, float pitch = 1.0f);
// The band: the calliope plays alone at 0, and one more player joins per layer, up to 6.
void music(int layers, bool quiet);
// The snare roll under a risky wheel: 0 is silent, 1 is the tightest roll.
void risk(float chance);
void volumes(float master, float music, float sfx);

} // namespace audio

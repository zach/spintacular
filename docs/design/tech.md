# Tech: how the game is built

Spintacular is a desktop PC game with a custom engine in C++, composed from libraries with Automap 0.1.0. This document sets the shape of the code and the libraries. The library facts come from `../research/01-design-foundations.md` §6; only miniaudio and the ECS choice were checked in depth there.

## Shape of the code

The game splits into two parts, with a hard line between them.

| Part | Holds | Depends on |
| --- | --- | --- |
| **Rules core** | Everything in `scoring.md`, `rounds.md` and `economy.md`: the bag, the deal, spins, loads, deliveries, the shop, the run. | The C++ standard library only. No window, no clock, no audio. |
| **Presentation** | Drawing, animation, sound, input, menus, saves. | The rules core, and the engine libraries below. |

Rules for the rules core:

- **Deterministic.** Given a seed and the player's choices, it produces the same run on every machine. It has separate random streams for the shop, the deal and the spins (`progression.md`).
- **Data-driven.** Every number and every piece of content loads from data tables, as in `prototypes/haul/tables.json`. Most balance changes are one number.
- **Steps, not results.** A delivery returns the list of steps that made its score, such as "+4 mult from Color Guard". The presentation plays those steps back, so what the player sees can't drift from what the rules did.
- **Headless.** The core runs without the presentation. The balance simulation and the session rubric (`session-rubric.md`) run against the real rules in CI, so a content change that breaks the difficulty curve fails a check.

The C++ core passes the same rubric as the Python prototype on the same tables, bot for bot within sampling noise, so `game/rules` is the reference for the rules. The prototype stays for design experiments.

## Libraries

From Automap 0.1.0's public map:

| Job | Library | Notes |
| --- | --- | --- |
| Window, input, GPU | SDL3 | One library for all three. A 2D game of wheels and cargo asks little of the GPU. |
| Shaders | SDL_shadercross | Turns the game's SPIR-V shaders into what each GPU backend takes. `tools/compile_shaders.py` makes the SPIR-V from HLSL. |
| Sound | miniaudio | The whole audio layer: wheel ticks, the quota tone, music in layers. |
| Images | stb_image | The window icon. The game's art is drawn from shapes, so it has no sprites. |
| Math | glm | |
| Debug and tuning panels | Dear ImGui | For live-editing the balance tables and inspecting a run. Not for the game's own UI. |
| Logging | spdlog | |
| Profiling | Tracy | |

Not needed: Jolt (3D physics), fastgltf (3D models). An ECS (flecs is the mapped one) is optional: the game's state is a bag, a few wheels and a truck, which plain structs hold comfortably. Start without one.

## Libraries from the private map

The 0.1.0 public map has no library for text or for data files. Both come from a private map in `automap-private/`, layered over the public one:

| Job | Library | Notes |
| --- | --- | --- |
| **Text and fonts** | stb_truetype | Bakes the two fonts into a glyph atlas at the window's scale. It has an allocation binding and a probe of its own, like stb_image's in the public map. |
| **Data files** | nlohmann/json | Balance tables, saved runs and the profile. Only the data layer (`game/data`) includes it. |

Tweening is a small in-house timeline in `game/app`: a queue of timed cues that the spin, the payout steps and the wagon all play through.

## Build order

1. **Rules core** in C++, checked against the prototype's results on the same seeds and tables.
2. **A playable greybox:** wheels, truck, the two buttons, and the step-by-step payout, with placeholder art.
3. **The shop and a full run.**
4. **Feel:** the spin, near misses, the quota line, sound (`presentation.md`).
5. **Progression:** rigs, stakes, unlocks (`progression.md`).

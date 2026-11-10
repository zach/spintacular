# Spintacular

A single-player roguelike for Windows about pressing your luck. You load a wagon one wheel-spin at a time; every spin makes the load worth more, and one bad spin spoils the lot. Between rounds you shop for wedges, wheels and items until your rig is a point factory.

The design is in [`docs/design/`](docs/design/README.md). The game is C++ with a custom engine composed from libraries with [Automap](https://github.com/zach/automap) 0.1.0, and it doubles as a proof of Automap: [`docs/automap/`](docs/automap/) records how that went.

## Get the game

Every push builds the Windows game. Open the newest run of the **Build** workflow on GitHub and download the `Spintacular-windows-x64` artifact: unzip it and run `spintacular.exe`. `README.txt` inside has the controls.

## Layout

| Path | What lives there |
| --- | --- |
| `game/rules/` | The rules core: the bag, the deal, spins, loads, deliveries, the shop, the run. C++ standard library only; deterministic from a seed. |
| `game/data/` | Balance tables, saved runs and the profile, as JSON. |
| `game/sim/` | The balance simulation: bots play the real rules and check the session rubric. |
| `game/app/` | The game: renderer, text, sound, screens, the tuning panel and the scripted demo. |
| `assets/` | `data/tables.json` (every number and piece of content), fonts, the icon. |
| `tests/` | Tests of the rules core, saves and the profile. |
| `automap.manifest.json`, `automap.lock.json`, `cmake/automap/`, `glue/`, `evidence/` | The Automap composition: which library fills each slot, the glue, and the evidence that it verifies. |
| `automap-private/`, `third_party/stb-truetype/` | A private map for the two libraries the public map lacks: stb_truetype (text) and nlohmann/json (data files). |
| `tools/compile_shaders.py` | Compiles the two HLSL shaders to the SPIR-V committed in `game/app/shaders/`. |
| `prototypes/`, `docs/research/` | The Python and browser prototypes the design was tuned on, and the research behind it. |
| `docs/automap/` | Field notes and a report on building with Automap. |

## Build

You need CMake 3.25+, Ninja, a C++20 compiler (MSVC on Windows, clang on Linux), vcpkg and the Automap CLI ([release `cli-v0.1.0`](https://github.com/zach/automap/releases/tag/cli-v0.1.0)).

```bash
automap verify --json
```

builds the composition in `build/automap/<platform>/`, runs the game headless for 120 frames, runs every probe, and writes `evidence/<platform>.json`.

```bash
cmake -S . -B build/release -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE=$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake
```

```bash
cmake --build build/release
```

builds the game, the simulation and the tests for release.

| Task | Command |
| --- | --- |
| Run the game | `build/release/spintacular` |
| Rules tests | `ctest --test-dir build/release --output-on-failure` |
| Balance rubric (fails if a check fails) | `build/release/spintacular-sim sim 1000` |
| Every rig, then the stakes | `build/release/spintacular-sim ladder 400` |
| Try a number | `build/release/spintacular-sim sim 500 round.tank=12` |
| Watch the game play itself, saving screenshots | `build/release/spintacular --script tour --shots shots` |

CI (`.github/workflows/build.yml`) does all of this on Windows and Linux on every push.

## License

MIT. The fonts in `assets/fonts/` are under the SIL Open Font License.

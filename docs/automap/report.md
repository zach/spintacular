# Building Spintacular with Automap 0.1.0: report

Spintacular is a complete single-player Windows game built on an engine composed with Automap: CLI `cli-v0.1.0`, map release `map-2026.11.07`. This report sorts what was learned by theme. `field-notes.md` beside it is the log it draws on, written as the work happened; numbers in brackets, such as [28], point to its entries.

## Verdict

Automap did its main job well. Eight libraries from the public map and two from a private map were composed, built and wired on Windows and Linux, and the composition reached `probed` on both platforms the first time CI ran it, with one line of the generated glue changed. I never read a vcpkg port, a CMake find-module or a library's build documentation.

It is weakest at everything after the testbed: turning the testbed into a game, changing a composition once it exists, private maps that touch the public map's ports, and shipping. Those cost most of the time spent on Automap, and three of them are bugs.

| Measure | Result |
| --- | --- |
| Libraries composed | 10: SDL3, Dear ImGui, SDL_shadercross, miniaudio, stb_image, GLM, spdlog (public map); stb_truetype, nlohmann/json (private map); Tracy, added and later removed |
| Platforms at `probed` | Windows x64 (MSVC) and Linux x64 (clang), in GitHub Actions |
| First full verification in CI | `probed` on both platforms at the first run |
| Glue Automap wrote | About 2,000 lines in 40 files |
| Glue I changed | 1 line (the window's title and size) |
| Glue I wrote | About 40 lines: stb_truetype's allocation binding and probe, in the private map |
| Game code on top | About 8,000 lines: rules core, data layer, balance simulation, presentation, tests |
| Automap commands used | `find`, `show`, `start`, `extend`, `verify`, `verify --seed`, `upgrade`, `eject` |
| Findings logged | 50, in `field-notes.md` |

## 1. Step by step

1. **Install.** Downloaded the release binary for Linux and checked its SHA-256. One file, no setup. [1–4]
2. **Look at the map.** `automap find` listed thirteen libraries; `automap show library sdl3` gave slots, hooks, lifecycle steps, GPU backend per platform and Linux system packages. [5–8]
3. **Start.** `automap start --with sdl3 --with dear-imgui:docking --platform windows-x64-msvc --platform linux-x64-clang` wrote the manifest, lock, vcpkg files, a CMake project, 30 glue files, an `AGENTS.md` section and the router skill in under a second, then verified. [9–15]
4. **First verification.** After installing the system packages the map names, the day-one composition built through vcpkg and reached `probed` on Linux in about 5½ minutes. [16]
5. **Extend.** `automap extend` for spdlog, GLM, stb_image, miniaudio, Tracy and SDL_shadercross, one at a time. Each took seconds to write and moved the manifest's choices by itself. [17–20]
6. **CI.** Wrote a workflow that downloads the released CLI and runs `automap verify --json` on Windows and Linux. The eight-library composition reached `probed` on both at the first run. [21–23]
7. **Private map.** Wrote `automap-private/` with two new slots (`text`, `data-files`), two libraries, their recipes, and an allocation binding with a probe and a seeded violation for stb_truetype. Five commits and three CLI errors later, `automap extend` took both. [24–30]
8. **Make it a game.** Renamed the manifest's executable, replaced the testbed's `CMakeLists.txt` and `main.cpp` with the game's, and wrote the game against the glue: SDL_GPU through the context, shaders through the shadercross pairing, sound on the miniaudio engine, files through the assets interface, logs and profile zones through the diagnostics facade. [31–38]
9. **Verify the game.** `automap verify` built the game with MSVC and clang, ran it headless as its smoke test, ran every probe inside it and reached `probed` on both platforms, after one failure on Windows caused by line endings. [39–41]
10. **Ship.** CI builds a release, runs the rules tests, the balance rubric and two unattended play-throughs, and packages the Windows build. Shipping turned up two problems with what the map builds, and Tracy came out of the composition. [42–45]
11. **For the record.** CI also runs `automap verify --seed` on the private map's probe and `automap eject` on a copy of the repo, then builds and runs the ejected game without Automap. [48–49]

## 2. Challenges and unexpected issues

**Bugs in Automap**

- **Two map libraries that share a vcpkg port make `vcpkg.json` invalid.** stb_truetype and stb_image both come from the `stb` port. The CLI wrote the port twice and vcpkg refused the manifest. The file is the CLI's, so the only way out was to describe stb_truetype as an in-repo library, which isn't true. [28]
- **An in-repo library's pin fails on a Windows checkout.** The lock pins a `path` library by its files' bytes. Git's CRLF conversion on Windows changes them, and `verify` stops with `LOCK_MISMATCH` before building. [39]
- **`upgrade` doesn't carry changed reference glue from a private map into the repo.** It reported "0 files updated" when the private recipe's build fragment had changed and my copy still matched the old one. [29]

**Things the map says that aren't so**

- **SDL3's default variant on Linux.** The map says IBus is off; the build has it, with dbus and nine more ports, because Dear ImGui's port asks for SDL3's default features. [14]
- **SDL_shadercross on Windows.** The notes say a game needs no DXC at run time; the DLL the port builds imports `dxcompiler.dll`, 18 MB that must ship. [42]

**Surprises**

- **The reference CI workflow didn't fit.** It runs macOS whatever the manifest targets, installs the CLI from `main`, keeps no build logs and no executable. [21]
- **Tracy as the map builds it can't ship.** It records from startup, keeps everything until a profiler connects, and listens on the network. A composition can't say "in debug builds only". [43]
- **Changing a composition is undocumented.** Re-resolving a changed private library and removing a library both go through `automap upgrade`, which the CLI's usage text doesn't list. [3, 29, 44]

**The sandbox, not Automap**

- This build's cloud sandbox blocks GitHub archive and commit-patch downloads, which vcpkg depends on. A vcpkg asset-source script got the day-one composition building locally; I stopped extending it when the sandbox's policy checks declined, and verified everything after that in CI. Agent sandboxes often have this restriction, so it's worth Automap's attention even though it isn't Automap's fault. [13, 18]

## 3. Effective and ineffective interactions

**Effective**

- **`start` and `extend`.** Fast, complete, and they left the repo consistent even when the build then failed. This is the product working as designed. [9, 17, 20]
- **`--json` results.** `ok`, `level`, a one-line `summary` and failures with codes were all I read to decide what to do next. A failed build said `none`, never more than happened. [11]
- **Failure messages that name the fix.** `MANIFEST_INVALID` told me to commit the private map; `LOCK_MISMATCH` named the library, the directory and `automap upgrade`; `verify --platform` on the wrong machine said where to run it. [26, 39, 44, 47]
- **`show library` and `show recipe`.** Facts, hooks and adaptation notes in one place, layered across public and private maps. [7]
- **The facades.** Game code logs, profiles, allocates and reads files through `am::diag`, `am::alloc` and `am::assets`, never through spdlog or Tracy. Taking Tracy out changed no game code. [31, 45]
- **Contracts as a writing guide.** The allocation contract was clear enough to bind a library the map doesn't know, with a probe and a seeded violation, in about 40 lines. [30]
- **The released binaries in CI.** One download and a checksum on each runner. [23]
- **`verify --seed`.** One command showed that the probe I wrote for stb_truetype catches the violation it's meant to. [48]
- **`eject`.** The ejected repo built and ran with plain CMake and vcpkg; leaving is as cheap as promised. [49]

**Ineffective**

- **`find --json`.** About 600 lines for thirteen libraries, almost all of it evidence rows that say the same thing, and no slots. [6]
- **`find <query>`.** Found `audio`, `json` and `physics`; found nothing for `text` or `font`, though a library fills the `text` slot. [46]
- **The build log as the only detail.** Six `extend`s in a row failed on the same blocked download; each result pointed at the same log file, which the next run overwrote. [19]
- **The router skill past the testbed.** It covers start, extend, upgrade and verify. It has nothing on making the testbed a game, on private maps, on removing a library, on shaders, or on release builds. [15, 25, 35, 38]
- **The Dear ImGui pairing's reference glue.** Its begin-frame and end-frame own event polling and the render pass, so a game can't use them and writes the frame loop again. [33]
- **Iterating on a private map.** Every change needs a commit before the CLI reads it. [27]

## 4. Missing abilities, features and consistency

**Abilities**

- **Remove a library.** No command; done by editing the manifest and running `upgrade`, with the library's files left behind. [44]
- **Re-resolve after a map change.** No command named for it; `upgrade` does it. [29]
- **Choices per build type.** Profiling in debug builds and not in release can't be expressed. [43]
- **Shader compilation.** The map has the run-time half (SPIR-V to each backend) and not the build-time half (source to SPIR-V). [35]
- **A release build and packaging.** `verify` builds Debug, one target. Which DLLs to ship is left to the developer, and one of them is a surprise. [38, 42]
- **Network-restricted builds.** `fetch` and `vendor` exist for offline work, but nothing helps when the first download is the problem. [13]
- **Version and help.** No `--version`, no `help`, no per-command help. [2]

**Map coverage**

- **Text and fonts, and data files:** nothing in the public map. Both are things nearly every game needs before it needs 3D physics. [24]
- **A shippable profiler variant:** on-demand and localhost-only. [43]

**Consistency**

- **Usage text and documentation:** `upgrade` is documented everywhere but the CLI's own usage. [3]
- **README and release:** the README says `go install`; the release ships binaries. [1]
- **Reference workflow and manifest:** the workflow's platform matrix ignores the manifest's `platforms`. [21]
- **Reference workflow and lock:** it installs the CLI from `main`, whatever CLI wrote the composition. [21]
- **Map facts and builds:** two facts about variants don't match what vcpkg builds. [14, 42]
- **`find` across map layers:** a query matches public slots but not a private one. [46]
- **System dependencies:** a fact in prose, where every other fact is structured. [8]
- **`.gitignore`:** `verify` writes `build/automap/`, and `start` doesn't ignore it. [37]
- **Duplicate failures:** `LOCK_MISMATCH(stb-truetype)` printed twice in one result. [29]
- **After `eject`:** the `AGENTS.md` section and the router skill stay, still pointing agents at a CLI and manifest that are gone. [49]
- **`verify --seed`'s result** reports `level: smoke`, which isn't the composition's level. [48]

## 5. Other opportunities

In rough order of how much they would have saved:

1. **Merge vcpkg ports by name** when two libraries share one, and take the union of their features. It unblocks the commonest private-map case: one more header from a port the map already uses.
2. **Hash in-repo libraries with line endings normalized,** or from git's committed content.
3. **A "from testbed to game" section in the router skill:** rename the executable in the manifest, keep `--frames` and `--probe`, put the executable in the build root, where assets go, how to do a release build, what to ship.
4. **`automap remove <library>` and `automap resolve`,** or document `upgrade` as both, list it in the usage, and delete a removed library's untouched files.
5. **Carry the first error of a failed build in the result,** and report a failed download as `FETCH_FAILED` with its URL from `verify` too.
6. **A private-map guide and `automap show` templates:** "copy this recipe as a start" for a library like one the map has.
7. **Read a private map from the working tree,** warning that the lock can't pin it until it's committed.
8. **Split the Dear ImGui pairing glue** into process-event, new-frame and render-into-this-pass, so a game can use it unchanged.
9. **Take the window's title and size from the manifest** or an argument to SDL3's setup, so the first edit every game makes doesn't diverge the glue.
10. **Per-configuration choices,** at least `diagnostics.profiler`, and an on-demand, localhost-only Tracy variant with a probe that fits it.
11. **A shader build step** in SDL_shadercross's build fragment, using the `dxc` or `shadercross` tool vcpkg already installs.
12. **A summary mode for `find --json`:** one object per library with its slots, license class and worst evidence state per platform.
13. **Make the reference workflow follow the manifest and the lock:** platforms from one, CLI version from the other; upload the build logs; offer a hook to keep the built executable.
14. **Structured system dependencies,** and a check in `verify` that names the missing package instead of a failed port build.
15. **Add text, fonts and data files to the public map.** stb_truetype and nlohmann/json from this repo's private map are a start.
16. **A recipe for restricted networks:** vcpkg asset sources, documented, with `automap fetch` as the way to fill a cache somewhere the network is open.
17. **Check each variant's facts against vcpkg's install plan** in Automap's CI, so "IBus is off" and "no DXC at run time" can't drift from what is built.

# Automap field notes

A running log of building Spintacular with Automap 0.1.0 (CLI `cli-v0.1.0`, map release `map-2026.11.07`), written as the work happened. Spintacular is a proof for Automap, so every step with the CLI, the map and the glue is recorded here, with what worked and what didn't. `report.md` beside this file sorts the findings by theme.

Each entry is marked:

- **OK** worked as documented.
- **FRICTION** worked, but cost time or needed a guess.
- **GAP** something Automap doesn't do, or does inconsistently.
- **ENV** a problem of this build's sandbox, not of Automap; kept because other agent sandboxes will hit it.

The build ran in a Linux cloud container (Ubuntu 24.04, clang 18, CMake 3.28, 4 cores) driven by a coding agent, with Windows builds on GitHub Actions.

## 1. Installing the CLI

1. **FRICTION: the README's install line isn't the release.** The README says `go install github.com/zach/automap/cmd/automap@latest`. The release `cli-v0.1.0` ships static binaries (`automap-0.1.0-linux-amd64`, `-windows-amd64.exe`, `-darwin-arm64`) and a `.sha256` file, which is the better route for a game developer with no Go toolchain, but neither the README nor the router skill mentions them. I downloaded the Linux binary from the release and checked its SHA-256 by hand.
2. **GAP: no `--version`, `version`, `help` or `<command> --help`.** Each prints `unknown command` and the full usage, with exit status 2. The version is only visible as `cli_version` inside a `--json` result. An agent checking what it installed has to run `automap find --json` to learn it.
3. **GAP: the usage text leaves out `upgrade`.** `automap` with no arguments lists nine commands. The README, the spec and the router skill all document `upgrade`, and the usage doesn't.
4. **OK: no setup.** The binary needed nothing else: the first `find` fetched the newest map release and its evidence into `~/.cache/automap/`.

## 2. Looking at the map

5. **OK: `automap find` (human output)** is one readable line: 13 libraries and their versions.
6. **FRICTION: `automap find --json` is about 600 lines for 13 libraries.** Nearly all of it is evidence: one object per binding per platform (three platforms), every one `probed` and `current`. The facts I wanted first, which slots each library fills and what it is, aren't in the entries at all; I had to run `show library` for each. For an agent this is context spent on nothing. A summary per library (`slots`, `license`, worst evidence state per platform) with evidence detail left to `show` would serve the job better.
7. **OK: `automap show library sdl3 --json`** is thorough: slots, license class, contribution policy, pins with content hashes, per-variant vcpkg features, allocation and diagnostics hooks, lifecycle steps, GPU backend per platform and the headless environment. This is the information I'd otherwise dig out of headers and docs.
8. **FRICTION: system dependencies are prose.** `facts.platform.system_dependencies["linux-x64-clang"]` is one English sentence naming apt packages for X11, Wayland, autotools and a Vulkan driver. It was accurate (I installed exactly those and the build went through), but it can't be fed to `apt-get`, and neither `start` nor `verify` checks for them or mentions them. I only found the sentence by walking the JSON paths of `show`.

## 3. `automap start`

9. **OK: `start` wrote a coherent composition in under a second:** manifest, lock, vcpkg files, a CMake project, 30 glue files, the `AGENTS.md` section and the router skill. The `AGENTS.md` section is short and right: where the manifest and lock are, the slots, the build, test and run commands.
10. **OK: `--platform`** let me name Windows and Linux only; the manifest records it.
11. **OK: the failure result is honest and useful.** In a trial directory `start` built, failed, and said so: `ok: false`, `level: none`, `BUILD_FAILED` with the log as `details_file`, exit status 1. The evidence file it wrote marks every rule `unverifiable` with the reason. Nothing claimed more than happened.
12. **FRICTION: the summary of a failed start reads oddly:** "the composition is none on linux-x64-clang: BUILD_FAILED".
13. **ENV: source downloads were blocked.** This sandbox's proxy refuses GitHub's `/archive/` URLs (403), which is where vcpkg gets most sources. Registries (git) and release assets work. Automap has no setting for this. I got through with vcpkg's own asset-source hook, `X_VCPKG_ASSET_SOURCES=x-script,...`, and a 20-line script that rebuilds each archive with `git fetch` and `git archive | gzip -n`, which reproduces GitHub's bytes, so vcpkg's SHA-512 check still passes. The Automap handoff notes say "builds of public-map libraries run for real only in CI" in such sandboxes; with this script they run locally. It would be worth shipping as a documented recipe, since agent sandboxes commonly have this restriction.
14. **GAP: `sdl3`'s variant description and what vcpkg builds disagree.** The map says the default variant has "IBus input methods … off" (`vcpkg_default_features: false`). The install plan on Linux was `sdl3[core,dbus,ibus,vulkan,wayland,x11]`, which pulls in dbus, libsystemd, libmount, libcap, libxcrypt, lz4, zstd, liblzma, expat and gperf. Dear ImGui's port depends on `sdl3` with its default features, which switches them back on. That roughly doubles the cold build of the day-one composition on Linux, and the map's fact about the variant isn't true of the composition.
15. **GAP: the router skill doesn't say how a testbed becomes a game.** `start` writes a `testbed` target with a `main.cpp` that owns the frame loop, and the manifest records `executable: {target: "testbed", sources: ["main.cpp"]}`. Nothing in the skill, the README or `AGENTS.md` says what the developer may change: whether the target can be renamed, whether sources can move, or that the executable must keep honouring `--frames <n>` and `--probe <file>` for `verify` to work. I learned the protocol by reading the generated `main.cpp` and SPEC §5.6.

<!-- automap:begin -->
## Automap

This engine is an Automap composition. Before changing any glue, read the router skill (`.claude/skills/automap/SKILL.md`) and go through the `automap` CLI.

- Manifest: `automap.manifest.json`. Lock: `automap.lock.json`, which only the CLI writes.
- Slots: audio = miniaudio, data-files = nlohmann-json, gpu = sdl3, image-decode = stb-image, input = sdl3, logging = spdlog, math = glm, profiling = tracy, shader-compiler = sdl-shadercross, text = stb-truetype, tools-ui = dear-imgui, windowing = sdl3.
- Build: `cmake -S . -B build -G Ninja -DCMAKE_TOOLCHAIN_FILE=$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake && cmake --build build`
- Test: `automap verify --json`
- Run: `./build/testbed`
<!-- automap:end -->

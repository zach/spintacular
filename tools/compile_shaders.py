#!/usr/bin/env python3
"""Compiles the game's HLSL shaders to SPIR-V and writes them into
game/app/shaders/shaders_spv.h as byte arrays.

    python3 tools/compile_shaders.py <path to dxc>

dxc comes with the composition: vcpkg installs it under
build/automap/<platform>/vcpkg_installed/<triplet>/tools/directx-dxc/. The
header is committed, so a normal build needs no shader compiler; at run time
the sdl-shadercross--sdl3 pairing glue turns the SPIR-V into what the GPU
backend takes.
"""
import os
import subprocess
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
SHADERS = os.path.join(HERE, "..", "game", "app", "shaders")
STAGES = [("shape.vert.hlsl", "vs_6_0", "shape_vert_spv"), ("shape.frag.hlsl", "ps_6_0", "shape_frag_spv")]


def main():
    dxc = sys.argv[1] if len(sys.argv) > 1 else "dxc"
    out = ["// Written by tools/compile_shaders.py from the .hlsl files beside it. Don't edit.", "#pragma once", ""]
    for src, profile, name in STAGES:
        with tempfile.TemporaryDirectory() as tmp:
            spv = os.path.join(tmp, "out.spv")
            subprocess.run([dxc, "-spirv", "-T", profile, "-E", "main", os.path.join(SHADERS, src), "-Fo", spv], check=True)
            data = open(spv, "rb").read()
        out.append(f"static const unsigned char {name}[] = {{")
        for i in range(0, len(data), 20):
            out.append("    " + ", ".join(f"0x{b:02x}" for b in data[i:i + 20]) + ",")
        out.append("};")
        out.append("")
    with open(os.path.join(SHADERS, "shaders_spv.h"), "w") as f:
        f.write("\n".join(out))


if __name__ == "__main__":
    main()

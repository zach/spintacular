// Automap assets file-I/O interface, assets contract 1.0.0.
// SPDX-License-Identifier: MIT-0
//
// Every file an asset library reads goes through this interface, tagged with
// the library's id, so the game decides where files come from (a directory,
// an archive, a cooked package) in one place, and probes can see what each
// library read. It reads from the file system with the C runtime; a game that
// keeps its assets elsewhere replaces read() here.
#pragma once
#include <cstddef>
#include <cstdint>
#include <vector>

namespace am::assets {

// Reads the whole file at path for a library. Returns false, with bytes
// empty, when the file can't be read.
bool read(const char* library, const char* path, std::vector<std::byte>& bytes);

// Files the interface has read for a library.
std::uint64_t read_count(const char* library);

} // namespace am::assets

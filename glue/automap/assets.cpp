// Automap assets file-I/O interface, assets contract 1.0.0.
// SPDX-License-Identifier: MIT-0
#include "automap/assets.h"

#include <cstdio>
#include <map>
#include <mutex>
#include <string>

namespace am::assets {
namespace {

struct State {
    std::mutex mutex;
    std::map<std::string, std::uint64_t> reads;
};

State& state() {
    static State* s = new State;
    return *s;
}

} // namespace

bool read(const char* library, const char* path, std::vector<std::byte>& bytes) {
    bytes.clear();
    {
        State& s = state();
        std::lock_guard<std::mutex> lock(s.mutex);
        ++s.reads[library];
    }
    std::FILE* f = std::fopen(path, "rb");
    if (f == nullptr) return false;
    bool ok = std::fseek(f, 0, SEEK_END) == 0;
    long size = ok ? std::ftell(f) : -1;
    ok = size >= 0 && std::fseek(f, 0, SEEK_SET) == 0;
    if (ok) {
        bytes.resize(static_cast<std::size_t>(size));
        ok = size == 0 || std::fread(bytes.data(), 1, bytes.size(), f) == bytes.size();
    }
    std::fclose(f);
    if (!ok) bytes.clear();
    return ok;
}

std::uint64_t read_count(const char* library) {
    State& s = state();
    std::lock_guard<std::mutex> lock(s.mutex);
    auto it = s.reads.find(library);
    return it == s.reads.end() ? 0 : it->second;
}

} // namespace am::assets

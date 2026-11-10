// A small deterministic random stream: the same seed gives the same run on every
// machine and standard library. xoshiro256** seeded through SplitMix64.
#pragma once

#include <array>
#include <cstdint>
#include <vector>

namespace spin {

struct Rng {
    std::array<std::uint64_t, 4> s{};

    Rng() = default;
    explicit Rng(std::uint64_t seed) {
        for (auto& v : s) {
            seed += 0x9E3779B97F4A7C15ull;
            std::uint64_t z = seed;
            z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ull;
            z = (z ^ (z >> 27)) * 0x94D049BB133111EBull;
            v = z ^ (z >> 31);
        }
    }

    std::uint64_t next() {
        auto rotl = [](std::uint64_t x, int k) { return (x << k) | (x >> (64 - k)); };
        const std::uint64_t result = rotl(s[1] * 5, 7) * 9;
        const std::uint64_t t = s[1] << 17;
        s[2] ^= s[0];
        s[3] ^= s[1];
        s[1] ^= s[2];
        s[0] ^= s[3];
        s[2] ^= t;
        s[3] = rotl(s[3], 45);
        return result;
    }

    // Uniform in [0, n), without modulo bias.
    int below(int n) {
        if (n <= 1) return 0;
        const std::uint64_t bound = static_cast<std::uint64_t>(n);
        const std::uint64_t limit = UINT64_MAX - UINT64_MAX % bound;
        std::uint64_t v;
        do {
            v = next();
        } while (v >= limit);
        return static_cast<int>(v % bound);
    }

    // Uniform in [0, 1).
    double unit() { return static_cast<double>(next() >> 11) * (1.0 / 9007199254740992.0); }

    // Index chosen in proportion to its weight.
    int weighted(const std::vector<double>& weights) {
        double total = 0;
        for (double w : weights) total += w;
        double x = unit() * total;
        for (std::size_t i = 0; i < weights.size(); ++i) {
            if (x < weights[i]) return static_cast<int>(i);
            x -= weights[i];
        }
        return static_cast<int>(weights.size()) - 1;
    }

    template <class T>
    void shuffle(std::vector<T>& v) {
        for (int i = static_cast<int>(v.size()) - 1; i > 0; --i) {
            int j = below(i + 1);
            if (i != j) std::swap(v[i], v[j]);
        }
    }
};

} // namespace spin

"""Monte Carlo of how often scoring patterns appear in a random dice sequence.

Each die has one of 4 colors and, in the variants that use them, a d6 value.
Prints, for each sequence length, the chance a uniformly random sequence
contains at least one occurrence of each pattern of length 3, 4 and 5.

Usage: python3 prototypes/dice_patterns.py [trials]
"""
import random
import sys

COLORS = 4
FACES = 6


def longest_contig(seq, same):
    best = cur = 1
    for a, b in zip(seq, seq[1:]):
        cur = cur + 1 if same(a, b) else 1
        best = max(best, cur)
    return best


def longest_any_color(seq):
    # Non-contiguous: the most dice sharing one color anywhere in the sequence.
    counts = [0] * COLORS
    for c, _ in seq:
        counts[c] += 1
    return max(counts)


PATTERNS = {
    "color run (contiguous)": lambda s: longest_contig(s, lambda a, b: a[0] == b[0]),
    "color set (anywhere)": longest_any_color,
    "value run (contiguous)": lambda s: longest_contig(s, lambda a, b: a[1] == b[1]),
    "straight (contiguous, +1 each)": lambda s: longest_contig(s, lambda a, b: b[1] == a[1] + 1),
    "color straight (contiguous)": lambda s: longest_contig(
        s, lambda a, b: a[0] == b[0] and b[1] == a[1] + 1),
}


def main():
    trials = int(sys.argv[1]) if len(sys.argv) > 1 else 100_000
    rng = random.Random(1)
    lengths = [6, 8, 10, 12, 15]
    print(f"{trials} trials per length; P(at least one pattern of length >= k)\n")
    for name, longest in PATTERNS.items():
        print(name)
        print("  len  " + "  ".join(f"k={k:<5}" for k in (3, 4, 5)))
        for n in lengths:
            hits = {3: 0, 4: 0, 5: 0}
            for _ in range(trials):
                seq = [(rng.randrange(COLORS), rng.randrange(1, FACES + 1)) for _ in range(n)]
                m = longest(seq)
                for k in hits:
                    hits[k] += m >= k
            print(f"  {n:>3}  " + "  ".join(f"{hits[k] / trials:7.1%}" for k in (3, 4, 5)))
        print()


if __name__ == "__main__":
    main()

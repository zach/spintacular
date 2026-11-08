"""PROTOTYPE: a quota bonus for the haul (docs/design/haul-proposal.md).

Duplicate rule: landing a value already in the truck spoils the load (a tarp cancels one).
Load value = sum of cargo values. Delivered score = value x (1 + 0.5 per cargo), doubled if
the load value reached the quota. Compares, per bag, the best "stop after k cargo" policy
(ignoring the quota) with "keep going until the quota is reached".

Usage: python3 prototypes/haul_quota.py [trials]
"""
import random
import sys


def make_bag(copies=4, values=range(1, 7), tarps=0, bump=0):
    bag = [("cargo", min(6 + bump, v + bump)) for v in values for _ in range(copies)]
    return bag + [("tarp", 0)] * tarps


def trip(bag, rng, stop_at=None, quota=None, bonus=2.0):
    pool = bag[:]
    rng.shuffle(pool)
    held, seen, tarps = [], set(), 0
    for kind, v in pool:
        if kind == "tarp":
            tarps += 1
            continue
        if v in seen:
            if tarps:
                tarps -= 1
                continue
            return 0.0, False
        held.append(v)
        seen.add(v)
        total = sum(held)
        if quota is not None and total >= quota:
            return total * (1 + 0.5 * len(held)) * bonus, True
        if stop_at is not None and len(held) >= stop_at:
            break
    total = sum(held)
    hit = quota is not None and total >= quota
    return total * (1 + 0.5 * len(held)) * (bonus if hit else 1), hit


def ev(bag, n, rng, **kw):
    tot = hits = 0
    for _ in range(n):
        s, h = trip(bag, rng, **kw)
        tot += s
        hits += h
    return tot / n, hits / n


def main():
    n = int(sys.argv[1]) if len(sys.argv) > 1 else 30_000
    rng = random.Random(2)
    bags = [
        ("start: 4 copies of 1-6", make_bag()),
        ("2 tarps", make_bag(tarps=2)),
        ("thinned: 2 copies of 1-6", make_bag(copies=2)),
        ("bumped: values +1 (2-7)", make_bag(bump=1, values=range(1, 7))),
        ("thinned + 2 tarps", make_bag(copies=2, tarps=2)),
    ]
    quotas = [10, 12, 14, 16, 18, 20, 22]
    print("Best prudent policy (stop after k cargo, no quota) vs going for a x2 quota bonus\n")
    print(f"{'bag':<26}{'prudent':>12}" + "".join(f"{'Q=' + str(q):>14}" for q in quotas))
    for label, bag in bags:
        best = max((ev(bag, n, rng, stop_at=k)[0], k) for k in range(1, 7))
        cells = []
        for q in quotas:
            e, h = ev(bag, n, rng, quota=q)
            cells.append(f"{e:6.1f} ({h:4.0%})")
        print(f"{label:<26}{best[0]:7.1f} @k={best[1]}" + "".join(f"{c:>14}" for c in cells))
    print("\nCells: mean score when pressing on until the quota is reached (share of trips that reach it).")


if __name__ == "__main__":
    main()

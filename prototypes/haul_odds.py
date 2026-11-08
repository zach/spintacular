"""PROTOTYPE: push-your-luck odds for a "haul" round (docs/design/haul-proposal.md).

One trip: land wedges one at a time (each landed wedge leaves the wheel for the rest of the trip),
then deliver or keep going. Compares two bust rules on the starting bag:

  duplicate: like Flip 7, landing a value already in the truck spoils the load
             (a tarp cancels one duplicate). Six different values fill the truck: +bonus.
  spoilage:  like Quacks of Quedlinburg, hazard wedges add spoilage; above the truck's
             tolerance the load spoils.

Load score: (sum of values) x (1 + 0.5 per item in the truck), so pressing on is worth more.
Usage: python3 prototypes/haul_odds.py [trials]
"""
import random
import sys


def bag_duplicate(tarps):
    bag = [("cargo", v) for v in range(1, 7) for _ in range(4)]
    return bag + [("tarp", 0)] * tarps


def bag_spoilage(hazards):
    bag = [("cargo", v) for v in range(1, 7) for _ in range(3)]
    return bag + [("hazard", h) for h in hazards]


def trip(bag, rule, stop_at, rng, tolerance=4, full_bonus=2.0):
    pool = bag[:]
    rng.shuffle(pool)
    held, values, spoil, tarps = [], set(), 0, 0
    for kind, v in pool:
        if kind == "tarp":
            tarps += 1
            continue
        if kind == "hazard":
            spoil += v
            if spoil > tolerance:
                return 0, True
            continue
        if rule == "duplicate" and v in values:
            if tarps:
                tarps -= 1
                continue
            return 0, True
        held.append(v)
        values.add(v)
        if rule == "duplicate" and len(values) == 6:
            return sum(held) * (1 + 0.5 * len(held)) * full_bonus, False
        if len(held) >= stop_at:
            break
    return sum(held) * (1 + 0.5 * len(held)), False


def main():
    n = int(sys.argv[1]) if len(sys.argv) > 1 else 50_000
    rng = random.Random(1)
    setups = [
        ("duplicate, no tarps", "duplicate", bag_duplicate(0)),
        ("duplicate, 2 tarps", "duplicate", bag_duplicate(2)),
        ("spoilage 1,1,2,3 tol 4", "spoilage", bag_spoilage([1, 1, 2, 3])),
        ("spoilage 1,1,1,2,2,3 tol 4", "spoilage", bag_spoilage([1, 1, 1, 2, 2, 3])),
    ]
    for label, rule, bag in setups:
        print(f"{label}  (bag {len(bag)})")
        print("  stop after   spoiled   mean score")
        for k in range(1, 9):
            tot = busts = 0
            for _ in range(n):
                s, b = trip(bag, rule, k, rng)
                tot += s
                busts += b
            print(f"  {k:>10}   {busts / n:>7.1%}   {tot / n:>10.1f}")
        print()


if __name__ == "__main__":
    main()

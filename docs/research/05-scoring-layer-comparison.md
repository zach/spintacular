# 05 — Comparing three scoring layers

**Scope.** This note compares three scoring layers from `04-scoring-mechanisms.md`, all run on the same prototype, rubric and bots:

- **adjacent:** the current design. Dice score as runs of adjacent dice, in the order the wheels add them.
- **path:** reading order. Dice stay on a grid for 3 spins, with one row per spin. Each spin, the player traces one run of 3–5 orthogonally adjacent dice that touches the newest row, and those dice are used up. The traced order is the reading order, in either direction.
- **holders:** Azul-style. Lines hold 3, 4 and 5 dice. A line becomes a color, value or straight line when its first die goes in. The player places each die in a line that accepts it, or on the floor. A full line scores its pattern and empties. Floor penalties are 1, 1, 2, 2, 2, 3, 3 points, and lines carry over between spins within a round.

The holders layer has no adjacency "wall" yet, so items are its only compounding.

Run: 2026-11-07, `python3 prototypes/run/spintacular.py compare 300`.

**Method.** Each layer scores on a different scale, so comparing them on one target curve would be unfair. `compare` first scales each layer's target curve so that the best no-engine bot wins about 2% of runs. The scaling grows from nothing at round 1 to the full factor at round 12. It then runs the full rubric with 300 runs per bot. Bots place dice greedily (holders) or pick the highest-scoring path (path).

## Results [Verified by simulation]

| Check | adjacent | path | holders |
| --- | --- | --- | --- |
| Target scale | 0.89 | 1.73 | 1.91 |
| A1 round 1 safe | PASS (0%) | PASS (0%) | PASS (0%) |
| A2 reckless loses round 1 | PASS (99.7%) | **FAIL (44%)** | **FAIL (46%)** |
| B1 no-engine wins | PASS (3.0%) | PASS (2.3%) | PASS (2.0%) |
| B2 best engine wins | PASS (45%) | PASS (55%) | PASS (46%) |
| C1 straight factories win | 0% | 6% | 3% |
| D1 no cliff | PASS (14%) | **FAIL (40% in round 12)** | PASS (32%) |
| Checks passed | 10/10 | 8/10 | 9/10 |

Best win rate by focus:

| Focus | adjacent | path | holders |
| --- | --- | --- | --- |
| color | 38% | 55% | 34% |
| value | 40% | 51% | 46% |
| straight | 0% | 6% | 3% |
| adaptive | 45% | 45% | 36% |
| bargain style (mean) | 13% | 26% | 16% |

## Findings [Inference]

- **Path does the most for point factories and for the weaker strategies.** It has the highest engine win rates. It narrows the gap for the bargain style and roughly doubles color factories' wins. It lifts straights off zero, though not to a viable build.
- **Neither variant makes straights a real build.** Choosing the reading order or placement helps a little, but straights still need consecutive values close together, and nothing in the bag tilts toward consecutive values. If straights are to be a build, they need their own tilt tool. Otherwise they should stay a bonus.
- **Carrying results over between spins changes the risk in round 1.** In both path and holders, patterns build across spins, so even the reckless bot with one wheel survives round 1 more than half the time. The game gets more forgiving early, and losses pile up late. That's where path's D1 failure comes from: no bot loses before round 7. A different target curve, not just a scaled one, would be needed to spread losses out.
- **The holders' floor costs straight builds the most.** Straight bots are the only engine bots that lose in rounds 2–4 under holders, because consecutive values rarely arrive in order and the rest go to the floor. This is Cavanagh's leftover problem showing up (`04-scoring-mechanisms.md`).
- **Late spins become nearly deterministic in every layer.** Bots that win end with a median of 1.2 to 1.7 wedges per wheel, because the prototype lets the bag shrink to one wedge per wheel. A wheel with one wedge always lands the same die. That breaks "spins stay random" (`synergy.md`), so the comparison above flatters every layer equally.

# 05 — Comparing three scoring layers

**Scope.** This note compares three scoring layers from `04-scoring-mechanisms.md`, all run on the same prototype, rubric and bots:

- **adjacent:** the current design. Dice score as runs of adjacent dice, in the order the wheels add them.
- **path:** reading order. Dice stay on a grid for 3 spins, with one row per spin. Each spin, the player traces one run of 3–5 orthogonally adjacent dice that touches the newest row, and those dice are used up. The traced order is the reading order, in either direction.
- **holders:** Azul-style. Lines hold 3, 4 and 5 dice. A line becomes a color, value or straight line when its first die goes in. The player places each die in a line that accepts it, or on the floor. A full line scores its pattern and empties. Floor penalties are 1, 1, 2, 2, 2, 3, 3 points, and lines carry over between spins within a round.

The holders layer has no adjacency "wall" yet, so items are its only compounding.

Every wheel always holds its full number of wedges. Slots the bag can't fill hold blank wedges, which add no die and break a run (`docs/design/rounds.md`).

Run: 2026-11-07, `python3 prototypes/run/spintacular.py compare 300`.

**Method.** Each layer scores on a different scale, so comparing them on one target curve would be unfair. `compare` first scales each layer's target curve so that the best no-engine bot wins about 2% of runs. The scaling grows from nothing at round 1 to the full factor at round 12. It then runs the full rubric with 300 runs per bot. Bots place dice greedily (holders) or pick the highest-scoring path (path). They thin the bag only while it still fills every slot.

## Results [Verified by simulation]

| Check | adjacent | path | holders |
| --- | --- | --- | --- |
| Target scale | 0.74 | 1.24 | 1.48 |
| A1 round 1 safe | PASS (0%) | PASS (0%) | PASS (0%) |
| A2 reckless loses round 1 | PASS (99.7%) | **FAIL (44%)** | **FAIL (46%)** |
| B1 no-engine wins | PASS (0.7%) | PASS (2.3%) | PASS (2.0%) |
| B2 best engine wins (bar 40%) | **FAIL (11%)** | **FAIL (30%)** | **FAIL (36%)** |
| C1 kinds of factory | PASS | **FAIL** | PASS |
| D1 no cliff | PASS (12%) | PASS (33%) | PASS (33%) |
| Checks passed | 9/10 | 7/10 | 8/10 |

Best win rate by focus:

| Focus | adjacent | path | holders |
| --- | --- | --- | --- |
| color | 8% | 30% | 36% |
| value | 6% | 14% | 30% |
| straight | 0% | 5% | 4% |
| adaptive | 11% | 22% | 33% |

The earlier run, without blanks, let bots thin a wheel down to one wedge. Winning bots ended with a median of 1.2 to 1.7 wedges per wheel, and the best engine wins were 45% (adjacent), 55% (path) and 46% (holders).

## Findings [Inference]

- **Once spins stay random, the player has to choose where results go.** Blanks cut the adjacent layer's best factory from 45% to 11% of wins. Most of its success came from wheels that always landed the same die. Path (30%) and holders (36%) hold up, because the player chooses where each die goes or how it's read, and so can work around a bad spin. That's the agency finding from `04-scoring-mechanisms.md`, now measured.
- **Holders are the strongest layer under random spins.** They have the best factory win rates, three kinds of factory within half of the best, no cliff, and the bargain style no longer weak (18% mean, against 2% for adjacent). They still lack Azul's adjacency wall, the compounding source that research suggests, so items are their only multiplier.
- **Straights are still not a build in any layer (0–5%).** Nothing in the bag tilts toward consecutive values. Straights need their own tilt tool, or should stay a bonus.
- **Carrying results over between spins makes round 1 forgiving.** In path and holders, even the reckless bot with one wheel survives round 1 more than half the time. A1 is still met, but A2's idea, that an insane risk should be able to lose round 1, isn't.
- **B2 fails in every layer at the same wall height.** The 40% bar was set before spins were kept random. Either factories need more compounding, such as the adjacency wall for holders, or the wall needs to come down.

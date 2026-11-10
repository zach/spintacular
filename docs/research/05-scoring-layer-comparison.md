# 05 — Comparing three scoring layers

**Scope.** This note compares three scoring layers from `04-scoring-mechanisms.md`, all run on the same prototype, rubric and bots:

- **adjacent:** the design at the time. Dice score as runs of adjacent dice, in the order the wheels add them.
- **path:** reading order. Dice stay on a grid for 3 spins, with one row per spin. Each spin, the player traces one run of 3–5 orthogonally adjacent dice that touches the newest row, and those dice are used up. The traced order is the reading order, in either direction.
- **holders:** Azul-style. Lines hold 3, 4 and 5 dice. A line becomes a color, value or straight line when its first die goes in. The player places each die in a line that accepts it, or on the floor. A full line scores its pattern and empties. Floor penalties are 1, 1, 2, 2, 2, 3, 3 points, and lines carry over between spins within a round.

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
- **Holders are the strongest layer under random spins.** They have the best factory win rates, three kinds of factory within half of the best, no cliff, and the bargain style no longer weak (18% mean, against 2% for adjacent).
- **Straights are still not a build in any layer (0–5%).** Nothing in the bag tilts toward consecutive values. Straights need their own tilt tool, or should stay a bonus.
- **Carrying results over between spins makes round 1 forgiving.** In path and holders, even the reckless bot with one wheel survives round 1 more than half the time. A1 is still met, but A2's idea, that an insane risk should be able to lose round 1, isn't.
- **B2 fails in every layer at the same wall height.** The 40% bar was set before spins were kept random. Either factories need more compounding, such as the adjacency wall for holders, or the wall needs to come down.

## Adding the wall

**Setup.** The wall is a run-long 4×6 board, with a row per color and a column per value. When a line fills, the die that filled it places a marker on its cell, if the cell is free. The marker scores as an Azul tile: 1 alone, or its row chain plus its column chain. That score is paid as mult. Each variant below calibrates its own wall height first.

**Results [Verified by simulation].**

| Variant | Target scale | Best engine wins | Value focus | Checks |
| --- | --- | --- | --- | --- |
| Holders, no wall | 1.48 | 36.0% | 30% | 8/10 |
| Wall, +1 mult per point | 1.71 | 36.7% | 27% | 8/10 |
| Wall, +3 mult per point | 2.39 | 37.0% | 21% | 7/10 |
| Wall, +1, plus 0.5 mult per marker every spin | 2.62 | 38.7% | 19% | 7/10 |
| Wall, +1, plus 1 mult per marker every spin | 3.87 | 30.7% | 13% | 7/10 |
| Wall, +1, bots aim placements | 1.56 | **43.0%** | 28% | 7/10 |
| Wall, +3, bots aim placements | 2.21 | 36.0% | 23% | 7/10 |

A bot that *aims* holds a line one short when the die that would fill it would place an isolated marker, and starts another line with that die instead.

With aiming at +1 per point, the failing checks are:

- A2: the reckless bot loses round 1 46% of the time.
- B1: 3.7%, against a 3% bar, from calibration noise.
- D1: round 12 causes 36% of losses.

**Findings [Inference].**

- **The wall only pays a player who aims at it.** When markers land wherever the filling die happens to fall, the wall lifts every bot's score about equally, including bots with no engine. Its payouts then only raise the wall height, and value builds get narrower. Once bots hold lines to place markers next to chains, the best point factory goes from 37% to 43%. So the wall rewards planning, which is the agency finding again.
- **Bigger wall payouts hurt.** Larger placement payouts and per-spin payouts make every bot score more, and push builds toward color.
- **Next:** items keyed to the wall, such as "x mult per marker in your longest row", would make the wall a build axis of its own. A way to choose a marker's cell, such as a consumable, would add more aiming. Straights are still at 3–5%.

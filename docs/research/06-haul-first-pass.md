# 06 — The haul in the run prototype

**Scope.** This note compares the haul (`docs/design/haul-proposal.md`) with holders and the wall, on the same rubric, bots and calibration. Details the proposal leaves open use these prototype defaults:

- **A round:** 4 trips, each spinning one wheel at a time.
- **Landed wedges** leave their wheel until the trip ends.
- **Spoiling:** a duplicate value spoils the load.
- **Truck:** it's full at 6 different values, which pays ×1.5.
- **Load mult:** +0.5 per cargo.
- **Quota:** 8 in round 1, rising to 36 by round 12; reaching it pays ×2.
- **Load patterns:** the biggest one-color group, and the longest run of consecutive values. Both have their own smaller payout table.
- **Haul-only content:** tarp wedges, crate wedges (×2), Spare Tarp (one tarp per round) and Overtime (the quota pays ×3).
- **Values:** they can reach 9 through bump wheels.
- **Bots:** they keep spinning while one more spin, on the best wheel, is worth more on average than delivering. The reckless bot never delivers by choice.

Run: 2026-11-08, `python3 prototypes/run/spintacular.py compare 300 wall,haul`.

## Results [Verified by simulation]

| Check | wall | haul |
| --- | --- | --- |
| Target scale | 0.99 | 0.46 |
| A1 round 1 safe | PASS (0%) | PASS (0.7%) |
| A2 reckless loses round 1 | FAIL (46%) | **PASS (66%)** |
| B1 no-engine wins | FAIL (3.7%) | PASS (1.3%) |
| B2 best engine wins (bar 40%) | PASS (43%) | **FAIL (11%)** |
| C1 kinds of factory | PASS | FAIL |
| D1 no cliff | FAIL (36%) | **PASS (23%)** |
| Checks passed | 7/10 | 8/10 |

Best win rate by focus in the haul: straight 11%, color 8%, adaptive 5%, value 0.3%.

## Findings [Inference]

- **The haul gets the shape of risk right.** It's the first layer to pass A1, A2 and D1 together:
  - sensible bots almost never lose round 1;
  - pressing your luck recklessly loses it two times in three;
  - losses spread across the run, with no cliff.
- **Straights become a build at last** (best focus, 11%). Under the duplicate rule every value in the truck is different, so runs of consecutive values come naturally.
- **Point factories are too weak.** The best engine wins 11%, against 43% under the wall. The bots' shopping rules were written for the earlier layers, and some now work against them. Value-focused bots recut wedges to the same value, which makes duplicates, and so spoiled loads, more likely.
- **The rules have a hole: a bag with one wedge per value can never spoil.** Landed wedges leave the wheel for the rest of the trip, so a bag thinned to one wedge of each value (with blanks filling the slots) makes every trip risk-free. Bots don't find this yet, but players would. It ends the press-your-luck decision for exactly the builds the game should reward.

## Second pass: wedges stay on the wheel [Verified by simulation]

With the hole closed, a landed wedge stays on its wheel and can come up again. Bots also got shopping rules for the haul:

- thin out duplicated values;
- recut wedges toward rare values;
- value tarps and crates highly;
- avoid Second Die.

Run: 2026-11-08, `python3 prototypes/run/spintacular.py compare 300 haul`.

| Check | haul, wedges stay |
| --- | --- |
| Target scale | 0.18 (targets 30 → 4,994) |
| A1 round 1 safe | **FAIL (2.0%)** |
| A2 reckless loses round 1 | PASS (100%) |
| B1 no-engine wins | FAIL (3.3%) |
| B2 best engine wins | **FAIL (6.7%)** |
| D1 no cliff | PASS (15%) |
| Checks passed | 7/10 |

## Findings, second pass [Inference]

- **There is no point factory in the haul yet.** Engine bots win 1–7%, barely more than bots with no engine (up to 3.3%). Calibration then pulled the targets down to a fifth of the wall's. Two things cap a load:
  - Bust risk caps it at about three cargo, whatever the build.
  - The multipliers on offer (ribbons, flat mult, crates) help every build about equally.
- **Nothing in the content rewards carrying a bigger load safely.** The haul needs engine content of its own. Two kinds are missing:
  - **Ways to make big loads safer.** For example, controlling which values sit on which wheel, so the player can spin each wheel in turn without repeating a value.
  - **Payouts that grow with load size or with risk taken.** Examples: "+mult per cargo past 3", "×2 for 5+ cargo".
- **Round 1 is slightly too risky** for bots that weigh one spin ahead (2% lose). A lower round-1 target or a fifth trip would fix it.

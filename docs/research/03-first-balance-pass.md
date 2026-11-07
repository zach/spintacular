# 03 — First balance pass

**Scope.** Whether the first-round and end-of-run difficulty in `docs/design/session-rubric.md` can be reached with the prototype (`prototypes/run/`), and what the bots' play shows about the design. The rules follow `docs/design/`. Details the design leaves open use the prototype's defaults (`prototypes/run/README.md`).

Run: 2026-11-07, `python3 prototypes/run/spintacular.py sim 1000`, with the `tables.json` committed alongside this note.

## Results [Verified by simulation]

9 of the 10 rubric checks pass. B2 sits on its line: the best engine bot wins 39.9% of runs against a bar of 40%.

| Bot | Wins | Loses round 1 |
| --- | --- | --- |
| passive | 0.0% | 0.0% |
| random | 0.5% | 0.0% |
| flat/spender | 2.1% | 0.0% |
| reckless | 0.0% | 99.6% |
| color/reroller | 38.6% | 0.0% |
| adaptive/reroller | 39.9% | 0.1% |
| value/reroller | 33.6% | 0.0% |
| color/saver | 32.0% | 0.2% |
| straight/* | 0.0–0.1% | 0.0–0.2% |
| */bargain | 0.0–18.9% | 0.0–0.3% |

The no-engine bots lose mostly in rounds 10–12. `flat/spender` loses 465 runs in round 11 and 355 in round 12. Engine bots lose a few percent in every round, so no single round makes up more than 14% of their losses.

Getting here took these changes to the first tables:

- **Thinning the bag had to be possible.** Bots could not tilt the bag until wedges could be dealt to wheels round-robin, the bag could shrink to one wedge per wheel, and Paint, Recut and a Rod that works on every spin existed.
- **Flat bonuses had to be weaker:** Gilded Wheel +4, Lucky Coin +2, Heavy Purse +15.
- **Factory pieces had to be stronger,** and rare items had to show up more often from round 8.
- **Rounds needed 5 spins.** With 4, one bad round sank most factories.
- **Score targets** start gentle and double over the last rounds: 30 → 18,000.

## Findings [Inference]

- **A point factory is mostly a thinned bag.** Zap is bought in 93% of winning runs, Second Die in 84% and The Rod in 76%. Tilting the bag is the core of every factory, which matches `synergy.md`. But The Rod is near the 80% must-have line in E2. It needs company: other ways to tilt the bag that do the same job differently.
- **Straight factories don't work (0%).** A straight needs rising values on adjacent wheels, and no tilt tool controls value *order*. The Step Counter, Ladder and Stairway items are dead weight (win presence 0–1%). The design needs a way to tilt toward straights, or straights should stay a rare bonus rather than a kind of build.
- **The bargain style is weak when played alone** (0–19%). Pure bargain bots die in rounds 3–5. That fits the economy design, where bargain hunting is something to switch into, but it can't be shown until a bot switches styles.
- **Some content is rarely bought:** Piggy Bank (8% pick rate), Low Ribbon (4%), Even Steven and Odd Fellow (9–11%). These are candidates to rework.
- **The margins are thin.** B1 passes at 2.1% against a 3% bar, and B2 misses by 0.1 points. Small changes to the tables move both, so a larger sample and better bots should come before finer tuning.

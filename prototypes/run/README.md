# PROTOTYPE: text-only run

Throwaway code for tuning Spintacular's numbers before any engine work. It uses only Python 3's standard library.

```bash
python3 prototypes/run/spintacular.py sim 1000
```

Bots play 1,000 runs each. The command prints win rates, the round each loss happened in, the rubric checks (`docs/design/session-rubric.md`) and per-content telemetry. It exits non-zero if any rubric check fails.

```bash
python3 prototypes/run/spintacular.py play 42
```

Play a run yourself with seed 42. Press Enter to play a round; in the shop, type commands such as `buy 0`, `sell 1`, `order 2,0,1`, `wheels 1,0,2`, `reroll` and `done`.

```bash
python3 prototypes/run/spintacular.py capacity 100
```

Shows each bot's median round score when no one can lose. Use it to set score targets.

```bash
python3 prototypes/run/spintacular.py compare 300
```

Compares the scoring layers. It scales each layer's target curve so that bots without a point factory win about 2% of runs, then runs the rubric on each (`docs/research/05-scoring-layer-comparison.md`).

Every balance number and every piece of content is in `tables.json`. `scoring.layer` picks the scoring layer: `wall` (the current design: Azul-style lines with a floor, plus a run-long wall), `holders` (lines without the wall), `adjacent` (runs of adjacent dice) or `path` (the player traces a run through a grid of recent spins). In `play`, the path or placement is chosen for you by the bots' rules; only `adjacent` has no choices to make.

## Prototype defaults

The design (`docs/design/`) leaves these open. The prototype picks a default for each so it can run. They are not decisions.

- **Score:** points × mult. Each die gives points equal to its value. Each pattern adds points and mult (`patterns` in `tables.json`). Then items apply, in order.
- **Holders:**
  - lines of 3, 4 and 5;
  - straight lines can run either way;
  - floor penalties of 1, 1, 2, 2, 2, 3, 3 points, then 3 for each die after that;
  - lines reset each round.
- **The wall:**
  - it lasts the whole run;
  - a marker goes on the filling die's cell, or nowhere if that cell is taken;
  - a wild die picks its best free color;
  - a marker pays +1 mult per point it scores, only on the spin it's placed.
- **In the adjacent and path layers:** runs longer than 5 score as a run of 5. Straights in the adjacent layer only go up.
- **A run** has 12 rounds of 5 spins each.
- **Start:** a bag of 24 wedges (each color with values 1–6), 4 plain wheels with room for 4 wedges each, and $4.
- **Filling the wheels:** wedges are dealt round-robin. Slots the bag can't fill hold blank wedges, which add no die and break a run. Bots thin the bag only while it still fills every slot. Holder bots fill lines greedily and hold a line one short rather than place an isolated marker on the wall.
- **Shop:** 4 offers. Rarity is gated by round: only commons until round 3, uncommons from round 3, rares from round 5, and more rares from round 8. Reroll costs $3, plus $1 more each time within a visit. Selling returns half the price. Limits: 5 items, 7 wheels, 2 consumables.
- **Income:** $5 per round, $1 per unused spin, and Balatro-style interest of $1 per $5 held, up to $5.
- **Tilt tools:**
  - **Zap:** a shop service that removes a wedge.
  - **Paint** and **Recut:** upgrades that change a wedge's color or value.
  - **The Rod:** an item that may remove a landed wedge after each spin.
- **Consumables:**
  - **Nudge:** moves one wheel's pointer one wedge.
  - **Respin:** spins again.
  - **Outer Band:** makes one color's wedges 3× as wide for one spin.
  - When you play by hand, nudge and respin fire automatically on a round's last spin if it would otherwise fall short.
- **Spendthrift:** money left after the shop is lost.

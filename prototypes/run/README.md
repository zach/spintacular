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

Every balance number and every piece of content is in `tables.json`.

## Prototype defaults

The design (`docs/design/`) leaves these open. The prototype picks a default for each so it can run. They are not decisions.

- **Score:** points × mult. Each die gives points equal to its value. Each pattern adds points and mult (`patterns` in `tables.json`). Then items apply, in order.
- **Runs longer than 5** score as a run of 5.
- **Straights** only go up: 3-4-5 counts, 5-4-3 doesn't.
- **A run** has 12 rounds of 5 spins each.
- **Start:** a bag of 24 wedges (each color with values 1–6), 4 plain wheels with room for 4 wedges each, and $4.
- **Filling the wheels:** wedges are dealt round-robin, so a small bag still reaches every wheel. A wheel with no wedges adds nothing.
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

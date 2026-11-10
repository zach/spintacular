# PROTOTYPE: the haul

Throwaway code for tuning Spintacular's design before any engine work. It plays the current design (`docs/design/`), and uses only Python 3's standard library.

```bash
python3 prototypes/haul/haul.py play 42
```

Play a run yourself with seed 42. Each spin, type a wheel's number, or `d` to deliver. Every wheel shows its chance of spoiling the load. In the shop, type commands such as `buy 0`, `sell 1`, `order 2,0,1`, `reroll` and `done`.

```bash
python3 prototypes/haul/haul.py sim 1000
```

Bots play 1,000 runs each. The command prints win rates, the round each loss happened in, the rubric checks (`docs/design/session-rubric.md`), how the bots pressed their luck, and telemetry for every piece of content. It exits non-zero if a rubric check fails.

```bash
python3 prototypes/haul/haul.py calibrate 500
```

Scales the score targets until bots with no point factory win about 2% of runs, then runs `sim`. Use it after changing rules or content.

```bash
python3 prototypes/haul/haul.py capacity 200
```

Shows each bot's median round score when no one can lose.

```bash
python3 prototypes/haul/haul.py experiments 300
```

Compares the open design choices, each calibrated to the same difficulty (`docs/research/07-haul-design-experiments.md`).

Every number and every piece of content is in `tables.json`. Any command takes `key=value` to override one for that run:

```bash
python3 prototypes/haul/haul.py calibrate 300 round.tank=14 scoring.quota_bonus=3
```

## What the prototype leaves out

- **Rigs, stakes and unlocks** (`docs/design/progression.md`).
- **Seeing the inspection ahead of time.** Bots don't prepare for inspections.
- **Choosing when to use an Outer Band or a rescue.** Bots use them by simple rules; in `play` you're asked.

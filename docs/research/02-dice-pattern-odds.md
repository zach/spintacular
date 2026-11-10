# 02 — How often dice patterns come up

**Scope.** How often patterns of 3, 4 and 5 dice appear in a random sequence of four-colored dice, to inform which patterns the dice layer (`docs/design/scoring.md`) scores. Values are d6 where a variant uses them. Not covered: payout sizes, or how many dice a round produces.

Run: 2026-11-07, `python3 prototypes/dice_patterns.py 50000` (seeded, 50,000 trials per length). Figures are Monte Carlo estimates, good to about ±0.5 percentage points.

## Results [Verified by simulation]

Chance that a uniformly random sequence contains at least one pattern of length k or longer:

| Pattern | Dice | k=3 | k=4 | k=5 |
| --- | --- | --- | --- | --- |
| Same color, adjacent | 6 | 19.8% | 4.1% | 0.7% |
| | 10 | 35.4% | 8.4% | 1.8% |
| | 15 | 50.4% | 13.9% | 3.3% |
| Same color, anywhere | 6 | 64.8% | 15.3% | 1.9% |
| | 10 | 100.0% | 79.1% | 31.0% |
| | 15 | 100.0% | 100.0% | 94.1% |
| Same value, adjacent | 10 | 17.8% | 2.6% | 0.4% |
| Straight (+1 each), adjacent | 10 | 12.7% | 1.5% | 0.1% |
| Same-color straight, adjacent | 10 | 1.0% | 0.0% | 0.0% |

Same color anywhere in the sequence saturates, by the pigeonhole principle. With four colors, any 9 dice contain 3 of one color, any 13 contain 4, and any 17 contain 5.

Tilting toward one color, for example by removing unwanted wedges (`docs/design/synergy.md`), changes the odds of an adjacent run of that color in 10 dice:

| Share of dice in the favored color | Run of 3 | Run of 4 | Run of 5 |
| --- | --- | --- | --- |
| 25% (untilted) | 9.3% | 2.1% | 0.5% |
| 40% | 31.6% | 11.8% | 4.3% |
| 55% | 60.6% | 33.1% | 16.4% |
| 70% | 85.8% | 63.2% | 41.7% |

## Implications [Inference]

- **Adjacent patterns stay graded; "anywhere" patterns don't.** An anywhere pattern of 3 is guaranteed from 9 dice, so it can't be an exciting payout. Adjacent runs give a natural rarity ladder: 3 is common, 4 uncommon, 5 rare.
- **Adjacency makes the order of the wheels matter,** because the wheels append their dice left to right. That supports the rearrangeable resolution order.
- **Tilting pays off steeply and stays random.** Moving the favored color from 25% to 55% raises a run of 4 from about 2% to about 33%, which is the payoff for "tilt the game" synergy without a guarantee.
- **Values add a second axis,** but straights and same-value runs are rarer than color runs. A same-color straight is a jackpot (about 1% for 3 in 10 dice).

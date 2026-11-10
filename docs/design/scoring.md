# Scoring: the haul

The middle layer of the game is a press-your-luck haul. The player loads a truck one spin at a time. A bigger load pays far more than a small one, but one bad spin spoils the whole load. The core decision, taken on every spin, is: **one more, or deliver?**

The haul is where the game's scoring lives. Five other middle layers were tried before it (`docs/research/05-scoring-layer-comparison.md`, `06-haul-first-pass.md`). The numbers below are the prototype's (`prototypes/haul/tables.json`), set by experiment (`docs/research/07-haul-design-experiments.md`).

## A trip

1. The player picks any wheel and spins it.
2. The wedge it lands on goes into the truck. The wedge stays on its wheel.
3. The player spins again, on any wheel, or **delivers**. Delivering scores the load and ends the trip.

A trip has a **tank of 10 spins**. When the tank is empty, the load delivers.

## What a wedge carries

| Wedge | In the truck |
| --- | --- |
| **Cargo** | Has a color and a value. It adds its value in points, and makes the load one cargo bigger. |
| **Tarp** | Covers one repeat (see below), then it's used up. |
| **Crate** | Multiplies the load by 1.5 when it's delivered. |
| **Empty** | Nothing. A slot the bag couldn't fill is an empty wedge. |

## No repeats

**A spin spoils the load if it lands:**

- **a wedge that has already landed this trip,** whatever it carries; or
- **cargo whose value is already in the truck.**

A spoiled load scores nothing, and the trip is over.

Colors never spoil a load, so they're free to build around. The game marks every wedge that would spoil the load, and shows each wheel's chance of spoiling it as a percentage, so the risk is always visible (`presentation.md`).

The first spin on each wheel can only repeat a value, never a wedge. After that, re-spinning a wheel always carries some risk, however the bag is built. That's what keeps the decision alive for every build: scripted builds that tried to remove all risk now spoil most of their loads (`07-haul-design-experiments.md`).

### What can save a load

When a spin would spoil the load, these apply in order:

1. A **Padded Wheel** throws the repeat out.
2. **Double Clutch** throws out the first repeat of each load.
3. A **tarp** covers it and is used up.
4. The player may use a **Nudge** or a **Respin** (`synergy.md`).

If none applies, the load spoils.

## What a delivery pays

**Points × mult, then bonuses.** Each step is shown as it happens.

1. **Points:** the values of the cargo, plus any bonus points from stamps, wheels and items.
2. **Mult:** 1 for each cargo in the load. So a load twice as big has twice the points *and* twice the mult.
3. **Sets** add mult:

   | Size | 3 | 4 | 5 | 6 |
   | --- | --- | --- | --- | --- |
   | Cargo of one color | +2 | +4 | +7 | +11 |
   | A run of consecutive values | +2 | +4 | +7 | +11 |

4. **Items** apply, in the order the player arranged them.
5. **Crates:** ×1.5 each.
6. **Quota:** if the load's points reach the round's quota, ×2.
7. **Full truck:** a truck holding 6 different values is full. It delivers itself and pays ×1.5.

For example, a load of R2, G3, B4 is 9 points × (3 for three cargo + 2 for a run of three) = 45. If the quota is 9 or less, it pays 90.

A delivery also pays money: $1 for each cargo past the third (`economy.md`).

## The quota

Each round sets a quota, and a load whose points reach it pays double. The quota does three jobs:

- **Temptation.** A truck just short of the quota begs for one more spin.
- **Rewarding better loads.** Builds that carry more, or carry higher values, reach it far more often.
- **Scaling.** It's one number that rises round by round: 10 in round 1, 36 in round 12.

Six different values from 1 to 6 add up to 21, so late quotas can't be met with starting cargo. The player has to raise values with Premium, stamps and wheels. In simulation, taking the quota out of the game cut the best point factory's win rate from 42% to 22%.

## Why these rules

Each was tested against alternatives at the same difficulty (`07-haul-design-experiments.md`):

- **Trips with a tank each, not one pool of fuel for the round.** With one pool, the best play was many small safe deliveries. With trips, loads are bigger and spoil more often, which is the game.
- **A random deal, not placing wedges by hand.** Placing wedges let a careful player take most of the risk out.
- **A truck of 6 values.** At 5 the truck fills too easily, and reckless play stops being punished. At 7 a full truck is out of reach.
- **Wedges stay on the wheel,** so a bag can never be thinned until nothing can repeat.

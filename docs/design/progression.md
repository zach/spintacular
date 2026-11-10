# Progression: runs, rigs and stakes

How a run is structured, and what changes from one run to the next. Nothing here is in the prototype except the 12 rounds and the three inspections; the rest is design to build and test.

## A run

- **Length:** 12 rounds, grouped into 4 routes of 3 stops. A run should take 20–30 minutes.
- **Inspections:** the third stop of each route is an inspection: a round with one extra rule (`rounds.md`). The game shows the route's inspection as soon as the route starts, so the player has two shops to prepare for it.
- **Winning:** clearing round 12 wins the run. The player can then keep going in an endless mode, where targets and quotas keep rising, to see how far the build goes.
- **Losing:** missing a round's target ends the run (`rounds.md`).
- **Seeds:** every run comes from one seed, which the player can see, copy and replay. The shop, the deal and the spins each draw from their own stream, so rerolling the shop never changes what the wheels do.
- **Saving:** a run can be left and resumed at any shop or spin.

## Rigs

A rig is a starting loadout, like a deck in Balatro. Each one pushes a different way of playing from the first shop. The player starts with the Standard rig and unlocks the others by winning.

| Rig | Starts with | Pushes toward |
| --- | --- | --- |
| **Standard** | 12 cargo wedges (values 1–6, twice), 4 plain wheels | Anything |
| **Tanker** | +2 to the tank, but one fewer item slot | Longer trips |
| **Reefer** | 2 tarp wedges, but quotas are 20% higher | Long, protected loads |
| **Flatbed** | A truck that holds 7 values, but only 3 wheels | Full-truck bonuses |
| **Hotshot** | 2 crate wedges and $10, but 2 less in the tank | High-risk, high-multiplier loads |
| **Rainbow** | Every wedge a different value and color (1–9), but no interest | Runs of values, with few duplicates |

## Stakes

A stake is a difficulty level. Winning on one stake unlocks the next for that rig. Each stake keeps the rules of the ones below it and adds one.

| Stake | Adds |
| --- | --- |
| 1 | The base game |
| 2 | Targets are 15% higher |
| 3 | Shop prices are $1 higher |
| 4 | Inspections happen every second round |
| 5 | The tank holds 1 fewer spin |
| 6 | Spoiled loads cost $2 |
| 7 | Quotas are 20% higher |
| 8 | Tarps wear out: each one works only in the round it's landed |

## Unlocks and the collection

- **Content unlocks.** The first run uses a starter set of about half the items, wheels and upgrades. The rest unlock by doing things in play, such as "deliver a full truck" or "win without buying a tarp". This follows the same rule as the shop: simple content first, complex content later (`economy.md`).
- **The collection** lists every item, wheel, upgrade, wedge and inspection the player has seen, and the best stake won with each rig.
- **Nothing carries over that makes a run stronger.** Unlocks widen what can appear; they don't add power. Each run starts even.

## Daily haul

One seed a day, the same for everyone, on a fixed rig and stake, with a results screen to share. It needs no server beyond the date.

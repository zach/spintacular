# Progression: runs, rigs and stakes

How a run is structured, and what changes from one run to the next. Rounds, inspections, rigs and stakes are in the prototype and tested by simulation. Seeds are checked by a self-test. Unlocks, the collection and the daily haul are in the browser prototype (`prototypes/web/`), with a simplified unlock rule.

## A run

- **Length:** 12 rounds, grouped into 4 routes of 3 stops. A run should take 20–30 minutes.
- **Inspections:** the third stop of each route is an inspection: a round with one extra rule (`rounds.md`). The game shows the route's inspection as soon as the route starts, so the player has two shops to prepare for it.
- **Winning:** clearing round 12 wins the run. The player can then keep going in an endless mode, where targets and quotas keep rising, to see how far the build goes.
- **Losing:** missing a round's target ends the run (`rounds.md`).
- **Seeds:** every run comes from one seed, which the player can see, copy and replay. The shop, the deal and the spins each draw from their own stream, so rerolling the shop never changes what the wheels do. The prototype checks both (`python3 prototypes/haul/haul.py selftest`).
- **Saving:** a run can be left and resumed at any shop or spin.

## Rigs

A rig is a starting loadout, like a deck in Balatro. Each one pushes a different way of playing from the first shop. The player starts with the Standard rig and unlocks the others by winning.

Every rig was simulated on stake 1 (`../research/07-haul-design-experiments.md`). "Factory" is the best point-factory bot's win rate; "No factory" is the best bot without one.

| Rig | Starts with | Pushes toward | Factory | No factory |
| --- | --- | --- | --- | --- |
| **Standard** | 12 cargo wedges (values 1–6, twice), 4 plain wheels | Anything | 43% | 2.2% |
| **Tanker** | +2 to the tank, but one fewer item slot | Longer trips | 40% | 1.2% |
| **Reefer** | 2 tarp wedges, but quotas are 20% higher | Long, protected loads | 36% | 2.0% |
| **Flatbed** | A truck that holds 7 values, but only 3 wheels | Full-truck bonuses | 37% | 1.8% |
| **Hotshot** | 2 crate wedges and $10, but 3 less in the tank | High-risk, high-multiplier loads | 35% | 3.5% |
| **Monochrome** | Every wedge red, but targets are 30% higher | Color sets | 48% | 3.8% |

On every rig, sensible bots lose round 1 at most 1% of the time.

**A rule for future rigs: don't add values to the starting bag.** How many different values the bag holds is the game's strongest difficulty dial. Two rigs that started with more values were tried and dropped. With nine unique values, bots with no point factory won 24% of runs. With values 1–8 twice, they won 37%.

## Stakes

A stake is a difficulty level. Winning on one stake unlocks the next for that rig. Each stake keeps the rules of the ones below it and adds one. The ladder was simulated on the Standard rig; the win rate is the best point-factory bot's.

| Stake | Adds | Factory wins |
| --- | --- | --- |
| 1 | The base game | 43% |
| 2 | Targets are 15% higher | 37% |
| 3 | Shop prices are $1 higher | 24% |
| 4 | Inspections happen every second round | 22% |
| 5 | The tank holds 1 fewer spin | 19% |
| 6 | A spoiled load costs $2 | 18% |
| 7 | Quotas are 20% higher | 10% |
| 8 | One fewer item slot | 9% |

The ladder falls at every step and never reaches zero. Round 1 stays safe on every stake (0.8% at worst). The bots are weaker than a practiced player, so read these as floors.

## Unlocks and the collection

- **Content unlocks.** The first run uses a starter set: about two thirds of the items, wheels and upgrades. The rest unlock by doing things in play, such as "deliver a full truck" or "win without buying a tarp". This follows the same rule as the shop: simple content first, complex content later (`economy.md`).
  - **Locked at first:** Thermos, Daredevil, Overtime, The Rod, Spendthrift, Assembly Line, Hazard Pay, Big Rig, Tip Jar, the Express and Padded wheels, Stamp, Wild Color and the Outer Band.
  - **The starter set is friendlier.** In simulation the best point factory wins 61% with the starter set, against 42% with everything unlocked, and play without a factory still wins only 3%. A smaller pool makes the pieces of a build easier to find, so a new player's first runs are the gentlest.
- **The collection** lists every item, wheel, upgrade, wedge and inspection the player has seen, and the best stake won with each rig.
- **Nothing carries over that makes a run stronger.** Unlocks widen what can appear; they don't add power. Each run starts even.

## Daily haul

One seed a day, the same for everyone, on a fixed rig and stake, with a results screen to share. It needs no server beyond the date.

# Proposal: the haul (press your luck)

**Status: proposal, not decided.** It would replace the holders and the wall (`scoring.md`) as the middle layer.

The core decision on every spin becomes: **keep loading the truck, or deliver now?** Each spin adds to a load that pays more the bigger it gets. One bad spin you can't fix spoils the whole load. This is the tension in Flip 7, where flipping a number you already have busts your turn. Quacks of Quedlinburg does the same with a bag you build, which Spintacular already has.

## A trip

1. The player picks a wheel and spins it. The landed wedge's contents go into the truck, and the wedge leaves the wheel for the rest of the trip.
2. If the load spoils (see "Bust rules" below), the trip ends and scores nothing. A fix, if the player has one, can stop that.
3. Otherwise the player either **spins again** (any wheel) or **delivers**. Delivering scores the load and ends the trip.

A round is a fixed number of trips to reach the score target, as in `rounds.md`. Wedges return to the wheels after each trip.

## What wedges carry

| Resource | Does | Example |
| --- | --- | --- |
| **Cargo** | Has a color and a value, and adds to the load | R4 |
| **Spoilage** | Pushes the load toward spoiling | a cracked crate |
| **Tarp** | Cancels one thing that would spoil the load | Flip 7's Second Chance |
| **Booster** | Multiplies or adds to the load if it's delivered | ×2 crate |

The truck is visible the whole time, and so are the wedges left on each wheel. The player can count exactly which spins would spoil the load. The risk is open information, which keeps the decision readable.

## Bust rules

Two candidate rules. `prototypes/haul_odds.py` measures them on a starting bag (2026-11-08). Its load score is the sum of the values × (1 + 0.5 per item), so pressing on is worth more.

- **Duplicate (Flip 7).** Landing a value already in the truck spoils the load. Six different values fill the truck, and a full truck pays a bonus and delivers itself. Colors never bust, so they stay free for synergies.

  | Stop after | 1 | 2 | 3 | 4 | 5 |
  | --- | --- | --- | --- | --- | --- |
  | Spoiled, no tarps | 0% | 13% | 37% | 64% | 85% |
  | Mean score, no tarps | 5 | 12 | **17** | 15 | 9 |
  | Mean score, 2 tarps in the bag | 5 | 13 | 18 | **19** | 14 |

  The risk climbs sharply and peaks early. Each tarp pushes the best stopping point about one spin later.

- **Spoilage (Quacks).** Hazard wedges add 1–3 spoilage, and above the truck's tolerance the load spoils. With 4–6 hazards and a tolerance of 4, the risk rises slowly: 8% spoiled at 3 cargo, 44% at 8. The mean score keeps rising past 8, so the numbers need tuning before pressing on is ever a mistake. It's gentler and easier to tune than the duplicate rule, but the tension is weaker.

## How the rest of the design fits

- **Tilting the bag** (`synergy.md`) changes job. Under the duplicate rule, a bag with fewer copies of each value is safer, so thinning becomes risk management. A mono-color bag stays safe, because colors don't bust. Under the spoilage rule, zapping hazards and adding tarps make the truck safer, and items can raise the tolerance.
- **Wheels get a new job.** The player chooses which wheel to spin next. Wheels can differ in risk and reward, such as a safe wheel with small cargo next to a risky wheel with boosters. That replaces the left-to-right order, which matters less under holders.
- **Consumables are the fixes.** A respin or a nudge on the spin that would spoil the load is exactly the moment the design wants (`synergy.md`). Tarps are the fix that comes from the wheel itself.
- **Compounding.** Score grows faster than the load. Items add more on top, such as "+mult per red cargo delivered" or "deliveries of 5+ pay ×2". A point factory is a build that can safely carry big loads: tarps, a thinned bag, more tolerance, and multipliers for big deliveries.
- **First round and last round.**
  - **Round 1:** the first spin of a trip never spoils under the duplicate rule, and the second spoils 13% of the time. A target that two-spin trips can reach keeps round 1 nearly unlosable, unless the player presses recklessly.
  - **Late rounds:** the targets need loads that are only reachable with an engine and some luck pressed.
- **Economy.** A spoiled load could still pay a little money, as exploding in Quacks still pays something. That softens a bust without removing it.

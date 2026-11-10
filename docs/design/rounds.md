# Rounds

A round works like a blind in Balatro: the player has a fixed number of trips to reach a score target.

## A round

- **Trips:** 4 per round. Each trip is a haul (`scoring.md`), with a tank of 10 spins.
- **Target:** the deliveries of the round add up toward a score target. Targets rise round by round, from 24 in round 1 to 2,900 in round 12. The curve lives in a data table with the rest of the balance numbers.
- **Unused trips:** a round ends as soon as the target is reached. Each trip left over pays $2 (`economy.md`), so finishing early with big loads earns money for the shop.
- **Quota:** each round has a quota, which doubles any load that reaches it (`scoring.md`).

## The bag and the wheels

- **The bag:** the player owns a bag of wedges. A run starts with 12 cargo wedges: values 1 to 6 twice over, three of each color.
- **Wheels:** the player owns their wheels, and buys more in the shop. A run starts with 4 plain wheels that hold 3 wedges each, and a player can own up to 6. Each wheel has:
  - **a size:** how many wedges it holds; and
  - **an effect:** what it does to the cargo it lands, or to the spin. Examples: "+1 to the value of cargo it lands", and "a repeat landed here is thrown out".
- **The deal:** at the start of each round, the wedges in the bag are dealt onto the wheels at random, face up. The same build plays out differently from round to round, and the player reads the wheels before the first spin.
  - If the bag is bigger than the wheels, some wedges sit the round out.
  - If it's smaller, the leftover slots hold empty wedges.
- **Choosing a wheel** is the player's move on every spin. Wheels differ in what they hold and how risky they are right now, so the choice is where the skill is.

## Inspections

Every third round is an inspection: a round with one extra rule. The player sees it two shops ahead (`progression.md`).

| Inspection | Rule |
| --- | --- |
| **Weigh Station** | The quota is 25% higher. |
| **Fuel Shortage** | The tank holds 2 fewer spins. |
| **High Winds** | Tarps don't work. |

Each one leans on a different kind of build, so a run can't rely on a single trick.

## Losing

Missing a round's target ends the run. Because a run can end at any round, choosing when to press and when to deliver matters, and each run stays short enough to replay.

## The shape of a run

A run should feel like this (`session-rubric.md`):

- **Round 1 is almost impossible to lose,** unless the player takes an insane risk. Two or three cargo per trip is plenty, and a first spin can never spoil a load.
- **By the last rounds it's almost impossible to win without a point factory:** a build that can carry big loads, or make each load pay far more.

In simulation, sensible bots lose round 1 less than 1% of the time, and a bot that never delivers by choice loses it two times in three. Bots with no point factory win at most 2% of runs, and the best point factory wins 42% (`docs/research/07-haul-design-experiments.md`).

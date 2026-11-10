# Content

Everything the player can own, with the prototype's numbers (`prototypes/haul/tables.json`). This is the starter set the balance was tuned on, not the final count: a release wants roughly three times as many items.

Three rules shape all of it (`synergy.md`, `economy.md`):

- **It pays on broad tags,** never on a named companion.
- **It never chooses a spin's result.**
- **Rare means powerful and narrow,** not better.

## Wedges

| Wedge | Price | In the truck |
| --- | --- | --- |
| Cargo, any color, value 1–6 | $2 | Points equal to its value; one more cargo |
| Tarp | $3 | Covers one repeat |
| Crate | $4 | ×1.5 to the load |

## Upgrades for a wedge

| Upgrade | Rarity | Does |
| --- | --- | --- |
| Recut | Common | Change the wedge to any value from 1 to 6 |
| Paint | Common | Change the wedge to a color you choose |
| Premium | Common | +1 to the wedge's value, up to 9 |
| Wild Color | Uncommon | The wedge counts as any color |
| Stamp | Uncommon | +3 points to the wedge |

Zap ($2) removes a wedge from the bag.

## Wheels

| Wheel | Rarity | Holds | Effect |
| --- | --- | --- | --- |
| Plain | Common | 3 | – |
| Gilded | Common | 3 | +3 points per cargo it lands |
| Bump | Common | 3 | +1 to the value of cargo it lands |
| Wide | Uncommon | 5 | – (safer to re-spin) |
| Express | Uncommon | 3 | Its first spin each trip uses no fuel |
| Padded | Rare | 3 | A repeat landed here is thrown out |

## Items

Always on, and applied in the order the player sets.

**Common: simple and additive**

| Item | Does |
| --- | --- |
| Red, Green, Blue, Yellow Ribbon | +1 mult per cargo of that color |
| High Roller | +3 points per cargo worth 5 or more |
| Odd Fellow, Even Steven | +1 mult per odd (or even) cargo |
| Lucky Coin | +2 mult |
| Heavy Purse | +6 points |
| Color Guard | +4 mult with 3 or more of a color |
| Step Counter | +4 mult with a run of 3 or more values |
| Tip Jar | +$1 for each delivery that reaches the quota |

**Uncommon: conditions and tools**

| Item | Does |
| --- | --- |
| Spare Tarp | One free tarp each round |
| Thermos | +2 to the tank |
| Long Haul | +3 mult per cargo past the third |
| Convoy | ×2 for loads of 5 or more cargo |
| Prism | ×2 with 4 or more of a color |
| Ladder | ×2 with a run of 4 or more values |
| Daredevil | +4 mult for each cargo landed at 25% or more to spoil |
| Insurance | A spoiled load still pays 30% |
| Overtime | The quota pays ×3 |
| The Rod | After a delivery, you may remove one delivered wedge from the bag |
| Piggy Bank | +30% interest on money held, up to $10 |
| Spendthrift | +50% of money held each round; money left after the shop is lost |

**Rare: powerful and narrow**

| Item | Does |
| --- | --- |
| Assembly Line | Gains ×0.2 for good each time a delivery reaches the quota |
| Odometer | Gains +3 mult for good each time you deliver 5 or more cargo |
| Chromatic Engine | ×(1 + 0.5 for each cargo past two in your biggest color group) |
| Hazard Pay | ×2.5 if a tarp saved this load |
| Double Clutch | The first repeat in each load is thrown out |
| Big Rig | A full truck pays ×3 |

## Consumables

| Consumable | Rarity | Does |
| --- | --- | --- |
| Nudge | Common | When a spin would spoil the load, move the pointer one wedge |
| Respin | Common | When a spin would spoil the load, spin that wheel again |
| Outer Band | Uncommon | For one spin, wedges that would spoil the load are half as wide |

## Point factories

A point factory is a build whose score compounds. These are the kinds the content supports. In simulation, each of the first four wins 38–43% of runs when a bot builds toward it, and a bot that follows whatever the shop offers (41%) does about as well (`docs/research/07-haul-design-experiments.md`).

| Factory | Tilts the bag toward | Pays off through | Risk it takes |
| --- | --- | --- | --- |
| **Convoy** | Tarps, and more different values | Long Haul, Convoy, Odometer, Big Rig, Double Clutch, Padded Wheel | Long loads: more spins, each one a chance to spoil |
| **Weighbridge** | High values: Premium, Stamp, Bump and Gilded wheels | Overtime, Assembly Line, High Roller, Tip Jar | Pressing on to reach a rising quota |
| **Ladder** | Values spread from 1 to 9 | Step Counter, Ladder, and the load-size items | Needs the right values, not just any |
| **Paint shop** | One color: Paint, Wild Color | Ribbons, Color Guard, Prism, Chromatic Engine | Color is safe, so it leans on big loads to make sets |
| **Daredevil** | Crates | Daredevil, Hazard Pay, Insurance | Spins taken at bad odds, on purpose |

Money items (Piggy Bank, Spendthrift, Tip Jar) fit beside any of them.

## Content to rework

Telemetry from simulated runs flags these (`docs/research/07-haul-design-experiments.md`). They're prompts to look closer, not verdicts:

- **Odd Fellow, Even Steven and off-color Ribbons are weak on purpose.** They help in the early rounds and suit play without a factory, but a factory does better without them: when bots were made to buy them, the best factory's win rate fell from 45% to 38%. Making them stronger helped no-factory play most, so they stay as they are. Learning to pass on them is part of the game.
- **Piggy Bank** is rarely bought.
- **Premium and tarp wedges** are bought in more than 90% of winning runs. They're bag upgrades, not items, so they don't break the no-must-have rule, but more ways to do the same jobs would spread the choice.

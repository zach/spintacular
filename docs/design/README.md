# Spintacular: game design

Spintacular is a single-player roguelike for desktop PC about pressing your luck. You load a truck one wheel-spin at a time. Every spin makes the load worth more, and one bad spin spoils the lot. Between rounds you shop for wedges, wheels and items that let you carry more, and get paid more for it, until your rig is a point factory.

It sits in the line of Luck be a Landlord, Balatro and CloverPit (`../research/01-design-foundations.md`), with the push-your-luck heart of Flip 7 and Quacks of Quedlinburg.

## The game in one minute

- **A run** is 12 rounds. Miss a round's score target and the run is over.
- **A round** is 4 trips. Each trip, you spin wheels to load cargo into your truck, then deliver it for points.
- **The catch:** no repeats. Land a cargo value you already hold, or the same wedge twice, and the load spoils.
- **The pull:** a load twice as big pays about four times as much, a load that reaches the round's quota pays double, and every cargo past the third pays a dollar.
- **The shop** comes between rounds. You reshape your bag of wedges, add wheels, and collect items that multiply what a delivery pays.
- **The arc:** round 1 is almost impossible to lose. The last rounds are almost impossible to win without a point factory.

## Design pillars

1. **One decision, always live: one more spin, or deliver?** Every rule either sharpens that decision or pays it off.
2. **You choose the odds; the wheel chooses the result.** The player can see every wedge and every chance. Nothing in the game picks an outcome.
3. **Tilt, don't complete.** Builds lean the game toward broad tags. No item needs a named partner, and the shop never has only one thing you need.
4. **Risk never reaches zero.** However good the build, a wedge that lands twice spoils the load.
5. **Numbers you can follow.** Every payout plays out step by step.

## Layers

Each layer is its own way to build.

| Layer | What it is | Its job |
| --- | --- | --- |
| **Wedges** | A bag of cargo, tarps and crates, dealt onto the wheels each round | Decide what can land, and how often a spin repeats |
| **Wheels** | Owned; the player chooses which to spin | Shape what lands on them, and how risky a spin is |
| **The haul** | The middle layer where scoring happens | Load the truck spin by spin; deliver before it spoils |
| **Items** | Always on, in an order the player sets | Multiply what a delivery pays |
| **Consumables** | One use: Nudge, Respin, Outer Band | Rescue a load, or better the odds, at the right moment |

## Documents

| Document | Covers |
| --- | --- |
| `scoring.md` | The haul: trips, the no-repeats rule, what a delivery pays, the quota |
| `rounds.md` | Rounds, the bag and the wheels, inspections, losing |
| `synergy.md` | Tilt the game, don't complete a set; consumables |
| `economy.md` | The shop, prices, income, rarity |
| `content.md` | Every wedge, upgrade, wheel, item and consumable; the kinds of point factory |
| `progression.md` | Routes, rigs, stakes, unlocks, seeds |
| `presentation.md` | What the player sees and hears |
| `tech.md` | How the game is built: C++ with Automap |
| `session-rubric.md` | How to judge a run, by simulation and by playtest |

Research is in `../research/`. The prototypes are in `../../prototypes/`: `haul/` simulates the current design and checks the rubric, and `web/` is a playable browser version of it.

## What's tested and what isn't

- **Tested by simulation:** the haul's rules, the round structure, the economy's numbers and the starter content. Together they pass all ten checks of the session rubric. The six rigs and the eight stakes are tested too (`../research/07-haul-design-experiments.md`).
- **Checked by simulation as well:** seeds (the same seed replays the same run, and rerolling the shop changes neither the deal nor the spins) and the starter content set (`progression.md`).
- **Played by its author, not yet by anyone else:** the opening of a run was played by hand, which found one gap and led to the freight bonus (`../research/07-haul-design-experiments.md`). The browser prototype (`../../prototypes/web/`) puts the presentation rules on screen: every wedge face up, each wheel's chance to spoil, the quota line, and step-by-step payouts. It also has the start screen, rigs, stakes, the daily haul, unlocks and the collection. It has no sound, and no art beyond shapes and color.
- **Not testable by simulation:** whether it's fun. The rubric's playtest scorecard is for that, and the browser prototype is the thing to put in front of a person.

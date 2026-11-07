# Spintacular design

Spintacular is a single-player, desktop-first roguelike built on compounding wheel spins. Each round the player spins a row of wheels to beat a rising score target, then shops between rounds.

## Layers

Each layer is its own way to build.

| Layer | What it is | Its job |
| --- | --- | --- |
| **Wedges** | Dice in a bag, drawn onto the wheels each round | Decide which dice can appear |
| **Wheels** | An owned row the player arranges | Shape the dice they add |
| **Holders and wall** | The middle layer where scoring happens | Dice fill lines of 3–5; full lines score and place markers on a run-long wall |
| **Items** | Always on, in an order the player sets | Act on every spin |
| **Consumables** | One use: respins, nudges, outer bands | Improve the odds at the right moment |

## Documents

- `rounds.md`: rounds, spins, the bag and wheels.
- `scoring.md`: holders, the wall, wedges and their upgrades, and resolution order.
- `synergy.md`: tilt the game, don't complete a set; consumables.
- `economy.md`: the shop's spending strategies, rarity, and tuning defaults.
- `session-rubric.md`: how to judge a run: a safe first round, and a wall at the end that only a point factory gets over.

Research is in `../research/`. The prototypes are in `../../prototypes/`: `dice_patterns.py` measures pattern odds, and `run/` plays whole runs and checks the rubric.

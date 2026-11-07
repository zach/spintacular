# Spintacular design

Spintacular is a single-player, desktop-first roguelike built on compounding wheel spins. Each round the player spins a row of wheels to beat a rising score target, then shops between rounds.

## Layers

Each layer is its own way to build.

| Layer | What it is | Its job |
| --- | --- | --- |
| **Wedges** | Dice in a bag, drawn onto the wheels each round | Decide which dice can appear |
| **Wheels** | An owned row the player arranges | Shape the dice where they fall in the sequence |
| **Dice sequence** | The middle layer where scoring happens | Runs of 3–5 adjacent dice score |
| **Items** | Always on, in an order the player sets | Act on every spin |
| **Consumables** | One use: respins, nudges, outer bands | Improve the odds at the right moment |

## Documents

- `rounds.md`: rounds, spins, the bag and wheels.
- `scoring.md`: the dice layer, wedges and their upgrades, pattern families, and resolution order.
- `synergy.md`: tilt the game, don't complete a set; consumables.
- `economy.md`: the shop's spending strategies, rarity, and tuning defaults.

Research is in `../research/`, and the pattern-odds prototype is in `../../prototypes/`.

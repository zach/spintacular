# Between-round economy

The shop between rounds is a decision space, not a checklist. The player picks among a handful of spending strategies and switches between them as results come in.

## Strategies the economy supports

- **Reroll hunting.** Pay to refresh the shop and look for a good deal.
- **Big buy.** Save up, spend essentially everything on hand, or sell items down to afford one high-value item.
- **Wait.** Spend only on essentials now, expecting better options later.
- **Bargain hunting.** Min-max spending to pick up cheap but useful items.
- **Cash play.** Spend down every round or save, to make the most of money-related power-ups. Examples:
  - Gain 50% of your currency each round, but unspent currency can't be carried over.
  - Earn 30% interest next round on currency left unspent.

Each strategy should be the best choice in some situations and a poor one in others, so that switching between them is a real decision.

## Rules

- **No store-item synergies that force a purchase.** A strategy never requires spending all your money on one particular store item. That takes the fun out of shopping. Synergies tilt the game toward tags many items share, rather than needing a companion item (see `synergy.md`).
- **Shopping never feels forced.** There is never only one thing the player needs.

## Rarity

Rare items aren't inherently good. They have a more powerful effect on scoring, which helps some builds and does nothing for others. So the shop has no rarity floor and no pity timer: a run without rare offers isn't a run of bad luck.

## Tuning defaults

These are numbers to tune, not decisions. They live in data tables with the rest of the balance numbers (see `docs/research/01-design-foundations.md` §1).

- Interest has a cap per round, so saving can't snowball into unbeatable runs. Balatro also caps its interest.
- The reroll price goes up within a shop visit and resets at the next visit.
- Selling returns part of an item's price, so selling down to afford something has a cost.

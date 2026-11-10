# Between-round economy

The shop between rounds is a decision space, not a checklist. The player picks among a handful of spending strategies and switches between them as results come in.

## Strategies the economy supports

- **Reroll hunting.** Pay to refresh the shop and look for a good deal.
- **Big buy.** Save up, spend essentially everything on hand, or sell items down to afford one high-value item.
- **Wait.** Spend only on essentials now, expecting better options later.
- **Bargain hunting.** Min-max spending to pick up cheap but useful things.
- **Cash play.** Spend down every round or save, to make the most of money-related items:
  - **Spendthrift** gives 50% of the money held each round, but money left after the shop is lost.
  - **Piggy Bank** pays 30% interest on money held.

Each strategy should be the best choice in some situations and a poor one in others, so that switching between them is a real decision.

## Rules

- **No store-item synergies that force a purchase.** A strategy never requires spending all your money on one particular store item. That takes the fun out of shopping. Synergies tilt the game toward tags many things share, rather than needing a companion item (`synergy.md`).
- **Shopping never feels forced.** There is never only one thing the player needs.

## Rarity

Rare items aren't inherently good. They have a more powerful effect on scoring, which helps some builds and does nothing for others. So the shop has no rarity floor and no pity timer: a run without rare offers isn't a run of bad luck.

Rarity also sets *when* things appear. The first shops offer only common, simple things, and complexity arrives as the run goes on:

| From round | Common | Uncommon | Rare |
| --- | --- | --- | --- |
| 1 | 100% | – | – |
| 3 | 70% | 30% | – |
| 5 | 50% | 32% | 18% |
| 8 | 40% | 32% | 28% |

## The shop

- **Offers:** 5 at a time, drawn from six kinds:

  | Kind | Share of offers | Price |
  | --- | --- | --- |
  | Item | 36% | $4 common, $6 uncommon, $9 rare |
  | Upgrade for a wedge | 20% | $3 common, $5 uncommon |
  | Wedge | 16% | $2 cargo, $3 tarp, $4 crate |
  | Wheel | 12% | $5 common, $7 uncommon, $10 rare |
  | Consumable | 10% | $2 common, $3 uncommon |
  | Zap (remove a wedge) | 6% | $2 |

- **Reroll:** $4, and $1 more each time within a visit. The price resets at the next visit.
- **Selling:** an item sells for half its price, so selling down to afford something has a cost.
- **Limits:** 5 items, 6 wheels and 2 consumables. The limits force choices: a sixth good item means selling one.

## Income

| Source | Pays |
| --- | --- |
| Winning a round | $5 |
| Each trip left unused | $2, up to $4 |
| Interest | $1 for every $5 held, up to $5 |

A run starts with $4. Interest is capped, as in Balatro, so saving can't snowball.

## Tuning

These numbers are the prototype's, and they pass the session rubric together (`docs/research/07-haul-design-experiments.md`). They live in data tables so they can keep changing: most balance changes are one number.

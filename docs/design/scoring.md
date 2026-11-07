# Scoring: the dice layer

Scoring happens in a middle layer between the wheels and the score. Spinning the wheels produces a sequence of dice. Each die has one of four colors and a value from 1 to 6. The score comes from patterns in that sequence: runs of three, four and five adjacent dice. A pattern only counts dice that sit next to each other in the sequence, never dice scattered through it.

The dice layer is where scoring variations get tried. The wheels and the shop sit above it, and the patterns and their payouts below it can change while the rest stays the same. `prototypes/dice_patterns.py` measures how often candidate patterns come up (`docs/research/02-dice-pattern-odds.md`).

Adjacency is what keeps patterns exciting. Patterns allowed anywhere become certain once there are enough dice: with four colors, any 9 dice hold 3 of one color. Adjacent runs stay graded, with 3 common, 4 uncommon and 5 rare (`docs/research/02-dice-pattern-odds.md`). Adjacency also makes the order of the wheels matter.

## Pattern families

These are the families to try first. Their odds are in `docs/research/02-dice-pattern-odds.md`.

| Family | Rule | Runs of 3 in 10 dice |
| --- | --- | --- |
| Color run | adjacent dice of one color | 35% |
| Value run | adjacent dice of one value | 18% |
| Straight | adjacent values going up by 1 | 13% |
| Color straight | a straight in one color | 1% |

Color is the common way to score. Values give rarer patterns and a jackpot, and more directions to build toward.

## Resolution order

Every spin resolves in a fixed order. The player can see the order and rearrange it.

1. The wheels spin left to right. Each adds its dice to the end of the sequence.
2. The patterns in the sequence score.
3. Always-on items apply, in the order the player arranged them.

The player arranges the wheels and the items, so building a payout is a skill. For example, putting additive bonuses ahead of multipliers can multiply a score: in Balatro, rearranging jokers that way doubled one example hand (`docs/research/01-design-foundations.md` §1).

The game shows a payout step by step in this same order, so the player can see how each piece of their build contributed.

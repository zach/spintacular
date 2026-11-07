# Scoring: the dice layer

Scoring happens in a middle layer between the wheels and the score. Spinning the wheels produces a sequence of dice, each in one of four colors. The score comes from patterns in that sequence: subsequences of three, four and five dice.

The dice layer is where scoring variations get tried. The wheels and the shop sit above it, and the patterns and their payouts below it can change while the rest stays the same. `prototypes/dice_patterns.py` measures how often candidate patterns come up (`docs/research/02-dice-pattern-odds.md`).

## Resolution order

Every spin resolves in a fixed order. The player can see the order and rearrange it.

1. The wheels spin left to right. Each adds its dice to the end of the sequence.
2. The patterns in the sequence score.
3. Always-on items apply, in the order the player arranged them.

The player arranges the wheels and the items, so building a payout is a skill. For example, putting additive bonuses ahead of multipliers can multiply a score: in Balatro, rearranging jokers that way doubled one example hand (`docs/research/01-design-foundations.md` §1).

The game shows a payout step by step in this same order, so the player can see how each piece of their build contributed.

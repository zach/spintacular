# Scoring: holders and the wall

Scoring happens in a middle layer between the wheels and the score. Spinning the wheels produces dice. Each die has one of four colors and a value from 1 to 6. The player places each die in a holder, and full holders score.

The middle layer is where scoring variations get tried. The wheels and the shop sit above it, and the layer below them can change while the rest stays the same. Three layers have been compared so far:

- runs of adjacent dice;
- a path traced through recent spins;
- holders.

Holders were the strongest once spins were kept random (`docs/research/05-scoring-layer-comparison.md`). The reason is that the player decides where each result goes, so a bad spin can be worked around (`docs/research/04-scoring-mechanisms.md`).

## Holders

- **Lines:** there are lines of three, four and five dice. A line takes its type from the first die placed in it: a **color line** (one color), a **value line** (one value) or a **straight line** (consecutive values, going either way).
- **Placing dice:** each die goes into a line that accepts it, or onto the **floor**. Dice on the floor cost a few points each, and the cost rises with each die.
- **Scoring a line:** a full line scores its pattern (see the table below) and empties. Lines carry over between spins within a round.
- **Leftovers:** the floor is the release valve for dice that fit nowhere. Leftover dice are the main risk of holders (`docs/research/04-scoring-mechanisms.md`), so the floor's cost stays small and capped.

| Line | Fills with | Size 3 / 4 / 5 |
| --- | --- | --- |
| Color | dice of one color | common payouts |
| Value | dice of one value | rarer, bigger payouts |
| Straight | consecutive values | rarest, biggest payouts |

## The wall

The wall is a board that lasts the whole run. It has a row for each color and a column for each value.

- **Placing a marker:** when a line fills, the die that filled it places a marker on its color-and-value cell, if that cell is free.
- **Scoring a marker:** it scores the way an Azul tile does. A lone marker scores 1. Otherwise it scores the length of the unbroken chain it joins in its row plus the length of the one in its column. The score is paid as mult.
- **What the wall rewards:**
  - **Rows:** consecutive values in one color, so straights build up over a run.
  - **Columns:** one value in many colors.
- **Aiming:** a player can hold a line one short and wait for a die whose marker would join a chain. In simulation, aiming lifted the best point factory from 37% of wins to 43% (`docs/research/05-scoring-layer-comparison.md`).

## Wedges

Each wedge starts as one fixed die: landing on it adds that die's color and value. The player can see exactly what each spin might add. What a wedge produces can change during a run, and tilting the game (`synergy.md`) works by editing wedges. The first upgrades are:

- **Second die:** the wedge adds two dice, so lines fill faster.
- **Wild color:** the wedge's die counts as any color.
- **Value range:** the wedge's value is rolled within a range, such as 3–5, instead of being fixed.

Each one improves the odds without guaranteeing anything, because the wheel still has to land on the wedge.

## Resolution order

Every spin resolves in a fixed order. The player can see the order and rearrange it.

1. The wheels spin left to right. Each hands over its dice in turn.
2. The player places each die in a line or on the floor.
3. Full lines score, and place their markers on the wall, which score too.
4. Always-on items apply, in the order the player arranged them.

The player arranges the items, so building a payout is a skill. For example, putting additive bonuses ahead of multipliers can multiply a score: in Balatro, rearranging jokers that way doubled one example hand (`docs/research/01-design-foundations.md` §1).

The game shows a payout step by step in this same order, so the player can see how each piece of their build contributed.

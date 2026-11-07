# Rounds

A round works like a blind in Balatro. The player has a fixed number of spins to reach a score target.

- **Wheels:** the player owns their wheels; they aren't drawn. Each is bought in the shop and kept in a row the player arranges. The row stays the same from round to round, and its left-to-right order is the order dice are handed over for placement (`scoring.md`). Each wheel has two attributes:
  - **Size:** how many wedges it holds.
  - **Effect:** one effect on the dice it adds, such as "+1 to its dice's values" or "its dice count double in color lines".
- **Full wheels:** every wheel always holds its full number of wedges. Slots the bag can't fill hold blank wedges, which add no die. So spins stay random however thin the bag gets, and thinning has a cost: past a point, removing a wedge means more blanks rather than more of what you want.
- **The bag:** the player owns a bag of wedges. At the start of each round, the wheels' wedges are drawn from the bag, the way Balatro deals a hand from a deck. The same build plays out differently from round to round. The shop adds wedges to the bag, and removing a wedge from the bag for good is how the player tilts it (`synergy.md`).
- **Spins:** each spin turns every wheel once, left to right. The dice they add form that spin's sequence, which scores as described in `scoring.md`.
- **Target:** the scores of the round's spins add up toward the target. Targets rise round by round. The curve lives in a data table, with the rest of the balance numbers.
- **Unused spins:** spins left over when the target is reached pay a bonus into the economy (`economy.md`). That makes spins a resource to manage: finishing early earns money for the shop.
- **Consumables:** respins and nudges (`synergy.md`) are meant for the spin that decides the round.

## Losing

Missing a round's target ends the run. Because a run can end at any round, choosing when to save and when to spend matters, and each run stays short enough to replay.

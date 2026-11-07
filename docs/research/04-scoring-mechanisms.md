# 04 — Scoring mechanisms for the middle layer

**Scope.** Options for the scoring layer between the wheels and the score (`docs/design/scoring.md`). The current layer is a sequence of dice, scored on runs of adjacent dice. The options considered are:

- drafted objective cards, as in Ticket to Ride;
- fixed-size pattern holders, as in Azul;
- dice placement and roll-and-write;
- spatial scoring and tableaus;
- set collection and poker-hand categories;
- replacing dice with other elements.

Each option is judged on four things: agency over random results, compounding, build variety, and readability. The prototype's problem with straights (`03-first-balance-pass.md`) is the motivating case.

Accessed: 2026-11-07

**Method.** The deep-research workflow searched six angles and fetched 24 sources. It extracted 102 claims and took 25 of them to verification, where each got three adversarial votes and needed two refutes to be dropped. 22 survived and 3 were refuted; the rest were dropped for budget. The board-game rules claims were checked against rulebooks. Most claims about how a mechanism *feels* rest on one or two designer anecdotes.

Each finding is marked **[Verified]** (survived 3-0) or **[Inference]** (what it suggests for Spintacular).

**Gaps.** No verified evidence came back on Ganz Schön Clever, Roll Player, Yahtzee categories, polyomino scoring, or a direct study of non-dice intermediate elements. What §3 says about them is inference.

---

## Key findings

- **[Verified] Random results feel good when the player decides where each one goes, and bad when they fire on their own.** Terry Cavanagh replaced a Witch spellbook that fired spells automatically on dice conditions, because it felt like "a random avalanche of events". Letting players choose made it "a sequence you were planning". He also found that "any kind of dice manipulation that's predictable was pretty fun" [1].
- **[Verified] Leftovers are the main risk of fixed-size holders.** Cavanagh dropped a 3×3 dice grid for the Witch partly because about half the dice couldn't be placed: "Having dice left over is the worst feeling in the game" [1].
- **[Verified] Azul turns random draws into placement decisions, and contiguity compounds.** Here is how it works [2][3][4]:
  - **Pattern lines** hold one tile type each, and only a completed line moves a tile to the wall.
  - **Overflow** goes to a floor line that costs -1, -1, -2, -2, -2, -3, -3, but a score never drops below zero.
  - **Wall scoring:** an isolated tile scores 1. A connected tile scores 1 per contiguous tile in its row, plus the same again for its column.
  - **Carry-over:** unfinished lines carry over to the next round.
- **[Verified] Sagrada layers objectives over a placement grid.** Its window grid restricts each space by color or value, and adjacent dice can't share either. One private objective and several public objectives score on top. Each empty space costs -1 [5][6]:
  - Tool cards, paid for with limited favor tokens, are the main way to soften luck.
  - Reviewers find the choices tactical more than strategic, and some find the restrictions frustrating.
- **[Verified] Drafted objective cards add risk and reward, and get cheaper as you build.** Ticket to Ride penalizes destination tickets left unfinished. A ticket that fits your existing network pays easy points. That compounding lowers costs and adds points; it doesn't multiply them [7]. The claims putting numbers on how much it compounds were refuted.
- **[Verified] Familiar categories make scoring readable.** LocalThunk chose poker hands so players could understand Balatro through vocabulary they already knew. The structure of playing hands came from Big Two [8].
- **[Verified] Roguelikes have already adopted spatial board-game scoring:**
  - **Pokermancer** has the player trace a path through a 4×4 grid of cards to form a hand. Position is "half the game", and items between rounds change how hands score [9].
  - **Backpack Hero** uses the backpack grid as its play area: items depend on placement, rotation and adjacency [10].
- **[Verified] Put variety into always-on items, not into the random pool.** The designer of Luck be a Landlord makes forced strategies work less often, and argues that relics would be worse as cards in the deck [11].
- **[Verified] Low rolls need a use of their own,** and choosing among outcomes feels like agency [12].

**Refuted (0–3):**
- Drawing random train cards is the dullest part of Ticket to Ride.
- Specific trunk-route overlap figures for the USA map.
- An 86.2% chance that a ticket draw contains a nearly complete ticket.

---

## 1. Where agency comes from

**[Verified]** Agency comes from three things: where a result goes [2][5], the order results are read in [9], and choosing among outcomes [12]. Automatic triggers on random conditions read as noise [1].

**[Inference]** Spintacular's current layer gives the player no say after the spin. The wheel order fixes the order dice land in, so the dice sequence is read exactly as it lands. Tilting the bag shapes *which* dice come up, but never *where they go*. That's why color and value runs can be built toward and straights can't: a straight needs order, and nothing controls order. Any of three changes fixes it:

1. **Reading order.** The player traces a path through the spin's results, as in Pokermancer, or reorders them.
2. **Placement.** Each result goes into a holder the player picks, as in Azul and Sagrada.
3. **Selection.** The player keeps some results and banks or discards the rest.

None of the three guarantees an outcome: the spin still decides what the player has to work with. So all three fit `synergy.md`.

## 2. The options

| Option | Agency over the spin | How it compounds | Build variety | Readability | Main risk |
| --- | --- | --- | --- | --- | --- |
| **Adjacent runs (current)** | None after the spin | Run length × item multipliers | Color and value only; straights can't be built | High | Straights are dead; no decisions during a round |
| **Path or reorder** (Pokermancer) | Choose the read order | As now, but straights become buildable | Adds straights back | High, since the path is visible | Taking the time to find the best path; solvable by a strong player |
| **Pattern holders** (Azul) | Choose which holder each result fills | Filled holders feed an adjacency board that scores again and again, with multipliers on top | Holder shapes and board bonuses become build axes | Medium: two stages, fill then score | Leftovers [1]; needs a release valve |
| **Constrained grid** (Sagrada) | Placement under rules | Objectives plus a full grid | Objectives define builds | Medium | Restrictive and frustrating [6]; tactical, not strategic |
| **Objective cards** (Ticket to Ride) | Choose which goals to hold | Cheaper with each goal met, but only additive [7] | Strong, if goals key off tags | High, since each goal is a sentence | Forced hunting for the cards a goal needs; can clash with "never forced" |
| **Categories** (Yahtzee, poker hands) | Choose which category a result scores in | Category levels scale, as with Balatro's planets | Category upgrades are build axes | Very high, because the vocabulary is familiar [8] | Categories run out; each can be used only once |
| **Non-dice elements** (tiles, symbols) | Depends on the scoring rule | Depends | Elements can carry several tags | Depends | No evidence gathered |

## 3. Notes on each option [Inference]

- **Path or reorder** is the smallest change that fixes straights. Results could be laid out in a short grid, such as wheels × spins this round, and read along a path the player traces, as in Pokermancer. Or each spin could give a few swaps. It keeps everything already decided. It could also give the nudge consumable a twin: a swap.
- **Pattern holders** are the biggest change and the most promising for a point factory. Each spin's dice go into lines of size 3, 4 and 5, with one color or value per line. Completed lines move to a board that scores adjacency, and items multiply the board's score. A release valve keeps leftovers from feeling bad: a capped floor penalty as in Azul, holders that take anything at a lower rate, or low-value uses [12]. Round 1 stays safe if the floor can't cost a round on its own.
- **Objective cards** fit as an extra layer, not a replacement. A card might read "score a color run of 4+ three times this round". It keys off tags, never a named item, which fits `synergy.md`. Making cards optional and replaceable keeps them from forcing purchases. But they add rather than multiply, so they need the item layer to compound [7].
- **Categories** (a Yahtzee-like sheet each round) are readable and give a choice for every spin, but each category is used once. That works against compounding within a round.
- **Non-dice elements.** Dice give two tags (color and value) and a natural order (value). Tiles or symbols could carry more tags, such as suit, shape and element, which means more build axes and more ways to tilt. The cost is losing the "value 1–6" intuition that makes straights and sums easy to read. This option has no evidence behind it yet. It's cheap to try in the prototype, because the dice layer is meant to be swapped (`scoring.md`).

## 4. Next steps for the prototype

The scoring layer is swappable, so the prototype can test variants against the same rubric (`docs/design/session-rubric.md`). Three variants cover the range: **path or reorder** (smallest change), **pattern holders with a floor** (biggest change), and **objective cards on top of either**. The rubric's checks C1 (straights viable?) and E1/E2 (shopping not forced) are where they would differ.

---

## Sources

1. Wiltshire, "Witch craft: how Dicey Dungeons balances chance and predictability", *Game Developer*, 2018-09-19 (quotes Terry Cavanagh). <https://gamedeveloper.com/design/witch-craft-how-i-dicey-dungeons-i-balances-chance-and-predictability> (secondary, designer quotes)
2. Dale Yu, "Review of Azul", *Opinionated Gamers*, 2017-10-03. <https://opinionatedgamers.com/2017/10/03/dale-yu-review-of-azul/> (secondary)
3. "Azul review", *Meeple Mountain*. <https://www.meeplemountain.com/reviews/azul-review-tile-your-way-to-victory/> (secondary)
4. Azul rulebook (Plan B Games, 2017), hosted by Dover Public Library. <https://www.dover.nh.gov/Assets/government/city-operations/library/borrow/library-of-things/azul-rulebook.pdf> (primary)
5. "Sagrada (2017)", *Meeple Like Us*. <https://www.meeplelikeus.co.uk/sagrada-2017/> (secondary)
6. "Sagrada game review", *The Board Game Family*, 2017-09. <https://www.theboardgamefamily.com/2017/09/sagrada-game-review/> (secondary)
7. J. L. Ericson, "Ticket to Ride review", 2020-05-26. <https://jlericson.netlify.app/2020/05/26/ticket-to-ride-review> (blog)
8. "I don't play poker at all, says solo developer…", *PC Gamer*, 2024. <https://www.pcgamer.com/i-dont-play-poker-at-all-says-solo-developer-who-made-the-poker-roguelike-i-cant-stop-playing/> (secondary, developer quotes)
9. "Pokermancer iOS launch", *Pocket Gamer*, 2026. <https://www.pocketgamer.com/pokermancer/ios-launch/> (secondary)
10. *Destructoid* on Backpack Hero. <https://destructoid.com/?p=454085> (secondary)
11. Dan DiIorio, "Questions and Danswers", Trampoline Tales blog, 2023-05-29. <https://blog.trampolinetales.com/questions-and-danswers/> (primary)
12. Joe Slack, "10 ways to use dice in your board game other than roll-and-move", *Game Developer*, 2022. <https://www.gamedeveloper.com/game-platforms/10-ways-to-use-dice-in-your-board-game-other-than-roll-and-move-> (blog)

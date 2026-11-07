# 01 — Design foundations for Spintacular

**Scope.** Research to inform the design of Spintacular, a single-player, desktop-first, Balatro-style roguelike built on compounding wheel-of-fortune spins. Each round the player buys and draws wedges and wheels whose attributes synergize. Covers Balatro's scoring and its approach to balance, casino-game roguelikes, synergy and balance principles, randomness that feels fair but exciting, replayability through rarity and adaptive play, and a C++ library shortlist for an engine composed with Automap 0.1.0. Art style is open. Not covered: art direction, rendering techniques, monetization, platforms other than desktop PC.

Accessed: 2026-11-07

**Method.** Five search angles ran in parallel. 22 sources were fetched and 90 claims extracted from them. 25 claims got three independent adversarial verification votes each (it took 2 of 3 refutes to drop a claim): 23 survived and 2 were refuted. The surviving claims are merged into the findings below. Sources were ranked primary, secondary, blog, forum or unreliable, and nothing below rests on an unreliable source. The library shortlist was also checked against the Automap 0.1.0 public map (`zach/automap`, `map/`).

Each finding is marked **[Verified]** (survived 3-0) or **[Inference]** (what it suggests for Spintacular). Almost all design evidence is single-source designer testimony, which suits claims about design intent but is anecdote, not player data.

---

## Key findings

- **[Verified] The lineage is Luck be a Landlord → Balatro → CloverPit.** LocalThunk describes Balatro's central mechanic as scoring "as many chips as possible to defeat an ever rising required score" [1]. He named Luck be a Landlord, a slot-machine roguelike, as "the one largest influence" on Balatro [2]. CloverPit's developers call Balatro "a huge inspiration" [3]. Other influences they named: Big Two for Balatro, Buckshot Roulette for CloverPit.
- **[Verified] The order effects resolve in is something the player decides.** In Balatro, Joker effects trigger left to right unless stated otherwise. In one guide's example, putting additive +Mult before ×Mult turned 3,480 into 6,966. Chips also differed between the two layouts, so the comparison is not fully controlled. Per-card triggers and some Jokers are timing exceptions [4].
- **[Verified] Balatro tunes score targets with per-ante tables and tunes balance by feel.** Patch 1.0.1f made the early game easier and, on Green Stake, the late game harder. Green Stake antes 2–8 changed 1000→900, 3200→2600, 9000→8000, 18000→20000, 32000→36000, 56000→60000 and 90000→100000 [5]. LocalThunk: balance is tuned by feel, and "almost all balance changes can be made by changing a number" (cost, rarity, chip or mult value) [1].
- **[Verified] Telemetry complements tuning by feel.** Mega Crit logged every decision in Slay the Spire from the prototype stage on, because intuition could not balance its card pool, and grew from 3 graphs to at least 90. They named two metrics as most important: pick rate (how often a card is taken when offered) and presence in winning decks. Both are confounded and do not say how a card feels to play [6].
- **[Verified] Rarity follows run progress, and the design rewards adapting.** Luck be a Landlord follows Magic's "New World Order": the first few spins offer only Common (simple) symbols, and complex ones appear later. The game rewards using several strategies in one run and makes forced strategies, such as Cultists, work less often [7].
- **[Verified] Keep an always-on item layer apart from the random layer.** LBaL's designer sees items as "a way to add more variety and strategy without adding another symbol to the slot machine", and argues that Slay the Spire's relics would be worse as cards in the deck [7].
- **[Verified] Randomness feels fair when plans meet rolls and the player can see what happened.** In Dicey Dungeons, automatic triggers firing all at once felt like a "random avalanche" and were redesigned. When spells summoned equipment the player had chosen, the result felt like "a sequence you were planning". Terry Cavanagh calls the mix of planned equipment and random dice "fundamentally correct" [8].
- **[Verified] Pity timers and rarity floors cap streaks of bad luck, at a cost.** Commercial games guarantee a good outcome after a long run of bad luck, and Hearthstone guarantees a Legendary within 40 packs (published by Blizzard in 2023). Riot's Paul Sottosanti warns that once players notice the pseudo-randomness, it can undercut the joy of luck, and players may simply wait for the timer [9].
- **[Verified] CloverPit's strongest point is that its premise is clear at once.** Its developers say players immediately understand the game: break or cheat the slot machine for extra cash [3].
- **[Verified] miniaudio can be the whole audio layer.** It is a C library with no dependencies beyond the standard library. You compile `miniaudio.c` and add `-lpthread -lm` on Linux/BSD, and it supports CMake. Its high-level engine handles mixing, effects, optional 3D spatialization and a node graph. WAV, FLAC and MP3 decoding are built in; OGG needs stb_vorbis. Its ABI is not stable across versions [10]. It is in Automap's public map as the `audio` slot's library.
- **[Verified] EnTT fits, but Automap 0.1.0 maps flecs for the ECS.** EnTT is header-only, MIT-licensed, and installable with vcpkg [11]. Automap's public map lists flecs as the `ecs` slot's library and has no EnTT entry, so using EnTT means adding it through a private map.

**Refuted (0–3), and left out:** a claim that Balatro's first balance patch moved several Jokers between rarity tiers with matching price changes, and a claim that Civilization guarantees a win on the third try at 1-in-3 odds.

---

## 1. Scoring and the target curve

**[Verified]** Balatro's loop: score against a required total that rises every round (blind), with synergies that interlock and steer the build [1]. Joker effects resolve in a fixed order the player can rearrange [4]. Targets live in per-ante tables that differ by stake, and patches change single numbers in them [1][5].

**[Inference] For Spintacular** (the resolution order is now decided, in `docs/design/scoring.md`):
- Use a fixed resolution order: wedges in the order the pointer passes them, then wheels left to right, then always-on items. Additive bonuses should come before multipliers, so arranging the order is a skill.
- Keep timing phases few and named, like Balatro's split between per-card triggers and global triggers. For example: *on land*, *on pass*, *after spin*, *end of round*.
- Keep the target curve, prices, rarity weights and every effect's numbers in data tables. Balatro's patch history shows most tuning is changing a single number.
- Compounding is the point, so plan for numbers growing exponentially from the start. That means the score representation, display formatting and late-game targets.

## 2. Economy

**Not established by this research.** Balatro's interest, shop prices, reroll costs and selling were not verified: the one patch-economy claim was refuted, and the economy sources fetched were blogs or unreliable. Spintacular's economy is a design decision, in `docs/design/economy.md`.

## 3. Synergy, rarity and replayability

**[Verified]** Bring in complexity with run progress [7]. Reward mixing strategies, and make forced strategies less reliable [7]. Separate always-on items from what spins [7].

**[Inference] For Spintacular:**
- Use three layers: **wedges** (random; spin results land on them), **wheels** (structure: how many wedges, how many spins, how spins chain together) and **items** (always on, a Joker- or relic-like layer the player plans around).
- Offer only Common wedges in the first rounds. Unlock Uncommon and Rare by round or ante, so new players learn the basic attributes before seeing conditional or compounding ones.
- Make synergy attributes tags (color, suit, number, material, and so on), so the shop's random offers suggest builds instead of locking one in. Leave room for a pivot mid-run.
- Log pick rate and presence in winning runs for every wedge, wheel and item from the first prototype. Treat outliers as prompts to look closer, not verdicts [6].

## 4. Randomness that feels fair and exciting

**[Verified]** Planned pieces plus random rolls, with visible cause and effect [8]. Pity timers and rarity floors trade fairness against the joy of luck [9].

**[Inference] For Spintacular:**
- Give the player authorship. They chose the wedges and the wheel layout; the spin only picks among them. Offer agency after the spin as well, such as nudges, re-spins and choosing which wheel to cash in first.
- Resolve compounding chains one step at a time, with each step's contribution visible, so a big payout reads as *your build working*, not as a jackpot from nowhere.
- Spintacular has no rarity floor (`docs/design/economy.md` § Rarity). If one were used, it would belong in the shop (rare offers guaranteed every N rounds), not on spin outcomes, and decide whether it is visible. Hearthstone publishes its floor, and Sottosanti warns that hidden ones get noticed.
- Seed each run from one seed, with separate RNG streams for shop, spin and events. Then a run can be replayed and shared, and rerolling the shop does not change spin outcomes. This was not covered by a verified source.

## 5. Framing

**[Verified]** CloverPit is understood at once: break the machine for cash [3]. **[Inference]** Spintacular can frame compounding spins as *rigging the wheel*: a carnival or game-show wheel the player gradually turns into a machine for making money.

## 6. C++ library shortlist (Automap 0.1.0)

The Automap 0.1.0 public map has one library per slot. Only miniaudio and the ECS choice were adversarially verified; the rest are listed from the map as-is.

| Slot | Library | Status for Spintacular |
| --- | --- | --- |
| `windowing`, `input`, `gpu` | sdl3 | From map. SDL3 covers the window, input and SDL_GPU rendering. A 2D wheel and card game needs little from the GPU. |
| `shader-compiler` | sdl-shadercross | From map. Needed if rendering goes through SDL_GPU. |
| `audio` | miniaudio | **Verified** [10]. Covers spin, tick and payout sounds, and music. |
| `ecs` | flecs | From map. EnTT [11] is the alternative and needs a private-map entry. A game state this small may not need an ECS at all. |
| `math` | glm | From map. |
| `image-decode` | stb-image | From map. Decodes sprites and textures. |
| `tools-ui` | dear-imgui | From map. Use it for debug and tuning panels (live-editing the balance tables); it is not suited to the game's own UI. |
| `logging` | spdlog | From map. |
| `profiling` | tracy | From map. |
| `jobs` | enkits | From map. Probably unneeded early on. |
| `allocator` | mimalloc | From map. Optional. |
| `physics-3d`, `model-import` | jolt, fastgltf | Not needed for a 2D game. |

**Gaps [Inference]:**
- The 0.1.0 map has no font or text-rendering slot, and a score-heavy game needs one. SDL3_ttf or stb_truetype would need a private-map entry.
- The map has no tweening or animation library, which matters for the "juice" of spins.
- The map has no slot for serialization (save files, data-driven tables).

---

## Open questions

1. Text rendering and data serialization: which libraries, and add them through a private map or propose them for Automap's public map?
2. ECS: use flecs as mapped, add EnTT privately, or use plain structs for a game state this small?

## Sources

1. LocalThunk, interview, *Rogueliker*, 2024-03-07. <https://rogueliker.com/balatro-interview/> (primary)
2. "Balatro inspiration: Luck Be a Landlord", *Shacknews*, relaying LocalThunk's Reddit AMA. <https://shacknews.com/article/139116/balatro-inspiration-luck-be-a-landlord-reddit-ama> (secondary)
3. Panik Arcade interview, *Gamepressure*, 2025-11. <https://www.gamepressure.com/newsroom/many-aaa-titles-have-been-exploiting-players-cloverpit-developers/ze89e8> (secondary)
4. "How to order Jokers in Balatro", *Destructoid*. <https://www.destructoid.com/how-to-order-jokers-in-balatro/> (secondary)
5. "Balatro's first big balance patch…", *PC Gamer* (quoting patch 1.0.1f notes). <https://pcgamer.com/games/card-games/balatros-first-big-balance-patch-fiddles-with-fibonacci-and-makes-a-mod-to-odd-todd> (secondary)
6. "How Slay the Spire's devs use data to balance their roguelike deck-builder", *Game Developer* (GDC 2018). <https://gamedeveloper.com/design/how-i-slay-the-spire-i-s-devs-use-data-to-balance-their-roguelike-deck-builder> (secondary)
7. Dan DiIorio, "Questions and Danswers", Trampoline Tales blog, 2023-05-29. <https://blog.trampolinetales.com/questions-and-danswers/> (primary)
8. "Witch craft: how Dicey Dungeons balances chance and predictability", *Game Developer*, 2018-09-19. <https://gamedeveloper.com/design/witch-craft-how-i-dicey-dungeons-i-balances-chance-and-predictability> (secondary)
9. "How designers engineer luck into video games", *Nautilus*, 2017-01. <https://nautil.us/how-designers-engineer-luck-into-video-games-236363> (secondary)
10. miniaudio README. <https://github.com/mackron/miniaudio> (primary)
11. EnTT README. <https://github.com/skypjack/entt> (primary)
12. Automap public map, `map/slots/` and `map/libraries/`, `zach/automap` at the 0.1.0 map. (primary)

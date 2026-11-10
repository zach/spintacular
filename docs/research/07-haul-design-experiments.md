# 07 — Experiments that set the haul's design

**Scope.** This note covers the experiments behind the haul's rules and numbers (`docs/design/scoring.md`, `rounds.md`, `economy.md`, `content.md`): the round structure, the deal, the truck, the quota, the loopholes found and closed, and the final rubric result. It follows `06-haul-first-pass.md`.

Run: 2026-11-10, with `prototypes/haul/` and the `tables.json` committed alongside this note.

**Method.** Three kinds of test:

- **Variants at equal difficulty.** `haul.py experiments 300` changes one rule at a time. For each variant it first scales the score targets until bots with no point factory win about 2% of runs, then runs the full rubric with 300 runs per bot. The scaling uses 150 runs per step, so a variant's wall can sit a point or two off 2%.
- **Scripted builds.** A hand-made end-of-run build plays round 12 four hundred times with no target, to find a build's ceiling and look for strategies that bots don't discover.
- **The rubric** (`docs/design/session-rubric.md`), on the final tables with 1,000 runs per bot.

Bots weigh one more spin against delivering, by expected value, and play to the round's target.

## Variants at equal difficulty [Verified by simulation]

Columns:

- **Engine:** the best point-factory bot's win rate.
- **Reckless r1:** how often the bot that never delivers by choice loses round 1.
- **Cargo:** mean cargo per delivery.
- **Spoiled:** share of loads spoiled.
- **Quota:** share of deliveries that reach the quota.

| Variant | Checks | Engine | Reckless r1 | Cargo | Spoiled | Quota | Fails |
| --- | --- | --- | --- | --- | --- | --- | --- |
| **Baseline: 4 trips, tank of 10, quota ×2, truck of 6** | 10 | 42% | 66% | 3.9 | 34% | 75% | – |
| 3 trips | 8 | 39% | 72% | 3.7 | 31% | 77% | A1, B2 |
| 5 trips | 10 | 40% | 56% | 4.0 | 37% | 74% | – |
| Tank of 7 | 10 | 40% | 66% | 3.7 | 29% | 67% | – |
| Tank of 14 | 10 | 45% | 66% | 3.9 | 34% | 76% | – |
| One pool of 10 fuel for the round, no trips | 8 | 20% | 49% | 2.8 | 15% | 49% | A2, B2 |
| The player places wedges on wheels | 8 | 31% | 41% | 4.1 | 34% | 83% | A2, B2 |
| No quota bonus | 9 | 22% | 64% | 4.1 | 32% | 0% | B2 |
| Quotas 25% higher | 9 | 50% | 67% | 3.9 | 32% | 42% | B1 |
| Quotas 50% higher | 9 | 27% | 65% | 4.1 | 33% | 19% | B2 |
| Quota bonus ×3 | 9 | 35% | 65% | 3.8 | 35% | 79% | B2 |
| Truck holds 5 values | 9 | 42% | 24% | 3.9 | 32% | 70% | A2 |
| Truck holds 7 values | 10 | 40% | 100% | 3.9 | 32% | 74% | – |
| No full-truck bonus | 10 | 41% | 66% | 3.9 | 32% | 74% | – |
| No inspections | 9 | 37% | 66% | 3.9 | 34% | 81% | B2 |

A larger run of "quotas 25% higher" (600 runs per bot, calibrated with 300) gave an engine win rate of 35%, with 45% of deliveries reaching the quota. The 50% in the table came from a wall set too low: its no-engine bots won 4.3%.

## What the variants show [Inference]

- **Trips, not a pool of fuel.** With one pool, the best play is many small, safe deliveries: 2.8 cargo each, with 15% of loads spoiled. Point factories fall to 20%, and recklessness goes unpunished. With trips, loads are bigger and a third of them spoil. Trips are what make it a press-your-luck game.
- **A random deal, not placement.** Letting the player place wedges takes risk out (the reckless bot survives round 1 more often) and weakens factories.
- **The quota earns its place.** Without it, the best factory wins 22% instead of 42%. The quota is what separates a build that carries better loads from one that doesn't.
  - Raising quotas by 25% makes the quota a real stretch (45% of deliveries reach it, against 75%), at a cost of about 7 points of factory win rate.
  - A ×3 bonus is worse than ×2.
  - The quota level is the main dial for how tempting "one more spin" is, and it needs playtests: bots press by arithmetic, not temptation.
- **A truck of 6.** At 5 the truck fills so easily that never delivering is nearly safe (24%). At 7 the reckless bot always loses round 1, which is fine, but a full truck becomes very rare. 6 keeps the full truck as a reachable high point.
- **4 trips and a tank of 10.** Three trips make round 1 too swingy (A1 fails). The tank's size matters little between 7 and 14.
- **The full-truck bonus and inspections are neutral for balance.** They stay for what they add to play: a goal to chase, and rounds that ask different things of a build.

## Loopholes found with scripted builds [Verified by simulation]

Round 12, no target, 400 plays each. A loophole is a build that scores well and never spoils.

| Build | Rule set | Median score | Loads spoiled |
| --- | --- | --- | --- |
| A bag of 12 "sealed" wedges that can never spoil a load | With the Sealed upgrade | 7,696 | 0% |
| 6 unique wedges on 6 wheels, 2 Thermoses, load-size items | Empty slots are free spins | 13,878 | 0% |
| The same build | With the no-repeats rule | 0 | 91% |
| 6 unique wedges and 6 crates | With the no-repeats rule | 803 | 57% |
| A realistic Convoy build | With the no-repeats rule | 10,464 | 37% |

Three fixes came out of this:

- **The Sealed upgrade is gone.** Enough of them made a risk-free bag.
- **No repeats.** A wedge that lands twice in a trip spoils the load, whatever it carries. Before this, empty slots, tarps and crates were free spins, so a bag thinned to one cargo wedge per wheel could fill the truck with no risk. Now every re-spin of a wheel carries risk, however the bag is built.
- **Tarps and crates count once per trip,** which follows from no repeats.

The first haul's own loophole, a bag with one wedge per value, was closed earlier by keeping landed wedges on the wheel (`06-haul-first-pass.md`).

## Final rubric result [Verified by simulation]

`python3 prototypes/haul/haul.py sim 1000`: **10 of 10 checks pass.**

| Check | Result |
| --- | --- |
| A1 Round 1 safe | Worst bot loses round 1 0.4% of the time |
| A2 An insane risk can lose round 1 | Reckless loses it 64.6% |
| B1 No win without a point factory | Best no-engine bot wins 2.9% |
| B2 A point factory can win | Best engine bot wins 45.3% |
| B3 A point factory is risky | No bot wins 90% |
| C1 Many factories | color 36%, straight 42%, safety 41%, quota 45%, adaptive 43% |
| C2 No dominant spending style | spender 37%, saver 32%, reroller 41%, bargain 5% |
| D1 No cliff | Round 12 causes 21% of engine-bot losses |
| E1 Shopping is a choice | 61% of shops with a good fit have two or more |
| E2 No must-have | High Roller, the most-bought item, is in 46% of wins |

Engine bots carry 3.9 cargo per delivery, spoil 34% of their loads, and reach the quota on 75% of deliveries.

## Notes on the result [Inference]

- **Every kind of factory works,** and following the shop (adaptive) is as good as committing to one. That's the "adapt to circumstances" goal.
- **The margins on B1 and B2 are thin** (2.9% against 3%, 45% against 40%). The target curve is one scale factor away from either line, so it should be re-calibrated after any content change.
- **The bargain style is weak alone** (5%), as in every earlier layer. Bots never switch styles, so this doesn't show the strategy is bad, only that it can't be the whole plan.
- **Telemetry to act on:**
  - Odd Fellow and Even Steven are never bought by engine bots.
  - Ribbons are bought 4% of the time, and Piggy Bank 8%.
  - Premium (95% of wins) and tarp wedges (93%) are near-universal. They're bag upgrades, which E2 doesn't cover, but more ways to do the same jobs would spread the choice.
- **What simulation can't say:** whether the quota tempts a person, whether a spoiled load stings the right amount, and whether watching the odds is fun. Those need the playtest half of the rubric.

## Rigs and stakes [Verified by simulation]

`python3 prototypes/haul/haul.py ladder 400`, on the final tables. "Engine" is the best point-factory bot's win rate, and "no engine" the best bot without one.

| Rig, stake 1 | Engine | No engine | Worst round-1 loss | Reckless loses round 1 |
| --- | --- | --- | --- | --- |
| Standard | 43% | 2.2% | 0.8% | 65% |
| Tanker | 40% | 1.2% | 0.8% | 65% |
| Reefer | 36% | 2.0% | 1.0% | 51% |
| Flatbed | 37% | 1.8% | 0.8% | 100% |
| Hotshot | 35% | 3.5% | 0.5% | 66% |
| Monochrome | 48% | 3.8% | 0.8% | 64% |

| Standard rig | Engine | No engine |
| --- | --- | --- |
| Stake 1 | 43% | 2.2% |
| Stake 2: targets +15% | 37% | 1.8% |
| Stake 3: prices +$1 | 24% | 1.5% |
| Stake 4: inspections every second round | 22% | 2.0% |
| Stake 5: tank −1 | 19% | 1.5% |
| Stake 6: a spoiled load costs $2 | 18% | 1.0% |
| Stake 7: quotas +20% | 10% | 0.5% |
| Stake 8: one fewer item slot | 9% | 0.2% |

Two rigs were dropped after testing:

| Dropped rig | Engine | No engine | Reckless loses round 1 |
| --- | --- | --- | --- |
| Nine wedges, one of each value 1–9 | 59% | 23.8% | 39% |
| The same, with targets 60% higher | 32% | 7.8% | 39% |
| Sixteen wedges, values 1–8 twice, one fewer item slot | 68% | 37.2% | 40% |

**Findings [Inference].**

- **Rigs sit in one band.** Factories win 35–48% and no-engine play at most 3.8%, with round 1 safe throughout. Each rig changes how the game is played without breaking its shape.
- **The stake ladder falls at every step,** from 43% to 9%, and never reaches zero. Higher prices (stake 3) and higher quotas (stake 7) are the two biggest steps.
- **The number of different values in the starting bag is the strongest difficulty dial in the game.** More values means fewer repeats, so both dropped rigs made the game far easier for everyone, including reckless play. Raising targets didn't restore the gap between factory and no-factory play. Rigs should vary anything but this.

## Reworking rarely bought items [Verified by simulation]

Odd Fellow, Even Steven, the Ribbons and Piggy Bank were flagged as rarely bought. Two reworks were tried, each re-calibrated with 600 runs per bot:

| Change | Target scale | Best engine | Result |
| --- | --- | --- | --- |
| Double their payouts, and have engine bots buy them | 1.29 | 34% | The wall had to rise, because the items helped no-engine play most |
| Keep their payouts, and have engine bots buy them | 1.01 | 38% | Factories got weaker: the items take slots that better items need |
| Leave them, and have engine bots pass on them | 1.00 | 45% | The final tables |

**Finding [Inference].** These items are early-game help and suit play without a factory. A factory is better off without them. That's a fair job for a common item, so they stay, and the telemetry flag is explained, not fixed.

## Final tables, wider margins, seeds and the starter set [Verified by simulation]

**Margins.** At the targets above, the no-engine check passed at 2.9% against a 3% limit. Three target scales were run with 1,000 runs per bot:

| Target scale | Best engine | Best no-engine |
| --- | --- | --- |
| 1.00 | 45.7% | 2.6% |
| 1.05 | 43.8% | 2.4% |
| 1.10 | 41.4% | 1.7% |

The tables now use the 1.10 scale, rounded: 24, 42, 70, 100, 145, 215, 320, 495, 750, 1,150, 1,820, 2,900. A confirming run of 1,000 passes all ten checks:

- **No-engine play:** wins 1.8% at most.
- **The best engine:** wins 41.7%.
- **Each focus's best bot:** color 33%, straight 41%, safety 37%, quota 41%, adaptive 42%.

Both lines now have more than a point of headroom. The rig and stake tables above were measured at the 1.00 scale, so their win rates sit a few points higher than they would now.

**Seeds.** `haul.py selftest` checks that the same seed replays the same run line for line, and that 25 extra shop rerolls change neither the next deal nor the next spins.

**Starter set.** With 14 pieces of content locked (`docs/design/progression.md`), 1,000 runs per bot give:

- **The best engine:** 61%.
- **No-engine play:** 3.1%.
- **Round-1 losses:** 0.4% at worst.

A smaller pool makes a build's pieces easier to find, so the first runs are easier for a player who builds and no easier for one who doesn't.

**Browser prototype.** `prototypes/web/` is a playable version built from the same tables. An automated player ran it through several runs, rounds and shops with no script errors, and the page fits a 400-pixel-wide screen. No person has played it yet.

## Playing it by hand, and the freight bonus

**What was played.** The author played the opening of a run in the text prototype (seed 42): round 1, the first shop, and two trips of round 2, choosing every spin. This is one session by the designer, not a playtest by a new player.

**What it showed [Inference].**

- **The risk reads clearly.** With every wedge face up, repeats in brackets and a percentage on each wheel, each choice took a glance. A mistake (spinning a 33% wheel when a 0% wheel was open) was plainly the player's own.
- **Losing a load stings.** In round 2 a three-cargo load worth 168 was pressed on a 67% wheel and lost.
- **The first shop was a real choice:** a flat Lucky Coin, or two Paints toward a color build.
- **The early rounds had no reason to press on.** Round 1's load scored 112 against a target of 24, and round 2's 168 against 42. Score past the target is worth nothing, so once a load beats the target the only sensible move is to deliver. The game's central decision was switched off for the first few rounds. Bots could not show this: they deliver as soon as the round is won.

**The fix, tested [Verified by simulation].** A freight bonus pays $1 for each cargo past the third in a delivery, so a bigger load is always worth something. Bots take it only when a spin can't spoil the load. Each variant was calibrated with 500 runs per bot:

| Variant | Target scale | Best engine | No engine | Color focus | Cargo per delivery | Checks |
| --- | --- | --- | --- | --- | --- | --- |
| No freight bonus | 1.12 | 39.8% | 1.6% | 29% | 3.9 | 9 |
| Freight bonus of $1 | 1.16 | 43.0% | 1.6% | 40% | 4.1 | 10 |

The bonus is in the final tables, with targets raised to match: 24, 42, 71, 105, 155, 230, 345, 545, 835, 1,300, 2,100, 3,350. A confirming run of 1,000 passes all ten checks:

- **No-engine play:** wins 1.6% at most.
- **The best engine:** wins 42.6%.
- **Each focus's best bot:** color 38%, straight 41%, safety 40%, quota 43%, adaptive 41%.

The bonus also lifts the color build, the weakest focus, to the level of the others.

## The quota level: a decision

The quota list stays at 10, 12, 13, 15, 17, 19, 21, 23, 26, 29, 32, 36. Bots reach it on about 78% of deliveries. Raising it 25% makes it a harder stretch (45%) but costs point factories about 7 points of win rate, and the freight bonus now supplies the reason to go one further in every trip. The quota's job is to reward better loads, which it does at this level. It remains the first dial to turn if playtests find the quota too easy to matter.

## Rigs and stakes on the final tables [Verified by simulation]

`python3 prototypes/haul/haul.py ladder 500`, with the freight bonus and the final targets. These replace the earlier rig and stake figures above.

| Rig, stake 1 | Engine | No engine | Worst round-1 loss | Reckless loses round 1 |
| --- | --- | --- | --- | --- |
| Standard | 43% | 1.8% | 0.4% | 65% |
| Tanker | 38% | 1.0% | 0.4% | 65% |
| Reefer | 35% | 0.8% | 0.8% | 57% |
| Flatbed (truck of 7, tank −2) | 30% | 0.8% | 0.3% | 100% |
| Hotshot | 28% | 1.2% | 0.6% | 69% |
| Monochrome | 46% | 1.8% | 0.4% | 64% |

Flatbed's drawback changed. With three starting wheels, its worst round-1 loss was 1.6%, over the 1% limit. A smaller tank keeps round 1 safe.

| Standard rig | Engine | No engine | Worst round-1 loss |
| --- | --- | --- | --- |
| Stake 1 | 43% | 1.8% | 0.4% |
| Stake 2 | 37% | 0.6% | 0.4% |
| Stake 3 | 28% | 0.6% | 0.4% |
| Stake 4 | 26% | 0.8% | 0.4% |
| Stake 5 | 24% | 0.8% | 0.4% |
| Stake 6 | 20% | 0.6% | 0.4% |
| Stake 7 | 12% | 0.2% | 1.0% |
| Stake 8 | 9% | 0.2% | 1.0% |

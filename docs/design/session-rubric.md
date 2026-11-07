# Rubric for judging a game session

A session is one run, from the first round to a win or a loss. This rubric judges whether the run's difficulty has the shape we want:

- **Start:** the first round is almost impossible to lose, unless the player takes an insane risk.
- **End:** the final rounds are almost impossible to win, unless the player has built a point factory. A point factory is a high-reward, possibly risky strategy whose score compounds.

The rubric has two parts:

- **Simulated runs:** bots play thousands of runs in the prototype, and `prototypes/run/spintacular.py sim` checks the shape automatically.
- **Playtests:** people score a run by hand.

## Part 1: simulated runs

The prototype's bots stand in for kinds of players:

| Bot | Plays like |
| --- | --- |
| `passive` | Buys nothing. |
| `random` | Buys at random. |
| `flat/spender` | Buys whatever adds points now: flat bonuses and more wheels, with no synergy. |
| `reckless` | Takes an insane risk: sells three of the four starting wheels before round 1. |
| `<focus>/<style>` | Builds a point factory. The focus is `color`, `value`, `straight` or `adaptive` (it follows what the shop offers). The style is `spender`, `saver`, `reroller` or `bargain`, the shop strategies in `economy.md`. |

The first three are the **no-engine** bots; the `<focus>/<style>` bots are the **engine** bots.

| Check | What it asks | Pass when |
| --- | --- | --- |
| **A1** Safe start | Round 1 is almost impossible to lose. | Every bot except `reckless` loses round 1 in at most 1% of runs. |
| **A2** Risk is real | An insane risk can still lose round 1. | `reckless` loses round 1 in at least 50% of runs. |
| **B1** The wall | Winning without a point factory is almost impossible. | Every no-engine bot wins at most 3% of runs. |
| **B2** Factories win | A point factory can get over the wall. | The best engine bot wins at least 40% of runs. |
| **B3** Factories are risky | No strategy is a sure thing. | No bot wins 90% or more of runs. |
| **C1** Many factories | More than one kind of factory works. | At least 3 focuses have a win rate of at least half the best engine bot's. |
| **C2** No dominant spending style | No shop strategy beats the rest. | The best style's mean win rate is at most 1.5× the median style's. |
| **D1** No cliff | Difficulty rises smoothly. | No single round causes more than 35% of engine-bot losses. |
| **E1** Shopping is a choice | There's never only one thing you need. | Of the shops that have at least one offer that fits the bot's build well, at least 50% have two or more. |
| **E2** No must-have | No item or wheel is required to win. | No item or wheel is bought in more than 80% of winning runs. |

The report also prints telemetry for every piece of content. **Pick rate** is how often it's bought when offered. **Win presence** is the share of winning runs that bought it. These are prompts to look closer, not verdicts (`docs/research/01-design-foundations.md`).

### What bots can't tell us

- **Bots are worse players than people.** A pass or fail is about the bots as much as the numbers. Read B2 and C1 especially as lower bounds.
- **Bots never switch strategy.** The economy is meant to reward switching between shop strategies as results come in. A bot that only hunts bargains is expected to do poorly, so C2 checks that no style dominates, not that every style wins on its own.
- **Nothing here measures fun.** That's what Part 2 is for.

## Part 2: playtests

After a run, the player scores each line from 1 (no) to 5 (yes). The observer notes the round the run ended in, and the moment the player said their build "clicked", if it did.

| # | Question |
| --- | --- |
| 1 | Round 1 felt safe, and losing it would have taken a deliberately wild choice. |
| 2 | In the last rounds, I could tell I needed a point factory, and I knew which pieces it would take. |
| 3 | When my factory worked, the payout felt huge and earned, not lucky. |
| 4 | When I lost, I could see why, and what I'd try next time. |
| 5 | I could follow every payout step by step: dice, patterns, items. |
| 6 | Each shop gave me a real choice; I never felt there was only one thing to buy. |
| 7 | I changed how I spent money during the run because of how it was going. |
| 8 | The spins felt random but fair. I chose the odds; the wheel chose the result. |
| 9 | I want to start another run right now. |

A healthy run scores 4 or more on lines 1, 3, 5 and 9. A low score on 2 or 4 means the wall is unclear, and a low score on 6 or 7 means the shop is too narrow.

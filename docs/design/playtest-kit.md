# Playtest kit

How to run a playtest of Spintacular with someone who has never seen it, and what to write down. The scorecard is the playtest half of `session-rubric.md`. Aim for five new players before the C++ build starts: five is enough to see the same problem twice.

## Before the session

- **Open the browser prototype** (`prototypes/web/`, or its published link) on a laptop or phone, with sound on.
- **Choose the Standard rig, stake 1.** Note the seed, so a strange run can be replayed.
- **Clear the collection** only if the player has played before: a new browser starts with the starter set, which is what a first run should use.

## During the session

- **Say only this:** "Spin wheels to load the wagon, and deliver before the load spoils. Try to clear all 12 rounds."
- **Don't explain the rules.** The "How it works" panel is on the page. Whether the player finds it and understands it is part of the test.
- **Ask them to think aloud,** and stay quiet.
- **Let them play one full run,** to a win or a loss, and a second if they ask for one. Asking is itself a result.

## What to write down

| When | Note |
| --- | --- |
| First spin | How long before they spin? Do they read the odds? |
| First repeat | Do they understand why the load spoiled, without being told? |
| Each delivery | How many cargo were in the load? Did they stop at the quota, short of it or past it? |
| The moment they say "one more" | The load's size, the odds they took, and what happened |
| First shop | What they buy, and anything they say they don't understand |
| When the build "clicks" | The round, and what they'd bought |
| End of run | The round it ended in, and the first thing they say |

## After the session: the scorecard

The player scores each line from 1 (no) to 5 (yes).

| # | Question | Score |
| --- | --- | --- |
| 1 | Round 1 felt safe, and losing it would have taken a deliberately wild choice. | |
| 2 | In the last rounds, I could tell I needed a point factory, and I knew which pieces it would take. | |
| 3 | When my factory worked, the payout felt huge and earned, not lucky. | |
| 4 | When I lost, I could see why, and what I'd try next time. | |
| 5 | I could follow every payout step by step: cargo, sets, items, bonuses. | |
| 6 | Each shop gave me a real choice; I never felt there was only one thing to buy. | |
| 7 | I changed how I spent money during the run because of how it was going. | |
| 8 | The spins felt random but fair. I chose the odds; the wheel chose the result. | |
| 9 | I felt the pull to spin once more than was wise, and sometimes gave in. | |
| 10 | I want to start another run right now. | |

Then ask two open questions: "What was the best moment?" and "What was confusing or annoying?"

## Reading the results

- **Healthy:** 4 or more on lines 1, 3, 5, 9 and 10.
- **The wall is unclear:** low scores on 2 or 4.
- **The shop is too narrow:** low scores on 6 or 7.
- **The quota isn't tempting:** a low score on 9, or players who always stop exactly at the quota. Raise quotas first (`../research/07-haul-design-experiments.md`).
- **The early rounds are slack:** players who never press on in rounds 1 to 3. Raise the freight bonus, or the early targets.

Record each session in `docs/research/`, one file per round of playtests, with the seeds, the notes and the scores.

# wallbreaker

A machine that learns by hitting walls. Every distinct failure it runs into
becomes a new state, with its own stats on which escapes work. Over time the
state set is a map of everything that has ever stopped it, and how to get past
each thing.

Built as an FSMD, in C, with no AI libraries.

## How it works

**Controller (Moore FSM).** Output depends only on the current state:

```
SEED → JUDGE ──ok──────────────→ SOLVED
         │ known wall ─→ WALL:Wn ──→ JUDGE        (escape with best move)
         │                  └─timeout─→ EXPLORE   (kick it loose)
         └ unknown ─→ DEBOUNCE ─confirmed─→ SPAWN ─→ WALL:Wn
                          └─fluke─→ EXPLORE ─→ JUDGE
```

`WALL:Wn` states are created at runtime, one per wall. That's how the machine grows.

**Datapath.** Registers for the current candidate, the wall map, a debouncer
and a timeout counter.

- **Fingerprint.** An error message becomes a token set. Numbers collapse to a
  magnitude bucket, so `too small by 3198` and `too small by 2` are different
  walls, while similar walls merge (Jaccard ≥ 0.75).
- **Debouncer.** An unknown failure has to repeat 3 times before it gets its own
  state, so one-off flukes don't spawn junk walls.
- **Timeout.** After 25 escapes with no progress it kicks to EXPLORE. Progress
  resets the counter, and nothing gets thrown away.
- **Stats.** Each wall keeps a reward sum and try count per strategy. Reward is
  how much of the gap a move closed (0..1). Moves are picked with UCB1: lean on
  what worked, still poke at what hasn't been tried.
- **Memory.** The wall map is saved to `memory.txt`, so every run starts
  smarter than the last.

## Run it

```sh
make
./wallbreaker solve 137 -t      # build an expression = 137, print every transition
./wallbreaker bench             # 200 targets: no memory vs. with memory
make test
```

The v0 problem is a toy: build an arithmetic expression that equals a target
number. To add a problem, implement `problem_t` (seed, judge, moves); see
`src/expr.c`.

## v0 numbers (`./wallbreaker bench`)

```
fresh        solved 190/200  median steps 708
with memory  solved 178/200  median steps 409
```

Memory makes it about 40% faster on the median, but it solves a few fewer
targets because it over-trusts old habits. That's next on the list.

## Roadmap

- Fix the over-exploitation: memory shouldn't cost solves.
- The machine's own language: primitive moves it can glue into new words
  (macros of escapes that worked), so it grows its own vocabulary.
- Wikidata as the judge for real-world walls (who failed, who succeeded).
- Cross-domain wall merging: spotting that different problems hit the same
  kind of wall.

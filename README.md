# METRO — SAT-Based Metro Map Planner

Plan a subway network on a city grid: given a set of metro lines with fixed start and end stations,
a cap on how many turns each line may make, and a list of popular destinations that must be served,
METRO decides whether such a network exists — and if it does, draws it.

It does this by **reducing the routing problem to Boolean satisfiability**, handing the CNF to
[MiniSat](http://minisat.se/), and decoding the satisfying assignment back into concrete metro
routes. No search heuristics, no hand-written pathfinder: the constraints *are* the algorithm.

> Built for **COL226 / Assignment 3** at IIT Delhi. The encoder and decoder are ~1,600 lines of
> optimised C++11, with a Python test-case generator, format checker, and regression harness.

---

## Table of Contents

- [How It Works](#how-it-works)
- [Environment Setup](#environment-setup)
- [Build](#build)
- [Quick Start](#quick-start)
- [Running Step by Step](#running-step-by-step)
- [Input Format](#input-format)
- [Output Format](#output-format)
- [The SAT Encoding](#the-sat-encoding)
- [Generating Test Cases](#generating-test-cases)
- [Testing](#testing)
- [Project Structure](#project-structure)
- [Troubleshooting](#troubleshooting)
- [Contributing](#contributing)

---

## How It Works

```
  input.city ──▶ ./encoder ──▶ input.satinput ──▶ minisat ──▶ input.satoutput
                                                                    │
                                                                    ▼
                          input.metromap ◀── ./decoder ◀────────────┘
                                  │
                                  ▼
                          format_checker.py  (validates the plan)
```

Every grid cell gets one Boolean variable per metro line: `var[x][y][k]` is true iff cell `(x, y)`
is part of line `k`. The encoder emits clauses that make *any* satisfying assignment a legal metro
map; the decoder walks the true variables to recover each line's route in order.

![Example metro map solution](Example.png)

---

## Environment Setup

| Requirement | Purpose |
|---|---|
| g++ (C++11) | Compiles `encoder.cpp` and `decoder.cpp` |
| Python 3.7+ | Test-case generator, format checker, test runner |
| **MiniSat** | The SAT solver — must be on your `PATH` as `minisat` |

<details>
<summary><b>Linux (Debian / Ubuntu)</b></summary>

```bash
sudo apt update
sudo apt install -y build-essential python3 minisat
```
</details>

<details>
<summary><b>macOS</b></summary>

```bash
xcode-select --install
brew install minisat python3
```

`compile.sh` uses `-march=native`. On Apple Silicon, if the compiler rejects it, build manually:

```bash
g++ -std=c++11 -O3 encoder.cpp -o encoder
g++ -std=c++11 -O3 decoder.cpp -o decoder
```
</details>

<details>
<summary><b>Windows</b></summary>

Use **WSL2** and follow the Linux instructions. MiniSat is not readily available for native Windows.
</details>

Verify everything is in place:

```bash
g++ --version && python3 --version && minisat --help | head -1
```

No Python packages are required — the scripts use only the standard library.

---

## Build

```bash
git clone https://github.com/<your-username>/METRO.git
cd METRO

./compile.sh              # release build: -O3 -march=native, binaries stripped
```

| Command | Effect |
|---|---|
| `./compile.sh` | Release build (`-O3 -march=native -funroll-loops`) |
| `DEBUG=1 ./compile.sh` | Debug build with ASan + UBSan and symbols |
| `CLEAN=1 ./compile.sh` | Remove old binaries first |
| `make clean` | Delete binaries and intermediate `.satinput` / `.satoutput` / `.metromap` files |

---

## Quick Start

Solve the bundled `input.city`:

```bash
make run
```

That runs the whole pipeline — compile → encode → solve → decode → validate.

Generate a fresh random instance and solve it:

```bash
make runn        # 40×40 grid, 30 lines, max 3 turns, guaranteed solvable
```

---

## Running Step by Step

Useful when you want to inspect the CNF or reuse a solver run. All three stages share a basename.

```bash
./run1.sh input                          # input.city    -> input.satinput
minisat input.satinput input.satoutput   # solve
./run2.sh input                          # input.satoutput -> input.metromap
python3 format_checker.py input          # validate the result against the input
```

`./encoder <basename>` and `./decoder <basename>` can also be called directly; with no argument
they default to `input`.

> MiniSat exits with a non-zero status on UNSAT — that is expected, not a failure. The `Makefile`
> prefixes the call with `-` for exactly this reason.

---

## Input Format

A `.city` file:

```
<scenario>
<N> <M> <K> <J> [<P>]
<x1> <y1> <x2> <y2>        ← line 1: start and end
<x1> <y1> <x2> <y2>        ← line 2
...                        ← K lines total
<px> <py> <px> <py> ...    ← P popular cells (scenario 2 only)
```

| Field | Meaning |
|---|---|
| `scenario` | `1` = routing only · `2` = routing **plus** popular-cell coverage |
| `N`, `M` | Grid dimensions — the first value is the width (columns), the second the height (rows) |
| `K` | Number of metro lines |
| `J` | Maximum turns allowed per line |
| `P` | Number of popular cells (scenario 2 only) |

Example — a 40×40 city, 15 lines, at most 3 turns each, 3 popular destinations:

```
2
40 40 15 3 3
15 23 12 31
29 2 1 1
10 21 37 14
...
9 23 20 24 2 4
```

---

## Output Format

A `.metromap` file. First line is `1` (satisfiable) or `0` (no such network exists). On success,
each following line lists the cells of one metro line **in path order**:

```
1
15 23 15 24 15 25 ... 12 31
29 2 28 2 27 2 ... 1 1
...
```

---

## The SAT Encoding

Variables: `N × M × K` cell-colour variables, plus auxiliary variables introduced by the
at-most-one encoding and path selectors.

| Constraint | Guarantees |
|---|---|
| **G0 — exclusivity** | Each cell belongs to at most one line. Encoded with a *sequential* at-most-one (linear clauses, `O(K)` auxiliaries) rather than pairwise `O(K²)` |
| **G1 — path selection** | For each line, at least one of the enumerated ≤ J-turn paths from start to end is fully coloured. `PATH_GENERATOR` enumerates candidate routes; each gets a selector variable |
| **G2 — connectivity** | Any coloured cell that is not an endpoint must have a same-coloured neighbour — no orphan cells |
| **G3 — continuity** | If `(x, y)` and `(x, y+2)` share a colour, so must `(x, y+1)` (and the vertical analogue) — no gaps |
| **Coverage** *(scenario 2)* | Every popular cell is coloured by at least one line |

### Performance notes

The encoder produces millions of clauses on large grids, so it is written for throughput:

- Clauses are accumulated in a **10,000-clause buffer** and flushed through a `stringstream`,
  avoiding per-clause `ofstream` overhead
- Duplicate literals in long clauses are removed via a `set` before writing
- The CNF header is patched in at close time, once the true variable and clause counts are known
- `var_map[x][y][k]` is a preallocated 3-D table — no hashing on the hot path

---

## Generating Test Cases

```bash
python3 testcase_gen.py --N 20 --M 20 --K 5 --J 4 --mode constructive --count 1 --prefix mytest
```

| Flag | Meaning |
|---|---|
| `--N`, `--M` | Grid width (columns) and height (rows) — both required |
| `--K` | Number of metro lines |
| `--J` | Maximum turns per line |
| `--P` | Number of popular cells (scenario 2) |
| `--mode` | `constructive` (guaranteed SAT) · `random` (default, unknown) · `unsat` (guaranteed UNSAT) |
| `--count` | How many instances to emit |
| `--seed` | Random seed, for reproducible instances |
| `--outdir` | Output directory |
| `--prefix` | Output filename prefix (`<prefix>_000.city`, …) |

---

## Testing

A curated suite lives in `testcases/solvable/` and `testcases/unsolvable/`, covering corridors,
chokepoints, gauntlets, and scenario-2 coverage cases.

```bash
python3 TESTER.py testcases/solvable          # run every case in a directory
python3 TESTER.py testcases/solvable corridor_j3_000   # run a single case
```

`TESTER.py` copies each case into the working directory, drives
`run1.sh → minisat → run2.sh → format_checker.py`, cleans up intermediates, and prints a pass/fail
summary with timings.

Validate a single result by hand:

```bash
python3 format_checker.py input
```

The checker re-reads the `.city` file and independently verifies that every route is contiguous,
connects the right endpoints, respects the turn budget, never overlaps another line, and (in
scenario 2) covers all popular cells.

---

## Project Structure

```
.
├── encoder.cpp        # .city  -> DIMACS CNF   (823 lines: var mapping, G0-G3, coverage)
├── decoder.cpp        # SAT model -> .metromap (788 lines: model parse, path reconstruction)
├── compile.sh         # Release / DEBUG build script
├── Makefile           # `run` and `runn` end-to-end pipelines
├── run1.sh, run2.sh   # Thin wrappers: encode / decode by basename
├── testcase_gen.py    # Instance generator (constructive / random / unsat)
├── format_checker.py  # Independent solution validator
├── TESTER.py          # Regression harness over a directory of cases
├── testcases/
│   ├── solvable/
│   └── unsolvable/
├── input.city         # Default instance
└── Example.png        # Rendered solution
```

**Reading order:** `encoder.cpp::main` → `read_PROBLEM` → `G0` → `PATH_GENERATOR` →
`AT_LEAST_ONE_PATH` → `G2` / `G3` → `POPULAR_CELL_COVERAGE`, then `decoder.cpp`.

---

## Troubleshooting

| Symptom | Fix |
|---|---|
| `minisat: command not found` | `brew install minisat` (macOS) · `sudo apt install minisat` (Linux) |
| `make run` reports a non-zero exit from minisat | Normal — MiniSat signals UNSAT via exit code |
| `error: unknown argument '-march=native'` | Compile manually: `g++ -std=c++11 -O3 encoder.cpp -o encoder` |
| Validation fails | Run `python3 format_checker.py <basename>` for the specific violated constraint |
| UNSAT on an instance you expect to be solvable | Increase `J`, or check that endpoints are inside the grid and mutually reachable |
| Encoder is slow / memory-hungry | Expected on large `N·M·K`; try a smaller grid or fewer lines, or build with `DEBUG=0` |

---

## Contributing

Contributions are welcome. Good first issues:

- Support solvers other than MiniSat (CaDiCaL, Kissat) behind a `--solver` flag
- Replace path enumeration with an incremental turn-counting encoding to scale past large `J`
- Emit SVG/PNG renderings directly from the decoder instead of relying on external tooling
- Add CI that runs `TESTER.py` over both test directories
- Report clause and variable counts as a machine-readable summary

```bash
git checkout -b feat/your-change
./compile.sh && make run && python3 TESTER.py testcases/solvable
git commit -m "feat: describe your change"
```

Keep the encoder C++11 and dependency-free, and mention in your PR if a change alters the clause
count for existing instances.

---

## License

Released for educational use. See `A3.pdf` and `A3-TC Generator_ README.md.pdf` for the original
problem statement.

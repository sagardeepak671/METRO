# METRO - Metro Map Planning Solver

Automatically plan subway/metro systems that connect popular destinations while staying within junction constraints. This solver takes a city grid with starting/ending points and popular locations, then uses SAT solving to find optimal metro line routes.

## What Does It Do?

Imagine you're designing a subway system for a new city. You have:
- A grid-based city layout
- Several metro lines you want to build (each with a start and end point)  
- Popular destinations that need to be served
- A limit on how many turns each line can make

METRO takes these constraints and figures out if it's possible to build such a system, and if so, where exactly the lines should go.

Under the hood, it converts this planning problem into a SAT (Boolean satisfiability) problem, then uses a SAT solver to find the answer.

## Quick Start

### Prerequisites

You'll need:
- A C++ compiler (g++ or similar)
- Python 3
- MiniSat SAT solver (the `minisat` command should be available)

On macOS with Homebrew:
```bash
brew install minisat
```

### Building

```bash
./compile.sh
```

This compiles the encoder and decoder programs.

### Running a Test

```bash
make run
```

This will:
1. Compile the code
2. Run the encoder to convert the city problem to SAT format
3. Solve it using MiniSat
4. Run the decoder to convert the solution back to metro map format
5. Validate the solution

Want to generate and test a random case?
```bash
make runn
```

## How to Use

### Input Format

Create a `.city` file describing your metro planning problem:

```
<scenario>
<N> <M> <K> <J> <P>
<metro_line_1_x1> <metro_line_1_y1> <metro_line_1_x2> <metro_line_1_y2>
<metro_line_2_x1> <metro_line_2_y1> <metro_line_2_x2> <metro_line_2_y2>
...
<popular_cell_1_x> <popular_cell_1_y>
<popular_cell_2_x> <popular_cell_2_y>
...
```

**Parameters:**
- `scenario`: Test scenario number (affects what constraints apply)
- `N`, `M`: Grid dimensions (N × M city)
- `K`: Number of metro lines
- `J`: Maximum number of turns allowed per metro line
- `P`: Number of popular destination cells
- **Metro lines**: Start point (x₁, y₁) and end point (x₂, y₂)
- **Popular cells**: Coordinates (x, y) that must be visited by at least one metro line

**Example:**
```
2
40 40 15 3 3
15 23 12 31
29 2 1 1
10 21 37 14
20 31 39 13
9 23 20 24
2 4
```

This describes a 40×40 city with 3 metro lines (each allowing up to 3 turns) and 2 popular destinations to serve.

### Output Format

The solver produces a `.metromap` file showing the routes:

```
1
<line_1_x1> <line_1_y1> <line_1_x2> <line_1_y2> <line_1_x3> <line_1_y3> ...
<line_2_x1> <line_2_y1> <line_2_x2> <line_2_y2> ...
...
```

Or simply `0` if no solution exists (unsatisfiable).

## Generating Test Cases

Want to create your own problem instances? Use the test case generator:

```bash
python3 testcase_gen.py --N 20 --M 20 --K 5 --J 4 --mode constructive --count 1 --prefix mytest
```

**Options:**
- `--N`, `--M`: Grid size
- `--K`: Number of metro lines
- `--J`: Max turns per line
- `--mode`: `constructive` (guaranteed solvable), `random` (may be unsolvable), or `unsat` (deliberately unsolvable)
- `--count`: How many test cases to generate
- `--prefix`: Output filename prefix

## Example Visualization

See how your metro system gets planned:

![Metro Map Example](example.png "Example metro map solution")

## Project Structure

```
.
├── encoder.cpp           # Converts city problems to SAT format
├── decoder.cpp           # Converts SAT solutions back to metro maps
├── testcase_gen.py       # Generates random problem instances
├── format_checker.py     # Validates solutions
├── Makefile             # Build and test targets
├── compile.sh           # Compilation script
└── testcases/           # Pre-built test suite
    ├── solvable/        # Problems with solutions
    └── unsolvable/      # Problems with no solution
```

## Testing

Run the built-in test suite:

```bash
python3 TESTER.py
```

This validates the solver against various test cases and reports results.

## Constraints Overview

The solver enforces:
1. **No overlap**: Each grid cell contains at most one metro line
2. **Valid paths**: Each metro line actually connects its start and end points
3. **Turn limits**: Each line respects the maximum turn constraint (J)
4. **Coverage**: (Scenario 2) Every popular cell is visited by at least one metro line

## Troubleshooting

**"minisat: command not found"**
- Install MiniSat: `brew install minisat` (macOS) or `apt-get install minisat` (Linux)

**Solution validation fails**
- Check your `.city` file format using `python3 format_checker.py <basename>`
- Ensure coordinates are within grid bounds

**No solution found for a problem you think is solvable**
- Try increasing J (max turns allowed)
- Check that start/end points are actually reachable

## License

Part of Assignment 3 coursework.

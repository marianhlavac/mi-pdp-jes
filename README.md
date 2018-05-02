# mi-pdp-jes

> Coursework for MI-PDP course at FIT CTU

mi-pdp-jes is the shortest path on chess board solver, written as BB-DFS algorithm to be parallelized later.

The algorithm is thereafter parallelized using OpenMP library, for parallelization on a single computing node, and using MPI library for parallelization across many computing nodes. The program is then run on a cluster.

## Assignment

The original assignment is written in czech.

[Assignment](./assignment.md)

## Requirements

- OpenMP library
- MPI library

## Compiling

Compile using `make`. Default target compiles with clang, target `dev` compiles without optimizations, `atstar` target is modified for compiling at star cluster (using gcc)

Example: 
```
 $ make atstar
```

## Usage

```
 $ ./bin/pdpjes [--noomp] [--serial] datafiles...
```

Example:
```
 $ ./bin/pdpjes --serial data/fixtures/kun01.txt data/fixtures/kun03.txt
```
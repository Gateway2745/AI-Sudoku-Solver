## AI Sudoku Solver

The 9x9 sudoku puzzle has been modelled as a constraint satisfaction problem and solved using several heuristics. 

* Solution highly modular using functional paradigm.
* AC-3 algorithm for initial domain pruning.
* Minimum Remaining Value (MRV) heuristic for variable ordering.
* Least Constraining Value (LCV) Heuristic for value ordering.
* Forward Checking (FC) to prune domains after variable assignment.
* < 1s running time for hardest puzzles.

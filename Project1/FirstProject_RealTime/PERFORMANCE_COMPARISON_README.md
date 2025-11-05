# Matrix Operations with Performance Comparison

## Overview
This project implements matrix operations (addition, subtraction, multiplication) using three different approaches to compare their performance:

1. **Single-threaded** - Sequential execution
2. **OpenMP** - Shared-memory parallelization
3. **Multiprocessing** - Fork-based process parallelization with pipes

## Project Structure

```
FirstProject_RealTime/
├── menu_demo_v2.c                  # Main menu and user interface
├── matrix_types.h                  # Shared data structures
├── matrix_utils.c                  # Matrix lifecycle management
├── matrix_file_ops.h/c             # File I/O operations
├── matrix_arithmetic.h/c           # Basic arithmetic operations
├── matrix_arithmetic_parallel.h/c  # Parallel implementations with timing
├── Makefile_demo                   # Build system with OpenMP support
├── run_performance_test.sh         # Performance testing script
└── matrices/                       # Sample matrix files
    ├── Matrix_A.txt (3x3)
    ├── Matrix_B.txt (3x3)
    ├── Identity_3x3.txt (3x3)
    ├── Large_A.txt (50x50)         # Created by test script
    └── Large_B.txt (50x50)         # Created by test script
```

## Implementation Details

### Options 10-12: Arithmetic Operations with Performance Comparison

When you select options 10 (Add), 11 (Subtract), or 12 (Multiply), the program:

1. **Runs all three methods** sequentially on the same input matrices
2. **Measures execution time** for each method using high-resolution timers
3. **Compares results** and displays:
   - Individual execution times
   - Speedup factors (relative to single-threaded baseline)
   - Fastest method identification
4. **Returns the result** from the fastest method

### Single-Threaded Implementation
```c
// Sequential nested loops - baseline performance
for (int i = 0; i < rows; i++) {
    for (int j = 0; j < cols; j++) {
        result[i][j] = matrix1[i][j] + matrix2[i][j];
    }
}
```

### OpenMP Implementation
```c
// Parallel loops with compiler directives
#pragma omp parallel for collapse(2) schedule(static)
for (int i = 0; i < rows; i++) {
    for (int j = 0; j < cols; j++) {
        result[i][j] = matrix1[i][j] + matrix2[i][j];
    }
}
```
- Uses `collapse(2)` to parallelize both loops
- `schedule(static)` for load balancing
- Automatically manages thread pool

### Multiprocessing Implementation
```c
// Fork one child process per element
for each element (i, j):
    pipe(fd)
    if (fork() == 0):
        // Child process
        result = compute(matrix1[i][j], matrix2[i][j])
        write(fd[1], &result, sizeof(double))
        exit(0)
    // Parent continues to next element

// Collect results
for each element:
    read(fd[0], &result, sizeof(double))
    waitpid(child_pid)
```
- **Addition/Subtraction**: Creates `rows × cols` child processes
- **Multiplication**: Creates `rows1 × cols2` child processes
- Each child computes one result element
- Uses pipes for IPC (Inter-Process Communication)
- Parent waits for all children before returning

## Performance Characteristics

### Expected Results

| Method           | Small Matrices (3×3) | Large Matrices (50×50) | Overhead |
|------------------|---------------------|------------------------|----------|
| Single-threaded  | Baseline (fastest)  | Baseline               | None     |
| OpenMP           | Slower (overhead)   | **Fastest** (2-4x)     | Thread creation/sync |
| Multiprocessing  | **Slowest** (100x+) | Slowest (10-50x)       | Fork + pipes |

### Why These Results?

**Small matrices (3×3 = 9 elements):**
- Computation time: ~microseconds
- OpenMP overhead: Thread pool management, synchronization
- Multiprocess overhead: 9 fork() calls + pipe creation + context switches
- **Conclusion**: Overhead >> computation time, single-threaded wins

**Large matrices (50×50 = 2,500 elements):**
- Computation time: ~milliseconds to seconds
- OpenMP: Efficiently distributes work across CPU cores
- Multiprocess: 2,500 fork() calls = massive overhead, but computation benefits from parallelism
- **Conclusion**: OpenMP wins due to low overhead + good parallelism

**Multiplication (50×50):**
- Even more computational work (50³ = 125,000 operations)
- OpenMP speedup more pronounced
- Multiprocess still suffers from fork overhead

## Compilation and Running

### Build the Project
```bash
cd FirstProject_RealTime
make -f Makefile_demo clean
make -f Makefile_demo
```

### Run with Performance Test
```bash
./run_performance_test.sh
```
This creates large test matrices (50×50) for meaningful performance comparisons.

### Manual Run
```bash
./menu_demo_v2
```

## Usage Example

```
1. Select option 6: "Read a set of matrices from a folder"
   Enter: /full/path/to/matrices
   
2. Select option 10: "Add 2 matrices"
   Enter first matrix name: Large_A
   Enter second matrix name: Large_B
   Enter result matrix name: SumAB
   
   [The program will automatically:]
   - Run single-threaded addition
   - Run OpenMP addition
   - Run multiprocess addition
   - Display timing comparison
   - Save fastest result as 'SumAB'

3. Select option 2: "Display a matrix"
   Enter matrix name: SumAB
   [View the result]
```

## Sample Output

```
========================================
Performance Comparison: Addition
Matrix 1: Large_A (50x50), Matrix 2: Large_B (50x50)
========================================

[1/3] Running Single-threaded method...
   ✓ Completed in 0.000842 seconds

[2/3] Running OpenMP method...
   ✓ Completed in 0.000231 seconds
   Speedup: 3.65x

[3/3] Running Multiprocessing method...
   ✓ Completed in 0.287653 seconds
   Speedup: 0.00x

========================================
PERFORMANCE SUMMARY
========================================
Single-threaded:   0.000842 s (baseline)
OpenMP:            0.000231 s (3.65x faster)
Multiprocessing:   0.287653 s (0.00x slower)
========================================

★ Fastest method: OpenMP (0.000231 s)

✓ Result matrix 'SumAB' added to collection.
```

## Key Features

### Real-Time Requirements Met
✅ **Process-based parallelism** (fork + pipes)
✅ **IPC via pipes** for data sharing
✅ **Performance timing** and comparison
✅ **OpenMP integration** for shared-memory parallelism
✅ **Interactive menu** returns after each operation
✅ **Multiple matrices** loaded from configuration folder

### Design Decisions

1. **Why pipes over FIFOs/shared memory?**
   - Simpler for point-to-point parent-child communication
   - Automatic cleanup on process exit
   - No filesystem dependencies

2. **Why one process per element?**
   - Directly implements project requirement
   - Demonstrates maximum parallelism (even if inefficient)
   - Shows clear performance trade-offs

3. **Why keep all three methods?**
   - Educational: demonstrates parallelism trade-offs
   - Comparative: shows when each method is appropriate
   - Research: quantifies overhead vs. benefit

## Limitations and Future Improvements

### Current Limitations
- Multiprocessing: fork() overhead dominates for small/medium matrices
- No process pool implementation (creating fresh processes each time)
- Large matrices (e.g., 100×100) may hit system limits on max processes
- Memory duplication in fork() (not shared memory)

### Recommended Improvements (for production)
1. **Process Pool**: Pre-fork worker processes, reuse for multiple operations
2. **Work Batching**: Each child processes multiple elements (not just one)
3. **Shared Memory**: Use `shmget/shmat` instead of pipes for large data
4. **Adaptive Strategy**: Choose method based on matrix size
   - Small (<10×10): Single-threaded
   - Medium (10-100): OpenMP
   - Large (>100): Hybrid (process pool + OpenMP per process)

## Testing

### Quick Test (3×3 matrices)
```bash
./menu_demo_v2
# Option 6 → load from matrices/
# Option 10 → add Matrix_A + Matrix_B
# Observe: Single-threaded likely fastest
```

### Performance Test (50×50 matrices)
```bash
./run_performance_test.sh
# Option 6 → load from matrices/
# Option 10 → add Large_A + Large_B
# Observe: OpenMP significantly faster
# Option 12 → multiply Large_A × Large_B
# Observe: OpenMP speedup even more pronounced
```

## Compilation Flags

```makefile
CFLAGS = -Wall -Wextra -g -O2 -fopenmp
LDFLAGS = -fopenmp
```

- `-fopenmp`: Enables OpenMP directives and links runtime library
- `-O2`: Optimization level (balances speed and debugging)
- `-g`: Debug symbols (for gdb if needed)

## Environment Variables

Set `OMP_NUM_THREADS` to control OpenMP parallelism:
```bash
export OMP_NUM_THREADS=4
./menu_demo_v2
```

## Troubleshooting

**Q: "fork: Resource temporarily unavailable"**
A: Too many processes. Use smaller matrices or increase system limits:
```bash
ulimit -u 4096
```

**Q: OpenMP not faster?**
A: Check CPU cores: `nproc`
Set threads: `export OMP_NUM_THREADS=$(nproc)`

**Q: Multiprocessing incredibly slow?**
A: Expected for small matrices! Use Large_A/Large_B (50×50+)

## Authors
Real-Time Systems Project - Matrix Operations with Multi-Processing

## License
Educational use only

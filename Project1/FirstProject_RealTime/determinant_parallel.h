#ifndef DETERMINANT_PARALLEL_H
#define DETERMINANT_PARALLEL_H

#include "matrix_types.h"
#include "matrix_arithmetic_parallel.h" /* For PerformanceMetrics */

/* Single-threaded determinant using Gaussian elimination with partial pivoting */
int determinant_single(const Matrix* m, double* out_det, double* exec_time);

/* OpenMP-parallel determinant (row updates per step in parallel) */
int determinant_openmp(const Matrix* m, double* out_det, double* exec_time);

/* Multiprocess determinant (per-row update children per elimination step) */
int determinant_multiprocess(const Matrix* m, double* out_det, double* exec_time);

/* Runs all three determinant methods and prints a performance comparison.
 * Returns 1 on success and writes the chosen determinant to *out_det.
 */
int run_determinant_comparison(const Matrix* m, PerformanceMetrics* metrics, double* out_det);

#endif /* DETERMINANT_PARALLEL_H */

#ifndef MATRIX_ARITHMETIC_PARALLEL_H
#define MATRIX_ARITHMETIC_PARALLEL_H

#include "matrix_types.h"

/**
 * Performance comparison structure
 */
typedef struct {
    double single_thread_time;
    double openmp_time;
    double multiprocess_time;
} PerformanceMetrics;

/**
 * Add two matrices using single-threaded approach
 */
Matrix* add_matrices_single(const Matrix* m1, const Matrix* m2, const char* result_name, double* exec_time);

/**
 * Add two matrices using OpenMP parallelization
 */
Matrix* add_matrices_openmp(const Matrix* m1, const Matrix* m2, const char* result_name, double* exec_time);

/**
 * Add two matrices using multiprocessing (one child per element)
 */
Matrix* add_matrices_multiprocess(const Matrix* m1, const Matrix* m2, const char* result_name, double* exec_time);

/**
 * Subtract two matrices using single-threaded approach
 */
Matrix* subtract_matrices_single(const Matrix* m1, const Matrix* m2, const char* result_name, double* exec_time);

/**
 * Subtract two matrices using OpenMP parallelization
 */
Matrix* subtract_matrices_openmp(const Matrix* m1, const Matrix* m2, const char* result_name, double* exec_time);

/**
 * Subtract two matrices using multiprocessing (one child per element)
 */
Matrix* subtract_matrices_multiprocess(const Matrix* m1, const Matrix* m2, const char* result_name, double* exec_time);

/**
 * Multiply two matrices using single-threaded approach
 */
Matrix* multiply_matrices_single(const Matrix* m1, const Matrix* m2, const char* result_name, double* exec_time);

/**
 * Multiply two matrices using OpenMP parallelization
 */
Matrix* multiply_matrices_openmp(const Matrix* m1, const Matrix* m2, const char* result_name, double* exec_time);

/**
 * Multiply two matrices using multiprocessing (one child per result element)
 */
Matrix* multiply_matrices_multiprocess(const Matrix* m1, const Matrix* m2, const char* result_name, double* exec_time);

/**
 * Run all three methods and compare performance
 */
Matrix* run_operation_comparison(const Matrix* m1, const Matrix* m2, const char* result_name, 
                                  const char* operation, PerformanceMetrics* metrics);

#endif // MATRIX_ARITHMETIC_PARALLEL_H

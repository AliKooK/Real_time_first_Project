#ifndef MATRIX_ARITHMETIC_H
#define MATRIX_ARITHMETIC_H

#include "matrix_types.h"

/**
 * Add two matrices (element-wise addition)
 * Returns a new matrix that is the sum of m1 and m2
 * Returns NULL if matrices are incompatible or on error
 */
Matrix* add_matrices(const Matrix* m1, const Matrix* m2, const char* result_name);

/**
 * Subtract two matrices (element-wise subtraction: m1 - m2)
 * Returns a new matrix that is the difference of m1 and m2
 * Returns NULL if matrices are incompatible or on error
 */
Matrix* subtract_matrices(const Matrix* m1, const Matrix* m2, const char* result_name);

/**
 * Multiply two matrices (matrix multiplication: m1 × m2)
 * Returns a new matrix that is the product of m1 and m2
 * Returns NULL if matrices are incompatible or on error
 */
Matrix* multiply_matrices(const Matrix* m1, const Matrix* m2, const char* result_name);

#endif // MATRIX_ARITHMETIC_H

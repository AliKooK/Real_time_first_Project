#include "matrix_arithmetic.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/**
 * Add two matrices (element-wise addition)
 */
Matrix* add_matrices(const Matrix* m1, const Matrix* m2, const char* result_name) {
    if (!m1 || !m2 || !result_name) {
        fprintf(stderr, "Error: NULL parameter in add_matrices\n");
        return NULL;
    }
    
    // Check dimensions compatibility
    if (m1->rows != m2->rows || m1->cols != m2->cols) {
        fprintf(stderr, "Error: Matrix dimensions incompatible for addition\n");
        fprintf(stderr, "Matrix '%s' is %dx%d, Matrix '%s' is %dx%d\n",
                m1->name, m1->rows, m1->cols, m2->name, m2->rows, m2->cols);
        return NULL;
    }
    
    // Create result matrix
    Matrix* result = create_matrix(result_name, m1->rows, m1->cols);
    if (!result) {
        fprintf(stderr, "Error: Failed to create result matrix\n");
        return NULL;
    }
    
    // Perform addition
    for (int i = 0; i < m1->rows; i++) {
        for (int j = 0; j < m1->cols; j++) {
            result->data[i][j] = m1->data[i][j] + m2->data[i][j];
        }
    }
    
    printf("Matrix addition successful: '%s' + '%s' = '%s' (%dx%d)\n",
           m1->name, m2->name, result->name, result->rows, result->cols);
    
    return result;
}

/**
 * Subtract two matrices (element-wise subtraction: m1 - m2)
 */
Matrix* subtract_matrices(const Matrix* m1, const Matrix* m2, const char* result_name) {
    if (!m1 || !m2 || !result_name) {
        fprintf(stderr, "Error: NULL parameter in subtract_matrices\n");
        return NULL;
    }
    
    // Check dimensions compatibility
    if (m1->rows != m2->rows || m1->cols != m2->cols) {
        fprintf(stderr, "Error: Matrix dimensions incompatible for subtraction\n");
        fprintf(stderr, "Matrix '%s' is %dx%d, Matrix '%s' is %dx%d\n",
                m1->name, m1->rows, m1->cols, m2->name, m2->rows, m2->cols);
        return NULL;
    }
    
    // Create result matrix
    Matrix* result = create_matrix(result_name, m1->rows, m1->cols);
    if (!result) {
        fprintf(stderr, "Error: Failed to create result matrix\n");
        return NULL;
    }
    
    // Perform subtraction
    for (int i = 0; i < m1->rows; i++) {
        for (int j = 0; j < m1->cols; j++) {
            result->data[i][j] = m1->data[i][j] - m2->data[i][j];
        }
    }
    
    printf("Matrix subtraction successful: '%s' - '%s' = '%s' (%dx%d)\n",
           m1->name, m2->name, result->name, result->rows, result->cols);
    
    return result;
}

/**
 * Multiply two matrices (matrix multiplication: m1 × m2)
 */
Matrix* multiply_matrices(const Matrix* m1, const Matrix* m2, const char* result_name) {
    if (!m1 || !m2 || !result_name) {
        fprintf(stderr, "Error: NULL parameter in multiply_matrices\n");
        return NULL;
    }
    
    // Check dimensions compatibility (m1 cols must equal m2 rows)
    if (m1->cols != m2->rows) {
        fprintf(stderr, "Error: Matrix dimensions incompatible for multiplication\n");
        fprintf(stderr, "Matrix '%s' has %d columns, Matrix '%s' has %d rows\n",
                m1->name, m1->cols, m2->name, m2->rows);
        fprintf(stderr, "For multiplication, columns of first matrix must equal rows of second matrix\n");
        return NULL;
    }
    
    // Create result matrix (rows from m1, cols from m2)
    Matrix* result = create_matrix(result_name, m1->rows, m2->cols);
    if (!result) {
        fprintf(stderr, "Error: Failed to create result matrix\n");
        return NULL;
    }
    
    // Perform matrix multiplication
    for (int i = 0; i < m1->rows; i++) {
        for (int j = 0; j < m2->cols; j++) {
            double sum = 0.0;
            for (int k = 0; k < m1->cols; k++) {
                sum += m1->data[i][k] * m2->data[k][j];
            }
            result->data[i][j] = sum;
        }
    }
    
    printf("Matrix multiplication successful: '%s' × '%s' = '%s' (%dx%d)\n",
           m1->name, m2->name, result->name, result->rows, result->cols);
    
    return result;
}

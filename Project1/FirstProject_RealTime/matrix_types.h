#ifndef MATRIX_TYPES_H
#define MATRIX_TYPES_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Shared header for matrix data structures and operations
 * Used by both menu_demo.c and matrix_file_ops.c to stay synchronized
 */

#define MAX_NAME_LENGTH 64

/* Matrix structure */
typedef struct {
    char name[MAX_NAME_LENGTH];
    int rows;
    int cols;
    double **data;
} Matrix;

/* Collection of matrices */
typedef struct {
    Matrix **items;
    int count;
    int capacity;
} MatrixCollection;

/* ===== Matrix lifecycle functions ===== */
Matrix *create_matrix(const char *name, int rows, int cols);
void free_matrix(Matrix *m);

/* ===== Collection management ===== */
MatrixCollection *create_collection(void);
void free_collection(MatrixCollection *c);
Matrix *find_matrix(MatrixCollection *c, const char *name);
int add_matrix(MatrixCollection *c, Matrix *m);
int remove_matrix(MatrixCollection *c, const char *name);

/* ===== Display functions ===== */
void display_matrix(const Matrix *m);
void display_all_matrices(const MatrixCollection *c);

#endif /* MATRIX_TYPES_H */

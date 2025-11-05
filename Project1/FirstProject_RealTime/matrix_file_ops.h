#ifndef MATRIX_FILE_OPS_H
#define MATRIX_FILE_OPS_H

#include "matrix_types.h"

/*
 * File I/O operations for matrix collection (options 5-9)
 * All functions synchronized with the same Matrix/MatrixCollection types
 */

/* Option 5: Read a single matrix from a file
 * File format: name\n rows cols\n data...
 * Returns: Matrix pointer or NULL on error
 */
Matrix *read_matrix_from_file(const char *filepath);

/* Option 6: Read all .txt matrices from a folder into collection
 * Returns: number of matrices successfully loaded
 */
int read_matrices_from_folder(const char *folder, MatrixCollection *col);

/* Option 7: Write a single matrix to a file
 * Returns: 1 on success, 0 on failure
 */
int write_matrix_to_file(const Matrix *m, const char *filepath);

/* Option 8: Save all matrices in collection to a folder (one file per matrix)
 * Creates folder if it doesn't exist
 * Returns: number of matrices successfully saved
 */
int save_all_matrices_to_folder(const MatrixCollection *col, const char *folder);

/* Option 9: Display summary of all matrices in collection
 * (Already declared in matrix_types.h, but included here for completeness)
 */
// void display_all_matrices(const MatrixCollection *c);

#endif /* MATRIX_FILE_OPS_H */

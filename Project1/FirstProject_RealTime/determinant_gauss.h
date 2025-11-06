#ifndef DETERMINANT_GAUSS_H
#define DETERMINANT_GAUSS_H

#include "matrix_types.h"

/*
 * Compute the determinant of a square matrix using Gaussian Elimination
 * with partial pivoting.
 *
 * Contract:
 * - Input: const Matrix* m (must be non-NULL)
 * - Requirement: m->rows == m->cols (square)
 * - Output: writes result to *out_det
 * - Returns: 1 on success, 0 on failure (e.g., not square or allocation error)
 *
 * Notes:
 * - Uses partial pivoting to improve numerical stability.
 * - If a near-zero pivot is encountered (|pivot| < 1e-12), the matrix is
 *   considered singular and the determinant is set to 0.
 */
int determinant_gauss_partial_pivot(const Matrix *m, double *out_det);

#endif /* DETERMINANT_GAUSS_H */

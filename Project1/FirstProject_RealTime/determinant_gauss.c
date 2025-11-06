#include <stdlib.h>
#include <math.h>
#include <string.h>
#include "matrix_types.h"
#include "determinant_gauss.h"

#ifndef PIVOT_EPS
#define PIVOT_EPS 1e-12
#endif

int determinant_gauss_partial_pivot(const Matrix *m, double *out_det) {
    if (!m || !out_det) return 0;
    if (m->rows != m->cols) return 0; /* not square */

    int n = m->rows;

    /* Allocate a contiguous copy of the matrix for in-place elimination */
    double *A = (double *)malloc((size_t)n * (size_t)n * sizeof(double));
    if (!A) return 0;

    for (int i = 0; i < n; ++i) {
        memcpy(A + (size_t)i * n, m->data[i], (size_t)n * sizeof(double));
    }

    double det_sign = 1.0;

    for (int k = 0; k < n; ++k) {
        /* Partial pivoting: find row with max |A[i,k]| for i >= k */
        int pivot_row = k;
        double max_abs = fabs(A[(size_t)k * n + k]);
        for (int i = k + 1; i < n; ++i) {
            double v = fabs(A[(size_t)i * n + k]);
            if (v > max_abs) {
                max_abs = v;
                pivot_row = i;
            }
        }

        /* If pivot is effectively zero, determinant is zero */
        double pivot_val = A[(size_t)pivot_row * n + k];
        if (fabs(pivot_val) < PIVOT_EPS) {
            *out_det = 0.0;
            free(A);
            return 1;
        }

        /* Swap rows if needed */
        if (pivot_row != k) {
            for (int j = k; j < n; ++j) {
                double tmp = A[(size_t)k * n + j];
                A[(size_t)k * n + j] = A[(size_t)pivot_row * n + j];
                A[(size_t)pivot_row * n + j] = tmp;
            }
            det_sign = -det_sign; /* row swap flips determinant sign */
        }

        /* Now eliminate entries below the pivot */
        double akk = A[(size_t)k * n + k];
        for (int i = k + 1; i < n; ++i) {
            double factor = A[(size_t)i * n + k] / akk;
            /* Set the element to exact zero to control growth */
            A[(size_t)i * n + k] = 0.0;
            for (int j = k + 1; j < n; ++j) {
                A[(size_t)i * n + j] -= factor * A[(size_t)k * n + j];
            }
        }
    }

    /* Determinant = sign * product of diagonal */
    double det = det_sign;
    for (int i = 0; i < n; ++i) {
        det *= A[(size_t)i * n + i];
    }

    *out_det = det;
    free(A);
    return 1;
}

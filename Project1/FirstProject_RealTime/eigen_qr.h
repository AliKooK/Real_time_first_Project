#ifndef EIGEN_QR_H
#define EIGEN_QR_H

#include "matrix_types.h"
#include "matrix_arithmetic_parallel.h" /* For PerformanceMetrics */

/* Result structure for eigenvalues and eigenvectors */
typedef struct {
    double* eigenvalues;   /* array of size n */
    Matrix* eigenvectors;  /* n x n matrix, columns are eigenvectors */
    int n;
    int iterations;
} EigenResult;

/* Free an EigenResult */
void free_eigen_result(EigenResult* res);

/* Single-threaded QR iteration to find all eigenvalues */
EigenResult* eigen_qr_single(const Matrix* m, int max_iter, double tol, double* exec_time);

/* OpenMP-parallel QR iteration (parallelize QR decomposition steps) */
EigenResult* eigen_qr_openmp(const Matrix* m, int max_iter, double tol, double* exec_time);

/* Multiprocess QR iteration (distribute QR decomposition per iteration among children) */
EigenResult* eigen_qr_multiprocess(const Matrix* m, int max_iter, double tol, double* exec_time);

/* Run all three methods and compare performance, return the fastest result */
EigenResult* run_eigen_comparison(const Matrix* m, int max_iter, double tol, PerformanceMetrics* metrics);

#endif /* EIGEN_QR_H */

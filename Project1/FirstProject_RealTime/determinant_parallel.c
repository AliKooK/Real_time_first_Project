#include "determinant_parallel.h"
#include "determinant_gauss.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <sys/time.h>
#include <unistd.h>
#include <sys/wait.h>
#ifdef _OPENMP
#include <omp.h>
#endif

#ifndef PIVOT_EPS
#define PIVOT_EPS 1e-12
#endif

static double get_time() {
    struct timeval tv; gettimeofday(&tv, NULL); return tv.tv_sec + tv.tv_usec / 1000000.0;
}

/* Helper: copy matrix into contiguous array A (row-major n*n) */
static double* copy_matrix_contiguous(const Matrix* m) {
    int n = m->rows;
    double* A = (double*)malloc((size_t)n * n * sizeof(double));
    if (!A) return NULL;
    for (int i = 0; i < n; ++i) {
        memcpy(A + (size_t)i * n, m->data[i], (size_t)n * sizeof(double));
    }
    return A;
}

/* Compute determinant via Gaussian elimination with partial pivoting (single-thread) */
int determinant_single(const Matrix* m, double* out_det, double* exec_time) {
    if (!m || !out_det || m->rows != m->cols) return 0;
    int n = m->rows;
    double* A = copy_matrix_contiguous(m); if (!A) return 0;
    double start = get_time();

    double det_sign = 1.0;
    for (int k = 0; k < n; ++k) {
        int pivot_row = k; double max_abs = fabs(A[(size_t)k * n + k]);
        for (int i = k + 1; i < n; ++i) {
            double v = fabs(A[(size_t)i * n + k]);
            if (v > max_abs) { max_abs = v; pivot_row = i; }
        }
        double pivot_val = A[(size_t)pivot_row * n + k];
        if (fabs(pivot_val) < PIVOT_EPS) { *out_det = 0.0; if (exec_time) *exec_time = get_time() - start; free(A); return 1; }
        if (pivot_row != k) {
            for (int j = k; j < n; ++j) {
                double tmp = A[(size_t)k * n + j];
                A[(size_t)k * n + j] = A[(size_t)pivot_row * n + j];
                A[(size_t)pivot_row * n + j] = tmp;
            }
            det_sign = -det_sign;
        }
        double akk = A[(size_t)k * n + k];
        for (int i = k + 1; i < n; ++i) {
            double factor = A[(size_t)i * n + k] / akk;
            A[(size_t)i * n + k] = 0.0;
            for (int j = k + 1; j < n; ++j) {
                A[(size_t)i * n + j] -= factor * A[(size_t)k * n + j];
            }
        }
    }
    double det = det_sign;
    for (int i = 0; i < n; ++i) det *= A[(size_t)i * n + i];

    if (exec_time) *exec_time = get_time() - start;
    *out_det = det; free(A); return 1;
}

/* OpenMP-parallel determinant: parallelize row updates for each k */
int determinant_openmp(const Matrix* m, double* out_det, double* exec_time) {
    if (!m || !out_det || m->rows != m->cols) return 0;
    int n = m->rows;
    double* A = copy_matrix_contiguous(m); if (!A) return 0;
    double start = get_time();

    double det_sign = 1.0;
    for (int k = 0; k < n; ++k) {
        /* pivot search (serial) */
        int pivot_row = k; double max_abs = fabs(A[(size_t)k * n + k]);
        for (int i = k + 1; i < n; ++i) {
            double v = fabs(A[(size_t)i * n + k]);
            if (v > max_abs) { max_abs = v; pivot_row = i; }
        }
        double pivot_val = A[(size_t)pivot_row * n + k];
        if (fabs(pivot_val) < PIVOT_EPS) { *out_det = 0.0; if (exec_time) *exec_time = get_time() - start; free(A); return 1; }
        if (pivot_row != k) {
            for (int j = k; j < n; ++j) {
                double tmp = A[(size_t)k * n + j];
                A[(size_t)k * n + j] = A[(size_t)pivot_row * n + j];
                A[(size_t)pivot_row * n + j] = tmp;
            }
            det_sign = -det_sign;
        }
        double akk = A[(size_t)k * n + k];
        /* parallel row updates below pivot */
        #pragma omp parallel for schedule(static)
        for (int i = k + 1; i < n; ++i) {
            double factor = A[(size_t)i * n + k] / akk;
            A[(size_t)i * n + k] = 0.0;
            for (int j = k + 1; j < n; ++j) {
                A[(size_t)i * n + j] -= factor * A[(size_t)k * n + j];
            }
        }
    }
    double det = det_sign;
    for (int i = 0; i < n; ++i) det *= A[(size_t)i * n + i];

    if (exec_time) *exec_time = get_time() - start;
    *out_det = det; free(A); return 1;
}

/* Multiprocess variant: for each k, spawn children to update rows i=k+1..n-1 */
int determinant_multiprocess(const Matrix* m, double* out_det, double* exec_time) {
    if (!m || !out_det || m->rows != m->cols) return 0;
    int n = m->rows;
    double* A = copy_matrix_contiguous(m); if (!A) return 0;
    double start = get_time();

    double det_sign = 1.0;
    for (int k = 0; k < n; ++k) {
        /* Pivoting in parent */
        int pivot_row = k; double max_abs = fabs(A[(size_t)k * n + k]);
        for (int i = k + 1; i < n; ++i) {
            double v = fabs(A[(size_t)i * n + k]);
            if (v > max_abs) { max_abs = v; pivot_row = i; }
        }
        double pivot_val = A[(size_t)pivot_row * n + k];
        if (fabs(pivot_val) < PIVOT_EPS) { *out_det = 0.0; if (exec_time) *exec_time = get_time() - start; free(A); return 1; }
        if (pivot_row != k) {
            for (int j = k; j < n; ++j) {
                double tmp = A[(size_t)k * n + j];
                A[(size_t)k * n + j] = A[(size_t)pivot_row * n + j];
                A[(size_t)pivot_row * n + j] = tmp;
            }
            det_sign = -det_sign;
        }
        double akk = A[(size_t)k * n + k];

        int rowsBelow = n - (k + 1);
        if (rowsBelow <= 0) continue;
        int (*pipes)[2] = malloc((size_t)rowsBelow * sizeof(int[2]));
        pid_t* pids = (pid_t*)malloc((size_t)rowsBelow * sizeof(pid_t));
        if (!pipes || !pids) { free(pipes); free(pids); free(A); return 0; }

        /* Fork a child per row below pivot */
        for (int idx = 0; idx < rowsBelow; ++idx) {
            int i = k + 1 + idx;
            if (pipe(pipes[idx]) == -1) { perror("pipe"); free(pipes); free(pids); free(A); return 0; }
            pid_t pid = fork();
            if (pid == -1) { perror("fork"); free(pipes); free(pids); free(A); return 0; }
            if (pid == 0) {
                /* Child: compute updated row segment [k..n-1] */
                close(pipes[idx][0]);
                int seg = n - k; /* columns k..n-1 inclusive */
                double* buf = (double*)malloc((size_t)seg * sizeof(double));
                if (!buf) { _exit(1); }
                double factor = A[(size_t)i * n + k] / akk;
                buf[0] = 0.0; /* column k becomes 0 */
                for (int j = k + 1; j < n; ++j) {
                    buf[j - k] = A[(size_t)i * n + j] - factor * A[(size_t)k * n + j];
                }
                write(pipes[idx][1], buf, (size_t)seg * sizeof(double));
                close(pipes[idx][1]);
                free(buf);
                _exit(0);
            } else {
                pids[idx] = pid;
                close(pipes[idx][1]);
            }
        }

        /* Parent: collect rows and store back into A */
        for (int idx = 0; idx < rowsBelow; ++idx) {
            int i = k + 1 + idx;
            int seg = n - k;
            size_t bytes = (size_t)seg * sizeof(double);
            double* buf = (double*)malloc(bytes);
            if (!buf) { free(pipes); free(pids); free(A); return 0; }
            read(pipes[idx][0], buf, bytes);
            close(pipes[idx][0]);
            /* write back */
            for (int j = k; j < n; ++j) {
                A[(size_t)i * n + j] = buf[j - k];
            }
            free(buf);
            waitpid(pids[idx], NULL, 0);
        }
        free(pipes);
        free(pids);
    }

    double det = det_sign;
    for (int i = 0; i < n; ++i) det *= A[(size_t)i * n + i];

    if (exec_time) *exec_time = get_time() - start;
    *out_det = det; free(A); return 1;
}

int run_determinant_comparison(const Matrix* m, PerformanceMetrics* metrics, double* out_det) {
    if (!m || !metrics || !out_det) return 0;
    if (m->rows != m->cols) return 0;

    printf("\n========================================\n");
    printf("Performance Comparison: Determinant (Gaussian Elimination)\n");
    printf("Matrix: %s (%dx%d)\n", m->name, m->rows, m->cols);
    printf("========================================\n\n");

    double det1 = 0.0, det2 = 0.0, det3 = 0.0;

    printf("[1/3] Running Single-threaded method...\n");
    determinant_single(m, &det1, &metrics->single_thread_time);
    printf("   ✓ Completed in %.6f seconds\n\n", metrics->single_thread_time);

    printf("[2/3] Running OpenMP method...\n");
    determinant_openmp(m, &det2, &metrics->openmp_time);
    printf("   ✓ Completed in %.6f seconds\n", metrics->openmp_time);
    printf("   Speedup: %.2fx\n\n", metrics->single_thread_time / metrics->openmp_time);

    printf("[3/3] Running Multiprocessing method...\n");
    determinant_multiprocess(m, &det3, &metrics->multiprocess_time);
    printf("   ✓ Completed in %.6f seconds\n", metrics->multiprocess_time);
    printf("   Speedup: %.2fx\n\n", metrics->single_thread_time / metrics->multiprocess_time);

    printf("========================================\n");
    printf("PERFORMANCE SUMMARY\n");
    printf("========================================\n");
    printf("Single-threaded:   %.6f s (baseline)\n", metrics->single_thread_time);
    printf("OpenMP:            %.6f s (%.2fx %s)\n", metrics->openmp_time,
           metrics->single_thread_time / metrics->openmp_time,
           metrics->openmp_time < metrics->single_thread_time ? "faster" : "slower");
    printf("Multiprocessing:   %.6f s (%.2fx %s)\n", metrics->multiprocess_time,
           metrics->single_thread_time / metrics->multiprocess_time,
           metrics->multiprocess_time < metrics->single_thread_time ? "faster" : "slower");
    printf("========================================\n\n");

    /* Sanity: det values should be close; choose det1 as reference */
    const char* fastest = "Single-threaded";
    double fastest_time = metrics->single_thread_time;
    double chosen_det = det1;
    if (metrics->openmp_time < fastest_time) { fastest = "OpenMP"; fastest_time = metrics->openmp_time; chosen_det = det2; }
    if (metrics->multiprocess_time < fastest_time) { fastest = "Multiprocessing"; fastest_time = metrics->multiprocess_time; chosen_det = det3; }

    printf("★ Fastest method: %s (%.6f s)\n\n", fastest, fastest_time);

    *out_det = chosen_det;
    return 1;
}

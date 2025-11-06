#include "eigen_qr.h"
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

static double get_time() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec + tv.tv_usec / 1000000.0;
}

void free_eigen_result(EigenResult* res) {
    if (!res) return;
    free(res->eigenvalues);
    if (res->eigenvectors) free_matrix(res->eigenvectors);
    free(res);
}

/* Helper: copy matrix into contiguous double array (row-major n*n) */
static double* copy_matrix_flat(const Matrix* m) {
    int n = m->rows;
    double* A = (double*)malloc((size_t)n * n * sizeof(double));
    if (!A) return NULL;
    for (int i = 0; i < n; ++i) {
        for (int j = 0; j < n; ++j) {
            A[i * n + j] = m->data[i][j];
        }
    }
    return A;
}

/* Helper: copy flat array back into Matrix */
static void copy_flat_to_matrix(const double* A, Matrix* m) {
    int n = m->rows;
    for (int i = 0; i < n; ++i) {
        for (int j = 0; j < n; ++j) {
            m->data[i][j] = A[i * n + j];
        }
    }
}

/* Gram-Schmidt QR decomposition: A = QR (single-thread)
 * Input: A (n x n flat), output: Q (n x n orthonormal), R (n x n upper triangular)
 * Returns 0 on failure, 1 on success.
 */
static int qr_decompose_single(const double* A, double* Q, double* R, int n) {
    /* Q will hold orthonormal columns; R upper triangular */
    memset(Q, 0, (size_t)n * n * sizeof(double));
    memset(R, 0, (size_t)n * n * sizeof(double));
    
    for (int j = 0; j < n; ++j) {
        /* Start with column j of A */
        for (int i = 0; i < n; ++i) {
            Q[i * n + j] = A[i * n + j];
        }
        /* Orthogonalize against previous columns */
        for (int k = 0; k < j; ++k) {
            double dot = 0.0;
            for (int i = 0; i < n; ++i) {
                dot += Q[i * n + k] * A[i * n + j];
            }
            R[k * n + j] = dot;
            for (int i = 0; i < n; ++i) {
                Q[i * n + j] -= dot * Q[i * n + k];
            }
        }
        /* Normalize */
        double norm = 0.0;
        for (int i = 0; i < n; ++i) {
            norm += Q[i * n + j] * Q[i * n + j];
        }
        norm = sqrt(norm);
        if (norm < 1e-14) {
            /* Column is zero or numerically dependent */
            return 0;
        }
        R[j * n + j] = norm;
        for (int i = 0; i < n; ++i) {
            Q[i * n + j] /= norm;
        }
    }
    return 1;
}

/* Matrix multiply: C = A * B (n x n, single-thread) */
static void matmul_single(const double* A, const double* B, double* C, int n) {
    for (int i = 0; i < n; ++i) {
        for (int j = 0; j < n; ++j) {
            double sum = 0.0;
            for (int k = 0; k < n; ++k) {
                sum += A[i * n + k] * B[k * n + j];
            }
            C[i * n + j] = sum;
        }
    }
}

/* Check convergence: max off-diagonal element < tol */
static int is_converged(const double* A, int n, double tol) {
    for (int i = 0; i < n; ++i) {
        for (int j = 0; j < n; ++j) {
            if (i != j && fabs(A[i * n + j]) > tol) {
                return 0;
            }
        }
    }
    return 1;
}

/* Extract eigenvalues from diagonal */
static void extract_eigenvalues(const double* A, double* eig, int n) {
    for (int i = 0; i < n; ++i) {
        eig[i] = A[i * n + i];
    }
}

/* ========== Single-threaded QR Iteration ========== */
EigenResult* eigen_qr_single(const Matrix* m, int max_iter, double tol, double* exec_time) {
    if (!m || m->rows != m->cols) return NULL;
    int n = m->rows;
    double start = get_time();
    
    double* A = copy_matrix_flat(m);
    if (!A) return NULL;
    
    double* Q = (double*)malloc((size_t)n * n * sizeof(double));
    double* R = (double*)malloc((size_t)n * n * sizeof(double));
    double* A_next = (double*)malloc((size_t)n * n * sizeof(double));
    double* V = (double*)malloc((size_t)n * n * sizeof(double)); // Accumulated eigenvectors
    double* V_temp = (double*)malloc((size_t)n * n * sizeof(double));
    
    if (!Q || !R || !A_next || !V || !V_temp) {
        free(A); free(Q); free(R); free(A_next); free(V); free(V_temp);
        return NULL;
    }
    
    // Initialize V as identity matrix
    memset(V, 0, (size_t)n * n * sizeof(double));
    for (int i = 0; i < n; ++i) {
        V[i * n + i] = 1.0;
    }
    
    int iter = 0;
    for (iter = 0; iter < max_iter; ++iter) {
        if (is_converged(A, n, tol)) break;
        
        if (!qr_decompose_single(A, Q, R, n)) {
            fprintf(stderr, "ERROR: QR decomposition failed (matrix may be singular or numerically rank-deficient)\n");
            free(A); free(Q); free(R); free(A_next); free(V); free(V_temp);
            return NULL;
        }
        
        /* Accumulate eigenvectors: V = V * Q */
        matmul_single(V, Q, V_temp, n);
        memcpy(V, V_temp, (size_t)n * n * sizeof(double));
        
        /* A_next = R * Q */
        matmul_single(R, Q, A_next, n);
        memcpy(A, A_next, (size_t)n * n * sizeof(double));
    }
    
    EigenResult* res = (EigenResult*)malloc(sizeof(EigenResult));
    res->n = n;
    res->iterations = iter;
    res->eigenvalues = (double*)malloc((size_t)n * sizeof(double));
    extract_eigenvalues(A, res->eigenvalues, n);
    
    /* Store accumulated eigenvectors */
    res->eigenvectors = create_matrix("Eigenvectors", n, n);
    if (res->eigenvectors) {
        copy_flat_to_matrix(V, res->eigenvectors);
    }
    
    free(A); free(Q); free(R); free(A_next); free(V); free(V_temp);
    
    if (exec_time) *exec_time = get_time() - start;
    return res;
}

/* ========== OpenMP QR Iteration (parallelize matrix ops) ========== */
static int qr_decompose_openmp(const double* A, double* Q, double* R, int n) {
    memset(Q, 0, (size_t)n * n * sizeof(double));
    memset(R, 0, (size_t)n * n * sizeof(double));
    
    for (int j = 0; j < n; ++j) {
        /* Copy column j */
        #pragma omp parallel for
        for (int i = 0; i < n; ++i) {
            Q[i * n + j] = A[i * n + j];
        }
        
        /* Orthogonalize */
        for (int k = 0; k < j; ++k) {
            double dot = 0.0;
            #pragma omp parallel for reduction(+:dot)
            for (int i = 0; i < n; ++i) {
                dot += Q[i * n + k] * A[i * n + j];
            }
            R[k * n + j] = dot;
            #pragma omp parallel for
            for (int i = 0; i < n; ++i) {
                Q[i * n + j] -= dot * Q[i * n + k];
            }
        }
        
        /* Normalize */
        double norm = 0.0;
        #pragma omp parallel for reduction(+:norm)
        for (int i = 0; i < n; ++i) {
            norm += Q[i * n + j] * Q[i * n + j];
        }
        norm = sqrt(norm);
        if (norm < 1e-14) return 0;
        R[j * n + j] = norm;
        #pragma omp parallel for
        for (int i = 0; i < n; ++i) {
            Q[i * n + j] /= norm;
        }
    }
    return 1;
}

static void matmul_openmp(const double* A, const double* B, double* C, int n) {
    #pragma omp parallel for collapse(2)
    for (int i = 0; i < n; ++i) {
        for (int j = 0; j < n; ++j) {
            double sum = 0.0;
            for (int k = 0; k < n; ++k) {
                sum += A[i * n + k] * B[k * n + j];
            }
            C[i * n + j] = sum;
        }
    }
}

EigenResult* eigen_qr_openmp(const Matrix* m, int max_iter, double tol, double* exec_time) {
    if (!m || m->rows != m->cols) return NULL;
    int n = m->rows;
    double start = get_time();
    
    double* A = copy_matrix_flat(m);
    if (!A) return NULL;
    
    double* Q = (double*)malloc((size_t)n * n * sizeof(double));
    double* R = (double*)malloc((size_t)n * n * sizeof(double));
    double* A_next = (double*)malloc((size_t)n * n * sizeof(double));
    double* V = (double*)malloc((size_t)n * n * sizeof(double)); // Accumulated eigenvectors
    double* V_temp = (double*)malloc((size_t)n * n * sizeof(double));
    
    if (!Q || !R || !A_next || !V || !V_temp) {
        free(A); free(Q); free(R); free(A_next); free(V); free(V_temp);
        return NULL;
    }
    
    // Initialize V as identity matrix
    memset(V, 0, (size_t)n * n * sizeof(double));
    for (int i = 0; i < n; ++i) {
        V[i * n + i] = 1.0;
    }
    
    int iter = 0;
    for (iter = 0; iter < max_iter; ++iter) {
        if (is_converged(A, n, tol)) break;
        
        if (!qr_decompose_openmp(A, Q, R, n)) {
            fprintf(stderr, "ERROR: QR decomposition failed (matrix may be singular or numerically rank-deficient)\n");
            free(A); free(Q); free(R); free(A_next); free(V); free(V_temp);
            return NULL;
        }
        
        /* Accumulate eigenvectors: V = V * Q */
        matmul_openmp(V, Q, V_temp, n);
        memcpy(V, V_temp, (size_t)n * n * sizeof(double));
        
        matmul_openmp(R, Q, A_next, n);
        memcpy(A, A_next, (size_t)n * n * sizeof(double));
    }
    
    EigenResult* res = (EigenResult*)malloc(sizeof(EigenResult));
    res->n = n;
    res->iterations = iter;
    res->eigenvalues = (double*)malloc((size_t)n * sizeof(double));
    extract_eigenvalues(A, res->eigenvalues, n);
    
    res->eigenvectors = create_matrix("Eigenvectors", n, n);
    if (res->eigenvectors) {
        copy_flat_to_matrix(V, res->eigenvectors);
    }
    
    free(A); free(Q); free(R); free(A_next); free(V); free(V_temp);
    
    if (exec_time) *exec_time = get_time() - start;
    return res;
}

/* ========== Multiprocess QR Iteration (fork per iteration step) ========== */
/* For simplicity, we fork a child to compute one QR step, send back Q and R via pipes.
 * This is a heavy approach but demonstrates multiprocessing.
 */
EigenResult* eigen_qr_multiprocess(const Matrix* m, int max_iter, double tol, double* exec_time) {
    if (!m || m->rows != m->cols) return NULL;
    int n = m->rows;
    double start = get_time();
    
    double* A = copy_matrix_flat(m);
    if (!A) return NULL;
    
    double* Q = (double*)malloc((size_t)n * n * sizeof(double));
    double* R = (double*)malloc((size_t)n * n * sizeof(double));
    double* A_next = (double*)malloc((size_t)n * n * sizeof(double));
    double* V = (double*)malloc((size_t)n * n * sizeof(double)); // Accumulated eigenvectors
    double* V_temp = (double*)malloc((size_t)n * n * sizeof(double));
    
    if (!Q || !R || !A_next || !V || !V_temp) {
        free(A); free(Q); free(R); free(A_next); free(V); free(V_temp);
        return NULL;
    }
    
    // Initialize V as identity matrix
    memset(V, 0, (size_t)n * n * sizeof(double));
    for (int i = 0; i < n; ++i) {
        V[i * n + i] = 1.0;
    }
    
    int iter = 0;
    for (iter = 0; iter < max_iter; ++iter) {
        if (is_converged(A, n, tol)) break;
        
        /* Fork a child to compute QR decomposition */
        int pipeQR[2];
        if (pipe(pipeQR) == -1) {
            perror("pipe");
            free(A); free(Q); free(R); free(A_next); free(V); free(V_temp);
            return NULL;
        }
        
        pid_t pid = fork();
        if (pid == -1) {
            perror("fork");
            free(A); free(Q); free(R); free(A_next); free(V); free(V_temp);
            return NULL;
        }
        
        if (pid == 0) {
            /* Child: compute QR, send Q and R back */
            close(pipeQR[0]);
            if (!qr_decompose_single(A, Q, R, n)) {
                _exit(1);
            }
            size_t bytes = (size_t)n * n * sizeof(double);
            write(pipeQR[1], Q, bytes);
            write(pipeQR[1], R, bytes);
            close(pipeQR[1]);
            _exit(0);
        } else {
            /* Parent: read Q and R */
            close(pipeQR[1]);
            size_t bytes = (size_t)n * n * sizeof(double);
            read(pipeQR[0], Q, bytes);
            read(pipeQR[0], R, bytes);
            close(pipeQR[0]);
            waitpid(pid, NULL, 0);
            
            /* Accumulate eigenvectors: V = V * Q */
            matmul_single(V, Q, V_temp, n);
            memcpy(V, V_temp, (size_t)n * n * sizeof(double));
            
            /* A_next = R * Q */
            matmul_single(R, Q, A_next, n);
            memcpy(A, A_next, (size_t)n * n * sizeof(double));
        }
    }
    
    EigenResult* res = (EigenResult*)malloc(sizeof(EigenResult));
    res->n = n;
    res->iterations = iter;
    res->eigenvalues = (double*)malloc((size_t)n * sizeof(double));
    extract_eigenvalues(A, res->eigenvalues, n);
    
    res->eigenvectors = create_matrix("Eigenvectors", n, n);
    if (res->eigenvectors) {
        copy_flat_to_matrix(V, res->eigenvectors);
    }
    
    free(A); free(Q); free(R); free(A_next); free(V); free(V_temp);
    
    if (exec_time) *exec_time = get_time() - start;
    return res;
}

/* ========== Performance Comparison ========== */
EigenResult* run_eigen_comparison(const Matrix* m, int max_iter, double tol, PerformanceMetrics* metrics) {
    if (!m || !metrics || m->rows != m->cols) return NULL;
    
    printf("\n========================================\n");
    printf("Performance Comparison: QR Iteration (Eigenvalues)\n");
    printf("Matrix: %s (%dx%d)\n", m->name, m->rows, m->cols);
    printf("Max iterations: %d, Tolerance: %.2e\n", max_iter, tol);
    printf("========================================\n\n");
    
    EigenResult *res1 = NULL, *res2 = NULL, *res3 = NULL;
    
    printf("[1/3] Running Single-threaded method...\n");
    res1 = eigen_qr_single(m, max_iter, tol, &metrics->single_thread_time);
    if (res1) {
        printf("   ✓ Completed in %.6f seconds (%d iterations)\n\n", 
               metrics->single_thread_time, res1->iterations);
    } else {
        printf("   ✗ Failed\n\n");
    }
    
    printf("[2/3] Running OpenMP method...\n");
    res2 = eigen_qr_openmp(m, max_iter, tol, &metrics->openmp_time);
    if (res2) {
        printf("   ✓ Completed in %.6f seconds (%d iterations)\n", 
               metrics->openmp_time, res2->iterations);
        if (res1) {
            printf("   Speedup: %.2fx\n\n", metrics->single_thread_time / metrics->openmp_time);
        } else {
            printf("\n");
        }
    } else {
        printf("   ✗ Failed\n\n");
    }
    
    printf("[3/3] Running Multiprocessing method...\n");
    res3 = eigen_qr_multiprocess(m, max_iter, tol, &metrics->multiprocess_time);
    if (res3) {
        printf("   ✓ Completed in %.6f seconds (%d iterations)\n", 
               metrics->multiprocess_time, res3->iterations);
        if (res1) {
            printf("   Speedup: %.2fx\n\n", metrics->single_thread_time / metrics->multiprocess_time);
        } else {
            printf("\n");
        }
    } else {
        printf("   ✗ Failed\n\n");
    }
    
    printf("========================================\n");
    printf("PERFORMANCE SUMMARY\n");
    printf("========================================\n");
    if (res1) {
        printf("Single-threaded:   %.6f s (baseline)\n", metrics->single_thread_time);
    }
    if (res2) {
        printf("OpenMP:            %.6f s", metrics->openmp_time);
        if (res1) {
            printf(" (%.2fx %s)", metrics->single_thread_time / metrics->openmp_time,
                   metrics->openmp_time < metrics->single_thread_time ? "faster" : "slower");
        }
        printf("\n");
    }
    if (res3) {
        printf("Multiprocessing:   %.6f s", metrics->multiprocess_time);
        if (res1) {
            printf(" (%.2fx %s)", metrics->single_thread_time / metrics->multiprocess_time,
                   metrics->multiprocess_time < metrics->single_thread_time ? "faster" : "slower");
        }
        printf("\n");
    }
    printf("========================================\n\n");
    
    /* Choose the fastest result that succeeded */
    EigenResult* fastest = res1;
    double fastest_time = res1 ? metrics->single_thread_time : 1e99;
    const char* fastest_name = "Single-threaded";
    
    if (res2 && metrics->openmp_time < fastest_time) {
        fastest = res2;
        fastest_time = metrics->openmp_time;
        fastest_name = "OpenMP";
    }
    if (res3 && metrics->multiprocess_time < fastest_time) {
        fastest = res3;
        fastest_time = metrics->multiprocess_time;
        fastest_name = "Multiprocessing";
    }
    
    if (fastest) {
        printf("★ Fastest method: %s (%.6f s)\n\n", fastest_name, fastest_time);
    }
    
    /* Clean up non-fastest results */
    if (res1 && res1 != fastest) free_eigen_result(res1);
    if (res2 && res2 != fastest) free_eigen_result(res2);
    if (res3 && res3 != fastest) free_eigen_result(res3);
    
    return fastest;
}

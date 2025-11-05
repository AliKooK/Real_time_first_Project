#include "matrix_arithmetic_parallel.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include <omp.h>

// Get current time in seconds
static double get_time() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec + tv.tv_usec / 1000000.0;
}

// ============================================================================
// ADDITION OPERATIONS
// ============================================================================

Matrix* add_matrices_single(const Matrix* m1, const Matrix* m2, const char* result_name, double* exec_time) {
    if (!m1 || !m2 || !result_name) return NULL;
    if (m1->rows != m2->rows || m1->cols != m2->cols) {
        fprintf(stderr, "Error: Matrix dimensions incompatible for addition\n");
        return NULL;
    }

    double start = get_time();
    
    Matrix* result = create_matrix(result_name, m1->rows, m1->cols);
    if (!result) return NULL;

    // Single-threaded computation
    for (int i = 0; i < m1->rows; i++) {
        for (int j = 0; j < m1->cols; j++) {
            result->data[i][j] = m1->data[i][j] + m2->data[i][j];
        }
    }

    double end = get_time();
    *exec_time = end - start;

    return result;
}

Matrix* add_matrices_openmp(const Matrix* m1, const Matrix* m2, const char* result_name, double* exec_time) {
    if (!m1 || !m2 || !result_name) return NULL;
    if (m1->rows != m2->rows || m1->cols != m2->cols) {
        fprintf(stderr, "Error: Matrix dimensions incompatible for addition\n");
        return NULL;
    }

    double start = get_time();
    
    Matrix* result = create_matrix(result_name, m1->rows, m1->cols);
    if (!result) return NULL;

    // OpenMP parallelization
    #pragma omp parallel for collapse(2) schedule(static)
    for (int i = 0; i < m1->rows; i++) {
        for (int j = 0; j < m1->cols; j++) {
            result->data[i][j] = m1->data[i][j] + m2->data[i][j];
        }
    }

    double end = get_time();
    *exec_time = end - start;

    return result;
}

Matrix* add_matrices_multiprocess(const Matrix* m1, const Matrix* m2, const char* result_name, double* exec_time) {
    if (!m1 || !m2 || !result_name) return NULL;
    if (m1->rows != m2->rows || m1->cols != m2->cols) {
        fprintf(stderr, "Error: Matrix dimensions incompatible for addition\n");
        return NULL;
    }

    double start = get_time();
    
    Matrix* result = create_matrix(result_name, m1->rows, m1->cols);
    if (!result) return NULL;

    int total_elements = m1->rows * m1->cols;
    int pipes[total_elements][2];  // One pipe per element
    pid_t* pids = malloc(total_elements * sizeof(pid_t));
    
    if (!pids) {
        free_matrix(result);
        return NULL;
    }

    // Create one child process per element
    int elem_idx = 0;
    for (int i = 0; i < m1->rows; i++) {
        for (int j = 0; j < m1->cols; j++) {
            if (pipe(pipes[elem_idx]) == -1) {
                perror("pipe");
                free(pids);
                free_matrix(result);
                return NULL;
            }

            pid_t pid = fork();
            if (pid == -1) {
                perror("fork");
                free(pids);
                free_matrix(result);
                return NULL;
            }

            if (pid == 0) {
                // Child process: compute one element
                close(pipes[elem_idx][0]);  // Close read end
                
                double sum = m1->data[i][j] + m2->data[i][j];
                write(pipes[elem_idx][1], &sum, sizeof(double));
                
                close(pipes[elem_idx][1]);
                exit(0);
            } else {
                // Parent process
                pids[elem_idx] = pid;
                close(pipes[elem_idx][1]);  // Close write end
                elem_idx++;
            }
        }
    }

    // Collect results from all children
    elem_idx = 0;
    for (int i = 0; i < m1->rows; i++) {
        for (int j = 0; j < m1->cols; j++) {
            double sum;
            read(pipes[elem_idx][0], &sum, sizeof(double));
            result->data[i][j] = sum;
            close(pipes[elem_idx][0]);
            
            // Wait for child
            waitpid(pids[elem_idx], NULL, 0);
            elem_idx++;
        }
    }

    free(pids);

    double end = get_time();
    *exec_time = end - start;

    return result;
}

// ============================================================================
// SUBTRACTION OPERATIONS
// ============================================================================

Matrix* subtract_matrices_single(const Matrix* m1, const Matrix* m2, const char* result_name, double* exec_time) {
    if (!m1 || !m2 || !result_name) return NULL;
    if (m1->rows != m2->rows || m1->cols != m2->cols) {
        fprintf(stderr, "Error: Matrix dimensions incompatible for subtraction\n");
        return NULL;
    }

    double start = get_time();
    
    Matrix* result = create_matrix(result_name, m1->rows, m1->cols);
    if (!result) return NULL;

    // Single-threaded computation
    for (int i = 0; i < m1->rows; i++) {
        for (int j = 0; j < m1->cols; j++) {
            result->data[i][j] = m1->data[i][j] - m2->data[i][j];
        }
    }

    double end = get_time();
    *exec_time = end - start;

    return result;
}

Matrix* subtract_matrices_openmp(const Matrix* m1, const Matrix* m2, const char* result_name, double* exec_time) {
    if (!m1 || !m2 || !result_name) return NULL;
    if (m1->rows != m2->rows || m1->cols != m2->cols) {
        fprintf(stderr, "Error: Matrix dimensions incompatible for subtraction\n");
        return NULL;
    }

    double start = get_time();
    
    Matrix* result = create_matrix(result_name, m1->rows, m1->cols);
    if (!result) return NULL;

    // OpenMP parallelization
    #pragma omp parallel for collapse(2) schedule(static)
    for (int i = 0; i < m1->rows; i++) {
        for (int j = 0; j < m1->cols; j++) {
            result->data[i][j] = m1->data[i][j] - m2->data[i][j];
        }
    }

    double end = get_time();
    *exec_time = end - start;

    return result;
}

Matrix* subtract_matrices_multiprocess(const Matrix* m1, const Matrix* m2, const char* result_name, double* exec_time) {
    if (!m1 || !m2 || !result_name) return NULL;
    if (m1->rows != m2->rows || m1->cols != m2->cols) {
        fprintf(stderr, "Error: Matrix dimensions incompatible for subtraction\n");
        return NULL;
    }

    double start = get_time();
    
    Matrix* result = create_matrix(result_name, m1->rows, m1->cols);
    if (!result) return NULL;

    int total_elements = m1->rows * m1->cols;
    int pipes[total_elements][2];
    pid_t* pids = malloc(total_elements * sizeof(pid_t));
    
    if (!pids) {
        free_matrix(result);
        return NULL;
    }

    // Create one child process per element
    int elem_idx = 0;
    for (int i = 0; i < m1->rows; i++) {
        for (int j = 0; j < m1->cols; j++) {
            if (pipe(pipes[elem_idx]) == -1) {
                perror("pipe");
                free(pids);
                free_matrix(result);
                return NULL;
            }

            pid_t pid = fork();
            if (pid == -1) {
                perror("fork");
                free(pids);
                free_matrix(result);
                return NULL;
            }

            if (pid == 0) {
                // Child process: compute one element
                close(pipes[elem_idx][0]);
                
                double diff = m1->data[i][j] - m2->data[i][j];
                write(pipes[elem_idx][1], &diff, sizeof(double));
                
                close(pipes[elem_idx][1]);
                exit(0);
            } else {
                // Parent process
                pids[elem_idx] = pid;
                close(pipes[elem_idx][1]);
                elem_idx++;
            }
        }
    }

    // Collect results from all children
    elem_idx = 0;
    for (int i = 0; i < m1->rows; i++) {
        for (int j = 0; j < m1->cols; j++) {
            double diff;
            read(pipes[elem_idx][0], &diff, sizeof(double));
            result->data[i][j] = diff;
            close(pipes[elem_idx][0]);
            
            waitpid(pids[elem_idx], NULL, 0);
            elem_idx++;
        }
    }

    free(pids);

    double end = get_time();
    *exec_time = end - start;

    return result;
}

// ============================================================================
// MULTIPLICATION OPERATIONS
// ============================================================================

Matrix* multiply_matrices_single(const Matrix* m1, const Matrix* m2, const char* result_name, double* exec_time) {
    if (!m1 || !m2 || !result_name) return NULL;
    if (m1->cols != m2->rows) {
        fprintf(stderr, "Error: Matrix dimensions incompatible for multiplication\n");
        return NULL;
    }

    double start = get_time();
    
    Matrix* result = create_matrix(result_name, m1->rows, m2->cols);
    if (!result) return NULL;

    // Single-threaded computation
    for (int i = 0; i < m1->rows; i++) {
        for (int j = 0; j < m2->cols; j++) {
            double sum = 0.0;
            for (int k = 0; k < m1->cols; k++) {
                sum += m1->data[i][k] * m2->data[k][j];
            }
            result->data[i][j] = sum;
        }
    }

    double end = get_time();
    *exec_time = end - start;

    return result;
}

Matrix* multiply_matrices_openmp(const Matrix* m1, const Matrix* m2, const char* result_name, double* exec_time) {
    if (!m1 || !m2 || !result_name) return NULL;
    if (m1->cols != m2->rows) {
        fprintf(stderr, "Error: Matrix dimensions incompatible for multiplication\n");
        return NULL;
    }

    double start = get_time();
    
    Matrix* result = create_matrix(result_name, m1->rows, m2->cols);
    if (!result) return NULL;

    // OpenMP parallelization
    #pragma omp parallel for collapse(2) schedule(static)
    for (int i = 0; i < m1->rows; i++) {
        for (int j = 0; j < m2->cols; j++) {
            double sum = 0.0;
            for (int k = 0; k < m1->cols; k++) {
                sum += m1->data[i][k] * m2->data[k][j];
            }
            result->data[i][j] = sum;
        }
    }

    double end = get_time();
    *exec_time = end - start;

    return result;
}

Matrix* multiply_matrices_multiprocess(const Matrix* m1, const Matrix* m2, const char* result_name, double* exec_time) {
    if (!m1 || !m2 || !result_name) return NULL;
    if (m1->cols != m2->rows) {
        fprintf(stderr, "Error: Matrix dimensions incompatible for multiplication\n");
        return NULL;
    }

    double start = get_time();
    
    Matrix* result = create_matrix(result_name, m1->rows, m2->cols);
    if (!result) return NULL;

    int total_elements = m1->rows * m2->cols;
    int pipes[total_elements][2];
    pid_t* pids = malloc(total_elements * sizeof(pid_t));
    
    if (!pids) {
        free_matrix(result);
        return NULL;
    }

    // Create one child process per result element
    int elem_idx = 0;
    for (int i = 0; i < m1->rows; i++) {
        for (int j = 0; j < m2->cols; j++) {
            if (pipe(pipes[elem_idx]) == -1) {
                perror("pipe");
                free(pids);
                free_matrix(result);
                return NULL;
            }

            pid_t pid = fork();
            if (pid == -1) {
                perror("fork");
                free(pids);
                free_matrix(result);
                return NULL;
            }

            if (pid == 0) {
                // Child process: compute one element (row i × column j)
                close(pipes[elem_idx][0]);
                
                double sum = 0.0;
                for (int k = 0; k < m1->cols; k++) {
                    sum += m1->data[i][k] * m2->data[k][j];
                }
                
                write(pipes[elem_idx][1], &sum, sizeof(double));
                close(pipes[elem_idx][1]);
                exit(0);
            } else {
                // Parent process
                pids[elem_idx] = pid;
                close(pipes[elem_idx][1]);
                elem_idx++;
            }
        }
    }

    // Collect results from all children
    elem_idx = 0;
    for (int i = 0; i < m1->rows; i++) {
        for (int j = 0; j < m2->cols; j++) {
            double product;
            read(pipes[elem_idx][0], &product, sizeof(double));
            result->data[i][j] = product;
            close(pipes[elem_idx][0]);
            
            waitpid(pids[elem_idx], NULL, 0);
            elem_idx++;
        }
    }

    free(pids);

    double end = get_time();
    *exec_time = end - start;

    return result;
}

// ============================================================================
// COMPARISON FUNCTION
// ============================================================================

Matrix* run_operation_comparison(const Matrix* m1, const Matrix* m2, const char* result_name, 
                                  const char* operation, PerformanceMetrics* metrics) {
    if (!m1 || !m2 || !result_name || !operation || !metrics) return NULL;

    printf("\n========================================\n");
    printf("Performance Comparison: %s\n", operation);
    printf("Matrix 1: %s (%dx%d), Matrix 2: %s (%dx%d)\n", 
           m1->name, m1->rows, m1->cols, m2->name, m2->rows, m2->cols);
    printf("========================================\n\n");

    Matrix *result1 = NULL, *result2 = NULL, *result3 = NULL;
    char temp_name[128];

    // Method 1: Single-threaded
    printf("[1/3] Running Single-threaded method...\n");
    snprintf(temp_name, sizeof(temp_name), "%s_single", result_name);
    if (strcmp(operation, "Addition") == 0) {
        result1 = add_matrices_single(m1, m2, temp_name, &metrics->single_thread_time);
    } else if (strcmp(operation, "Subtraction") == 0) {
        result1 = subtract_matrices_single(m1, m2, temp_name, &metrics->single_thread_time);
    } else if (strcmp(operation, "Multiplication") == 0) {
        result1 = multiply_matrices_single(m1, m2, temp_name, &metrics->single_thread_time);
    }
    printf("   ✓ Completed in %.6f seconds\n\n", metrics->single_thread_time);

    // Method 2: OpenMP
    printf("[2/3] Running OpenMP method...\n");
    snprintf(temp_name, sizeof(temp_name), "%s_openmp", result_name);
    if (strcmp(operation, "Addition") == 0) {
        result2 = add_matrices_openmp(m1, m2, temp_name, &metrics->openmp_time);
    } else if (strcmp(operation, "Subtraction") == 0) {
        result2 = subtract_matrices_openmp(m1, m2, temp_name, &metrics->openmp_time);
    } else if (strcmp(operation, "Multiplication") == 0) {
        result2 = multiply_matrices_openmp(m1, m2, temp_name, &metrics->openmp_time);
    }
    printf("   ✓ Completed in %.6f seconds\n", metrics->openmp_time);
    printf("   Speedup: %.2fx\n\n", metrics->single_thread_time / metrics->openmp_time);

    // Method 3: Multiprocessing
    printf("[3/3] Running Multiprocessing method...\n");
    snprintf(temp_name, sizeof(temp_name), "%s_multiproc", result_name);
    if (strcmp(operation, "Addition") == 0) {
        result3 = add_matrices_multiprocess(m1, m2, temp_name, &metrics->multiprocess_time);
    } else if (strcmp(operation, "Subtraction") == 0) {
        result3 = subtract_matrices_multiprocess(m1, m2, temp_name, &metrics->multiprocess_time);
    } else if (strcmp(operation, "Multiplication") == 0) {
        result3 = multiply_matrices_multiprocess(m1, m2, temp_name, &metrics->multiprocess_time);
    }
    printf("   ✓ Completed in %.6f seconds\n", metrics->multiprocess_time);
    printf("   Speedup: %.2fx\n\n", metrics->single_thread_time / metrics->multiprocess_time);

    // Summary
    printf("========================================\n");
    printf("PERFORMANCE SUMMARY\n");
    printf("========================================\n");
    printf("Single-threaded:   %.6f s (baseline)\n", metrics->single_thread_time);
    printf("OpenMP:            %.6f s (%.2fx %s)\n", 
           metrics->openmp_time, 
           metrics->single_thread_time / metrics->openmp_time,
           metrics->openmp_time < metrics->single_thread_time ? "faster" : "slower");
    printf("Multiprocessing:   %.6f s (%.2fx %s)\n", 
           metrics->multiprocess_time,
           metrics->single_thread_time / metrics->multiprocess_time,
           metrics->multiprocess_time < metrics->single_thread_time ? "faster" : "slower");
    printf("========================================\n\n");

    // Determine fastest method
    const char* fastest = "Single-threaded";
    double fastest_time = metrics->single_thread_time;
    Matrix* fastest_result = result1;
    
    if (metrics->openmp_time < fastest_time) {
        fastest = "OpenMP";
        fastest_time = metrics->openmp_time;
        fastest_result = result2;
    }
    if (metrics->multiprocess_time < fastest_time) {
        fastest = "Multiprocessing";
        fastest_time = metrics->multiprocess_time;
        fastest_result = result3;
    }

    printf("★ Fastest method: %s (%.6f s)\n\n", fastest, fastest_time);

    // Clean up temporary results (keep the fastest one to return)
    if (result1 && result1 != fastest_result) free_matrix(result1);
    if (result2 && result2 != fastest_result) free_matrix(result2);
    if (result3 && result3 != fastest_result) free_matrix(result3);

    // Rename the fastest result to the requested name
    if (fastest_result) {
        strncpy(fastest_result->name, result_name, sizeof(fastest_result->name) - 1);
        fastest_result->name[sizeof(fastest_result->name) - 1] = '\0';
    }

    return fastest_result;
}

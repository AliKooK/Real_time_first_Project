#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <signal.h>
#include "matrix_types.h"
#include "matrix_file_ops.h"
#include "matrix_arithmetic.h"
#include "matrix_arithmetic_parallel.h"
#include "determinant_gauss.h"
#include "determinant_parallel.h"
#include "eigen_qr.h"

/*
 * Professional interactive menu (modular version)
 * Options 1-4: manual entry/display/delete/modify (defined here)
 * Options 5-9: file I/O operations (in matrix_file_ops.c)
 * Shared data structures in matrix_types.h and matrix_utils.c
 */

static volatile sig_atomic_t g_interrupted = 0;

static void on_sigint(int sig) {
    (void)sig;
    g_interrupted = 1;
}

static void trim_newline(char *s) {
    if (!s) return;
    size_t n = strlen(s);
    if (n > 0 && (s[n-1] == '\n' || s[n-1] == '\r')) s[n-1] = '\0';
}

static int read_int_choice(const char *prompt, int *out) {
    char buf[256];
    fputs(prompt, stdout);
    fflush(stdout);

    if (!fgets(buf, sizeof(buf), stdin)) return -1;
    trim_newline(buf);

    char *p = buf;
    while (*p && isspace((unsigned char)*p)) p++;
    if (*p == '\0') return 0;

    char *end = NULL;
    long val = strtol(p, &end, 10);
    if (p == end) return 0;
    while (end && *end) {
        if (!isspace((unsigned char)*end)) return 0;
        end++;
    }

    *out = (int)val;
    return 1;
}

static void press_enter_to_continue(void) {
    fputs("\nPress Enter to continue...", stdout);
    fflush(stdout);
    int c = fgetc(stdin);
    if (c == EOF) {
        fputs("\nEnd of input detected. Exiting...\n", stdout);
    }
}

static void clear_screen(void) {
    fputs("\033[2J\033[H", stdout);
}

static void print_header(void) {
    puts("");
    puts("╔════════════════════════════════════════════════════════════╗");
    puts("║         MATRIX OPERATIONS - MULTI-PROCESSING TOOL          ║");
    puts("╚════════════════════════════════════════════════════════════╝");
    puts("");
}

static void print_menu(void) {
    puts("  [1]  Enter a matrix");
    puts("  [2]  Display a matrix");
    puts("  [3]  Delete a matrix");
    puts("  [4]  Modify a matrix (row/col/value)");
    puts("  [5]  Read a matrix from a file");
    puts("  [6]  Read a set of matrices from a folder");
    puts("  [7]  Save a matrix to a file");
    puts("  [8]  Save all matrices in memory to a folder");
    puts("  [9]  Display all matrices in memory");
    puts("  [10] Add 2 matrices");
    puts("  [11] Subtract 2 matrices");
    puts("  [12] Multiply 2 matrices");
    puts("  [13] Find the determinant of a matrix");
    puts("  [14] Find eigenvalues & eigenvectors of a matrix");
    puts("  [15] Exit");
    puts("");
    puts("════════════════════════════════════════════════════════════");
}

/* ===== Input helpers ===== */
static int read_line_prompt(const char *prompt, char *buf, size_t n) {
    fputs(prompt, stdout);
    fflush(stdout);
    if (!fgets(buf, n, stdin)) return -1;
    trim_newline(buf);
    return (int)strlen(buf);
}

static int read_int_prompt(const char *prompt, int *out) {
    return read_int_choice(prompt, out);
}

static int read_double_prompt(const char *prompt, double *out) {
    char buf[256];
    fputs(prompt, stdout);
    fflush(stdout);
    if (!fgets(buf, sizeof(buf), stdin)) return -1;
    trim_newline(buf);
    char *p = buf; while (*p && isspace((unsigned char)*p)) p++;
    if (*p == '\0') return 0;
    char *end = NULL; double v = strtod(p, &end);
    if (p == end) return 0;
    while (end && *end) { if (!isspace((unsigned char)*end)) return 0; end++; }
    *out = v; return 1;
}

/* ===== Options 1-4 handlers ===== */
static void handle_enter_matrix(MatrixCollection *col) {
    puts("--- Enter a Matrix ---");
    char name[MAX_NAME_LENGTH];
    int r, c, rc;

    rc = read_line_prompt("Enter matrix name: ", name, sizeof(name));
    if (rc == -1) { puts("EOF. Exiting..."); return; }
    if (rc == 0) { puts("Name cannot be empty."); return; }
    if (find_matrix(col, name)) { puts("Matrix already exists."); return; }

    rc = read_int_prompt("Enter number of rows: ", &r);
    if (rc == -1) { puts("EOF. Exiting..."); return; }
    if (rc != 1 || r <= 0) { puts("Invalid rows."); return; }

    rc = read_int_prompt("Enter number of columns: ", &c);
    if (rc == -1) { puts("EOF. Exiting..."); return; }
    if (rc != 1 || c <= 0) { puts("Invalid columns."); return; }

    Matrix *m = create_matrix(name, r, c);
    if (!m) { puts("Allocation failed."); return; }

    puts("\nEnter matrix elements (row by row):");
    for (int i = 0; i < r; i++) {
        for (int j = 0; j < c; j++) {
            char prompt[80]; snprintf(prompt, sizeof(prompt), "a[%d][%d] = ", i, j);
            double val; int drc = read_double_prompt(prompt, &val);
            if (drc == -1) { puts("EOF. Cancelling."); free_matrix(m); return; }
            if (drc != 1) { puts("Invalid number. Cancelling."); free_matrix(m); return; }
            m->data[i][j] = val;
        }
    }

    if (!add_matrix(col, m)) {
        puts("Failed to add matrix.");
        free_matrix(m);
        return;
    }
    printf("\nMatrix '%s' (%dx%d) added successfully!\n", name, r, c);
}

static void handle_display_matrix(MatrixCollection *col) {
    puts("--- Display a Matrix ---");
    char name[MAX_NAME_LENGTH];
    int rc = read_line_prompt("Enter matrix name: ", name, sizeof(name));
    if (rc == -1) { puts("EOF. Exiting..."); return; }
    if (rc == 0) { puts("Name cannot be empty."); return; }
    Matrix *m = find_matrix(col, name);
    if (!m) { printf("Matrix '%s' not found.\n", name); return; }
    display_matrix(m);
}

static void handle_delete_matrix(MatrixCollection *col) {
    puts("--- Delete a Matrix ---");
    char name[MAX_NAME_LENGTH];
    int rc = read_line_prompt("Enter matrix name to delete: ", name, sizeof(name));
    if (rc == -1) { puts("EOF. Exiting..."); return; }
    if (rc == 0) { puts("Name cannot be empty."); return; }
    if (remove_matrix(col, name)) {
        printf("Matrix '%s' deleted successfully.\n", name);
    } else {
        printf("Matrix '%s' not found.\n", name);
    }
}

static void handle_modify_matrix(MatrixCollection *col) {
    puts("--- Modify a Matrix ---");
    char name[MAX_NAME_LENGTH];
    int rc = read_line_prompt("Enter matrix name: ", name, sizeof(name));
    if (rc == -1) { puts("EOF. Exiting..."); return; }
    if (rc == 0) { puts("Name cannot be empty."); return; }
    Matrix *m = find_matrix(col, name);
    if (!m) { printf("Matrix '%s' not found.\n", name); return; }

    puts("1. Modify a specific element");
    puts("2. Modify entire row");
    puts("3. Modify entire column");
    int choice; rc = read_int_prompt("Enter choice: ", &choice);
    if (rc == -1) { puts("EOF. Exiting..."); return; }
    if (rc != 1) { puts("Invalid input."); return; }

    if (choice == 1) {
        int i, j; double v;
        if (read_int_prompt("Row index: ", &i) != 1 || i < 0 || i >= m->rows) { puts("Invalid row."); return; }
        if (read_int_prompt("Col index: ", &j) != 1 || j < 0 || j >= m->cols) { puts("Invalid column."); return; }
        if (read_double_prompt("New value: ", &v) != 1) { puts("Invalid value."); return; }
        m->data[i][j] = v;
        printf("Updated a[%d][%d] = %.4f\n", i, j, v);
    } else if (choice == 2) {
        int i; if (read_int_prompt("Row index: ", &i) != 1 || i < 0 || i >= m->rows) { puts("Invalid row."); return; }
        for (int j = 0; j < m->cols; j++) {
            char prompt[64]; snprintf(prompt, sizeof(prompt), "value[%d][%d]: ", i, j);
            double v; if (read_double_prompt(prompt, &v) != 1) { puts("Invalid value."); return; }
            m->data[i][j] = v;
        }
        printf("Row %d updated.\n", i);
    } else if (choice == 3) {
        int j; if (read_int_prompt("Column index: ", &j) != 1 || j < 0 || j >= m->cols) { puts("Invalid column."); return; }
        for (int i = 0; i < m->rows; i++) {
            char prompt[64]; snprintf(prompt, sizeof(prompt), "value[%d][%d]: ", i, j);
            double v; if (read_double_prompt(prompt, &v) != 1) { puts("Invalid value."); return; }
            m->data[i][j] = v;
        }
        printf("Column %d updated.\n", j);
    } else {
        puts("Invalid choice.");
    }
}

/* ===== Options 5-9 handlers (wrappers for file ops) ===== */
static void handle_read_from_file(MatrixCollection *col) {
    puts("--- Read Matrix from File ---");
    char path[512];
    int rc = read_line_prompt("Enter file path: ", path, sizeof(path));
    if (rc == -1) { puts("EOF. Exiting..."); return; }
    if (rc == 0) { puts("Path cannot be empty."); return; }

    Matrix *m = read_matrix_from_file(path);
    if (m) {
        if (add_matrix(col, m)) {
            printf("Matrix '%s' added to collection.\n", m->name);
        } else {
            printf("Matrix '%s' already exists or failed to add.\n", m->name);
            free_matrix(m);
        }
    }
}

static void handle_read_from_folder(MatrixCollection *col) {
    puts("--- Read Matrices from Folder ---");
    char path[512];
    int rc = read_line_prompt("Enter folder path: ", path, sizeof(path));
    if (rc == -1) { puts("EOF. Exiting..."); return; }
    if (rc == 0) { puts("Path cannot be empty."); return; }

    read_matrices_from_folder(path, col);
}

static void handle_save_to_file(MatrixCollection *col) {
    puts("--- Save Matrix to File ---");
    char name[MAX_NAME_LENGTH];
    int rc = read_line_prompt("Enter matrix name: ", name, sizeof(name));
    if (rc == -1) { puts("EOF. Exiting..."); return; }
    if (rc == 0) { puts("Name cannot be empty."); return; }

    Matrix *m = find_matrix(col, name);
    if (!m) { printf("Matrix '%s' not found.\n", name); return; }

    char path[512];
    rc = read_line_prompt("Enter file path: ", path, sizeof(path));
    if (rc == -1) { puts("EOF. Exiting..."); return; }
    if (rc == 0) { puts("Path cannot be empty."); return; }

    write_matrix_to_file(m, path);
}

static void handle_save_all(MatrixCollection *col) {
    puts("--- Save All Matrices to Folder ---");
    if (col->count == 0) {
        puts("No matrices in memory to save.");
        return;
    }

    char path[512];
    int rc = read_line_prompt("Enter folder path: ", path, sizeof(path));
    if (rc == -1) { puts("EOF. Exiting..."); return; }
    if (rc == 0) { puts("Path cannot be empty."); return; }

    save_all_matrices_to_folder(col, path);
}

static void handle_add_matrices(MatrixCollection *col) {
    puts("--- Add Two Matrices (Performance Comparison) ---");
    if (col->count < 2) {
        puts("Need at least 2 matrices in memory to perform addition.");
        return;
    }

    char name1[64], name2[64], result_name[64];
    
    int rc = read_line_prompt("Enter first matrix name: ", name1, sizeof(name1));
    if (rc == -1) { puts("EOF. Exiting..."); return; }
    if (rc == 0) { puts("Name cannot be empty."); return; }

    rc = read_line_prompt("Enter second matrix name: ", name2, sizeof(name2));
    if (rc == -1) { puts("EOF. Exiting..."); return; }
    if (rc == 0) { puts("Name cannot be empty."); return; }

    rc = read_line_prompt("Enter result matrix name: ", result_name, sizeof(result_name));
    if (rc == -1) { puts("EOF. Exiting..."); return; }
    if (rc == 0) { puts("Name cannot be empty."); return; }

    Matrix *m1 = find_matrix(col, name1);
    Matrix *m2 = find_matrix(col, name2);

    if (!m1) { printf("Matrix '%s' not found.\n", name1); return; }
    if (!m2) { printf("Matrix '%s' not found.\n", name2); return; }

    // Run all three methods and compare performance
    PerformanceMetrics metrics;
    Matrix *result = run_operation_comparison(m1, m2, result_name, "Addition", &metrics);
    
    if (result) {
        if (!add_matrix(col, result)) {
            printf("Warning: Could not add result matrix '%s' to collection.\n", result_name);
            free_matrix(result);
        } else {
            printf("✓ Result matrix '%s' added to collection.\n", result_name);
        }
    }
}

static void handle_subtract_matrices(MatrixCollection *col) {
    puts("--- Subtract Two Matrices (Performance Comparison) ---");
    if (col->count < 2) {
        puts("Need at least 2 matrices in memory to perform subtraction.");
        return;
    }

    char name1[64], name2[64], result_name[64];
    
    int rc = read_line_prompt("Enter first matrix name (minuend): ", name1, sizeof(name1));
    if (rc == -1) { puts("EOF. Exiting..."); return; }
    if (rc == 0) { puts("Name cannot be empty."); return; }

    rc = read_line_prompt("Enter second matrix name (subtrahend): ", name2, sizeof(name2));
    if (rc == -1) { puts("EOF. Exiting..."); return; }
    if (rc == 0) { puts("Name cannot be empty."); return; }

    rc = read_line_prompt("Enter result matrix name: ", result_name, sizeof(result_name));
    if (rc == -1) { puts("EOF. Exiting..."); return; }
    if (rc == 0) { puts("Name cannot be empty."); return; }

    Matrix *m1 = find_matrix(col, name1);
    Matrix *m2 = find_matrix(col, name2);

    if (!m1) { printf("Matrix '%s' not found.\n", name1); return; }
    if (!m2) { printf("Matrix '%s' not found.\n", name2); return; }

    // Run all three methods and compare performance
    PerformanceMetrics metrics;
    Matrix *result = run_operation_comparison(m1, m2, result_name, "Subtraction", &metrics);
    
    if (result) {
        if (!add_matrix(col, result)) {
            printf("Warning: Could not add result matrix '%s' to collection.\n", result_name);
            free_matrix(result);
        } else {
            printf("✓ Result matrix '%s' added to collection.\n", result_name);
        }
    }
}

static void handle_multiply_matrices(MatrixCollection *col) {
    puts("--- Multiply Two Matrices (Performance Comparison) ---");
    if (col->count < 2) {
        puts("Need at least 2 matrices in memory to perform multiplication.");
        return;
    }

    char name1[64], name2[64], result_name[64];
    
    int rc = read_line_prompt("Enter first matrix name: ", name1, sizeof(name1));
    if (rc == -1) { puts("EOF. Exiting..."); return; }
    if (rc == 0) { puts("Name cannot be empty."); return; }

    rc = read_line_prompt("Enter second matrix name: ", name2, sizeof(name2));
    if (rc == -1) { puts("EOF. Exiting..."); return; }
    if (rc == 0) { puts("Name cannot be empty."); return; }

    rc = read_line_prompt("Enter result matrix name: ", result_name, sizeof(result_name));
    if (rc == -1) { puts("EOF. Exiting..."); return; }
    if (rc == 0) { puts("Name cannot be empty."); return; }

    Matrix *m1 = find_matrix(col, name1);
    Matrix *m2 = find_matrix(col, name2);

    if (!m1) { printf("Matrix '%s' not found.\n", name1); return; }
    if (!m2) { printf("Matrix '%s' not found.\n", name2); return; }

    // Run all three methods and compare performance
    PerformanceMetrics metrics;
    Matrix *result = run_operation_comparison(m1, m2, result_name, "Multiplication", &metrics);
    
    if (result) {
        if (!add_matrix(col, result)) {
            printf("Warning: Could not add result matrix '%s' to collection.\n", result_name);
            free_matrix(result);
        } else {
            printf("✓ Result matrix '%s' added to collection.\n", result_name);
        }
    }
}

/* ===== Option 13: Determinant (Gaussian elimination with partial pivoting) ===== */
static void handle_determinant(MatrixCollection *col) {
    puts("--- Determinant (Gaussian Elimination with Partial Pivoting) ---");
    puts("(Single-thread vs OpenMP vs Multiprocessing)\n");
    char name[MAX_NAME_LENGTH];
    int rc = read_line_prompt("Enter matrix name: ", name, sizeof(name));
    if (rc == -1) { puts("EOF. Exiting..."); return; }
    if (rc == 0) { puts("Name cannot be empty."); return; }

    Matrix *m = find_matrix(col, name);
    if (!m) { printf("Matrix '%s' not found.\n", name); return; }
    if (m->rows != m->cols) {
        printf("Matrix '%s' is not square (%dx%d). Determinant undefined.\n", name, m->rows, m->cols);
        return;
    }

    PerformanceMetrics metrics;
    double det = 0.0;
    if (!run_determinant_comparison(m, &metrics, &det)) {
        puts("Failed to compute determinant.");
        return;
    }

    printf("Determinant of '%s': %.10g\n", m->name, det);
}

/* ===== Option 14: Eigenvalues & Eigenvectors (QR Iteration) ===== */
static void handle_eigen(MatrixCollection *col) {
    puts("--- Eigenvalues & Eigenvectors (QR Iteration) ---");
    puts("(Single-thread vs OpenMP vs Multiprocessing)\n");
    char name[MAX_NAME_LENGTH];
    int rc = read_line_prompt("Enter matrix name: ", name, sizeof(name));
    if (rc == -1) { puts("EOF. Exiting..."); return; }
    if (rc == 0) { puts("Name cannot be empty."); return; }

    Matrix *m = find_matrix(col, name);
    if (!m) { printf("Matrix '%s' not found.\n", name); return; }
    if (m->rows != m->cols) {
        printf("Matrix '%s' is not square (%dx%d). Eigenvalues undefined.\n", name, m->rows, m->cols);
        return;
    }

    int max_iter = 500;
    double tol = 1e-10;
    
    PerformanceMetrics metrics;
    EigenResult *result = run_eigen_comparison(m, max_iter, tol, &metrics);
    
    if (!result) {
        puts("Failed to compute eigenvalues.");
        return;
    }

    printf("\n========================================\n");
    printf("EIGENVALUE & EIGENVECTOR RESULTS\n");
    printf("========================================\n");
    printf("Matrix: %s (%dx%d)\n", m->name, m->rows, m->cols);
    printf("Converged in %d iterations\n\n", result->iterations);
    
    printf("Eigenvalues (%d):\n", result->n);
    for (int i = 0; i < result->n; ++i) {
        printf("  λ[%d] = %.10g\n", i, result->eigenvalues[i]);
    }
    
    // Display full eigenvector matrix (columns are eigenvectors)
    if (result->eigenvectors) {
        printf("\nEigenvectors matrix [%dx%d] (columns are eigenvectors):\n", result->n, result->n);
        for (int r = 0; r < result->n; ++r) {
            printf("  [");
            for (int c = 0; c < result->n; ++c) {
                printf(" % .6f", result->eigenvectors->data[r][c]);
            }
            printf(" ]\n");
        }
    }
    
    printf("========================================\n\n");
    
    free_eigen_result(result);
}


static void handle_action(int choice, MatrixCollection *col) {
    switch (choice) {
        case 1:  handle_enter_matrix(col); break;
        case 2:  handle_display_matrix(col); break;
        case 3:  handle_delete_matrix(col); break;
        case 4:  handle_modify_matrix(col); break;
        case 5:  handle_read_from_file(col); break;
        case 6:  handle_read_from_folder(col); break;
        case 7:  handle_save_to_file(col); break;
        case 8:  handle_save_all(col); break;
        case 9:  display_all_matrices(col); break;
        case 10: handle_add_matrices(col); break;
        case 11: handle_subtract_matrices(col); break;
        case 12: handle_multiply_matrices(col); break;
        case 13: handle_determinant(col); break;
        case 14: handle_eigen(col); break;
        default: puts("→ Unknown action"); break;
    }
}

int main(void) {
    MatrixCollection *collection = create_collection();
    if (!collection) { fprintf(stderr, "Failed to initialize collection.\n"); return 1; }

    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = on_sigint;
    sigaction(SIGINT, &sa, NULL);

    int running = 1;
    while (running && !g_interrupted) {
        clear_screen();
        print_header();
        print_menu();

        int choice = 0;
        int rc = read_int_choice("Enter your choice: ", &choice);
        if (rc == -1) {
            puts("\nEnd of input detected. Exiting...");
            break;
        } else if (rc == 0) {
            puts("Invalid input. Please enter a number between 1 and 15.");
            press_enter_to_continue();
            if (feof(stdin)) break;
            continue;
        }

        if (choice == 15) {
            puts("\nExiting program...");
            break;
        }

        if (choice < 1 || choice > 15) {
            puts("Invalid choice. Please select 1-15.");
            press_enter_to_continue();
            if (feof(stdin)) break;
            continue;
        }

        puts("");
        handle_action(choice, collection);
        press_enter_to_continue();
        if (feof(stdin)) break;
    }

    puts("\nGoodbye.");
    free_collection(collection);
    return 0;
}

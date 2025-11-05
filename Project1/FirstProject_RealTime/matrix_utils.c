#include "matrix_types.h"
#include <ctype.h>

/* 
 * Implementation of shared matrix functions
 * Used by both menu_demo.c and matrix_file_ops.c
 */

Matrix *create_matrix(const char *name, int rows, int cols) {
    if (rows <= 0 || cols <= 0) return NULL;
    Matrix *m = (Matrix*)calloc(1, sizeof(Matrix));
    if (!m) return NULL;
    if (name) {
        strncpy(m->name, name, MAX_NAME_LENGTH - 1);
        m->name[MAX_NAME_LENGTH - 1] = '\0';
    }
    m->rows = rows;
    m->cols = cols;
    m->data = (double**)calloc(rows, sizeof(double*));
    if (!m->data) { free(m); return NULL; }
    for (int i = 0; i < rows; i++) {
        m->data[i] = (double*)calloc(cols, sizeof(double));
        if (!m->data[i]) {
            for (int k = 0; k < i; k++) free(m->data[k]);
            free(m->data); free(m); return NULL;
        }
    }
    return m;
}

void free_matrix(Matrix *m) {
    if (!m) return;
    if (m->data) {
        for (int i = 0; i < m->rows; i++) free(m->data[i]);
        free(m->data);
    }
    free(m);
}

MatrixCollection *create_collection(void) {
    MatrixCollection *c = (MatrixCollection*)calloc(1, sizeof(MatrixCollection));
    if (!c) return NULL;
    c->capacity = 8;
    c->items = (Matrix**)calloc(c->capacity, sizeof(Matrix*));
    if (!c->items) { free(c); return NULL; }
    return c;
}

void free_collection(MatrixCollection *c) {
    if (!c) return;
    for (int i = 0; i < c->count; i++) free_matrix(c->items[i]);
    free(c->items);
    free(c);
}

Matrix *find_matrix(MatrixCollection *c, const char *name) {
    if (!c || !name) return NULL;
    for (int i = 0; i < c->count; i++) {
        if (strcmp(c->items[i]->name, name) == 0) return c->items[i];
    }
    return NULL;
}

int add_matrix(MatrixCollection *c, Matrix *m) {
    if (!c || !m) return 0;
    if (find_matrix(c, m->name)) return 0; // duplicate
    if (c->count >= c->capacity) {
        int newcap = c->capacity * 2;
        Matrix **tmp = (Matrix**)realloc(c->items, newcap * sizeof(Matrix*));
        if (!tmp) return 0;
        c->items = tmp;
        c->capacity = newcap;
    }
    c->items[c->count++] = m;
    return 1;
}

int remove_matrix(MatrixCollection *c, const char *name) {
    if (!c || !name) return 0;
    for (int i = 0; i < c->count; i++) {
        if (strcmp(c->items[i]->name, name) == 0) {
            free_matrix(c->items[i]);
            for (int j = i + 1; j < c->count; j++) c->items[j-1] = c->items[j];
            c->count--;
            return 1;
        }
    }
    return 0;
}

void display_matrix(const Matrix *m) {
    if (!m) { puts("Matrix not found."); return; }
    printf("\n========================================\n");
    printf("Matrix: %s\n", m->name);
    printf("Dimensions: %d x %d\n", m->rows, m->cols);
    printf("========================================\n");
    for (int i = 0; i < m->rows; i++) {
        for (int j = 0; j < m->cols; j++) {
            printf("%10.4f ", m->data[i][j]);
        }
        printf("\n");
    }
    printf("========================================\n\n");
}

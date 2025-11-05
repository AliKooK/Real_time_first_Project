#include "matrix_types.h"
#include <dirent.h>
#include <sys/stat.h>
#include <ctype.h>

/*
 * File I/O and bulk operations for matrix collection (options 5-9)
 * Synchronized with menu_demo.c via matrix_types.h
 */

/* ===== Helper: check if file/directory exists ===== */
static int file_exists(const char *path) {
    struct stat st;
    return (stat(path, &st) == 0 && S_ISREG(st.st_mode));
}

static int dir_exists(const char *path) {
    struct stat st;
    return (stat(path, &st) == 0 && S_ISDIR(st.st_mode));
}

/* ===== Helper: trim whitespace ===== */
static void trim_newline(char *s) {
    if (!s) return;
    size_t n = strlen(s);
    while (n > 0 && (s[n-1] == '\n' || s[n-1] == '\r' || isspace((unsigned char)s[n-1]))) {
        s[--n] = '\0';
    }
}

/* ===== Option 5: Read matrix from file ===== */
Matrix *read_matrix_from_file(const char *filepath) {
    if (!filepath || !file_exists(filepath)) {
        fprintf(stderr, "File '%s' not found or invalid.\n", filepath);
        return NULL;
    }

    FILE *f = fopen(filepath, "r");
    if (!f) {
        perror("fopen");
        return NULL;
    }

    char name[MAX_NAME_LENGTH];
    int rows, cols;

    // Read: name, rows, cols
    if (fscanf(f, "%63s", name) != 1) {
        fprintf(stderr, "Failed to read matrix name from %s\n", filepath);
        fclose(f);
        return NULL;
    }

    if (fscanf(f, "%d %d", &rows, &cols) != 2 || rows <= 0 || cols <= 0) {
        fprintf(stderr, "Invalid dimensions in %s\n", filepath);
        fclose(f);
        return NULL;
    }

    Matrix *m = create_matrix(name, rows, cols);
    if (!m) {
        fprintf(stderr, "Failed to allocate matrix.\n");
        fclose(f);
        return NULL;
    }

    // Read matrix data
    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols; j++) {
            if (fscanf(f, "%lf", &m->data[i][j]) != 1) {
                fprintf(stderr, "Failed to read element [%d][%d] from %s\n", i, j, filepath);
                free_matrix(m);
                fclose(f);
                return NULL;
            }
        }
    }

    fclose(f);
    printf("Successfully loaded matrix '%s' (%dx%d) from %s\n", name, rows, cols, filepath);
    return m;
}

/* ===== Option 6: Read all matrices from a folder ===== */
int read_matrices_from_folder(const char *folder, MatrixCollection *col) {
    if (!folder || !col) return 0;
    if (!dir_exists(folder)) {
        fprintf(stderr, "Directory '%s' not found.\n", folder);
        return 0;
    }

    DIR *dir = opendir(folder);
    if (!dir) {
        perror("opendir");
        return 0;
    }

    int loaded = 0;
    struct dirent *ent;
    while ((ent = readdir(dir)) != NULL) {
        // Skip . and ..
        if (strcmp(ent->d_name, ".") == 0 || strcmp(ent->d_name, "..") == 0) continue;

        // Only process .txt files
        size_t len = strlen(ent->d_name);
        if (len < 5 || strcmp(ent->d_name + len - 4, ".txt") != 0) continue;

        // Build full path
        char path[512];
        snprintf(path, sizeof(path), "%s/%s", folder, ent->d_name);

        Matrix *m = read_matrix_from_file(path);
        if (m) {
            if (add_matrix(col, m)) {
                loaded++;
            } else {
                fprintf(stderr, "Matrix '%s' already exists or failed to add.\n", m->name);
                free_matrix(m);
            }
        }
    }

    closedir(dir);
    printf("Loaded %d matri%s from '%s'.\n", loaded, loaded == 1 ? "x" : "ces", folder);
    return loaded;
}

/* ===== Option 7: Save matrix to file ===== */
int write_matrix_to_file(const Matrix *m, const char *filepath) {
    if (!m || !filepath) return 0;

    FILE *f = fopen(filepath, "w");
    if (!f) {
        perror("fopen");
        return 0;
    }

    // Write: name, dimensions, data
    fprintf(f, "%s\n", m->name);
    fprintf(f, "%d %d\n", m->rows, m->cols);

    for (int i = 0; i < m->rows; i++) {
        for (int j = 0; j < m->cols; j++) {
            fprintf(f, "%.10f", m->data[i][j]);
            if (j < m->cols - 1) fprintf(f, " ");
        }
        fprintf(f, "\n");
    }

    fclose(f);
    printf("Matrix '%s' saved to %s\n", m->name, filepath);
    return 1;
}

/* ===== Option 8: Save all matrices to folder ===== */
int save_all_matrices_to_folder(const MatrixCollection *col, const char *folder) {
    if (!col || !folder) return 0;

    // Create directory if it doesn't exist
    if (!dir_exists(folder)) {
        if (mkdir(folder, 0755) != 0) {
            perror("mkdir");
            return 0;
        }
        printf("Created directory '%s'.\n", folder);
    }

    int saved = 0;
    for (int i = 0; i < col->count; i++) {
        char path[512];
        snprintf(path, sizeof(path), "%s/%s.txt", folder, col->items[i]->name);
        if (write_matrix_to_file(col->items[i], path)) {
            saved++;
        }
    }

    printf("Saved %d matri%s to '%s'.\n", saved, saved == 1 ? "x" : "ces", folder);
    return saved;
}

/* ===== Option 9: Display all matrices (summary) ===== */
void display_all_matrices(const MatrixCollection *c) {
    if (!c) {
        puts("Collection is NULL.");
        return;
    }

    if (c->count == 0) {
        puts("\nNo matrices in memory.\n");
        return;
    }

    printf("\n========================================\n");
    printf("MATRICES IN MEMORY (%d total)\n", c->count);
    printf("========================================\n");

    for (int i = 0; i < c->count; i++) {
        printf("%d. %s - %dx%d\n",
               i + 1,
               c->items[i]->name,
               c->items[i]->rows,
               c->items[i]->cols);
    }

    printf("========================================\n\n");
}

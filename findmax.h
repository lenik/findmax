#ifndef FINDMAX_H
#define FINDMAX_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <time.h>
#include <pwd.h>
#include <grp.h>
#include <errno.h>
#include <locale.h>
#include <ctype.h>

#define MAX_PATH_LEN 4096
#define MAX_FORMAT_LEN 1024
#define DEFAULT_NUM_FILES 1

typedef enum {
    SORT_MTIME,
    SORT_ATIME,
    SORT_CTIME,
    SORT_BTIME,
    SORT_SIZE,
    SORT_NAME
} sort_type_t;

typedef enum {
    FILTER_ALL,
    FILTER_FILE_ONLY,
    FILTER_DIR_ONLY
} filter_type_t;

typedef struct {
    char path[MAX_PATH_LEN];
    struct stat st;
    time_t sort_time;
    off_t sort_size;
} file_entry_t;

typedef struct {
    file_entry_t *entries;
    size_t count;
    size_t capacity;
} file_list_t;

typedef struct {
    int recursive;
    int reverse;
    int dereference;
    int max_depth;
    sort_type_t sort_type;
    filter_type_t filter_type;
    char format[MAX_FORMAT_LEN];
    int num_files;
    int verbose;
    int quiet;
} options_t;

// Function prototypes
void print_usage(void);
void print_version(void);
int parse_arguments(int argc, char *argv[], options_t *opts, char ***paths, int *path_count);
int traverse_directory(const char *path, const options_t *opts, file_list_t *files);
int traverse_directory_depth(const char *path, const options_t *opts, file_list_t *files, int current_depth);
int compare_files(const void *a, const void *b, const options_t *opts);
void sort_files(file_list_t *files, const options_t *opts);
void print_file_entry(const file_entry_t *entry, const options_t *opts);
void format_output(const file_entry_t *entry, const char *format, char *output, size_t output_size);
file_list_t *create_file_list(void);
void free_file_list(file_list_t *files);
int add_file_entry(file_list_t *files, const char *path, const struct stat *st, const options_t *opts);
int should_include_file(const struct stat *st, const options_t *opts);

#endif

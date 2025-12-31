#include "findmax.h"

file_list_t *create_file_list(void) {
    file_list_t *files = malloc(sizeof(file_list_t));
    if (!files) return NULL;
    
    files->entries = malloc(sizeof(file_entry_t) * 16);
    if (!files->entries) {
        free(files);
        return NULL;
    }
    
    files->count = 0;
    files->capacity = 16;
    return files;
}

void free_file_list(file_list_t *files) {
    if (files) {
        free(files->entries);
        free(files);
    }
}

int add_file_entry(file_list_t *files, const char *path, const struct stat *st, const options_t *opts) {
    // Check if we need to resize the array
    if (files->count >= files->capacity) {
        size_t new_capacity = files->capacity * 2;
        file_entry_t *new_entries = realloc(files->entries, sizeof(file_entry_t) * new_capacity);
        if (!new_entries) {
            return -1;
        }
        files->entries = new_entries;
        files->capacity = new_capacity;
    }
    
    file_entry_t *entry = &files->entries[files->count];
    
    // Copy path
    strncpy(entry->path, path, MAX_PATH_LEN - 1);
    entry->path[MAX_PATH_LEN - 1] = '\0';
    
    // Copy stat info
    entry->st = *st;
    
    // Set sort criteria based on sort type
    switch (opts->sort_type) {
        case SORT_ATIME:
            entry->sort_time = st->st_atime;
            break;
        case SORT_CTIME:
            entry->sort_time = st->st_ctime;
            break;
        case SORT_MTIME:
            entry->sort_time = st->st_mtime;
            break;
        case SORT_BTIME:
#ifdef __APPLE__
            entry->sort_time = st->st_birthtime;
#else
            entry->sort_time = st->st_ctime; // Fallback to ctime on Linux
#endif
            break;
        case SORT_SIZE:
            entry->sort_size = st->st_size;
            break;
        case SORT_NAME:
            // For name sorting, we'll use the path directly in comparison
            break;
    }
    
    files->count++;
    return 0;
}

int should_include_file(const struct stat *st, const options_t *opts) {
    switch (opts->filter_type) {
        case FILTER_FILE_ONLY:
            return S_ISREG(st->st_mode);
        case FILTER_DIR_ONLY:
            return S_ISDIR(st->st_mode);
        case FILTER_ALL:
        default:
            return 1;
    }
}

int traverse_directory(const char *path, const options_t *opts, file_list_t *files) {
    return traverse_directory_depth(path, opts, files, 0);
}

int traverse_directory_depth(const char *path, const options_t *opts, file_list_t *files, int current_depth) {
    struct stat st;
    
    // Check depth limit
    if (opts->max_depth >= 0 && current_depth > opts->max_depth) {
        return 0;
    }
    
    // Get file/directory stats
    int stat_result;
    if (opts->dereference) {
        stat_result = stat(path, &st);
    } else {
        stat_result = lstat(path, &st);
    }
    
    if (stat_result != 0) {
        if (!opts->quiet) {
            perror(path);
        }
        return -1;
    }
    
    // Add current file/directory if it matches filter
    if (should_include_file(&st, opts)) {
        if (add_file_entry(files, path, &st, opts) != 0) {
            if (!opts->quiet) {
                fprintf(stderr, "findmax: memory allocation failed\n");
            }
            return -1;
        }
    }
    
    // If it's a directory and recursive mode is enabled, traverse it
    if (S_ISDIR(st.st_mode) && opts->recursive) {
        DIR *dir = opendir(path);
        if (!dir) {
            if (!opts->quiet) {
                perror(path);
            }
            return -1;
        }
        
        struct dirent *entry;
        while ((entry = readdir(dir)) != NULL) {
            // Skip . and ..
            if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
                continue;
            }
            
            // Build full path
            char full_path[MAX_PATH_LEN];
            int ret = snprintf(full_path, sizeof(full_path), "%s/%s", path, entry->d_name);
            if (ret >= sizeof(full_path)) {
                if (!opts->quiet) {
                    fprintf(stderr, "findmax: path too long: %s/%s\n", path, entry->d_name);
                }
                continue;
            }
            
            // Recursively traverse with incremented depth
            traverse_directory_depth(full_path, opts, files, current_depth + 1);
        }
        
        closedir(dir);
    }
    
    return 0;
}

static int compare_time(const void *a, const void *b, int reverse) {
    const file_entry_t *fa = (const file_entry_t *)a;
    const file_entry_t *fb = (const file_entry_t *)b;
    
    time_t diff = fa->sort_time - fb->sort_time;
    if (diff == 0) return 0;
    
    int result = (diff > 0) ? 1 : -1;
    return reverse ? -result : result;
}

static int compare_size(const void *a, const void *b, int reverse) {
    const file_entry_t *fa = (const file_entry_t *)a;
    const file_entry_t *fb = (const file_entry_t *)b;
    
    off_t diff = fa->sort_size - fb->sort_size;
    if (diff == 0) return 0;
    
    int result = (diff > 0) ? 1 : -1;
    return reverse ? -result : result;
}

static int compare_name(const void *a, const void *b, int reverse) {
    const file_entry_t *fa = (const file_entry_t *)a;
    const file_entry_t *fb = (const file_entry_t *)b;
    
    // Extract just the filename from the path for comparison
    const char *name_a = strrchr(fa->path, '/');
    const char *name_b = strrchr(fb->path, '/');
    
    name_a = name_a ? name_a + 1 : fa->path;
    name_b = name_b ? name_b + 1 : fb->path;
    
    int result = strcoll(name_a, name_b);
    return reverse ? -result : result;
}

static int file_compare_wrapper(const void *a, const void *b) {
    // This is a global variable hack - not ideal but needed for qsort
    extern const options_t *g_sort_opts;
    
    switch (g_sort_opts->sort_type) {
        case SORT_ATIME:
        case SORT_CTIME:
        case SORT_MTIME:
        case SORT_BTIME:
            return compare_time(a, b, g_sort_opts->reverse);
        case SORT_SIZE:
            return compare_size(a, b, g_sort_opts->reverse);
        case SORT_NAME:
            return compare_name(a, b, g_sort_opts->reverse);
        default:
            return 0;
    }
}

// Global variable for qsort (not ideal but necessary)
const options_t *g_sort_opts = NULL;

void sort_files(file_list_t *files, const options_t *opts) {
    if (files->count <= 1) return;
    
    g_sort_opts = opts;
    qsort(files->entries, files->count, sizeof(file_entry_t), file_compare_wrapper);
}

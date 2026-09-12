#include "findmax.h"
#include <stdbool.h>

struct min_heap {
    file_entry_t *entries;
    size_t size;
    size_t capacity;
    const options_t *opts;
};

static int heap_compare(const file_entry_t *a, const file_entry_t *b, const options_t *opts) {
    int result = 0;
    
    switch (opts->sort_type) {
        case SORT_ATIME:
        case SORT_CTIME:
        case SORT_MTIME:
        case SORT_BTIME:
            if (a->sort_time < b->sort_time) result = -1;
            else if (a->sort_time > b->sort_time) result = 1;
            else result = 0;
            break;
        case SORT_SIZE:
            if (a->sort_size < b->sort_size) result = -1;
            else if (a->sort_size > b->sort_size) result = 1;
            else result = 0;
            break;
        case SORT_NAME:
            {
                const char *name_a = strrchr(a->path, '/');
                const char *name_b = strrchr(b->path, '/');
                name_a = name_a ? name_a + 1 : a->path;
                name_b = name_b ? name_b + 1 : b->path;
                result = strcoll(name_a, name_b);
            }
            break;
    }
    
    // For heap ordering: when reverse=1 (max first), we want to keep the N largest values
    // The heap root should be the smallest of those N values
    // So if a > b, a should be below b in the heap (return positive)
    // This means we DON'T flip for heap operations - we use natural comparison
    // The reverse flag only affects final output sorting, not heap maintenance
    return result;
}

static void heap_swap(file_entry_t *a, file_entry_t *b) {
    file_entry_t temp = *a;
    *a = *b;
    *b = temp;
}

static void heap_sift_up(min_heap_t *heap, size_t index) {
    while (index > 0) {
        size_t parent = (index - 1) / 2;
        if (heap_compare(&heap->entries[index], &heap->entries[parent], heap->opts) >= 0) {
            break;
        }
        heap_swap(&heap->entries[index], &heap->entries[parent]);
        index = parent;
    }
}

static void heap_sift_down(min_heap_t *heap, size_t index) {
    while (true) {
        size_t left = 2 * index + 1;
        size_t right = 2 * index + 2;
        size_t smallest = index;
        
        if (left < heap->size && 
            heap_compare(&heap->entries[left], &heap->entries[smallest], heap->opts) < 0) {
            smallest = left;
        }
        
        if (right < heap->size && 
            heap_compare(&heap->entries[right], &heap->entries[smallest], heap->opts) < 0) {
            smallest = right;
        }
        
        if (smallest == index) {
            break;
        }
        
        heap_swap(&heap->entries[index], &heap->entries[smallest]);
        index = smallest;
    }
}

min_heap_t *create_min_heap(size_t capacity, const options_t *opts) {
    min_heap_t *heap = malloc(sizeof(min_heap_t));
    if (!heap) return NULL;
    
    heap->entries = malloc(sizeof(file_entry_t) * capacity);
    if (!heap->entries) {
        free(heap);
        return NULL;
    }
    
    heap->size = 0;
    heap->capacity = capacity;
    heap->opts = opts;
    return heap;
}

void free_min_heap(min_heap_t *heap) {
    if (heap) {
        free(heap->entries);
        free(heap);
    }
}

size_t get_heap_size(min_heap_t *heap) {
    return heap ? heap->size : 0;
}

file_entry_t *get_heap_entries(min_heap_t *heap) {
    return heap ? heap->entries : NULL;
}

static int heap_insert(min_heap_t *heap, const file_entry_t *entry) {
    if (heap->size < heap->capacity) {
        // Heap not full, just insert
        heap->entries[heap->size] = *entry;
        heap_sift_up(heap, heap->size);
        heap->size++;
        return 0;
    } else {
        // Heap is full, check if new entry should replace the minimum
        // When reverse=1 (max first), we keep N largest values, root is smallest of them
        // When reverse=0 (min first), we keep N smallest values, root is largest of them
        // So we compare: if new entry is "better" than root, replace it
        int cmp = heap_compare(entry, &heap->entries[0], heap->opts);
        if (heap->opts->reverse) {
            // Max first: replace root if new entry is larger (positive comparison)
            if (cmp > 0) {
                heap->entries[0] = *entry;
                heap_sift_down(heap, 0);
            }
        } else {
            // Min first: replace root if new entry is smaller (negative comparison)
            if (cmp < 0) {
                heap->entries[0] = *entry;
                heap_sift_down(heap, 0);
            }
        }
        return 0;
    }
}

// Optimized file traversal using heap for O(1) performance
int traverse_directory_optimized(const char *path, const options_t *opts, min_heap_t *heap, int current_depth) {
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
        file_entry_t entry;
        strncpy(entry.path, path, MAX_PATH_LEN - 1);
        entry.path[MAX_PATH_LEN - 1] = '\0';
        entry.st = st;
        
        // Set sort criteria
        switch (opts->sort_type) {
            case SORT_ATIME:
                entry.sort_time = st.st_atime;
                break;
            case SORT_CTIME:
                entry.sort_time = st.st_ctime;
                break;
            case SORT_MTIME:
                entry.sort_time = st.st_mtime;
                break;
            case SORT_BTIME:
#ifdef __APPLE__
                entry.sort_time = st.st_birthtime;
#else
                entry.sort_time = st.st_ctime;
#endif
                break;
            case SORT_SIZE:
                entry.sort_size = st.st_size;
                break;
            case SORT_NAME:
                break;
        }
        
        heap_insert(heap, &entry);
    }
    
    // Recursive traversal for directories
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
            
            char full_path[MAX_PATH_LEN];
            int ret = snprintf(full_path, sizeof(full_path), "%s/%s", path, entry->d_name);
            if (ret < 0 || (size_t)ret >= sizeof(full_path)) {
                if (!opts->quiet) {
                    fprintf(stderr, "findmax: path too long: %s/%s\n", path, entry->d_name);
                }
                continue;
            }
            
            traverse_directory_optimized(full_path, opts, heap, current_depth + 1);
        }
        
        closedir(dir);
    }
    
    return 0;
}

// Modified main function to use optimized heap-based approach
int main_optimized(int argc, char *argv[]) {
    options_t opts = {0};
    char **paths = NULL;
    int path_count = 0;
    
    // Set default options
    opts.sort_type = SORT_MTIME;
    opts.filter_type = FILTER_ALL;
    strcpy(opts.format, "%n");
    opts.num_files = DEFAULT_NUM_FILES;
    
    setlocale(LC_ALL, "");
    
    if (parse_arguments(argc, argv, &opts, &paths, &path_count) != 0) {
        return 1;
    }
    
    if (path_count == 0) {
        paths = malloc(sizeof(char*));
        paths[0] = ".";
        path_count = 1;
    }
    
    // Create min-heap with capacity equal to num_files for O(1) performance
    min_heap_t *heap = create_min_heap(opts.num_files, &opts);
    if (!heap) {
        fprintf(stderr, "findmax: memory allocation failed\n");
        return 1;
    }
    
    // Traverse all paths using optimized heap approach
    for (int i = 0; i < path_count; i++) {
        if (traverse_directory_optimized(paths[i], &opts, heap, 0) != 0) {
            if (!opts.quiet) {
                fprintf(stderr, "findmax: error processing '%s'\n", paths[i]);
            }
        }
    }
    
    // Extract results from heap and sort them properly for output
    file_list_t *results = create_file_list();
    if (!results) {
        fprintf(stderr, "findmax: memory allocation failed\n");
        free_min_heap(heap);
        return 1;
    }
    
    // Copy heap contents to results
    for (size_t i = 0; i < heap->size; i++) {
        if (add_file_entry(results, heap->entries[i].path, &heap->entries[i].st, &opts) != 0) {
            fprintf(stderr, "findmax: memory allocation failed\n");
            break;
        }
    }
    
    // Sort results for proper output order
    sort_files(results, &opts);
    
    // Print results
    for (size_t i = 0; i < results->count; i++) {
        print_file_entry(&results->entries[i], &opts);
    }
    
    // Cleanup
    free_file_list(results);
    free_min_heap(heap);
    if (paths && path_count > 0 && strcmp(paths[0], ".") != 0) {
        free(paths);
    }
    
    return 0;
}

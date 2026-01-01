#include "findmax.h"

int main(int argc, char *argv[]) {
    options_t opts = {0};
    char **paths = NULL;
    int path_count = 0;
    
    // Set default options
    opts.sort_type = SORT_MTIME;
    opts.filter_type = FILTER_ALL;
    strcpy(opts.format, "%n");
    opts.num_files = DEFAULT_NUM_FILES;
    opts.max_depth = -1; // No limit by default
    opts.reverse = 1; // Default: max first (reverse normal order)
    
    // Set locale for proper string comparison
    setlocale(LC_ALL, "");
    
    // Parse command line arguments
    if (parse_arguments(argc, argv, &opts, &paths, &path_count) != 0) {
        return 1;
    }
    
    // If no paths specified, use current directory
    int allocated_paths = 0;
    if (path_count == 0) {
        paths = malloc(sizeof(char*));
        paths[0] = ".";
        path_count = 1;
        allocated_paths = 1;
    }
    
    // Optimization: if num_files == 1, use direct comparison instead of heap
    if (opts.num_files == 1) {
        file_entry_t best = {0};
        best.path[0] = '\0';
        
        // Traverse all specified paths using direct comparison
        for (int i = 0; i < path_count; i++) {
            if (traverse_directory_single(paths[i], &opts, &best, 0) != 0) {
                if (!opts.quiet) {
                    fprintf(stderr, "findmax: error processing '%s'\n", paths[i]);
                }
            }
        }
        
        // Print result if found
        if (best.path[0] != '\0') {
            print_file_entry(&best, &opts);
        }
        
        if (allocated_paths) {
            free(paths);
        }
        return 0;
    }
    
    // Use heap-based optimization: maintain only top N files during traversal
    // This avoids collecting all files and sorting them, which is much faster for large directories
    min_heap_t *heap = create_min_heap(opts.num_files, &opts);
    if (!heap) {
        fprintf(stderr, "findmax: memory allocation failed\n");
        if (allocated_paths) {
            free(paths);
        }
        return 1;
    }
    
    // Traverse all specified paths using optimized heap approach
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
        if (allocated_paths) {
            free(paths);
        }
        return 1;
    }
    
    // Copy heap contents to results
    file_entry_t *heap_entries = get_heap_entries(heap);
    size_t heap_size = get_heap_size(heap);
    
    for (size_t i = 0; i < heap_size; i++) {
        if (add_file_entry(results, heap_entries[i].path, &heap_entries[i].st, &opts) != 0) {
            fprintf(stderr, "findmax: memory allocation failed\n");
            break;
        }
    }
    
    // Sort results for proper output order (heap maintains min at top, we need final sort)
    sort_files(results, &opts);
    
    // Print results (top N files)
    size_t print_count = (results->count < (size_t)opts.num_files) ? results->count : (size_t)opts.num_files;
    for (size_t i = 0; i < print_count; i++) {
        print_file_entry(&results->entries[i], &opts);
    }
    
    // Cleanup
    free_file_list(results);
    free_min_heap(heap);
    if (allocated_paths) {
        free(paths);
    }
    
    return 0;
}

void print_usage(void) {
    printf("Usage: findmax [OPTIONS] FILE...\n");
    printf("Find files with maximum values for specified criteria.\n");
    printf("Default behavior: show largest/newest files first (max-first).\n\n");
    printf("Options:\n");
    printf("  -R, --recursive     recursive into directories\n");
    printf("  -r, --reverse       normal order (smallest/oldest first)\n");
    printf("  -u                  access time\n");
    printf("  -c                  metadata change time\n");
    printf("  -t                  file time (modification time)\n");
    printf("      --time=WORD     select timestamp (atime/access/use, ctime/status,\n");
    printf("                      mtime/modification, birth/creation)\n");
    printf("  -S                  file size\n");
    printf("  -n, --name          file name, order in current locale setting\n");
    printf("  -f, --file-only     print plain file only\n");
    printf("  -d, --dir-only      print directory only\n");
    printf("  -F, --format FMT    output format\n");
    printf("  -NUM                show top NUM files, default 1\n");
    printf("  -v, --verbose       verbose output\n");
    printf("  -q, --quiet         quiet mode\n");
    printf("  -L, --dereference  follow symbolic links\n");
    printf("      --maxdepth NUM  limit directory traversal depth\n");
    printf("      --version       show version information\n");
    printf("      --help          show this help\n");
}

void print_version(void) {
    printf("findmax 1.0.0\n");
    printf("Copyright (C) 2026 Lenik <findmax@bodz.net>\n");
    printf("License: GPL-3.0-or-later\n");
    printf("Fast file finding utility optimized for O(1) queries.\n");
}

int parse_arguments(int argc, char *argv[], options_t *opts, char ***paths, int *path_count) {
    int opt;
    int option_index = 0;
    
    // Handle -NUM arguments first
    for (int i = 1; i < argc; i++) {
        if (argv[i][0] == '-' && isdigit(argv[i][1])) {
            char *endptr;
            long num = strtol(argv[i] + 1, &endptr, 10);
            if (*endptr == '\0' && num > 0) {
                opts->num_files = (int)num;
                // Remove this argument from argv
                for (int j = i; j < argc - 1; j++) {
                    argv[j] = argv[j + 1];
                }
                argc--;
                i--; // Check the same position again
            }
        }
    }
    
    static struct option long_options[] = {
        {"recursive", no_argument, 0, 'R'},
        {"reverse", no_argument, 0, 'r'},
        {"name", no_argument, 0, 'n'},
        {"file-only", no_argument, 0, 'f'},
        {"dir-only", no_argument, 0, 'd'},
        {"format", required_argument, 0, 'F'},
        {"verbose", no_argument, 0, 'v'},
        {"quiet", no_argument, 0, 'q'},
        {"dereference", no_argument, 0, 'L'},
        {"time", required_argument, 0, 1000},
        {"maxdepth", required_argument, 0, 1001},
        {"version", no_argument, 0, 1002},
        {"help", no_argument, 0, 1003},
        {0, 0, 0, 0}
    };
    
    while ((opt = getopt_long(argc, argv, "RructnSfdF:vqL", long_options, &option_index)) != -1) {
        switch (opt) {
            case 'R':
                opts->recursive = 1;
                break;
            case 'r':
                opts->reverse = 0;  // --reverse means normal order (min first)
                break;
            case 'u':
                opts->sort_type = SORT_ATIME;
                break;
            case 'c':
                opts->sort_type = SORT_CTIME;
                break;
            case 't':
                opts->sort_type = SORT_MTIME;
                break;
            case 'S':
                opts->sort_type = SORT_SIZE;
                break;
            case 'n':
                opts->sort_type = SORT_NAME;
                break;
            case 'f':
                opts->filter_type = FILTER_FILE_ONLY;
                break;
            case 'd':
                opts->filter_type = FILTER_DIR_ONLY;
                break;
            case 'F':
                strncpy(opts->format, optarg, MAX_FORMAT_LEN - 1);
                opts->format[MAX_FORMAT_LEN - 1] = '\0';
                break;
            case 'v':
                opts->verbose = 1;
                break;
            case 'q':
                opts->quiet = 1;
                break;
            case 'L':
                opts->dereference = 1;
                break;
            case 1000: // --time
                if (strcmp(optarg, "atime") == 0 || strcmp(optarg, "access") == 0 || strcmp(optarg, "use") == 0) {
                    opts->sort_type = SORT_ATIME;
                } else if (strcmp(optarg, "ctime") == 0 || strcmp(optarg, "status") == 0) {
                    opts->sort_type = SORT_CTIME;
                } else if (strcmp(optarg, "mtime") == 0 || strcmp(optarg, "modification") == 0) {
                    opts->sort_type = SORT_MTIME;
                } else if (strcmp(optarg, "birth") == 0 || strcmp(optarg, "creation") == 0) {
                    opts->sort_type = SORT_BTIME;
                } else {
                    fprintf(stderr, "findmax: invalid time type '%s'\n", optarg);
                    return 1;
                }
                break;
            case 1001: // --maxdepth
                {
                    char *endptr;
                    long depth = strtol(optarg, &endptr, 10);
                    if (*endptr != '\0' || depth < 0) {
                        fprintf(stderr, "findmax: invalid maxdepth '%s'\n", optarg);
                        return 1;
                    }
                    opts->max_depth = (int)depth;
                }
                break;
            case 1002: // --version
                print_version();
                exit(0);
            case 1003: // --help
                print_usage();
                exit(0);
            case '?':
            default:
                print_usage();
                return 1;
        }
    }
    
    // Collect remaining arguments as paths
    *path_count = argc - optind;
    if (*path_count > 0) {
        *paths = &argv[optind];
    }
    
    return 0;
}

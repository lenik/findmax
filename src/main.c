#include "findmax.h"
#include "config.h"

#include <bas/locale/i18n.h>
#include <bas/log/deflog.h>
#include <bas/proc/env.h>

int main(int argc, char *argv[]) {
    const char *exe = self_exe();
    (void)exe;
    init_i18n(LOCALEDIR);

    options_t opts = {0};
    char **paths = NULL;
    int path_count = 0;

    opts.sort_type = SORT_MTIME;
    opts.filter_type = FILTER_ALL;
    strcpy(opts.format, "%n");
    opts.num_files = DEFAULT_NUM_FILES;
    opts.max_depth = -1;
    opts.reverse = 1;

    setlocale(LC_ALL, "");

    if (parse_arguments(argc, argv, &opts, &paths, &path_count) != 0) {
        return 1;
    }

    int allocated_paths = 0;
    if (path_count == 0) {
        paths = malloc(sizeof(char *));
        paths[0] = ".";
        path_count = 1;
        allocated_paths = 1;
    }

    if (opts.verbose) {
        loginfo_fmt("%s: searching %d path(s), top %d", exe, path_count, opts.num_files);
    }

    if (opts.num_files == 1) {
        file_entry_t best = {0};
        best.path[0] = '\0';

        for (int i = 0; i < path_count; i++) {
            if (traverse_directory_single(paths[i], &opts, &best, 0) != 0) {
                if (!opts.quiet) {
                    fprintf(stderr, _("findmax: error processing '%s'\n"), paths[i]);
                }
            }
        }

        if (best.path[0] != '\0') {
            print_file_entry(&best, &opts);
        }

        if (allocated_paths) {
            free(paths);
        }
        return 0;
    }

    min_heap_t *heap = create_min_heap(opts.num_files, &opts);
    if (!heap) {
        fprintf(stderr, _("findmax: memory allocation failed\n"));
        if (allocated_paths) {
            free(paths);
        }
        return 1;
    }

    for (int i = 0; i < path_count; i++) {
        if (traverse_directory_optimized(paths[i], &opts, heap, 0) != 0) {
            if (!opts.quiet) {
                fprintf(stderr, _("findmax: error processing '%s'\n"), paths[i]);
            }
        }
    }

    file_list_t *results = create_file_list();
    if (!results) {
        fprintf(stderr, _("findmax: memory allocation failed\n"));
        free_min_heap(heap);
        if (allocated_paths) {
            free(paths);
        }
        return 1;
    }

    file_entry_t *heap_entries = get_heap_entries(heap);
    size_t heap_size = get_heap_size(heap);

    for (size_t i = 0; i < heap_size; i++) {
        if (add_file_entry(results, heap_entries[i].path, &heap_entries[i].st, &opts) != 0) {
            fprintf(stderr, _("findmax: memory allocation failed\n"));
            break;
        }
    }

    sort_files(results, &opts);

    size_t print_count = (results->count < (size_t)opts.num_files) ? results->count : (size_t)opts.num_files;
    for (size_t i = 0; i < print_count; i++) {
        print_file_entry(&results->entries[i], &opts);
    }

    free_file_list(results);
    free_min_heap(heap);
    if (allocated_paths) {
        free(paths);
    }

    return 0;
}

void print_usage(void) {
    fputs(_("Usage: findmax [OPTION]... [FILE]...\n"
            "Find files with maximum values for specified criteria.\n"
            "Default behavior: show largest/newest files first (max-first).\n"),
          stdout);
    fputs("\n", stdout);
    fputs(_("Options:\n"), stdout);
    fputs("  -R, --recursive     ", stdout);
    fputs(_("recursive into directories\n"), stdout);
    fputs("  -r, --reverse       ", stdout);
    fputs(_("normal order (smallest/oldest first)\n"), stdout);
    fputs("  -u                  ", stdout);
    fputs(_("access time\n"), stdout);
    fputs("  -c                  ", stdout);
    fputs(_("metadata change time\n"), stdout);
    fputs("  -t                  ", stdout);
    fputs(_("file time (modification time)\n"), stdout);
    fputs("      --time=WORD     ", stdout);
    fputs(_("select timestamp (atime/access/use, ctime/status,\n"), stdout);
    fputs("                      ", stdout);
    fputs(_("mtime/modification, birth/creation)\n"), stdout);
    fputs("  -S                  ", stdout);
    fputs(_("file size\n"), stdout);
    fputs("  -n, --name          ", stdout);
    fputs(_("file name, order in current locale setting\n"), stdout);
    fputs("  -f, --file-only     ", stdout);
    fputs(_("print plain file only\n"), stdout);
    fputs("  -d, --dir-only      ", stdout);
    fputs(_("print directory only\n"), stdout);
    fputs("  -F, --format FMT    ", stdout);
    fputs(_("output format\n"), stdout);
    fputs("  -NUM                ", stdout);
    fputs(_("show top NUM files, default 1\n"), stdout);
    fputs("  -v, --verbose       ", stdout);
    fputs(_("repeat for more verbose loggings\n"), stdout);
    fputs("  -q, --quiet         ", stdout);
    fputs(_("show less logging messages\n"), stdout);
    fputs("  -L, --dereference   ", stdout);
    fputs(_("follow symbolic links\n"), stdout);
    fputs("      --maxdepth NUM  ", stdout);
    fputs(_("limit directory traversal depth\n"), stdout);
    fputs("  -h, --help          ", stdout);
    fputs(_("display this help and exit\n"), stdout);
    fputs("      --version       ", stdout);
    fputs(_("output version information and exit\n"), stdout);
    fputs("\n", stdout);
    fprintf(stdout, _("Report bugs to: <%s>\n"), PROJECT_EMAIL);
}

void print_version(void) {
    printf("findmax %s\n", PROJECT_VERSION);
    printf(_("Copyright (C) %d %s\n"), PROJECT_YEAR, PROJECT_AUTHOR);
    fputs(_("License AGPL-3.0-or-later: <https://www.gnu.org/licenses/agpl-3.0.html>\n"),
          stdout);
    fputs(_("This is free software: you are free to change and redistribute it.\n"), stdout);
    fputs(_("There is NO WARRANTY, to the extent permitted by law.\n"), stdout);
}

int parse_arguments(int argc, char *argv[], options_t *opts, char ***paths, int *path_count) {
    int opt;
    int option_index = 0;

    for (int i = 1; i < argc; i++) {
        if (argv[i][0] == '-' && isdigit(argv[i][1])) {
            char *endptr;
            long num = strtol(argv[i] + 1, &endptr, 10);
            if (*endptr == '\0' && num > 0) {
                opts->num_files = (int)num;
                for (int j = i; j < argc - 1; j++) {
                    argv[j] = argv[j + 1];
                }
                argc--;
                i--;
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
        {"help", no_argument, 0, 'h'},
        {"time", required_argument, 0, 1000},
        {"maxdepth", required_argument, 0, 1001},
        {"version", no_argument, 0, 1002},
        {0, 0, 0, 0}
    };

    while ((opt = getopt_long(argc, argv, "RructnSfdF:vqLh", long_options, &option_index)) != -1) {
        switch (opt) {
            case 'R':
                opts->recursive = 1;
                break;
            case 'r':
                opts->reverse = 0;
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
                log_more();
                break;
            case 'q':
                opts->quiet = 1;
                log_less();
                break;
            case 'L':
                opts->dereference = 1;
                break;
            case 'h':
                print_usage();
                exit(0);
            case 1000:
                if (strcmp(optarg, "atime") == 0 || strcmp(optarg, "access") == 0 || strcmp(optarg, "use") == 0) {
                    opts->sort_type = SORT_ATIME;
                } else if (strcmp(optarg, "ctime") == 0 || strcmp(optarg, "status") == 0) {
                    opts->sort_type = SORT_CTIME;
                } else if (strcmp(optarg, "mtime") == 0 || strcmp(optarg, "modification") == 0) {
                    opts->sort_type = SORT_MTIME;
                } else if (strcmp(optarg, "birth") == 0 || strcmp(optarg, "creation") == 0) {
                    opts->sort_type = SORT_BTIME;
                } else {
                    fprintf(stderr, _("findmax: invalid time type '%s'\n"), optarg);
                    return 1;
                }
                break;
            case 1001:
                {
                    char *endptr;
                    long depth = strtol(optarg, &endptr, 10);
                    if (*endptr != '\0' || depth < 0) {
                        fprintf(stderr, _("findmax: invalid maxdepth '%s'\n"), optarg);
                        return 1;
                    }
                    opts->max_depth = (int)depth;
                }
                break;
            case 1002:
                print_version();
                exit(0);
            case '?':
            default:
                print_usage();
                return 1;
        }
    }

    *path_count = argc - optind;
    if (*path_count > 0) {
        *paths = &argv[optind];
    }

    return 0;
}

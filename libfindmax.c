/*
 * libfindmax - Fast file finding library
 * Copyright (C) 2026 Lenik <findmax@bodz.net>
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include "libfindmax.h"

static int lib_initialized = 0;

int findmax_init(void) {
    if (lib_initialized) {
        return 0;
    }
    
    setlocale(LC_ALL, "");
    lib_initialized = 1;
    return 0;
}

void findmax_cleanup(void) {
    lib_initialized = 0;
}

int findmax_search(char **paths, int path_count, const options_t *opts, file_list_t **results) {
    if (!lib_initialized) {
        return -1;
    }
    
    if (!paths || path_count <= 0 || !opts || !results) {
        return -1;
    }
    
    *results = create_file_list();
    if (!*results) {
        return -1;
    }
    
    for (int i = 0; i < path_count; i++) {
        if (traverse_directory(paths[i], opts, *results) != 0) {
            // Continue with other paths even if one fails
        }
    }
    
    sort_files(*results, opts);
    return 0;
}

options_t *findmax_create_options(void) {
    options_t *opts = malloc(sizeof(options_t));
    if (!opts) {
        return NULL;
    }
    
    memset(opts, 0, sizeof(options_t));
    opts->sort_type = SORT_MTIME;
    opts->filter_type = FILTER_ALL;
    strcpy(opts->format, "%n");
    opts->num_files = DEFAULT_NUM_FILES;
    opts->max_depth = -1; // No limit by default
    
    return opts;
}

void findmax_free_options(options_t *opts) {
    free(opts);
}

int findmax_parse_time_type(const char *time_str) {
    if (!time_str) return -1;
    
    if (strcmp(time_str, "atime") == 0 || strcmp(time_str, "access") == 0 || strcmp(time_str, "use") == 0) {
        return SORT_ATIME;
    } else if (strcmp(time_str, "ctime") == 0 || strcmp(time_str, "status") == 0) {
        return SORT_CTIME;
    } else if (strcmp(time_str, "mtime") == 0 || strcmp(time_str, "modification") == 0) {
        return SORT_MTIME;
    } else if (strcmp(time_str, "birth") == 0 || strcmp(time_str, "creation") == 0) {
        return SORT_BTIME;
    }
    
    return -1;
}

const char *findmax_get_version(void) {
    return "1.0.0";
}

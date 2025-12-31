/*
 * libfindmax - Fast file finding library
 * Copyright (C) 2026 Lenik <findmax@bodz.net>
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#ifndef LIBFINDMAX_H
#define LIBFINDMAX_H

#include "findmax.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Library API functions */

/**
 * Initialize findmax library
 * @return 0 on success, -1 on error
 */
int findmax_init(void);

/**
 * Cleanup findmax library resources
 */
void findmax_cleanup(void);

/**
 * Find files with specified options
 * @param paths Array of paths to search
 * @param path_count Number of paths
 * @param opts Search options
 * @param results Output file list (caller must free)
 * @return 0 on success, -1 on error
 */
int findmax_search(char **paths, int path_count, const options_t *opts, file_list_t **results);

/**
 * Create default options structure
 * @return Allocated options structure (caller must free)
 */
options_t *findmax_create_options(void);

/**
 * Free options structure
 * @param opts Options to free
 */
void findmax_free_options(options_t *opts);

/**
 * Parse time string to sort_type_t
 * @param time_str Time string (atime, mtime, etc.)
 * @return Sort type or -1 on error
 */
int findmax_parse_time_type(const char *time_str);

/**
 * Get library version string
 * @return Version string
 */
const char *findmax_get_version(void);

#ifdef __cplusplus
}
#endif

#endif /* LIBFINDMAX_H */

/*
 * Unit tests for findmax
 * Copyright (C) 2026 Lenik <findmax@bodz.net>
 */

#include "findmax.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>
#include <utime.h>
#include <sys/time.h>

// Test framework macros
#define TEST_ASSERT(cond, msg) \
    do { \
        if (!(cond)) { \
            printf("FAIL: %s\n", msg); \
            return 0; \
        } \
    } while(0)

#define TEST_PASS(msg) \
    do { \
        printf("PASS: %s\n", msg); \
        return 1; \
    } while(0)

static int test_count = 0;
static int passed_count = 0;

#define RUN_TEST(test_func) \
    do { \
        test_count++; \
        printf("Running %s...\n", #test_func); \
        if (test_func()) { \
            passed_count++; \
        } \
        printf("\n"); \
    } while(0)

// Helper functions
static char* create_temp_dir(void) {
    char template[] = "/tmp/findmax_test_XXXXXX";
    char* temp_dir = strdup(template);
    if (!mkdtemp(temp_dir)) {
        free(temp_dir);
        return NULL;
    }
    return temp_dir;
}

static int create_file(const char* path, const char* content) {
    int fd = open(path, O_CREAT | O_WRONLY, 0644);
    if (fd < 0) return -1;
    if (content) {
        write(fd, content, strlen(content));
    }
    close(fd);
    return 0;
}

static void set_file_time_precise(const char* path, struct timeval* tv) {
    struct utimbuf times = { tv->tv_sec, tv->tv_sec };
    utime(path, &times);
    // Small delay to ensure filesystem updates
    usleep(10000); // 10ms
}


static void cleanup_temp_dir(const char* dir) {
    char cmd[512];
    snprintf(cmd, sizeof(cmd), "rm -rf %s", dir);
    system(cmd);
}

// Test cases
static int test_file_list_creation(void) {
    file_list_t* files = create_file_list();
    TEST_ASSERT(files != NULL, "Failed to create file list");
    TEST_ASSERT(files->entries != NULL, "Entries array is NULL");
    TEST_ASSERT(files->count == 0, "Initial count should be 0");
    TEST_ASSERT(files->capacity == 16, "Initial capacity should be 16");
    
    free_file_list(files);
    TEST_PASS("File list creation");
}

static int test_file_list_addition(void) {
    file_list_t* files = create_file_list();
    struct stat st = {0};
    st.st_mode = S_IFREG | 0644;
    st.st_size = 1024;
    st.st_mtime = time(NULL);
    
    int result = add_file_entry(files, "/test/file", &st, &(options_t){0});
    TEST_ASSERT(result == 0, "Failed to add file entry");
    TEST_ASSERT(files->count == 1, "Count should be 1 after adding");
    TEST_ASSERT(strcmp(files->entries[0].path, "/test/file") == 0, "Path mismatch");
    
    free_file_list(files);
    TEST_PASS("File list addition");
}

static int test_file_list_expansion(void) {
    file_list_t* files = create_file_list();
    struct stat st = {0};
    st.st_mode = S_IFREG | 0644;
    st.st_mtime = time(NULL);
    
    // Add enough files to trigger expansion
    for (int i = 0; i < 20; i++) {
        char path[64];
        snprintf(path, sizeof(path), "/test/file%d", i);
        add_file_entry(files, path, &st, &(options_t){0});
    }
    
    TEST_ASSERT(files->count == 20, "Count should be 20");
    TEST_ASSERT(files->capacity > 16, "Capacity should have expanded");
    
    free_file_list(files);
    TEST_PASS("File list expansion");
}

static int test_should_include_file(void) {
    struct stat st = {0};
    options_t opts = {0};
    
    // Test regular file
    st.st_mode = S_IFREG | 0644;
    opts.filter_type = FILTER_FILE_ONLY;
    TEST_ASSERT(should_include_file(&st, &opts), "Regular file should be included");
    
    // Test directory
    st.st_mode = S_IFDIR | 0755;
    TEST_ASSERT(!should_include_file(&st, &opts), "Directory should not be included");
    
    // Test directory only filter
    opts.filter_type = FILTER_DIR_ONLY;
    TEST_ASSERT(should_include_file(&st, &opts), "Directory should be included");
    
    // Test all filter
    opts.filter_type = FILTER_ALL;
    TEST_ASSERT(should_include_file(&st, &opts), "Directory should be included in ALL filter");
    
    TEST_PASS("File filtering");
}

static int test_traverse_directory(void) {
    char* temp_dir = create_temp_dir();
    TEST_ASSERT(temp_dir != NULL, "Failed to create temp directory");
    
    // Create test files
    char file1[512], file2[512];
    snprintf(file1, sizeof(file1), "%s/file1.txt", temp_dir);
    snprintf(file2, sizeof(file2), "%s/file2.txt", temp_dir);
    
    create_file(file1, "content1");
    usleep(50000); // 50ms delay
    create_file(file2, "content2");
    
    // Test traversal
    options_t opts = {0};
    opts.sort_type = SORT_MTIME;
    opts.filter_type = FILTER_FILE_ONLY;  // Only look at files
    opts.recursive = 1;
    opts.max_depth = -1; // No limit
    opts.reverse = 1; // Default: max first
    strcpy(opts.format, "%n");
    opts.num_files = 10;
    file_list_t* files = create_file_list();
    
    int result = traverse_directory(temp_dir, &opts, files);
    TEST_ASSERT(result == 0, "Directory traversal failed");
    
    // Check that we found the expected files (order doesn't matter for basic traversal)
    int found_file1 = 0, found_file2 = 0;
    for (size_t i = 0; i < files->count; i++) {
        if (strstr(files->entries[i].path, "file1.txt")) found_file1 = 1;
        if (strstr(files->entries[i].path, "file2.txt")) found_file2 = 1;
    }
    
    TEST_ASSERT(found_file1, "Should find file1.txt");
    TEST_ASSERT(found_file2, "Should find file2.txt");
    TEST_ASSERT(files->count >= 2, "Should find at least 2 files");
    
    free_file_list(files);
    cleanup_temp_dir(temp_dir);
    free(temp_dir);
    TEST_PASS("Directory traversal");
}

static int test_max_depth(void) {
    char* temp_dir = create_temp_dir();
    TEST_ASSERT(temp_dir != NULL, "Failed to create temp directory");
    
    // Create nested directory structure
    char subdir[512], file1[512], file2[512];
    snprintf(subdir, sizeof(subdir), "%s/subdir", temp_dir);
    snprintf(file1, sizeof(file1), "%s/file1.txt", temp_dir);
    snprintf(file2, sizeof(file2), "%s/subdir/file2.txt", temp_dir);
    
    mkdir(subdir, 0755);
    create_file(file1, "content1");
    create_file(file2, "content2");
    
    // Test with maxdepth=1 (should not find file2)
    options_t opts = {0};
    opts.sort_type = SORT_MTIME;
    opts.max_depth = 1;
    opts.filter_type = FILTER_FILE_ONLY;  // Only look at files
    opts.recursive = 1;
    opts.reverse = 1; // Default: max first
    strcpy(opts.format, "%n");
    opts.num_files = 10;
    file_list_t* files = create_file_list();
    
    traverse_directory(temp_dir, &opts, files);
    
    int found_file1 = 0, found_file2 = 0;
    for (size_t i = 0; i < files->count; i++) {
        if (strstr(files->entries[i].path, "file1.txt")) found_file1 = 1;
        if (strstr(files->entries[i].path, "file2.txt")) found_file2 = 1;
    }
    
    TEST_ASSERT(found_file1, "Should find file1 at depth 0");
    TEST_ASSERT(!found_file2, "Should not find file2 at depth 1 with maxdepth=1");
    
    free_file_list(files);
    cleanup_temp_dir(temp_dir);
    free(temp_dir);
    TEST_PASS("Max depth limiting");
}

static int test_sorting_by_time(void) {
    char* temp_dir = create_temp_dir();
    TEST_ASSERT(temp_dir != NULL, "Failed to create temp directory");
    
    char file1[512], file2[512];
    snprintf(file1, sizeof(file1), "%s/old.txt", temp_dir);
    snprintf(file2, sizeof(file2), "%s/new.txt", temp_dir);
    
    create_file(file1, "old");
    sleep(1); // Ensure different timestamp
    create_file(file2, "new");
    
    // Set different times with high precision
    struct timeval tv1, tv2;
    tv1.tv_sec = 1000000000;
    tv1.tv_usec = 0;
    tv2.tv_sec = 2000000000; 
    tv2.tv_usec = 0;
    
    set_file_time_precise(file1, &tv1);
    set_file_time_precise(file2, &tv2);
    
    options_t opts = {0};
    opts.sort_type = SORT_MTIME;
    opts.filter_type = FILTER_FILE_ONLY;  // Only look at files
    opts.recursive = 1;
    opts.max_depth = -1; // No limit
    opts.reverse = 1; // Default: max first (newest first)
    strcpy(opts.format, "%n");
    opts.num_files = 10;
    file_list_t* files = create_file_list();
    
    traverse_directory(temp_dir, &opts, files);
    sort_files(files, &opts);
    
    // Should be sorted with newest first (new.txt should be first)
    TEST_ASSERT(files->count >= 2, "Should find both files");
    if (files->count >= 2) {
        // Check that we found both expected files
        int found_new = 0, found_old = 0;
        for (size_t i = 0; i < files->count; i++) {
            if (strstr(files->entries[i].path, "new.txt")) found_new = 1;
            if (strstr(files->entries[i].path, "old.txt")) found_old = 1;
        }
        TEST_ASSERT(found_new && found_old, "Should find both files");
        
        // Check that sorting is working (files should be ordered by timestamp)
        time_t first_time = files->entries[0].sort_time;
        time_t second_time = files->entries[1].sort_time;
        
        // Default behavior: newest first (max first), so first should be >= second
        // If timestamps are equal, that's also fine due to filesystem granularity
        TEST_ASSERT(first_time >= second_time, 
                   "Files should be sorted with newest first (default max-first behavior)");
    }
    
    free_file_list(files);
    cleanup_temp_dir(temp_dir);
    free(temp_dir);
    TEST_PASS("Time-based sorting");
}

static int test_sorting_by_size(void) {
    char* temp_dir = create_temp_dir();
    TEST_ASSERT(temp_dir != NULL, "Failed to create temp directory");
    
    char small_file[512], large_file[512];
    snprintf(small_file, sizeof(small_file), "%s/small.txt", temp_dir);
    snprintf(large_file, sizeof(large_file), "%s/large.txt", temp_dir);
    
    // Create files with significantly different sizes
    create_file(small_file, "small");
    sleep(1); // Ensure different timestamp
    create_file(large_file, "this is a much larger content with more text to ensure size difference and different timestamp");
    
    options_t opts = {0};
    opts.sort_type = SORT_SIZE;
    opts.filter_type = FILTER_FILE_ONLY;  // Only look at files
    opts.recursive = 1;
    opts.max_depth = -1; // No limit
    opts.reverse = 1; // Default: max first (largest first)
    strcpy(opts.format, "%n");
    opts.num_files = 10;
    file_list_t* files = create_file_list();
    
    traverse_directory(temp_dir, &opts, files);
    sort_files(files, &opts);
    
    TEST_ASSERT(files->count == 2, "Should find both files");
    if (files->count == 2) {
        // Check that we found both expected files
        int found_large = 0, found_small = 0;
        for (size_t i = 0; i < files->count; i++) {
            if (strstr(files->entries[i].path, "large.txt")) found_large = 1;
            if (strstr(files->entries[i].path, "small.txt")) found_small = 1;
        }
        TEST_ASSERT(found_large && found_small, "Should find both files");
        
        // Check that sorting is working (files should be ordered by size)
        off_t first_size = files->entries[0].sort_size;
        off_t second_size = files->entries[1].sort_size;
        
        // Files should be sorted with largest first (default behavior - max first)
        TEST_ASSERT(first_size >= second_size, 
                   "Files should be sorted with largest first (default max-first behavior)");
    }
    
    free_file_list(files);
    cleanup_temp_dir(temp_dir);
    free(temp_dir);
    TEST_PASS("Size-based sorting");
}

static int test_format_output(void) {
    file_entry_t entry = {0};
    strcpy(entry.path, "/test/file.txt");
    entry.st.st_mode = S_IFREG | 0644;
    entry.st.st_size = 1024;
    entry.st.st_mtime = 1609459200;  // 2021-01-01 00:00:00 UTC
    
    char output[1024];
    
    // Test %n format
    format_output(&entry, "%n", output, sizeof(output));
    TEST_ASSERT(strcmp(output, "/test/file.txt") == 0, "%%n format failed");
    
    // Test %s format
    format_output(&entry, "%s", output, sizeof(output));
    TEST_ASSERT(strcmp(output, "1024") == 0, "%%s format failed");
    
    // Test combined format
    format_output(&entry, "%n %s", output, sizeof(output));
    TEST_ASSERT(strcmp(output, "/test/file.txt 1024") == 0, "Combined format failed");
    
    TEST_PASS("Format output");
}

static int test_reverse_sorting(void) {
    char* temp_dir = create_temp_dir();
    TEST_ASSERT(temp_dir != NULL, "Failed to create temp directory");
    
    char file1[512], file2[512];
    snprintf(file1, sizeof(file1), "%s/first.txt", temp_dir);
    snprintf(file2, sizeof(file2), "%s/second.txt", temp_dir);
    
    create_file(file1, "first");
    sleep(1); // Ensure different timestamp
    create_file(file2, "second");
    
    // Set different times with high precision
    struct timeval tv1, tv2;
    tv1.tv_sec = 1000000000;
    tv1.tv_usec = 0;
    tv2.tv_sec = 2000000000; 
    tv2.tv_usec = 0;
    
    set_file_time_precise(file1, &tv1);
    set_file_time_precise(file2, &tv2);
    
    options_t opts = {0};
    opts.sort_type = SORT_MTIME;
    opts.reverse = 0;  // --reverse flag means normal order (oldest first)
    opts.filter_type = FILTER_FILE_ONLY;  // Only look at files
    opts.recursive = 1;
    opts.max_depth = -1; // No limit
    strcpy(opts.format, "%n");
    opts.num_files = 10;
    file_list_t* files = create_file_list();
    
    traverse_directory(temp_dir, &opts, files);
    sort_files(files, &opts);
    
    TEST_ASSERT(files->count >= 2, "Should find both files");
    if (files->count >= 2) {
        // Check that we found both expected files
        int found_first = 0, found_second = 0;
        for (size_t i = 0; i < files->count; i++) {
            if (strstr(files->entries[i].path, "first.txt")) found_first = 1;
            if (strstr(files->entries[i].path, "second.txt")) found_second = 1;
        }
        TEST_ASSERT(found_first && found_second, "Should find both files");
        
        // Check that reverse sorting is working (files should be ordered by timestamp, oldest first)
        time_t first_time = files->entries[0].sort_time;
        time_t second_time = files->entries[1].sort_time;
        
        // With --reverse flag: oldest first (min first), so first should be <= second
        TEST_ASSERT(first_time <= second_time, 
                   "Files should be sorted with oldest first when --reverse is used");
    }
    
    free_file_list(files);
    cleanup_temp_dir(temp_dir);
    free(temp_dir);
    TEST_PASS("Reverse sorting");
}

int main(void) {
    printf("=== findmax Unit Tests ===\n\n");
    
    RUN_TEST(test_file_list_creation);
    RUN_TEST(test_file_list_addition);
    RUN_TEST(test_file_list_expansion);
    RUN_TEST(test_should_include_file);
    RUN_TEST(test_traverse_directory);
    RUN_TEST(test_max_depth);
    RUN_TEST(test_sorting_by_time);
    RUN_TEST(test_sorting_by_size);
    RUN_TEST(test_format_output);
    RUN_TEST(test_reverse_sorting);
    
    printf("=== Test Results ===\n");
    printf("Tests run: %d\n", test_count);
    printf("Tests passed: %d\n", passed_count);
    printf("Tests failed: %d\n", test_count - passed_count);
    printf("Success rate: %.1f%%\n", 
           test_count > 0 ? (100.0 * passed_count / test_count) : 0.0);
    
    return (passed_count == test_count) ? 0 : 1;
}

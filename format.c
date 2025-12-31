#include "findmax.h"

static void format_permissions_octal(mode_t mode, char *buf, size_t size) {
    snprintf(buf, size, "%o", mode & 07777);
}

static void format_permissions_human(mode_t mode, char *buf, size_t size) {
    char type;
    if (S_ISREG(mode)) type = '-';
    else if (S_ISDIR(mode)) type = 'd';
    else if (S_ISLNK(mode)) type = 'l';
    else if (S_ISBLK(mode)) type = 'b';
    else if (S_ISCHR(mode)) type = 'c';
    else if (S_ISFIFO(mode)) type = 'p';
    else if (S_ISSOCK(mode)) type = 's';
    else type = '?';
    
    snprintf(buf, size, "%c%c%c%c%c%c%c%c%c%c",
        type,
        (mode & S_IRUSR) ? 'r' : '-',
        (mode & S_IWUSR) ? 'w' : '-',
        (mode & S_IXUSR) ? 'x' : '-',
        (mode & S_IRGRP) ? 'r' : '-',
        (mode & S_IWGRP) ? 'w' : '-',
        (mode & S_IXGRP) ? 'x' : '-',
        (mode & S_IROTH) ? 'r' : '-',
        (mode & S_IWOTH) ? 'w' : '-',
        (mode & S_IXOTH) ? 'x' : '-');
}

static void format_file_type(mode_t mode, char *buf, size_t size) {
    const char *type;
    if (S_ISREG(mode)) type = "regular file";
    else if (S_ISDIR(mode)) type = "directory";
    else if (S_ISLNK(mode)) type = "symbolic link";
    else if (S_ISBLK(mode)) type = "block special file";
    else if (S_ISCHR(mode)) type = "character special file";
    else if (S_ISFIFO(mode)) type = "fifo";
    else if (S_ISSOCK(mode)) type = "socket";
    else type = "unknown";
    
    strncpy(buf, type, size - 1);
    buf[size - 1] = '\0';
}

static void format_time_human(time_t t, char *buf, size_t size) {
    if (t == 0) {
        strncpy(buf, "-", size - 1);
        buf[size - 1] = '\0';
        return;
    }
    
    struct tm *tm = localtime(&t);
    if (tm) {
        strftime(buf, size, "%Y-%m-%d %H:%M:%S", tm);
    } else {
        strncpy(buf, "-", size - 1);
        buf[size - 1] = '\0';
    }
}

static void get_username(uid_t uid, char *buf, size_t size) {
    struct passwd *pw = getpwuid(uid);
    if (pw) {
        strncpy(buf, pw->pw_name, size - 1);
        buf[size - 1] = '\0';
    } else {
        snprintf(buf, size, "%u", uid);
    }
}

static void get_groupname(gid_t gid, char *buf, size_t size) {
    struct group *gr = getgrgid(gid);
    if (gr) {
        strncpy(buf, gr->gr_name, size - 1);
        buf[size - 1] = '\0';
    } else {
        snprintf(buf, size, "%u", gid);
    }
}

void format_output(const file_entry_t *entry, const char *format, char *output, size_t output_size) {
    const char *p = format;
    char *out = output;
    size_t remaining = output_size - 1; // Leave space for null terminator
    
    while (*p && remaining > 0) {
        if (*p == '%' && *(p + 1)) {
            p++; // Skip %
            char temp_buf[256];
            const char *replacement = NULL;
            
            switch (*p) {
                case 'a': // permission bits in octal
                    format_permissions_octal(entry->st.st_mode, temp_buf, sizeof(temp_buf));
                    replacement = temp_buf;
                    break;
                    
                case 'A': // permission bits and file type in human readable form
                    format_permissions_human(entry->st.st_mode, temp_buf, sizeof(temp_buf));
                    replacement = temp_buf;
                    break;
                    
                case 'b': // number of blocks allocated
                    snprintf(temp_buf, sizeof(temp_buf), "%ld", (long)entry->st.st_blocks);
                    replacement = temp_buf;
                    break;
                    
                case 'B': // size in bytes of each block
                    snprintf(temp_buf, sizeof(temp_buf), "512"); // Standard block size
                    replacement = temp_buf;
                    break;
                    
                case 'd': // device number in decimal
                    snprintf(temp_buf, sizeof(temp_buf), "%ld", (long)entry->st.st_dev);
                    replacement = temp_buf;
                    break;
                    
                case 'D': // device number in hex
                    snprintf(temp_buf, sizeof(temp_buf), "%lx", (long)entry->st.st_dev);
                    replacement = temp_buf;
                    break;
                    
                case 'f': // raw mode in hex
                    snprintf(temp_buf, sizeof(temp_buf), "%x", entry->st.st_mode);
                    replacement = temp_buf;
                    break;
                    
                case 'F': // file type
                    format_file_type(entry->st.st_mode, temp_buf, sizeof(temp_buf));
                    replacement = temp_buf;
                    break;
                    
                case 'g': // group ID of owner
                    snprintf(temp_buf, sizeof(temp_buf), "%u", entry->st.st_gid);
                    replacement = temp_buf;
                    break;
                    
                case 'G': // group name of owner
                    get_groupname(entry->st.st_gid, temp_buf, sizeof(temp_buf));
                    replacement = temp_buf;
                    break;
                    
                case 'h': // number of hard links
                    snprintf(temp_buf, sizeof(temp_buf), "%ld", (long)entry->st.st_nlink);
                    replacement = temp_buf;
                    break;
                    
                case 'i': // inode number
                    snprintf(temp_buf, sizeof(temp_buf), "%ld", (long)entry->st.st_ino);
                    replacement = temp_buf;
                    break;
                    
                case 'n': // file name
                    replacement = entry->path;
                    break;
                    
                case 'N': // quoted file name with dereference if symbolic link
                    // For simplicity, just quote the name
                    if (strlen(entry->path) < sizeof(temp_buf) - 3) {
                        snprintf(temp_buf, sizeof(temp_buf), "'%s'", entry->path);
                        replacement = temp_buf;
                    } else {
                        replacement = "'[path too long]'";
                    }
                    break;
                    
                case 's': // total size, in bytes
                    snprintf(temp_buf, sizeof(temp_buf), "%ld", (long)entry->st.st_size);
                    replacement = temp_buf;
                    break;
                    
                case 'u': // user ID of owner
                    snprintf(temp_buf, sizeof(temp_buf), "%u", entry->st.st_uid);
                    replacement = temp_buf;
                    break;
                    
                case 'U': // user name of owner
                    get_username(entry->st.st_uid, temp_buf, sizeof(temp_buf));
                    replacement = temp_buf;
                    break;
                    
                case 'w': // time of file birth, human-readable
#ifdef __APPLE__
                    format_time_human(entry->st.st_birthtime, temp_buf, sizeof(temp_buf));
#else
                    strncpy(temp_buf, "-", sizeof(temp_buf) - 1);
                    temp_buf[sizeof(temp_buf) - 1] = '\0';
#endif
                    replacement = temp_buf;
                    break;
                    
                case 'W': // time of file birth, seconds since Epoch
#ifdef __APPLE__
                    snprintf(temp_buf, sizeof(temp_buf), "%ld", (long)entry->st.st_birthtime);
#else
                    snprintf(temp_buf, sizeof(temp_buf), "0");
#endif
                    replacement = temp_buf;
                    break;
                    
                case 'x': // time of last access, human-readable
                    format_time_human(entry->st.st_atime, temp_buf, sizeof(temp_buf));
                    replacement = temp_buf;
                    break;
                    
                case 'X': // time of last access, seconds since Epoch
                    snprintf(temp_buf, sizeof(temp_buf), "%ld", (long)entry->st.st_atime);
                    replacement = temp_buf;
                    break;
                    
                case 'y': // time of last data modification, human-readable
                    format_time_human(entry->st.st_mtime, temp_buf, sizeof(temp_buf));
                    replacement = temp_buf;
                    break;
                    
                case 'Y': // time of last data modification, seconds since Epoch
                    snprintf(temp_buf, sizeof(temp_buf), "%ld", (long)entry->st.st_mtime);
                    replacement = temp_buf;
                    break;
                    
                case 'z': // time of last status change, human-readable
                    format_time_human(entry->st.st_ctime, temp_buf, sizeof(temp_buf));
                    replacement = temp_buf;
                    break;
                    
                case 'Z': // time of last status change, seconds since Epoch
                    snprintf(temp_buf, sizeof(temp_buf), "%ld", (long)entry->st.st_ctime);
                    replacement = temp_buf;
                    break;
                    
                case '%': // literal %
                    temp_buf[0] = '%';
                    temp_buf[1] = '\0';
                    replacement = temp_buf;
                    break;
                    
                default:
                    // Unknown format specifier, just copy it literally
                    temp_buf[0] = '%';
                    temp_buf[1] = *p;
                    temp_buf[2] = '\0';
                    replacement = temp_buf;
                    break;
            }
            
            if (replacement) {
                size_t len = strlen(replacement);
                if (len > remaining) len = remaining;
                memcpy(out, replacement, len);
                out += len;
                remaining -= len;
            }
            
            p++;
        } else {
            *out++ = *p++;
            remaining--;
        }
    }
    
    *out = '\0';
}

void print_file_entry(const file_entry_t *entry, const options_t *opts) {
    char formatted_output[4096];
    format_output(entry, opts->format, formatted_output, sizeof(formatted_output));
    printf("%s\n", formatted_output);
}

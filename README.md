# findmax

A fast file finding utility optimized for O(1) queries to find files with maximum values for specified criteria.

## Features

- **Fast Performance**: Optimized for O(1) mode using priority queue/heap structures
- **Multiple Sort Criteria**: Sort by modification time, access time, creation time, size, or name
- **Flexible Filtering**: Filter by file type (files only, directories only, or all)
- **Recursive Search**: Traverse directories recursively
- **Custom Output Formatting**: Extensive format string support similar to `stat`
- **Locale-Aware**: Proper string comparison using current locale settings

## Installation

```bash
make
sudo make install
```

## Usage

```
findmax [OPTIONS] FILE...
```

### Options

- `-R, --recursive`: Recursive into directories
- `-r, --reverse`: Reversed order
- `-u`: Access time
- `-c`: Metadata change time  
- `-t`: File time (modification time, default)
- `--time=WORD`: Select timestamp type
  - `atime`, `access`, `use`: access time
  - `ctime`, `status`: metadata change time
  - `mtime`, `modification`: modification time (default)
  - `birth`, `creation`: birth time
- `-S`: File size
- `-n, --name`: File name (locale-aware sorting)
- `-f, --file-only`: Print plain files only
- `-d, --dir-only`: Print directories only
- `-F, --format FMT`: Output format string
- `-NUM`: Show top NUM files (default: 1)
- `-v, --verbose`: Verbose output
- `-q, --quiet`: Quiet mode
- `--version`: Show version information
- `--help`: Show help message

## Examples

### Find the latest modified file
```bash
findmax -t -R /path/to/directory
```

### Find the oldest file with custom format
```bash
findmax -r -t -R -F "%n %y" /path/to/directory
```

### Find the largest 5 files
```bash
findmax -S -R -5 /path/to/directory
```

### Find latest file with detailed information
```bash
findmax -t -F "%n %s %y %U:%G" /path/to/directory
```

## Format Strings

The `-F` option supports extensive format specifiers:

### File Information
- `%n`: File name
- `%N`: Quoted file name with dereference if symbolic link
- `%s`: Total size in bytes
- `%F`: File type
- `%f`: Raw mode in hex
- `%a`: Permission bits in octal
- `%A`: Permission bits and file type in human readable form

### Ownership
- `%u`: User ID of owner
- `%U`: User name of owner
- `%g`: Group ID of owner
- `%G`: Group name of owner

### Timestamps
- `%x`: Time of last access, human-readable
- `%X`: Time of last access, seconds since Epoch
- `%y`: Time of last data modification, human-readable
- `%Y`: Time of last data modification, seconds since Epoch
- `%z`: Time of last status change, human-readable
- `%Z`: Time of last status change, seconds since Epoch
- `%w`: Time of file birth, human-readable
- `%W`: Time of file birth, seconds since Epoch

### System Information
- `%d`: Device number in decimal
- `%D`: Device number in hex
- `%i`: Inode number
- `%h`: Number of hard links
- `%b`: Number of blocks allocated
- `%B`: Size in bytes of each block

## Performance

`findmax` is specifically optimized for fast queries and attempts to process files in O(1) mode by:

1. Using a min-heap data structure to maintain only the top N results
2. Avoiding full directory sorting when only the maximum values are needed
3. Efficient memory management with dynamic allocation
4. Locale-aware string comparisons for name sorting

## Building from Source

### Requirements
- GCC or compatible C compiler
- POSIX-compliant system (Linux, macOS, etc.)
- Standard C library with POSIX extensions

### Build Commands
```bash
# Standard build
make

# Debug build
make debug

# Optimized build with heap structures
make optimized

# Clean build artifacts
make clean

# Run tests
make test
```

## Examples in Action

```bash
# Find the most recently modified file in current directory
$ findmax -t .
./README.md

# Find top 3 largest files recursively
$ findmax -S -R -3 /home/user/documents
/home/user/documents/video.mp4
/home/user/documents/presentation.pptx
/home/user/documents/archive.zip

# Find newest file with size and timestamp
$ findmax -t -R -F "%n (%s bytes) - %y" /var/log
/var/log/syslog (1024 bytes) - 2025-12-31 21:45:23

# Find files only (no directories) by name
$ findmax -n -f -R /etc
/etc/zzz-update-motd
```

## License

This project is released under the MIT License.

## Contributing

Contributions are welcome! Please feel free to submit pull requests or open issues for bugs and feature requests.

#!/bin/bash
    : ${RCSID:=$Id: - 0.4.0 2013-12-25 23:43:30 - $}
    : ${PROGRAM_TITLE:="FindMax Benchmark Script"}
    : ${PROGRAM_SYNTAX:="[OPTIONS] [--] DIRS..."
                        "Benchmark findmax performance against shell commands"}

    LOGLEVEL=0

    . shlib-import cliboot
    option -o --output =FILE    "Base filename for reports (default: benchmark_report)"
    option -n --num =NUM        "Number of files to compare (default: 10)"
    option      --csv           "Generate CSV report"
    option      --latex         "Generate LaTeX report"
    option      --pdf           "Generate PDF report (requires pdflatex)"
    option      --markdown      "Generate Markdown report"
    option -q --quiet
    option -v --verbose
    option -h --help
    option    --version

    OUTPUT_FILE="benchmark_report"
    NUM_FILES=10
    DO_CSV=0
    DO_LATEX=0
    DO_PDF=0
    DO_MARKDOWN=0
    DIRS=()

function setopt() {
    case "$1" in
        -o|--output)
            OUTPUT_FILE="$2";;
        -n|--num)
            NUM_FILES="$2";;
        --csv)
            DO_CSV=1;;
        --latex)
            DO_LATEX=1;;
        --pdf)
            DO_PDF=1;;
        --markdown)
            DO_MARKDOWN=1;;
        -h|--help)
            help $1; exit;;
        -q|--quiet)
            LOGLEVEL=$((LOGLEVEL - 1));;
        -v|--verbose)
            LOGLEVEL=$((LOGLEVEL + 1));;
        --version)
            show_version; exit;;
        *)
            quit "invalid option: $1";;
    esac
}

# High-precision timing function
function time_command() {
    local cmd="$1"
    local start_time end_time elapsed
    
    start_time=$(date +%s.%N)
    eval "$cmd" > /tmp/benchmark_output 2>/dev/null || return 1
    end_time=$(date +%s.%N)
    
    elapsed=$(echo "($end_time - $start_time) * 1000" | bc -l)
    file_count=$(wc -l < /tmp/benchmark_output)
    
    echo "$elapsed $file_count"
}

# Run benchmark for a single directory
function benchmark_directory() {
    local dir="$1"
    local findmax_result shell_result
    local findmax_time findmax_files shell_time shell_files speedup
    
    _log1 "Benchmarking $dir..."
    
    # Check if directory exists
    if [[ ! -d "$dir" ]]; then
        _log0 "Error: Directory $dir does not exist"
        return 1
    fi
    
    # Benchmark findmax (sort by size, files only, recursive, reverse order)
    if ! findmax_result=$(time_command "./findmax -S -f -R -r -${NUM_FILES} '$dir'"); then
        _log0 "Error: Failed to run findmax on $dir"
        return 1
    fi
    
    findmax_time=$(echo "$findmax_result" | cut -d' ' -f1)
    findmax_files=$(echo "$findmax_result" | cut -d' ' -f2)
    
    # Benchmark shell equivalent (find + sort + head)
    if ! shell_result=$(time_command "find '$dir' -type f -exec stat -c '%s %n' {} + 2>/dev/null | sort -nr | head -${NUM_FILES}"); then
        _log0 "Error: Failed to run shell command on $dir"
        return 1
    fi
    
    shell_time=$(echo "$shell_result" | cut -d' ' -f1)
    shell_files=$(echo "$shell_result" | cut -d' ' -f2)
    
    # Calculate speedup
    if (( $(echo "$shell_time > 0" | bc -l) )); then
        speedup=$(echo "scale=2; $shell_time / $findmax_time" | bc -l)
    else
        speedup="0.00"
    fi
    
    _log1 "FindMax: ${findmax_time}ms ($findmax_files files)"
    _log1 "Shell:   ${shell_time}ms ($shell_files files)"
    _log1 "Speedup: ${speedup}x"
    
    # Store results globally
    BENCHMARK_RESULTS+=("$dir,$findmax_time,$shell_time,$speedup,$findmax_files,$shell_files")
}

# Generate CSV report
function generate_csv() {
    local output_file="$1"
    local csv_file="${output_file}.csv"
    
    echo "Directory,FindMax_Time(ms),Shell_Time(ms),Speedup,FindMax_Files,Shell_Files" > "$csv_file"
    
    for result in "${BENCHMARK_RESULTS[@]}"; do
        echo "$result" >> "$csv_file"
    done
    
    _log1 "CSV report generated: $csv_file"
}

# Generate Markdown report
function generate_markdown() {
    local output_file="$1"
    local md_file="${output_file}.md"
    local total_findmax=0 total_shell=0 total_findmax_files=0 total_shell_files=0
    
    cat > "$md_file" << EOF
# FindMax Benchmark Report

Generated on: $(date)
Directories tested: ${#BENCHMARK_RESULTS[@]}

## Summary

| Directory | FindMax (ms) | Shell (ms) | Speedup | FindMax Files | Shell Files |
|-----------|--------------|------------|---------|---------------|-------------|
EOF
    
    for result in "${BENCHMARK_RESULTS[@]}"; do
        IFS=',' read -r dir findmax_time shell_time speedup findmax_files shell_files <<< "$result"
        echo "| $dir | $findmax_time | $shell_time | ${speedup}x | $findmax_files | $shell_files |" >> "$md_file"
        
        total_findmax=$(echo "$total_findmax + $findmax_time" | bc -l)
        total_shell=$(echo "$total_shell + $shell_time" | bc -l)
        total_findmax_files=$((total_findmax_files + findmax_files))
        total_shell_files=$((total_shell_files + shell_files))
    done
    
    local avg_speedup
    if (( $(echo "$total_findmax > 0" | bc -l) )); then
        avg_speedup=$(echo "scale=2; $total_shell / $total_findmax" | bc -l)
    else
        avg_speedup="0.00"
    fi
    
    cat >> "$md_file" << EOF

## Overall Performance

- **Total FindMax time**: ${total_findmax} milliseconds
- **Total Shell time**: ${total_shell} milliseconds  
- **Average speedup**: ${avg_speedup}x
- **Total files processed**: $total_findmax_files (FindMax) vs $total_shell_files (Shell)

## Test Environment

- **FindMax version**: $(./findmax --version | head -1)
- **System**: $(uname -a)
- **Date**: $(date)
EOF
    
    _log1 "Markdown report generated: $md_file"
}

# Generate LaTeX report
function generate_latex() {
    local output_file="$1"
    local tex_file="${output_file}.tex"
    local total_findmax=0 total_shell=0
    
    cat > "$tex_file" << 'EOF'
\documentclass{article}
\usepackage{booktabs}
\usepackage{graphicx}
\usepackage{geometry}
\usepackage{longtable}
\geometry{a4paper, margin=1in}

\title{FindMax Benchmark Report}
EOF
    
    echo "\\author{Generated on $(date)}" >> "$tex_file"
    
    cat >> "$tex_file" << 'EOF'
\begin{document}

\maketitle

\section*{Summary}
EOF
    
    echo "Directories tested: ${#BENCHMARK_RESULTS[@]}\\\\" >> "$tex_file"
    
    cat >> "$tex_file" << 'EOF'

\begin{longtable}{lccccc}
\toprule
Directory & FindMax (ms) & Shell (ms) & Speedup & FindMax Files & Shell Files \\
\midrule
EOF
    
    for result in "${BENCHMARK_RESULTS[@]}"; do
        IFS=',' read -r dir findmax_time shell_time speedup findmax_files shell_files <<< "$result"
        # Escape underscores for LaTeX
        dir_escaped=$(echo "$dir" | sed 's/_/\\_/g')
        echo "$dir_escaped & $findmax_time & $shell_time & ${speedup}x & $findmax_files & $shell_files \\\\" >> "$tex_file"
        
        total_findmax=$(echo "$total_findmax + $findmax_time" | bc -l)
        total_shell=$(echo "$total_shell + $shell_time" | bc -l)
    done
    
    local avg_speedup
    if (( $(echo "$total_findmax > 0" | bc -l) )); then
        avg_speedup=$(echo "scale=2; $total_shell / $total_findmax" | bc -l)
    else
        avg_speedup="0.00"
    fi
    
    cat >> "$tex_file" << EOF
\\bottomrule
\\end{longtable}

\\section*{Overall Performance}
\\begin{itemize}
\\item Total FindMax time: ${total_findmax} milliseconds
\\item Total Shell time: ${total_shell} milliseconds
\\item Average speedup: ${avg_speedup}x
\\end{itemize}

\\end{document}
EOF
    
    _log1 "LaTeX report generated: $tex_file"
}

# Generate PDF from LaTeX
function generate_pdf() {
    local output_file="$1"
    local tex_file="${output_file}.tex"
    
    if [[ ! -f "$tex_file" ]]; then
        _log1 "LaTeX file not found, generating it first..."
        generate_latex "$output_file"
    fi
    
    _log1 "Generating PDF from LaTeX..."
    
    if command -v pdflatex >/dev/null 2>&1; then
        pdflatex -interaction=nonstopmode "$tex_file" >/dev/null 2>&1
        if [[ $? -eq 0 ]]; then
            _log1 "PDF report generated: ${output_file}.pdf"
            # Clean up auxiliary files
            rm -f "${output_file}.aux" "${output_file}.log"
        else
            _log0 "Failed to generate PDF"
        fi
    else
        _log0 "pdflatex not found. Please install texlive-latex-base to generate PDF reports."
    fi
}

function main() {
    # Collect directories from remaining arguments
    while [[ $# -gt 0 ]]; do
        DIRS+=("$1")
        shift
    done

    # Check if at least one directory is provided
    if [[ ${#DIRS[@]} -eq 0 ]]; then
        quit "At least one directory must be specified"
    fi

    # Check if findmax exists
    if [[ ! -x "./findmax" ]]; then
        quit "findmax executable not found in current directory. Please run 'make' to build findmax first."
    fi

    # Check for required tools
    if ! command -v bc >/dev/null 2>&1; then
        quit "bc (calculator) is required but not installed"
    fi

    # Validate NUM_FILES is a positive integer
    if ! [[ "$NUM_FILES" =~ ^[1-9][0-9]*$ ]]; then
        quit "NUM must be a positive integer, got: $NUM_FILES"
    fi

    # If no format specified, generate all
    if [[ $DO_CSV -eq 0 && $DO_LATEX -eq 0 && $DO_PDF -eq 0 && $DO_MARKDOWN -eq 0 ]]; then
        DO_CSV=1
        DO_LATEX=1
        DO_PDF=1
        DO_MARKDOWN=1
    fi

    # Initialize results array
    BENCHMARK_RESULTS=()

    _log1 "FindMax Benchmark"
    _log1 "================="

    # Run benchmarks for each directory
    for dir in "${DIRS[@]}"; do
        if benchmark_directory "$dir"; then
            : # Success, continue
        else
            _log0 "Failed to benchmark $dir"
        fi
    done

    if [[ ${#BENCHMARK_RESULTS[@]} -eq 0 ]]; then
        quit "No successful benchmarks"
    fi

    # Generate reports
    _log1 "Generating reports..."

    [[ $DO_CSV -eq 1 ]] && generate_csv "$OUTPUT_FILE"
    [[ $DO_MARKDOWN -eq 1 ]] && generate_markdown "$OUTPUT_FILE"
    [[ $DO_LATEX -eq 1 ]] && generate_latex "$OUTPUT_FILE"
    [[ $DO_PDF -eq 1 ]] && generate_pdf "$OUTPUT_FILE"

    _log1 "Benchmark completed successfully!"
}

boot "$@"

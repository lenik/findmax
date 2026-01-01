#!/bin/bash
    : ${RCSID:=$Id: - 0.4.0 2013-12-25 23:43:30 - $}
    : ${PROGRAM_TITLE:="FindMax Benchmark Script"}
    : ${PROGRAM_SYNTAX:="[OPTIONS] [--] DIRS..."
                        "Benchmark findmax performance against shell commands"}

    LOGLEVEL=0

    . shlib-import cliboot
    option -o --output =FILE    "Base filename for reports (default: benchmark_report)"
    option -n --num =NUM        "Number of files to compare (default: 10)"
    option -l --list            "Output and compare filenames between findmax and shell"
    option      --csv           "Generate CSV report"
    option      --latex         "Generate LaTeX report"
    option      --pdf           "Generate PDF report (requires pdflatex)"
    option      --markdown      "Generate Markdown report"
    option -q --quiet
    option -v --verbose
    option -h --help
    option    --version

    OUTPUT_FILE=benchmark_report
    NUM_ARRAY=()
    DO_LIST=0
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
            NUM_ARRAY+=("$2");;
        -l|--list)
            DO_LIST=1;;
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

# Format number with commas and 3 decimal places
function format_number() {
    local num="$1"
    # Format to 3 decimal places first
    local formatted=$(printf "%.3f" "$num")
    # Split into integer and decimal parts
    local int_part="${formatted%.*}"
    local dec_part="${formatted#*.}"
    # Add commas to integer part
    local int_with_commas=$(echo "$int_part" | sed -E ':a; s/([0-9])([0-9]{3})$/\1,\2/; ta')
    # Combine
    echo "${int_with_commas}.${dec_part}"
}

# Format integer with commas
function format_integer() {
    local num="$1"
    # Use awk for reliable comma formatting (works from right to left)
    echo "$num" | awk '{
        n = $1
        if (n == 0) {
            print "0"
            exit
        }
        result = ""
        count = 0
        while (n > 0) {
            if (count > 0 && count % 3 == 0) {
                result = "," result
            }
            result = (n % 10) result
            n = int(n / 10)
            count++
        }
        print result
    }'
}

# High-precision timing function
function time_command() {
    local cmd="$1"
    local output_file="$2"
    local start_time end_time elapsed
    
    if [[ -z "$output_file" ]]; then
        output_file="/tmp/benchmark_output"
    fi
    
    start_time=$(date +%s.%N)
    eval "$cmd" > "$output_file" 2>/dev/null || return 1
    end_time=$(date +%s.%N)
    
    elapsed=$(echo "($end_time - $start_time) * 1000" | bc -l)
    file_count=$(wc -l < "$output_file")
    
    echo "$elapsed $file_count"
}

# Run benchmark for a single directory
function benchmark_directory() {
    local dir="$1"
    local findmax_result shell_result
    local findmax_time findmax_files shell_time shell_files speedup
    local findmax_output="/tmp/benchmark_findmax_$$"
    local shell_output="/tmp/benchmark_shell_$$"
    local findmax_files_list shell_files_list files_match="yes"
    
    _log1 "Benchmarking $dir..."
    
    # Check if directory exists
    if [[ ! -d "$dir" ]]; then
        _log0 "Error: Directory $dir does not exist"
        return 1
    fi
    
    # Benchmark findmax (sort by size, files only, recursive, reverse order)
    if ! findmax_result=$(time_command "./findmax -S -f -R -r -${NUM_FILES} '$dir'" "$findmax_output"); then
        _log0 "Error: Failed to run findmax on $dir"
        rm -f "$findmax_output" "$shell_output"
        return 1
    fi
    
    findmax_time=$(echo "$findmax_result" | cut -d' ' -f1)
    findmax_files=$(echo "$findmax_result" | cut -d' ' -f2)
    
    # Benchmark shell equivalent (find + sort + head)
    if ! shell_result=$(time_command "find '$dir' -type f -exec stat -c '%s %n' {} + 2>/dev/null | sort -nr | head -${NUM_FILES}" "$shell_output"); then
        _log0 "Error: Failed to run shell command on $dir"
        rm -f "$findmax_output" "$shell_output"
        return 1
    fi
    
    shell_time=$(echo "$shell_result" | cut -d' ' -f1)
    shell_files=$(echo "$shell_result" | cut -d' ' -f2)
    
    # Compare filenames if --list option is enabled
    if [[ $DO_LIST -eq 1 ]]; then
        # Extract filenames from both outputs (shell output has "size filename" format)
        findmax_files_list=$(sort "$findmax_output" | tr '\n' '|')
        shell_files_list=$(awk '{print $2}' "$shell_output" | sort | tr '\n' '|')
        
        if [[ "$findmax_files_list" != "$shell_files_list" ]]; then
            files_match="no"
            _log0 "ERROR: Filenames differ between findmax and shell!"
            _log1 "FindMax files:"
            cat "$findmax_output" | while read line; do
                _log1 "  $line"
            done
            _log1 "Shell files:"
            cat "$shell_output" | while read line; do
                _log1 "  $line"
            done
        else
            _log2 "Filenames match between findmax and shell"
        fi
    fi
    
    # Calculate speedup
    if (( $(echo "$shell_time > 0" | bc -l) )); then
        speedup=$(echo "scale=2; $shell_time / $findmax_time" | bc -l)
    else
        speedup="0.00"
    fi
    
    _log1 "FindMax: ${findmax_time}ms ($findmax_files files)"
    _log1 "Shell:   ${shell_time}ms ($shell_files files)"
    _log1 "Speedup: ${speedup}x"
    
    # Store results globally (include NUM_FILES, and filenames if --list)
    if [[ $DO_LIST -eq 1 ]]; then
        BENCHMARK_RESULTS+=("$NUM_FILES,$dir,$findmax_time,$shell_time,$speedup,$findmax_files,$shell_files,$files_match,$findmax_files_list,$shell_files_list")
    else
        BENCHMARK_RESULTS+=("$NUM_FILES,$dir,$findmax_time,$shell_time,$speedup,$findmax_files,$shell_files")
    fi
    
    rm -f "$findmax_output" "$shell_output"
}

# Generate CSV report
function generate_csv() {
    local output_file="$1"
    local csv_file="${output_file}.csv"
    
    if [[ $DO_LIST -eq 1 ]]; then
        echo "NUM_Files,Directory,FindMax_Time(ms),Shell_Time(ms),Speedup,FindMax_Files,Shell_Files,Files_Match,FindMax_Filenames,Shell_Filenames" > "$csv_file"
    else
        echo "NUM_Files,Directory,FindMax_Time(ms),Shell_Time(ms),Speedup,FindMax_Files,Shell_Files" > "$csv_file"
    fi
    
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
    
    # Get unique NUM_FILES values
    local num_files_list=()
    for result in "${BENCHMARK_RESULTS[@]}"; do
        IFS=',' read -r num_files _ <<< "$result"
        if [[ ! " ${num_files_list[@]} " =~ " ${num_files} " ]]; then
            num_files_list+=("$num_files")
        fi
    done
    
    # Sort NUM_FILES
    IFS=$'\n' num_files_list=($(sort -n <<<"${num_files_list[*]}"))
    unset IFS
    
    cat > "$md_file" << EOF
# FindMax Benchmark Report

Generated on: $(date)
Directories tested: ${#BENCHMARK_RESULTS[@]}

EOF
    
    # Generate table for each NUM_FILES value
    for num_files in "${num_files_list[@]}"; do
        echo "## Results for NUM_FILES = $num_files" >> "$md_file"
        echo "" >> "$md_file"
        
        if [[ $DO_LIST -eq 1 ]]; then
            echo "| Directory | FindMax (ms) | Shell (ms) | Speedup | FindMax Files | Shell Files | Match | FindMax Filenames | Shell Filenames |" >> "$md_file"
            echo "|-----------|--------------|------------|---------|---------------|-------------|-------|-------------------|-----------------|" >> "$md_file"
        else
            echo "| Directory | FindMax (ms) | Shell (ms) | Speedup | FindMax Files | Shell Files |" >> "$md_file"
            echo "|-----------|--------------|------------|---------|---------------|-------------|" >> "$md_file"
        fi
        
        local section_total_findmax=0 section_total_shell=0 section_total_findmax_files=0 section_total_shell_files=0
        
        for result in "${BENCHMARK_RESULTS[@]}"; do
            if [[ $DO_LIST -eq 1 ]]; then
                IFS=',' read -r result_num dir findmax_time shell_time speedup findmax_files shell_files files_match findmax_list shell_list <<< "$result"
            else
                IFS=',' read -r result_num dir findmax_time shell_time speedup findmax_files shell_files <<< "$result"
            fi
            
            if [[ "$result_num" == "$num_files" ]]; then
                # Format numbers with commas
                findmax_time_fmt=$(format_number "$findmax_time")
                shell_time_fmt=$(format_number "$shell_time")
                speedup_fmt=$(format_number "$speedup")
                findmax_files_fmt=$(format_integer "$findmax_files")
                shell_files_fmt=$(format_integer "$shell_files")
                
                if [[ $DO_LIST -eq 1 ]]; then
                    findmax_list_display=$(echo "$findmax_list" | tr '|' ',' | sed 's/,$//')
                    shell_list_display=$(echo "$shell_list" | tr '|' ',' | sed 's/,$//')
                    echo "| $dir | $findmax_time_fmt | $shell_time_fmt | ${speedup_fmt}x | $findmax_files_fmt | $shell_files_fmt | $files_match | $findmax_list_display | $shell_list_display |" >> "$md_file"
                else
                    echo "| $dir | $findmax_time_fmt | $shell_time_fmt | ${speedup_fmt}x | $findmax_files_fmt | $shell_files_fmt |" >> "$md_file"
                fi
                
                section_total_findmax=$(echo "$section_total_findmax + $findmax_time" | bc -l)
                section_total_shell=$(echo "$section_total_shell + $shell_time" | bc -l)
                section_total_findmax_files=$((section_total_findmax_files + findmax_files))
                section_total_shell_files=$((section_total_shell_files + shell_files))
                
                total_findmax=$(echo "$total_findmax + $findmax_time" | bc -l)
                total_shell=$(echo "$total_shell + $shell_time" | bc -l)
                total_findmax_files=$((total_findmax_files + findmax_files))
                total_shell_files=$((total_shell_files + shell_files))
            fi
        done
        
        local section_avg_speedup
        if (( $(echo "$section_total_findmax > 0" | bc -l) )); then
            section_avg_speedup=$(echo "scale=3; $section_total_shell / $section_total_findmax" | bc -l)
        else
            section_avg_speedup="0.000"
        fi
        
        # Format summary numbers with commas
        section_total_findmax_fmt=$(format_number "$section_total_findmax")
        section_total_shell_fmt=$(format_number "$section_total_shell")
        section_avg_speedup_fmt=$(format_number "$section_avg_speedup")
        section_total_findmax_files_fmt=$(format_integer "$section_total_findmax_files")
        section_total_shell_files_fmt=$(format_integer "$section_total_shell_files")
        
        echo "" >> "$md_file"
        echo "**Section Summary (NUM_FILES = $num_files):**" >> "$md_file"
        echo "- Total FindMax time: ${section_total_findmax_fmt} milliseconds" >> "$md_file"
        echo "- Total Shell time: ${section_total_shell_fmt} milliseconds" >> "$md_file"
        echo "- Average speedup: ${section_avg_speedup_fmt}x" >> "$md_file"
        echo "- Files processed: ${section_total_findmax_files_fmt} (FindMax) vs ${section_total_shell_files_fmt} (Shell)" >> "$md_file"
        echo "" >> "$md_file"
    done
    
    # Overall summary
    local avg_speedup
    if (( $(echo "$total_findmax > 0" | bc -l) )); then
        avg_speedup=$(echo "scale=3; $total_shell / $total_findmax" | bc -l)
    else
        avg_speedup="0.000"
    fi
    
    # Format overall summary numbers with commas
    total_findmax_fmt=$(format_number "$total_findmax")
    total_shell_fmt=$(format_number "$total_shell")
    avg_speedup_fmt=$(format_number "$avg_speedup")
    total_findmax_files_fmt=$(format_integer "$total_findmax_files")
    total_shell_files_fmt=$(format_integer "$total_shell_files")
    
    cat >> "$md_file" << EOF
## Overall Performance Summary

- **Total FindMax time**: ${total_findmax_fmt} milliseconds
- **Total Shell time**: ${total_shell_fmt} milliseconds  
- **Average speedup**: ${avg_speedup_fmt}x
- **Total files processed**: ${total_findmax_files_fmt} (FindMax) vs ${total_shell_files_fmt} (Shell)

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
\usepackage{pdflscape}
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
    
    # Get unique NUM_FILES values
    local num_files_list=()
    for result in "${BENCHMARK_RESULTS[@]}"; do
        IFS=',' read -r num_files _ <<< "$result"
        if [[ ! " ${num_files_list[@]} " =~ " ${num_files} " ]]; then
            num_files_list+=("$num_files")
        fi
    done
    
    # Sort NUM_FILES
    IFS=$'\n' num_files_list=($(sort -n <<<"${num_files_list[*]}"))
    unset IFS
    
    # Generate table for each NUM_FILES value
    for num_files in "${num_files_list[@]}"; do
        echo "\\subsection*{Results for NUM\_FILES = $num_files}" >> "$tex_file"
        
        # Start landscape environment for the long table
        cat >> "$tex_file" << 'EOF'
\small
EOF
        
        if [[ $DO_LIST -eq 1 ]]; then
            cat >> "$tex_file" << 'EOF'
\begin{longtable}{lrrrrrc}
\toprule
Directory & FindMax (ms) & Shell (ms) & Speedup & FindMax Files & Shell Files & Match \\
\midrule
EOF
            local section_total_findmax=0 section_total_shell=0
            for result in "${BENCHMARK_RESULTS[@]}"; do
                IFS=',' read -r result_num dir findmax_time shell_time speedup findmax_files shell_files files_match findmax_list shell_list <<< "$result"
                if [[ "$result_num" == "$num_files" ]]; then
                    # Escape underscores for LaTeX and format numbers to 3 decimal places with commas
                    dir_escaped=$(echo "$dir" | sed 's/_/\\_/g')
                    findmax_time_fmt=$(format_number "$findmax_time")
                    shell_time_fmt=$(format_number "$shell_time")
                    speedup_fmt=$(format_number "$speedup")
                    findmax_files_fmt=$(format_integer "$findmax_files")
                    shell_files_fmt=$(format_integer "$shell_files")
                    echo "$dir_escaped & $findmax_time_fmt & $shell_time_fmt & ${speedup_fmt}x & $findmax_files_fmt & $shell_files_fmt & $files_match \\\\" >> "$tex_file"
                    
                    section_total_findmax=$(echo "$section_total_findmax + $findmax_time" | bc -l)
                    section_total_shell=$(echo "$section_total_shell + $shell_time" | bc -l)
                    total_findmax=$(echo "$total_findmax + $findmax_time" | bc -l)
                    total_shell=$(echo "$total_shell + $shell_time" | bc -l)
                fi
            done
        else
            cat >> "$tex_file" << 'EOF'
\begin{longtable}{lrrrrr}
\toprule
Directory & FindMax (ms) & Shell (ms) & Speedup & FindMax Files & Shell Files \\
\midrule
EOF
            local section_total_findmax=0 section_total_shell=0
            for result in "${BENCHMARK_RESULTS[@]}"; do
                IFS=',' read -r result_num dir findmax_time shell_time speedup findmax_files shell_files <<< "$result"
                if [[ "$result_num" == "$num_files" ]]; then
                    # Escape underscores for LaTeX and format numbers to 3 decimal places with commas
                    dir_escaped=$(echo "$dir" | sed 's/_/\\_/g')
                    findmax_time_fmt=$(format_number "$findmax_time")
                    shell_time_fmt=$(format_number "$shell_time")
                    speedup_fmt=$(format_number "$speedup")
                    findmax_files_fmt=$(format_integer "$findmax_files")
                    shell_files_fmt=$(format_integer "$shell_files")
                    echo "$dir_escaped & $findmax_time_fmt & $shell_time_fmt & ${speedup_fmt}x & $findmax_files_fmt & $shell_files_fmt \\\\" >> "$tex_file"
                    
                    section_total_findmax=$(echo "$section_total_findmax + $findmax_time" | bc -l)
                    section_total_shell=$(echo "$section_total_shell + $shell_time" | bc -l)
                    total_findmax=$(echo "$total_findmax + $findmax_time" | bc -l)
                    total_shell=$(echo "$total_shell + $shell_time" | bc -l)
                fi
            done
        fi
        
        # End landscape environment for this section
        cat >> "$tex_file" << EOF
\\bottomrule
\\end{longtable}
\\normalsize
EOF
        
        local section_avg_speedup
        if (( $(echo "$section_total_findmax > 0" | bc -l) )); then
            section_avg_speedup=$(echo "scale=3; $section_total_shell / $section_total_findmax" | bc -l)
        else
            section_avg_speedup="0.000"
        fi
        
        # Format summary numbers to 3 decimal places with commas
        section_total_findmax_fmt=$(format_number "$section_total_findmax")
        section_total_shell_fmt=$(format_number "$section_total_shell")
        section_avg_speedup_fmt=$(format_number "$section_avg_speedup")
        
        echo "\\textbf{Section Summary (NUM\_FILES = $num_files):}" >> "$tex_file"
        echo "\\begin{itemize}" >> "$tex_file"
        echo "\\item Total FindMax time: ${section_total_findmax_fmt} milliseconds" >> "$tex_file"
        echo "\\item Total Shell time: ${section_total_shell_fmt} milliseconds" >> "$tex_file"
        echo "\\item Average speedup: ${section_avg_speedup_fmt}x" >> "$tex_file"
        echo "\\end{itemize}" >> "$tex_file"
        echo "" >> "$tex_file"
    done
    
    local avg_speedup
    if (( $(echo "$total_findmax > 0" | bc -l) )); then
        avg_speedup=$(echo "scale=3; $total_shell / $total_findmax" | bc -l)
    else
        avg_speedup="0.000"
    fi
    
    # Format overall summary numbers to 3 decimal places with commas
    total_findmax_fmt=$(format_number "$total_findmax")
    total_shell_fmt=$(format_number "$total_shell")
    avg_speedup_fmt=$(format_number "$avg_speedup")
    
    cat >> "$tex_file" << EOF

\\section*{Overall Performance Summary}
\\begin{itemize}
\\item Total FindMax time: ${total_findmax_fmt} milliseconds
\\item Total Shell time: ${total_shell_fmt} milliseconds
\\item Average speedup: ${avg_speedup_fmt}x
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

    # If no format specified, generate all
    if [[ $DO_CSV -eq 0 && $DO_LATEX -eq 0 && $DO_PDF -eq 0 && $DO_MARKDOWN -eq 0 ]]; then
        DO_CSV=1
        DO_LATEX=1
        DO_PDF=1
        DO_MARKDOWN=1
    fi

    # Initialize results array
    BENCHMARK_RESULTS=()

    for NUM_FILES in ${NUM_ARRAY[@]}; do
        _log1 "FindMax Benchmark for num $NUM_FILES"
        _log1 "================================="

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
    done

    # Generate reports
    _log1 "Generating reports..."
    
    # If PDF is requested but LaTeX isn't, generate LaTeX automatically
    if [[ $DO_PDF -eq 1 && $DO_LATEX -eq 0 ]]; then
        DO_LATEX=1
    fi
    
    [[ $DO_CSV -eq 1 ]] && generate_csv "$OUTPUT_FILE"
    [[ $DO_MARKDOWN -eq 1 ]] && generate_markdown "$OUTPUT_FILE"
    [[ $DO_LATEX -eq 1 ]] && generate_latex "$OUTPUT_FILE"
    [[ $DO_PDF -eq 1 ]] && generate_pdf "$OUTPUT_FILE"

    _log1 "Benchmark completed successfully!"
}

boot "$@"

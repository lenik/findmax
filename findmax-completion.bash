# Bash completion for findmax
# Copyright (C) 2026 Lenik <findmax@bodz.net>

_findmax() {
    local cur prev opts
    COMPREPLY=()
    cur="${COMP_WORDS[COMP_CWORD]}"
    prev="${COMP_WORDS[COMP_CWORD-1]}"

    # Long options
    local long_opts="--recursive --reverse --name --file-only --dir-only --format --verbose --quiet --dereference --time --maxdepth --version --help"
    
    # Short options
    local short_opts="-R -r -u -c -t -S -n -f -d -F -v -q -L"
    
    # All options combined
    opts="$long_opts $short_opts"

    # Handle options that take arguments
    case "$prev" in
        --format|-F)
            # Format string completion - provide common examples
            COMPREPLY=( $(compgen -W '"%n" "%n %y" "%n %s" "%n %s %y" "%A %n" "%U:%G %n"' -- "$cur") )
            return 0
            ;;
        --time)
            # Time type completion
            COMPREPLY=( $(compgen -W "atime access use ctime status mtime modification birth creation" -- "$cur") )
            return 0
            ;;
        --maxdepth)
            # Number completion for maxdepth
            COMPREPLY=( $(compgen -W "1 2 3 4 5 10" -- "$cur") )
            return 0
            ;;
    esac

    # Handle -NUM options
    if [[ "$cur" =~ ^-[0-9]+$ ]]; then
        return 0
    fi

    # Complete options
    if [[ "$cur" == -* ]]; then
        COMPREPLY=( $(compgen -W "$opts" -- "$cur") )
        return 0
    fi

    # Complete file and directory names
    COMPREPLY=( $(compgen -f -- "$cur") )
    return 0
}

# Register the completion function
complete -F _findmax findmax

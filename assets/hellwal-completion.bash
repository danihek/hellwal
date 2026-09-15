#!/usr/bin/env bash

_hellwal_complete() {
    local cur prev opts
    COMPREPLY=()
    cur="${COMP_WORDS[COMP_CWORD]}"
    prev="${COMP_WORDS[COMP_CWORD-1]}"
    
    opts="-i --image -d --dark -l --light -c --color -v --invert -m --neon-mode -r --random -q --quiet -j --json \
          -s --script -f --template-folder -o --output -t --theme -k --theme-folder -g --gray-scale -n --dark-offset \
          -b --bright-offset --debug --no-cache --static-background --dominant-background --static-foreground -h --help"

    case "$prev" in
        -i|--image)
            COMPREPLY=( $(compgen -f -- "$cur") ) # Complete file names
            return 0
            ;;
        -s|--script)
            COMPREPLY=( $(compgen -f -- "$cur") ) # Complete script files
            return 0
            ;;
        -f|--template-folder|-o|--output|-k|--theme-folder)
            COMPREPLY=( $(compgen -d -- "$cur") ) # Complete directories
            return 0
            ;;
        -t|--theme)
            COMPREPLY=( $(compgen -f -- "$cur") ) # Complete theme files
            return 0
            ;;
        -g|--gray-scale|-n|--dark-offset|-b|--bright-offset)
            COMPREPLY=( $(compgen -W "0.0 0.1 0.2 0.3 0.4 0.5 0.6 0.7 0.8 0.9 1.0" -- "$cur") ) # Suggest valid floats
            return 0
            ;;
        --static-background|--static-foreground)
            COMPREPLY=( $(compgen -W "#000000 #FFFFFF #FF0000 #00FF00 #0000FF #FFFF00 #FF00FF #00FFFF" -- "$cur") )
            return 0
            ;;
    esac

    if [[ ${cur} == -* ]] ; then
        COMPREPLY=( $(compgen -W "$opts" -- "$cur") )
        return 0
    fi
}

complete -F _hellwal_complete hellwal

set -g __fish_colors black red green brown blue magenta cyan white normal

complete -c set_color -x -d "Color" -a '$__fish_colors'
complete -c set_color -s b -l background -x -a '$__fish_colors'


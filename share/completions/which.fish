
complete -c which -s a -l all -d (N_ "Print all matching executables in PATH, not just the first")
complete -c which -s i -l read-alias -d (N_ "Read aliases from stdin, reporting matching ones on stdout")
complete -c which -l skip-alias -d (N_ "Ignore option '--read-alias'")
complete -c which -l read-functions -d (N_ "Read shell function definitions from stdin, reporting matching ones on stdout")
complete -c which -l skip-functions -d (N_ "Ignore option '--read-functions'")
complete -c which -l skip-dot -d (N_ "Skip directories in PATH that start with a dot")
complete -c which -l skip-tilde -d (N_ "Skip directories in PATH that start with a tilde and executables which reside in the HOME directory")
complete -c which -l show-dot -d (N_ "If a directory in PATH starts with a dot and a matching executable was found for that path, then print './programname'")
complete -c which -l show-tilde -d (N_ "Output a tilde when a directory matches the HOME directory")
complete -c which -l tty-only -d (N_ "Stop processing options on the right if not on tty")
complete -c which -s v -s V -l version -d (N_ "Display version and exit")
complete -c which -l help -d (N_ "Display help and exit")

complete -c which -a "(__fish_complete_command)" -d "Command" -x

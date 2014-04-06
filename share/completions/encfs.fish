# Completions for encfs
# SanskritFritz (gmail)

complete -c encfs -s i -l idle -d "Unmount when idle for specified MINUTES"
complete -c encfs -s f -d "Run in the foreground"
complete -c encfs -s v -l verbose -d "Verbose messages when run foreground"
complete -c encfs -s s -d "Run in single threaded mode"
complete -c encfs -s d -l fuse-debug -d "Enables debugging within the FUSE library"
complete -c encfs -l forcedecode -d "Return data even from corrupted files"
complete -c encfs -l public -d "Make files public to all other users"
complete -c encfs -l ondemand -d "Mount the filesystem on-demand"
complete -c encfs -l reverse -d "Produce encrypted view of plain files"
complete -c encfs -l standard -d "Use standard options when creating filesystem"
complete -c encfs -s o -d "Pass on options to FUSE"
complete -c encfs -l no-default-flags -d "Don't use the default FUSE flags"
complete -c encfs -l extpass -d "Get password from an external program"
complete -c encfs -s S -l stdinpass -d "Read password from standard input"
complete -c encfs -l anykey -d "Turn off key validation checking"

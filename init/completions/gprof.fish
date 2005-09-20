complete -c gprof -s A -l annoted-source -d "Print annotated source"
complete -c gprof -s b -l brief -d "Do not print explanations"
complete -c gprof -s C -l exec-counts -d "Print tally"
complete -c gprof -s i -l file-info -d "Display summary"
complete -c gprof -s I -l directory-path -d "Search directories for source"
complete -c gprof -s J -l no-annotated-source -d "No annotated source"
complete -c gprof -s L -l print-path -d "Print full path of source"
complete -c gprof -s p -l flat-profile -d "Print flat profile"
complete -c gprof -s P -l no-flat-profile -d "No flat profile"
complete -c gprof -s q -l graph -d "Print call graph"
complete -c gprof -s Q -l no-graph -d "No call graph"
complete -c gprof -s y -l separate-files -d "Annotate to file"
complete -c gprof -s Z -l no-exec-counts -d "No tally"
complete -c gprof -l function-ordering -d "Suggest function ordering"
complete -rc gprof -l file-ordering -d "Suggest file ordering"
complete -c gprof -s T -l traditional -d "Traditional mode"
complete -xc gprof -s w -l width -d "Set width of output"
complete -c gprof -s x -l all-lines -d "Anotate every line"
complete -xc gprof -l demangle -d "Set demangling style"
complete -c gprof -l no-demangle -d "Turn of demangling"
complete -xc gprof -s a -l no-static -d "Supress static functions"
complete -xc gprof -s c -l static-call-graph -d ""
complete -xc gprof -s D -l ignore-non-functions -d "Ignore symbols not known to be functions"
complete -xc gprof -s k -r -d "Delete arcs from callgraph"
complete -xc gprof -s l -l line -d "Line by line profiling"
complete -xc gprof -s m -l min-count -r -d "Supress output when executed less than specified times"
complete -xc gprof -s n -l time -d "Only propagate times for matching symbols"
complete -xc gprof -s N -l no-time -d "Do not propagate times for matching symbols"
complete -xc gprof -s z -l display-unused-functions -d "Mention unused functions in flat profile"
complete -xc gprof -s d -l debug -d "Specify debugging options"
complete -xc gprof -s h -l help -d "Print help and exit"
complete -xc gprof -s v -l version -d "Print version and exit"
complete -xc gprof -s O -l file-format -x -a "auto bsd 4.4bsd magic prof" -d "Profile data format"
complete -xc gprof -s s -l sum -d "Print summary"

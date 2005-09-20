complete -c gunzip -s c -l stdout -d "Compress to stdout"
complete -c gunzip -x -a "(
	__fish_complete_suffix (commandline -ct) .gz 'Compressed file'
	__fish_complete_suffix (commandline -ct) .tgz 'Compressed archive'
)
"
complete -c gunzip -s f -l force -d "Overwrite"
complete -c gunzip -s h -l help -d "Display help"
complete -c gunzip -s l -l list -d "List compression information"
complete -c gunzip -s L -l license -d "Print license"
complete -c gunzip -s n -l no-name -d "Do not save/restore filename"
complete -c gunzip -s N -l name -d "Save/restore filename"
complete -c gunzip -s q -l quiet -d "Supress warnings"
complete -c gunzip -s r -l recursive -d "Recurse directories"
complete -c gunzip -s S -l suffix -r -d "Suffix"
complete -c gunzip -s t -l test -d "Check integrity"
complete -c gunzip -s v -l verbose -d "Display compression ratios"
complete -c gunzip -s V -l version -d "Display version"

complete -c gunzip -x -a "(
	__fish_complete_suffix (commandline -ct) .gz 'Compressed file'
	__fish_complete_suffix (commandline -ct) .tgz 'Compressed archive'
)
"


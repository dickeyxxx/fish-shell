complete -c cp -s a -l archive --description "Same as -dpR"
complete -c cp -s b -l backup --description "Make backup of each existing destination file" -a "none off numbered t existing nil simple never"
complete -c cp -l copy-contents --description "Copy contents of special files when recursive"
complete -c cp -s d --description "Same as --no-dereference --preserve=link"
complete -c cp -s f -l force --description "Do not prompt before overwriting"
complete -c cp -s i -l interactive --description "Prompt before overwrite"
complete -c cp -s H --description "Follow command-line symbolic links"
complete -c cp -s l -l link --description "Link files instead of copying"
complete -c cp -l strip-trailing-slashes --description "Remove trailing slashes from source"
complete -c cp -s S -l suffix -r --description "Backup suffix"
complete -c cp -s t -l target-directory --description "Target directory" -x -a "(__fish_complete_directories (commandline -ct) 'Target directory')"
complete -c cp -s u -l update --description "Do not overwrite newer files"
complete -c cp -s v -l verbose --description "Verbose mode"
complete -c cp -l help --description "Display help and exit"
complete -c cp -l version --description "Display version and exit"
complete -c cp -s L -l dereference --description "Always follow symbolic links"
complete -c cp -s P -l no-dereference --description "Never follow symbolic links"
complete -c cp -s p  --description "Same as --preserve=mode,ownership,timestamps"
complete -c cp -l preserve --description "Preserve the specified attributes and security contexts, if possible" -a "mode ownership timestamps links all"
complete -c cp -l no-preserve -r --description "Don't preserve the specified attributes" -a "mode ownership timestamps links all"
complete -c cp -l parents --description "Use full source file name under DIRECTORY"
complete -c cp -s r -s R -l recursive --description "Copy directories recursively"
complete -c cp -l remove-destination --description "Remove each existing destination file before attempting to open it (contrast with --force)"
complete -c cp -l sparse -r --description "Control creation of sparse files" -a "always auto never"
complete -c cp -s s -l symbolic-link --description "Make symbolic links instead of copying"
complete -c cp -s T -l no-target-directory --description "Treat DEST as a normal file"
complete -c cp -s x -l one-file-system --description "Stay on this file system"
complete -c cp -s X -l context -r --description "Set security context of copy to CONTEXT"

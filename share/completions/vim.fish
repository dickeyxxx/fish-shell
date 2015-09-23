# these don't work
#complete vim -a - --description 'The file to edit is read from stdin. Commands are read from stderr, which should be a tty'

# todo
# +[num]        : Position the cursor on line number
# +/{pat}       : Position the cursor on the first occurence of {pat}
# +{command}    : Execute Ex command after the first file has been read
complete vim -s c -r --description 'Execute Ex command after the first file has been read'
complete vim -s S -r --description 'Source file after the first file has been read'
complete vim -l cmd -r --description 'Execute Ex command before loading any vimrc'
complete vim -s d -r --description 'Use device as terminal (Amiga only)'
complete vim -s i -r --description 'Set the viminfo file location'
complete vim -s o -r --description 'Open stacked windows for each file'
complete vim -s O -r --description 'Open side by side windows for each file'
complete vim -s p -r --description 'Open tab pages for each file'
complete vim -s q -r --description 'Start in quickFix mode'
complete vim -s r -r --description 'Use swap files for recovery'
complete vim -s s -r --description 'Source and execute script file'
complete vim -s t -r --description 'Set the cursor to tag'
complete vim -s T -r --description 'Terminal name'
complete vim -s u -r --description 'Use alternative vimrc'
complete vim -s U -r --description 'Use alternative vimrc in GUI mode'
complete vim -s w -r --description 'Record all typed characters'
complete vim -s W -r --description 'Record all typed characters (overwrite file)'

complete vim -s A --description 'Start in Arabic mode'
complete vim -s b --description 'Start in binary mode'
complete vim -s C --description 'Behave mostly like vi'
complete vim -s d --description 'Start in diff mode'
complete vim -s D --description 'Debugging mode'
complete vim -s e --description 'Start in Ex mode'
complete vim -s E --description 'Start in improved Ex mode'
complete vim -s f --description 'Start in foreground mode'
complete vim -s F --description 'Start in Farsi mode'
complete vim -s g --description 'Start in GUI mode'
complete vim -s h --description 'Print help message and exit'
complete vim -s H --description 'Start in Hebrew mode'
complete vim -s L --description 'List swap files'
complete vim -s l --description 'Start in lisp mode'
complete vim -s m --description 'Disable file modification'
complete vim -s M --description 'Disallow file modification'
complete vim -s N --description 'Reset compatibility mode'
complete vim -s n --description 'Don\'t use swap files'
complete vim -s R --description 'Read only mode'
complete vim -s r --description 'List swap files'
complete vim -s s --description 'Start in silent mode'
complete vim -s V --description 'Start in verbose mode'
complete vim -s v --description 'Start in vi mode'
complete vim -s x --description 'Use encryption when writing files'
complete vim -s X --description 'Don\'t connect to X server'
complete vim -s y --description 'Start in easy mode'
complete vim -s Z --description 'Start in restricted mode'

complete vim -o nb --description 'Become an editor server for NetBeans'

complete vim -l no-fork --description 'Start in foreground mode'
complete vim -l echo-wid --description 'Echo the Window ID on stdout (GTK GUI only)'
complete vim -l help --description 'Print help message and exit'
complete vim -l literal --description 'Do not expand wildcards'
complete vim -l noplugin --description 'Skip loading plugins'
complete vim -l remote --description 'Edit files on Vim server'
complete vim -l remote-expr --description 'Evaluate expr on Vim server'
complete vim -l remote-send --description 'Send keys to Vim server'
complete vim -l remote-silent --description 'Edit files on Vim server'
complete vim -l remote-wait --description 'Edit files on Vim server'
complete vim -l remote-wait-silent --description 'Edit files on Vim server'
complete vim -l serverlist --description 'List all Vim servers that can be found'
complete vim -l servername --description 'Set server name'
complete vim -l version --description 'Print version information and exit'

complete vim -l socketid -r --description 'Run gvim in another window (GTK GUI only)'

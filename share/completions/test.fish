
complete -c test -l help -d (N_ "Display help and exit")
complete -c test -l version -d (N_ "Display version and exit")
complete -c test -a ! -d (N_ "Negate expression")
complete -c test -s a -d (N_ "Logical and")
complete -c test -s o -d (N_ "Logical or")
complete -c test -s n -d (N_ "String length is non-zero")
complete -c test -s z -d (N_ "String length is zero")
complete -c test -a = -d (N_ "Strings are equal")
complete -c test -a != -d (N_ "Strings are not equal")
complete -c test -o eq -d (N_ "Integers are equal")
complete -c test -o ge -d (N_ "Left integer larger than or equal to right integer")
complete -c test -o gt -d (N_ "Left integer larger than right integer")
complete -c test -o le -d (N_ "Left integer less than or equal to right integer")
complete -c test -o lt -d (N_ "Left integer less than right integer")
complete -c test -o ne -d (N_ "Left integer not equal to right integer")
complete -c test -o ef -d (N_ "Left file equal to right file")
complete -c test -o nt -d (N_ "Left file newer than right file")
complete -c test -o ot -d (N_ "Left file older than right file")
complete -c test -s b -d (N_ "File is block device")
complete -c test -s c -d (N_ "File is character device")
complete -c test -s d -d (N_ "File is directory")
complete -c test -s e -d (N_ "File exists")
complete -c test -s f -d (N_ "File is regular")
complete -c test -s g -d (N_ "File is set-group-ID")
complete -c test -s h -d (N_ "File is symlink")
complete -c test -s G -d (N_ "File owned by effective group ID")
complete -c test -s k -d (N_ "File has sticky bit set")
complete -c test -s L -d (N_ "File is symlink")
complete -c test -s O -d (N_ "File owned by effective user ID")
complete -c test -s p -d (N_ "File is named pipe")
complete -c test -s r -d (N_ "File is readable")
complete -c test -s s -d (N_ "File size is non-zero")
complete -c test -s S -d (N_ "File is socket")
complete -c test -s t -d (N_ "FD is terminal")
complete -c test -s u -d (N_ "File set-user-ID bit is set")
complete -c test -s w -d (N_ "File is writable")
complete -c test -s x -d (N_ "File is executable")


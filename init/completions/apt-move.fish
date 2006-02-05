#apt-move
complete -c apt-move -a get -d (_ "Generate master file")
complete -c apt-move -a getlocal -d (_ "Alias for 'get'")
complete -f -c apt-move -a move -d (_ "Move packages to local tree")
complete -f -c apt-move -a delete -d (_ "Delete obsolete package files")
complete -f -c apt-move -a packages -d (_ "Build new local files")
complete -f -c apt-move -a fsck -d (_ "Rebuild index files")
complete -f -c apt-move -a update -d (_ "Move packages from cache to local mirror")
complete -f -c apt-move -a local -d (_ "Alias for 'move delete packages'")
complete -f -c apt-move -a localupdate -d (_ "Alias for 'update'")
complete -f -c apt-move -a mirror -d (_ "Download package missing from mirror")
complete -f -c apt-move -a sync -d (_ "Sync packages installed")
complete -f -c apt-move -a exclude -d 'test $LOCALDIR/.exclude file'
complete -c apt-move -a movefile -d (_ "Move file specified on commandline")
complete -f -c apt-move -a listbin -d (_ "List packages that may serve as input to mirrorbin or mirrorsource" )
complete -f -c apt-move -a mirrorbin -d (_ "Fetch package from STDIN")
complete -f -c apt-move -a mirrorsrc -d (_ "Fetch source package from STDIN")
complete -f -c apt-move -s a -d (_ "Process all packages")
complete -c apt-move -s c -d (_ "Use specific conffile")
complete -f -c apt-move -s f -d (_ "Force deletion")
complete -f -c apt-move -s q -d (_ "Suppresses normal output")
complete -f -c apt-move -s t -d (_ "Test run")

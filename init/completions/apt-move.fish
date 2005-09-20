#apt-move
complete -c apt-move -a get -d "generate master file"
complete -c apt-move -a getlocal -d "alias of get"
complete -f -c apt-move -a move -d "move pkgs to local tree"
complete -f -c apt-move -a delete -d "delete obsolete pkg files"
complete -f -c apt-move -a packages -d "build new local files"
complete -f -c apt-move -a fsck -d "rebuild index files"
complete -f -c apt-move -a update -d "move pkgs from cache to local mirror"
complete -f -c apt-move -a local -d "alias to move delete packages"
complete -f -c apt-move -a localupdate -d "alias for update"
complete -f -c apt-move -a mirror -d "download pkg missing from mirror"
complete -f -c apt-move -a sync -d "sync pkgs installed"
complete -f -c apt-move -a exclude -d 'test $LOCALDIR/.exclude file'
complete -c apt-move -a movefile -d "move file from CLI"
complete -f -c apt-move -a listbin -d "mirror|sync|repo"
complete -f -c apt-move -a mirrorbin -d "fetch pkg from STDIN"
complete -f -c apt-move -a mirrorsrc -d "fetch src pkg from STDIN"
complete -f -c apt-move -s a -d "process all pkgs"
complete -c apt-move -s c -d "use specific conffile"
complete -f -c apt-move -s d -d "use specific dist"
complete -f -c apt-move -s f -d "force deletion"
complete -f -c apt-move -s q -d "suppresses normal output"
complete -f -c apt-move -s t -d "test run"

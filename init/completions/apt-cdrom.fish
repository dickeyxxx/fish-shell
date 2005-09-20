#apt-cdrom
complete -c apt-cdrom -s h -l help -d "apt-cdrom command help"
complete -r -c apt-cdrom -a add -d "add new disc to source list"
complete -x -c apt-cdrom -a ident -d "report identity of disc"
complete -r -c apt-cdrom -s d -l cdrom -d "mount point"
complete -f -c apt-cdrom -s r -l rename -d "rename a disc"
complete -f -c apt-cdrom -s m -l no-mount -d "no mounting"
complete -f -c apt-cdrom -s f -l fast -d "fast copy"
complete -f -c apt-cdrom -s a -l thorough -d "thorough pkg scan"
complete -f -c apt-cdrom -s n -l no-act -d "no changes"
complete -f -c apt-cdrom -s v -l version -d "show version"
complete -r -c apt-cdrom -s c -l config-file -d "specify config file"
complete -x -c apt-cdrom -s o -l option -d "specify options"

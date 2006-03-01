#apt-src
complete -c apt-src -s h -l help -d (N_ "Display help and exit")
complete -f -c apt-src -a "update" -d (N_ "Update list of source packages")
complete -f -c apt-src -a "install" -d (N_ "Install source packages")
complete -f -c apt-src -a "upgrade" -d (N_ "Upgrade source packages")
complete -f -c apt-src -a "remove" -d (N_ "Remove source packages")
complete -f -c apt-src -a "build" -d (N_ "Build source packages")
complete -f -c apt-src -a "clean" -d (N_ "Clean source packages")
complete -f -c apt-src -a "import" -d (N_ "Detect known source tree")
complete -f -c apt-src -a "list" -d (N_ "List installed source package\(s\)")
complete -f -c apt-src -a "location" -d (N_ "Root source tree")
complete -f -c apt-src -a "version" -d (N_ "Version of source package")
complete -f -c apt-src -a "name" -d (N_ "Name of the source package")
complete -f -c apt-src -s b -l build -d (N_ "Build source packages")
complete -f -c apt-src -s i -l installdebs -d (N_ "Install after build")
complete -f -c apt-src -s p -l patch -d (N_ "Patch local changes")
complete -r -c apt-src -s l -l location -d (N_ "Specify a dir")
complete -c apt-src -s c -l here -d (N_ "Run on current dir")
complete -f -c apt-src -l upstream-version -d (N_ "Omit debian version")
complete -f -c apt-src -s k -l keep-built -d (N_ "Do not del built files")
complete -f -c apt-src -s n -l no-delete-source -d (N_ "Do not del source files")
complete -f -c apt-src -l version -d (N_ "Source tree version")
complete -f -c apt-src -s q -l quiet -d (N_ "Output to /dev/null")
complete -f -c apt-src -s t -l trace -d (N_ "Output trace")

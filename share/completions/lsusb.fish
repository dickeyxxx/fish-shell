complete -c lsusb -s v -l verbose --description "Increase verbosity (show descriptors)"
complete -x -c lsusb -s s -a '(__fish_complete_usb)' --description "Show only devices with specified device and/or bus numbers (in decimal)"
complete -c lsusb -s d --description "Show only devices with the specified vendor and product ID numbers (in hexadecimal)"
complete -c lsusb -s D -l device --description "Selects which device lsusb will examine"
complete -c lsusb -s t -l tree --description "Dump the physical USB device hierarchy as a tree"
complete -c lsusb -s V -l version --description "Show version of program"
complete -c lsusb -s h -l help --description "Show usage and help"

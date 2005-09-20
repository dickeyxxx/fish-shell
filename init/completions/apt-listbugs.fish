#apt-listbugs
complete -c apt-listbugs -s h -l help -d "apt-listbugs command help"
complete -f -c apt-listbugs -s s -l severity -a "critical grave"  -d "set severity"
complete -f -c apt-listbugs -s T -l tag -d "Targs you want to see"
complete -f -c apt-listbugs -s S -l stats -d "outstanding 'pending upload' resolved done open" -d "status you want to see"
complete -f -c apt-listbugs -s l -l showless -d "ignore bugs in your system"
complete -f -c apt-listbugs -s g -l showgreater -d "ignore newer bugs than upgrade pkgs"
complete -f -c apt-listbugs -s D -l show-downgrade -d "bugs for downgrade pkgs"
complete -f -c apt-listbugs -s H -l hostname -a "osdn.debian.or.jp" -d "Bug Tracking system"
complete -f -c apt-listbugs -s p -l port -d "specify port for web interface"
complete -f -c apt-listbugs -s R -l release-critical -d "use daily bug report"
complete -f -c apt-listbugs -s I -l index -d "use the raw index.db"
complete -f -c apt-listbugs -s X -l indexdir -d "specify index dir"
complete -f -c apt-listbugs -s P -l pin-priority -d "specify Pin-Priority value"
complete -f -c apt-listbugs -l title -d "specify the title of rss"
complete -f -c apt-listbugs -s f -l force-download -d "retrieve fresh bugs"
complete -f -c apt-listbugs -s q -l quiet -d "do not display progress bar"
complete -f -c apt-listbugs -s c -l cache-dir -a "/var/cache/apt-listbugs/" -d "specify local cache dir"
complete -f -c apt-listbugs -s t -l timer -d "specify the expire cache timer"
complete -c apt-listbugs -s C -l aptconf -d "specify apt config file"
complete -f -c apt-listbugs -s y -l force-yes -d "assume all yes"
complete -f -c apt-listbugs -s n -l force-no -d "assume all no"
complete -c apt-listbugs -a list -d "list bugs from pkgs"
complete -c apt-listbugs -a rss -d "list bugs in rss format"


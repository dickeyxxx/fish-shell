# Completions for the rpm command. Insanely complicated,
# since rpm has multiple operation modes, and a perverse number of switches.

complete -c rpm -s "?" -l help --description "Display help and exit"
complete -c rpm -l version --description "Display version and exit"
complete -c rpm -l quiet --description "Quiet mode"
complete -c rpm -s v --description "Verbose mode"
complete -c rpm -l rcfile --description "List of rpm configuration files" -f
complete -c rpm -l pipe --description "Pipe output through specified command" -r
complete -c rpm -l dbpath --description "Specify directory for rpm database" -a "
(
	__fish_complete_directories (commandline -ct) 'Rpm database directory'
)
"
complete -c rpm -l root --description "Specify root directory for rpm operations" -a "
(
	__fish_complete_directories (commandline -ct) 'Root directory for rpm operations'
)
"

set rpm_install -c rpm -n "__fish_contains_opt -s i -s U -s F install upgrade freshen"
complete $rpm_install -l aid --description "Add suggested packages to the transaction set when needed"
complete $rpm_install -l allfiles --description "Installs or upgrades all the files in the package, even if they aren't needed (missingok) and don't exist"
complete $rpm_install -l badreloc --description "Used with --relocate, permit relocations on all file paths, not just those OLD-PATH's included in the binary package relocation hint(s)"
complete $rpm_install -l excludepath --description "Don't install files whose name begins with specified path" -xa "(__fish_complete_directories (commandline -ct) 'Skip installation of files in this directory')"
complete $rpm_install -l excludedocs --description "Don't install any files which are marked as documentation"
complete $rpm_install -l force --description 'Same as using --replacepkgs, --replacefiles, and --oldpackage'
complete $rpm_install -s h -l hash --description 'Print 50 hash marks as the package archive is unpacked'
complete $rpm_install -l ignoresize --description "Don't check for sufficient disk space before installation"
complete $rpm_install -l ignorearch --description "Allow installation or upgrading even if the architectures of the binary package and host don't match"
complete $rpm_install -l ignoreos --description "Allow installation or upgrading even if the operating systems of the binary package and host don't match"
complete $rpm_install -l includedocs --description 'Install documentation files (default)'
complete $rpm_install -l justdb --description 'Update only the database, not the filesystem'
complete $rpm_install -l nodigest --description "Don't verify package or header digests when reading"
complete $rpm_install -l nosignature --description "Don't verify package or header signatures when reading"
complete $rpm_install -l nodeps --description "Don't do a dependency check"
complete $rpm_install -l nosuggest --description "Don't suggest package(s) that provide a missing dependency"
complete $rpm_install -l noorder --description "Don't change the package installation order"
complete $rpm_install -l noscripts --description "Don't execute scripts"
complete $rpm_install -l nopre --description "Don't execute pre scripts"
complete $rpm_install -l nopost --description "Don't execute post scripts"
complete $rpm_install -l nopreun --description "Don't execute preun scripts"
complete $rpm_install -l nopostun --description "Don't execute postun scripts"
complete $rpm_install -l notriggers --description "Don't execute trigger scriptlets"
complete $rpm_install -l notriggerin --description "Don't execute triggerin scriptlets"
complete $rpm_install -l notriggerun --description "Don't execute triggerun scriptlets"
complete $rpm_install -l notriggerpostun --description "Don't execute triggerpostun scriptlets"
complete $rpm_install -l oldpackage --description 'Allow an upgrade to replace a newer package with an older one'
complete $rpm_install -l percent --description 'Print percentages as files are unpacked from the package archive. This is intended to make rpm easy to run from other tools'
complete $rpm_install -l prefix --description 'For relocatable binary packages, translate all file paths that start with the installation prefix in the package relocation hint(s) to NEWPATH' -xa "(__fish_complete_directories (commandline -ct) 'Directory prefix for relocatable packages')"
complete $rpm_install -l relocate -x --description "Translate all paths that start with first half of following parameter to second half of following parameter"
complete $rpm_install -l repackage --description 'Re-package the files before erasing'
complete $rpm_install -l replacefiles --description 'Install the packages even if they replace files from other, already installed, packages'
complete $rpm_install -l replacepkgs --description 'Install the packages even if some of them are already installed on this system'
complete $rpm_install -l test --description "Don't install the package, simply check for and report potential conflicts"
set -e rpm_install

set rpm_query -c rpm -n "__fish_contains_opt -s q query"
complete $rpm_query -l changelog --description 'Display change information for the package'
complete $rpm_query -s c -l configfiles --description 'List only configuration files (implies -l)'
complete $rpm_query -s d -l docfiles --description 'List only documentation files (implies -l)'
complete $rpm_query -l dump --description 'Dump file information. Must be used with at least one of -l, -c, -d'
complete $rpm_query -l filesbypkg --description 'List all the files in each selected package'
complete $rpm_query -s i -l info --description 'Display package information, including name, version, and description. Uses --queryformat if specified'
complete $rpm_query -l last --description 'Orders the package listing by install time'
complete $rpm_query -s l -l list --description 'List files in package'
complete $rpm_query -l provides --description 'List capabilities this package provides'
complete $rpm_query -s R -l requires --description 'List packages on which this package depends'
complete $rpm_query -l scripts --description 'List the package specific scriptlets'
complete $rpm_query -s s -l state --description 'Display the states of files in the package. The state of each file is one of normal, not installed, or replaced'
complete $rpm_query -l triggers --description 'Display the trigger scripts contained in the package'
complete $rpm_query -l triggerscripts --description 'Display the trigger scripts contained in the package'
set -e rpm_query

set rpm_select -c rpm -n "__fish_contains_opt -s q -s V query verify"

complete $rpm_select -a "(__fish_print_packages)"
complete $rpm_select -s a -l all --description 'Query all installed packages'
complete $rpm_select -s f -l file --description 'Query package owning specified file' -r
complete $rpm_select -l fileid --description 'Query package that contains a given file identifier, i.e. the MD5 digest of the file contents' -x
complete $rpm_select -s g -l group --description 'Query packages with the specified group' -x
complete $rpm_select -l hdrid --description 'Query package that contains a given header identifier, i.e. the SHA1 digest of the immutable header region' -x
complete $rpm_select -s p -l package --description 'Query an (uninstalled) package in specified file' -xa "(__fish_complete_suffix .rpm)"
complete $rpm_select -l pkgid --description 'Query package that contains a given package identifier, i.e. the MD5 digest of the combined header and payload contents' -x
complete $rpm_select -l specfile --description 'Parse and query specified spec-file as if it were a package' -xa "(__fish_complete_suffix .spec)"
complete $rpm_select -l tid --description 'Query package(s) that have the specified TID (transaction identifier)' -x
complete $rpm_select -l triggeredby --description 'Query packages that are triggered by the specified packages' -x -a "(__fish_print_packages)"
complete $rpm_select -l whatprovides --description 'Query all packages that provide the specified capability' -x
complete $rpm_select -l whatrequires --description 'Query all packages that requires the specified capability for functioning' -x
set -e rpm_select

set rpm_verify -c rpm -n "__fish_contains_opt -s V verify"
complete $rpm_verify -l nodeps --description "Don't verify dependencies of packages"
complete $rpm_verify -l nodigest --description "Don't verify package or header digests when reading"
complete $rpm_verify -l nofiles --description "Don't verify any attributes of package files"
complete $rpm_verify -l noscripts --description "Don't execute the %verifyscript scriptlet"
complete $rpm_verify -l nosignature --description "Don't verify package or header signatures when reading"
complete $rpm_verify -l nolinkto --description "Don't verify linkto attribute"
complete $rpm_verify -l nomd5 --description "Don't verify md5 attribute"
complete $rpm_verify -l nosize --description "Don't verify size attribute"
complete $rpm_verify -l nouser --description "Don't verify user attribute"
complete $rpm_verify -l nogroup --description "Don't verify group attribute"
complete $rpm_verify -l nomtime --description "Don't verify time attribute"
complete $rpm_verify -l nomode --description "Don't verify mode attribute"
complete $rpm_verify -l nordev --description "Don't verify dev attribute"
set -e rpm_verify

set rpm_erase -c rpm -n "__fish_contains_opt -s e erase"
complete $rpm_erase -a "(__fish_print_packages)"
complete $rpm_erase -l allmatches --description 'Remove all versions of the package which match specified string'
complete $rpm_erase -l nodeps --description "Don't check dependencies before uninstalling the packages"
complete $rpm_erase -l noscripts --description "Don't execute scriplets"
complete $rpm_erase -l nopreun --description "Don't execute preun scriptlet"
complete $rpm_erase -l nopostun --description "Don't execute postun scriptlet"
complete $rpm_erase -l notriggers --description "Don't execute trigger scriptlets"
complete $rpm_erase -l notriggerun --description "Don't execute triggerun scriptlets"
complete $rpm_erase -l notriggerpostun --description "Don't execute triggerpostun scriptlets"
complete $rpm_erase -l repackage --description 'Re-package the files before erasing'
complete $rpm_erase -l test --description "Don't really uninstall anything"
set -e rpm_erase

set rpm_mode -c rpm -n '__fish_contains_opt -s e -s i -s F -s V -s U -s q erase install freshen verify upgrade query; if test $status = 0; false; else; true; end'
complete $rpm_mode -s i -l install --description 'Install new package'
complete $rpm_mode -s U -l upgrade --description 'Upgrade existing package'
complete $rpm_mode -s F -l freshen --description 'Upgrade package if already installed'
complete $rpm_mode -s q -l query --description 'Query installed packages'
complete $rpm_mode -s V -l verify --description 'Verify package integrity'
complete $rpm_mode -s e -l erase --description 'Erase package'
set -e rpm_mode

complete -c rpm -xa '(__fish_complete_suffix .rpm)'

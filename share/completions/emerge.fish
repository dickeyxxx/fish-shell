# Completions for emerge

# Author: Tassilo Horn <tassilo@member.fsf.org>

function __fish_emerge_print_installed_pkgs -d (N_ 'Prints completions for installed packages on the system from /var/db/pkg')
 if test -d /var/db/pkg
   find /var/db/pkg/ -type d | cut -d'/' -f5-6 | sort | uniq | sed 's/-[0-9]\{1,\}\..*$//'
   return
 end
end

function __fish_emerge_print_all_pkgs -d (N_ 'Prints completions for all available packages on the system from /usr/portage')
 if test -d /usr/portage
   find /usr/portage/ -maxdepth 2 -type d | cut -d'/' -f4-5 | sed 's/^\(distfiles\|profiles\|eclass\).*$//' | sort | uniq | sed 's/-[0-9]\{1,\}\..*$//'
   return
 end
end

#########################
# Actions
complete -c emerge -xua "(__fish_emerge_print_all_pkgs)"
complete -c emerge -l sync -d "Synchronize the portage tree"
complete -c emerge -l info -d "Get informations to include in bug reports"
complete -c emerge -s V -l version -d "Displays the version number of emerge"
complete -c emerge -s h -l help -xa '""\t"'(_ "Usage overview of emerge")'" system\t"'(_ "Help on subject system")'" config\t"'(_ "Help on subject config")'" sync\t"'(_ "Help on subject sync")'"' -d "Displays help information for emerge"
complete -c emerge -s c -l clean -d "Remove packages that will not affect the functionality of the system"
complete -c emerge -l config -xa "(__fish_emerge_print_installed_pkgs)" -d "Run package specific actions needed to be executed after the emerge process"
complete -c emerge -l depclean -d "WARNING: Delete all packages that are neither deps nor in world"
complete -c emerge -l metadata -d "Process all meta-cache files"
complete -c emerge -s P -l prune -xa "(__fish_emerge_print_installed_pkgs)" -d "WARNING: Remove all but the latest version of package"
complete -c emerge -l regen -d "Check and update the dependency cache"
complete -c emerge -l resume -d "Resume the last merge operation"
complete -c emerge -s s -l search -d "Search for matches in the portage tree"
complete -c emerge -s S -l searchdesc -d "Search for matches in package names and descriptions"
complete -c emerge -s C -l unmerge -xa "(__fish_emerge_print_installed_pkgs)" -d "WARNING: Remove the given package"
complete -c emerge -s u -l update -xa "(__fish_emerge_print_installed_pkgs)" -d "Update the given package"
# END Actions
#########################

#########################
# Options
complete -c emerge -l alphabetical -d "Sort flag lists alphabetically"
complete -c emerge -s a -l ask -d "Prompt the user before peforming the merge"
complete -c emerge -s b -l buildpkg -d "Build a binary package additionally"
complete -c emerge -s B -l buildpkgonly -d "Only build a binary package"
complete -c emerge -s l -l changelog -d "Show changelog of package. Use with --pretend"
complete -c emerge -l color -xa 'y\t"'(_ "Use colors in output")'" n\t"'(_ "Don't use colors in output")'"' -d "Toggle colorized output"
complete -c emerge -l colums -d "Align output. Use with --pretend"
complete -c emerge -s d -l debug -d "Run in debug mode"
complete -c emerge -s D -l deep -d "Consider the whole dependency tree"
complete -c emerge -s e -l emptytree -d "Reinstall all world packages"
complete -c emerge -s f -l fetchonly -d "Only download the packages but don't install them"
complete -c emerge -s F -l fetch-all-uri -d "Same as --fetchonly and grab all potential files"
complete -c emerge -s g -l getbinpkg -d "Download infos from each binary package. Implies -k"
complete -c emerge -s G -l getbinpkgonly -d "As -g but don't use local infos"
complete -c emerge -l ignore-default-opts -d "Ignore EMERGE_DEFAULT_OPTS"
complete -c emerge -s N -l newuse -d "Include installed packages with changed USE flags"
complete -c emerge -l noconfmem -d "Disregard merge records"
complete -c emerge -s O -l nodeps -d "Don't merge dependencies"
complete -c emerge -s n -l noreplace -d "Skip already installed packages"
complete -c emerge -l nospinner -d "Disable the spinner"
complete -c emerge -s 1 -l oneshot -d "Don't add packages to world"
complete -c emerge -s o -l onlydeps -d "Only merge dependencies"
complete -c emerge -s p -l pretend -d "Display what would be done without doing it"
complete -c emerge -s q -l quit -d "Use a condensed output"
complete -c emerge -l skipfirst -d "Remove the first package in the resume list. Use with --resume"
complete -c emerge -s t -l tree -d "Show the dependency tree"
complete -c emerge -s k -l usepkg -d "Use binary package if available"
complete -c emerge -s K -l usepkgonly -d "Only use binary packages"
complete -c emerge -s v -l verbose -d "Run  in  verbose  mode"
complete -c emerge -l with-bdeps -xa 'y\t"'(_ "Pull in build time dependencies")'" n\t"'(_ "Don't pull in build time dependencies")'"' -d "Toggle build time dependencies"
# END Options
#########################


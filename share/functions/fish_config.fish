function fish_config --description "Launch fish's web based configuration"
	# Support passing an initial tab like "colors" or "functions"
	set -l initial_tab
	if test (count $argv) > 0
		set initial_tab $argv[1]
	end
	eval $__fish_datadir/tools/web_config/webconfig.py $initial_tab
end

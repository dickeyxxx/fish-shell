
function __trap_translate_signal
	set upper (echo $argv[1]|tr a-z A-Z)
	if expr $upper : 'SIG.*' >/dev/null
		echo $upper | cut -c 4-
	else
		echo $upper
	end
end

function __trap_switch

	switch $argv[1]
		case EXIT
			echo --on-exit %self
		
		case '*'
			echo --on-signal $argv[1]
	end	

end

function trap -d 'Perform an action when the shell recives a signal'

	set -l mode
	set -l cmd 
	set -l sig 
	set -l shortopt
	set -l longopt

	set -l shortopt -o lph
	set -l longopt
	if not getopt -T >/dev/null
		set longopt -l print,help,list-signals
	end

	if not getopt -n type -Q $shortopt $longopt -- $argv
		return 1
	end

	set -l tmp (getopt $shortopt $longopt -- $argv)

	eval set opt $tmp

	while count $opt >/dev/null
		switch $opt[1]
			case -h --help
				__fish_print_help trap
				return 0
			
			case -p --print
				set mode print
			
			case -l --list-signals
				set mode list
			
			case --
				 set -e opt[1]
				 break

		end
		set -e opt[1]
	end

	if not count $mode >/dev/null

		switch (count $opt)

			case 0
				set mode print

			case 1
				set mode clear

			case '*'
				if test opt[1] = -
					set -e opt[1]
					set mode clear
				else
					set mode set
				end
		end
	end

	switch $mode
		case clear
			for i in $opt
				set -- sig (__trap_translate_signal $i)
				if test $sig
					functions -e __trap_handler_$sig				
				end
			end

		case set
			set -l cmd $opt[1]
			set -e opt[1]

			for i in $opt

				set -l -- sig (__trap_translate_signal $i)
				set -- sw (__trap_switch $sig)

				if test $sig
					eval "function __trap_handler_$sig $sw; $cmd; end"
				else
					return 1
				end
			end

		case print
			set -l names 

			if count $opt >/dev/null
				set -- names $opt
			else
				set -- names (functions -na|grep "^__trap_handler_"|sed -e 's/__trap_handler_//' )
			end

			for i in $names

				set -- sig (__trap_translate_signal $i)

				if test sig
					functions __trap_handler_$i
				else
					return 1
				end

			end

		case list
			kill -l
			
	end

end

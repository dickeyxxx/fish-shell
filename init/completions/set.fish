
complete -y set_color

complete -c set -s e -l erase -d (_ "Erase variable")
complete -c set -s x -l export -d (_ "Export variable to subprocess")
complete -c set -s u -l unexport -d (_ "Do not export variable to subprocess")
complete -c set -s g -l global -d (_ "Make variable scope global")
complete -c set -s l -l local -d (_ "Make variable scope local")
complete -c set -s U -l universal -d (_ "Make variable scope universal, i.e. share variable with all the users fish processes on this computer")
complete -c set -s q -l query -d (_ "Test if variable is defined")
complete -c set -s h -l help -d (_ "Display help and exit")

function __fish_set_is_first -d 'Test if no non-switch argument has been specified yet'
	set -- cmd (commandline -poc)
	set -e -- cmd[1]
	for i in $cmd
		switch $i
			case '-*'

			case '*'
				return 1;
		end
	end
	return 0
end

complete -c set -n '__fish_set_is_first' -x -a "(set|sed -e 's/ /\tVariable: /')"

function __fish_set_is_color -d 'Test if We are specifying a color value for the prompt'
	set -- cmd (commandline -poc)
	set -e -- cmd[1]
	for i in $cmd
		switch $i

			case 'fish_color_*' 'fish_pager_color_*'
				return 0

			case '-*'
				 
			case '*'
				 return 1
		end
	end	
end

complete -c set -n '__fish_set_is_color' -x -a '(set_color --print-colors)' -d (_ Color)

#
# This function deletes a character from the commandline if it is
# non-empty, and exits the shell otherwise. Implementing this
# functionality has been a longstanding request from various
# fish-users.
#

function delete-or-exit --key-binding 
	if test (commandline)
		commandline -f delete-char
	else
		exit
	end
end


function __fish_tmux_sessions --description 'available sessions'
        tmux list-sessions -F "#S	#{session_windows} windows Created: #{session_created_string} [#{session_width}x#{session_height}] Attached=#{session_attached}" ^/dev/null
end

function __fish_tmux_clients --description 'connected clients'
        tmux list-clients -F "#{client_tty}	#{session_name}: Created: #{client_created_string} [#{client_width}x#{client_height} #{client_termname}]" ^/dev/null
end

function __fish_tmux_panes --description 'window panes'
        set -l panes (tmux list-panes -F "#S:#{window_name}.")

        #fully qualified pane names
        for i in (seq (count $panes))
                echo "$panes[$i]"(math $i - 1)'	session:window.pane'
        end

        #panes by themselves
        for i in (seq (count $panes))
                echo (math $i - 1)'	pane'
        end

        #windows by themselves
        tmux list-panes -F '#{window_name}	window'
end

#don't allow dirs in the completion list...
complete -c tmux -x

###############  Begin: Front  Flags ###############
#these do not require parameters
complete -c tmux -n '__fish_use_subcommand' -s 2 -d 'Force tmux to assume the terminal supports 256 colours'
complete -c tmux -n '__fish_use_subcommand' -s 8 -d 'Like -2, but indicates that the terminal supports 88 colours'
complete -c tmux -n '__fish_use_subcommand' -s l    -d 'Behave as a login shell'
complete -c tmux -n '__fish_use_subcommand' -s q    -d 'Set the quiet server option'
complete -c tmux -n '__fish_use_subcommand' -s u    -d 'Flag explicitly informs tmux that UTF-8 is supported'
complete -c tmux -n '__fish_use_subcommand' -s v    -d 'Request verbose logging'
complete -c tmux -n '__fish_use_subcommand' -s V    -d 'Report the tmux version'

#these require parameters
complete -c tmux -n '__fish_use_subcommand' -xs c   -d 'Execute command using the default shell'
complete -c tmux -n '__fish_use_subcommand' -rs f   -d 'Alternate config file'
complete -c tmux -n '__fish_use_subcommand' -rs L   -d 'Specify the name of the server socket to use'
complete -c tmux -n '__fish_use_subcommand' -rs S   -d 'Full path to sever socket. If set, -L is ignored.'
###############  End:   Front  Flags ###############

###############  Begin: Clients and Sessions ###############
set -l attach 'attach-session attach'
set -l detach 'detach-client detach'
set -l has 'has-session has'
set -l killserver 'kill-server'
set -l killsession 'kill-session'
set -l lsc 'list-clients lsc'
set -l lscm 'list-commands lscm'
set -l ls 'list-sessions ls'
set -l lockc 'lock-client lockc'
set -l locks 'lock-session locks'
set -l new 'new-session new'
set -l refresh 'refresh-client refresh'
set -l rename 'rename-session rename'
set -l showmsgs 'show-messages showmsgs'
set -l source 'source-file source'
set -l start 'start-server start'
set -l suspendc 'suspend-client suspendc'
set -l switchc 'switch-client switchc'

complete -c tmux -n '__fish_use_subcommand' -a $attach -d 'attach to existing session'
complete -c tmux -n "__fish_seen_subcommand_from $attach" -s d -d 'detach other clients'
complete -c tmux -n "__fish_seen_subcommand_from $attach" -s r -d 'attach in read-only mode'

complete -c tmux -n '__fish_use_subcommand' -a $detach -d 'detach current client'
complete -c tmux -n "__fish_seen_subcommand_from $detach" -s P -d 'SIGHUP parent process of client, likely causing it to exit'

complete -c tmux -n '__fish_use_subcommand' -a $has -d 'report error and exit with 1 if the specified session does not exist'
complete -c tmux -n '__fish_use_subcommand' -a $killserver -d 'kill tmux server, clients, and sessions'
complete -c tmux -n '__fish_use_subcommand' -a $killsession -d 'destroy session, closing windows linked to it and no other sessions, detach all clients'
complete -c tmux -n '__fish_use_subcommand' -a $lsc -d 'list all attached clients'
complete -c tmux -n '__fish_use_subcommand' -a $lscm -d 'list syntax for all tmux commands'
complete -c tmux -n '__fish_use_subcommand' -a $ls -d 'list all sessions'
complete -c tmux -n '__fish_use_subcommand' -a $lockc -d 'lock client'
complete -c tmux -n '__fish_use_subcommand' -a $locks -d 'lock session'

complete -c tmux -n '__fish_use_subcommand' -a $new -d 'create a new session with name session-name'
complete -c tmux -n "__fish_seen_subcommand_from $new" -s d -d "don't attach to current window"
complete -c tmux -n "__fish_seen_subcommand_from $new" -xs n -d 'window-name'
complete -c tmux -n "__fish_seen_subcommand_from $new" -xs s -d 'session-name'
complete -c tmux -n "__fish_seen_subcommand_from $new" -xs x -d 'width'
complete -c tmux -n "__fish_seen_subcommand_from $new" -xs y -d 'height'

complete -c tmux -n '__fish_use_subcommand' -a $refresh -d 'refresh client'
complete -c tmux -n "__fish_seen_subcommand_from $refresh" -s S -d 'update client status bar'

complete -c tmux -n '__fish_use_subcommand' -a $rename -d 'rename session'
complete -c tmux -n '__fish_use_subcommand' -a $showmsgs -d 'save msgs in status bar in per-client msg log'

complete -c tmux -n '__fish_use_subcommand' -a $source -d 'execute commands from path'
complete -c tmux -n '__fish_use_subcommand' -a $start -d 'start tmux server if not running; do not create a session'

complete -c tmux -n '__fish_use_subcommand' -a $suspendc -d 'send SIGTSTP signal to client (tty stop)'

complete -c tmux -n '__fish_use_subcommand' -a $switchc -d 'Switch the current session for client target-client to target-session'
complete -c tmux -n "__fish_seen_subcommand_from $switchc" -s l -d 'move client to the last session'
complete -c tmux -n "__fish_seen_subcommand_from $switchc" -s n -d 'move client to the next session'
complete -c tmux -n "__fish_seen_subcommand_from $switchc" -s p -d 'move client to the previous session'
complete -c tmux -n "__fish_seen_subcommand_from $switchc" -s r -d 'toggle if client is read-only'

#commands with a session flag
complete -c tmux -xs t -n "__fish_seen_subcommand_from $attach $new $has $killsession $lsc $locks $rename $switchc" -a '(__fish_tmux_sessions)' -d 'target-session'
complete -c tmux -xs s -n "__fish_seen_subcommand_from $detach" -a '(__fish_tmux_sessions)' -d 'target-session'

#commands with a client flag
complete -c tmux -xs t -n "__fish_seen_subcommand_from $detach $lockc $refresh $showmsgs $suspendc" -a '(__fish_tmux_clients)' -d 'target-client'
complete -c tmux -xs c -n "__fish_seen_subcommand_from $switchc" -a '(__fish_tmux_clients)' -d 'target-client'

#commands with the -F format flag
complete -c tmux -n "__fish_seen_subcommand_from $lsc $ls" -rs F -d 'format string'

###############  End: Clients and Sessions ###############

###############  Begin: Windows and Panes ###############
#TODO - these commands are not currently implemented.
#there is a section in the tmux man page that has the same title as this section
#use the "Clients and Sessions" code as an example when implementing this
###############  End:   Windows and Panes ###############

###############  Begin: Key Bindings ###############
set -l bind 'bind-key bind'
set -l lsk 'list-keys lsk'
set -l send 'send-keys send'
set -l sendprefix 'send-prefix'
set -l unbind 'unbind-key unbind'

set -l key_table 'vi-edit emacs-edit vi-choice emacs-choice vi-copy emacs-copy'

complete -c tmux -n '__fish_use_subcommand' -a $bind -d 'bind key to command'
complete -c tmux -n "__fish_seen_subcommand_from $bind" -s c -d 'bind for command mode instead of normal mode'
complete -c tmux -n "__fish_seen_subcommand_from $bind" -s n -d 'make the binding work without using a prefix key'
complete -c tmux -n "__fish_seen_subcommand_from $bind" -s r -d 'key may repeat'
complete -c tmux -n "__fish_seen_subcommand_from $bind" -xs t -d 'choose key table for binding' -xa "$key_table"

complete -c tmux -n '__fish_use_subcommand' -a $lsk -d 'list all key bindings'
complete -c tmux -n "__fish_seen_subcommand_from $lsk" -s t -d 'key table' -xa "$key_table"

complete -c tmux -n '__fish_use_subcommand' -a $send -d 'list all key bindings'
complete -c tmux -n "__fish_seen_subcommand_from $send" -s R -d 'reset terminal state'
complete -c tmux -n "__fish_seen_subcommand_from $send" -xs t -a '(__fish_tmux_panes)' -d 'target pane'

complete -c tmux -n '__fish_use_subcommand' -a $sendprefix -d 'send the prefix key'
complete -c tmux -n "__fish_seen_subcommand_from $sendprefix" -s 2 -d 'use secondary prefix'
complete -c tmux -n "__fish_seen_subcommand_from $sendprefix" -xs t -a '(__fish_tmux_panes)' -d 'target pane'

complete -c tmux -n '__fish_use_subcommand' -a $unbind -d 'unbind the command bound to key'
complete -c tmux -n "__fish_seen_subcommand_from $unbind" -s a -d 'remove all key bindings'
complete -c tmux -n "__fish_seen_subcommand_from $unbind" -s c -d 'binding for command mode'
complete -c tmux -n "__fish_seen_subcommand_from $unbind" -s n -d 'command bound to key without a prefix (if any) removed'
complete -c tmux -n "__fish_seen_subcommand_from $unbind" -xs t -d 'key table' -xa "$key_table"

###############  End:   Key Bindings ###############

###############  Begin: Options ###############
#TODO - these commands are not currently implemented.
#there is a section in the tmux man page that has the same title as this section
#use the "Clients and Sessions" code as an example when implementing this
###############  End:   Options ###############

###############  Begin: Environment ###############
#TODO - these commands are not currently implemented.
#there is a section in the tmux man page that has the same title as this section
#use the "Clients and Sessions" code as an example when implementing this
###############  End:   Environment ###############

###############  Begin: Status Line ###############
#TODO - these commands are not currently implemented.
#there is a section in the tmux man page that has the same title as this section
#use the "Clients and Sessions" code as an example when implementing this
###############  End:   Status Line ###############

###############  Begin: Buffers ###############
#TODO - these commands are not currently implemented.
#there is a section in the tmux man page that has the same title as this section
#use the "Clients and Sessions" code as an example when implementing this
###############  End:   Buffers ###############

###############  Begin: Miscellaneous ###############
#TODO - these commands are not currently implemented.
#there is a section in the tmux man page that has the same title as this section
#use the "Clients and Sessions" code as an example when implementing this
###############  End:   Miscellaneous ###############

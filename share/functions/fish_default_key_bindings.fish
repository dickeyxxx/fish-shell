
function fish_default_key_bindings -d "Default (Emacs-like) key bindings for fish"
	if not set -q argv[1]
		if test "$fish_key_bindings" != "fish_default_key_bindings"
			# Allow the user to set the variable universally
			set -q fish_key_bindings; or set -g fish_key_bindings
			set fish_key_bindings fish_default_key_bindings # This triggers the handler, which calls us again and ensures the user_key_bindings are executed
			return
		end
		# Clear earlier bindings, if any
		bind --erase --all
	end

    # These are shell-specific bindings that we share with vi mode.
    __fish_shared_key_bindings

	# This is the default binding, i.e. the one used if no other binding matches
	bind $argv "" self-insert

	bind $argv \n execute
	bind $argv \r execute

	bind $argv \ck kill-line

	bind $argv \eOC forward-char
	bind $argv \eOD backward-char
	bind $argv \e\[C forward-char
	bind $argv \e\[D backward-char
	bind $argv -k right forward-char
	bind $argv -k left backward-char

	bind $argv -k dc delete-char
	bind $argv -k backspace backward-delete-char
	bind $argv \x7f backward-delete-char

	# for PuTTY
	# https://github.com/fish-shell/fish-shell/issues/180
	bind $argv \e\[1~ beginning-of-line
	bind $argv \e\[3~ delete-char
	bind $argv \e\[4~ end-of-line

	# OS X SnowLeopard doesn't have these keys. Don't show an annoying error message.
	bind $argv -k home beginning-of-line 2> /dev/null
	bind $argv -k end end-of-line 2> /dev/null
	bind $argv \e\[3\;2~ backward-delete-char # Mavericks Terminal.app shift-delete

	bind $argv \ca beginning-of-line
	bind $argv \ce end-of-line
	bind $argv \ch backward-delete-char
	bind $argv \cp up-or-search
	bind $argv \cn down-or-search
	bind $argv \cf forward-char
	bind $argv \cb backward-char
	bind $argv \ct transpose-chars
	bind $argv \et transpose-words
	bind $argv \eu upcase-word

	# This clashes with __fish_list_current_token
	# bind $argv \el downcase-word
	bind $argv \ec capitalize-word
	bind $argv \e\x7f backward-kill-word
	bind $argv \eb backward-word
	bind $argv \ef forward-word
	bind $argv \e\[1\;5C forward-word
	bind $argv \e\[1\;5D backward-word
	bind $argv -k ppage beginning-of-history
	bind $argv -k npage end-of-history
	bind $argv \e\< beginning-of-buffer
	bind $argv \e\> end-of-buffer

	bind \ed forward-kill-word
	bind \ed kill-word

	# escape cancels stuff	
	bind \e cancel

	# Ignore some known-bad control sequences
	# https://github.com/fish-shell/fish-shell/issues/1917
	bind \e\[I 'begin;end'
	bind \e\[O 'begin;end'

	# term-specific special bindings
	switch "$TERM"
		case 'rxvt*'
			bind $argv \e\[8~ end-of-line
			bind $argv \eOc forward-word
			bind $argv \eOd backward-word
	end
end

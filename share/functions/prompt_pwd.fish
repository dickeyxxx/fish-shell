function prompt_pwd --description "Print the current working directory, shortened to fit the prompt"
    set -q argv[1]
    and switch $argv[1]
        case -h --help
            __fish_print_help prompt_pwd
            return 0
    end
    if set -q $__is_launched_without_string
        # There is no `string' builtin - do something sensible. XXX nostring
        echo $PWD
        return 0
    end

    # This allows overriding fish_prompt_pwd_dir_length from the outside (global or universal) without leaking it
    set -q fish_prompt_pwd_dir_length
    or set -l fish_prompt_pwd_dir_length 1

    # Replace $HOME with "~"
    set realhome ~
    set -l tmp (string replace -r '^'"$realhome"'($|/)' '~$1' $PWD)

    if [ $fish_prompt_pwd_dir_length -eq 0 ]
        echo $tmp
    else
        # Shorten to at most $fish_prompt_pwd_dir_length characters per directory
        string replace -ar '(\.?[^/]{'"$fish_prompt_pwd_dir_length"'})[^/]*/' '$1/' $tmp
    end
end

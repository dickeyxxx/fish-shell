/** \file highlight.c
	Functions for syntax highlighting
*/
#include "config.h"

#include <stdlib.h>
#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
#include <wchar.h>
#include <wctype.h>
#include <termios.h>
#include <signal.h>

#include "fallback.h"
#include "util.h"

#include "wutil.h"
#include "highlight.h"
#include "tokenizer.h"
#include "proc.h"
#include "parser.h"
#include "parse_util.h"
#include "parser_keywords.h"
#include "builtin.h"
#include "function.h"
#include "env.h"
#include "expand.h"
#include "sanity.h"
#include "common.h"
#include "complete.h"
#include "output.h"
#include "wildcard.h"
#include "path.h"

/**
   Number of elements in the highlight_var array
*/
#define VAR_COUNT ( sizeof(highlight_var)/sizeof(wchar_t *) )

static void highlight_universal_internal( const wchar_t * buff,
					  int *color, 
					  int pos );

/**
   The environment variables used to specify the color of different tokens.
*/
static const wchar_t * const highlight_var[] = 
{
	L"fish_color_normal",
	L"fish_color_error",
	L"fish_color_command",
	L"fish_color_end",
	L"fish_color_param",
	L"fish_color_comment",
	L"fish_color_match",
	L"fish_color_search_match",
	L"fish_color_operator",
	L"fish_color_escape",
	L"fish_color_quote",
	L"fish_color_redirection",
	L"fish_color_valid_path",
    L"fish_color_autosuggestion"
};

/**
   Tests if the specified string is the prefix of any valid path in the system. 
   
   \require_dir Whether the valid path must be a directory
   \return zero it this is not a valid prefix, non-zero otherwise
*/
// PCA DOES_IO
static bool is_potential_path( const wcstring &cpath, bool require_dir = false )
{
    ASSERT_IS_BACKGROUND_THREAD();
    
	const wchar_t *unescaped, *in;
    wcstring cleaned_path;
	int has_magic = 0;
	bool res = false;
    
    wcstring path(cpath);
    expand_tilde(path);
    if (! unescape_string(path, 1))
        return false;
    
    unescaped = path.c_str();
    
    //	debug( 1, L"%ls -> %ls ->%ls", path, tilde, unescaped );
    
    for( in = unescaped; *in; in++ )
    {
        switch( *in )
        {
            case PROCESS_EXPAND:
            case VARIABLE_EXPAND:
            case VARIABLE_EXPAND_SINGLE:
            case BRACKET_BEGIN:
            case BRACKET_END:
            case BRACKET_SEP:
            case ANY_CHAR:
            case ANY_STRING:
            case ANY_STRING_RECURSIVE:
            {
                has_magic = 1;
                break;		
            }
				
            case INTERNAL_SEPARATOR:
            {
                break;
            }
				
            default:
            {
                cleaned_path += *in;
                break;
            }
				
        }
        
    }
    
    if( !has_magic && ! cleaned_path.empty() )
    {
        bool must_be_full_dir = cleaned_path[cleaned_path.length()-1] == L'/';
        DIR *dir;
        if( must_be_full_dir )
        {
            dir = wopendir( cleaned_path );
            res = !!dir;
            if( dir )
            {
                closedir( dir );
            }
        }
        else
        {
            wcstring dir_name = wdirname(cleaned_path);
            wcstring base_name = wbasename(cleaned_path);
            
            if( dir_name == L"/" && base_name == L"/" )
            {
                res = true;
            }
            else if( (dir = wopendir( dir_name)) )
            {
                wcstring ent;
                bool is_dir;
                while (wreaddir(dir, ent, &is_dir))
                {
                    if (string_prefixes_string(base_name, ent) && (! require_dir || is_dir))
                    {
                        res = true;
                        break;
                    }
                }
                
                closedir( dir );
            }
        }
        
    }
	
	return res;
	
}

rgb_color_t highlight_get_color( int highlight, bool is_background )
{
	size_t i;
	int idx=0;
	rgb_color_t result;

	if( highlight < 0 )
		return rgb_color_t::normal();
	if( highlight > (1<<VAR_COUNT) )
		return rgb_color_t::normal();
	for( i=0; i<VAR_COUNT; i++ )
	{
		if( highlight & (1<<i ))
		{
			idx = i;
			break;
		}
	}
    
	env_var_t val_wstr = env_get_string( highlight_var[idx]); 

//	debug( 1, L"%d -> %d -> %ls", highlight, idx, val );	
	
	if (val_wstr.missing())
		val_wstr = env_get_string( highlight_var[0]);
	
	if( ! val_wstr.missing() )
		result = parse_color( val_wstr, is_background );
	
	if( highlight & HIGHLIGHT_VALID_PATH )
	{
		env_var_t val2_wstr =  env_get_string( L"fish_color_valid_path" );
		const wcstring val2 = val2_wstr.missing() ? L"" : val2_wstr.c_str(); 

		rgb_color_t result2 = parse_color( val2, is_background );
		if( result.is_normal() )
			result = result2;
		else 
		{
			if( result2.is_bold() )
				result.set_bold(true);
			if( result2.is_underline() )
				result.set_underline(true);
		}
	}
	return result;
}


/**
   Highligt operators (such as $, ~, %, as well as escaped characters.
*/
static void highlight_param( const wchar_t * buff,
							 int *color,
							 int pos,
							 wcstring_list_t *error )
{
	
	
	int mode = 0; 
	int in_pos, len = wcslen( buff );
	int bracket_count=0;
	wchar_t c;
	int normal_status = *color;
	
	for( in_pos=0;
		 in_pos<len; 
		 in_pos++ )
	{
		c = buff[in_pos];
		switch( mode )
		{

			/*
			  Mode 0 means unquoted string
			*/
			case 0:
			{
				if( c == L'\\' )
				{
					int start_pos = in_pos;
					in_pos++;
					
					if( wcschr( L"~%", buff[in_pos] ) )
					{
						if( in_pos == 1 )
						{
							color[start_pos] = HIGHLIGHT_ESCAPE;
							color[in_pos+1] = normal_status;
						}
					}
					else if( buff[in_pos]==L',' )
					{
						if( bracket_count )
						{
							color[start_pos] = HIGHLIGHT_ESCAPE;
							color[in_pos+1] = normal_status;
						}
					}
					else if( wcschr( L"abefnrtv*?$(){}[]'\"<>^ \\#;|&", buff[in_pos] ) )
					{
						color[start_pos]=HIGHLIGHT_ESCAPE;
						color[in_pos+1]=normal_status;
					}
					else if( wcschr( L"c", buff[in_pos] ) )
						{
						color[start_pos]=HIGHLIGHT_ESCAPE;
						color[in_pos+2]=normal_status;
					}
					else if( wcschr( L"uUxX01234567", buff[in_pos] ) )
					{
						int i;
						long long res=0;
						int chars=2;
						int base=16;
						
						int byte = 0;
						wchar_t max_val = ASCII_MAX;
						
						switch( buff[in_pos] )
						{
							case L'u':
							{
								chars=4;
								max_val = UCS2_MAX;
								break;
							}
							
							case L'U':
							{
								chars=8;
								max_val = WCHAR_MAX;
								break;
							}
							
							case L'x':
							{
								break;
							}
							
							case L'X':
							{
								byte=1;
								max_val = BYTE_MAX;
								break;
							}
							
							default:
							{
								base=8;
								chars=3;
								in_pos--;
								break;
							}								
						}
						
						for( i=0; i<chars; i++ )
						{
							int d = convert_digit( buff[++in_pos],base);
							
							if( d < 0 )
							{
								in_pos--;
								break;
							}
							
							res=(res*base)|d;
						}

						if( (res <= max_val) )
						{
							color[start_pos] = HIGHLIGHT_ESCAPE;
							color[in_pos+1] = normal_status;								
						}
						else
						{	
							color[start_pos] = HIGHLIGHT_ERROR;
							color[in_pos+1] = normal_status;								
						}
					}

				}
				else 
				{
					switch( buff[in_pos]){
						case L'~':
						case L'%':
						{
							if( in_pos == 0 )
							{
								color[in_pos] = HIGHLIGHT_OPERATOR;
								color[in_pos+1] = normal_status;
							}
							break;
						}

						case L'$':
						{
							wchar_t n = buff[in_pos+1];							
							color[in_pos] = (n==L'$'||wcsvarchr(n))? HIGHLIGHT_OPERATOR:HIGHLIGHT_ERROR;
							color[in_pos+1] = normal_status;								
							break;
						}


						case L'*':
						case L'?':
						case L'(':
						case L')':
						{
							color[in_pos] = HIGHLIGHT_OPERATOR;
							color[in_pos+1] = normal_status;
							break;
						}
						
						case L'{':
						{
							color[in_pos] = HIGHLIGHT_OPERATOR;
							color[in_pos+1] = normal_status;
							bracket_count++;
							break;					
						}
						
						case L'}':
						{
							color[in_pos] = HIGHLIGHT_OPERATOR;
							color[in_pos+1] = normal_status;
							bracket_count--;
							break;						
						}

						case L',':
						{
							if( bracket_count )
							{
								color[in_pos] = HIGHLIGHT_OPERATOR;
								color[in_pos+1] = normal_status;
							}

							break;					
						}
						
						case L'\'':
						{
							color[in_pos] = HIGHLIGHT_QUOTE;
							mode = 1;
							break;					
						}
				
						case L'\"':
						{
							color[in_pos] = HIGHLIGHT_QUOTE;
							mode = 2;
							break;
						}

					}
				}		
				break;
			}

			/*
			  Mode 1 means single quoted string, i.e 'foo'
			*/
			case 1:
			{
				if( c == L'\\' )
				{
					int start_pos = in_pos;
					switch( buff[++in_pos] )
					{
						case '\\':
						case L'\'':
						{
							color[start_pos] = HIGHLIGHT_ESCAPE;
							color[in_pos+1] = HIGHLIGHT_QUOTE;
							break;
						}
						
						case 0:
						{
							return;
						}
						
					}
					
				}
				if( c == L'\'' )
				{
					mode = 0;
					color[in_pos+1] = normal_status;
				}
				
				break;
			}

			/*
			  Mode 2 means double quoted string, i.e. "foo"
			*/
			case 2:
			{
				switch( c )
				{
					case '"':
					{
						mode = 0;
						color[in_pos+1] = normal_status;
						break;
					}
				
					case '\\':
					{
						int start_pos = in_pos;
						switch( buff[++in_pos] )
						{
							case L'\0':
							{
								return;
							}
							
							case '\\':
							case L'$':
							case '"':
							{
								color[start_pos] = HIGHLIGHT_ESCAPE;
								color[in_pos+1] = HIGHLIGHT_QUOTE;
								break;
							}
						}
						break;
					}
					
					case '$':
					{
						wchar_t n = buff[in_pos+1];
						color[in_pos] = (n==L'$'||wcsvarchr(n))? HIGHLIGHT_OPERATOR:HIGHLIGHT_ERROR;
						color[in_pos+1] = HIGHLIGHT_QUOTE;								
						break;
					}
				
				}						
				break;
			}
		}
	}
}

static int has_expand_reserved( const wchar_t *str )
{
	while( *str )
	{
		if( *str >= EXPAND_RESERVED &&
		    *str <= EXPAND_RESERVED_END )
		{
			return 1;
		}
		str++;
	}
	return 0;
}

bool autosuggest_handle_special(const wcstring &str, const env_vars &vars, const wcstring &working_directory, bool *outSuggestionOK) {
    ASSERT_IS_BACKGROUND_THREAD();
    assert(outSuggestionOK != NULL);
    
    if (str.empty())
        return false;
    
    wcstring cmd;
    bool had_cmd = false;
    
    bool handled = false;
    bool suggestionOK = true;
    
    tokenizer tok;
	for( tok_init( &tok, str.c_str(), TOK_SQUASH_ERRORS );
		tok_has_next( &tok );
		tok_next( &tok ) )
	{	
        int last_type = tok_last_type( &tok );
		
		switch( last_type )
		{
			case TOK_STRING:
			{
				if( had_cmd )
				{
					if( cmd == L"cd" )
					{
                        wcstring dir = tok_last( &tok );
                        if (expand_one(dir, EXPAND_SKIP_CMDSUBST))
						{
                            /* We can specially handle the cd command */
                            handled = true;
							bool is_help = string_prefixes_string(dir, L"--help") || string_prefixes_string(dir, L"-h");
                            if (! is_help && ! path_can_get_cdpath(dir, working_directory.c_str())) {
                                suggestionOK = false;
                                break;
                            }
						}
					}
				}
				else
				{ 	
					/*
					 Command. First check that the command actually exists.
					 */
                    cmd = tok_last( &tok );
                    bool expanded = expand_one(cmd, EXPAND_SKIP_CMDSUBST | EXPAND_SKIP_VARIABLES);
					if (! expanded || has_expand_reserved(cmd.c_str()))
					{

					}
					else
					{
						int is_subcommand = 0;
						int mark = tok_get_pos( &tok );
						
						if( parser_keywords_is_subcommand( cmd ) )
						{
							
							int sw;
							
							if( cmd == L"builtin")
							{
							}
							else if( cmd == L"command")
							{
							}
							
							tok_next( &tok );
							
							sw = parser_keywords_is_switch( tok_last( &tok ) );
							
							if( !parser_keywords_is_block( cmd ) &&
							   sw == ARG_SWITCH )
							{

							}
							else
							{
								if( sw == ARG_SKIP )
								{
									mark = tok_get_pos( &tok );
								}
								
								is_subcommand = 1;
							}
							tok_set_pos( &tok, mark );
						}
						
						if( !is_subcommand )
						{	
							had_cmd = true;
						}
					}
					
				}
				break;
			}
				
			case TOK_REDIRECT_NOCLOB:
			case TOK_REDIRECT_OUT:
			case TOK_REDIRECT_IN:
			case TOK_REDIRECT_APPEND:
			case TOK_REDIRECT_FD:
			{
				if( !had_cmd )
				{
					break;
				}
				tok_next( &tok );				
                break;
			}
				
			case TOK_PIPE:
			case TOK_BACKGROUND:
			{
				had_cmd = false;
				break;
			}
				
			case TOK_END:
			{
				had_cmd = false;
				break;
			}
				
			case TOK_COMMENT:
			{
				break;
			}
				
			case TOK_ERROR:
			default:
			{
				break;				
			}			
		}
    }
    tok_destroy( &tok );
    
    *outSuggestionOK = suggestionOK;
    return handled;
}

// This function does I/O
static void tokenize( const wchar_t * const buff, int * const color, const int pos, wcstring_list_t *error, const env_vars &vars) {
    ASSERT_IS_BACKGROUND_THREAD();

	wcstring cmd;    
	int had_cmd=0;
	int i;
	wcstring last_cmd;
	int len;

	int accept_switches = 1;
	
	int use_function = 1;
	int use_command = 1;
	int use_builtin = 1;
	
	CHECK( buff, );
	CHECK( color, );

	len = wcslen(buff);

	if( !len )
		return;

	for( i=0; buff[i] != 0; i++ )
		color[i] = -1;

    tokenizer tok;
	for( tok_init( &tok, buff, TOK_SHOW_COMMENTS | TOK_SQUASH_ERRORS );
		tok_has_next( &tok );
		tok_next( &tok ) )
	{	
		int last_type = tok_last_type( &tok );
		
		switch( last_type )
		{
			case TOK_STRING:
			{
				if( had_cmd )
				{
					
					/*Parameter */
					wchar_t *param = tok_last( &tok );
					if( param[0] == L'-' )
					{
						if (wcscmp( param, L"--" ) == 0 )
						{
							accept_switches = 0;
							color[ tok_get_pos( &tok ) ] = HIGHLIGHT_PARAM;
						}
						else if( accept_switches )
						{
							if( complete_is_valid_option( last_cmd.c_str(), param, error, false /* no autoload */ ) )
								color[ tok_get_pos( &tok ) ] = HIGHLIGHT_PARAM;
							else
								color[ tok_get_pos( &tok ) ] = HIGHLIGHT_ERROR;
						}
						else
						{
							color[ tok_get_pos( &tok ) ] = HIGHLIGHT_PARAM;
						}
					}
					else
					{
						color[ tok_get_pos( &tok ) ] = HIGHLIGHT_PARAM;
					}					
					
					if( cmd == L"cd" )
					{
                        wcstring dir = tok_last( &tok );
                        if (expand_one(dir, EXPAND_SKIP_CMDSUBST))
						{
							int is_help = string_prefixes_string(dir, L"--help") || string_prefixes_string(dir, L"-h");
							if( !is_help && ! path_can_get_cdpath(dir))
							{
                                color[ tok_get_pos( &tok ) ] = HIGHLIGHT_ERROR;							
							}
						}
					}
					
					highlight_param( param,
									&color[tok_get_pos( &tok )],
									pos-tok_get_pos( &tok ), 
									error );
					
					
				}
				else
				{ 
					/*
					 Command. First check that the command actually exists.
					 */
                    cmd = tok_last( &tok );
                    bool expanded = expand_one(cmd, EXPAND_SKIP_CMDSUBST | EXPAND_SKIP_VARIABLES);
					if (! expanded || has_expand_reserved(cmd.c_str()))
					{
						color[ tok_get_pos( &tok ) ] = HIGHLIGHT_ERROR;
					}
					else
					{
						int is_cmd = 0;
						int is_subcommand = 0;
						int mark = tok_get_pos( &tok );
						color[ tok_get_pos( &tok ) ] = HIGHLIGHT_COMMAND;
						
						if( parser_keywords_is_subcommand( cmd ) )
						{
							
							int sw;
							
							if( cmd == L"builtin")
							{
								use_function = 0;
								use_command  = 0;
								use_builtin  = 1;
							}
							else if( cmd == L"command")
							{
								use_command  = 1;
								use_function = 0;
								use_builtin  = 0;
							}
							
							tok_next( &tok );
							
							sw = parser_keywords_is_switch( tok_last( &tok ) );
							
							if( !parser_keywords_is_block( cmd ) &&
							   sw == ARG_SWITCH )
							{
								/* 
								 The 'builtin' and 'command' builtins
								 are normally followed by another
								 command, but if they are invoked
								 with a switch, they aren't.
								 
								 */
								use_command  = 1;
								use_function = 1;
								use_builtin  = 2;
							}
							else
							{
								if( sw == ARG_SKIP )
								{
									color[ tok_get_pos( &tok ) ] = HIGHLIGHT_PARAM;
									mark = tok_get_pos( &tok );
								}
								
								is_subcommand = 1;
							}
							tok_set_pos( &tok, mark );
						}
						
						if( !is_subcommand )
						{
							/*
							 OK, this is a command, it has been
							 successfully expanded and everything
							 looks ok. Lets check if the command
							 exists.
							 */
							
							/*
							 First check if it is a builtin or
							 function, since we don't have to stat
							 any files for that
							 */
							if( use_builtin )
								is_cmd |= builtin_exists( cmd );
							
							if( use_function )
								is_cmd |= function_exists_no_autoload( cmd.c_str(), vars );
							
							/*
							 Moving on to expensive tests
							 */
							
							/*
							 Check if this is a regular command
							 */
							if( use_command )
                            {
                                wcstring tmp;
								is_cmd |= !!path_get_path_string( cmd, tmp, vars );
                            }
							
							if( is_cmd )
							{								
								color[ tok_get_pos( &tok ) ] = HIGHLIGHT_COMMAND;
							}
							else
							{
								if( error ) {
                                    error->push_back(format_string(L"Unknown command \'%ls\'", cmd.c_str()));
                                }
								color[ tok_get_pos( &tok ) ] = (HIGHLIGHT_ERROR);
							}
							had_cmd = 1;
						}
						
						if( had_cmd )
						{
							last_cmd = tok_last( &tok );
						}
					}
					
				}
				break;
			}
				
			case TOK_REDIRECT_NOCLOB:
			case TOK_REDIRECT_OUT:
			case TOK_REDIRECT_IN:
			case TOK_REDIRECT_APPEND:
			case TOK_REDIRECT_FD:
			{
				if( !had_cmd )
				{
					color[ tok_get_pos( &tok ) ] = HIGHLIGHT_ERROR;
					if( error )
                        error->push_back(L"Redirection without a command");
					break;
				}
				
                wcstring target_str;
				const wchar_t *target=NULL;
				
				color[ tok_get_pos( &tok ) ] = HIGHLIGHT_REDIRECTION;
				tok_next( &tok );
				
				/*
				 Check that we are redirecting into a file
				 */
				
				switch( tok_last_type( &tok ) )
				{
					case TOK_STRING:
					{
                        target_str = tok_last( &tok );
                        if (expand_one(target_str, EXPAND_SKIP_CMDSUBST)) {
                            target = target_str.c_str();
                        }
						/*
						 Redirect filename may contain a cmdsubst. 
						 If so, it will be ignored/not flagged.
						 */
					}
						break;
					default:
					{
						color[ tok_get_pos( &tok ) ] = HIGHLIGHT_ERROR;
						if( error )
                            error->push_back(L"Invalid redirection");
					}
						
				}
				
				if( target != 0 )
				{
                    wcstring dir = target;
                    size_t slash_idx = dir.find_last_of(L'/');
					struct stat buff;
					/* 
					 If file is in directory other than '.', check
					 that the directory exists.
					 */
					if( slash_idx != wcstring::npos )
					{
						dir.resize(slash_idx);
						if( wstat( dir, &buff ) == -1 )
						{
							color[ tok_get_pos( &tok ) ] = HIGHLIGHT_ERROR;
							if( error )
                                error->push_back(format_string(L"Directory \'%ls\' does not exist", dir.c_str()));
							
						}
					}
					
					/*
					 If the file is read from or appended to, check
					 if it exists.
					 */
					if( last_type == TOK_REDIRECT_IN || 
					   last_type == TOK_REDIRECT_APPEND )
					{
						if( wstat( target, &buff ) == -1 )
						{
							color[ tok_get_pos( &tok ) ] = HIGHLIGHT_ERROR;
							if( error )
                                error->push_back(format_string(L"File \'%ls\' does not exist", target));
						}
					}
					if( last_type == TOK_REDIRECT_NOCLOB )
					{
						if( wstat( target, &buff ) != -1 )
						{
							color[ tok_get_pos( &tok ) ] = HIGHLIGHT_ERROR;
							if( error )
                                error->push_back(format_string(L"File \'%ls\' exists", target));
						}
					}
				}
				break;
			}
				
			case TOK_PIPE:
			case TOK_BACKGROUND:
			{
				if( had_cmd )
				{
					color[ tok_get_pos( &tok ) ] = HIGHLIGHT_END;
					had_cmd = 0;
					use_command  = 1;
					use_function = 1;
					use_builtin  = 1;
					accept_switches = 1;
				}
				else
				{
					color[ tok_get_pos( &tok ) ] = HIGHLIGHT_ERROR;					
					if( error )
                        error->push_back(L"No job to put in background" );
				}
				
				break;
			}
				
			case TOK_END:
			{
				color[ tok_get_pos( &tok ) ] = HIGHLIGHT_END;
				had_cmd = 0;
				use_command  = 1;
				use_function = 1;
				use_builtin  = 1;
				accept_switches = 1;
				break;
			}
				
			case TOK_COMMENT:
			{
				color[ tok_get_pos( &tok ) ] = HIGHLIGHT_COMMENT;
				break;
			}
				
			case TOK_ERROR:
			default:
			{
				/*
				 If the tokenizer reports an error, highlight it as such.
				 */
				if( error )
                    error->push_back(tok_last( &tok));
				color[ tok_get_pos( &tok ) ] = HIGHLIGHT_ERROR;
				break;				
			}			
		}
	}
    tok_destroy( &tok );
}


// PCA DOES_IO (calls is_potential_path, path_get_path, maybe others)
void highlight_shell( const wchar_t * const buff, int *color, int pos, wcstring_list_t *error, const env_vars &vars )
{
    ASSERT_IS_BACKGROUND_THREAD();
    
    int i;
    int len;
	int last_val;
	
	CHECK( buff, );
	CHECK( color, );
	
	len = wcslen(buff);
	
	if( !len )
		return;
	
	for( i=0; buff[i] != 0; i++ )
		color[i] = -1;

    /* Tokenize the string */
    tokenize(buff, color, pos, error, vars);

	/*
	  Locate and syntax highlight cmdsubsts recursively
	*/

	wchar_t * const subbuff = wcsdup(buff);
    wchar_t * subpos = subbuff;
	int done=0;
	
	while( 1 )
	{
		wchar_t *begin, *end;
	
		if( parse_util_locate_cmdsubst(subpos, &begin, &end, 1) <= 0)
		{
			break;
		}
		
		if( !*end )
			done=1;
		else
			*end=0;
		
		highlight_shell( begin+1, color + (begin-subbuff)+1, -1, error, vars );
		color[end-subbuff]=HIGHLIGHT_OPERATOR;
		
		if( done )
			break;
		
		subpos = end+1;
	}
    free(subbuff);

	/*
	  The highlighting code only changes the first element when the
	  color changes. This fills in the rest.
	*/
	last_val=0;
	for( i=0; buff[i] != 0; i++ )
	{
		if( color[i] >= 0 )
			last_val = color[i];
		else
			color[i] = last_val;
	}

	/*
	  Color potentially valid paths in a special path color if they
	  are the current token.
	*/

	if( pos >= 0 && pos <= len )
	{
		
		const wchar_t *tok_begin, *tok_end;
		parse_util_token_extent( buff, pos, &tok_begin, &tok_end, 0, 0 );
		if( tok_begin && tok_end )
		{
            const wcstring token(tok_begin, tok_end-tok_begin);
			if( is_potential_path( token ) )
			{
				for( i=tok_begin-buff; i < (tok_end-buff); i++ )
				{
                    // Don't color HIGHLIGHT_ERROR because it looks dorky. For example, trying to cd into a non-directory would show an underline and also red.
                    if (! (color[i] & HIGHLIGHT_ERROR))
                        color[i] |= HIGHLIGHT_VALID_PATH;
				}
			}
		}
	}
	

	highlight_universal_internal( buff, color, pos );

	/*
	  Spaces should not be highlighted at all, since it makes cursor look funky in some terminals
	*/
	for( i=0; buff[i]; i++ )
	{
		if( iswspace(buff[i]) )
		{
			color[i]=0;
		}
	}
}



/**
   Perform quote and parenthesis highlighting on the specified string.
*/
static void highlight_universal_internal( const wchar_t * buff,
										 int *color, 
										 int pos )
{	

	if( (pos >= 0) && ((size_t)pos < wcslen(buff)) )
	{
		
		/*
		  Highlight matching quotes
		*/
		if( (buff[pos] == L'\'') || (buff[pos] == L'\"') )
		{
			std::deque<long> lst;
		
			int level=0;
			wchar_t prev_q=0;
		
			const wchar_t *str=buff;

			int match_found=0;
		
			while(*str)
			{
				switch( *str )
				{
					case L'\\':
						str++;
						break;
					case L'\"':
					case L'\'':
						if( level == 0 )
						{
							level++;
                            lst.push_back((long)(str-buff));
							prev_q = *str;
						}
						else
						{
							if( prev_q == *str )
							{
								long pos1, pos2;
							
								level--;
                                pos1 = lst.back();
								pos2 = str-buff;
								if( pos1==pos || pos2==pos )
								{
									color[pos1]|=HIGHLIGHT_MATCH<<16;
									color[pos2]|=HIGHLIGHT_MATCH<<16;
									match_found = 1;
									
								}
								prev_q = *str==L'\"'?L'\'':L'\"';
							}
							else
							{
								level++;
                                lst.push_back((long)(str-buff));
								prev_q = *str;
							}
						}
					
						break;
				}
				if( (*str == L'\0'))
					break;

				str++;
			}
		
			if( !match_found )
				color[pos] = HIGHLIGHT_ERROR<<16;
		}

		/*
		  Highlight matching parenthesis
		*/
		if( wcschr( L"()[]{}", buff[pos] ) )
		{
			int step = wcschr(L"({[", buff[pos])?1:-1;
			wchar_t dec_char = *(wcschr( L"()[]{}", buff[pos] ) + step);
			wchar_t inc_char = buff[pos];
			int level = 0;
			const wchar_t *str = &buff[pos];
			int match_found=0;			

			while( (str >= buff) && *str)
			{
				if( *str == inc_char )
					level++;
				if( *str == dec_char )
					level--;
				if( level == 0 )
				{
					int pos2 = str-buff;
					color[pos]|=HIGHLIGHT_MATCH<<16;
					color[pos2]|=HIGHLIGHT_MATCH<<16;
					match_found=1;
					break;
				}
				str+= step;
			}
			
			if( !match_found )
				color[pos] = HIGHLIGHT_ERROR<<16;
		}
	}
}

void highlight_universal( const wchar_t *buff, int *color, int pos, wcstring_list_t *error, const env_vars &vars )
{
	int i;
	
	for( i=0; buff[i]; i++ )
		color[i] = 0;
	
	highlight_universal_internal( buff, color, pos );
}

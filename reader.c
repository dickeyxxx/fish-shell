/** \file reader.c

Functions for reading data from stdin and passing to the
parser. If stdin is a keyboard, it supplies a killring, history,
syntax highlighting, tab-completion and various other interactive features.

Internally the interactive mode functions rely in the functions of the
input library to read individual characters of input.

Token search is handled incrementally. Actual searches are only done
on when searching backwards, since the previous results are saved. The
last search position is remembered and a new search continues from the
last search position. All search results are saved in the list
'search_prev'. When the user searches forward, i.e. presses Alt-down,
the list is consulted for previous search result, and subsequent
backwards searches are also handled by consultiung the list up until
the end of the list is reached, at which point regular searching will
commence.

*/

#include "config.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <termios.h>
#include <sys/types.h>
#include <sys/stat.h>

#ifdef HAVE_SYS_TERMIOS_H
#include <sys/termios.h>
#endif

#ifdef HAVE_SYS_IOCTL_H
#include <sys/ioctl.h>
#endif

#include <time.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <sys/poll.h>
#include <unistd.h>
#include <wctype.h>

#if HAVE_NCURSES_H
#include <ncurses.h>
#else
#include <curses.h>
#endif

#if HAVE_TERMIO_H
#include <termio.h>
#endif

#if HAVE_TERM_H
#include <term.h>
#elif HAVE_NCURSES_TERM_H
#include <ncurses/term.h>
#endif

#ifdef HAVE_SIGINFO_H
#include <siginfo.h>
#endif

#ifdef HAVE_SYS_SELECT_H
#include <sys/select.h>
#endif

#include <signal.h>
#include <fcntl.h>
#include <dirent.h>
#include <wchar.h>

#include <assert.h>


#include "fallback.h"
#include "util.h"

#include "wutil.h"
#include "highlight.h"
#include "reader.h"
#include "proc.h"
#include "parser.h"
#include "complete.h"
#include "history.h"
#include "common.h"
#include "sanity.h"
#include "env.h"
#include "exec.h"
#include "expand.h"
#include "tokenizer.h"
#include "kill.h"
#include "input_common.h"
#include "input.h"
#include "function.h"
#include "output.h"
#include "signal.h"
#include "screen.h"

#include "parse_util.h"

/**
   Maximum length of prefix string when printing completion
   list. Longer prefixes will be ellipsized.
*/
#define PREFIX_MAX_LEN 8

/**
   A simple prompt for reading shell commands that does not rely on
   fish specific commands, meaning it will work even if fish is not
   installed. This is used by read_i.
*/
#define DEFAULT_PROMPT L"whoami; echo @; hostname|cut -d . -f 1; echo \" \"; pwd; printf '> ';"

#define PROMPT_FUNCTION_NAME L"fish_prompt"

/**
   The default title for the reader. This is used by reader_readline.
*/
#define DEFAULT_TITLE L"echo $_ \" \"; pwd"

/**
   The maximum number of characters to read from the keyboard without
   repainting. Note that this readahead will only occur if new
   characters are avaialble for reading, fish will never block for
   more input without repainting.
*/
#define READAHEAD_MAX 256

#define KILL_APPEND 0
#define KILL_PREPEND 1

/**
   A struct describing the state of the interactive reader. These
   states can be stacked, in case reader_readline() calls are
   nested. This happens when the 'read' builtin is used.
*/
typedef struct reader_data
{
	/**
	   Buffer containing the whole current commandline
	*/
	wchar_t *buff;

	screen_t screen;

	/**
	   Buffer containing the current search item
	*/
	wchar_t *search_buff;

	/**
	   Saved position used by token history search
	*/
	int token_history_pos;

	/**
	   Saved search string for token history search. Not handled by check_size.
	*/
	const wchar_t *token_history_buff;

	/**
	   List for storing previous search results. Used to avoid duplicates.
	*/
	array_list_t search_prev;

	/**
	   The current position in search_prev
	*/

	int search_pos;

	/**
	   Current size of the buffers
	*/
	size_t buff_sz;

	/**
	   Length of the command in buff. (Not the length of buff itself)
	*/

	size_t buff_len;

	/**
	   The current position of the cursor in buff.
	*/
	size_t buff_pos;

	/**
	   Name of the current application
	*/
	wchar_t *name;

	/** The prompt command */
	wchar_t *prompt;

	/** The output of the last evaluation of the prompt command */
	string_buffer_t prompt_buff;
	
	/**
	   Color is the syntax highlighting for buff.  The format is that
	   color[i] is the classification (according to the enum in
	   highlight.h) of buff[i].
	*/
	int *color;

	/**
	   An array defining the block level at each character.
	*/
	int *indent;

	/**
	   Function for tab completion
	*/
	void (*complete_func)( const wchar_t *,
						   array_list_t * );

	/**
	   Function for syntax highlighting
	*/
	void (*highlight_func)( wchar_t *,
							int *,
							int,
							array_list_t * );

	/**
	   Function for testing if the string can be returned
	*/
	int (*test_func)( wchar_t * );

	/**
	   When this is true, the reader will exit
	*/
	int end_loop;

	/**
	   If this is true, exit reader even if there are running
	   jobs. This happens if we press e.g. ^D twice.
	*/
	int prev_end_loop;

	string_buffer_t kill_item;

	/**
	   Pointer to previous reader_data
	*/
	struct reader_data *next;
}
	reader_data_t;

/**
   The current interactive reading context
*/
static reader_data_t *data=0;


/**
   Flag for ending non-interactive shell
*/
static int end_loop = 0;

/**
   The list containing names of files that are being parsed
*/
static array_list_t current_filename;


/**
   Store the pid of the parent process, so the exit function knows whether it should reset the terminal or not.
*/
static pid_t original_pid;

/**
   This variable is set to true by the signal handler when ^C is pressed
*/
static int interrupted=0;


/*
  Prototypes for a bunch of functions defined later on.
*/

/**
   Stores the previous termios mode so we can reset the modes when
   we execute programs and when the shell exits.
*/
static struct termios saved_modes;

static void reader_super_highlight_me_plenty( int pos, array_list_t *error );

/**
   Variable to keep track of forced exits - see \c reader_exit_forced();
*/
static int exit_forced;


/**
   Give up control of terminal
*/
static void term_donate()
{
	set_color(FISH_COLOR_NORMAL, FISH_COLOR_NORMAL);

	while( 1 )
	{
		if(	tcsetattr(0,TCSANOW,&saved_modes) )
		{
			if( errno != EINTR )
			{
				debug( 1, _( L"Could not set terminal mode for new job" ) );
				wperror( L"tcsetattr" );
				break;
			}
		}
		else
			break;
	}


}

/**
   Grab control of terminal
*/
static void term_steal()
{

	while( 1 )
	{
		if(	tcsetattr(0,TCSANOW,&shell_modes) )
		{
			if( errno != EINTR )
			{
				debug( 1, _( L"Could not set terminal mode for shell" ) );
				wperror( L"tcsetattr" );
				break;
			}
		}
		else
			break;
	}

	common_handle_winch(0 );

}

int reader_exit_forced()
{
	return exit_forced;
}

/**
   Internal helper function for handling killing parts of text.
*/
static void reader_kill( wchar_t *begin, int length, int mode, int new )
{
	if( new )
	{
		sb_clear( &data->kill_item );
		sb_append_substring( &data->kill_item, begin, length );
		kill_add( (wchar_t *)data->kill_item.buff );
	}
	else
	{

		wchar_t *old = wcsdup( (wchar_t *)data->kill_item.buff);

		if( mode == KILL_APPEND )
		{
			sb_append_substring( &data->kill_item, begin, length );
		}
		else
		{
			sb_clear( &data->kill_item );
			sb_append_substring( &data->kill_item, begin, length );
			sb_append( &data->kill_item, old );
		}

		
		kill_replace( old, (wchar_t *)data->kill_item.buff );
		free( old );
	}

	if( data->buff_pos > (begin-data->buff) )
	{
		data->buff_pos = maxi( begin-data->buff, data->buff_pos-length );
	}
	
	data->buff_len -= length;
	memmove( begin, begin+length, sizeof( wchar_t )*(wcslen( begin+length )+1) );
	
	reader_super_highlight_me_plenty( data->buff_pos, 0 );
	repaint();
	
}

void reader_handle_int( int sig )
{
	block_t *c = current_block;
	while( c )
	{
		c->skip=1;
		c=c->outer;
	}
	interrupted = 1;
	
}

wchar_t *reader_current_filename()
{
	return al_get_count( &current_filename )?(wchar_t *)al_peek( &current_filename ):0;
}


void reader_push_current_filename( const wchar_t *fn )
{
	al_push( &current_filename, fn );
}


wchar_t *reader_pop_current_filename()
{
	return (wchar_t *)al_pop( &current_filename );
}


/**
   Make sure buffers are large enough to hold current data plus one extra character.
*/
static int check_size()
{
	if( data->buff_sz < data->buff_len + 2 )
	{
		data->buff_sz = maxi( 128, data->buff_len*2 );

		data->buff = realloc( data->buff,
							  sizeof(wchar_t)*data->buff_sz);
		data->search_buff = realloc( data->search_buff,
									 sizeof(wchar_t)*data->buff_sz);

		data->color = realloc( data->color,
							   sizeof(int)*data->buff_sz);

		data->indent = realloc( data->indent,
								sizeof(int)*data->buff_sz);

		if( data->buff==0 ||
			data->search_buff==0 ||
			data->color==0 ||
			data->indent == 0 )
		{
			DIE_MEM();
		}
	}
	return 1;
}

/**
   Compare two completions, ignoring their description.
*/
static int fldcmp( wchar_t *a, wchar_t *b )
{
	while( 1 )
	{
		if( *a != *b )
			return *a-*b;
		if( ( (*a == COMPLETE_SEP) || (*a == L'\0') ) &&
			( (*b == COMPLETE_SEP) || (*b == L'\0') ) )
			return 0;
		a++;
		b++;
	}

}

/**
   Remove any duplicate completions in the list. This relies on the
   list first beeing sorted.
*/
static void remove_duplicates( array_list_t *l )
{
	int in, out;
	wchar_t *prev;
	if( al_get_count( l ) == 0 )
		return;

	prev = (wchar_t *)al_get( l, 0 );
	for( in=1, out=1; in < al_get_count( l ); in++ )
	{		
		wchar_t *curr = (wchar_t *)al_get( l, in );

		if( fldcmp( prev, curr )==0 )
		{
			free( curr );			
		}
		else
		{
			al_set( l, out++, curr );
			prev = curr;
		}
	}
	al_truncate( l, out );
}


int reader_interrupted()
{
	int res=interrupted;
	if( res )
		interrupted=0;
	return res;
}

void reader_write_title()
{
	wchar_t *title;
	array_list_t l;
	wchar_t *term = env_get( L"TERM" );

	/*
	  This is a pretty lame heuristic for detecting terminals that do
	  not support setting the title. If we recognise the terminal name
	  as that of a virtual terminal, we assume it supports setting the
	  title. Otherwise we check the ttyname.
	*/
	if( !term || !contains_str( term, L"xterm", L"screen", L"nxterm", L"rxvt", (wchar_t *)0 ) )
	{
		char *n = ttyname( STDIN_FILENO );
		if( strstr( n, "tty" ) || strstr( n, "/vc/") )
			return;
	}

	title = function_exists( L"fish_title" )?L"fish_title":DEFAULT_TITLE;

	if( wcslen( title ) ==0 )
		return;

	al_init( &l );

	proc_push_interactive(0);
	if( exec_subshell( title, &l ) != -1 )
	{
		int i;
		if( al_get_count( &l ) > 0 )
		{
			writestr( L"\e]2;" );
			for( i=0; i<al_get_count( &l ); i++ )
			{
				writestr( (wchar_t *)al_get( &l, i ) );
			}
			writestr( L"\7" );
		}
	}
	proc_pop_interactive();
		
	al_foreach( &l, &free );
	al_destroy( &l );
	set_color( FISH_COLOR_RESET, FISH_COLOR_RESET );
}

/**
   Reexecute the prompt command. The output is inserted into data->prompt_buff.
*/
static void exec_prompt()
{
	int i;

	array_list_t prompt_list;
	al_init( &prompt_list );
	
	if( data->prompt )
	{
		proc_push_interactive( 0 );
		
		if( exec_subshell( data->prompt, &prompt_list ) == -1 )
		{
			/* If executing the prompt fails, make sure we at least don't print any junk */
			al_foreach( &prompt_list, &free );
			al_destroy( &prompt_list );
			al_init( &prompt_list );
		}
		proc_pop_interactive();
	}
	
	reader_write_title();
	
	sb_clear( &data->prompt_buff );
	
	for( i=0; i<al_get_count( &prompt_list); i++ )
	{
		sb_append( &data->prompt_buff, (wchar_t *)al_get( &prompt_list, i ) );
	}
	
	al_foreach( &prompt_list, &free );
	al_destroy( &prompt_list );
	
}

void reader_init()
{

	tcgetattr(0,&shell_modes);        /* get the current terminal modes */
	memcpy( &saved_modes,
			&shell_modes,
			sizeof(saved_modes));     /* save a copy so we can reset the terminal later */
	
	shell_modes.c_lflag &= ~ICANON;   /* turn off canonical mode */
	shell_modes.c_lflag &= ~ECHO;     /* turn off echo mode */
    shell_modes.c_cc[VMIN]=1;
    shell_modes.c_cc[VTIME]=0;

	al_init( &current_filename);
}


void reader_destroy()
{
	al_destroy( &current_filename);
	tcsetattr(0, TCSANOW, &saved_modes);
}


void reader_exit( int do_exit, int forced )
{
	if( data )
		data->end_loop=do_exit;
	end_loop=do_exit;
	if( forced )
		exit_forced = 1;
	
}

void repaint()
{
	parser_test( data->buff, data->indent, 0, 0 );
	
	s_write( &data->screen,
			 (wchar_t *)data->prompt_buff.buff,
			 data->buff,
			 data->color, 
			 data->indent, 
			 data->buff_pos );
	
}

/**
   Remove the previous character in the character buffer and on the
   screen using syntax highlighting, etc.
*/
static void remove_backward()
{

	if( data->buff_pos <= 0 )
		return;

	if( data->buff_pos < data->buff_len )
	{
		memmove( &data->buff[data->buff_pos-1],
				 &data->buff[data->buff_pos],
				 sizeof(wchar_t)*(data->buff_len-data->buff_pos+1) );
	}
	data->buff_pos--;
	data->buff_len--;
	data->buff[data->buff_len]=0;

	reader_super_highlight_me_plenty( data->buff_pos,
									  0 );

	repaint();

}


/**
   Insert the characters of the string into the command line buffer
   and print them to the screen using syntax highlighting, etc.
*/
static int insert_str(wchar_t *str)
{
	int len = wcslen( str );
	int old_len = data->buff_len;
	
	assert( data->buff_pos >= 0 );
	assert( data->buff_pos <= data->buff_len );
	assert( len >= 0 );
	
	data->buff_len += len;
	check_size();
	
	/* Insert space for extra characters at the right position */
	if( data->buff_pos < old_len )
	{
		memmove( &data->buff[data->buff_pos+len],
				 &data->buff[data->buff_pos],
				 sizeof(wchar_t)*(old_len-data->buff_pos) );
	}
	memmove( &data->buff[data->buff_pos], str, sizeof(wchar_t)*len );
	data->buff_pos += len;
	data->buff[data->buff_len]='\0';
	
	/*
	  Syntax highlight 
	*/
	reader_super_highlight_me_plenty( data->buff_pos-1,
									  0 );
	
	repaint();
	return 1;
}


/**
   Insert the character into the command line buffer and print it to
   the screen using syntax highlighting, etc.
*/
static int insert_char( int c )
{
	wchar_t str[]=
		{
			0, 0 
		}
	;
	str[0] = c;
	return insert_str( str );
}


/**
   Calculate the length of the common prefix substring of two strings.
*/
static int comp_len( wchar_t *a, wchar_t *b )
{
	int i;
	for( i=0;
		 a[i] != '\0' && b[i] != '\0' && a[i]==b[i];
		 i++ )
		;
	return i;
}

/**
   Find the outermost quoting style of current token. Returns 0 if
   token is not quoted.

*/
static wchar_t get_quote( wchar_t *cmd, int len )
{
	int i=0;
	wchar_t res=0;

	while( 1 )
	{
		if( !cmd[i] )
			break;

		if( cmd[i] == L'\\' )
		{
			i++;
			if( !cmd[i] )
				break;
			i++;			
		}
		else
		{
			if( cmd[i] == L'\'' || cmd[i] == L'\"' )
			{
				const wchar_t *end = quote_end( &cmd[i] );
				//fwprintf( stderr, L"Jump %d\n",  end-cmd );
				if(( end == 0 ) || (!*end) || (end-cmd > len))
				{
					res = cmd[i];
					break;
				}
				i = end-cmd+1;
			}
			else
				i++;
		}
	}

	return res;
}

/**
   Calculates information on the parameter at the specified index.

   \param cmd The command to be analyzed
   \param pos An index in the string which is inside the parameter
   \param quote If not 0, store the type of quote this parameter has, can be either ', " or \\0, meaning the string is not quoted.
   \param offset If not 0, get_param will store a pointer to the beginning of the parameter.
   \param string If not 0, get_parm will store a copy of the parameter string as returned by the tokenizer.
   \param type If not 0, get_param will store the token type as returned by tok_last.
*/
static void get_param( wchar_t *cmd,
					   int pos,
					   wchar_t *quote,
					   wchar_t **offset,
					   wchar_t **string,
					   int *type )
{
	int prev_pos=0;
	wchar_t last_quote = '\0';
	int unfinished;

	tokenizer tok;
	tok_init( &tok, cmd, TOK_ACCEPT_UNFINISHED );

	for( ; tok_has_next( &tok ); tok_next( &tok ) )
	{
		if( tok_get_pos( &tok ) > pos )
			break;

		if( tok_last_type( &tok ) == TOK_STRING )
			last_quote = get_quote( tok_last( &tok ),
									pos - tok_get_pos( &tok ) );

		if( type != 0 )
			*type = tok_last_type( &tok );
		if( string != 0 )
			wcscpy( *string, tok_last( &tok ) );

		prev_pos = tok_get_pos( &tok );
	}

	tok_destroy( &tok );

	wchar_t c = cmd[pos];
	cmd[pos]=0;
	int cmdlen = wcslen( cmd );
	unfinished = (cmdlen==0);
	if( !unfinished )
	{
		unfinished = (quote != 0);

		if( !unfinished )
		{
			if( wcschr( L" \t\n\r", cmd[cmdlen-1] ) != 0 )
			{
				if( ( cmdlen == 1) || (cmd[cmdlen-2] != L'\\') )
				{
					unfinished=1;
				}
			}
		}
	}

	if( quote )
		*quote = last_quote;

	if( offset != 0 )
	{
		if( !unfinished )
		{
			while( (cmd[prev_pos] != 0) && (wcschr( L";|",cmd[prev_pos])!= 0) )
				prev_pos++;

			*offset = cmd + prev_pos;
		}
		else
		{
			*offset = cmd + pos;
		}
	}
	cmd[pos]=c;
}

/**
   Insert the string at the current cursor position. The function
   checks if the string is quoted or not and correctly escapes the
   string.

   \param val the string to insert
   \param is_complete Whether this completion is the whole string or
   just the common prefix of several completions. If the former, end by
   printing a space (and an end quote if the parameter is quoted).
*/
static void completion_insert( wchar_t *val, int is_complete )
{
	wchar_t *replaced;

	wchar_t quote;

	get_param( data->buff,
			   data->buff_pos,
			   &quote,
			   0, 0, 0 );

	if( quote == L'\0' )
	{
		replaced = escape( val, 1 );
	}
	else
	{
		int unescapable=0;

		wchar_t *pin, *pout;

		replaced = pout =
			malloc( sizeof(wchar_t)*(wcslen(val) + 1) );

		for( pin=val; *pin; pin++ )
		{
			switch( *pin )
			{
				case L'\n':
				case L'\t':
				case L'\b':
				case L'\r':
					unescapable=1;
					break;
				default:
					*pout++ = *pin;
					break;
			}
		}
		if( unescapable )
		{
			free( replaced );
			wchar_t *tmp = escape( val, 1 );
			replaced = wcsdupcat( L" ", tmp );
			free( tmp);
			replaced[0]=quote;
		}
		else
			*pout = 0;
	}

	if( insert_str( replaced ) )
	{
		/*
		  Print trailing space since this is the only completion 
		*/
		if( is_complete ) 
		{

			if( (quote) &&
				(data->buff[data->buff_pos] != quote ) ) 
			{
				/* 
				   This is a quoted parameter, first print a quote 
				*/
				insert_char( quote );
			}
			insert_char( L' ' );
		}
	}

	free(replaced);
}

/**
   Run the fish_pager command to display the completion list. If the
   fish_pager outputs any text, it is inserted into the input
   backbuffer.

   \param prefix the string to display before every completion. 
   \param is_quoted should be set if the argument is quoted. This will change the display style.
   \param comp the list of completions to display
*/

static void run_pager( wchar_t *prefix, int is_quoted, array_list_t *comp )
{
	int i;
	string_buffer_t cmd;
	string_buffer_t msg;
	wchar_t * prefix_esc;
	char *foo;
	
	if( !prefix || (wcslen(prefix)==0))
		prefix_esc = wcsdup(L"\"\"");
	else
		prefix_esc = escape( prefix,1);

	sb_init( &cmd );
	sb_init( &msg );
	sb_printf( &cmd,
			   L"fish_pager -c 3 -r 4 %ls -p %ls",
//			   L"valgrind --track-fds=yes --log-file=pager.txt --leak-check=full ./fish_pager %d %ls",
			   is_quoted?L"-q":L"",
			   prefix_esc );

	free( prefix_esc );

	io_data_t *in= io_buffer_create( 1 );
	in->fd = 3;
	
	for( i=0; i<al_get_count( comp); i++ )
	{
	    wchar_t *el = escape((wchar_t*)al_get( comp, i ), 1);
		sb_printf( &msg, L"%ls\n", el );
		free( el );		
	}
	
	foo = wcs2str( (wchar_t *)msg.buff );
	b_append( in->param2.out_buffer, foo, strlen(foo) );
	free( foo );
	
	term_donate();
	
	io_data_t *out = io_buffer_create( 0 );
	out->next = in;
	out->fd = 4;
	
	eval( (wchar_t *)cmd.buff, out, TOP);
	term_steal();

	io_buffer_read( out );

	sb_destroy( &cmd );
	sb_destroy( &msg );

	int nil=0;
	b_append( out->param2.out_buffer, &nil, 1 );

	wchar_t *tmp;
	wchar_t *str = str2wcs((char *)out->param2.out_buffer->buff);

	if( str )
	{
		for( tmp = str + wcslen(str)-1; tmp >= str; tmp-- )
		{
			input_unreadch( *tmp );
		}
		free( str );
	}


	io_buffer_destroy( out);
	io_buffer_destroy( in);
}

/*
  Flash the screen. This function only changed the color of the
  current line, since the flash_screen sequnce is rather painful to
  look at in most terminal emulators.
*/
static void reader_flash()
{
	struct timespec pollint;

	int i;
	
	for( i=0; i<data->buff_pos; i++ )
	{
		data->color[i] = HIGHLIGHT_SEARCH_MATCH<<16;
	}
	
	repaint();
	
	pollint.tv_sec = 0;
	pollint.tv_nsec = 100 * 1000000;
	nanosleep( &pollint, NULL );

	reader_super_highlight_me_plenty( data->buff_pos, 0 );
	repaint();
	
	
}


/**
   Handle the list of completions. This means the following:
   
   - If the list is empty, flash the terminal.
   - If the list contains one element, write the whole element, and if
   the element does not end on a '/', '@', ':', or a '=', also write a trailing
   space.
   - If the list contains multiple elements with a common prefix, write
   the prefix.
   - If the list contains multiple elements without.
   a common prefix, call run_pager to display a list of completions. Depending on terminal size and the length of the list, run_pager may either show less than a screenfull and exit or use an interactive pager to allow the user to scroll through the completions.
   
   \param comp the list of completion strings
*/


static int handle_completions( array_list_t *comp )
{
	int i;
	
	if( al_get_count( comp ) == 0 )
	{
		reader_flash();
		return 0;
	}
	else if( al_get_count( comp ) == 1 )
	{
		wchar_t *comp_str = wcsdup((wchar_t *)al_get( comp, 0 ));
		wchar_t *woot = wcschr( comp_str, COMPLETE_SEP );
		if( woot != 0 )
			*woot = L'\0';
		completion_insert( comp_str,
						   ( wcslen(comp_str) == 0 ) ||
						   ( wcschr( L"/=@:",
									 comp_str[wcslen(comp_str)-1] ) == 0 ) );
		free( comp_str );
		return 1;
	}
	else
	{
		wchar_t *base = wcsdup( (wchar_t *)al_get( comp, 0 ) );
		int len = wcslen( base );
		for( i=1; i<al_get_count( comp ); i++ )
		{
			int new_len = comp_len( base, (wchar_t *)al_get( comp, i ) );
			len = new_len < len ? new_len: len;
		}
		if( len > 0 )
		{
			base[len]=L'\0';
			wchar_t *woot = wcschr( base, COMPLETE_SEP );
			if( woot != 0 )
				*woot = L'\0';
			completion_insert(base, 0);
		}
		else
		{
			/*
			  There is no common prefix in the completions, and show_list
			  is true, so we print the list
			*/
			int len;
			wchar_t * prefix;
			wchar_t * prefix_start;
			get_param( data->buff,
					   data->buff_pos,
					   0,
					   &prefix_start,
					   0,
					   0 );

			len = &data->buff[data->buff_pos]-prefix_start+1;

			if( len <= PREFIX_MAX_LEN )
			{
				prefix = malloc( sizeof(wchar_t)*(len+1) );
				wcslcpy( prefix, prefix_start, len );
				prefix[len]=L'\0';
			}
			else
			{
				wchar_t tmp[2]=
					{
						ellipsis_char,
						0
					}
				;

				prefix = wcsdupcat( tmp,
									prefix_start + (len - PREFIX_MAX_LEN) );
				prefix[PREFIX_MAX_LEN] = 0;

			}

			{
				int is_quoted;

				wchar_t quote;
				get_param( data->buff, data->buff_pos, &quote, 0, 0, 0 );
				is_quoted = (quote != L'\0');
				
				write(1, "\n", 1 );

				run_pager( prefix, is_quoted, comp );
			}

			free( prefix );
			s_reset( &data->screen );
			repaint();

		}

		free( base );
		return len;
	}
}


/**
   Initialize data for interactive use
*/
static void reader_interactive_init()
{
	/* See if we are running interactively.  */
	pid_t shell_pgid;

	input_init();
	kill_init();
	shell_pgid = getpgrp ();

	/*
	  This should enable job control on fish, even if our parent process did
	  not enable it for us.
	*/

	/* Loop until we are in the foreground.  */
	while (tcgetpgrp( 0 ) != shell_pgid)
	{
		killpg( shell_pgid, SIGTTIN);
	}

	/* Put ourselves in our own process group.  */
	shell_pgid = getpid ();
	if( getpgrp() != shell_pgid )
	{
		if (setpgid (shell_pgid, shell_pgid) < 0)
		{
			debug( 1,
				   _( L"Couldn't put the shell in its own process group" ));
			wperror( L"setpgid" );
			exit (1);
		}
	}

	/* Grab control of the terminal.  */
	if( tcsetpgrp (STDIN_FILENO, shell_pgid) )
	{
		debug( 1,
			   _( L"Couldn't grab control of terminal" ) );
		wperror( L"tcsetpgrp" );
		exit(1);
	}

	common_handle_winch(0);

    if( tcsetattr(0,TCSANOW,&shell_modes))      /* set the new modes */
    {
        wperror(L"tcsetattr");
    }

	/* 
	   We need to know our own pid so we'll later know if we are a
	   fork 
	*/
	original_pid = getpid();

	env_set( L"_", L"fish", ENV_GLOBAL );
}

/**
   Destroy data for interactive use
*/
static void reader_interactive_destroy()
{
	kill_destroy();
	writestr( L"\n" );
	set_color( FISH_COLOR_RESET, FISH_COLOR_RESET );
	input_destroy();
}


void reader_sanity_check()
{
	if( is_interactive)
	{
		if(!( data->buff_pos <= data->buff_len ))
			sanity_lose();
		if(!( data->buff_len == wcslen( data->buff ) ))
			sanity_lose();
	}
}

void reader_replace_current_token( wchar_t *new_token )
{

	wchar_t *begin, *end;
	string_buffer_t sb;
	int new_pos;

	/*
	  Find current token
	*/
	parse_util_token_extent( data->buff, data->buff_pos, &begin, &end, 0, 0 );

	if( !begin || !end )
		return;

	/*
	  Make new string
	*/
	sb_init( &sb );
	sb_append_substring( &sb, data->buff, begin-data->buff );
	sb_append( &sb, new_token );
	sb_append( &sb, end );

	new_pos = (begin-data->buff) + wcslen(new_token);

	reader_set_buffer( (wchar_t *)sb.buff, new_pos );
	sb_destroy( &sb );

}


/**
   Set the specified string from the history as the current buffer. Do
   not modify prefix_width.
*/
static void handle_history( const wchar_t *new_str )
{
	if( new_str )
	{
		data->buff_len = wcslen( new_str );
		check_size();
		wcscpy( data->buff, new_str );
		data->buff_pos=wcslen(data->buff);
		reader_super_highlight_me_plenty( data->buff_pos, 0 );

		repaint();
	}
}

/**
   Check if the specified string is contained in the list, using
   wcscmp as a comparison function
*/
static int contains( const wchar_t *needle,
					 array_list_t *haystack )
{
	int i;
	for( i=0; i<al_get_count( haystack ); i++ )
	{
		if( !wcscmp( needle, al_get( haystack, i ) ) )
			return 1;
	}
	return 0;

}

/**
   Reset the data structures associated with the token search
*/
static void reset_token_history()
{
	wchar_t *begin, *end;

	parse_util_token_extent( data->buff, data->buff_pos, &begin, &end, 0, 0 );
	if( begin )
	{
		wcslcpy(data->search_buff, begin, end-begin+1);
	}
	else
		data->search_buff[0]=0;

	data->token_history_pos = -1;
	data->search_pos=0;
	al_foreach( &data->search_prev, &free );
	al_truncate( &data->search_prev, 0 );
	al_push( &data->search_prev, wcsdup( data->search_buff ) );
}


/**
   Handles a token search command.

   \param forward if the search should be forward or reverse
   \param reset whether the current token should be made the new search token
*/
static void handle_token_history( int forward, int reset )
{
	wchar_t *str=0;
	int current_pos;
	tokenizer tok;

	if( reset )
	{
		/*
		  Start a new token search using the current token
		*/
		reset_token_history();

	}


	current_pos  = data->token_history_pos;

	if( forward || data->search_pos < (al_get_count( &data->search_prev )-1) )
	{
		if( forward )
		{
			if( data->search_pos > 0 )
			{
				data->search_pos--;
			}
			str = (wchar_t *)al_get( &data->search_prev, data->search_pos );
		}
		else
		{
			data->search_pos++;
			str = (wchar_t *)al_get( &data->search_prev, data->search_pos );
		}

		reader_replace_current_token( str );
		reader_super_highlight_me_plenty( data->buff_pos, 0 );
		repaint();
	}
	else
	{
		if( current_pos == -1 )
		{
			const wchar_t *item;
			
			/*
			  Move to previous line
			*/
			free( (void *)data->token_history_buff );		

			/*
			  Search for previous item that contains this substring
			*/
			item = history_prev_match(data->search_buff);

			/*
			  If there is no match, the original string is returned

			  If so, we clear the match string to avoid infinite loop
			*/
			if( wcscmp( item, data->search_buff ) == 0 )
			{
				item=L"";
			}

			data->token_history_buff = wcsdup( item );
			current_pos = wcslen(data->token_history_buff);

		}

		if( ! wcslen( data->token_history_buff ) )
		{
			/*
			  We have reached the end of the history - check if the
			  history already contains the search string itself, if so
			  return, otherwise add it.
			*/

			const wchar_t *last = al_get( &data->search_prev, al_get_count( &data->search_prev ) -1 );
			if( wcscmp( last, data->search_buff ) )
			{
				str = wcsdup(data->search_buff);
			}
			else
			{
				return;
			}
		}
		else
		{

			//debug( 3, L"new '%ls'", data->token_history_buff );

			for( tok_init( &tok, data->token_history_buff, TOK_ACCEPT_UNFINISHED );
				 tok_has_next( &tok);
				 tok_next( &tok ))
			{
				switch( tok_last_type( &tok ) )
				{
					case TOK_STRING:
					{
						if( wcsstr( tok_last( &tok ), data->search_buff ) )
						{
							//debug( 3, L"Found token at pos %d\n", tok_get_pos( &tok ) );
							if( tok_get_pos( &tok ) >= current_pos )
							{
								break;
							}
							//debug( 3, L"ok pos" );

							if( !contains( tok_last( &tok ), &data->search_prev ) )
							{
								free(str);
								data->token_history_pos = tok_get_pos( &tok );
								str = wcsdup(tok_last( &tok ));
							}

						}
					}
				}
			}

			tok_destroy( &tok );
		}

		if( str )
		{
			reader_replace_current_token( str );
			reader_super_highlight_me_plenty( data->buff_pos, 0 );
			repaint();
			al_push( &data->search_prev, str );
			data->search_pos = al_get_count( &data->search_prev )-1;
		}
		else if( ! reader_interrupted() )
		{
			data->token_history_pos=-1;
			handle_token_history( 0, 0 );
		}
	}
}


/**
   Move buffer position one word or erase one word. This function
   updates both the internal buffer and the screen. It is used by
   M-left, M-right and ^W to do block movement or block erase.

   \param dir Direction to move/erase. 0 means move left, 1 means move right.
   \param erase Whether to erase the characters along the way or only move past them.
   \param do_append if erase is true, this flag decides if the new kill item should be appended to the previous kill item.
*/
static void move_word( int dir, int erase, int new )
{
	int end_buff_pos=data->buff_pos;
	int step = dir?1:-1;

	/*
	  Return if we are already at the edge
	*/
	if( !dir && data->buff_pos == 0 )
	{
		return;
	}
	
	if( dir && data->buff_pos == data->buff_len )
	{
		return;
	}
	
	/*
	  If we are beyond the last character and moving left, start by
	  moving one step, since otehrwise we'll start on the \0, which
	  should be ignored.
	*/
	if( !dir && (end_buff_pos == data->buff_len) )
	{
		if( !end_buff_pos )
			return;
		
		end_buff_pos--;
	}
	
	/*
	  When moving left, ignore the character under the cursor
	*/
	if( !dir )
	{
		end_buff_pos+=2*step;
	}
	
	/*
	  Remove all whitespace characters before finding a word
	*/
	while( 1 )
	{
		wchar_t c;

		if( !dir )
		{
			if( end_buff_pos <= 0 )
				break;
		}
		else
		{
			if( end_buff_pos >= data->buff_len )
				break;
		}

		/*
		  Always eat at least one character
		*/
		if( end_buff_pos != data->buff_pos )
		{
			
			c = data->buff[end_buff_pos];
			
			if( !iswspace( c ) )
			{
				break;
			}
		}
		
		end_buff_pos+=step;

	}
	
	/*
	  Remove until we find a character that is not alphanumeric
	*/
	while( 1 )
	{
		wchar_t c;
		
		if( !dir )
		{
			if( end_buff_pos <= 0 )
				break;
		}
		else
		{
			if( end_buff_pos >= data->buff_len )
				break;
		}
		
		c = data->buff[end_buff_pos];
		
		if( !iswalnum( c ) )
		{
			/*
			  Don't gobble the boundary character when moving to the
			  right
			*/
			if( !dir )
				end_buff_pos -= step;
			break;
		}
		end_buff_pos+=step;
	}

	/*
	  Make sure we move at least one character
	*/
	if( end_buff_pos==data->buff_pos )
	{
		end_buff_pos+=step;
	}

	/*
	  Make sure we don't move beyond begining or end of buffer
	*/
	end_buff_pos = maxi( 0, mini( end_buff_pos, data->buff_len ) );
	


	if( erase )
	{
		int remove_count = abs(data->buff_pos - end_buff_pos);
		int first_char = mini( data->buff_pos, end_buff_pos );
//		fwprintf( stderr, L"Remove from %d to %d\n", first_char, first_char+remove_count );
		
		reader_kill( data->buff + first_char, remove_count, dir?KILL_APPEND:KILL_PREPEND, new );
		
	}
	else
	{
		data->buff_pos = end_buff_pos;
		repaint();
	}
}


wchar_t *reader_get_buffer()
{
	return data?data->buff:0;
}

void reader_set_buffer( wchar_t *b, int p )
{
	int l = wcslen( b );

	if( !data )
		return;

	data->buff_len = l;
	check_size();

	if( data->buff != b )
		wcscpy( data->buff, b );

	if( p>=0 )
	{
		data->buff_pos=mini( p, l );
	}
	else
	{
		data->buff_pos=l;
	}

	reader_super_highlight_me_plenty( data->buff_pos,
									  0 );
}


int reader_get_cursor_pos()
{
	if( !data )
		return -1;

	return data->buff_pos;
}


void reader_run_command( const wchar_t *cmd )
{

	wchar_t *ft;

	ft= tok_first( cmd );

	if( ft != 0 )
		env_set( L"_", ft, ENV_GLOBAL );
	free(ft);

	reader_write_title();

	term_donate();

	eval( cmd, 0, TOP );
	job_reap( 1 );

	term_steal();

	env_set( L"_", program_name, ENV_GLOBAL );

#ifdef HAVE__PROC_SELF_STAT
	proc_update_jiffies();
#endif


}


/**
   Test if the given shell command contains errors. Uses parser_test
   for testing.
*/

static int shell_test( wchar_t *b )
{
	int res = parser_test( b, 0, 0, 0 );
	
	if( res & PARSER_TEST_ERROR )
	{
		string_buffer_t sb;
		sb_init( &sb );

		int tmp[1];
		int tmp2[1];
		
		s_write( &data->screen, L"", L"", tmp, tmp2, 0 );
		
		parser_test( b, 0, &sb, L"fish" );
		fwprintf( stderr, L"%ls", sb.buff );
		sb_destroy( &sb );
	}
	return res;
}

/**
   Test if the given string contains error. Since this is the error
   detection for general purpose, there are no invalid strings, so
   this function always returns false.
*/
static int default_test( wchar_t *b )
{
	return 0;
}

void reader_push( wchar_t *name )
{
	reader_data_t *n = calloc( 1, sizeof( reader_data_t ) );
	n->name = wcsdup( name );
	n->next = data;
	sb_init( &n->kill_item );

	data=n;

	s_init( &data->screen );
	sb_init( &data->prompt_buff );

	check_size();
	data->buff[0]=data->search_buff[0]=0;

	if( data->next == 0 )
	{
		reader_interactive_init();
	}

	exec_prompt();
	reader_set_highlight_function( &highlight_universal );
	reader_set_test_function( &default_test );
	reader_set_prompt( L"" );
	history_set_mode( name );

	al_init( &data->search_prev );
	data->token_history_buff=0;

}

void reader_pop()
{
	reader_data_t *n = data;

	if( data == 0 )
	{
		debug( 0, _( L"Pop null reader block" ) );
		sanity_lose();
		return;
	}

	data=data->next;

	free(n->name );
	free( n->prompt );
	free( n->buff );
	free( n->color );
	free( n->indent );
	free( n->search_buff );
	sb_destroy( &n->kill_item );
	
	s_destroy( &n->screen );
	sb_destroy( &n->prompt_buff );

	/*
	  Clean up after history search
	*/
	al_foreach( &n->search_prev, &free );
	al_destroy( &n->search_prev );
    free( (void *)n->token_history_buff);

	free(n);

	if( data == 0 )
	{
		reader_interactive_destroy();
	}
	else
	{
		history_set_mode( data->name );
		exec_prompt();
	}
}

void reader_set_prompt( wchar_t *new_prompt )
{
 	free( data->prompt );
	data->prompt=wcsdup(new_prompt);
}

void reader_set_complete_function( void (*f)( const wchar_t *,
											  array_list_t * ) )
{
	data->complete_func = f;
}

void reader_set_highlight_function( void (*f)( wchar_t *,
											   int *,
											   int,
											   array_list_t * ) )
{
	data->highlight_func = f;
}

void reader_set_test_function( int (*f)( wchar_t * ) )
{
	data->test_func = f;
}

/**
   Call specified external highlighting function and then do search
   highlighting. Lastly, clear the background color under the cursor
   to avoid repaint issues on terminals where e.g. syntax highligthing
   maykes characters under the sursor unreadable.

   \param match_highlight_pos the position to use for bracket matching. This need not be the same as the surrent cursor position
   \param error if non-null, any possible errors in the buffer are further descibed by the strings inserted into the specified arraylist
*/
static void reader_super_highlight_me_plenty( int match_highlight_pos, array_list_t *error )
{
	data->highlight_func( data->buff, data->color, match_highlight_pos, error );

	if( data->search_buff && wcslen(data->search_buff) )
	{
		wchar_t * match = wcsstr( data->buff, data->search_buff );
		if( match )
		{
			int start = match-data->buff;
			int count = wcslen(data->search_buff );
			int i;

			for( i=0; i<count; i++ )
			{
				data->color[start+i] |= HIGHLIGHT_SEARCH_MATCH<<16;
			}
		}
	}
}


int exit_status()
{
	if( is_interactive )
		return first_job == 0 && data->end_loop;
	else
		return end_loop;
}

/**
   Read interactively. Read input from stdin while providing editing
   facilities.
*/
static int read_i()
{

	reader_push(L"fish");
	reader_set_complete_function( &complete );
	reader_set_highlight_function( &highlight_shell );
	reader_set_test_function( &shell_test );

	data->prev_end_loop=0;

	while( (!data->end_loop) && (!sanity_check()) )
	{
		wchar_t *tmp;

		if( function_exists( PROMPT_FUNCTION_NAME ) )
			reader_set_prompt( PROMPT_FUNCTION_NAME );
		else
			reader_set_prompt( DEFAULT_PROMPT );

		/*
		  Put buff in temporary string and clear buff, so
		  that we can handle a call to reader_set_buffer
		  during evaluation.
		*/

		tmp = reader_readline();

		if( data->end_loop)
		{
			job_t *j;
			int has_job=0;
			
			for( j=first_job; j; j=j->next )
			{
				if( !job_is_completed(j) )
				{
					has_job = 1;
					break;
				}
			}
			
			if( !reader_exit_forced() && !data->prev_end_loop && has_job )
			{
				writestr(_( L"There are stopped jobs\n" ));

				reader_exit( 0, 0 );
				data->prev_end_loop=1;

				repaint();
			}
			else
			{
				if( !isatty(0) )
				{
					/*
					  We already know that stdin is a tty since we're
					  in interactive mode. If isatty returns false, it
					  means stdin must have been closed. 
					*/
					for( j = first_job; j; j=j->next )
					{
						if( ! job_is_completed( j ) )
						{
							job_signal( j, SIGHUP );						
						}
					}
				}
			}
		}
		else if( tmp )
		{
			tmp = wcsdup( tmp );
			
			data->buff_pos=data->buff_len=0;
			data->buff[data->buff_len]=L'\0';
			reader_run_command( tmp );
			free( tmp );

			data->prev_end_loop=0;
		}

	}
	reader_pop();
	return 0;
}

/**
   Test if there are bytes available for reading on the specified file
   descriptor
*/
static int can_read( int fd )
{
	struct timeval can_read_timeout = { 0, 0 };
	fd_set fds;

	FD_ZERO(&fds);
    FD_SET(fd, &fds);
    return select(fd + 1, &fds, 0, 0, &can_read_timeout) == 1;
}

/**
   Test if the specified character is in the private use area that
   fish uses to store internal characters
*/
static int wchar_private( wchar_t c )
{
	return ( (c >= 0xe000) && (c <= 0xf8ff ) );
}

/**
   Test if the specified character in the specified string is
   backslashed.
*/
static int is_backslashed( const wchar_t *str, int pos )
{
	int count = 0;
	int i;
	
	for( i=pos-1; i>=0; i-- )
	{
		if( str[i] != L'\\' )
			break;
		
		count++;
	}

	return count %2;
}


wchar_t *reader_readline()
{

	wint_t c;
	int i;
	int last_char=0, yank=0;
	wchar_t *yank_str;
	array_list_t comp;
	int comp_empty=1;
	int finished=0;
	struct termios old_modes;

	check_size();
	data->search_buff[0]=data->buff[data->buff_len]='\0';


	al_init( &comp );

	s_reset( &data->screen );
	
	exec_prompt();

	reader_super_highlight_me_plenty( data->buff_pos, 0 );
	repaint();

	/* 
	   get the current terminal modes. These will be restored when the
	   function returns.
	*/
	tcgetattr(0,&old_modes);        
	/* set the new modes */
	if( tcsetattr(0,TCSANOW,&shell_modes))
	{
		wperror(L"tcsetattr");
    }

	while( !finished && !data->end_loop)
	{

		/*
		  Sometimes strange input sequences seem to generate a zero
		  byte. I believe these simply mean a character was pressed
		  but it should be ignored. (Example: Trying to add a tilde
		  (~) to digit)
		*/
		while( 1 )
		{
			c=input_readch();

			
			if( ( (!wchar_private(c))) && (c>31) && (c != 127) )
			{
				if( can_read(0) )
				{

					wchar_t arr[READAHEAD_MAX+1];
					int i;

					memset( arr, 0, sizeof( arr ) );
					arr[0] = c;

					for( i=1; i<READAHEAD_MAX; i++ )
					{

						if( !can_read( 0 ) )
						{
							c = 0;
							break;
						}
						c = input_readch();
						if( (!wchar_private(c)) && (c>31) && (c != 127) )
						{
							arr[i]=c;
							c=0;
						}
						else
							break;
					}

					insert_str( arr );

				}
			}

			if( c != 0 )
				break;
		}

		if( (last_char == R_COMPLETE) && (c != R_COMPLETE) && (!comp_empty) )
		{
			al_foreach( &comp, &free );
			al_truncate( &comp, 0 );
			comp_empty = 1;
		}

		if( last_char != R_YANK && last_char != R_YANK_POP )
			yank=0;
		
		switch( c )
		{

			/* go to beginning of line*/
			case R_BEGINNING_OF_LINE:
			{
				while( ( data->buff_pos>0 ) && 
					   ( data->buff[data->buff_pos-1] != L'\n' ) )
				{
					data->buff_pos--;
				}
				
				repaint();
				break;
			}

			case R_END_OF_LINE:
			{
				while( data->buff[data->buff_pos] && 
					   data->buff[data->buff_pos] != L'\n' )
				{
					data->buff_pos++;
				}
				
				repaint();
				break;
			}

			
			case R_BEGINNING_OF_BUFFER:
			{
				data->buff_pos = 0;

				repaint();
				break;
			}

			/* go to EOL*/
			case R_END_OF_BUFFER:
			{
				data->buff_pos = data->buff_len;

				repaint();
				break;
			}

			case R_NULL:
			{
//				exec_prompt();
				write( 1, "\r", 1 );
				s_reset( &data->screen );
				repaint();
				break;
			}

			case R_REPAINT:
			{
				exec_prompt();
				write( 1, "\r", 1 );
				s_reset( &data->screen );
				repaint();
				break;
			}

			case R_WINCH:
			{
				repaint();
				break;
			}

			case R_EOF:
			{
				exit_forced = 1;
				data->end_loop=1;
				break;
			}

			/* complete */
			case R_COMPLETE:
			{

				if( !data->complete_func )
					break;

 				if( comp_empty || last_char != R_COMPLETE)
				{
					wchar_t *begin, *end;
					wchar_t *token_begin, *token_end;
					wchar_t *buffcpy;
					int len;
					int cursor_steps;
					
					parse_util_cmdsubst_extent( data->buff, data->buff_pos, &begin, &end );

					parse_util_token_extent( begin, data->buff_pos - (begin-data->buff), &token_begin, &token_end, 0, 0 );
					
					cursor_steps = token_end - data->buff- data->buff_pos;
					data->buff_pos += cursor_steps;
					if( is_backslashed( data->buff, data->buff_pos ) )
					{
						remove_backward();
					}
					
					repaint();
					
					len = data->buff_pos - (begin-data->buff);
					buffcpy = wcsndup( begin, len );

					data->complete_func( buffcpy, &comp );

					sort_list( &comp );
					remove_duplicates( &comp );

					free( buffcpy );

				}
				if( (comp_empty =
					 handle_completions( &comp ) ) )
				{
					al_foreach( &comp, &free );
					al_truncate( &comp, 0 );
				}

				break;
			}

			/* kill */
			case R_KILL_LINE:
			{
				wchar_t *begin = &data->buff[data->buff_pos];
				wchar_t *end = begin;
				int len;
								
				while( *end && *end != L'\n' )
					end++;
				
				if( end==begin && *end )
					end++;
				
				len = end-begin;
				
				if( len )
				{
					reader_kill( begin, len, KILL_APPEND, last_char!=R_KILL_LINE );
				}
				
				break;
			}

			case R_BACKWARD_KILL_LINE:
			{
				if( data->buff_pos > 0 )
				{
					wchar_t *end = &data->buff[data->buff_pos];
					wchar_t *begin = end;
					int len;
					
					while( begin > data->buff  && *begin != L'\n' )
						begin--;
					
					if( *begin == L'\n' )
						begin++;
					
					len = maxi( end-begin, 1 );
					begin = end - len;
										
					reader_kill( begin, len, KILL_PREPEND, last_char!=R_BACKWARD_KILL_LINE );					
					
				}
				break;

			}

			case R_KILL_WHOLE_LINE:
			{
				wchar_t *end = &data->buff[data->buff_pos];
				wchar_t *begin = end;
				int len;
		
				while( begin > data->buff  && *begin != L'\n' )
					begin--;
				
				if( *begin == L'\n' )
					begin++;
				
				len = maxi( end-begin, 0 );
				begin = end - len;

				while( *end && *end != L'\n' )
					end++;
				
				if( begin == end && *end )
					end++;
				
				len = end-begin;
				
				if( len )
				{
					reader_kill( begin, len, KILL_APPEND, last_char!=R_KILL_WHOLE_LINE );					
				}
				
				break;
			}

			/* yank*/
			case R_YANK:
			{
				yank_str = kill_yank();
				insert_str( yank_str );
				yank = wcslen( yank_str );
				break;
			}

			/* rotate killring*/
			case R_YANK_POP:
			{
				if( yank )
				{
					for( i=0; i<yank; i++ )
						remove_backward();

					yank_str = kill_yank_rotate();
					insert_str(yank_str);
					yank = wcslen(yank_str);
				}
				break;
			}

			/* Escape was pressed */
			case L'\e':
			{
				if( *data->search_buff )
				{
					if( data->token_history_pos==-1 )
					{
						history_reset();
						reader_set_buffer( data->search_buff,
										   wcslen(data->search_buff ) );
					}
					else
					{
						reader_replace_current_token( data->search_buff );
					}
					*data->search_buff=0;
					reader_super_highlight_me_plenty( data->buff_pos, 0 );
					repaint();

				}

				break;
			}

			/* delete backward*/
			case R_BACKWARD_DELETE_CHAR:
			{
				remove_backward();
				break;
			}

			/* delete forward*/
			case R_DELETE_CHAR:
			{
				/**
				   Remove the current character in the character buffer and on the
				   screen using syntax highlighting, etc.
				*/
				if( data->buff_pos < data->buff_len )
				{
					data->buff_pos++;
					remove_backward();
				}
				break;
			}

			/* exit, but only if line is empty */
			case R_EXIT:
			{
				if( data->buff_len == 0 )
				{
					writestr( L"\n" );
					data->end_loop=1;
				}
				break;
			}

			/*
			  Evaluate. If the current command is unfinished, or if
			  the charater is escaped using a backslash, insert a
			  newline
			*/
			case R_EXECUTE:
			{
				/*
				  Allow backslash-escaped newlines
				*/
				if( is_backslashed( data->buff, data->buff_pos ) )
				{
					insert_char( '\n' );
					break;
				}
				
				switch( data->test_func( data->buff ) )
				{

					case 0:
					{
						/*
						  Finished commend, execute it
						*/
						if( wcslen( data->buff ) )
						{
//						wcscpy(data->search_buff,L"");
							history_add( data->buff );
						}
						finished=1;
						data->buff_pos=data->buff_len;
						repaint();
						writestr( L"\n" );
						break;
					}
					
					/*
					  We are incomplete, continue editing
					*/
					case PARSER_TEST_INCOMPLETE:
					{						
						insert_char( '\n' );
						break;
					}

					/*
					  Result must be some combination including an
					  error. The error message will already be
					  printed, all we need to do is repaint
					*/
					default:
					{
						s_reset( &data->screen );
						repaint();
						break;
					}

				}
				
				break;
			}

			/* History up */
			case R_HISTORY_SEARCH_BACKWARD:
			{
				if( (last_char != R_HISTORY_SEARCH_BACKWARD) &&
					(last_char != R_HISTORY_SEARCH_FORWARD) &&
					(last_char != R_FORWARD_CHAR) &&
					(last_char != R_BACKWARD_CHAR) )

				{
					wcscpy(data->search_buff, data->buff );
					data->search_buff[data->buff_pos]=0;
				}

				handle_history(history_prev_match(data->search_buff));
				break;
			}

			/* History down */
			case R_HISTORY_SEARCH_FORWARD:
			{
				if( (last_char != R_HISTORY_SEARCH_BACKWARD) &&
					(last_char != R_HISTORY_SEARCH_FORWARD) &&
					(last_char != R_FORWARD_CHAR) &&
					(last_char != R_BACKWARD_CHAR) )
				{
					wcscpy(data->search_buff, data->buff );
					data->search_buff[data->buff_pos]=0;
				}

				handle_history(history_next_match(data->search_buff));
				break;
			}

			/* Token search for a earlier match */
			case R_HISTORY_TOKEN_SEARCH_BACKWARD:
			{
				int reset=0;
				if( (last_char != R_HISTORY_TOKEN_SEARCH_BACKWARD) &&
					(last_char != R_HISTORY_TOKEN_SEARCH_FORWARD) )
				{
					reset=1;
				}

				handle_token_history( 0, reset );

				break;
			}

			/* Token search for a later match */
			case R_HISTORY_TOKEN_SEARCH_FORWARD:
			{
				int reset=0;

				if( (last_char != R_HISTORY_TOKEN_SEARCH_BACKWARD) &&
					(last_char != R_HISTORY_TOKEN_SEARCH_FORWARD) )
				{
					reset=1;
				}

				handle_token_history( 1, reset );

				break;
			}


			/* Move left*/
			case R_BACKWARD_CHAR:
			{
				if( data->buff_pos > 0 )
				{
					data->buff_pos--;
					repaint();
				}
				break;
			}
			
			/* Move right*/
			case R_FORWARD_CHAR:
			{
				if( data->buff_pos < data->buff_len )
				{
					data->buff_pos++;					
					repaint();
				}
				break;
			}

			case R_DELETE_LINE:
			{
				data->buff[0]=0;
				data->buff_len=0;
				data->buff_pos=0;
				repaint();
				break;
			}

			/* kill one word left */
			case R_BACKWARD_KILL_WORD:
			{
				move_word(0,1, last_char!=R_BACKWARD_KILL_WORD);
				break;
			}

			/* kill one word right */
			case R_KILL_WORD:
			{
				move_word(1,1, last_char!=R_KILL_WORD);
				break;
			}

			/* move one word left*/
			case R_BACKWARD_WORD:
			{
				move_word(0,0,0);
				break;
			}

			/* move one word right*/
			case R_FORWARD_WORD:
			{
				move_word( 1,0,0);
				break;
			}

			case R_BEGINNING_OF_HISTORY:
			{
				history_first();
				break;
			}

			case R_END_OF_HISTORY:
			{
				history_reset();
				break;
			}

			/* Other, if a normal character, we add it to the command */
			default:
			{
				if( (!wchar_private(c)) && (( (c>31) || (c==L'\n'))&& (c != 127)) )
				{
					insert_char( c );
				}
				else
				{
					/*
					  Low priority debug message. These can happen if
					  the user presses an unefined control
					  sequnece. No reason to report.
					*/
					debug( 2, _( L"Unknown keybinding %d" ), c );
				}
				break;
			}

		}

		if( (c != R_HISTORY_SEARCH_BACKWARD) &&
			(c != R_HISTORY_SEARCH_FORWARD) &&
			(c != R_HISTORY_TOKEN_SEARCH_BACKWARD) &&
			(c != R_HISTORY_TOKEN_SEARCH_FORWARD) &&
			(c != R_FORWARD_CHAR) &&
			(c != R_BACKWARD_CHAR) )
		{
			data->search_buff[0]=0;
			history_reset();
			data->token_history_pos=-1;
		}


		last_char = c;
	}

	al_destroy( &comp );
	if( !reader_exit_forced() )
	{
		if( tcsetattr(0,TCSANOW,&old_modes))      /* return to previous mode */
		{
			wperror(L"tcsetattr");
		}
		
		set_color( FISH_COLOR_RESET, FISH_COLOR_RESET );
	}
	
	return finished ? data->buff : 0;
}

/**
   Read non-interactively.  Read input from stdin without displaying
   the prompt, using syntax highlighting. This is used for reading
   scripts and init files.
*/
static int read_ni( int fd )
{
	FILE *in_stream;
	wchar_t *buff=0;
	buffer_t acc;

	int des = fd == 0 ? dup(0) : fd;
	int res=0;

	if (des == -1)
	{
		wperror( L"dup" );
		return 1;
	}

	b_init( &acc );

	in_stream = fdopen( des, "r" );
	if( in_stream != 0 )
	{
		wchar_t *str;
		int acc_used;

		while(!feof( in_stream ))
		{
			char buff[4096];
			int c;

			c = fread(buff, 1, 4096, in_stream);
			if( ferror( in_stream ) )
			{
				debug( 1,
					   _( L"Error while reading commands" ) );

				/*
				  Reset buffer on error. We won't evaluate incomplete files.
				*/
				acc.used=0;
				break;
			}

			b_append( &acc, buff, c );
		}
		b_append( &acc, "\0", 1 );
		acc_used = acc.used;
		str = str2wcs( acc.buff );
		b_destroy( &acc );

		if(	fclose( in_stream ))
		{
			debug( 1,
				   _( L"Error while closing input stream" ) );
			wperror( L"fclose" );
			res = 1;
		}

		if( str )
		{
		string_buffer_t sb;
		sb_init( &sb );
		
		if( !parser_test( str, 0, &sb, L"fish" ) )
		{
			eval( str, 0, TOP );
		}
		else
		{
			fwprintf( stderr, L"%ls", sb.buff );
			res = 1;
		}
		sb_destroy( &sb );
		
		free( str );
		}
		else
		{
			if( acc_used > 1 )
			{
				debug( 1,
					   _( L"Could not convert input. Read %d bytes." ),
					   acc_used-1 );
			}
			else
			{
				debug( 1,
					   _( L"Could not read input stream" ) );
			}
			res=1;
		}

	}
	else
	{
		debug( 1,
			   _( L"Error while opening input stream" ) );
		wperror( L"fdopen" );
		free( buff );
		res=1;
	}
	return res;
}

int reader_read( int fd )
{
	int res;

	/*
	  If reader_read is called recursively through the '.' builtin, we
	  need to preserve is_interactive. This, and signal handler setup
	  is handled by proc_push_interactive/proc_pop_interactive.
	*/

	proc_push_interactive( ((fd == 0) && isatty(STDIN_FILENO)));
	
	res= is_interactive?read_i():read_ni( fd );

	/*
	  If the exit command was called in a script, only exit the
	  script, not the program.
	*/
	if( data )
		data->end_loop = 0;
	end_loop = 0;
	
	proc_pop_interactive();
	return res;
}

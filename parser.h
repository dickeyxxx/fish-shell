/** \file parser.h
	The fish parser. 	
*/

#ifndef FISH_PARSER_H
#define FISH_PARSER_H

#include <wchar.h>

#include "proc.h"
#include "util.h"
#include "parser.h"

/**
   event_block_t represents a block on events of the specified type
*/
typedef struct event_block
{
	/**
	   The types of events to block. This is interpreted as a bitset
	   whete the value is 1 for every bit corresponding to a blocked
	   event type. For example, if EVENT_VARIABLE type events should
	   be blocked, (type & 1<<EVENT_BLOCKED) should be set. 

	   Note that EVENT_ANY can be used to specify any event.
	*/
	int type;
	
	/**
	   The next event_block struct
	*/
	struct event_block *next;
}
	event_block_t;



/**
   block_t represents a block of commands. 
*/
typedef struct block
{
	int type; /**< Type of block. Can be one of WHILE, FOR, IF and FUNCTION */
	int skip; /**< Whether execution of the commands in this block should be skipped */
	int tok_pos; /**< The start index of the block */

	/**
	   Status for the current loop block. Can be any of the values from the loop_status enum.
	*/
	int loop_status;

	/**
	   The job that is currently evaluated in the specified block.
	*/
	job_t *job;
	
	/**
	   First block type specific variable
	*/
	union 
	{
		int while_state;  /**< True if the loop condition has not yet been evaluated*/
		wchar_t *for_variable; /**< Name of the variable to loop over */
		int if_state; /**< The state of the if block */
		wchar_t *switch_value; /**< The value to test in a switch block */
		wchar_t *function_name; /**< The name of the function to define or the function called*/
		wchar_t *source_dest; /**< The name of the file to source*/
	} param1;

	/**
	   Second block type specific variable
	*/
	union
	{
		array_list_t for_vars; /**< List of values for a for block */	
		int switch_taken; /**< Whether a switch match has already been found */
		wchar_t *function_description; /**< The description of the function to define */
		array_list_t function_vars;		/**< List of arguments for a function call */
	} param2;

	/**
	   Third block type specific variable
	*/
	union
	{
		int function_is_binding; /**< Whether a function is a keybinding */
		int call_lineno; /**< Function invocation line number */		 
	} param3;

	/**
	   Fourth block type specific variable
	*/
	union
	{
		array_list_t *function_events;
		wchar_t *call_filename;
	} param4;
	
	/**
	   Some naming confusion. This is a pointer to the first element in the list of all event blocks.
	*/
	event_block_t *first_event_block;
	
    /**
	   Next outer block 
	*/
	struct block *outer; 
} block_t;

/** 
	Types of blocks
*/
enum block_type
{
	WHILE, /**< While loop block */
	FOR,  /**< For loop block */
	IF, /**< If block */
	FUNCTION_DEF, /**< Function definition block */
	FUNCTION_CALL, /**< Function invocation block */
	SWITCH, /**< Switch block */
	FAKE, /**< Fake block */
	SUBST, /**< Command substitution scope */
	TOP, /**< Outermost block */
	BEGIN, /**< Unconditional block */
	SOURCE, /**< Block created by the . (source) builtin */
}
;

/**
   Possible states for a loop
*/
enum loop_status 
{
	LOOP_NORMAL, /**< Current loop block executed as normal */
	LOOP_BREAK, /**< Current loop block should be removed */
	LOOP_CONTINUE, /**< Current loop block should be skipped */
};


/**
   Possible states for a while block
*/
enum while_status
{
	WHILE_TEST_FIRST, /**< This is the first command of the first lap of a while loop */
	WHILE_TEST_AGAIN, /**< This is not the first lap of the while loop, but it is the first command of the loop */
	WHILE_TESTED, /**< This is not the first command in the loop */
}
;



/**
   Errors that can be generated by the parser
*/
enum parser_error 
{
	/**
	   No error
	*/
	NO_ERR=0,
	/**
	   An error in the syntax 
	*/
	SYNTAX_ERROR,
	/**
	   Error occured while evaluating commands
	*/
	EVAL_ERROR,
	/**
	   Error while evaluating subshell
	*/
	SUBSHELL_ERROR,
}
;

/** The current innermost block */
extern block_t *current_block;

/** Global event blocks */
extern event_block_t *global_event_block;


/** The current error code */
extern int error_code;

/**
   Current block level io redirections 
*/
extern io_data_t *block_io;

/**
  Finds the full path of an executable in a newly allocated string.
  
  \param cmd The name of the executable.
  \return 0 if the command can not be found, the path of the command otherwise.
*/
wchar_t *get_filename( const wchar_t *cmd );

/**
  Evaluate the expressions contained in cmd.

  \param cmd the string to evaluate
  \param io io redirections to perform on all started jobs
  \param block_type The type of block to push on the block stack

  \return 0 on success, 1 otherwise
*/
int eval( const wchar_t *cmd, io_data_t *io, int block_type );

/**
  Evaluate line as a list of parameters, i.e. tokenize it and perform parameter expansion and subshell execution on the tokens.
  The output is inserted into output, and should be freed by the caller.

  \param line Line to evaluate
  \param output List to insert output to
*/
int eval_args( const wchar_t *line,
				array_list_t *output );

/**
   Sets the current error

   \param ec The new error code
   \param p The character offset at which the error occured
   \param str The printf-style error message filter
*/
void error( int ec, int p, const wchar_t *str, ... );


/**
   Tests if the specified commands parameters should be interpreted as another command, which will be true if the command is either 'command', 'exec', 'if', 'while' or 'builtin'.  

   \param cmd The command name to test
   \return 1 of the command parameter is a command, 0 otherwise
*/

int parser_is_subcommand( const wchar_t *cmd );

/**
   Tests if the specified command is a reserved word, i.e. if it is
   the name of one of the builtin functions that change the block or
   command scope, like 'for', 'end' or 'command' or 'exec'. These
   functions may not be overloaded, so their names are reserved.

   \param word The command name to test
   \return 1 of the command parameter is a command, 0 otherwise
*/
int parser_is_reserved( wchar_t *word );

/**
   Returns a string describing the current parser pisition in the format 'FILENAME (line LINE_NUMBER): LINE'.
   Example: 

   init.fish (line 127): ls|grep pancake
*/
wchar_t *parser_current_line();

/**
   Returns the current line number
*/
int parser_get_lineno();

/**
   Returns the current position in the latest string of the tokenizer.
*/
int parser_get_pos();

/**
   Returns the position where the current job started in the latest string of the tokenizer.
*/
int parser_get_job_pos();

/**
   Set the current position in the latest string of the tokenizer.
*/
void parser_set_pos( int p);

/**
   Get the string currently parsed
*/
const wchar_t *parser_get_buffer();

/**
   Create block of specified type
*/
void parser_push_block( int type);

/**
   Remove the outermost block namespace
*/
void parser_pop_block();

/**
   Return a description of the given blocktype
*/
const wchar_t *parser_get_block_desc( int block );


/**
   Test if the specified string can be parsed, or if more bytes need to be read first. 
   The result has the first bit set if the string contains errors, and the second bit is set if the string contains an unclosed block.
*/
int parser_test( wchar_t * buff, int babble );

/**
   Returns the full path of the specified directory. If the \c in is a
   full path to an existing directory, a copy of the string is
   returned. If \c in is a directory relative to one of the
   directories i the CDPATH, the full path is returned. If no
   directory can be found, 0 is returned.
*/
wchar_t *parser_cdpath_get( wchar_t *in );

/**
   Tell the parser that the specified function may not be run if not
   inside of a conditional block. This is to remove some possibilities
   of infinite recursion.
*/
void parser_forbid_function( wchar_t *function );
/**
   Undo last call to parser_forbid_function().
*/
void parser_allow_function();

/**
   Initialize static parser data
*/
void parser_init();

/**
   Destroy static parser data
*/
void parser_destroy();

/**
   This function checks if the specified string is a help option. 

   \param s the string to test
   \param min_match is the minimum number of characters that must match in a long style option, i.e. the longest common prefix between --help and any other option. If less than 3, 3 will be assumed.
*/
int parser_is_help( wchar_t *s, int min_match );

/**
   Returns the file currently evaluated by the parser. This can be
   different than reader_current_filename, e.g. if we are evaulating a
   function defined in a different file than the one curently read.
*/
const wchar_t *parser_current_filename();


#endif

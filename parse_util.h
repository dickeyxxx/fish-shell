/** \file parse_util.h

    Various mostly unrelated utility functions related to parsing,
    loading and evaluating fish code.
*/

#ifndef FISH_PARSE_UTIL_H
#define FISH_PARSE_UTIL_H

#include "autoload.h"
#include <wchar.h>
#include <map>
#include <set>

/**
   Find the beginning and end of the first subshell in the specified string.
   
   \param in the string to search for subshells
   \param begin the starting paranthesis of the subshell
   \param end the ending paranthesis of the subshell
   \param flags set this variable to ACCEPT_INCOMPLETE if in tab_completion mode
   \return -1 on syntax error, 0 if no subshells exist and 1 on sucess
*/

int parse_util_locate_cmdsubst( const wchar_t *in, 
								wchar_t **begin, 
								wchar_t **end,
								int flags );

/**
   Find the beginning and end of the command substitution under the
   cursor. If no subshell is found, the entire string is returned. If
   the current command substitution is not ended, i.e. the closing
   parenthesis is missing, then the string from the beginning of the
   substitution to the end of the string is returned.

   \param buff the string to search for subshells
   \param cursor_pos the position of the cursor
   \param a the start of the searched string
   \param b the end of the searched string
*/
void parse_util_cmdsubst_extent( const wchar_t *buff,
								 int cursor_pos,
								 wchar_t **a, 
								 wchar_t **b );

/**
   Find the beginning and end of the process definition under the cursor

   \param buff the string to search for subshells
   \param cursor_pos the position of the cursor
   \param a the start of the searched string
   \param b the end of the searched string
*/
void parse_util_process_extent( const wchar_t *buff,
								int cursor_pos,
								wchar_t **a, 
								wchar_t **b );


/**
   Find the beginning and end of the job definition under the cursor

   \param buff the string to search for subshells
   \param cursor_pos the position of the cursor
   \param a the start of the searched string
   \param b the end of the searched string
*/
void parse_util_job_extent( const wchar_t *buff,
							int cursor_pos,
							wchar_t **a, 
							wchar_t **b );

/**
   Find the beginning and end of the token under the cursor and the
   toekn before the current token. Any combination of tok_begin,
   tok_end, prev_begin and prev_end may be null.

   \param buff the string to search for subshells
   \param cursor_pos the position of the cursor
   \param tok_begin the start of the current token
   \param tok_end the end of the current token
   \param prev_begin the start o the token before the current token
   \param prev_end the end of the token before the current token
*/
void parse_util_token_extent( const wchar_t *buff,
							  int cursor_pos,
							  wchar_t **tok_begin,
							  wchar_t **tok_end,
							  wchar_t **prev_begin, 
							  wchar_t **prev_end );


/**
   Get the linenumber at the specified character offset
*/
int parse_util_lineno( const wchar_t *str, int len );

/**
   Calculate the line number of the specified cursor position
 */
int parse_util_get_line_from_offset( wchar_t *buff, int pos );

/**
   Get the offset of the first character on the specified line
 */
int parse_util_get_offset_from_line( wchar_t *buff, int line );


/**
   Return the total offset of the buffer for the cursor position nearest to the specified poition
 */
int parse_util_get_offset( wchar_t *buff, int line, int line_offset );

/**
   Set the argv environment variable to the specified null-terminated
   array of strings. 
*/
void parse_util_set_argv( wchar_t **argv, const wcstring_list_t &named_arguments );

/**
   Make a duplicate of the specified string, unescape wildcard
   characters but not performing any other character transformation.
*/
wchar_t *parse_util_unescape_wildcards( const wchar_t *in );



#endif

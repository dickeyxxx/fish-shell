/** \file wildcard.h 

    My own globbing implementation. Needed to implement this instead
    of using libs globbing to support tab-expansion of globbed
    paramaters.

*/

#ifndef FISH_WILDCARD_H
/**
   Header guard
*/
#define FISH_WILDCARD_H

#include <wchar.h>
#include <vector>

#include "util.h"
#include "common.h"
#include "expand.h"

/*
  Use unencoded private-use keycodes for internal characters
*/

#define WILDCARD_RESERVED 0xf400

class completion_t;
/**
   Enumeration of all wildcard types
*/
enum
{
	/** Character representing any character except '/' */
	ANY_CHAR = WILDCARD_RESERVED,

	/** Character representing any character string not containing '/' (A slash) */
	ANY_STRING,

	/** Character representing any character string */
	ANY_STRING_RECURSIVE,
}
	;

/**
    Expand the wildcard by matching against the filesystem. 

	New strings are allocated using malloc and should be freed by the caller.

	wildcard_expand works by dividing the wildcard into segments at
	each directory boundary. Each segment is processed separatly. All
	except the last segment are handled by matching the wildcard
	segment against all subdirectories of matching directories, and
	recursively calling wildcard_expand for matches. On the last
	segment, matching is made to any file, and all matches are
	inserted to the list.

	If wildcard_expand encounters any errors (such as insufficient
	priviliges) during matching, no error messages will be printed and
	wildcard_expand will continue the matching process.

	\param wc The wildcard string
	\param base_dir The base directory of the filesystem to perform the match against
	\param flags flags for the search. Can be any combination of ACCEPT_INCOMPLETE and EXECUTABLES_ONLY
	\param out The list in which to put the output

	\return 1 if matches where found, 0 otherwise. Return -1 on abort (I.e. ^C was pressed).
   
*/
int wildcard_expand_string(const wcstring &wc, const wcstring &base_dir, expand_flags_t flags, std::vector<completion_t> &out );
/**
   Test whether the given wildcard matches the string. Does not perform any I/O.

   \param str The string to test
   \param wc The wildcard to test against
   \return true if the wildcard matched
*/
int wildcard_match( const wcstring &str, const wcstring &wc );


/**
   Check if the specified string contains wildcards
*/
int wildcard_has( const wchar_t *str, int internal );

/**
   Test wildcard completion
*/
bool wildcard_complete(const wcstring &str,
					   const wchar_t *wc,
					   const wchar_t *desc,						
					   wcstring (*desc_func)(const wcstring &),
					   std::vector<completion_t> &out,
					   expand_flags_t flags );

#endif

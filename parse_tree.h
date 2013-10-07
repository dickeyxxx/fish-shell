/**\file parse_tree.h

    Programmatic representation of fish code.
*/

#ifndef FISH_PARSE_PRODUCTIONS_H
#define FISH_PARSE_PRODUCTIONS_H

#include <wchar.h>

#include "config.h"
#include "util.h"
#include "common.h"
#include "tokenizer.h"
#include <vector>

#define PARSE_ASSERT(a) assert(a)
#define PARSER_DIE() do { fprintf(stderr, "Parser dying!\n"); exit_without_destructors(-1); } while (0)

class parse_node_t;
class parse_node_tree_t;
typedef size_t node_offset_t;
#define NODE_OFFSET_INVALID (static_cast<node_offset_t>(-1))

struct parse_error_t
{
    /** Text of the error */
    wcstring text;

    /** Offset and length of the token in the source code that triggered this error */
    size_t source_start;
    size_t source_length;

    /** Return a string describing the error, suitable for presentation to the user */
    wcstring describe(const wcstring &src) const;
};
typedef std::vector<parse_error_t> parse_error_list_t;

enum parse_token_type_t
{
    token_type_invalid,

    // Non-terminal tokens
    symbol_job_list,
    symbol_job,
    symbol_job_continuation,
    symbol_statement,
    symbol_block_statement,
    symbol_block_header,
    symbol_for_header,
    symbol_while_header,
    symbol_begin_header,
    symbol_function_header,

    symbol_if_statement,
    symbol_if_clause,
    symbol_else_clause,
    symbol_else_continuation,

    symbol_switch_statement,
    symbol_case_item_list,
    symbol_case_item,

    symbol_boolean_statement,
    symbol_decorated_statement,
    symbol_plain_statement,
    symbol_arguments_or_redirections_list,
    symbol_argument_or_redirection,

    symbol_argument_list,

    symbol_argument,
    symbol_redirection,

    symbol_optional_background,

    // Terminal types
    parse_token_type_string,
    parse_token_type_pipe,
    parse_token_type_redirection,
    parse_token_type_background,
    parse_token_type_end,
    parse_token_type_terminate,

    // Very special terminal types that don't appear in the production list
    parse_special_type_parse_error,
    parse_special_type_tokenizer_error,
    parse_special_type_comment,

    FIRST_TERMINAL_TYPE = parse_token_type_string,
    LAST_TERMINAL_TYPE = parse_token_type_terminate,

    LAST_TOKEN_OR_SYMBOL = parse_token_type_terminate,
    FIRST_PARSE_TOKEN_TYPE = parse_token_type_string
};

enum parse_keyword_t
{
    parse_keyword_none,
    parse_keyword_if,
    parse_keyword_else,
    parse_keyword_for,
    parse_keyword_in,
    parse_keyword_while,
    parse_keyword_begin,
    parse_keyword_function,
    parse_keyword_switch,
    parse_keyword_case,
    parse_keyword_end,
    parse_keyword_and,
    parse_keyword_or,
    parse_keyword_not,
    parse_keyword_command,
    parse_keyword_builtin,

    LAST_KEYWORD = parse_keyword_builtin
};


enum
{
    parse_flag_none = 0,

    /* Attempt to build a "parse tree" no matter what. This may result in a 'forest' of disconnected trees. This is intended to be used by syntax highlighting. */
    parse_flag_continue_after_error = 1 << 0,
    
    /* Include comment tokens */
    parse_flag_include_comments = 1 << 1
};
typedef unsigned int parse_tree_flags_t;

class parse_ll_t;
class parse_t
{
    parse_ll_t * const parser;

public:
    parse_t();
    ~parse_t();

    /* Parse a string */
    bool parse(const wcstring &str, parse_tree_flags_t flags, parse_node_tree_t *output, parse_error_list_t *errors, bool log_it = false);

    /* Parse a single token */
    bool parse_1_token(parse_token_type_t token, parse_keyword_t keyword, parse_node_tree_t *output, parse_error_list_t *errors);
    
    /* Reset, ready to parse something else */
    void clear();

};

wcstring parse_dump_tree(const parse_node_tree_t &tree, const wcstring &src);

wcstring token_type_description(parse_token_type_t type);
wcstring keyword_description(parse_keyword_t type);

/** Class for nodes of a parse tree */
class parse_node_t
{
public:

    /* Type of the node */
    enum parse_token_type_t type;

    /* Start in the source code */
    size_t source_start;

    /* Length of our range in the source code */
    size_t source_length;
    
    /* Parent */
    node_offset_t parent;

    /* Children */
    node_offset_t child_start;
    node_offset_t child_count;

    /* Type-dependent data */
    uint32_t tag;

    /* Which production was used */
    uint8_t production_idx;

    /* Description */
    wcstring describe(void) const;

    /* Constructor */
    explicit parse_node_t(parse_token_type_t ty) : type(ty), source_start(-1), source_length(0), parent(NODE_OFFSET_INVALID), child_start(0), child_count(0), tag(0)
    {
    }

    node_offset_t child_offset(node_offset_t which) const
    {
        PARSE_ASSERT(which < child_count);
        return child_start + which;
    }

    /* Indicate if this node has a range of source code associated with it */
    bool has_source() const
    {
        return source_start != (size_t)(-1);
    }
};

/* The parse tree itself */
class parse_node_tree_t : public std::vector<parse_node_t>
{
public:

    /* Get the node corresponding to a child of the given node, or NULL if there is no such child. If expected_type is provided, assert that the node has that type. */
    const parse_node_t *get_child(const parse_node_t &parent, node_offset_t which, parse_token_type_t expected_type = token_type_invalid) const;    
    
    /* Get the node corresponding to the parent of the given node, or NULL if there is no such child. If expected_type is provided, only returns the parent if it is of that type. Note the asymmetry: get_child asserts since the children are known, but get_parent does not, since the parent may not be known. */
    const parse_node_t *get_parent(const parse_node_t &node, parse_token_type_t expected_type = token_type_invalid) const;


    /* Find all the nodes of a given type underneath a given node */
    typedef std::vector<const parse_node_t *> parse_node_list_t;
    parse_node_list_t find_nodes(const parse_node_t &parent, parse_token_type_t type) const;
};


/* Node type specific data, stored in the tag field */

/* Statement decorations, stored in the tag of plain_statement. This matches the order of productions in decorated_statement */
enum parse_statement_decoration_t
{
    parse_statement_decoration_none,
    parse_statement_decoration_command,
    parse_statement_decoration_builtin
};

/* Argument flags as a bitmask, stored in the tag of argument */
enum parse_argument_flags_t
{
    /* Indicates that this or a prior argument was --, so this should not be treated as an option */
    parse_argument_no_options = 1 << 0,
    
    /* Indicates that the argument is for a cd command */
    parse_argument_is_for_cd = 1 << 1
};

/* Fish grammar:

# A job_list is a list of jobs, separated by semicolons or newlines

    job_list = <empty> |
                job job_list
                <TOK_END> job_list

# A job is a non-empty list of statements, separated by pipes. (Non-empty is useful for cases like if statements, where we require a command). To represent "non-empty", we require a statement, followed by a possibly empty job_continuation

    job = statement job_continuation
    job_continuation = <empty> |
                       <TOK_PIPE> statement job_continuation

# A statement is a normal command, or an if / while / and etc

    statement = boolean_statement | block_statement | if_statement | switch_statement | decorated_statement

# A block is a conditional, loop, or begin/end

    if_statement = if_clause else_clause <END> arguments_or_redirections_list
    if_clause = <IF> job STATEMENT_TERMINATOR job_list
    else_clause = <empty> |
                 <ELSE> else_continuation
    else_continuation = if_clause else_clause |
                        STATEMENT_TERMINATOR job_list

    switch_statement = SWITCH <TOK_STRING> STATEMENT_TERMINATOR case_item_list <END>
    case_item_list = <empty> |
                    case_item case_item_list
    case_item = CASE argument_list STATEMENT_TERMINATOR job_list

    block_statement = block_header <TOK_END> job_list <END> arguments_or_redirections_list
    block_header = for_header | while_header | function_header | begin_header
    for_header = FOR var_name IN arguments_or_redirections_list
    while_header = WHILE statement
    begin_header = BEGIN
    function_header = FUNCTION function_name argument_list

# A boolean statement is AND or OR or NOT

    boolean_statement = AND statement | OR statement | NOT statement

# A decorated_statement is a command with a list of arguments_or_redirections, possibly with "builtin" or "command"
# The tag of a plain statement indicates which mode to use

    decorated_statement = plain_statement | COMMAND plain_statement | BUILTIN plain_statement
    plain_statement = <TOK_STRING> arguments_or_redirections_list optional_background

    argument_list = <empty> | argument argument_list


    arguments_or_redirections_list = <empty> |
                                     argument_or_redirection arguments_or_redirections_list
    argument_or_redirection = argument | redirection
    argument = <TOK_STRING>
    redirection = <TOK_REDIRECTION>

    terminator = <TOK_END> | <TOK_BACKGROUND>

    optional_background = <empty> | <TOK_BACKGROUND>

*/

#endif

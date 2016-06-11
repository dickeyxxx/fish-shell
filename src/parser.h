// The fish parser.
#ifndef FISH_PARSER_H
#define FISH_PARSER_H

#include <stdbool.h>
#include <stddef.h>
#include <list>
#include <vector>

#include "common.h"
#include "event.h"
#include "expand.h"
#include "parse_constants.h"
#include "parse_tree.h"
#include "proc.h"

class io_chain_t;

/// event_blockage_t represents a block on events of the specified type.
struct event_blockage_t {
    /// The types of events to block. This is interpreted as a bitset whete the value is 1 for every
    /// bit corresponding to a blocked event type. For example, if EVENT_VARIABLE type events should
    /// be blocked, (type & 1<<EVENT_BLOCKED) should be set.
    ///
    /// Note that EVENT_ANY can be used to specify any event.
    unsigned int typemask;
};

typedef std::list<event_blockage_t> event_blockage_list_t;

inline bool event_block_list_blocks_type(const event_blockage_list_t &ebls, int type) {
    for (event_blockage_list_t::const_iterator iter = ebls.begin(); iter != ebls.end(); ++iter) {
        if (iter->typemask & (1 << EVENT_ANY)) return true;
        if (iter->typemask & (1 << type)) return true;
    }
    return false;
}

/// Types of blocks.
enum block_type_t {
    WHILE,                    /// While loop block
    FOR,                      /// For loop block
    IF,                       /// If block
    FUNCTION_DEF,             /// Function definition block
    FUNCTION_CALL,            /// Function invocation block
    FUNCTION_CALL_NO_SHADOW,  /// Function invocation block with no variable shadowing
    SWITCH,                   /// Switch block
    FAKE,                     /// Fake block
    SUBST,                    /// Command substitution scope
    TOP,                      /// Outermost block
    BEGIN,                    /// Unconditional block
    SOURCE,                   /// Block created by the . (source) builtin
    EVENT,                    /// Block created on event notifier invocation
    BREAKPOINT,               /// Breakpoint block
};

/// Possible states for a loop.
enum loop_status_t {
    LOOP_NORMAL,    /// current loop block executed as normal
    LOOP_BREAK,     /// current loop block should be removed
    LOOP_CONTINUE,  /// current loop block should be skipped
};

/// block_t represents a block of commands.
struct block_t {
   protected:
    /// Protected constructor. Use one of the subclasses below.
    explicit block_t(block_type_t t);

   private:
    /// Type of block.
    const block_type_t block_type;

   public:
    /// Whether execution of the commands in this block should be skipped.
    bool skip;
    /// The start index of the block.
    int tok_pos;
    /// Offset of the node.
    node_offset_t node_offset;
    /// Status for the current loop block. Can be any of the values from the loop_status enum.
    enum loop_status_t loop_status;
    /// The job that is currently evaluated in the specified block.
    job_t *job;
    /// Name of file that created this block. This string is intern'd.
    const wchar_t *src_filename;
    /// Line number where this block was created.
    int src_lineno;
    /// Whether we should pop the environment variable stack when we're popped off of the block
    /// stack.
    bool wants_pop_env;

    block_type_t type() const { return this->block_type; }

    /// Description of the block, for debugging.
    wcstring description() const;

    /// List of event blocks.
    event_blockage_list_t event_blocks;

    /// Destructor
    virtual ~block_t();
};

struct if_block_t : public block_t {
    if_block_t();
};

struct event_block_t : public block_t {
    event_t const event;
    explicit event_block_t(const event_t &evt);
};

struct function_block_t : public block_t {
    const process_t *process;
    wcstring name;
    function_block_t(const process_t *p, const wcstring &n, bool shadows);
};

struct source_block_t : public block_t {
    const wchar_t *const source_file;
    explicit source_block_t(const wchar_t *src);
};

struct for_block_t : public block_t {
    for_block_t();
};

struct while_block_t : public block_t {
    while_block_t();
};

struct switch_block_t : public block_t {
    switch_block_t();
};

struct fake_block_t : public block_t {
    fake_block_t();
};

struct scope_block_t : public block_t {
    explicit scope_block_t(block_type_t type);  // must be BEGIN, TOP or SUBST
};

struct breakpoint_block_t : public block_t {
    breakpoint_block_t();
};

/// Errors that can be generated by the parser.
enum parser_error {
    /// No error.
    NO_ERR = 0,
    /// An error in the syntax.
    SYNTAX_ERROR,
    /// Error occured while evaluating commands.
    EVAL_ERROR,
    /// Error while evaluating cmdsubst.
    CMDSUBST_ERROR,
};

struct profile_item_t {
    /// Time spent executing the specified command, including parse time for nested blocks.
    int exec;
    /// Time spent parsing the specified command, including execution time for command
    /// substitutions.
    int parse;
    /// The block level of the specified command. nested blocks and command substitutions both
    /// increase the block level.
    size_t level;
    /// If the execution of this command was skipped.
    bool skipped;
    /// The command string.
    wcstring cmd;
};

class parse_execution_context_t;
class completion_t;

class parser_t {
    friend class parse_execution_context_t;

   private:
    /// Indication that we should skip all blocks.
    bool cancellation_requested;
    /// Indicates that we are within the process of initializing fish.
    bool is_within_fish_initialization;
    /// Stack of execution contexts. We own these pointers and must delete them.
    std::vector<parse_execution_context_t *> execution_contexts;
    /// List of called functions, used to help prevent infinite recursion.
    wcstring_list_t forbidden_function;
    /// The jobs associated with this parser.
    job_list_t my_job_list;
    /// The list of blocks, allocated with new. It's our responsibility to delete these.
    std::vector<block_t *> block_stack;

    /// Gets a description of the block stack, for debugging.
    wcstring block_stack_description() const;

    /// List of profile items, allocated with new.
    std::vector<profile_item_t *> profile_items;

    // No copying allowed.
    parser_t(const parser_t &);
    parser_t &operator=(const parser_t &);

    /// Adds a job to the beginning of the job list.
    void job_add(job_t *job);

    /// Returns the name of the currently evaluated function if we are currently evaluating a
    /// function, null otherwise. This is tested by moving down the block-scope-stack, checking
    /// every block if it is of type FUNCTION_CALL.
    const wchar_t *is_function() const;

    /// Helper for stack_trace().
    void stack_trace_internal(size_t block_idx, wcstring *out) const;

   public:
    /// Get the "principal" parser, whatever that is.
    static parser_t &principal_parser();

    /// Indicates that execution of all blocks in the principal parser should stop. This is called
    /// from signal handlers!
    static void skip_all_blocks();

    /// Create a parser.
    parser_t();

    /// Global event blocks.
    event_blockage_list_t global_event_blocks;

    /// Evaluate the expressions contained in cmd.
    ///
    /// \param cmd the string to evaluate
    /// \param io io redirections to perform on all started jobs
    /// \param block_type The type of block to push on the block stack
    ///
    /// \return 0 on success, 1 otherwise
    int eval(const wcstring &cmd, const io_chain_t &io, enum block_type_t block_type);

    /// Evaluate the expressions contained in cmd, which has been parsed into the given parse tree.
    /// This takes ownership of the tree.
    int eval_acquiring_tree(const wcstring &cmd, const io_chain_t &io, enum block_type_t block_type,
                            moved_ref<parse_node_tree_t> t);

    /// Evaluates a block node at the given node offset in the topmost execution context.
    int eval_block_node(node_offset_t node_idx, const io_chain_t &io, enum block_type_t block_type);

    /// Evaluate line as a list of parameters, i.e. tokenize it and perform parameter expansion and
    /// cmdsubst execution on the tokens. The output is inserted into output. Errors are ignored.
    ///
    /// \param arg_src String to evaluate as an argument list
    /// \param flags Some expand flags to use
    /// \param output List to insert output into
    static void expand_argument_list(const wcstring &arg_src, expand_flags_t flags,
                                     std::vector<completion_t> *output);

    /// Returns a string describing the current parser pisition in the format 'FILENAME (line
    /// LINE_NUMBER): LINE'. Example:
    ///
    /// init.fish (line 127): ls|grep pancake
    wcstring current_line();

    /// Returns the current line number.
    int get_lineno() const;

    /// Returns the block at the given index. 0 corresponds to the innermost block. Returns NULL
    /// when idx is at or equal to the number of blocks.
    const block_t *block_at_index(size_t idx) const;
    block_t *block_at_index(size_t idx);

    /// Returns the current (innermost) block.
    const block_t *current_block() const;
    block_t *current_block();

    /// Count of blocks.
    size_t block_count() const { return block_stack.size(); }

    /// Get the list of jobs.
    job_list_t &job_list() { return my_job_list; }

    // Hackish. In order to correctly report the origin of code with no associated file, we need to
    // know whether it's run during initialization or not.
    void set_is_within_fish_initialization(bool flag);

    /// Pushes the block. pop_block will call delete on it.
    void push_block(block_t *newv);

    /// Remove the outermost block namespace.
    void pop_block();

    /// Remove the outermost block, asserting it's the given one.
    void pop_block(const block_t *b);

    /// Return a description of the given blocktype.
    const wchar_t *get_block_desc(int block) const;

    /// Removes a job.
    bool job_remove(job_t *job);

    /// Promotes a job to the front of the list.
    void job_promote(job_t *job);

    /// Return the job with the specified job id. If id is 0 or less, return the last job used.
    job_t *job_get(int job_id);

    /// Returns the job with the given pid.
    job_t *job_get_from_pid(int pid);

    /// Returns a new profile item if profiling is active. The caller should fill it in. The
    /// parser_t will clean it up.
    profile_item_t *create_profile_item();

    /// Test if the specified string can be parsed, or if more bytes need to be read first. The
    /// result will have the PARSER_TEST_ERROR bit set if there is a syntax error in the code, and
    /// the PARSER_TEST_INCOMPLETE bit set if the code contains unclosed blocks.
    ///
    /// \param buff the text buffer to test
    /// \param block_level if non-null, the block nesting level will be filled out into this array
    /// \param out if non-null, any errors in the command will be filled out into this buffer
    /// \param prefix the prefix string to prepend to each error message written to the \c out
    /// buffer.
    void get_backtrace(const wcstring &src, const parse_error_list_t &errors,
                       wcstring *output) const;

    /// Detect errors in the specified string when parsed as an argument list. Returns true if an
    /// error occurred.
    bool detect_errors_in_argument_list(const wcstring &arg_list_src, wcstring *out_err,
                                        const wchar_t *prefix);

    /// Tell the parser that the specified function may not be run if not inside of a conditional
    /// block. This is to remove some possibilities of infinite recursion.
    void forbid_function(const wcstring &function);

    /// Undo last call to parser_forbid_function().
    void allow_function();

    /// Output profiling data to the given filename.
    void emit_profiling(const char *path) const;

    /// Returns the file currently evaluated by the parser. This can be different than
    /// reader_current_filename, e.g. if we are evaulating a function defined in a different file
    /// than the one curently read.
    const wchar_t *current_filename() const;

    /// Return a string representing the current stack trace.
    wcstring stack_trace() const;
};

#endif

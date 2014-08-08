/** \file docopt_registration.cpp

Functions for handling the set of docopt descriptions

*/

#include "../config.h"
#include "docopt_registration.h"
#include "../common.h"
#include "../parse_constants.h"
#include <map>
#include <vector>
#include <list>
#include <memory>

typedef docopt_fish::argument_parser_t<wcstring> docopt_parser_t;
typedef docopt_fish::error_t<wcstring> docopt_error_t;
typedef std::vector<docopt_error_t> docopt_error_list_t;

// Name, value pair
struct registration_t {
    wcstring name;
    wcstring description;
    docopt_parser_t parser;
    
    registration_t(const wcstring &n, const wcstring &d) : name(n), description(d)
    {}
    registration_t()
    {}
    
    /* TODO: terribly hackish, eliminate this */
    registration_t(const registration_t &rhs) : name(rhs.name), description(rhs.description)
    {}
};

// Class that holds a mapping from command name to list of docopt descriptions
class doc_register_t {
    typedef std::list<registration_t> registration_list_t;
    typedef std::map<wcstring, registration_list_t> registration_map_t;
    registration_map_t cmd_to_registration;
    mutex_lock_t lock;
    
    public:
    bool register_description(const wcstring &cmd, const wcstring &name, const wcstring &description, parse_error_list_t *out_errors)
    {
        // Try to parse it
        docopt_parser_t parser;
        docopt_error_list_t errors;
        bool parsed = parser.set_doc(description, &errors);

        // Translate errors from docopt to parse_error over
        if (out_errors != NULL)
        {
            parse_error_t parse_err;
            for (size_t i=0; i < errors.size(); i++)
            {
                const docopt_error_t &doc_err = errors.at(i);
                parse_err.text = doc_err.text;
                parse_err.code = parse_error_docopt;
                parse_err.source_start = doc_err.location;
                parse_err.source_length = 0;
                out_errors->push_back(parse_err);
            }
        }
        
        //
        if (parsed)
        {
            scoped_lock locker(lock);
            registration_list_t &regs = cmd_to_registration[cmd];
            
            // Remove any with the same name
            registration_list_t::iterator iter = regs.begin();
            while (iter != regs.end())
            {
                if (iter->name == name)
                {
                    iter = regs.erase(iter);
                }
                else
                {
                    ++iter;
                }
            }
            
            // Append a new one
            regs.push_back(registration_t());
            registration_t *reg = &regs.back();
            reg->name = name;
            reg->description = description;
            reg->parser = parser;
        }
        return parsed;
    }
    
    wcstring_list_t copy_registered_descriptions(const wcstring &cmd)
    {
        wcstring_list_t result;
        scoped_lock locker(lock);
        registration_map_t::iterator where = cmd_to_registration.find(cmd);
        if (where != cmd_to_registration.end()) {
            const registration_list_t &regs = where->second;
            for (registration_list_t::const_iterator iter = regs.begin(); iter != regs.end(); ++iter)
            {
                result.push_back(iter->description);
            }
        }
        return result;
    }
    
    const docopt_parser_t *first_parser(const wcstring &cmd) const
    {
        const docopt_parser_t *result = NULL;
        ASSERT_IS_LOCKED(lock);
        registration_map_t::const_iterator where = this->cmd_to_registration.find(cmd);
        if (where != this->cmd_to_registration.end() && ! where->second.empty())
        {
            result = &where->second.front().parser;
        }
        return result;
    }

    std::vector<docopt_fish::argument_status_t> validate_arguments(const wcstring &cmd, const wcstring_list_t &argv, docopt_fish::parse_flags_t flags)
    {
        scoped_lock locker(lock);
        std::vector<docopt_fish::argument_status_t> result;
        const docopt_parser_t *p = this->first_parser(cmd);
        if (p)
        {
            result = p->validate_arguments(argv, flags);
        }
        return result;
    }

    wcstring_list_t suggest_next_argument(const wcstring &cmd, const wcstring_list_t &argv, docopt_fish::parse_flags_t flags)
    {
        scoped_lock locker(lock);
        wcstring_list_t result;
        const docopt_parser_t *p = this->first_parser(cmd);
        if (p)
        {
            result = p->suggest_next_argument(argv, flags);
        }
        return result;
    }

    wcstring conditions_for_variable(const wcstring &cmd, const wcstring &var)
    {
        scoped_lock locker(lock);
        wcstring result;
        const docopt_parser_t *p = this->first_parser(cmd);
        if (p)
        {
            result = p->conditions_for_variable(var);
        }
        return result;
    }

    wcstring description_for_option(const wcstring &cmd, const wcstring &option)
    {
        scoped_lock locker(lock);
        wcstring result;
        const docopt_parser_t *p = this->first_parser(cmd);
        if (p)
        {
            result = p->description_for_option(option);
        }
        return result;
    }

};
static doc_register_t default_register;

bool docopt_register_description(const wcstring &cmd, const wcstring &name, const wcstring &description, parse_error_list_t *out_errors)
{
    return default_register.register_description(cmd, name, description, out_errors);
}

wcstring_list_t docopt_copy_registered_descriptions(const wcstring &cmd)
{
    return default_register.copy_registered_descriptions(cmd);
}


std::vector<docopt_fish::argument_status_t> docopt_validate_arguments(const wcstring &cmd, const wcstring_list_t &argv, docopt_fish::parse_flags_t flags)
{
    return default_register.validate_arguments(cmd, argv, flags);
}

wcstring_list_t docopt_suggest_next_argument(const wcstring &cmd, const wcstring_list_t &argv, docopt_fish::parse_flags_t flags)
{
    return default_register.suggest_next_argument(cmd, argv, flags);
}

wcstring docopt_conditions_for_variable(const wcstring &cmd, const wcstring &var)
{
    return default_register.conditions_for_variable(cmd, var);
}

wcstring docopt_description_for_option(const wcstring &cmd, const wcstring &option)
{
    return default_register.description_for_option(cmd, option);
}
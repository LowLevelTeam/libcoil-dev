/* coil/args.h */
#ifndef COIL_ARGS_H
#define COIL_ARGS_H

#include <stdbool.h>
#include "coil/log.h"
#include "coil/err.h"
#include "coil/mem.h"

/* Argument type */
typedef enum {
    COIL_ARG_FLAG,        /* Boolean flag (no value) */
    COIL_ARG_STRING,      /* String value */
    COIL_ARG_INT,         /* Integer value */
    COIL_ARG_FLOAT,       /* Float value */
    COIL_ARG_POSITIONAL,  /* Positional argument (not associated with a flag) */
} coil_arg_type_t;

/* Argument value */
typedef union {
    bool flag;
    const char *string;
    int integer;
    float floating;
} coil_arg_value_t;

/* Argument definition */
typedef struct {
    char short_name;          /* Short name (e.g., 'h' for -h) */
    const char *long_name;    /* Long name (e.g., "help" for --help) */
    const char *description;  /* Description for help text */
    coil_arg_type_t type;     /* Argument type */
    bool required;            /* Whether the argument is required */
    coil_arg_value_t default_value; /* Default value */
    const char *metavar;      /* Metavariable name for help text */
} coil_arg_def_t;

/* Argument parser */
typedef struct {
    const char *program_name;          /* Program name */
    const char *program_description;   /* Program description */
    const char *epilog;                /* Text to display after help */
    coil_arg_def_t *args;              /* Argument definitions */
    size_t arg_count;                  /* Number of argument definitions */
    size_t arg_capacity;               /* Capacity of argument definitions array */
    coil_arg_value_t *values;          /* Parsed argument values */
    bool *provided;                    /* Whether each argument was provided */
    coil_memory_arena_t *arena;        /* Memory arena for allocations */
    coil_logger_t *logger;             /* Logger */
    coil_error_manager_t *error_mgr;   /* Error manager */
    char **positional_args;            /* Positional arguments */
    size_t positional_count;           /* Number of positional arguments */
    bool help_requested;               /* Whether help was requested */
    bool initialized;                  /* Whether the parser is initialized */
} coil_arg_parser_t;

/* Initialize an argument parser */
int coil_arg_parser_init(coil_arg_parser_t *parser, 
                         const char *program_name,
                         const char *program_description,
                         const char *epilog,
                         coil_memory_arena_t *arena,
                         coil_logger_t *logger,
                         coil_error_manager_t *error_mgr);

/* Create a new argument parser */
coil_arg_parser_t *coil_arg_parser_create(const char *program_name,
                                         const char *program_description,
                                         const char *epilog,
                                         coil_memory_arena_t *arena,
                                         coil_logger_t *logger,
                                         coil_error_manager_t *error_mgr);

/* Add a flag argument */
int coil_arg_add_flag(coil_arg_parser_t *parser, 
                     char short_name,
                     const char *long_name,
                     const char *description,
                     bool default_value);

/* Add a string argument */
int coil_arg_add_string(coil_arg_parser_t *parser, 
                       char short_name,
                       const char *long_name,
                       const char *description,
                       const char *metavar,
                       const char *default_value,
                       bool required);

/* Add an integer argument */
int coil_arg_add_int(coil_arg_parser_t *parser, 
                    char short_name,
                    const char *long_name,
                    const char *description,
                    const char *metavar,
                    int default_value,
                    bool required);

/* Add a float argument */
int coil_arg_add_float(coil_arg_parser_t *parser, 
                      char short_name,
                      const char *long_name,
                      const char *description,
                      const char *metavar,
                      float default_value,
                      bool required);

/* Add a positional argument */
int coil_arg_add_positional(coil_arg_parser_t *parser, 
                           const char *name,
                           const char *description,
                           bool required);

/* Parse command line arguments */
bool coil_arg_parse(coil_arg_parser_t *parser, int argc, char **argv);

/* Print help text */
void coil_arg_print_help(coil_arg_parser_t *parser);

/* Get argument values */
bool coil_arg_get_flag(coil_arg_parser_t *parser, const char *name);
const char *coil_arg_get_string(coil_arg_parser_t *parser, const char *name);
int coil_arg_get_int(coil_arg_parser_t *parser, const char *name);
float coil_arg_get_float(coil_arg_parser_t *parser, const char *name);

/* Check if an argument was provided */
bool coil_arg_provided(coil_arg_parser_t *parser, const char *name);

/* Get positional arguments */
const char **coil_arg_get_positional(coil_arg_parser_t *parser, size_t *count);

/* Check if help was requested */
bool coil_arg_help_requested(coil_arg_parser_t *parser);

/* Validate required arguments */
bool coil_arg_validate(coil_arg_parser_t *parser);

/* Cleanup */
void coil_arg_parser_cleanup(coil_arg_parser_t *parser);
void coil_arg_parser_destroy(coil_arg_parser_t *parser);

/* Add standard arguments (help, verbose, quiet) */
void coil_arg_add_standard_args(coil_arg_parser_t *parser);

#endif /* COIL_ARGS_H */
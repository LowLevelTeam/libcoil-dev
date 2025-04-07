/* src/args.c */
#include "coil/args.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>

/* Find an argument by name (either short or long) */
static int coil_arg_find(coil_arg_parser_t *parser, const char *name) {
    if (!parser || !parser->initialized || !name) return -1;
    
    for (size_t i = 0; i < parser->arg_count; i++) {
        if ((name[0] == '-' && name[1] == '-' && parser->args[i].long_name && 
             strcmp(name + 2, parser->args[i].long_name) == 0) ||
            (name[0] == '-' && name[1] != '-' && parser->args[i].short_name && 
             name[1] == parser->args[i].short_name)) {
            return (int)i;
        } else if (name[0] != '-' && parser->args[i].long_name && 
                  strcmp(name, parser->args[i].long_name) == 0) {
            return (int)i;
        } else if (name[0] != '-' && parser->args[i].short_name && 
                  name[0] == parser->args[i].short_name && name[1] == '\0') {
            return (int)i;
        }
    }
    
    return -1;
}

int coil_arg_parser_init(coil_arg_parser_t *parser, 
                         const char *program_name,
                         const char *program_description,
                         const char *epilog,
                         coil_memory_arena_t *arena,
                         coil_logger_t *logger,
                         coil_error_manager_t *error_mgr) {
    if (!parser) return -1;
    
    parser->program_name = program_name ? 
        coil_memory_arena_strdup(arena, program_name) : NULL;
    
    parser->program_description = program_description ? 
        coil_memory_arena_strdup(arena, program_description) : NULL;
    
    parser->epilog = epilog ? 
        coil_memory_arena_strdup(arena, epilog) : NULL;
    
    parser->args = NULL;
    parser->arg_count = 0;
    parser->arg_capacity = 0;
    parser->values = NULL;
    parser->provided = NULL;
    parser->arena = arena ? arena : coil_global_arena;
    parser->logger = logger ? logger : coil_default_logger;
    parser->error_mgr = error_mgr ? error_mgr : coil_default_error_manager;
    parser->positional_args = NULL;
    parser->positional_count = 0;
    parser->help_requested = false;
    parser->initialized = true;
    
    return 0;
}

coil_arg_parser_t *coil_arg_parser_create(const char *program_name,
                                         const char *program_description,
                                         const char *epilog,
                                         coil_memory_arena_t *arena,
                                         coil_logger_t *logger,
                                         coil_error_manager_t *error_mgr) {
    if (!arena) {
        arena = coil_global_arena;
    }
    
    coil_arg_parser_t *parser = coil_memory_arena_calloc(arena, 1, sizeof(coil_arg_parser_t));
    if (!parser) return NULL;
    
    if (coil_arg_parser_init(parser, program_name, program_description, epilog, 
                            arena, logger, error_mgr) != 0) {
        return NULL;
    }
    
    return parser;
}

static int coil_arg_ensure_capacity(coil_arg_parser_t *parser) {
    if (!parser || !parser->initialized) return -1;
    
    /* Initial capacity or double when needed */
    size_t new_capacity = parser->arg_capacity == 0 ? 8 : parser->arg_capacity * 2;
    
    coil_arg_def_t *new_args = coil_memory_arena_calloc(parser->arena, 
                                                       new_capacity, 
                                                       sizeof(coil_arg_def_t));
    if (!new_args) return -1;
    
    coil_arg_value_t *new_values = coil_memory_arena_calloc(parser->arena, 
                                                           new_capacity, 
                                                           sizeof(coil_arg_value_t));
    if (!new_values) return -1;
    
    bool *new_provided = coil_memory_arena_calloc(parser->arena, 
                                                 new_capacity, 
                                                 sizeof(bool));
    if (!new_provided) return -1;
    
    /* Copy existing data if any */
    if (parser->args && parser->arg_count > 0) {
        memcpy(new_args, parser->args, parser->arg_count * sizeof(coil_arg_def_t));
        memcpy(new_values, parser->values, parser->arg_count * sizeof(coil_arg_value_t));
        memcpy(new_provided, parser->provided, parser->arg_count * sizeof(bool));
    }
    
    parser->args = new_args;
    parser->values = new_values;
    parser->provided = new_provided;
    parser->arg_capacity = new_capacity;
    
    return 0;
}

int coil_arg_add_flag(coil_arg_parser_t *parser, 
                     char short_name,
                     const char *long_name,
                     const char *description,
                     bool default_value) {
    if (!parser || !parser->initialized) return -1;
    
    /* Check if we need more capacity */
    if (parser->arg_count >= parser->arg_capacity) {
        if (coil_arg_ensure_capacity(parser) != 0) {
            return -1;
        }
    }
    
    /* Initialize the argument */
    coil_arg_def_t *arg = &parser->args[parser->arg_count];
    arg->short_name = short_name;
    arg->long_name = long_name ? coil_memory_arena_strdup(parser->arena, long_name) : NULL;
    arg->description = description ? coil_memory_arena_strdup(parser->arena, description) : NULL;
    arg->type = COIL_ARG_FLAG;
    arg->required = false;
    arg->default_value.flag = default_value;
    arg->metavar = NULL;
    
    /* Set default value */
    parser->values[parser->arg_count].flag = default_value;
    parser->provided[parser->arg_count] = false;
    
    parser->arg_count++;
    
    return 0;
}

int coil_arg_add_string(coil_arg_parser_t *parser, 
                       char short_name,
                       const char *long_name,
                       const char *description,
                       const char *metavar,
                       const char *default_value,
                       bool required) {
    if (!parser || !parser->initialized) return -1;
    
    /* Check if we need more capacity */
    if (parser->arg_count >= parser->arg_capacity) {
        if (coil_arg_ensure_capacity(parser) != 0) {
            return -1;
        }
    }
    
    /* Initialize the argument */
    coil_arg_def_t *arg = &parser->args[parser->arg_count];
    arg->short_name = short_name;
    arg->long_name = long_name ? coil_memory_arena_strdup(parser->arena, long_name) : NULL;
    arg->description = description ? coil_memory_arena_strdup(parser->arena, description) : NULL;
    arg->type = COIL_ARG_STRING;
    arg->required = required;
    arg->default_value.string = default_value ? 
        coil_memory_arena_strdup(parser->arena, default_value) : NULL;
    arg->metavar = metavar ? 
        coil_memory_arena_strdup(parser->arena, metavar) : 
        coil_memory_arena_strdup(parser->arena, "STRING");
    
    /* Set default value */
    parser->values[parser->arg_count].string = arg->default_value.string;
    parser->provided[parser->arg_count] = false;
    
    parser->arg_count++;
    
    return 0;
}

int coil_arg_add_int(coil_arg_parser_t *parser, 
                    char short_name,
                    const char *long_name,
                    const char *description,
                    const char *metavar,
                    int default_value,
                    bool required) {
    if (!parser || !parser->initialized) return -1;
    
    /* Check if we need more capacity */
    if (parser->arg_count >= parser->arg_capacity) {
        if (coil_arg_ensure_capacity(parser) != 0) {
            return -1;
        }
    }
    
    /* Initialize the argument */
    coil_arg_def_t *arg = &parser->args[parser->arg_count];
    arg->short_name = short_name;
    arg->long_name = long_name ? coil_memory_arena_strdup(parser->arena, long_name) : NULL;
    arg->description = description ? coil_memory_arena_strdup(parser->arena, description) : NULL;
    arg->type = COIL_ARG_INT;
    arg->required = required;
    arg->default_value.integer = default_value;
    arg->metavar = metavar ? 
        coil_memory_arena_strdup(parser->arena, metavar) : 
        coil_memory_arena_strdup(parser->arena, "NUMBER");
    
    /* Set default value */
    parser->values[parser->arg_count].integer = default_value;
    parser->provided[parser->arg_count] = false;
    
    parser->arg_count++;
    
    return 0;
}

int coil_arg_add_float(coil_arg_parser_t *parser, 
                      char short_name,
                      const char *long_name,
                      const char *description,
                      const char *metavar,
                      float default_value,
                      bool required) {
    if (!parser || !parser->initialized) return -1;
    
    /* Check if we need more capacity */
    if (parser->arg_count >= parser->arg_capacity) {
        if (coil_arg_ensure_capacity(parser) != 0) {
            return -1;
        }
    }
    
    /* Initialize the argument */
    coil_arg_def_t *arg = &parser->args[parser->arg_count];
    arg->short_name = short_name;
    arg->long_name = long_name ? coil_memory_arena_strdup(parser->arena, long_name) : NULL;
    arg->description = description ? coil_memory_arena_strdup(parser->arena, description) : NULL;
    arg->type = COIL_ARG_FLOAT;
    arg->required = required;
    arg->default_value.floating = default_value;
    arg->metavar = metavar ? 
        coil_memory_arena_strdup(parser->arena, metavar) : 
        coil_memory_arena_strdup(parser->arena, "NUMBER");
    
    /* Set default value */
    parser->values[parser->arg_count].floating = default_value;
    parser->provided[parser->arg_count] = false;
    
    parser->arg_count++;
    
    return 0;
}

int coil_arg_add_positional(coil_arg_parser_t *parser, 
                           const char *name,
                           const char *description,
                           bool required) {
    if (!parser || !parser->initialized) return -1;
    
    /* Check if we need more capacity */
    if (parser->arg_count >= parser->arg_capacity) {
        if (coil_arg_ensure_capacity(parser) != 0) {
            return -1;
        }
    }
    
    /* Initialize the argument */
    coil_arg_def_t *arg = &parser->args[parser->arg_count];
    arg->short_name = 0;
    arg->long_name = name ? coil_memory_arena_strdup(parser->arena, name) : NULL;
    arg->description = description ? coil_memory_arena_strdup(parser->arena, description) : NULL;
    arg->type = COIL_ARG_POSITIONAL;
    arg->required = required;
    arg->default_value.string = NULL;
    arg->metavar = name ? 
        coil_memory_arena_strdup(parser->arena, name) : 
        coil_memory_arena_strdup(parser->arena, "ARG");
    
    /* Set default value */
    parser->values[parser->arg_count].string = NULL;
    parser->provided[parser->arg_count] = false;
    
    parser->arg_count++;
    
    return 0;
}

bool coil_arg_parse(coil_arg_parser_t *parser, int argc, char **argv) {
    if (!parser || !parser->initialized || argc == 0 || !argv) return false;
    
    /* If we have any positional args, prepare to collect them */
    size_t max_positional = 0;
    for (size_t i = 0; i < parser->arg_count; i++) {
        if (parser->args[i].type == COIL_ARG_POSITIONAL) {
            max_positional++;
        }
    }
    
    parser->positional_args = coil_memory_arena_calloc(parser->arena, 
                                                      max_positional, 
                                                      sizeof(char *));
    if (!parser->positional_args && max_positional > 0) {
        return false;
    }
    
    parser->positional_count = 0;
    
    /* Skip program name */
    int i = 1;
    
    /* Process all arguments */
    while (i < argc) {
        char *arg = argv[i];
        
        /* Check if it's an option */
        if (arg[0] == '-') {
            /* Handle '--' (end of options) */
            if (strcmp(arg, "--") == 0) {
                i++;
                break;  /* Everything after this is a positional argument */
            }
            
            /* Look up the argument */
            int arg_index = coil_arg_find(parser, arg);
            
            if (arg_index < 0) {
                if (parser->error_mgr) {
                    coil_stream_pos_t pos = {0};
                    pos.file_name = "args";
                    coil_error_error(parser->error_mgr, COIL_ERR_ARGUMENT, &pos, 
                                   "Unknown argument: %s", arg);
                }
                return false;
            }
            
            coil_arg_def_t *arg_def = &parser->args[arg_index];
            
            /* Mark as provided */
            parser->provided[arg_index] = true;
            
            /* Handle based on type */
            switch (arg_def->type) {
                case COIL_ARG_FLAG:
                    /* Flags just need to be set to true */
                    parser->values[arg_index].flag = true;
                    
                    /* Check if it's help */
                    if ((arg_def->short_name == 'h' && strcmp(arg, "-h") == 0) ||
                        (arg_def->long_name && strcmp(arg_def->long_name, "help") == 0)) {
                        parser->help_requested = true;
                    }
                    break;
                    
                case COIL_ARG_STRING:
                case COIL_ARG_INT:
                case COIL_ARG_FLOAT:
                    /* These all need a value, either in the next argument or after = */
                    if (i + 1 >= argc) {
                        if (parser->error_mgr) {
                            coil_stream_pos_t pos = {0};
                            pos.file_name = "args";
                            coil_error_error(parser->error_mgr, COIL_ERR_ARGUMENT, &pos, 
                                           "Missing value for argument: %s", arg);
                        }
                        return false;
                    }
                    
                    char *value = argv[++i];
                    
                    if (arg_def->type == COIL_ARG_STRING) {
                        parser->values[arg_index].string = 
                            coil_memory_arena_strdup(parser->arena, value);
                    } else if (arg_def->type == COIL_ARG_INT) {
                        char *endptr;
                        parser->values[arg_index].integer = (int)strtol(value, &endptr, 0);
                        
                        if (*endptr != '\0') {
                            if (parser->error_mgr) {
                                coil_stream_pos_t pos = {0};
                                pos.file_name = "args";
                                coil_error_error(parser->error_mgr, COIL_ERR_ARGUMENT, &pos, 
                                               "Invalid integer value for argument %s: %s", 
                                               arg, value);
                            }
                            return false;
                        }
                    } else if (arg_def->type == COIL_ARG_FLOAT) {
                        char *endptr;
                        parser->values[arg_index].floating = strtof(value, &endptr);
                        
                        if (*endptr != '\0') {
                            if (parser->error_mgr) {
                                coil_stream_pos_t pos = {0};
                                pos.file_name = "args";
                                coil_error_error(parser->error_mgr, COIL_ERR_ARGUMENT, &pos, 
                                               "Invalid float value for argument %s: %s", 
                                               arg, value);
                            }
                            return false;
                        }
                    }
                    break;
                    
                case COIL_ARG_POSITIONAL:
                    /* Should not get here, positional args don't start with - */
                    break;
            }
        } else {
            /* Positional argument */
            if (parser->positional_count < max_positional) {
                /* Find the next positional argument definition */
                size_t pos_index = 0;
                for (size_t j = 0; j < parser->arg_count; j++) {
                    if (parser->args[j].type == COIL_ARG_POSITIONAL) {
                        if (pos_index == parser->positional_count) {
                            /* This is the one we want */
                            parser->values[j].string = 
                                coil_memory_arena_strdup(parser->arena, arg);
                            parser->provided[j] = true;
                            break;
                        }
                        pos_index++;
                    }
                }
                
                parser->positional_args[parser->positional_count++] = arg;
            } else {
                if (parser->error_mgr) {
                    coil_stream_pos_t pos = {0};
                    pos.file_name = "args";
                    coil_error_error(parser->error_mgr, COIL_ERR_ARGUMENT, &pos, 
                                   "Too many positional arguments, got: %s", arg);
                }
                return false;
            }
        }
        
        i++;
    }
    
    /* Handle any remaining arguments as positional (after --) */
    while (i < argc) {
        if (parser->positional_count < max_positional) {
            /* Find the next positional argument definition */
            size_t pos_index = 0;
            for (size_t j = 0; j < parser->arg_count; j++) {
                if (parser->args[j].type == COIL_ARG_POSITIONAL) {
                    if (pos_index == parser->positional_count) {
                        /* This is the one we want */
                        parser->values[j].string = 
                            coil_memory_arena_strdup(parser->arena, argv[i]);
                        parser->provided[j] = true;
                        break;
                    }
                    pos_index++;
                }
            }
            
            parser->positional_args[parser->positional_count++] = argv[i];
        } else {
            if (parser->error_mgr) {
                coil_stream_pos_t pos = {0};
                pos.file_name = "args";
                coil_error_error(parser->error_mgr, COIL_ERR_ARGUMENT, &pos, 
                               "Too many positional arguments, got: %s", argv[i]);
            }
            return false;
        }
        
        i++;
    }
    
    return true;
}

void coil_arg_print_help(coil_arg_parser_t *parser) {
    if (!parser || !parser->initialized) return;
    
    /* Calculate the length of the longest argument */
    size_t max_arg_len = 0;
    for (size_t i = 0; i < parser->arg_count; i++) {
        size_t len = 0;
        coil_arg_def_t *arg = &parser->args[i];
        
        if (arg->type == COIL_ARG_POSITIONAL) {
            len = strlen(arg->metavar);
        } else {
            len = arg->short_name ? 2 : 0;  /* -x */
            len += (arg->short_name && arg->long_name) ? 2 : 0;  /* , */
            len += arg->long_name ? 2 + strlen(arg->long_name) : 0;  /* --long */
            
            if (arg->type != COIL_ARG_FLAG) {
                len += 1 + strlen(arg->metavar);  /* =META */
            }
        }
        
        if (len > max_arg_len) {
            max_arg_len = len;
        }
    }
    
    /* Add padding */
    max_arg_len += 2;
    
    /* Print usage */
    printf("Usage: %s", parser->program_name ? parser->program_name : "program");
    
    /* Print options in usage */
    for (size_t i = 0; i < parser->arg_count; i++) {
        coil_arg_def_t *arg = &parser->args[i];
        
        if (arg->type != COIL_ARG_POSITIONAL) {
            if (arg->required) {
                printf(" ");
            } else {
                printf(" [");
            }
            
            if (arg->short_name) {
                printf("-%c", arg->short_name);
            } else if (arg->long_name) {
                printf("--%s", arg->long_name);
            }
            
            if (arg->type != COIL_ARG_FLAG) {
                printf(" %s", arg->metavar);
            }
            
            if (!arg->required) {
                printf("]");
            }
        }
    }
    
    /* Print positional arguments in usage */
    for (size_t i = 0; i < parser->arg_count; i++) {
        coil_arg_def_t *arg = &parser->args[i];
        
        if (arg->type == COIL_ARG_POSITIONAL) {
            if (arg->required) {
                printf(" %s", arg->metavar);
            } else {
                printf(" [%s]", arg->metavar);
            }
        }
    }
    
    printf("\n\n");
    
    /* Print description */
    if (parser->program_description) {
        printf("%s\n\n", parser->program_description);
    }
    
    /* Print arguments */
    printf("Options:\n");
    
    for (size_t i = 0; i < parser->arg_count; i++) {
        coil_arg_def_t *arg = &parser->args[i];
        
        printf("  ");
        
        size_t len = 0;
        
        if (arg->type == COIL_ARG_POSITIONAL) {
            printf("%s", arg->metavar);
            len = strlen(arg->metavar);
        } else {
            if (arg->short_name) {
                printf("-%c", arg->short_name);
                len += 2;
            }
            
            if (arg->short_name && arg->long_name) {
                printf(", ");
                len += 2;
            }
            
            if (arg->long_name) {
                printf("--%s", arg->long_name);
                len += 2 + strlen(arg->long_name);
            }
            
            if (arg->type != COIL_ARG_FLAG) {
                printf("=%s", arg->metavar);
                len += 1 + strlen(arg->metavar);
            }
        }
        
        /* Pad to align descriptions */
        for (size_t j = len; j < max_arg_len; j++) {
            printf(" ");
        }
        
        if (arg->description) {
            printf("%s", arg->description);
        }
        
        /* For non-flags, show default values */
        if (arg->type != COIL_ARG_FLAG && arg->type != COIL_ARG_POSITIONAL && !arg->required) {
            switch (arg->type) {
                case COIL_ARG_STRING:
                    if (arg->default_value.string) {
                        printf(" (default: \"%s\")", arg->default_value.string);
                    } else {
                        printf(" (default: NULL)");
                    }
                    break;
                case COIL_ARG_INT:
                    printf(" (default: %d)", arg->default_value.integer);
                    break;
                case COIL_ARG_FLOAT:
                    printf(" (default: %g)", arg->default_value.floating);
                    break;
                default:
                    break;
            }
        }
        
        printf("\n");
    }
    
    /* Print epilog */
    if (parser->epilog) {
        printf("\n%s\n", parser->epilog);
    }
}

bool coil_arg_get_flag(coil_arg_parser_t *parser, const char *name) {
    if (!parser || !parser->initialized || !name) return false;
    
    int index = coil_arg_find(parser, name);
    if (index < 0 || parser->args[index].type != COIL_ARG_FLAG) {
        return false;
    }
    
    return parser->values[index].flag;
}

const char *coil_arg_get_string(coil_arg_parser_t *parser, const char *name) {
    if (!parser || !parser->initialized || !name) return NULL;
    
    int index = coil_arg_find(parser, name);
    if (index < 0 || parser->args[index].type != COIL_ARG_STRING) {
        return NULL;
    }
    
    return parser->values[index].string;
}

int coil_arg_get_int(coil_arg_parser_t *parser, const char *name) {
    if (!parser || !parser->initialized || !name) return 0;
    
    int index = coil_arg_find(parser, name);
    if (index < 0 || parser->args[index].type != COIL_ARG_INT) {
        return 0;
    }
    
    return parser->values[index].integer;
}

float coil_arg_get_float(coil_arg_parser_t *parser, const char *name) {
    if (!parser || !parser->initialized || !name) return 0.0f;
    
    int index = coil_arg_find(parser, name);
    if (index < 0 || parser->args[index].type != COIL_ARG_FLOAT) {
        return 0.0f;
    }
    
    return parser->values[index].floating;
}

bool coil_arg_provided(coil_arg_parser_t *parser, const char *name) {
    if (!parser || !parser->initialized || !name) return false;
    
    int index = coil_arg_find(parser, name);
    if (index < 0) {
        return false;
    }
    
    return parser->provided[index];
}

const char **coil_arg_get_positional(coil_arg_parser_t *parser, size_t *count) {
    if (!parser || !parser->initialized || !count) {
        if (count) *count = 0;
        return NULL;
    }
    
    *count = parser->positional_count;
    return (const char **)parser->positional_args;
}

bool coil_arg_help_requested(coil_arg_parser_t *parser) {
    if (!parser || !parser->initialized) return false;
    
    return parser->help_requested;
}

bool coil_arg_validate(coil_arg_parser_t *parser) {
    if (!parser || !parser->initialized) return false;
    
    bool valid = true;
    
    /* Check that all required arguments are provided */
    for (size_t i = 0; i < parser->arg_count; i++) {
        if (parser->args[i].required && !parser->provided[i]) {
            if (parser->error_mgr) {
                coil_stream_pos_t pos = {0};
                pos.file_name = "args";
                
                if (parser->args[i].type == COIL_ARG_POSITIONAL) {
                    coil_error_error(parser->error_mgr, COIL_ERR_ARGUMENT, &pos, 
                                   "Missing required positional argument: %s", 
                                   parser->args[i].metavar);
                } else {
                    coil_error_error(parser->error_mgr, COIL_ERR_ARGUMENT, &pos, 
                                   "Missing required argument: %s%s", 
                                   parser->args[i].long_name ? "--" : "-",
                                   parser->args[i].long_name ? 
                                   parser->args[i].long_name : 
                                   (char[]){parser->args[i].short_name, '\0'});
                }
            }
            
            valid = false;
        }
    }
    
    return valid;
}

void coil_arg_parser_cleanup(coil_arg_parser_t *parser) {
    if (!parser || !parser->initialized) return;
    
    /* We don't need to free memory because it's all allocated from the arena */
    parser->initialized = false;
}

void coil_arg_parser_destroy(coil_arg_parser_t *parser) {
    if (!parser) return;
    
    coil_arg_parser_cleanup(parser);
    
    /* No need to free the parser itself, it's allocated from the arena */
}

void coil_arg_add_standard_args(coil_arg_parser_t *parser) {
    if (!parser || !parser->initialized) return;
    
    /* Add help argument */
    coil_arg_add_flag(parser, 'h', "help", "Show this help message and exit", false);
    
    /* Add verbose argument */
    coil_arg_add_flag(parser, 'v', "verbose", "Enable verbose output", false);
    
    /* Add quiet argument */
    coil_arg_add_flag(parser, 'q', "quiet", "Suppress all output except errors", false);
}
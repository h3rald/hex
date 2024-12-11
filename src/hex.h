
#ifndef HEX_H
#define HEX_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <signal.h>
#include <stdarg.h>
#include <stdint.h>

#ifdef _WIN32
#include <windows.h>
#include <io.h>
int isatty(int fd);
#else
#include <unistd.h>
#include <sys/wait.h>
#endif

// Constants
#define HEX_VERSION "0.1.0"
#define HEX_STDIN_BUFFER_SIZE 256
#define HEX_REGISTRY_SIZE 1024
#define HEX_STACK_SIZE 128
#define HEX_STACK_TRACE_SIZE 16
#define HEX_NATIVE_SYMBOLS 64

// Type Definitions
typedef enum hex_item_type_t
{
    HEX_TYPE_INTEGER,
    HEX_TYPE_STRING,
    HEX_TYPE_QUOTATION,
    HEX_TYPE_NATIVE_SYMBOL,
    HEX_TYPE_USER_SYMBOL,
    HEX_TYPE_INVALID
} hex_item_type_t;

typedef enum hex_token_type_t
{
    HEX_TOKEN_INTEGER,
    HEX_TOKEN_STRING,
    HEX_TOKEN_SYMBOL,
    HEX_TOKEN_QUOTATION_START,
    HEX_TOKEN_QUOTATION_END,
    HEX_TOKEN_COMMENT,
    HEX_TOKEN_INVALID
} hex_token_type_t;

typedef struct hex_file_position_t
{
    const char *filename;
    int line;
    int column;
} hex_file_position_t;

typedef struct hex_token_t
{
    hex_token_type_t type;
    char *value;
    hex_file_position_t position;
} hex_token_t;

typedef struct hex_context_t hex_context_t;

typedef struct hex_item_t
{
    hex_item_type_t type;
    union
    {
        int32_t int_value;
        char *str_value;
        int (*fn_value)(hex_context_t *);
        struct hex_item_t **quotation_value;
    } data;
    hex_token_t *token; // Token containing stack information (valid for HEX_TYPE_NATIVE_SYMBOL and HEX_TYPE_USER_SYMBOL)
    int quotation_size; // Size of the quotation (valid for HEX_TYPE_QUOTATION)
} hex_item_t;

typedef struct hex_registry_entry
{
    char *key;
    hex_item_t value;
} hex_registry_entry_t;

typedef struct hex_stack_trace_t
{
    hex_token_t entries[HEX_STACK_TRACE_SIZE];
    int start; // Index of the oldest item
    int size;  // Current number of items in the buffer
} hex_stack_trace_t;

typedef struct hex_stack_t
{
    hex_item_t entries[HEX_STACK_SIZE];
    int top;
} hex_stack_t;

typedef struct hex_registry_t
{
    hex_registry_entry_t entries[HEX_REGISTRY_SIZE];
    int size;
} hex_registry_t;

typedef struct hex_doc_entry_t
{
    const char *name;
    const char *description;
    const char *input;
    const char *output;
} hex_doc_entry_t;

typedef struct hex_doc_dictionary_t
{
    hex_doc_entry_t entries[64];
    int size;
} hex_doc_dictionary_t;

typedef struct hex_settings_t
{
    int debugging_enabled;
    int errors_enabled;
    int stack_trace_enabled;
} hex_settings_t;

typedef struct hex_context_t
{
    hex_stack_t stack;
    hex_registry_t registry;
    hex_stack_trace_t stack_trace;
    hex_settings_t settings;
    hex_doc_dictionary_t docs;
    int hashbang;
    char error[256];
    int argc;
    char **argv;
} hex_context_t;

// Help System
void hex_doc(hex_doc_dictionary_t *docs, const char *name, const char *description, const char *input, const char *output);
int hex_get_doc(hex_doc_dictionary_t *docs, const char *key, hex_doc_entry_t *result);
void hex_create_docs(hex_doc_dictionary_t *docs);
void hex_print_help();
void hex_print_docs(hex_doc_dictionary_t *docs);

// Free data
void hex_free_item(hex_context_t *ctx, hex_item_t item);
void hex_free_token(hex_token_t *token);
void hex_free_list(hex_context_t *ctx, hex_item_t **quotation, int size);

// Symbol management
int hex_valid_user_symbol(hex_context_t *ctx, const char *symbol);
int hex_valid_native_symbol(hex_context_t *ctx, const char *symbol);
int hex_set_symbol(hex_context_t *ctx, const char *key, hex_item_t value, int native);
void hex_set_native_symbol(hex_context_t *ctx, const char *name, int (*func)());
int hex_get_symbol(hex_context_t *ctx, const char *key, hex_item_t *result);

// Errors and debugging
void hex_error(hex_context_t *ctx, const char *format, ...);
void hex_debug(hex_context_t *ctx, const char *format, ...);
void hex_debug_item(hex_context_t *ctx, const char *message, hex_item_t item);
void hex_print_item(FILE *stream, hex_item_t item);
void add_to_stack_trace(hex_context_t *ctx, hex_token_t *token);
void print_stack_trace(hex_context_t *ctx);

// Item constructors
hex_item_t hex_string_item(hex_context_t *ctx, const char *value);
hex_item_t hex_integer_item(hex_context_t *ctx, int value);
hex_item_t hex_quotation_item(hex_context_t *ctx, hex_item_t **quotation, int size);

// Stack management
int hex_push(hex_context_t *ctx, hex_item_t item);
int hex_push_integer(hex_context_t *ctx, int value);
int hex_push_string(hex_context_t *ctx, const char *value);
int hex_push_quotation(hex_context_t *ctx, hex_item_t **quotation, int size);
int hex_push_symbol(hex_context_t *ctx, hex_token_t *token);
hex_item_t hex_pop(hex_context_t *ctx);

// Parser and interpreter
char *hex_process_string(hex_context_t *ctx, const char *value);
hex_token_t *hex_next_token(hex_context_t *ctx, const char **input, hex_file_position_t *position);
int32_t hex_parse_integer(const char *hex_str);
int hex_parse_quotation(hex_context_t *ctx, const char **input, hex_item_t *result, hex_file_position_t *position);
int hex_interpret(hex_context_t *ctx, const char *code, const char *filename, int line, int column);

// Helpers
char *hex_itoa(int num, int base);
char *hex_itoa_dec(int num);
char *hex_itoa_hex(int num);
void hex_raw_print_item(FILE *stream, hex_item_t item);
int hex_is_symbol(hex_token_t *token, char *value);
char *hex_type(hex_item_type_t type);

// Native symbols
int hex_symbol_store(hex_context_t *ctx);
int hex_symbol_free(hex_context_t *ctx);
int hex_symbol_type(hex_context_t *ctx);
int hex_symbol_i(hex_context_t *ctx);
int hex_symbol_eval(hex_context_t *ctx);
int hex_symbol_puts(hex_context_t *ctx);
int hex_symbol_warn(hex_context_t *ctx);
int hex_symbol_print(hex_context_t *ctx);
int hex_symbol_gets(hex_context_t *ctx);
int hex_symbol_add(hex_context_t *ctx);
int hex_symbol_subtract(hex_context_t *ctx);
int hex_symbol_multiply(hex_context_t *ctx);
int hex_symbol_divide(hex_context_t *ctx);
int hex_symbol_modulo(hex_context_t *ctx);
int hex_symbol_bitand(hex_context_t *ctx);
int hex_symbol_bitor(hex_context_t *ctx);
int hex_symbol_bitxor(hex_context_t *ctx);
int hex_symbol_bitnot(hex_context_t *ctx);
int hex_symbol_shiftleft(hex_context_t *ctx);
int hex_symbol_shiftright(hex_context_t *ctx);
int hex_symbol_int(hex_context_t *ctx);
int hex_symbol_str(hex_context_t *ctx);
int hex_symbol_dec(hex_context_t *ctx);
int hex_symbol_hex(hex_context_t *ctx);
int hex_symbol_equal(hex_context_t *ctx);
int hex_symbol_notequal(hex_context_t *ctx);
int hex_symbol_greater(hex_context_t *ctx);
int hex_symbol_less(hex_context_t *ctx);
int hex_symbol_greaterequal(hex_context_t *ctx);
int hex_symbol_lessequal(hex_context_t *ctx);
int hex_symbol_and(hex_context_t *ctx);
int hex_symbol_or(hex_context_t *ctx);
int hex_symbol_not(hex_context_t *ctx);
int hex_symbol_xor(hex_context_t *ctx);
int hex_symbol_cat(hex_context_t *ctx);
int hex_symbol_slice(hex_context_t *ctx);
int hex_symbol_len(hex_context_t *ctx);
int hex_symbol_get(hex_context_t *ctx);
int hex_symbol_insert(hex_context_t *ctx);
int hex_symbol_index(hex_context_t *ctx);
int hex_symbol_join(hex_context_t *ctx);
int hex_symbol_split(hex_context_t *ctx);
int hex_symbol_replace(hex_context_t *ctx);
int hex_symbol_read(hex_context_t *ctx);
int hex_symbol_write(hex_context_t *ctx);
int hex_symbol_append(hex_context_t *ctx);
int hex_symbol_args(hex_context_t *ctx);
int hex_symbol_exit(hex_context_t *ctx);
int hex_symbol_exec(hex_context_t *ctx);
int hex_symbol_run(hex_context_t *ctx);
int hex_symbol_if(hex_context_t *ctx);
int hex_symbol_when(hex_context_t *ctx);
int hex_symbol_while(hex_context_t *ctx);
int hex_symbol_each(hex_context_t *ctx);
int hex_symbol_error(hex_context_t *ctx);
int hex_symbol_try(hex_context_t *ctx);
int hex_symbol_q(hex_context_t *ctx);
int hex_symbol_map(hex_context_t *ctx);
int hex_symbol_filter(hex_context_t *ctx);
int hex_symbol_swap(hex_context_t *ctx);
int hex_symbol_dup(hex_context_t *ctx);
int hex_symbol_stack(hex_context_t *ctx);
int hex_symbol_clear(hex_context_t *ctx);
int hex_symbol_pop(hex_context_t *ctx);

// REPL and initialization
void hex_register_symbols(hex_context_t *ctx);
hex_context_t hex_init();
void hex_repl(hex_context_t *ctx);
void hex_process_stdin(hex_context_t *ctx);
void hex_handle_sigint(int sig);
char *hex_read_file(hex_context_t *ctx, const char *filename);

#endif // HEX_H

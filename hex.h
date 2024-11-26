
#ifndef HEX_H
#define HEX_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <signal.h>
#include <stdarg.h>

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
    HEX_TOKEN_NUMBER,
    HEX_TOKEN_STRING,
    HEX_TOKEN_SYMBOL,
    HEX_TOKEN_QUOTATION_START,
    HEX_TOKEN_QUOTATION_END,
    HEX_TOKEN_COMMENT,
    HEX_TOKEN_INVALID
} hex_token_type_t;

typedef struct hex_token_t
{
    hex_token_type_t type;
    char *value;
    char *filename;
    int line;
    int column;
} hex_token_t;

typedef struct hex_item_t
{
    hex_item_type_t type;
    union
    {
        int intValue;
        char *strValue;
        int (*functionPointer)();
        struct hex_item_t **quotationValue;
    } data;
    hex_token_t *token; // Token containing stack information (valid for HEX_TYPE_NATIVE_SYMBOL and HEX_TYPE_USER_SYMBOL)
    int quotationSize;  // Size of the quotation (valid for HEX_TYPE_QUOTATION)
} hex_item_t;

typedef struct hex_registry_entry
{
    char *key;
    hex_item_t value;
} hex_registry_entry_t;

typedef struct hex_stack_trace_t
{
    hex_token_t entries[HEX_STACK_TRACE_SIZE];
    int start; // Index of the oldest element
    int size;  // Current number of elements in the buffer
} hex_stack_trace_t;

// TODO: refactor and use this
typedef struct hex_stack_t
{
    hex_item_t item[HEX_STACK_SIZE];
    int top;
} hex_stack_t;

// TODO: refactor and use this
typedef struct hex_registry_t
{
    hex_registry_entry_t entries[HEX_REGISTRY_SIZE];
    int size;
} hex_registry_t;

// TODO: refactor and use this
typedef struct hex_settings_t
{
    int debugging_enabled;
    int errors_enabled;
    int stack_trace_enabled;
} hex_settings_t;

// TODO: refactor and use this
typedef struct hex_context_t
{
    hex_stack_t stack;
    hex_registry_t registry;
    hex_stack_trace_t stack_trace;
    hex_settings_t settings;
    char *error;
    int argc;
    char **argv;
} hex_context_t;

// Functions

void hex_free_element(hex_item_t element);
void hex_free_token(hex_token_t *token);
void hex_free_list(hex_item_t **quotation, int size);

int hex_valid_user_symbol(const char *symbol);
int hex_valid_native_symbol(const char *symbol);
int hex_set_symbol(const char *key, hex_item_t value, int native);
void hex_set_native_symbol(const char *name, int (*func)());
int hex_get_symbol(const char *key, hex_item_t *result);

void hex_error(const char *format, ...);
void hex_debug(const char *format, ...);
void hex_debug_element(const char *message, hex_item_t element);
void hex_print_element(FILE *stream, hex_item_t element);
void add_to_stack_trace(hex_token_t *token);
char *hex_type(hex_item_type_t type);

int hex_push(hex_item_t element);
int hex_push_int(int value);
int hex_push_string(const char *value);
int hex_push_quotation(hex_item_t **quotation, int size);
int hex_push_symbol(hex_token_t *token);
hex_item_t hex_pop();

char *hex_process_string(const char *value);

hex_token_t *hex_next_token(const char **input, int *line, int *column);

void print_stack_trace();

char *hex_itoa(int num, int base);
char *hex_itoa_dec(int num);
char *hex_itoa_hex(int num);

void hex_raw_print_element(FILE *stream, hex_item_t element);

int hex_is_symbol(hex_token_t *token, char *value);

int hex_symbol_store();
int hex_symbol_free();
int hex_symbol_type();
int hex_symbol_i();
int hex_symbol_eval();
int hex_symbol_puts();
int hex_symbol_warn();
int hex_symbol_print();
int hex_symbol_gets();
int hex_symbol_add();
int hex_symbol_subtract();
int hex_symbol_multiply();
int hex_symbol_divide();
int hex_symbol_modulo();
int hex_symbol_bitand();
int hex_symbol_bitor();
int hex_symbol_bitxor();
int hex_symbol_bitnot();
int hex_symbol_shiftleft();
int hex_symbol_shiftright();
int hex_symbol_int();
int hex_symbol_str();
int hex_symbol_dec();
int hex_symbol_hex();
int hex_symbol_equal();
int hex_symbol_notequal();
int hex_symbol_greater();
int hex_symbol_less();
int hex_symbol_greaterequal();
int hex_symbol_lessequal();
int hex_symbol_and();
int hex_symbol_or();
int hex_symbol_not();
int hex_symbol_xor();
int hex_symbol_cat();
int hex_symbol_slice();
int hex_symbol_len();
int hex_symbol_get();
int hex_symbol_insert();
int hex_symbol_index();
int hex_symbol_join();
int hex_symbol_split();
int hex_symbol_replace();
int hex_symbol_read();
int hex_symbol_write();
int hex_symbol_append();
int hex_symbol_args();
int hex_symbol_exit();
int hex_symbol_exec();
int hex_symbol_run();
int hex_symbol_if();
int hex_symbol_when();
int hex_symbol_while();
int hex_symbol_each();
int hex_symbol_error();
int hex_symbol_try();
int hex_symbol_q();
int hex_symbol_map();
int hex_symbol_filter();
int hex_symbol_swap();
int hex_symbol_dup();
int hex_symbol_stack();
int hex_symbol_clear();
int hex_symbol_pop();

void hex_register_symbols();

void hex_repl();
void hex_process_stdin();
void hex_handle_sigint(int sig);
char *hex_read_file(const char *filename);

int hex_parse_quotation(const char **input, hex_item_t *result, const char *filename, int *line, int *column);
int hex_interpret(const char *code, const char *filename, int line, int column);

#endif // HEX_H

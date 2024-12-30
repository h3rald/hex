
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
#define HEX_VERSION "0.3.0"
#define HEX_STDIN_BUFFER_SIZE 16384
#define HEX_INITIAL_REGISTRY_SIZE 128
#define HEX_REGISTRY_SIZE 1024
#define HEX_STACK_SIZE 256
#define HEX_STACK_TRACE_SIZE 16
#define HEX_NATIVE_SYMBOLS 64
#define HEX_MAX_SYMBOL_LENGTH 256
#define HEX_MAX_USER_SYMBOLS (HEX_REGISTRY_SIZE - HEX_NATIVE_SYMBOLS)

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
    size_t quotation_size;
    hex_file_position_t *position;
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
    int operator;
    hex_token_t *token;    // Token containing stack information (valid for HEX_TYPE_NATIVE_SYMBOL and HEX_TYPE_USER_SYMBOL)
    size_t quotation_size; // Size of the quotation (valid for HEX_TYPE_QUOTATION)
} hex_item_t;

typedef struct hex_registry_entry
{
    char *key;
    hex_item_t *value;
} hex_registry_entry_t;

typedef struct hex_stack_trace_t
{
    hex_token_t **entries;
    int start;   // Index of the oldest item
    size_t size; // Current number of items in the buffer
} hex_stack_trace_t;

typedef struct hex_stack_t
{
    hex_item_t **entries;
    int top;
    size_t capacity;
} hex_stack_t;

typedef struct hex_registry_t
{
    hex_registry_entry_t **entries;
    size_t size;
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
    hex_doc_entry_t **entries;
    size_t size;
} hex_doc_dictionary_t;

typedef struct hex_settings_t
{
    int debugging_enabled;
    int errors_enabled;
    int stack_trace_enabled;
} hex_settings_t;

typedef struct hex_symbol_table_t
{
    char **symbols;
    uint16_t count;
} hex_symbol_table_t;

typedef struct hex_context_t
{
    hex_stack_t *stack;
    hex_registry_t *registry;
    hex_stack_trace_t *stack_trace;
    hex_settings_t *settings;
    hex_doc_dictionary_t *docs;
    hex_symbol_table_t *symbol_table;
    int hashbang;
    char error[256];
    int argc;
    char **argv;
} hex_context_t;

// Opcodes
typedef enum hex_opcode_t
{
    // Core Operations: <op> [prefix] <len> <data>
    HEX_OP_LOOKUP = 0x00,
    HEX_OP_PUSHIN = 0x01,
    HEX_OP_PUSHST = 0x02,
    HEX_OP_PUSHQT = 0x03,

    // Native Symbols
    HEX_OP_STORE = 0x10,
    HEX_OP_DEFINE = 0x11,
    HEX_OP_FREE = 0x12,
    HEX_OP_SYMBOLS = 0x13,

    HEX_OP_IF = 0x14,
    HEX_OP_WHEN = 0x15,
    HEX_OP_WHILE = 0x16,
    HEX_OP_ERROR = 0x17,
    HEX_OP_TRY = 0x18,
    HEX_OP_THROW = 0x19,

    HEX_OP_DUP = 0x1a,
    HEX_OP_STACK = 0x1b,
    HEX_OP_POP = 0x1c,
    HEX_OP_SWAP = 0x1d,

    HEX_OP_I = 0x1e,
    HEX_OP_EVAL = 0x1f,
    HEX_OP_QUOTE = 0x20,

    HEX_OP_ADD = 0x21,
    HEX_OP_SUB = 0x22,
    HEX_OP_MUL = 0x23,
    HEX_OP_DIV = 0x24,
    HEX_OP_MOD = 0x25,

    HEX_OP_BITAND = 0x26,
    HEX_OP_BITOR = 0x27,
    HEX_OP_BITXOR = 0x28,
    HEX_OP_BITNOT = 0x29,
    HEX_OP_SHL = 0x2a,
    HEX_OP_SHR = 0x2b,

    HEX_OP_EQUAL = 0x2c,
    HEX_OP_NOTEQUAL = 0x2d,
    HEX_OP_GREATER = 0x2e,
    HEX_OP_LESS = 0x2f,
    HEX_OP_GREATEREQUAL = 0x30,
    HEX_OP_LESSEQUAL = 0x31,

    HEX_OP_AND = 0x32,
    HEX_OP_OR = 0x33,
    HEX_OP_NOT = 0x34,
    HEX_OP_XOR = 0x35,

    HEX_OP_INT = 0x36,
    HEX_OP_STR = 0x37,
    HEX_OP_DEC = 0x38,
    HEX_OP_HEX = 0x39,
    HEX_OP_ORD = 0x3a,
    HEX_OP_CHR = 0x3b,
    HEX_OP_TYPE = 0x3c,

    HEX_OP_CAT = 0x3d,
    HEX_OP_LEN = 0x3e,
    HEX_OP_GET = 0x3f,
    HEX_OP_INDEX = 0x40,
    HEX_OP_JOIN = 0x41,
    HEX_OP_SPLIT = 0x42,
    HEX_OP_REPLACE = 0x43,
    HEX_OP_MAP = 0x44,

    HEX_OP_PUTS = 0x45,
    HEX_OP_WARN = 0x46,
    HEX_OP_PRINT = 0x47,
    HEX_OP_GETS = 0x48,

    HEX_OP_READ = 0x49,
    HEX_OP_WRITE = 0x4A,
    HEX_OP_APPEND = 0x4B,
    HEX_OP_ARGS = 0x4C,
    HEX_OP_EXIT = 0x4D,
    HEX_OP_EXEC = 0x4E,
    HEX_OP_RUN = 0x4F,

} hex_opcode_t;

// Help System
void hex_set_doc(hex_doc_dictionary_t *docs, const char *name, const char *description, const char *input, const char *output);
int hex_get_doc(hex_doc_dictionary_t *docs, const char *key, hex_doc_entry_t *result);
void hex_create_docs(hex_doc_dictionary_t *docs);
void hex_print_help();
void hex_print_docs(hex_doc_dictionary_t *docs);

// Free data
void hex_free_item(hex_context_t *ctx, hex_item_t *item);
void hex_free_token(hex_token_t *token);
void hex_free_list(hex_context_t *ctx, hex_item_t **quotation, size_t size);

// Symbol management
int hex_valid_user_symbol(hex_context_t *ctx, const char *symbol);
int hex_valid_native_symbol(hex_context_t *ctx, const char *symbol);
int hex_set_symbol(hex_context_t *ctx, const char *key, hex_item_t *value, int native);
void hex_set_native_symbol(hex_context_t *ctx, const char *name, int (*func)());
int hex_get_symbol(hex_context_t *ctx, const char *key, hex_item_t *result);

// Errors and debugging
void hex_error(hex_context_t *ctx, const char *format, ...);
void hex_debug(hex_context_t *ctx, const char *format, ...);
void hex_debug_item(hex_context_t *ctx, const char *message, hex_item_t *item);
void hex_print_item(FILE *stream, hex_item_t *item);
void add_to_stack_trace(hex_context_t *ctx, hex_token_t *token);
void print_stack_trace(hex_context_t *ctx);

// Item constructors
hex_item_t *hex_string_item(hex_context_t *ctx, const char *value);
hex_item_t *hex_integer_item(hex_context_t *ctx, int value);
hex_item_t *hex_quotation_item(hex_context_t *ctx, hex_item_t **quotation, size_t size);

// Stack management
int hex_push(hex_context_t *ctx, hex_item_t *item);
int hex_push_integer(hex_context_t *ctx, int value);
int hex_push_string(hex_context_t *ctx, const char *value);
int hex_push_quotation(hex_context_t *ctx, hex_item_t **quotation, size_t size);
int hex_push_symbol(hex_context_t *ctx, hex_token_t *token);
hex_item_t *hex_pop(hex_context_t *ctx);
hex_item_t *hex_copy_item(hex_context_t *ctx, const hex_item_t *item);
hex_token_t *hex_copy_token(hex_context_t *ctx, const hex_token_t *token);

// Parser and interpreter
hex_token_t *hex_next_token(hex_context_t *ctx, const char **input, hex_file_position_t *position);
int32_t hex_parse_integer(const char *hex_str);
int hex_parse_quotation(hex_context_t *ctx, const char **input, hex_item_t *result, hex_file_position_t *position);
int hex_interpret(hex_context_t *ctx, const char *code, const char *filename, int line, int column);

// Utils
char *hex_itoa(int num, int base);
char *hex_itoa_dec(int num);
char *hex_itoa_hex(int num);
void hex_raw_print_item(FILE *stream, hex_item_t item);
char *hex_type(hex_item_type_t type);
void hex_rpad(const char *str, int total_length);
void hex_lpad(const char *str, int total_length);
void hex_encode_length(uint8_t **bytecode, size_t *size, size_t length);
int hex_is_binary(const uint8_t *data, size_t size);
void hex_print_string(FILE *stream, char *value);
char *hex_bytes_to_string(const uint8_t *bytes, size_t size);
char *hex_process_string(const char *value);
size_t hex_min_bytes_to_encode_integer(int32_t value);

// Native symbols
int hex_symbol_store(hex_context_t *ctx);
int hex_symbol_free(hex_context_t *ctx);
int hex_symbol_symbols(hex_context_t *ctx);
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
int hex_symbol_error(hex_context_t *ctx);
int hex_symbol_try(hex_context_t *ctx);
int hex_symbol_throw(hex_context_t *ctx);
int hex_symbol_q(hex_context_t *ctx);
int hex_symbol_map(hex_context_t *ctx);
int hex_symbol_swap(hex_context_t *ctx);
int hex_symbol_dup(hex_context_t *ctx);
int hex_symbol_stack(hex_context_t *ctx);
int hex_symbol_pop(hex_context_t *ctx);

// Opcodes
uint8_t hex_symbol_to_opcode(const char *symbol);
const char *hex_opcode_to_symbol(uint8_t opcode);

// VM
int hex_bytecode(hex_context_t *ctx, const char *input, uint8_t **output, size_t *output_size, hex_file_position_t *position);
int hex_generate_quotation_bytecode(hex_context_t *ctx, const char **input, uint8_t **output, size_t *output_size, size_t *n_items, hex_file_position_t *position);
int hex_bytecode_quotation(hex_context_t *ctx, uint8_t **bytecode, size_t *size, size_t *capacity, uint8_t **output, size_t *output_size, size_t *n_items);
int hex_bytecode_integer(hex_context_t *ctx, uint8_t **bytecode, size_t *size, size_t *capacity, int32_t value);
int hex_bytecode_string(hex_context_t *ctx, uint8_t **bytecode, size_t *size, size_t *capacity, const char *value);
int hex_bytecode_symbol(hex_context_t *ctx, uint8_t **bytecode, size_t *size, size_t *capacity, const char *value);
int hex_interpret_bytecode_integer(hex_context_t *ctx, uint8_t **bytecode, size_t *size, hex_item_t *result);
int hex_interpret_bytecode_string(hex_context_t *ctx, uint8_t **bytecode, size_t *size, hex_item_t *result);
int hex_interpret_bytecode_native_symbol(hex_context_t *ctx, uint8_t opcode, size_t position, hex_item_t *result);
int hex_interpret_bytecode_user_symbol(hex_context_t *ctx, uint8_t **bytecode, size_t *size, size_t position, hex_item_t *result);
int hex_interpret_bytecode_quotation(hex_context_t *ctx, uint8_t **bytecode, size_t *size, size_t position, hex_item_t *result);
int hex_interpret_bytecode(hex_context_t *ctx, uint8_t *bytecode, size_t size);
void hex_header(hex_context_t *ctx, uint8_t header[8]);
int hex_validate_header(uint8_t header[8]);

// Symbol table
int hex_symboltable_set(hex_context_t *ctx, const char *symbol);
int hex_symboltable_get_index(hex_context_t *ctx, const char *symbol);
char *hex_symboltable_get_value(hex_context_t *ctx, uint16_t index);
int hex_decode_bytecode_symboltable(hex_context_t *ctx, uint8_t **bytecode, size_t *size, size_t count);
uint8_t *hex_encode_bytecode_symboltable(hex_context_t *ctx, size_t *out_size);

// REPL and initialization
void hex_register_symbols(hex_context_t *ctx);
hex_context_t *hex_init();
void hex_repl(hex_context_t *ctx);
void hex_process_stdin(hex_context_t *ctx);
void hex_handle_sigint(int sig);
int hex_write_bytecode_file(hex_context_t *ctx, char *filename, uint8_t *bytecode, size_t size);
char *hex_read_file(hex_context_t *ctx, const char *filename);

// Common operations
#define HEX_POP(ctx, x) x = hex_pop(ctx)
#define HEX_FREE(ctx, x) hex_free_item(ctx, x)
#define HEX_PUSH(ctx, x) hex_push(ctx, x)
#define HEX_ALLOC(x) hex_item_t *x = (hex_item_t *)malloc(sizeof(hex_item_t));

#endif // HEX_H

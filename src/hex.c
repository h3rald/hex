/* *** hex amalgamation *** */
/* File: src/hex.h */
#line 1 "src/hex.h"

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
#define HEX_VERSION "0.6.0"
#define HEX_STDIN_BUFFER_SIZE 16384
#define HEX_INITIAL_REGISTRY_SIZE 512
#define HEX_REGISTRY_SIZE 4096
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
    int is_operator;
    hex_token_t *token;    // Token containing stack information (valid for HEX_TYPE_NATIVE_SYMBOL and HEX_TYPE_USER_SYMBOL)
    size_t quotation_size; // Size of the quotation (valid for HEX_TYPE_QUOTATION)
} hex_item_t;

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

typedef struct hex_registry_entry_t
{
    char *key;
    hex_item_t *value;
    struct hex_registry_entry_t *next; // For collision resolution (chaining)
} hex_registry_entry_t;

typedef struct hex_registry_t
{
    hex_registry_entry_t **buckets; // Array of bucket pointers
    size_t bucket_count;            // Number of buckets
    size_t size;                    // Number of stored entries
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
    HEX_OP_WHILE = 0x15,
    HEX_OP_ERROR = 0x16,
    HEX_OP_TRY = 0x17,
    HEX_OP_THROW = 0x18,

    HEX_OP_DUP = 0x19,
    HEX_OP_STACK = 0x1a,
    HEX_OP_DROP = 0x1b,
    HEX_OP_SWAP = 0x1c,

    HEX_OP_I = 0x1d,
    HEX_OP_EVAL = 0x1e,
    HEX_OP_QUOTE = 0x1f,

    HEX_OP_ADD = 0x20,
    HEX_OP_SUBTRACT = 0x21,
    HEX_OP_MULTIPLY = 0x22,
    HEX_OP_DIVIDE = 0x23,
    HEX_OP_MOD = 0x24,

    HEX_OP_BITAND = 0x25,
    HEX_OP_BITOR = 0x26,
    HEX_OP_BITXOR = 0x27,
    HEX_OP_BITNOT = 0x28,
    HEX_OP_SHL = 0x29,
    HEX_OP_SHR = 0x2a,

    HEX_OP_EQUAL = 0x2b,
    HEX_OP_NOTEQUAL = 0x2c,
    HEX_OP_GREATER = 0x2d,
    HEX_OP_LESS = 0x2e,
    HEX_OP_GREATEREQUAL = 0x2f,
    HEX_OP_LESSEQUAL = 0x30,

    HEX_OP_AND = 0x31,
    HEX_OP_OR = 0x32,
    HEX_OP_NOT = 0x33,
    HEX_OP_XOR = 0x34,

    HEX_OP_INT = 0x35,
    HEX_OP_STR = 0x36,
    HEX_OP_DEC = 0x37,
    HEX_OP_HEX = 0x38,
    HEX_OP_ORD = 0x39,
    HEX_OP_CHR = 0x3a,
    HEX_OP_TYPE = 0x3b,

    HEX_OP_CAT = 0x3c,
    HEX_OP_LEN = 0x3d,
    HEX_OP_GET = 0x3e,
    HEX_OP_INDEX = 0x3f,
    HEX_OP_JOIN = 0x40,
    HEX_OP_SPLIT = 0x41,
    HEX_OP_SUB = 0x42,
    HEX_OP_MAP = 0x43,

    HEX_OP_PUTS = 0x44,
    HEX_OP_WARN = 0x45,
    HEX_OP_PRINT = 0x46,
    HEX_OP_GETS = 0x47,

    HEX_OP_READ = 0x48,
    HEX_OP_WRITE = 0x49,
    HEX_OP_APPEND = 0x4a,
    HEX_OP_ARGS = 0x4b,
    HEX_OP_EXIT = 0x4c,
    HEX_OP_EXEC = 0x4d,
    HEX_OP_RUN = 0x4e,
    HEX_OP_TIMESTAMP = 0x4f,

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

// Symbol and registry management
hex_registry_t *hex_registry_create();
int hex_registry_resize(hex_context_t *ctx);
void hex_registry_destroy(hex_context_t *ctx);
int hex_valid_user_symbol(hex_context_t *ctx, const char *symbol);
int hex_valid_native_symbol(hex_context_t *ctx, const char *symbol);
int hex_set_symbol(hex_context_t *ctx, const char *key, hex_item_t *value, int native);
void hex_set_native_symbol(hex_context_t *ctx, const char *name, int (*func)());
int hex_get_symbol(hex_context_t *ctx, const char *key, hex_item_t *result);
int hex_delete_symbol(hex_context_t *ctx, const char *key);

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
void hex_free_item(hex_context_t *ctx, hex_item_t *item);
void hex_free_list(hex_context_t *ctx, hex_item_t **quotation, size_t size);
void hex_free_token(hex_token_t *token);
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
char *hex_unescape_string(const char *input);
void get_unix_timestamp_sec_usec(int32_t result[2]);

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
int hex_symbol_index(hex_context_t *ctx);
int hex_symbol_join(hex_context_t *ctx);
int hex_symbol_split(hex_context_t *ctx);
int hex_symbol_sub(hex_context_t *ctx);
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
int hex_symbol_drop(hex_context_t *ctx);
int hex_symbol_timestamp(hex_context_t *ctx);

// Debug / integrity helpers (always callable; no-op stubs in non-DEBUG builds)
#ifdef DEBUG
int hex_validate_quotation_integrity(hex_context_t *ctx, const hex_item_t *item);
int hex_debug_validate_stack(hex_context_t *ctx);
#else
static inline int hex_validate_quotation_integrity(hex_context_t *ctx, const hex_item_t *item)
{
    (void)ctx;
    (void)item;
    return 0;
}
static inline int hex_debug_validate_stack(hex_context_t *ctx)
{
    (void)ctx;
    return 0;
}
#endif

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
int hex_interpret_bytecode_native_symbol(hex_context_t *ctx, uint8_t opcode, size_t position, const char *filename, hex_item_t *result);
int hex_interpret_bytecode_user_symbol(hex_context_t *ctx, uint8_t **bytecode, size_t *size, size_t position, const char *filename, hex_item_t *result);
int hex_interpret_bytecode_quotation(hex_context_t *ctx, uint8_t **bytecode, size_t *size, size_t position, const char *filename, hex_item_t *result);
int hex_interpret_bytecode(hex_context_t *ctx, uint8_t *bytecode, size_t size, const char *filename);
void hex_header(hex_context_t *ctx, uint8_t header[8]);
int hex_validate_header(uint8_t header[8]);

// Symbol table
int hex_symboltable_set(hex_context_t *ctx, const char *symbol);
int hex_symboltable_get_index(hex_context_t *ctx, const char *symbol);
char *hex_symboltable_get_value(hex_context_t *ctx, uint16_t index);
int hex_decode_bytecode_symboltable(hex_context_t *ctx, uint8_t **bytecode, size_t *size, size_t count);
uint8_t *hex_encode_bytecode_symboltable(hex_context_t *ctx, size_t *out_size);
hex_symbol_table_t *hex_symboltable_copy(hex_context_t *ctx);
void hex_symboltable_destroy(hex_symbol_table_t *table);

// REPL and initialization
void hex_register_symbols(hex_context_t *ctx);
hex_context_t *hex_init();
void hex_destroy(hex_context_t *ctx);
void hex_repl(hex_context_t *ctx);
void hex_process_stdin(hex_context_t *ctx);
void hex_handle_sigint(int sig);
int hex_write_bytecode_file(hex_context_t *ctx, char *filename, uint8_t *bytecode, size_t size);
int hex_interpret_file(hex_context_t *ctx, const char *file);
char *hex_read_file(hex_context_t *ctx, const char *filename);

// Common operations
#define HEX_POP(ctx, x) hex_item_t *x = hex_pop(ctx)
#define HEX_FREE(ctx, x) hex_free_item(ctx, x)
#define HEX_PUSH(ctx, x) hex_push(ctx, x)
#define HEX_ALLOC(x) hex_item_t *x = (hex_item_t *)malloc(sizeof(hex_item_t));

#endif // HEX_H

/* File: src/stack.c */
#line 1 "src/stack.c"
#ifndef HEX_H
#include "hex.h"
#endif

////////////////////////////////////////
// Stack Implementation               //
////////////////////////////////////////

// Free a token
void hex_free_token(hex_token_t *token)
{
    if (token == NULL)
    {
        return;
    }

    if (token->value)
    {
        free(token->value);
        token->value = NULL;
    }

    if (token->position)
    {
        if (token->position->filename)
        {
            free((void *)token->position->filename);
            token->position->filename = NULL;
        }
        free(token->position);
        token->position = NULL;
    }

    free(token); // Free the token itself
}

// Push functions
int hex_push(hex_context_t *ctx, hex_item_t *item)
{
    if (ctx->stack->top >= HEX_STACK_SIZE - 1)
    {
        hex_error(ctx, "[push] Stack overflow");
        return 1;
    }
    if (ctx->settings && ctx->settings->debugging_enabled)
    {
        // Validate existing stack before modification
        hex_debug_validate_stack(ctx);
    }
    hex_debug_item(ctx, "PUSH", item);
    int result = 0;

    if (item->type == HEX_TYPE_USER_SYMBOL)
    {
        hex_item_t *value = calloc(1, sizeof(hex_item_t));
        if (value == NULL)
        {
            hex_error(ctx, "[push] Failed to allocate memory for value");
            return 1;
        }
        if (hex_get_symbol(ctx, item->token->value, value))
        {
            if (value->type == HEX_TYPE_QUOTATION && value->is_operator)
            {
                add_to_stack_trace(ctx, item->token);
                for (size_t i = 0; i < value->quotation_size; i++)
                {
                    // Create copies of the items to avoid ownership issues
                    hex_item_t *copy = hex_copy_item(ctx, value->data.quotation_value[i]);
                    if (!copy || hex_push(ctx, copy) != 0)
                    {
                        if (copy)
                            hex_free_item(ctx, copy);
                        hex_free_item(ctx, value);
                        hex_debug_item(ctx, "FAIL", item);
                        return 1;
                    }
                }
                hex_free_item(ctx, value); // Free the temporary value
            }
            else
            {
                result = hex_push(ctx, value);
            }
        }
        else
        {
            hex_error(ctx, "[push] Undefined user symbol: %s", item->token->value);
            hex_free_item(ctx, value);
            result = 1;
        }
    }
    else if (item->type == HEX_TYPE_NATIVE_SYMBOL)
    {
        hex_item_t *value = calloc(1, sizeof(hex_item_t));
        if (value == NULL)
        {
            hex_error(ctx, "[push] Failed to allocate memory for value");
            return 1;
        }
        if (hex_get_symbol(ctx, item->token->value, value))
        {
            add_to_stack_trace(ctx, item->token);
            hex_debug_item(ctx, "CALL", item);
            result = value->data.fn_value(ctx);
        }
        else
        {
            hex_error(ctx, "[push] Undefined native symbol: %s", item->token->value);
            result = 1;
        }
        hex_free_item(ctx, value); // Free the temporary value
    }
    else
    {
        ctx->stack->entries[++ctx->stack->top] = item;
    }

    if (result == 0)
    {
        hex_debug_item(ctx, "DONE", item);
    }
    else
    {
        hex_debug_item(ctx, "FAIL", item);
    }
    if (result == 0 && ctx->settings && ctx->settings->debugging_enabled)
    {
        // Validate stack after successful push
        hex_debug_validate_stack(ctx);
    }
    return result;
}

hex_item_t *hex_string_item(hex_context_t *ctx, const char *value)
{
    char *str = hex_process_string(value);
    if (str == NULL)
    {
        hex_error(ctx, "[create string] Failed to allocate memory for string");
        return NULL;
    }
    hex_item_t *item = calloc(1, sizeof(hex_item_t));
    if (item == NULL)
    {
        hex_error(ctx, "[create string] Failed to allocate memory for item");
        free(str);
        return NULL;
    }
    item->type = HEX_TYPE_STRING;
    item->data.str_value = str;
    item->is_operator = 0;
    item->token = NULL;
    item->quotation_size = 0;
    return item;
}

hex_item_t *hex_integer_item(hex_context_t *ctx, int value)
{
    (void)(ctx);
    hex_item_t *item = calloc(1, sizeof(hex_item_t));
    if (item == NULL)
    {
        hex_error(ctx, "[create integer] Failed to allocate memory for item");
        return NULL;
    }
    item->type = HEX_TYPE_INTEGER;
    item->data.int_value = value;
    item->is_operator = 0;
    item->token = NULL;
    item->quotation_size = 0;
    return item;
}

hex_item_t *hex_quotation_item(hex_context_t *ctx, hex_item_t **quotation, size_t size)
{
    hex_item_t *item = calloc(1, sizeof(hex_item_t));
    if (item == NULL)
    {
        hex_error(ctx, "[create quotation] Failed to allocate memory for item");
        return NULL;
    }
    item->is_operator = 0;
    item->type = HEX_TYPE_QUOTATION;
    item->data.quotation_value = quotation;
    item->quotation_size = size;
    item->token = NULL;
    return item;
}

hex_item_t *hex_symbol_item(hex_context_t *ctx, hex_token_t *token)
{
    hex_item_t *item = calloc(1, sizeof(hex_item_t));
    if (item == NULL)
    {
        hex_error(ctx, "[create symbol] Failed to allocate memory for item");
        return NULL;
    }
    item->type = hex_valid_native_symbol(ctx, token->value) ? HEX_TYPE_NATIVE_SYMBOL : HEX_TYPE_USER_SYMBOL;
    item->token = token;
    item->is_operator = 0;
    item->quotation_size = 0;
    return item;
}

int hex_push_string(hex_context_t *ctx, const char *value)
{
    hex_item_t *item = hex_string_item(ctx, value);
    if (item == NULL)
    {
        return 1;
    }
    int result = HEX_PUSH(ctx, item);
    return result;
}

int hex_push_integer(hex_context_t *ctx, int value)
{
    hex_item_t *item = hex_integer_item(ctx, value);
    if (item == NULL)
    {
        return 1;
    }
    int result = HEX_PUSH(ctx, item);
    return result;
}

int hex_push_quotation(hex_context_t *ctx, hex_item_t **quotation, size_t size)
{
    hex_item_t *item = hex_quotation_item(ctx, quotation, size);
    if (item == NULL)
    {
        return 1;
    }
    int result = HEX_PUSH(ctx, item);
    if (result == 0 && ctx->settings && ctx->settings->debugging_enabled)
    {
        // Validate the just-pushed quotation structure specifically
        hex_validate_quotation_integrity(ctx, item);
    }
    return result;
}

int hex_push_symbol(hex_context_t *ctx, hex_token_t *token)
{
    hex_item_t *item = hex_symbol_item(ctx, token);
    if (item == NULL)
    {
        return 1;
    }
    int result = HEX_PUSH(ctx, item);
    return result;
}

// Pop function
hex_item_t *hex_pop(hex_context_t *ctx)
{
    if (ctx->stack->top < 0)
    {
        hex_error(ctx, "[pop] Insufficient items on the stack");
        hex_item_t *item = calloc(1, sizeof(hex_item_t));
        if (item)
        {
            item->type = HEX_TYPE_INVALID;
            item->token = NULL;
            item->data.int_value = 0;
        }
        return item;
    }

    hex_item_t *item = ctx->stack->entries[ctx->stack->top];
    ctx->stack->entries[ctx->stack->top] = NULL; // Clear the stack reference
    ctx->stack->top--;

    hex_debug_item(ctx, " POP", item);
    if (ctx->settings && ctx->settings->debugging_enabled)
    {
        // Validate remaining stack after pop (helps catch corruption on removal)
        hex_debug_validate_stack(ctx);
    }
    return item;
}

void hex_free_list(hex_context_t *ctx, hex_item_t **quotation, size_t size)
{
    if (!quotation)
    {
        return;
    }

    for (size_t i = 0; i < size; i++)
    {
        if (quotation[i])
        {
            hex_debug(ctx, "FREE: item #%zu", i);
            hex_free_item(ctx, quotation[i]); // Free each item
            quotation[i] = NULL;              // Prevent double free
        }
    }
    free(quotation); // Free the quotation array itself
    hex_debug(ctx, "FREE: quotation freed (%zu items)", size);
}

void hex_free_item(hex_context_t *ctx, hex_item_t *item)
{
    if (item == NULL)
    {
        return;
    }

    switch (item->type)
    {
    case HEX_TYPE_STRING:
        if (item->data.str_value)
        {
            hex_debug_item(ctx, "FREE", item);
            free(item->data.str_value);
            item->data.str_value = NULL; // Set to NULL to avoid double free
        }
        break;

    case HEX_TYPE_QUOTATION:
        if (item->data.quotation_value)
        {
            hex_debug(ctx, "FREE: freeing quotation (%zu items)", item->quotation_size);
            hex_free_list(ctx, item->data.quotation_value, item->quotation_size);
            item->data.quotation_value = NULL; // Set to NULL to avoid double free
        }
        break;

    case HEX_TYPE_NATIVE_SYMBOL:
    case HEX_TYPE_USER_SYMBOL:
        if (item->token)
        {
            hex_free_token(item->token);
            item->token = NULL;
        }
        break;

    case HEX_TYPE_INTEGER:
    case HEX_TYPE_INVALID:
        // No dynamic memory to free for these types
        break;

    default:
        hex_debug(ctx, "FREE: unknown item type: %d", item->type);
        break;
    }

    // Always free the item itself at the end
    free(item);
}

hex_token_t *hex_copy_token(hex_context_t *ctx, const hex_token_t *token)
{
    if (!token)
    {
        hex_error(ctx, "[copy token] Token is NULL");
        return NULL;
    }

    // Allocate memory for the new token
    hex_token_t *copy = (hex_token_t *)malloc(sizeof(hex_token_t));
    if (!copy)
    {
        hex_error(ctx, "[copy token] Failed to allocate memory for token copy");
        return NULL;
    }

    // Copy basic fields
    copy->type = token->type;
    if (token->type == HEX_TOKEN_QUOTATION_START || token->type == HEX_TOKEN_QUOTATION_END)
    {
        copy->quotation_size = token->quotation_size;
    }
    else
    {
        copy->quotation_size = 0;
    }

    // Copy the token's value
    if (token->value)
    {
        copy->value = strdup(token->value);
        if (!copy->value)
        {
            hex_error(ctx, "[copy token] Failed to copy token value");
            free(copy);
            return NULL;
        }
    }
    else
    {
        copy->value = NULL;
    }

    // Copy the file position if it exists
    if (token->position)
    {
        copy->position = (hex_file_position_t *)malloc(sizeof(hex_file_position_t));
        if (!copy->position)
        {
            free(copy->value);
            free(copy);
            hex_error(ctx, "[copy token] Failed to allocate memory for position");
            return NULL;
        }

        // Copy filename string
        if (token->position->filename)
        {
            copy->position->filename = strdup(token->position->filename);
            if (!copy->position->filename)
            {
                free(copy->position);
                free(copy->value);
                free(copy);
                hex_error(ctx, "[copy token] Failed to copy filename");
                return NULL;
            }
        }
        else
        {
            copy->position->filename = NULL;
        }

        if (token->position->line)
        {
            copy->position->line = token->position->line;
        }
        else
        {
            copy->position->line = 0;
        }
        if (token->position->column)
        {
            copy->position->column = token->position->column;
        }
        else
        {
            copy->position->column = 0;
        }
    }
    else
    {
        copy->position = NULL;
    }

    return copy;
}

hex_item_t *hex_copy_item(hex_context_t *ctx, const hex_item_t *item)
{
    if (!item)
    {
        hex_error(ctx, "[copy item] Item is NULL");
        return NULL;
    }

    // Allocate memory for the new hex_item_t structure
    hex_item_t *copy = (hex_item_t *)calloc(1, sizeof(hex_item_t));
    if (!copy)
    {
        hex_error(ctx, "[copy item] Failed to allocate memory for item copy");
        return NULL;
    }

    // Copy basic fields
    copy->type = item->type;

    // Copy the union field based on the type
    switch (item->type)
    {
    case HEX_TYPE_INTEGER:
        copy->data.int_value = item->data.int_value;
        break;

    case HEX_TYPE_STRING:
        if (item->data.str_value)
        {
            copy->data.str_value = strdup(item->data.str_value); // Deep copy the string
            if (!copy->data.str_value)
            {
                hex_free_item(ctx, copy);
                hex_error(ctx, "[copy item] Failed to copy string value");
                return NULL;
            }
        }
        else
        {
            copy->data.str_value = NULL;
        }
        break;

    case HEX_TYPE_QUOTATION:
        copy->quotation_size = item->quotation_size;
        copy->is_operator = item->is_operator;
        if (item->data.quotation_value)
        {
            copy->data.quotation_value = (hex_item_t **)malloc(item->quotation_size * sizeof(hex_item_t *));
            if (!copy->data.quotation_value)
            {
                hex_error(ctx, "[copy item] Failed to allocate memory for quotation array");
                hex_free_item(ctx, copy);
                return NULL;
            }

            for (size_t i = 0; i < item->quotation_size; ++i)
            {
                copy->data.quotation_value[i] = hex_copy_item(ctx, item->data.quotation_value[i]); // Recursively copy each item
                if (!copy->data.quotation_value[i])
                {
                    // Cleanup on failure
                    hex_error(ctx, "[copy item] Failed to copy quotation item");
                    // Free already copied items
                    for (size_t j = 0; j < i; j++)
                    {
                        hex_free_item(ctx, copy->data.quotation_value[j]);
                    }
                    free(copy->data.quotation_value);
                    free(copy);
                    return NULL;
                }
            }
        }
        else
        {
            copy->data.quotation_value = NULL;
        }
        break;

    case HEX_TYPE_NATIVE_SYMBOL:
        copy->data.fn_value = item->data.fn_value; // Copy function pointer for native symbols
        break;

    case HEX_TYPE_USER_SYMBOL:
        break;

    default:
        // Unsupported type
        hex_error(ctx, "[copy item] Unsupported item type: %s", hex_type(item->type));
        hex_free_item(ctx, copy);
        return NULL;
    }

    // Copy the token field for native and user symbols
    if (item->type == HEX_TYPE_NATIVE_SYMBOL || item->type == HEX_TYPE_USER_SYMBOL)
    {
        copy->token = hex_copy_token(ctx, item->token);
        if (!copy->token)
        {
            hex_error(ctx, "[copy item] Failed to copy token");
            hex_free_item(ctx, copy);
            return NULL;
        }
    }
    else
    {
        copy->token = NULL;
    }

    return copy;
}

/* File: src/registry.c */
#line 1 "src/registry.c"
#ifndef HEX_H
#include "hex.h"
#endif

////////////////////////////////////////
// Registry Implementation            //
////////////////////////////////////////

// Helper: deep-copy an item into an existing struct (wrapper move)
static int hex_copy_item_into(hex_context_t *ctx, const hex_item_t *src, hex_item_t *dst)
{
    hex_item_t *tmp = hex_copy_item(ctx, src);
    if (!tmp)
    {
        return 0;
    }
    *dst = *tmp; // move struct fields
    free(tmp);   // free only the wrapper
    return 1;
}

static size_t hash_function(const char *key, size_t bucket_count)
{
    size_t hash = 5381;
    while (*key)
    {
        hash = ((hash << 5) + hash) + (unsigned char)(*key); // hash * 33 + key[i]
        key++;
    }
    return hash % bucket_count;
}

hex_registry_t *hex_registry_create()
{
    hex_registry_t *registry = calloc(1, sizeof(hex_registry_t));
    if (registry == NULL)
    {
        return NULL;
    }

    registry->bucket_count = HEX_INITIAL_REGISTRY_SIZE;
    registry->size = 0;
    registry->buckets = calloc(HEX_INITIAL_REGISTRY_SIZE, sizeof(hex_registry_entry_t *));
    if (registry->buckets == NULL)
    {
        free(registry);
        return NULL;
    }

    return registry;
}

int hex_registry_resize(hex_context_t *ctx)
{
    hex_registry_t *registry = ctx->registry;

    int new_bucket_count = registry->bucket_count * 2;
    hex_registry_entry_t **new_buckets = calloc(new_bucket_count, sizeof(hex_registry_entry_t *));
    if (new_buckets == NULL)
    {
        return 1; // Memory allocation failed
    }

    // Rehash all existing entries into the new buckets
    for (size_t i = 0; i < registry->bucket_count; i++)
    {
        hex_registry_entry_t *entry = registry->buckets[i];
        while (entry)
        {
            hex_registry_entry_t *next = entry->next;

            // Recompute hash index
            size_t new_bucket_index = hash_function(entry->key, new_bucket_count);
            entry->next = new_buckets[new_bucket_index];
            new_buckets[new_bucket_index] = entry;

            entry = next;
        }
    }

    // Replace old buckets with new ones
    free(registry->buckets);
    registry->buckets = new_buckets;
    registry->bucket_count = new_bucket_count;

    return 0;
}

void hex_registry_destroy(hex_context_t *ctx)
{
    hex_registry_t *registry = ctx->registry;

    if (!registry)
    {
        return;
    }

    for (size_t i = 0; i < registry->bucket_count; i++)
    {
        hex_registry_entry_t *entry = registry->buckets[i];
        while (entry)
        {
            hex_registry_entry_t *next = entry->next;
            if (entry->key)
            {
                free(entry->key);
                entry->key = NULL;
            }
            if (entry->value)
            {
                hex_free_item(ctx, entry->value);
                entry->value = NULL;
            }
            free(entry);
            entry = next;
        }
    }

    if (registry->buckets)
    {
        free(registry->buckets);
        registry->buckets = NULL;
    }
    free(registry);
}

int hex_valid_user_symbol(hex_context_t *ctx, const char *symbol)
{
    // Check that key starts with a letter, or underscore
    // and subsequent characters (if any) are letters, numbers, or underscores
    if (strlen(symbol) == 0)
    {
        hex_error(ctx, "Symbol name cannot be an empty string");
        return 0;
    }
    if (!isalpha(symbol[0]) && symbol[0] != '_')
    {
        hex_error(ctx, "Invalid symbol: %s", symbol);
        return 0;
    }
    if (strlen(symbol) > HEX_MAX_SYMBOL_LENGTH)
    {
        hex_error(ctx, "[check valid symbol] Symbol name too long: %s", symbol);
        return 0;
    }
    for (size_t j = 1; j < strlen(symbol); j++)
    {
        if (!isalnum(symbol[j]) && symbol[j] != '_' && symbol[j] != '-')
        {
            hex_error(ctx, "Invalid symbol: %s", symbol);
            return 0;
        }
    }
    return 1;
}

/*
 * hex_set_symbol
 * Ownership contract:
 *  - Caller passes a heap-allocated hex_item_t* (value).
 *  - This function ALWAYS consumes (frees) the passed pointer, whether it succeeds or fails.
 *  - The registry stores its own deep copy (value_copy) and becomes sole owner.
 *  - Callers MUST NOT use or free the original pointer after calling.
 *  - Rationale: prevents aliasing between caller's pointer and registry entry, eliminating
 *    double free / accidental mutation risks.
 */
int hex_set_symbol(hex_context_t *ctx, const char *key, hex_item_t *value, int native)
{
    hex_registry_t *registry = ctx->registry;
    if (!native && hex_valid_user_symbol(ctx, key) == 0)
    {
        hex_error(ctx, "[set symbol] Invalid user symbol %s", key);
        return 1;
    }

    if (!native && hex_valid_native_symbol(ctx, key))
    {
        hex_error(ctx, "[set symbol] Cannot overwrite native symbol '%s'", key);
        return 1;
    }

    if ((float)registry->size / registry->bucket_count > 0.75)
    {
        hex_registry_resize(ctx);
    }

    size_t bucket_index = hash_function(key, registry->bucket_count);
    hex_registry_entry_t *entry = registry->buckets[bucket_index];

    // Search for an existing key in the bucket
    while (entry)
    {
        if (strcmp(entry->key, key) == 0)
        {
            // Key already exists, update its value
            hex_free_item(ctx, entry->value); // Free old value
            entry->value = value;
            return 0;
        }
        entry = entry->next;
    }

    // Add a new entry to the bucket
    hex_registry_entry_t *new_entry = calloc(1, sizeof(hex_registry_entry_t));
    if (new_entry == NULL)
    {
        return 1; // Memory allocation failed
    }

    new_entry->key = strdup(key);
    new_entry->value = value;
    new_entry->next = registry->buckets[bucket_index];
    registry->buckets[bucket_index] = new_entry;

    registry->size++;

    return 0;
}

void hex_set_native_symbol(hex_context_t *ctx, const char *name, int (*func)())
{
    hex_item_t *func_item = calloc(1, sizeof(hex_item_t));
    if (func_item == NULL)
    {
        hex_error(ctx, "[set native symbol] Memory allocation failed for native symbol '%s'", name);
        return;
    }
    func_item->type = HEX_TYPE_NATIVE_SYMBOL;
    func_item->data.fn_value = func;
    // Need to create a fake token for native symbols as well.
    func_item->token = calloc(1, sizeof(hex_token_t));
    func_item->token->type = HEX_TOKEN_SYMBOL;
    func_item->token->value = strdup(name);
    func_item->token->position = NULL;
    // hex_set_symbol consumes func_item (success or failure); never free it here afterwards.
    if (hex_set_symbol(ctx, name, func_item, 1) != 0)
    {
        hex_error(ctx, "Error: Failed to register native symbol '%s'", name);
        return; // func_item already freed inside hex_set_symbol
    }
}

int hex_get_symbol(hex_context_t *ctx, const char *key, hex_item_t *result)
{
    hex_registry_t *registry = ctx->registry;
    hex_debug(ctx, "LOOK: %s", key);
    size_t bucket_index = hash_function(key, registry->bucket_count);

    hex_registry_entry_t *entry = registry->buckets[bucket_index];
    while (entry != NULL)
    {
        if (strcmp(entry->key, key) == 0)
        {
            if (!hex_copy_item_into(ctx, entry->value, result))
            {
                hex_error(ctx, "[get symbol] Failed to copy item for key: %s", key);
                return 0;
            }
            hex_debug_item(ctx, "DONE", result);
            return 1;
        }
        entry = entry->next; // Move to the next entry in the bucket
    }

    hex_debug_item(ctx, "FAIL", result);
    // Key not found
    return 0;
}

int hex_delete_symbol(hex_context_t *ctx, const char *key)
{
    hex_registry_t *registry = ctx->registry;

    size_t bucket_index = hash_function(key, registry->bucket_count);
    hex_registry_entry_t *entry = registry->buckets[bucket_index];
    hex_registry_entry_t *prev = NULL;

    while (entry)
    {
        if (strcmp(entry->key, key) == 0)
        {
            // Key found, remove it
            if (prev)
            {
                prev->next = entry->next;
            }
            else
            {
                registry->buckets[bucket_index] = entry->next;
            }

            free(entry->key);
            hex_free_item(ctx, entry->value);
            free(entry);
            registry->size--;

            return 0;
        }

        prev = entry;
        entry = entry->next;
    }

    return 1; // Key not found
}

/* File: src/error.c */
#line 1 "src/error.c"
#ifndef HEX_H
#include "hex.h"
#endif

////////////////////////////////////////
// Error & Debugging                  //
////////////////////////////////////////

void hex_error(hex_context_t *ctx, const char *format, ...)
{
    va_list args;
    va_start(args, format);
    vsnprintf(ctx->error, sizeof(ctx->error), format, args);
    if (ctx->settings->errors_enabled) /// FC
    {
        fprintf(stderr, "ERROR: ");
        fprintf(stderr, "%s\n", ctx->error);
    }
    va_end(args);
}

void hex_debug(hex_context_t *ctx, const char *format, ...)
{
    if (ctx->settings->debugging_enabled)
    {
        va_list args;
        va_start(args, format);
        fprintf(stdout, "*** ");
        vfprintf(stdout, format, args);
        fprintf(stdout, "\n");
        va_end(args);
    }
}

char *hex_type(hex_item_type_t type)
{
    switch (type)
    {
    case HEX_TYPE_INTEGER:
        return "integer";
    case HEX_TYPE_STRING:
        return "string";
    case HEX_TYPE_QUOTATION:
        return "quotation";
    case HEX_TYPE_NATIVE_SYMBOL:
        return "native-symbol";
    case HEX_TYPE_USER_SYMBOL:
        return "user-symbol";
    case HEX_TYPE_INVALID:
        return "invalid";
    default:
        return "unknown";
    }
}

void hex_debug_item(hex_context_t *ctx, const char *message, hex_item_t *item)
{
    if (ctx->settings->debugging_enabled)
    {
        fprintf(stdout, "*** %s: ", message);
        hex_print_item(stdout, item);
        fprintf(stdout, "\n");
    }
}

////////////////////////////////////////
// Stack trace implementation         //
////////////////////////////////////////

// Add an entry to the circular stack trace
void add_to_stack_trace(hex_context_t *ctx, hex_token_t *token)
{
    if (!token)
        return;

    int index = (ctx->stack_trace->start + ctx->stack_trace->size) % HEX_STACK_TRACE_SIZE;

    if (ctx->stack_trace->size < HEX_STACK_TRACE_SIZE)
    {
        // Buffer is not full; add item
        ctx->stack_trace->entries[index] = hex_copy_token(ctx, token);
        ctx->stack_trace->size++;
    }
    else
    {
        // Buffer is full; overwrite the oldest item
        if (ctx->stack_trace->entries[index])
        {
            hex_free_token(ctx->stack_trace->entries[index]);
        }
        ctx->stack_trace->entries[index] = hex_copy_token(ctx, token);
        ctx->stack_trace->start = (ctx->stack_trace->start + 1) % HEX_STACK_TRACE_SIZE;
    }
}

// Print the stack trace
void print_stack_trace(hex_context_t *ctx)
{
    if (!ctx->settings->stack_trace_enabled || !ctx->settings->errors_enabled || ctx->stack_trace->size <= 0)
    {
        return;
    }
    fprintf(stderr, "[stack trace] (most recent operator symbol first):\n");

    for (size_t i = 0; i < ctx->stack_trace->size; i++)
    {
        int index = (ctx->stack_trace->start + ctx->stack_trace->size - 1 - i) % HEX_STACK_TRACE_SIZE;
        hex_token_t *token = ctx->stack_trace->entries[index];
        if (token && token->value && token->position && token->position->filename)
        {
            fprintf(stderr, "  %s (%s:%d:%d)\n", token->value, token->position->filename, token->position->line, token->position->column);
        }
    }
}

/* File: src/doc.c */
#line 1 "src/doc.c"
#ifndef HEX_H
#include "hex.h"
#endif

////////////////////////////////////////
// Help System                        //
////////////////////////////////////////

void hex_set_doc(hex_doc_dictionary_t *dict, const char *name, const char *input, const char *output, const char *description)
{
    hex_doc_entry_t *doc = malloc(sizeof(hex_doc_entry_t));
    if (doc == NULL)
    {
        return;
    }
    doc->name = name;
    doc->description = description;
    doc->input = input;
    doc->output = output;
    dict->entries[dict->size] = doc;
    dict->size++;
}

int hex_get_doc(hex_doc_dictionary_t *docs, const char *key, hex_doc_entry_t *result)
{
    for (size_t i = 0; i < docs->size; i++)
    {
        if (strcmp(docs->entries[i]->name, key) == 0)
        {
            *result = *(docs->entries[i]);
            return 1;
        }
    }
    return 0;
}

void hex_create_docs(hex_doc_dictionary_t *docs)
{
    // Memory
    hex_set_doc(docs, ":", "a s", "", "Stores 'a' as the literal symbol 's'.");
    hex_set_doc(docs, "::", "a s", "", "Defines 'a' as the operator symbol 's'.");
    hex_set_doc(docs, "#", "s", "", "Deletes user symbol 's'.");
    hex_set_doc(docs, "symbols", "", "q", "Pushes a quotation containing all registered symbols on the stack.");

    // Control flow
    hex_set_doc(docs, "if", "q q q", "*", "If 'q1' is not 0x0, executes 'q2', else 'q3'.");
    hex_set_doc(docs, "while", "q1 q2", "*", "While 'q1' is not 0x0, executes 'q2'.");
    hex_set_doc(docs, "error", "", "s", "Returns the last error message.");
    hex_set_doc(docs, "try", "q1 q2", "*", "If 'q1' fails, executes 'q2'.");
    hex_set_doc(docs, "throw", "s", "", "Throws error 's'.");

    // Stack
    hex_set_doc(docs, "dup", "a", "a a", "Duplicates 'a'.");
    hex_set_doc(docs, "drop", "a", "", "Removes the top item from the stack.");
    hex_set_doc(docs, "swap", "a1 a2", "a2 a1", "Swaps 'a2' with 'a1'.");
    hex_set_doc(docs, "stack", "", "q", "Returns the contents of the stack.");

    // Evaluation
    hex_set_doc(docs, ".", "q", "*", "Pushes each item of 'q' on the stack.");
    hex_set_doc(docs, "!", "(s1|q) s2", "*", "Evaluates 's1' as a hex program or 'q' as hex bytecode (using s2 as file name).");
    hex_set_doc(docs, "'", "a", "q", "Wraps 'a' in a quotation.");
    hex_set_doc(docs, "debug", "q", "*", "Enables debug mode and pushes each item of 'q' on the stack.");

    // Arithmetic
    hex_set_doc(docs, "+", "i1 12", "i", "Adds two integers.");
    hex_set_doc(docs, "-", "i1 12", "i", "Subtracts 'i2' from 'i1'.");
    hex_set_doc(docs, "*", "i1 12", "i", "Multiplies two integers.");
    hex_set_doc(docs, "/", "i1 12", "i", "Divides 'i1' by 'i2'.");
    hex_set_doc(docs, "%", "i1 12", "i", "Calculates the modulo of 'i1' divided by 'i2'.");

    // Bitwise
    hex_set_doc(docs, "&", "i1 12", "i", "Calculates the bitwise AND of two integers.");
    hex_set_doc(docs, "|", "i1 12", "i", "Calculates the bitwise OR of two integers.");
    hex_set_doc(docs, "^", "i1 12", "i", "Calculates the bitwise XOR of two integers.");
    hex_set_doc(docs, "~", "i", "i", "Calculates the bitwise NOT of an integer.");
    hex_set_doc(docs, "<<", "i1 12", "i", "Shifts 'i1' by 'i2' bytes to the left.");
    hex_set_doc(docs, ">>", "i1 12", "i", "Shifts 'i1' by 'i2' bytes to the right.");

    // Comparison
    hex_set_doc(docs, "==", "a1 a2", "i", "Returns 0x1 if 'a1' == 'a2', 0x0 otherwise.");
    hex_set_doc(docs, "!=", "a1 a2", "i", "Returns 0x1 if 'a1' != 'a2', 0x0 otherwise.");
    hex_set_doc(docs, ">", "a1 a2", "i", "Returns 0x1 if 'a1' > 'a2', 0x0 otherwise.");
    hex_set_doc(docs, "<", "a1 a2", "i", "Returns 0x1 if 'a1' < 'a2', 0x0 otherwise.");
    hex_set_doc(docs, ">=", "a1 a2", "i", "Returns 0x1 if 'a1' >= 'a2', 0x0 otherwise.");
    hex_set_doc(docs, "<=", "a1 a2", "i", "Returns 0x1 if 'a1' <= 'a2', 0x0 otherwise.");

    // Logical
    hex_set_doc(docs, "and", "i1 i2", "i", "Returns 0x1 if both 'i1' and 'i2' are not 0x0.");
    hex_set_doc(docs, "or", "i1 i2", "i", "Returns 0x1 if either 'i1' or 'i2' are not 0x0.");
    hex_set_doc(docs, "not", "i", "i", "Returns 0x1 if 'i' is 0x0, 0x0 otherwise.");
    hex_set_doc(docs, "xor", "i1 i2", "i", "Returns 0x1 if only one item is not 0x0.");

    // Type
    hex_set_doc(docs, "int", "s", "i", "Converts a string to a hex integer.");
    hex_set_doc(docs, "str", "i", "s", "Converts a hex integer to a string.");
    hex_set_doc(docs, "dec", "i", "s", "Converts a hex integer to a decimal string.");
    hex_set_doc(docs, "hex", "s", "i", "Converter a decimal string to a hex integer.");
    hex_set_doc(docs, "chr", "i", "s", "Converts an integer to a single-character.");
    hex_set_doc(docs, "ord", "s", "i", "Converts a single-character to an integer.");
    hex_set_doc(docs, "type", "a", "s", "Pushes the data type of 'a' on the stack.");

    // List
    hex_set_doc(docs, "cat", "(s1 s2|q1 q2) ", "(s3|q3)", "Concatenates two quotations or two strings.");
    hex_set_doc(docs, "len", "(s|q)", "i ", "Returns the length of 's' or 'q'.");
    hex_set_doc(docs, "get", "(s|q)", "a", "Gets the item at position 'i' in 's' or 'q'.");
    hex_set_doc(docs, "index", "(s a|q a)", "i", "Returns the index of 'a' within 's' or 'q'.");
    hex_set_doc(docs, "join", "q s", "s", "Joins the strings in 'q' using separator 's'.");
    hex_set_doc(docs, "split", "s1 s2", "q", "Splits 's1' using separator 's2'.");
    hex_set_doc(docs, "sub", "s1 s2 s3", "s", "Replaces 's2' with 's3' within 's1'.");
    hex_set_doc(docs, "map", "q1 q2", "q3", "Applies 'q2' to 'q1' items and returns results.");

    // I/O
    hex_set_doc(docs, "puts", "a", "", "Prints 'a' and a new line to standard output.");
    hex_set_doc(docs, "warn", "a", "", "Prints 'a' and a new line to standard error.");
    hex_set_doc(docs, "print", "a", "", "Prints 'a' to standard output.");
    hex_set_doc(docs, "gets", "", "s", "Gets a string from standard input.");

    // File
    hex_set_doc(docs, "read", "s1", "(s2|q)", "Returns the contents of the specified file.");
    hex_set_doc(docs, "write", "(s1|q) s2", "", "Writes 's1' or 'q' to the file 's2'.");
    hex_set_doc(docs, "append", "(s1|q) s2", "", "Appends 's1' or 'q' to the file 's2'.");

    // Shell
    hex_set_doc(docs, "args", "", "q", "Returns the program arguments.");
    hex_set_doc(docs, "exit", "i", "", "Exits the program with exit code 'i'.");
    hex_set_doc(docs, "exec", "s", "i", "Executes the command 's'.");
    hex_set_doc(docs, "run", "s", "q", "Executes 's' and returns code, stdout, stderr.");
}

/* File: src/parser.c */
#line 1 "src/parser.c"
#ifndef HEX_H
#include "hex.h"
#endif

/////////////////////////////////////////
// Tokenizer and Parser Implementation //
/////////////////////////////////////////

// Process a token from the input
hex_token_t *hex_next_token(hex_context_t *ctx, const char **input, hex_file_position_t *position)
{
    const char *ptr = *input;

    // Skip whitespace
    while (isspace(*ptr))
    {
        if (*ptr == '\n')
        {
            position->line++;
            position->column = 1;
        }
        else
        {
            position->column++;
        }
        ptr++;
    }

    if (*ptr == '\0')
    {
        return NULL; // End of input
    }

    hex_token_t *token = (hex_token_t *)calloc(1, sizeof(hex_token_t));
    if (!token)
    {
        return NULL;
    }
    token->type = HEX_TOKEN_INVALID; // explicit for clarity
    token->position = (hex_file_position_t *)calloc(1, sizeof(hex_file_position_t));
    if (!token->position)
    {
        free(token);
        return NULL;
    }
    token->position->line = position->line;
    token->position->column = position->column;
    token->position->filename = position->filename ? strdup(position->filename) : NULL;

    if (*ptr == ';')
    {
        // Comment token
        const char *start = ptr;
        while (*ptr != '\0' && *ptr != '\n')
        {
            ptr++;
            position->column++;
        }
        int len = ptr - start;
        token->value = (char *)malloc(len + 1);
        strncpy(token->value, start, len);
        token->value[len] = '\0';
        token->type = HEX_TOKEN_COMMENT;
    }
    else if (strncmp(ptr, "#|", 2) == 0)
    {
        // Block comment token
        const char *start = ptr;
        ptr += 2; // Skip the "#|" prefix
        position->column += 2;
        while (*ptr != '\0' && strncmp(ptr, "|#", 2) != 0)
        {
            if (*ptr == '\n')
            {
                position->line++;
                position->column = 1;
            }
            else
            {
                position->column++;
            }
            ptr++;
        }
        if (*ptr == '\0')
        {
            token->type = HEX_TOKEN_INVALID;
            token->position->line = position->line;
            token->position->column = position->column;
            hex_error(ctx, "(%d,%d) Unterminated block comment", position->line, position->column);
            return token;
        }
        ptr += 2; // Skip the "|#" suffix
        position->column += 2;
        int len = ptr - start;
        token->value = (char *)malloc(len + 1);
        strncpy(token->value, start, len);
        token->value[len] = '\0';
        token->type = HEX_TOKEN_COMMENT;
    }
    else if (*ptr == '"')
    {
        // String token
        ptr++;
        const char *start = ptr;
        int len = 0;

        while (*ptr != '\0')
        {
            if (*ptr == '\\' && *(ptr + 1) == '"')
            {
                ptr += 2;
                len++;
                position->column += 2;
            }
            if (*ptr == '\\' && *(ptr + 1) == '\\')
            {
                ptr += 2;
                len++;
                position->column += 2;
            }
            else if (*ptr == '"')
            {
                break;
            }
            else if (*ptr == '\n')
            {
                token->type = HEX_TOKEN_INVALID;
                token->position->line = position->line;
                token->position->column = position->column;
                hex_error(ctx, "(%d,%d) Unescaped new line in string", position->line, position->column);
                return token;
            }
            else
            {
                ptr++;
                len++;
                position->column++;
            }
        }

        if (*ptr != '"')
        {
            token->type = HEX_TOKEN_INVALID;
            token->position->line = position->line;
            token->position->column = position->column;
            hex_error(ctx, "(%d,%d) Unterminated string", position->line, position->column);
            return token;
        }

        token->value = (char *)malloc(len + 1);
        char *dst = token->value;

        ptr = start;
        while (*ptr != '\0' && *ptr != '"')
        {
            if (*ptr == '\\' && *(ptr + 1) == '\\')
            {
                *dst++ = '\\';
                *dst++ = '\\';
                ptr += 2;
            }
            else if (*ptr == '\\' && *(ptr + 1) == '"')
            {
                *dst++ = '"';
                ptr += 2;
            }
            else
            {
                *dst++ = *ptr++;
            }
        }
        *dst = '\0';
        ptr++;
        position->column++;
        token->type = HEX_TOKEN_STRING;
    }
    else if (strncmp(ptr, "0x", 2) == 0 || strncmp(ptr, "0X", 2) == 0)
    {
        // Hexadecimal integer token
        const char *start = ptr;
        ptr += 2; // Skip the "0x" prefix
        position->column += 2;
        while (isxdigit(*ptr))
        {
            ptr++;
            position->column++;
        }
        int len = ptr - start;
        token->value = (char *)malloc(len + 1);
        strncpy(token->value, start, len);
        token->value[len] = '\0';
        token->type = HEX_TOKEN_INTEGER;
    }
    else if (*ptr == '(')
    {
        token->type = HEX_TOKEN_QUOTATION_START;
        token->value = (char *)malloc(4); // Allocate extra space for safety
        strcpy(token->value, "(");
        ptr++;
        position->column++;
    }
    else if (*ptr == ')')
    {
        token->type = HEX_TOKEN_QUOTATION_END;
        token->value = (char *)malloc(4); // Allocate extra space for safety
        strcpy(token->value, ")");
        ptr++;
        position->column++;
    }
    else
    {
        // Symbol token
        const char *start = ptr;
        while (*ptr != '\0' && !isspace(*ptr) && *ptr != ';' && *ptr != '(' && *ptr != ')' && *ptr != '"')
        {
            ptr++;
            position->column++;
        }

        int len = ptr - start;
        token->value = (char *)malloc(len + 1);
        strncpy(token->value, start, len);
        token->value[len] = '\0';
        if (hex_valid_native_symbol(ctx, token->value) || hex_valid_user_symbol(ctx, token->value))
        {
            token->type = HEX_TOKEN_SYMBOL;
        }
        else
        {
            token->type = HEX_TOKEN_INVALID;
            token->position->line = position->line;
            token->position->column = position->column;
        }
    }
    *input = ptr;
    return token;
}

int hex_valid_native_symbol(hex_context_t *ctx, const char *symbol)
{
    hex_doc_entry_t *doc = malloc(sizeof(hex_doc_entry_t));
    if (hex_get_doc(ctx->docs, symbol, doc))
    {
        free(doc);
        return 1;
    }
    free(doc);
    return 0;
}

int32_t hex_parse_integer(const char *hex_str)
{
    // Parse the hexadecimal string as an unsigned 32-bit integer
    uint32_t unsigned_value = (uint32_t)strtoul(hex_str, NULL, 16);

    // Cast the unsigned value to a signed 32-bit integer
    return (int32_t)unsigned_value;
}

int hex_parse_quotation(hex_context_t *ctx, const char **input, hex_item_t *result, hex_file_position_t *position)
{
    hex_item_t **quotation = NULL;
    size_t capacity = 2;
    size_t size = 0;
    int balanced = 1;

    quotation = (hex_item_t **)calloc(capacity, sizeof(hex_item_t *));
    if (!quotation)
    {
        hex_error(ctx, "[parse quotation] Memory allocation failed");
        return 1;
    }

    hex_token_t *token;
    while ((token = hex_next_token(ctx, input, position)) != NULL)
    {
        if (token->type == HEX_TOKEN_QUOTATION_END)
        {
            balanced--;
            hex_free_token(token); // Free the end token
            break;
        }

        if (size >= capacity)
        {
            capacity *= 2;
            hex_item_t **new_quotation = (hex_item_t **)realloc(quotation, capacity * sizeof(hex_item_t *));
            if (!new_quotation)
            {
                hex_error(ctx, "(%d,%d), Memory allocation failed", position->line, position->column);
                hex_free_token(token);
                hex_free_list(ctx, quotation, size);
                return 1;
            }
            quotation = new_quotation;
        }

        hex_item_t *item = NULL;
        if (token->type == HEX_TOKEN_INTEGER)
        {
            item = hex_integer_item(ctx, hex_parse_integer(token->value));
            hex_free_token(token); // Token no longer needed for integers
        }
        else if (token->type == HEX_TOKEN_STRING)
        {
            item = hex_string_item(ctx, token->value);
            hex_free_token(token); // Token no longer needed for strings
        }
        else if (token->type == HEX_TOKEN_SYMBOL)
        {
            if (hex_valid_native_symbol(ctx, token->value))
            {
                item = calloc(1, sizeof(hex_item_t));
                if (item)
                {
                    hex_item_t *value = calloc(1, sizeof(hex_item_t));
                    if (hex_get_symbol(ctx, token->value, value))
                    {
                        item->type = HEX_TYPE_NATIVE_SYMBOL;
                        item->data.fn_value = value->data.fn_value;
                        item->token = token;
                        free(value); // wrapper only
                    }
                    else
                    {
                        hex_error(ctx, "(%d,%d) Unable to reference native symbol: %s", position->line, position->column, token->value);
                        free(item);
                        hex_free_token(token);
                        hex_free_list(ctx, quotation, size);
                        return 1;
                    }
                }
            }
            else
            {
                item = calloc(1, sizeof(hex_item_t));
                if (item)
                {
                    item->type = HEX_TYPE_USER_SYMBOL;
                    item->token = token;
                }
            }
            if (item && token->position && position->filename)
            {
                if (token->position->filename)
                {
                    free((void *)token->position->filename);
                }
                token->position->filename = strdup(position->filename);
            }
        }
        else if (token->type == HEX_TOKEN_QUOTATION_START)
        {
            item = calloc(1, sizeof(hex_item_t));
            if (item)
            {
                item->type = HEX_TYPE_QUOTATION;
                if (hex_parse_quotation(ctx, input, item, position) != 0)
                {
                    free(item);
                    hex_free_token(token);
                    hex_free_list(ctx, quotation, size);
                    return 1;
                }
            }
            hex_free_token(token); // Token no longer needed after parsing
        }
        else if (token->type == HEX_TOKEN_COMMENT)
        {
            // Ignore comments
            hex_free_token(token);
            continue;
        }
        else
        {
            hex_error(ctx, "(%d,%d) Unexpected token in quotation: %s", position->line, position->column, token->value);
            hex_free_token(token);
            hex_free_list(ctx, quotation, size);
            return 1;
        }

        if (!item)
        {
            hex_error(ctx, "(%d,%d) Failed to create item", position->line, position->column);
            hex_free_list(ctx, quotation, size);
            return 1;
        }

        quotation[size] = item;
        size++;
    }

    if (balanced != 0)
    {
        hex_error(ctx, "(%d,%d) Unterminated quotation", position->line, position->column);
        hex_free_list(ctx, quotation, size);
        return 1;
    }

    result->type = HEX_TYPE_QUOTATION;
    result->data.quotation_value = quotation;
    result->quotation_size = size;
    result->is_operator = 0;
    return 0;
}

/* File: src/symboltable.c */
#line 1 "src/symboltable.c"
#ifndef HEX_H
#include "hex.h"
#endif

int hex_symboltable_set(hex_context_t *ctx, const char *symbol)
{
    hex_symbol_table_t *table = ctx->symbol_table;
    size_t len = strlen(symbol);

    if (len > HEX_MAX_SYMBOL_LENGTH)
    {
        return -1;
    }

    if (table->count >= HEX_MAX_USER_SYMBOLS)
    {
        return -1; // Table full
    }

    for (uint16_t i = 0; i < table->count; ++i)
    {
        if (strcmp(table->symbols[i], symbol) == 0)
        {
            return 0;
        }
    }

    table->symbols[table->count] = strdup(symbol);
    table->count++;
    return 0;
}

int hex_symboltable_get_index(hex_context_t *ctx, const char *symbol)
{
    hex_symbol_table_t *table = ctx->symbol_table;
    for (uint16_t i = 0; i < table->count; ++i)
    {
        if (strcmp(table->symbols[i], symbol) == 0)
        {
            return i;
        }
    }
    return -1;
}

char *hex_symboltable_get_value(hex_context_t *ctx, uint16_t index)
{
    if (index >= ctx->symbol_table->count)
    {
        return NULL;
    }
    return ctx->symbol_table->symbols[index];
}

int hex_decode_bytecode_symboltable(hex_context_t *ctx, uint8_t **bytecode, size_t *size, size_t total)
{
    hex_symbol_table_t *table = ctx->symbol_table;
    table->count = 0;

    for (size_t i = 0; i < total; i++)
    {
        size_t len = (size_t)(*bytecode)[0];
        (*bytecode)++;
        *size -= 1;

        char *symbol = malloc(len + 1);
        if (symbol == NULL)
        {
            hex_error(ctx, "[decode symbol table] Memory allocation failed");
            // Handle memory allocation failure
            return -1;
        }
        memcpy(symbol, *bytecode, len);
        symbol[len] = '\0';
        hex_symboltable_set(ctx, symbol);
        free(symbol);
        *bytecode += len;
        *size -= len;
    }
    return 0;
}

uint8_t *hex_encode_bytecode_symboltable(hex_context_t *ctx, size_t *out_size)
{
    hex_symbol_table_t *table = ctx->symbol_table;
    size_t total_size = 0;

    for (uint16_t i = 0; i < table->count; ++i)
    {
        total_size += 1 + strlen(table->symbols[i]);
    }

    uint8_t *bytecode = malloc(total_size);
    size_t offset = 0;

    for (uint16_t i = 0; i < table->count; ++i)
    {
        size_t len = strlen(table->symbols[i]);
        bytecode[offset++] = (uint8_t)len;
        memcpy(bytecode + offset, table->symbols[i], len);
        offset += len;
    }

    *out_size = total_size;
    return bytecode;
}

hex_symbol_table_t *hex_symboltable_copy(hex_context_t *ctx)
{
    hex_symbol_table_t *original = ctx->symbol_table;
    hex_symbol_table_t *copy = malloc(sizeof(hex_symbol_table_t));
    if (copy == NULL)
    {
        hex_error(ctx, "[symbol table copy] Memory allocation failed");
        return NULL;
    }

    copy->symbols = malloc(sizeof(char *) * HEX_MAX_USER_SYMBOLS);
    if (copy->symbols == NULL)
    {
        hex_error(ctx, "[symbol table copy] Memory allocation failed for symbols array");
        free(copy);
        return NULL;
    }

    copy->count = original->count;
    for (uint16_t i = 0; i < original->count; ++i)
    {
        copy->symbols[i] = strdup(original->symbols[i]);
        if (copy->symbols[i] == NULL)
        {
            hex_error(ctx, "[symbol table copy] Memory allocation failed for symbol");
            // Free already allocated symbols
            for (uint16_t j = 0; j < i; ++j)
            {
                free(copy->symbols[j]);
            }
            free(copy->symbols);
            free(copy);
            return NULL;
        }
    }

    return copy;
}

void hex_symboltable_destroy(hex_symbol_table_t *table)
{
    if (!table)
    {
        return;
    }
    if (table->symbols)
    {
        for (uint16_t i = 0; i < table->count; ++i)
        {
            if (table->symbols[i])
            {
                free(table->symbols[i]);
                table->symbols[i] = NULL;
            }
        }
        free(table->symbols);
        table->symbols = NULL;
    }
    free(table);
}

/* File: src/opcodes.c */
#line 1 "src/opcodes.c"
#ifndef HEX_H
#include "hex.h"
#endif

uint8_t hex_symbol_to_opcode(const char *symbol)
{
    // Native Symbols
    if (strcmp(symbol, ":") == 0)
    {
        return HEX_OP_STORE;
    }
    else if (strcmp(symbol, "::") == 0)
    {
        return HEX_OP_DEFINE;
    }
    else if (strcmp(symbol, "#") == 0)
    {
        return HEX_OP_FREE;
    }
    else if (strcmp(symbol, "symbols") == 0)
    {
        return HEX_OP_SYMBOLS;
    }
    else if (strcmp(symbol, "if") == 0)
    {
        return HEX_OP_IF;
    }
    else if (strcmp(symbol, "while") == 0)
    {
        return HEX_OP_WHILE;
    }
    else if (strcmp(symbol, "error") == 0)
    {
        return HEX_OP_ERROR;
    }
    else if (strcmp(symbol, "try") == 0)
    {
        return HEX_OP_TRY;
    }
    else if (strcmp(symbol, "throw") == 0)
    {
        return HEX_OP_THROW;
    }
    else if (strcmp(symbol, "dup") == 0)
    {
        return HEX_OP_DUP;
    }
    else if (strcmp(symbol, "stack") == 0)
    {
        return HEX_OP_STACK;
    }
    else if (strcmp(symbol, "drop") == 0)
    {
        return HEX_OP_DROP;
    }
    else if (strcmp(symbol, "swap") == 0)
    {
        return HEX_OP_SWAP;
    }
    else if (strcmp(symbol, ".") == 0)
    {
        return HEX_OP_I;
    }
    else if (strcmp(symbol, "!") == 0)
    {
        return HEX_OP_EVAL;
    }
    else if (strcmp(symbol, "'") == 0)
    {
        return HEX_OP_QUOTE;
    }
    else if (strcmp(symbol, "+") == 0)
    {
        return HEX_OP_ADD;
    }
    else if (strcmp(symbol, "-") == 0)
    {
        return HEX_OP_SUBTRACT;
    }
    else if (strcmp(symbol, "*") == 0)
    {
        return HEX_OP_MULTIPLY;
    }
    else if (strcmp(symbol, "/") == 0)
    {
        return HEX_OP_DIVIDE;
    }
    else if (strcmp(symbol, "%") == 0)
    {
        return HEX_OP_MOD;
    }
    else if (strcmp(symbol, "&") == 0)
    {
        return HEX_OP_BITAND;
    }
    else if (strcmp(symbol, "|") == 0)
    {
        return HEX_OP_BITOR;
    }
    else if (strcmp(symbol, "^") == 0)
    {
        return HEX_OP_BITXOR;
    }
    else if (strcmp(symbol, "~") == 0)
    {
        return HEX_OP_BITNOT;
    }
    else if (strcmp(symbol, "<<") == 0)
    {
        return HEX_OP_SHL;
    }
    else if (strcmp(symbol, ">>") == 0)
    {
        return HEX_OP_SHR;
    }
    else if (strcmp(symbol, "==") == 0)
    {
        return HEX_OP_EQUAL;
    }
    else if (strcmp(symbol, "!=") == 0)
    {
        return HEX_OP_NOTEQUAL;
    }
    else if (strcmp(symbol, ">") == 0)
    {
        return HEX_OP_GREATER;
    }
    else if (strcmp(symbol, "<") == 0)
    {
        return HEX_OP_LESS;
    }
    else if (strcmp(symbol, ">=") == 0)
    {
        return HEX_OP_GREATEREQUAL;
    }
    else if (strcmp(symbol, "<=") == 0)
    {
        return HEX_OP_LESSEQUAL;
    }
    else if (strcmp(symbol, "and") == 0)
    {
        return HEX_OP_AND;
    }
    else if (strcmp(symbol, "or") == 0)
    {
        return HEX_OP_OR;
    }
    else if (strcmp(symbol, "not") == 0)
    {
        return HEX_OP_NOT;
    }
    else if (strcmp(symbol, "xor") == 0)
    {
        return HEX_OP_XOR;
    }
    else if (strcmp(symbol, "int") == 0)
    {
        return HEX_OP_INT;
    }
    else if (strcmp(symbol, "str") == 0)
    {
        return HEX_OP_STR;
    }
    else if (strcmp(symbol, "dec") == 0)
    {
        return HEX_OP_DEC;
    }
    else if (strcmp(symbol, "hex") == 0)
    {
        return HEX_OP_HEX;
    }
    else if (strcmp(symbol, "ord") == 0)
    {
        return HEX_OP_ORD;
    }
    else if (strcmp(symbol, "chr") == 0)
    {
        return HEX_OP_CHR;
    }
    else if (strcmp(symbol, "type") == 0)
    {
        return HEX_OP_TYPE;
    }
    else if (strcmp(symbol, "cat") == 0)
    {
        return HEX_OP_CAT;
    }
    else if (strcmp(symbol, "len") == 0)
    {
        return HEX_OP_LEN;
    }
    else if (strcmp(symbol, "get") == 0)
    {
        return HEX_OP_GET;
    }
    else if (strcmp(symbol, "index") == 0)
    {
        return HEX_OP_INDEX;
    }
    else if (strcmp(symbol, "join") == 0)
    {
        return HEX_OP_JOIN;
    }
    else if (strcmp(symbol, "split") == 0)
    {
        return HEX_OP_SPLIT;
    }
    else if (strcmp(symbol, "sub") == 0)
    {
        return HEX_OP_SUB;
    }
    else if (strcmp(symbol, "map") == 0)
    {
        return HEX_OP_MAP;
    }
    else if (strcmp(symbol, "puts") == 0)
    {
        return HEX_OP_PUTS;
    }
    else if (strcmp(symbol, "warn") == 0)
    {
        return HEX_OP_WARN;
    }
    else if (strcmp(symbol, "print") == 0)
    {
        return HEX_OP_PRINT;
    }
    else if (strcmp(symbol, "gets") == 0)
    {
        return HEX_OP_GETS;
    }
    else if (strcmp(symbol, "read") == 0)
    {
        return HEX_OP_READ;
    }
    else if (strcmp(symbol, "write") == 0)
    {
        return HEX_OP_WRITE;
    }
    else if (strcmp(symbol, "append") == 0)
    {
        return HEX_OP_APPEND;
    }
    else if (strcmp(symbol, "args") == 0)
    {
        return HEX_OP_ARGS;
    }
    else if (strcmp(symbol, "exit") == 0)
    {
        return HEX_OP_EXIT;
    }
    else if (strcmp(symbol, "exec") == 0)
    {
        return HEX_OP_EXEC;
    }
    else if (strcmp(symbol, "run") == 0)
    {
        return HEX_OP_RUN;
    }
    else if (strcmp(symbol, "timestamp") == 0)
    {
        return HEX_OP_TIMESTAMP;
    }
    return 0;
}

const char *hex_opcode_to_symbol(uint8_t opcode)
{
    switch (opcode)
    {
    case HEX_OP_STORE:
        return ":";
    case HEX_OP_DEFINE:
        return "::";
    case HEX_OP_FREE:
        return "#";
    case HEX_OP_SYMBOLS:
        return "symbols";
    case HEX_OP_IF:
        return "if";
    case HEX_OP_WHILE:
        return "while";
    case HEX_OP_ERROR:
        return "error";
    case HEX_OP_TRY:
        return "try";
    case HEX_OP_THROW:
        return "throw";
    case HEX_OP_DUP:
        return "dup";
    case HEX_OP_STACK:
        return "stack";
    case HEX_OP_DROP:
        return "drop";
    case HEX_OP_SWAP:
        return "swap";
    case HEX_OP_I:
        return ".";
    case HEX_OP_EVAL:
        return "!";
    case HEX_OP_QUOTE:
        return "'";
    case HEX_OP_ADD:
        return "+";
    case HEX_OP_SUBTRACT:
        return "-";
    case HEX_OP_MULTIPLY:
        return "*";
    case HEX_OP_DIVIDE:
        return "/";
    case HEX_OP_MOD:
        return "%";
    case HEX_OP_BITAND:
        return "&";
    case HEX_OP_BITOR:
        return "|";
    case HEX_OP_BITXOR:
        return "^";
    case HEX_OP_BITNOT:
        return "~";
    case HEX_OP_SHL:
        return "<<";
    case HEX_OP_SHR:
        return ">>";
    case HEX_OP_EQUAL:
        return "==";
    case HEX_OP_NOTEQUAL:
        return "!=";
    case HEX_OP_GREATER:
        return ">";
    case HEX_OP_LESS:
        return "<";
    case HEX_OP_GREATEREQUAL:
        return ">=";
    case HEX_OP_LESSEQUAL:
        return "<=";
    case HEX_OP_AND:
        return "and";
    case HEX_OP_OR:
        return "or";
    case HEX_OP_NOT:
        return "not";
    case HEX_OP_XOR:
        return "xor";
    case HEX_OP_INT:
        return "int";
    case HEX_OP_STR:
        return "str";
    case HEX_OP_DEC:
        return "dec";
    case HEX_OP_HEX:
        return "hex";
    case HEX_OP_ORD:
        return "ord";
    case HEX_OP_CHR:
        return "chr";
    case HEX_OP_TYPE:
        return "type";
    case HEX_OP_CAT:
        return "cat";
    case HEX_OP_LEN:
        return "len";
    case HEX_OP_GET:
        return "get";
    case HEX_OP_INDEX:
        return "index";
    case HEX_OP_JOIN:
        return "join";
    case HEX_OP_SPLIT:
        return "split";
    case HEX_OP_SUB:
        return "sub";
    case HEX_OP_MAP:
        return "map";
    case HEX_OP_PUTS:
        return "puts";
    case HEX_OP_WARN:
        return "warn";
    case HEX_OP_PRINT:
        return "print";
    case HEX_OP_GETS:
        return "gets";
    case HEX_OP_READ:
        return "read";
    case HEX_OP_WRITE:
        return "write";
    case HEX_OP_APPEND:
        return "append";
    case HEX_OP_ARGS:
        return "args";
    case HEX_OP_EXIT:
        return "exit";
    case HEX_OP_EXEC:
        return "exec";
    case HEX_OP_RUN:
        return "run";
    case HEX_OP_TIMESTAMP:
        return "timestamp";
    default:
        return NULL;
    }
}

/* File: src/vm.c */
#line 1 "src/vm.c"
#ifndef HEX_H
#include "hex.h"
#endif

////////////////////////////////////////
// Virtual Machine                    //
////////////////////////////////////////

int hex_bytecode_integer(hex_context_t *ctx, uint8_t **bytecode, size_t *size, size_t *capacity, int32_t value)
{
    hex_debug(ctx, "PUSHIN[01]: 0x%x", value);
    // Check if we need to resize the buffer (size + int32_t size + opcode (1) + max encoded length (4))
    if (*size + sizeof(int32_t) + 1 + 4 > *capacity)
    {
        *capacity = (*size + sizeof(int32_t) + 1 + 4 + *capacity) * 2;
        uint8_t *new_bytecode = (uint8_t *)realloc(*bytecode, *capacity);
        if (!new_bytecode)
        {
            hex_error(ctx, "[add bytecode integer] Memory allocation failed");
            return 1;
        }
        *bytecode = new_bytecode;
    }
    (*bytecode)[*size] = HEX_OP_PUSHIN;
    *size += 1; // opcode
    // Encode the length of the integer value
    size_t bytes = hex_min_bytes_to_encode_integer(value);
    hex_encode_length(bytecode, size, bytes);
    // Encode the integer value in the minimum number of bytes, in little endian
    if (bytes == 1)
    {
        (*bytecode)[*size] = value & 0xFF;
        *size += 1;
    }
    else if (bytes == 2)
    {
        (*bytecode)[*size] = value & 0xFF;
        (*bytecode)[*size + 1] = (value >> 8) & 0xFF;
        *size += 2;
    }
    else if (bytes == 3)
    {
        (*bytecode)[*size] = value & 0xFF;
        (*bytecode)[*size + 1] = (value >> 8) & 0xFF;
        (*bytecode)[*size + 2] = (value >> 16) & 0xFF;
        *size += 3;
    }
    else
    {
        (*bytecode)[*size] = value & 0xFF;
        (*bytecode)[*size + 1] = (value >> 8) & 0xFF;
        (*bytecode)[*size + 2] = (value >> 16) & 0xFF;
        (*bytecode)[*size + 3] = (value >> 24) & 0xFF;
        *size += 4;
    }
    return 0;
}

int hex_bytecode_string(hex_context_t *ctx, uint8_t **bytecode, size_t *size, size_t *capacity, const char *value)
{
    char *str = hex_process_string(value);
    if (!str)
    {
        hex_error(ctx, "[add bytecode string] Memory allocation failed");
        return 1;
    }
    hex_debug(ctx, "PUSHST[02]: \"%s\"", str);
    size_t len = strlen(value);
    // Check if we need to resize the buffer (size + strlen + opcode (1) + max encoded length (4))
    if (*size + len + 1 + 4 > *capacity)
    {
        *capacity = (*size + len + 1 + 4 + *capacity) * 2;
        uint8_t *new_bytecode = (uint8_t *)realloc(*bytecode, *capacity);
        if (!new_bytecode)
        {
            hex_error(ctx, "[add bytecode string] Memory allocation failed");
            return 1;
        }
        *bytecode = new_bytecode;
    }
    (*bytecode)[*size] = HEX_OP_PUSHST;
    *size += 1; // opcode
    // Check for multi-byte characters
    for (size_t i = 0; i < len; i++)
    {
        if ((value[i] & 0x80) != 0)
        {
            hex_error(ctx, "[add bytecode string] Multi-byte characters are not supported - Cannot encode string: \"%s\"", value);
            free(str);
            return 1;
        }
    }
    hex_encode_length(bytecode, size, len);
    memcpy(&(*bytecode)[*size], value, len);
    *size += len;
    return 0;
}

int hex_bytecode_symbol(hex_context_t *ctx, uint8_t **bytecode, size_t *size, size_t *capacity, const char *value)
{
    if (hex_valid_native_symbol(ctx, value))
    {
        // Check if we need to resize the buffer (size + opcode (1))
        if (*size + 1 > *capacity)
        {
            *capacity = (*size + 1 + *capacity) * 2;
            uint8_t *new_bytecode = (uint8_t *)realloc(*bytecode, *capacity);
            if (!new_bytecode)
            {
                hex_error(ctx, "[add bytecode native symbol] Memory allocation failed");
                return 1;
            }
            *bytecode = new_bytecode;
        }
        uint8_t opcode = hex_symbol_to_opcode(value);
        hex_debug(ctx, "NATSYM[%02x]: %s", opcode, value);
        (*bytecode)[*size] = opcode;
        *size += 1; // opcode
    }
    else
    {
        // Add to symbol table
        hex_symboltable_set(ctx, value);
        int index = hex_symboltable_get_index(ctx, value);
        hex_debug(ctx, "LOOKUP[00]: %02x -> %s", index, value);
        //  Check if we need to resize the buffer (size + 1 opcode + 2 max index)
        if (*size + 1 + 2 > *capacity)
        {
            *capacity = (*size + 1 + 2) * 2;
            uint8_t *new_bytecode = (uint8_t *)realloc(*bytecode, *capacity);
            if (!new_bytecode)
            {
                hex_error(ctx, "[add bytecode user symbol] Memory allocation failed");
                return 1;
            }
            *bytecode = new_bytecode;
        }
        (*bytecode)[*size] = HEX_OP_LOOKUP;
        *size += 1; // opcode
        // Add index to bytecode (little endian)
        (*bytecode)[*size] = index & 0xFF;
        (*bytecode)[*size + 1] = (index >> 8) & 0xFF;
        *size += 2;
    }
    return 0;
}

int hex_bytecode_quotation(hex_context_t *ctx, uint8_t **bytecode, size_t *size, size_t *capacity, uint8_t **output, size_t *output_size, size_t *n_items)
{
    //  Check if we need to resize the buffer (size + opcode (1) + max encoded length (4) + quotation bytecode size)
    if (*size + 1 + 4 + *output_size > *capacity)
    {
        *capacity = (*size + 1 + 4 + *output_size + *capacity) * 2;
        uint8_t *new_bytecode = (uint8_t *)realloc(*bytecode, *capacity);
        if (!new_bytecode)
        {
            hex_error(ctx, "Memory allocation failed");
            return 1;
        }
        *bytecode = new_bytecode;
    }
    (*bytecode)[*size] = HEX_OP_PUSHQT;
    *size += 1; // opcode
    hex_encode_length(bytecode, size, *n_items);
    memcpy(&(*bytecode)[*size], *output, *output_size);
    *size += *output_size;
    hex_debug(ctx, "PUSHQT[03]: <end> (items: %d)", *n_items);
    return 0;
}

int hex_bytecode(hex_context_t *ctx, const char *input, uint8_t **output, size_t *output_size, hex_file_position_t *position)
{
    hex_token_t *token;
    size_t capacity = 128;
    size_t size = 0;
    uint8_t *bytecode = (uint8_t *)malloc(capacity);
    if (!bytecode)
    {
        hex_error(ctx, "[generate bytecode] Memory allocation failed");
        return 1;
    }
    hex_debug(ctx, "Generating bytecode");
    while ((token = hex_next_token(ctx, &input, position)) != NULL)
    {
        if (token->type == HEX_TOKEN_INTEGER)
        {
            int32_t value = hex_parse_integer(token->value);
            hex_bytecode_integer(ctx, &bytecode, &size, &capacity, value);
        }
        else if (token->type == HEX_TOKEN_STRING)
        {
            hex_bytecode_string(ctx, &bytecode, &size, &capacity, token->value);
        }
        else if (token->type == HEX_TOKEN_SYMBOL)
        {
            hex_bytecode_symbol(ctx, &bytecode, &size, &capacity, token->value);
        }
        else if (token->type == HEX_TOKEN_QUOTATION_START)
        {
            size_t n_items = 0;
            uint8_t *quotation_bytecode = NULL;
            size_t quotation_size = 0;
            hex_debug(ctx, "PUSHQT[03]: <start>");
            if (hex_generate_quotation_bytecode(ctx, &input, &quotation_bytecode, &quotation_size, &n_items, position) != 0)
            {
                hex_error(ctx, "[generate quotation bytecode] Failed to generate quotation bytecode (main)");
                return 1;
            }
            hex_bytecode_quotation(ctx, &bytecode, &size, &capacity, &quotation_bytecode, &quotation_size, &n_items);
        }
        else if (token->type == HEX_TOKEN_QUOTATION_END)
        {
            hex_error(ctx, "(%d, %d) Unexpected end of quotation", position->line, position->column);
            return 1;
        }
        else
        {
            //   Ignore other tokens
        }
    }
    hex_debug(ctx, "Bytecode generated: %d bytes", size);
    *output = bytecode;
    *output_size = size;
    return 0;
}

int hex_generate_quotation_bytecode(hex_context_t *ctx, const char **input, uint8_t **output, size_t *output_size, size_t *n_items, hex_file_position_t *position)
{
    hex_token_t *token;
    size_t capacity = 128;
    size_t size = 0;
    int balanced = 1;
    uint8_t *bytecode = (uint8_t *)malloc(capacity);
    if (!bytecode)
    {
        hex_error(ctx, "[generate quotation bytecode] Memory allocation failed");
        return 1;
    }
    *n_items = 0;

    while ((token = hex_next_token(ctx, input, position)) != NULL)
    {
        if (token->type == HEX_TOKEN_INTEGER)
        {
            int32_t value = hex_parse_integer(token->value);
            hex_bytecode_integer(ctx, &bytecode, &size, &capacity, value);
        }
        else if (token->type == HEX_TOKEN_STRING)
        {
            hex_bytecode_string(ctx, &bytecode, &size, &capacity, token->value);
        }
        else if (token->type == HEX_TOKEN_SYMBOL)
        {
            hex_bytecode_symbol(ctx, &bytecode, &size, &capacity, token->value);
        }
        else if (token->type == HEX_TOKEN_QUOTATION_START)
        {
            size_t n_items = 0;
            uint8_t *quotation_bytecode = NULL;
            size_t quotation_size = 0;
            hex_debug(ctx, "PUSHQT[03]: <start>");
            if (hex_generate_quotation_bytecode(ctx, input, &quotation_bytecode, &quotation_size, &n_items, position) != 0)
            {
                hex_error(ctx, "[generate quotation bytecode] Failed to generate quotation bytecode");
                return 1;
            }
            hex_bytecode_quotation(ctx, &bytecode, &size, &capacity, &quotation_bytecode, &quotation_size, &n_items);
        }
        else if (token->type == HEX_TOKEN_QUOTATION_END)
        {
            balanced--;
            break;
        }
        else
        {
            (*n_items)--; // Decrement the number of items if it's not a valid token (it will be incremeneted anyway)
            // Ignore other tokens
        }

        (*n_items)++;
    }
    if (balanced != 0)
    {
        hex_error(ctx, "(%d,%d) Unterminated quotation", position->line, position->column);
        hex_free_token(token);
        return 1;
    }
    *output = bytecode;
    *output_size = size;
    return 0;
}

int hex_interpret_bytecode_integer(hex_context_t *ctx, uint8_t **bytecode, size_t *size, hex_item_t *result)
{
    size_t length = 0;
    int32_t value = 0; // Use signed 32-bit integer to handle negative values
    int shift = 0;

    // Decode the variable-length integer for the length
    do
    {
        if (*size == 0)
        {
            hex_error(ctx, "[interpret bytecode integer] Bytecode size too small to contain an integer length");
            return 1;
        }
        length |= ((**bytecode & 0x7F) << shift);
        shift += 7;
    } while (*(*bytecode)++ & 0x80);

    *size -= shift / 7;

    if (*size < length)
    {
        hex_error(ctx, "[interpret bytecode integer] Bytecode size too small to contain the integer value");
        return 1;
    }

    // Decode the integer value based on the length in little-endian format
    value = 0;
    for (size_t i = 0; i < length; i++)
    {
        value |= (*bytecode)[i] << (8 * i); // Accumulate in little-endian order
    }

    // Handle sign extension for 32-bit value
    if (length == 4)
    {
        // If the value is negative, we need to sign-extend it.
        if (value & 0x80000000)
        {                        // If the sign bit is set (negative value)
            value |= 0xFF000000; // Sign-extend to 32 bits
        }
    }

    *bytecode += length;
    *size -= length;

    hex_debug(ctx, ">> PUSHIN[01]: 0x%x", value);
    HEX_ALLOC(item)
    item = hex_integer_item(ctx, value);
    *result = *item;
    return 0;
}

int hex_interpret_bytecode_string(hex_context_t *ctx, uint8_t **bytecode, size_t *size, hex_item_t *result)
{
    size_t length = 0;
    int shift = 0;

    // Decode the variable-length integer for the string length
    do
    {
        if (*size == 0)
        {
            hex_error(ctx, "[interpret bytecode string] Bytecode size too small to contain a string length");
            return 1;
        }
        length |= ((**bytecode & 0x7F) << shift);
        shift += 7;
        (*bytecode)++;
        (*size)--;
    } while (**bytecode & 0x80);

    if (*size < length)
    {
        hex_error(ctx, "[interpret bytecode string] Bytecode size (%d) too small to contain a string of length %d", *size, length);
        return 1;
    }

    char *value = (char *)malloc(length + 1);
    if (!value)
    {
        hex_error(ctx, "[interpret bytecode string] Memory allocation failed");
        return 1;
    }
    memcpy(value, *bytecode, length);
    value[length] = '\0';
    *bytecode += length;
    *size -= length;

    HEX_ALLOC(item);
    item = hex_string_item(ctx, value);
    *result = *item;
    char *str = hex_process_string(value);
    if (!str)
    {
        hex_error(ctx, "[interpret bytecode string] Memory allocation failed");
        return 1;
    }
    hex_debug(ctx, ">> PUSHST[02]: \"%s\"", str);
    return 0;
}

int hex_interpret_bytecode_native_symbol(hex_context_t *ctx, uint8_t opcode, size_t position, const char *filename, hex_item_t *result)
{

    const char *symbol = hex_opcode_to_symbol(opcode);
    if (!symbol)
    {
        hex_error(ctx, "[interpret bytecode native symbol] Invalid opcode for symbol");
        return 1;
    }

    HEX_ALLOC(item);
    item->type = HEX_TYPE_NATIVE_SYMBOL;
    HEX_ALLOC(value);
    hex_token_t *token = (hex_token_t *)malloc(sizeof(hex_token_t));
    token->value = strdup(symbol);
    token->position = (hex_file_position_t *)malloc(sizeof(hex_file_position_t));
    token->position->filename = strdup(filename);
    token->position->line = 0;
    token->position->column = position;
    if (hex_get_symbol(ctx, token->value, value))
    {
        item->token = token;
        item->type = HEX_TYPE_NATIVE_SYMBOL;
        item->data.fn_value = value->data.fn_value;
    }
    else
    {
        hex_error(ctx, "(%d,%d) Unable to reference native symbol: %s (bytecode)", token->position->line, token->position->column, token->value);
        hex_free_token(token);
        return 1;
    }
    hex_debug(ctx, ">> NATSYM[%02x]: %s", opcode, token->value);
    *result = *item;
    return 0;
}

int hex_interpret_bytecode_user_symbol(hex_context_t *ctx, uint8_t **bytecode, size_t *size, size_t position, const char *filename, hex_item_t *result)
{
    // Get the index of the symbol (one byte)
    if (*size == 0)
    {
        hex_error(ctx, "[interpret bytecode user symbol] Bytecode size too small to contain a symbol length");
        return 1;
    }
    size_t index = **bytecode;
    (*bytecode)++;
    (*size)--;

    if (index >= ctx->symbol_table->count)
    {
        hex_error(ctx, "[interpret bytecode user symbol] Symbol index out of bounds");
        return 1;
    }
    char *value = hex_symboltable_get_value(ctx, index);
    size_t length = strlen(value);

    if (!value)
    {
        hex_error(ctx, "[interpret bytecode user symbol] Memory allocation failed");
        return 1;
    }

    *bytecode += 1;
    *size -= 1;

    hex_token_t *token = (hex_token_t *)malloc(sizeof(hex_token_t));

    token->value = (char *)malloc(length + 1);
    strncpy(token->value, value, length + 1);
    token->position = (hex_file_position_t *)malloc(sizeof(hex_file_position_t));
    token->position->filename = strdup(filename);
    token->position->line = 0;
    token->position->column = position;

    hex_item_t item;
    item.type = HEX_TYPE_USER_SYMBOL;
    item.token = token;

    hex_debug(ctx, ">> LOOKUP[00]: %02x -> %s", index, value);
    *result = item;
    return 0;
}

int hex_interpret_bytecode_quotation(hex_context_t *ctx, uint8_t **bytecode, size_t *size, size_t position, const char *filename, hex_item_t *result)
{
    size_t n_items = 0;
    int shift = 0;

    // Decode the variable-length integer for the number of items

    do
    {
        if (*size == 0)
        {
            hex_error(ctx, "[interpret bytecode quotation] Bytecode size too small to contain a quotation length");
            return 1;
        }

        // Extract the current byte
        uint8_t current_byte = **bytecode;

        // Add the lower 7 bits of the current byte to the result
        n_items |= ((current_byte & 0x7F) << shift);

        // Check if MSB is set (continuation flag)
        if ((current_byte & 0x80) == 0)
        {
            // Last byte of the integer; decoding complete
            break;
        }

        // Update shift for the next byte
        shift += 7;

        // Prevent overflow for excessively large shifts
        if (shift >= 32)
        {
            hex_error(ctx, "[interpret bytecode quotation] Shift overflow while decoding variable-length integer");
            return 1;
        }

        // Move to the next byte
        (*bytecode)++;
        (*size)--;

    } while (1); // Loop until the break condition is met

    // Move to the next byte after the loop
    (*bytecode)++;
    (*size)--;

    hex_debug(ctx, ">> PUSHQT[03]: <start> (items: %zu)", n_items);

    hex_item_t **items = (hex_item_t **)malloc(n_items * sizeof(hex_item_t));
    if (!items)
    {
        hex_error(ctx, "[interpret bytecode quotation] Memory allocation failed");
        return 1;
    }

    for (size_t i = 0; i < n_items; i++)
    {
        uint8_t opcode = **bytecode;
        (*bytecode)++;
        (*size)--;

        HEX_ALLOC(item);
        switch (opcode)
        {
        case HEX_OP_PUSHIN:
            if (hex_interpret_bytecode_integer(ctx, bytecode, size, item) != 0)
            {
                hex_free_list(ctx, items, n_items);
                return 1;
            }
            break;
        case HEX_OP_PUSHST:
            if (hex_interpret_bytecode_string(ctx, bytecode, size, item) != 0)
            {
                hex_free_list(ctx, items, n_items);
                return 1;
            }
            break;
        case HEX_OP_LOOKUP:
            if (hex_interpret_bytecode_user_symbol(ctx, bytecode, size, position, filename, item) != 0)
            {
                hex_free_list(ctx, items, n_items);
                return 1;
            }
            break;
        case HEX_OP_PUSHQT:
            if (hex_interpret_bytecode_quotation(ctx, bytecode, size, position, filename, item) != 0)
            {
                hex_free_list(ctx, items, n_items);
                return 1;
            }
            break;
        default:
            if (hex_interpret_bytecode_native_symbol(ctx, opcode, *size, filename, item) != 0)
            {
                hex_free_list(ctx, items, n_items);
                return 1;
            }
            break;
        }
        items[i] = item;
    }
    result->type = HEX_TYPE_QUOTATION;
    result->data.quotation_value = items;
    result->quotation_size = n_items;

    hex_debug(ctx, ">> PUSHQT[03]: <end> (items: %zu)", n_items);
    return 0;
}

int hex_interpret_bytecode(hex_context_t *ctx, uint8_t *bytecode, size_t size, const char *filename)
{
    size_t bytecode_size = size;
    size_t position = bytecode_size;
    uint8_t header[8];
    if (size < 8)
    {
        hex_error(ctx, "[interpret bytecode header] Bytecode size too small to contain a header");
        return 1;
    }
    memcpy(header, bytecode, 8);
    int symbol_table_size = hex_validate_header(header);
    hex_debug(ctx, "[Hex Bytecode eXecutable File - version: %d - symbols: %d]", header[4], symbol_table_size);
    if (symbol_table_size < 0)
    {
        hex_error(ctx, "[interpret bytecode header] Invalid bytecode header");
        return 1;
    }
    bytecode += 8;
    size -= 8;
    // Extract the symbol table
    if (symbol_table_size > 0)
    {
        if (hex_decode_bytecode_symboltable(ctx, &bytecode, &size, symbol_table_size) != 0)
        {
            hex_error(ctx, "[interpret bytecode symbol table] Failed to decode the symbol table");
            return 1;
        }
    }
    // Debug: Print all symbols in the symbol table
    hex_debug(ctx, "--- Symbol Table Start ---");
    for (size_t i = 0; i < ctx->symbol_table->count; i++)
    {
        hex_debug(ctx, "%03d: %s", i, ctx->symbol_table->symbols[i]);
    }
    hex_debug(ctx, "---  Symbol Table End  ---");
    while (size > 0)
    {
        position = bytecode_size - size;
        uint8_t opcode = *bytecode;
        hex_debug(ctx, "-- [%08d] OPCODE: %02x", position, opcode);
        bytecode++;
        size--;

        HEX_ALLOC(item);
        switch (opcode)
        {
        case HEX_OP_PUSHIN:
            if (hex_interpret_bytecode_integer(ctx, &bytecode, &size, item) != 0)
            {
                HEX_FREE(ctx, item);
                return 1;
            }
            break;
        case HEX_OP_PUSHST:
            if (hex_interpret_bytecode_string(ctx, &bytecode, &size, item) != 0)
            {
                HEX_FREE(ctx, item);
                return 1;
            }
            break;
        case HEX_OP_LOOKUP:
            if (hex_interpret_bytecode_user_symbol(ctx, &bytecode, &size, position, filename, item) != 0)
            {
                HEX_FREE(ctx, item);
                return 1;
            }
            break;
        case HEX_OP_PUSHQT:

            if (hex_interpret_bytecode_quotation(ctx, &bytecode, &size, position, filename, item) != 0)
            {
                HEX_FREE(ctx, item);
                return 1;
            }
            break;
        default:
            if (hex_interpret_bytecode_native_symbol(ctx, opcode, position, filename, item) != 0)
            {
                HEX_FREE(ctx, item);
                return 1;
            }
            break;
        }
        if (hex_push(ctx, item) != 0)
        {
            HEX_FREE(ctx, item);
            return 1;
        }
    }
    return 0;
}

void hex_header(hex_context_t *ctx, uint8_t header[8])
{
    header[0] = 0x01;
    header[1] = 'h';
    header[2] = 'e';
    header[3] = 'x';
    header[4] = 0x01; // version
    uint16_t symbol_table_size = (uint16_t)ctx->symbol_table->count;
    header[5] = symbol_table_size & 0xFF;
    header[6] = (symbol_table_size >> 8) & 0xFF;
    header[7] = 0x02;
}

int hex_validate_header(uint8_t header[8])
{
    if (header[0] != 0x01 || header[1] != 'h' || header[2] != 'e' || header[3] != 'x' || header[4] != 0x01 || header[7] != 0x02)
    {
        return -1;
    }
    uint16_t symbol_table_size = header[5] | (header[6] << 8);
    return symbol_table_size;
}

/* File: src/interpreter.c */
#line 1 "src/interpreter.c"
#ifndef HEX_H
#include "hex.h"
#endif

////////////////////////////////////////
// Hex Interpreter Implementation     //
////////////////////////////////////////

hex_context_t *hex_init()
{
    hex_context_t *context = malloc(sizeof(hex_context_t));
    if (!context)
        return NULL;

    context->argc = 0;
    context->argv = NULL;
    context->registry = hex_registry_create();
    context->docs = malloc(sizeof(hex_doc_dictionary_t));
    if (context->docs)
    {
        context->docs->entries = malloc(HEX_NATIVE_SYMBOLS * sizeof(hex_doc_entry_t));
        context->docs->size = 0;
    }
    context->stack = malloc(sizeof(hex_stack_t));
    if (context->stack)
    {
        context->stack->entries = malloc(HEX_STACK_SIZE * sizeof(hex_item_t *));
        context->stack->top = -1;
        context->stack->capacity = HEX_STACK_SIZE;
        // Initialize all entries to NULL
        for (int i = 0; i < HEX_STACK_SIZE; i++)
        {
            context->stack->entries[i] = NULL;
        }
    }
    context->stack_trace = malloc(sizeof(hex_stack_trace_t));
    if (context->stack_trace)
    {
        context->stack_trace->entries = malloc(HEX_STACK_TRACE_SIZE * sizeof(hex_token_t *));
        context->stack_trace->start = 0;
        context->stack_trace->size = 0;
        // Initialize all entries to NULL
        for (int i = 0; i < HEX_STACK_TRACE_SIZE; i++)
        {
            context->stack_trace->entries[i] = NULL;
        }
    }
    context->settings = malloc(sizeof(hex_settings_t));
    if (context->settings)
    {
        context->settings->debugging_enabled = 0;
        context->settings->errors_enabled = 1;
        context->settings->stack_trace_enabled = 1;
    }
    context->symbol_table = malloc(sizeof(hex_symbol_table_t));
    if (context->symbol_table)
    {
        context->symbol_table->count = 0;
        context->symbol_table->symbols = malloc(HEX_MAX_USER_SYMBOLS * sizeof(char *));
        // Initialize all symbols to NULL
        for (int i = 0; i < HEX_MAX_USER_SYMBOLS; i++)
        {
            context->symbol_table->symbols[i] = NULL;
        }
    }
    return context;
}

int hex_interpret(hex_context_t *ctx, const char *code, const char *filename, int line, int column)
{

    const char *input = code;
    hex_file_position_t position = {filename, line, column};
    hex_token_t *token = hex_next_token(ctx, &input, &position);

    while (token != NULL && token->type != HEX_TOKEN_INVALID)
    {
        int result = 0;
        if (token->type == HEX_TOKEN_INTEGER)
        {
            result = hex_push_integer(ctx, hex_parse_integer(token->value));
        }
        else if (token->type == HEX_TOKEN_STRING)
        {
            result = hex_push_string(ctx, token->value);
        }
        else if (token->type == HEX_TOKEN_SYMBOL)
        {
            if (token->position && filename)
            {
                if (token->position->filename)
                {
                    free((void *)token->position->filename);
                }
                token->position->filename = strdup(filename);
            }
            result = hex_push_symbol(ctx, token);
        }
        else if (token->type == HEX_TOKEN_QUOTATION_END)
        {
            hex_error(ctx, "(%d,%d) Unexpected end of quotation", position.line, position.column);
            result = 1;
        }
        else if (token->type == HEX_TOKEN_QUOTATION_START)
        {
            hex_item_t *quotationItem = (hex_item_t *)malloc(sizeof(hex_item_t));
            if (!quotationItem)
            {
                hex_error(ctx, "(%d,%d) Failed to allocate memory for quotation", position.line, position.column);
                result = 1;
            }
            else if (hex_parse_quotation(ctx, &input, quotationItem, &position) != 0)
            {
                hex_error(ctx, "(%d,%d) Failed to parse quotation", position.line, position.column);
                free(quotationItem);
                result = 1;
            }
            else
            {
                result = hex_push_quotation(ctx, quotationItem->data.quotation_value, quotationItem->quotation_size);
                // Don't free quotationItem here as its content is now owned by the stack
                free(quotationItem); // Only free the wrapper, not the contents
            }
        }

        if (result != 0)
        {
            hex_error(ctx, "[interpret] Unable to push: %s", token->value);
            hex_free_token(token);
            print_stack_trace(ctx);
            return result;
        }

        // Free the token after successful processing (unless it's been stored somewhere)
        if (token->type != HEX_TOKEN_SYMBOL && token->type != HEX_TOKEN_QUOTATION_START)
        {
            hex_free_token(token);
        }

        token = hex_next_token(ctx, &input, &position);
    }
    if (token != NULL && token->type == HEX_TOKEN_INVALID)
    {
        token->position->filename = strdup(filename);
        add_to_stack_trace(ctx, token);
        print_stack_trace(ctx);
        return 1;
    }
    return 0;
}

/* File: src/utils.c */
#line 1 "src/utils.c"
#ifndef HEX_H
#include "hex.h"
#endif

////////////////////////////////////////
// Helper Functions                   //
////////////////////////////////////////

#if defined(__EMSCRIPTEN__)
#include <emscripten.h>

EM_ASYNC_JS(char *, em_fgets, (const char *buf, size_t bufsize), {
    return await new Promise(function(resolve, reject) {
               if (Module.pending_lines.length > 0)
               {
                   resolve(Module.pending_lines.shift());
               }
               else
               {
                   Module.pending_fgets.push(resolve);
               }
           })
        .then(function(s) {
            // convert JS string to WASM string
            let l = s.length + 1;
            if (l >= bufsize)
            {
                // truncate
                l = bufsize - 1;
            }
            Module.stringToUTF8(s.slice(0, l), buf, l);
            return buf;
        });
});

#endif

void hex_rpad(const char *str, int total_length)
{
    int len = strlen(str);
    printf("%s", str);
    for (int i = len; i < total_length; i++)
    {
        printf(" ");
    }
}
void hex_lpad(const char *str, int total_length)
{
    int len = strlen(str);
    for (int i = len; i < total_length; i++)
    {
        printf(" ");
    }
    printf("%s", str);
}

char *hex_itoa(int num, int base)
{
    static char str[20];
    int i = 0;

    // Handle 0 explicitly, otherwise empty string is printed
    if (num == 0)
    {
        str[i++] = '0';
        str[i] = '\0';
        return str;
    }

    // Process each digit
    while (num != 0)
    {
        int rem = num % base;
        str[i++] = (rem > 9) ? (rem - 10) + 'a' : rem + '0';
        num = num / base;
    }

    str[i] = '\0'; // Null-terminate string

    // Reverse the string
    int start = 0;
    int end = i - 1;
    while (start < end)
    {
        char temp = str[start];
        str[start] = str[end];
        str[end] = temp;
        start++;
        end--;
    }

    return str;
}

char *hex_itoa_dec(int num)
{
    return hex_itoa(num, 10);
}

char *hex_itoa_hex(int num)
{
    return hex_itoa(num, 16);
}

void hex_raw_print_item(FILE *stream, hex_item_t item)
{
    switch (item.type)
    {
    case HEX_TYPE_INTEGER:
        fprintf(stream, "0x%x", item.data.int_value);
        break;
    case HEX_TYPE_STRING:
        fprintf(stream, "%s", item.data.str_value);
        break;
    case HEX_TYPE_USER_SYMBOL:
    case HEX_TYPE_NATIVE_SYMBOL:
        fprintf(stream, "%s", item.token->value);
        break;
    case HEX_TYPE_QUOTATION:
        fprintf(stream, "(");
        for (size_t i = 0; i < item.quotation_size; i++)
        {
            if (i > 0)
            {
                fprintf(stream, " ");
            }
            hex_print_item(stream, item.data.quotation_value[i]);
        }
        fprintf(stream, ")");
        break;

    case HEX_TYPE_INVALID:
        fprintf(stream, "<invalid>");
        break;
    default:
        fprintf(stream, "<unknown:%d>", item.type);
        break;
    }
}

void hex_encode_length(uint8_t **bytecode, size_t *size, size_t length)
{
    while (length >= 0x80)
    {
        (*bytecode)[*size] = (length & 0x7F) | 0x80;
        length >>= 7;
        (*size)++;
    }
    (*bytecode)[*size] = length & 0x7F;
    (*size)++;
}

int hex_is_binary(const uint8_t *data, size_t size)
{
    const double binary_threshold = 0.1; // 10% of bytes being non-printable
    size_t non_printable_count = 0;
    for (size_t i = 0; i < size; i++)
    {
        uint8_t byte = data[i];
        // Check if the byte is a printable ASCII character or a common control character.
        if (!((byte >= 32 && byte <= 126) || byte == 9 || byte == 10 || byte == 13))
        {
            non_printable_count++;
        }
        // Early exit if the threshold is exceeded.
        if ((double)non_printable_count / size > binary_threshold)
        {
            return 1;
        }
    }
    return 0;
}

void hex_print_string(FILE *stream, char *value)
{
    fprintf(stream, "\"");
    for (char *c = value; *c != '\0'; c++)
    {
        switch (*c)
        {
        case '\n':
            fprintf(stream, "\\n");
            break;
        case '\t':
            fprintf(stream, "\\t");
            break;
        case '\r':
            fprintf(stream, "\\r");
            break;
        case '\b':
            fprintf(stream, "\\b");
            break;
        case '\f':
            fprintf(stream, "\\f");
            break;
        case '\v':
            fprintf(stream, "\\v");
            break;
        case '\\':
            fprintf(stream, "\\\\");
            break;
        case '\"':
            fprintf(stream, "\\\"");
            break;
        default:
            fputc(*c, stream);
            break;
        }
    }
    fprintf(stream, "\"");
}

void hex_print_item(FILE *stream, hex_item_t *item)
{
    switch (item->type)
    {
    case HEX_TYPE_INTEGER:
        fprintf(stream, "0x%x", item->data.int_value);
        break;

    case HEX_TYPE_STRING:
        hex_print_string(stream, item->data.str_value);
        break;

    case HEX_TYPE_USER_SYMBOL:
    case HEX_TYPE_NATIVE_SYMBOL:
        fprintf(stream, "%s", item->token->value);
        break;

    case HEX_TYPE_QUOTATION:
        fprintf(stream, "(");
        for (size_t i = 0; i < item->quotation_size; i++)
        {
            if (i > 0)
            {
                fprintf(stream, " ");
            }
            hex_print_item(stream, item->data.quotation_value[i]);
        }
        fprintf(stream, ")");
        break;

    case HEX_TYPE_INVALID:
        fprintf(stream, "<invalid>");
        break;
    default:
        fprintf(stream, "<unknown:%d>", item->type);
        break;
    }
}

char *hex_bytes_to_string(const uint8_t *bytes, size_t size)
{
    char *str = (char *)malloc(size * 6 + 1); // Allocate enough space for worst case (\uXXXX format)
    if (!str)
    {
        return NULL; // Allocation failed
    }

    char *ptr = str;
    for (size_t i = 0; i < size; i++)
    {
        uint8_t byte = bytes[i];
        switch (byte)
        {
        case '\n':
            *ptr++ = '\\';
            *ptr++ = 'n';
            break;
        case '\t':
            *ptr++ = '\\';
            *ptr++ = 't';
            break;
        case '\r':
            if (i + 1 < size && bytes[i + 1] == '\n')
            {
                i++; // Skip the '\n' part of the '\r\n' sequence
            }
            *ptr++ = '\\';
            *ptr++ = 'n';
            break;
        case '\b':
            *ptr++ = '\\';
            *ptr++ = 'b';
            break;
        case '\f':
            *ptr++ = '\\';
            *ptr++ = 'f';
            break;
        case '\v':
            *ptr++ = '\\';
            *ptr++ = 'v';
            break;
        case '\\':
            *ptr++ = '\\';
            break;
        case '\"':
            *ptr++ = '\\';
            *ptr++ = '\"';
            break;
        default:
            *ptr++ = byte;
            break;
        }
    }
    *ptr = '\0';
    return str;
}

char *hex_process_string(const char *value)
{
    int len = strlen(value);
    char *processed_str = (char *)malloc(len + 1);
    if (!processed_str)
    {
        return NULL;
    }

    char *dst = processed_str;
    const char *src = value;
    while (*src)
    {
        if (*src == '\\' && *(src + 1))
        {
            src++;
            switch (*src)
            {
            case 'n':
                *dst++ = '\n';
                break;
            case 't':
                *dst++ = '\t';
                break;
            case 'r':
                *dst++ = '\r';
                break;
            case 'b':
                *dst++ = '\b';
                break;
            case 'f':
                *dst++ = '\f';
                break;
            case 'v':
                *dst++ = '\v';
                break;
            case '\"':
                *dst++ = '\"';
                break;
            default:
                *dst++ = '\\';
                *dst++ = *src;
                break;
            }
        }
        else
        {
            *dst++ = *src;
        }
        src++;
    }
    *dst = '\0';
    return processed_str;
}

size_t hex_min_bytes_to_encode_integer(int32_t value)
{
    // If value is negative, we need to return 4 bytes because we must preserve the sign bits.
    if (value < 0)
    {
        return 4;
    }

    // For positive values, check the minimal number of bytes needed.
    for (int bytes = 1; bytes <= 4; bytes++)
    {
        int32_t mask = (1 << (bytes * 8)) - 1;
        int32_t truncated_value = value & mask;

        // If the truncated value is equal to the original, this is the minimal byte size
        if (truncated_value == value)
        {
            return bytes;
        }
    }

    return 4; // Default to 4 bytes if no smaller size is found.
}

char *hex_unescape_string(const char *input)
{
    size_t len = strlen(input);
    char *unescaped = (char *)malloc(len + 1);

    size_t j = 0;
    for (size_t i = 0; i < len; i++)
    {
        if (input[i] == '\\' && i + 1 < len)
        {
            switch (input[i + 1])
            {
            case 'n':
                unescaped[j++] = '\n';
                i++;
                break;
            case 't':
                unescaped[j++] = '\t';
                i++;
                break;
            case 'r':
                unescaped[j++] = '\r';
                i++;
                break;
            case '\\':
                unescaped[j++] = '\\';
                i++;
                break;
            case '\"':
                unescaped[j++] = '\"';
                i++;
                break;
            case '\'':
                unescaped[j++] = '\'';
                i++;
                break;
            case 'v':
                unescaped[j++] = '\v';
                i++;
                break;
            case 'f':
                unescaped[j++] = '\f';
                i++;
                break;
            case 'a':
                unescaped[j++] = '\a';
                i++;
                break;
            case 'b':
                unescaped[j++] = '\b';
                i++;
                break;
            default:
                unescaped[j++] = input[i];
            }
        }
        else
        {
            unescaped[j++] = input[i];
        }
    }
    unescaped[j] = '\0'; // Null-terminate the string
    return unescaped;
}

#ifdef _WIN32
#include <windows.h>
#include <sysinfoapi.h>

void get_unix_timestamp_sec_usec(int32_t result[2])
{
    FILETIME ft;
    GetSystemTimeAsFileTime(&ft);

    ULARGE_INTEGER uli;
    uli.LowPart = ft.dwLowDateTime;
    uli.HighPart = ft.dwHighDateTime;

    const uint64_t EPOCH_DIFFERENCE_100NS = 116444736000000000ULL;
    uint64_t timestamp_100ns = uli.QuadPart - EPOCH_DIFFERENCE_100NS;

    uint64_t total_microseconds = timestamp_100ns / 10;

    result[0] = (int32_t)(total_microseconds / 1000000); // seconds
    result[1] = (int32_t)(total_microseconds % 1000000); // microseconds
}

#else
#include <sys/time.h>

void get_unix_timestamp_sec_usec(int32_t result[2])
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    result[0] = (int32_t)tv.tv_sec;  // seconds
    result[1] = (int32_t)tv.tv_usec; // microseconds
}
#endif

void hex_destroy(hex_context_t *ctx)
{
    if (!ctx)
        return;

    // Clean up stack
    if (ctx->stack)
    {
        // Free all items remaining on the stack
        for (int i = 0; i <= ctx->stack->top; i++)
        {
            if (ctx->stack->entries[i])
            {
                hex_free_item(ctx, ctx->stack->entries[i]);
                ctx->stack->entries[i] = NULL;
            }
        }
        if (ctx->stack->entries)
        {
            free(ctx->stack->entries);
        }
        free(ctx->stack);
    }

    // Clean up registry
    if (ctx->registry)
    {
        hex_registry_destroy(ctx);
    }

    // Clean up stack trace
    if (ctx->stack_trace)
    {
        if (ctx->stack_trace->entries)
        {
            for (int i = 0; i < HEX_STACK_TRACE_SIZE; i++)
            {
                if (ctx->stack_trace->entries[i])
                {
                    hex_free_token(ctx->stack_trace->entries[i]);
                }
            }
            free(ctx->stack_trace->entries);
        }
        free(ctx->stack_trace);
    }

    // Clean up symbol table
    if (ctx->symbol_table)
    {
        if (ctx->symbol_table->symbols)
        {
            for (int i = 0; i < ctx->symbol_table->count && i < HEX_MAX_USER_SYMBOLS; i++)
            {
                if (ctx->symbol_table->symbols[i])
                {
                    free(ctx->symbol_table->symbols[i]);
                }
            }
            free(ctx->symbol_table->symbols);
        }
        free(ctx->symbol_table);
    }

    // Clean up docs
    if (ctx->docs)
    {
        if (ctx->docs->entries)
        {
            free(ctx->docs->entries);
        }
        free(ctx->docs);
    }

    // Clean up settings
    if (ctx->settings)
    {
        free(ctx->settings);
    }

    free(ctx);
}

/* File: src/symbols.c */
#line 1 "src/symbols.c"
#ifndef HEX_H
#include "hex.h"
#endif

////////////////////////////////////////
// Native Symbol Implementations      //
////////////////////////////////////////

// Definition symbols

int hex_symbol_store(hex_context_t *ctx)
{
    HEX_POP(ctx, name);
    ;
    if (name->type == HEX_TYPE_INVALID)
    {
        HEX_FREE(ctx, name);
        return 1;
    }
    HEX_POP(ctx, value);
    ;
    if (value->type == HEX_TYPE_INVALID)
    {
        HEX_FREE(ctx, name);
        HEX_FREE(ctx, value);
        return 1;
    }
    if (name->type != HEX_TYPE_STRING)
    {
        hex_error(ctx, "[symbol :] Symbol name must be a string");
        HEX_FREE(ctx, name);
        HEX_FREE(ctx, value);
        return 1;
    }

    if (value->type == HEX_TYPE_QUOTATION)
    {
        value->is_operator = 0;
    }
    if (hex_set_symbol(ctx, name->data.str_value, value, 0) != 0)
    {
        hex_error(ctx, "[symbol :] Failed to store symbol '%s'", name->data.str_value);
        HEX_FREE(ctx, name);
        HEX_FREE(ctx, value);
        return 1;
    }
    // Success: registry takes ownership of 'value'. Free name only.
    HEX_FREE(ctx, name);
    return 0;
}

int hex_symbol_define(hex_context_t *ctx)
{
    HEX_POP(ctx, name);
    ;
    if (name->type == HEX_TYPE_INVALID)
    {
        HEX_FREE(ctx, name);
        return 1;
    }
    HEX_POP(ctx, value);
    ;
    if (value->type == HEX_TYPE_INVALID)
    {
        HEX_FREE(ctx, name);
        HEX_FREE(ctx, value);
        return 1;
    }

    if (name->type != HEX_TYPE_STRING)
    {
        hex_error(ctx, "[symbol ::] Symbol name must be a string");
        HEX_FREE(ctx, name);
        HEX_FREE(ctx, value);
        return 1;
    }

    if (value->type == HEX_TYPE_QUOTATION)
    {
        value->is_operator = 1;
    }

    if (hex_set_symbol(ctx, name->data.str_value, value, 0) != 0)
    {
        hex_error(ctx, "[symbol ::] Failed to store symbol '%s'", name->data.str_value);
        HEX_FREE(ctx, name);
        HEX_FREE(ctx, value);
        return 1;
    }
    // Success: registry now owns 'value'; free only the name item.
    HEX_FREE(ctx, name);
    return 0;
}

int hex_symbol_free(hex_context_t *ctx)
{
    HEX_POP(ctx, item);
    ;
    if (item->type == HEX_TYPE_INVALID)
    {
        HEX_FREE(ctx, item);
        return 1;
    }
    if (item->type != HEX_TYPE_STRING)
    {
        HEX_FREE(ctx, item);
        hex_error(ctx, "[symbol #] Symbol name must be a string");
        return 1;
    }
    if (hex_valid_native_symbol(ctx, item->data.str_value))
    {
        hex_error(ctx, "[symbol #] Cannot free native symbol '%s'", item->data.str_value);
        HEX_FREE(ctx, item);
        return 1;
    }
    if (hex_delete_symbol(ctx, item->data.str_value) != 0)
    {
        hex_error(ctx, "[symbol #] Symbol not found: %s", item->data.str_value);
        HEX_FREE(ctx, item);
        return 1;
    }
    // Success: free the popped symbol name item
    HEX_FREE(ctx, item);
    return 0;
}

int hex_symbol_symbols(hex_context_t *ctx)
{
    // Allocate memory for the quotation
    hex_item_t **quotation = (hex_item_t **)calloc(ctx->registry->size, sizeof(hex_item_t *));
    if (!quotation)
    {
        hex_error(ctx, "[symbol symbols] Memory allocation failed for quotation");
        return 1;
    }

    size_t quotation_size = 0;

    // Iterate through each bucket in the hash table
    for (size_t i = 0; i < ctx->registry->bucket_count; i++)
    {
        hex_registry_entry_t *entry = ctx->registry->buckets[i];
        while (entry != NULL)
        {
            // Build a new string item using constructor for consistency
            hex_item_t *item = hex_string_item(ctx, entry->key);
            if (!item)
            {
                hex_error(ctx, "[symbol symbols] Failed to allocate string item");
                for (size_t j = 0; j < quotation_size; j++)
                {
                    hex_free_item(ctx, quotation[j]);
                }
                free(quotation);
                return 1;
            }
            quotation[quotation_size++] = item; // ownership transferred to quotation

            // Move to the next entry in the bucket
            entry = entry->next;
        }
    }

    // Push the quotation onto the stack
    if (hex_push_quotation(ctx, quotation, quotation_size) != 0)
    {
        hex_error(ctx, "[symbol symbols] Failed to push quotation onto the stack");
        for (size_t j = 0; j < quotation_size; j++)
        {
            hex_free_item(ctx, quotation[j]);
        }
        free(quotation);
        return 1;
    }

    // Successfully pushed quotation
    return 0;
}

int hex_symbol_type(hex_context_t *ctx)
{
    HEX_POP(ctx, item);
    ;
    if (item->type == HEX_TYPE_INVALID)
    {
        HEX_FREE(ctx, item);
        return 1;
    }
    int result = hex_push_string(ctx, hex_type(item->type));
    if (result == 0 && ctx->settings && ctx->settings->debugging_enabled)
    {
        hex_debug_validate_stack(ctx);
    }
    HEX_FREE(ctx, item);
    return result;
}

// Evaluation symbols

int hex_symbol_i(hex_context_t *ctx)
{
    HEX_POP(ctx, item);
    ;
    if (item->type == HEX_TYPE_INVALID)
    {
        HEX_FREE(ctx, item);
        return 1;
    }
    if (item->type != HEX_TYPE_QUOTATION)
    {
        hex_error(ctx, "[symbol .] Quotation required", hex_type(item->type));
        HEX_FREE(ctx, item);
        return 1;
    }
    for (size_t i = 0; i < item->quotation_size; i++)
    {
        hex_item_t *copy = hex_copy_item(ctx, item->data.quotation_value[i]);
        if (!copy || hex_push(ctx, copy) != 0)
        {
            if (copy)
            {
                hex_free_item(ctx, copy);
            }
            HEX_FREE(ctx, item);
            return 1;
        }
        if (ctx->settings && ctx->settings->debugging_enabled)
        {
            hex_debug_validate_stack(ctx);
        }
    }
    HEX_FREE(ctx, item); // free original quotation (no aliasing remains)
    return 0;
}

// evaluate a string or bytecode array
int hex_symbol_eval(hex_context_t *ctx)
{
    HEX_POP(ctx, file);
    HEX_POP(ctx, item);
    if (file->type == HEX_TYPE_INVALID)
    {
        HEX_FREE(ctx, file);
        return 1;
    }
    if (file->type != HEX_TYPE_STRING)
    {
        hex_error(ctx, "[symbol !] File name or scope identifier required");
        HEX_FREE(ctx, file);
        return 1;
    }
    if (item->type == HEX_TYPE_INVALID)
    {
        HEX_FREE(ctx, item);
        HEX_FREE(ctx, file);
        return 1;
    }
    if (item->type == HEX_TYPE_STRING)
    {
        int result = hex_interpret(ctx, item->data.str_value, file->data.str_value, 1, 1);
        HEX_FREE(ctx, item);
        HEX_FREE(ctx, file);
        return result;
    }
    else if (item->type == HEX_TYPE_QUOTATION)
    {
        for (size_t i = 0; i < item->quotation_size; i++)
        {
            if (item->data.quotation_value[i]->type != HEX_TYPE_INTEGER)
            {
                hex_error(ctx, "[symbol !] Quotation must contain only integers");
                HEX_FREE(ctx, item);
                HEX_FREE(ctx, file);
                return 1;
            }
        }
        uint8_t *bytecode = (uint8_t *)malloc(item->quotation_size * sizeof(uint8_t));
        if (!bytecode)
        {
            hex_error(ctx, "[symbol !] Memory allocation failed");
            HEX_FREE(ctx, item);
            HEX_FREE(ctx, file);
            return 1;
        }
        for (size_t i = 0; i < item->quotation_size; i++)
        {
            bytecode[i] = (uint8_t)item->data.quotation_value[i]->data.int_value;
        }
        // Sandbox: save current table pointer; create a working copy to mutate
        hex_symbol_table_t *original_table = ctx->symbol_table;
        hex_symbol_table_t *working_copy = hex_symboltable_copy(ctx);
        if (!working_copy)
        {
            free(bytecode);
            HEX_FREE(ctx, item);
            HEX_FREE(ctx, file);
            return 1;
        }
        ctx->symbol_table = working_copy;
        int result = hex_interpret_bytecode(ctx, bytecode, item->quotation_size, file->data.str_value);
        // Destroy mutated working copy and restore original
        hex_symboltable_destroy(ctx->symbol_table);
        ctx->symbol_table = original_table;
        free(bytecode);
        HEX_FREE(ctx, item);
        HEX_FREE(ctx, file);
        return result;
    }
    else
    {
        hex_error(ctx, "[symbol !] String or a quotation of integers required");
        HEX_FREE(ctx, item);
        HEX_FREE(ctx, file);
        return 1;
    }
}

int hex_symbol_debug(hex_context_t *ctx)
{
    HEX_POP(ctx, item);
    if (item->type == HEX_TYPE_INVALID)
    {
        HEX_FREE(ctx, item);
        return 1;
    }
    if (item->type != HEX_TYPE_QUOTATION)
    {
        hex_error(ctx, "[symbol debug] Quotation required", hex_type(item->type));
        HEX_FREE(ctx, item);
        return 1;
    }
    ctx->settings->debugging_enabled = 1;
    for (size_t i = 0; i < item->quotation_size; i++)
    {
        hex_item_t *copy = hex_copy_item(ctx, item->data.quotation_value[i]);
        if (!copy || hex_push(ctx, copy) != 0)
        {
            if (copy)
            {
                hex_free_item(ctx, copy);
            }
            HEX_FREE(ctx, item);
            ctx->settings->debugging_enabled = 0;
            return 1;
        }
    }
    HEX_FREE(ctx, item);
    ctx->settings->debugging_enabled = 0;
    return 0;
}

// IO Symbols

int hex_symbol_puts(hex_context_t *ctx)
{
    HEX_POP(ctx, item);
    ;
    if (item->type == HEX_TYPE_INVALID)
    {
        HEX_FREE(ctx, item);
        return 1;
    }
    hex_raw_print_item(stdout, *item);
    printf("\n");
    HEX_FREE(ctx, item);
    return 0;
}

int hex_symbol_warn(hex_context_t *ctx)
{
    HEX_POP(ctx, item);
    ;
    if (item->type == HEX_TYPE_INVALID)
    {
        HEX_FREE(ctx, item);
        return 1;
    }
    hex_raw_print_item(stderr, *item);
    printf("\n");
    HEX_FREE(ctx, item);
    return 0;
}

int hex_symbol_print(hex_context_t *ctx)
{
    HEX_POP(ctx, item);
    ;
    if (item->type == HEX_TYPE_INVALID)
    {
        HEX_FREE(ctx, item);
        return 1;
    }
    hex_raw_print_item(stdout, *item);
    fflush(stdout);
    HEX_FREE(ctx, item);
    return 0;
}

int hex_symbol_gets(hex_context_t *ctx)
{
    char input[HEX_STDIN_BUFFER_SIZE]; // Buffer to store the input (adjust size if needed)
    char *p = input;
#if defined(__EMSCRIPTEN__)
    p = em_fgets(input, 1024);
#else
    p = fgets(input, sizeof(input), stdin);
#endif

    if (p != NULL)
    {
        // Strip the newline character at the end of the string
        input[strcspn(input, "\n")] = '\0';

        // Push the input string onto the stack
        return hex_push_string(ctx, input);
    }
    else
    {
        hex_error(ctx, "[symbol gets] Failed to read input");
        return 1;
    }
}

// Mathematical symbols
int hex_symbol_add(hex_context_t *ctx)
{
    HEX_POP(ctx, b);
    ;
    if (b->type == HEX_TYPE_INVALID)
    {
        HEX_FREE(ctx, b);
        return 1;
    }
    HEX_POP(ctx, a);
    ;
    if (a->type == HEX_TYPE_INVALID)
    {
        HEX_FREE(ctx, a);
        HEX_FREE(ctx, b);
        return 1;
    }
    if (a->type == HEX_TYPE_INTEGER && b->type == HEX_TYPE_INTEGER)
    {
        int result = hex_push_integer(ctx, a->data.int_value + b->data.int_value);
        HEX_FREE(ctx, a);
        HEX_FREE(ctx, b);
        return result;
    }
    hex_error(ctx, "[symbol +] Two integers required");
    HEX_FREE(ctx, a);
    HEX_FREE(ctx, b);
    return 1;
}

int hex_symbol_subtract(hex_context_t *ctx)
{
    HEX_POP(ctx, b);
    ;
    if (b->type == HEX_TYPE_INVALID)
    {
        HEX_FREE(ctx, b);
        return 1;
    }
    HEX_POP(ctx, a);
    ;
    if (a->type == HEX_TYPE_INVALID)
    {
        HEX_FREE(ctx, a);
        HEX_FREE(ctx, b);
        return 1;
    }
    if (a->type == HEX_TYPE_INTEGER && b->type == HEX_TYPE_INTEGER)
    {
        int result = hex_push_integer(ctx, a->data.int_value - b->data.int_value);
        HEX_FREE(ctx, a);
        HEX_FREE(ctx, b);
        return result;
    }
    hex_error(ctx, "[symbol -] Two integers required");
    HEX_FREE(ctx, a);
    HEX_FREE(ctx, b);
    return 1;
}

int hex_symbol_multiply(hex_context_t *ctx)
{
    HEX_POP(ctx, b);
    ;
    if (b->type == HEX_TYPE_INVALID)
    {
        HEX_FREE(ctx, b);
        return 1;
    }
    HEX_POP(ctx, a);
    ;
    if (a->type == HEX_TYPE_INVALID)
    {
        HEX_FREE(ctx, a);
        HEX_FREE(ctx, b);
        return 1;
    }
    if (a->type == HEX_TYPE_INTEGER && b->type == HEX_TYPE_INTEGER)
    {
        int result = hex_push_integer(ctx, a->data.int_value * b->data.int_value);
        HEX_FREE(ctx, a);
        HEX_FREE(ctx, b);
        return result;
    }
    hex_error(ctx, "[symbol *] Two integers required");
    HEX_FREE(ctx, a);
    HEX_FREE(ctx, b);
    return 1;
}

int hex_symbol_divide(hex_context_t *ctx)
{
    HEX_POP(ctx, b);
    ;
    if (b->type == HEX_TYPE_INVALID)
    {
        HEX_FREE(ctx, b);
        return 1;
    }
    HEX_POP(ctx, a);
    ;
    if (a->type == HEX_TYPE_INVALID)
    {
        HEX_FREE(ctx, a);
        HEX_FREE(ctx, b);
        return 1;
    }
    if (a->type == HEX_TYPE_INTEGER && b->type == HEX_TYPE_INTEGER)
    {
        if (b->data.int_value == 0)
        {
            hex_error(ctx, "[symbol /] Division by zero");
            HEX_FREE(ctx, a);
            HEX_FREE(ctx, b);
            return 1;
        }
        int result = hex_push_integer(ctx, a->data.int_value / b->data.int_value);
        HEX_FREE(ctx, a);
        HEX_FREE(ctx, b);
        return result;
    }
    hex_error(ctx, "[symbol /] Two integers required");
    HEX_FREE(ctx, a);
    HEX_FREE(ctx, b);
    return 1;
}

int hex_symbol_modulo(hex_context_t *ctx)
{
    HEX_POP(ctx, b);
    ;
    if (b->type == HEX_TYPE_INVALID)
    {
        HEX_FREE(ctx, b);
        return 1;
    }
    HEX_POP(ctx, a);
    ;
    if (a->type == HEX_TYPE_INVALID)
    {
        HEX_FREE(ctx, a);
        HEX_FREE(ctx, b);
        return 1;
    }
    if (a->type == HEX_TYPE_INTEGER && b->type == HEX_TYPE_INTEGER)
    {
        if (b->data.int_value == 0)
        {
            hex_error(ctx, "[symbol %%] Division by zero");
            HEX_FREE(ctx, a);
            HEX_FREE(ctx, b);
            return 1;
        }
        int result = hex_push_integer(ctx, a->data.int_value % b->data.int_value);
        HEX_FREE(ctx, a);
        HEX_FREE(ctx, b);
        return result;
    }
    hex_error(ctx, "[symbol %%] Two integers required");
    HEX_FREE(ctx, a);
    HEX_FREE(ctx, b);
    return 1;
}

// Bit symbols

int hex_symbol_bitand(hex_context_t *ctx)
{
    HEX_POP(ctx, right);
    ;
    if (right->type == HEX_TYPE_INVALID)
    {
        HEX_FREE(ctx, right);
        return 1;
    }
    HEX_POP(ctx, left);
    ;
    if (left->type == HEX_TYPE_INVALID)
    {
        HEX_FREE(ctx, left);
        HEX_FREE(ctx, right);
        return 1;
    }
    if (left->type == HEX_TYPE_INTEGER && right->type == HEX_TYPE_INTEGER)
    {
        int result = hex_push_integer(ctx, left->data.int_value & right->data.int_value);
        HEX_FREE(ctx, left);
        HEX_FREE(ctx, right);
        return result;
    }
    hex_error(ctx, "[symbol &] Two integers required");
    HEX_FREE(ctx, left);
    HEX_FREE(ctx, right);
    return 1;
}

int hex_symbol_bitor(hex_context_t *ctx)
{
    HEX_POP(ctx, right);
    ;
    if (right->type == HEX_TYPE_INVALID)
    {
        HEX_FREE(ctx, right);
        return 1;
    }
    HEX_POP(ctx, left);
    ;
    if (left->type == HEX_TYPE_INVALID)
    {
        HEX_FREE(ctx, left);
        HEX_FREE(ctx, right);
        return 1;
    }
    if (left->type == HEX_TYPE_INTEGER && right->type == HEX_TYPE_INTEGER)
    {
        int result = hex_push_integer(ctx, left->data.int_value | right->data.int_value);
        HEX_FREE(ctx, left);
        HEX_FREE(ctx, right);
        return result;
    }
    hex_error(ctx, "[symbol |] Two integers required");
    HEX_FREE(ctx, left);
    HEX_FREE(ctx, right);
    return 1;
}

int hex_symbol_bitxor(hex_context_t *ctx)
{
    HEX_POP(ctx, right);
    if (right->type == HEX_TYPE_INVALID)
    {
        HEX_FREE(ctx, right);
        return 1;
    }
    HEX_POP(ctx, left);
    if (left->type == HEX_TYPE_INVALID)
    {
        HEX_FREE(ctx, left);
        HEX_FREE(ctx, right);
        return 1;
    }
    if (left->type == HEX_TYPE_INTEGER && right->type == HEX_TYPE_INTEGER)
    {
        int result = hex_push_integer(ctx, left->data.int_value ^ right->data.int_value);
        HEX_FREE(ctx, left);
        HEX_FREE(ctx, right);
        return result;
    }
    hex_error(ctx, "[symbol ^] Two integers required");
    HEX_FREE(ctx, left);
    HEX_FREE(ctx, right);
    return 1;
}

int hex_symbol_shiftleft(hex_context_t *ctx)
{
    HEX_POP(ctx, right);
    ;
    if (right->type == HEX_TYPE_INVALID)
    {
        HEX_FREE(ctx, right);
        return 1;
    }
    HEX_POP(ctx, left);
    ;
    if (left->type == HEX_TYPE_INVALID)
    {
        HEX_FREE(ctx, left);
        HEX_FREE(ctx, right);
        return 1;
    }
    if (left->type == HEX_TYPE_INTEGER && right->type == HEX_TYPE_INTEGER)
    {
        int result = hex_push_integer(ctx, left->data.int_value << right->data.int_value);
        HEX_FREE(ctx, left);
        HEX_FREE(ctx, right);
        return result;
    }
    hex_error(ctx, "[symbol <<] Two integers required");
    HEX_FREE(ctx, left);
    HEX_FREE(ctx, right);
    return 1;
}

int hex_symbol_shiftright(hex_context_t *ctx)
{
    HEX_POP(ctx, right);
    ;
    if (right->type == HEX_TYPE_INVALID)
    {
        HEX_FREE(ctx, right);
        return 1;
    }
    HEX_POP(ctx, left);
    ;
    if (left->type == HEX_TYPE_INVALID)
    {
        HEX_FREE(ctx, left);
        HEX_FREE(ctx, right);
        return 1;
    }
    if (left->type == HEX_TYPE_INTEGER && right->type == HEX_TYPE_INTEGER)
    {
        int result = hex_push_integer(ctx, left->data.int_value >> right->data.int_value);
        HEX_FREE(ctx, left);
        HEX_FREE(ctx, right);
        return result;
    }
    hex_error(ctx, "[symbol >>] Two integers required");
    HEX_FREE(ctx, left);
    HEX_FREE(ctx, right);
    return 1;
}

int hex_symbol_bitnot(hex_context_t *ctx)
{
    HEX_POP(ctx, item);
    ;
    if (item->type == HEX_TYPE_INVALID)
    {
        HEX_FREE(ctx, item);
        return 1;
    }
    if (item->type == HEX_TYPE_INTEGER)
    {
        int result = hex_push_integer(ctx, ~item->data.int_value);
        HEX_FREE(ctx, item);
        return result;
    }
    hex_error(ctx, "[symbol ~] Integer required");
    HEX_FREE(ctx, item);
    return 1;
}

// Conversion symbols

int hex_symbol_int(hex_context_t *ctx)
{
    HEX_POP(ctx, a);
    ;
    if (a->type == HEX_TYPE_INVALID)
    {
        HEX_FREE(ctx, a);
        return 1;
    }
    if (a->type == HEX_TYPE_STRING)
    {
        int result = hex_push_integer(ctx, strtol(a->data.str_value, NULL, 16));
        HEX_FREE(ctx, a);
        return result;
    }
    hex_error(ctx, "[symbol int] String representing a hexadecimal integer required");
    HEX_FREE(ctx, a);
    return 1;
}

int hex_symbol_str(hex_context_t *ctx)
{
    HEX_POP(ctx, a);
    ;
    if (a->type == HEX_TYPE_INVALID)
    {
        HEX_FREE(ctx, a);
        return 1;
    }
    if (a->type == HEX_TYPE_INTEGER)
    {
        int result = hex_push_string(ctx, hex_itoa_hex(a->data.int_value));
        HEX_FREE(ctx, a);
        return result;
    }
    hex_error(ctx, "[symbol str] Integer required");
    HEX_FREE(ctx, a);
    return 1;
}

int hex_symbol_dec(hex_context_t *ctx)
{
    HEX_POP(ctx, a);
    ;
    if (a->type == HEX_TYPE_INVALID)
    {
        HEX_FREE(ctx, a);
        return 1;
    }
    if (a->type == HEX_TYPE_INTEGER)
    {
        char buffer[32];
        snprintf(buffer, sizeof(buffer), "%d", a->data.int_value);
        int result = hex_push_string(ctx, buffer);
        HEX_FREE(ctx, a);
        return result;
    }
    hex_error(ctx, "[symbol dec] Integer required");
    HEX_FREE(ctx, a);
    return 1;
}

int hex_symbol_hex(hex_context_t *ctx)
{
    HEX_POP(ctx, item);
    ;
    if (item->type == HEX_TYPE_INVALID)
    {
        HEX_FREE(ctx, item);
        return 1;
    }
    if (item->type == HEX_TYPE_STRING)
    {
        int result = hex_push_integer(ctx, strtol(item->data.str_value, NULL, 10));
        HEX_FREE(ctx, item);
        return result;
    }
    hex_error(ctx, "[symbol hex] String representing a decimal integer required");
    HEX_FREE(ctx, item);
    return 1;
}

int hex_symbol_ord(hex_context_t *ctx)
{
    HEX_POP(ctx, item);
    ;
    if (item->type == HEX_TYPE_INVALID)
    {
        HEX_FREE(ctx, item);
        return 1;
    }
    if (item->type == HEX_TYPE_STRING)
    {
        if (strlen(item->data.str_value) > 1)
        {
            int result = hex_push_integer(ctx, -1);
            HEX_FREE(ctx, item);
            return result;
        }
        unsigned char *str = (unsigned char *)item->data.str_value;
        int result;
        if (str[0] < 128)
        {
            result = hex_push_integer(ctx, str[0]);
        }
        else
        {
            result = hex_push_integer(ctx, -1);
        }
        HEX_FREE(ctx, item);
        return result;
    }
    hex_error(ctx, "[symbol ord] String required");
    HEX_FREE(ctx, item);
    return 1;
}

int hex_symbol_chr(hex_context_t *ctx)
{
    HEX_POP(ctx, item);
    ;
    if (item->type == HEX_TYPE_INVALID)
    {
        HEX_FREE(ctx, item);
        return 1;
    }
    if (item->type == HEX_TYPE_INTEGER)
    {
        int result;
        if (item->data.int_value >= 0 && item->data.int_value < 128)
        {
            char str[2] = {(char)item->data.int_value, '\0'};
            result = hex_push_string(ctx, str);
        }
        else
        {
            result = hex_push_string(ctx, "");
        }
        HEX_FREE(ctx, item);
        return result;
    }
    hex_error(ctx, "[symbol chr] Integer required");
    HEX_FREE(ctx, item);
    return 1;
}

// Comparison symbols

static int hex_equal(hex_item_t *a, hex_item_t *b)
{
    if (a->type == HEX_TYPE_INVALID || b->type == HEX_TYPE_INVALID)
    {
        return 0;
    }
    if (a->type == HEX_TYPE_NATIVE_SYMBOL || a->type == HEX_TYPE_USER_SYMBOL)
    {
        return (strcmp(a->token->value, b->token->value) == 0);
    }
    if (a->type != b->type)
    {
        return 0;
    }
    if (a->type == HEX_TYPE_INTEGER)
    {
        return a->data.int_value == b->data.int_value;
    }
    if (a->type == HEX_TYPE_STRING)
    {
        return (strcmp(a->data.str_value, b->data.str_value) == 0);
    }
    if (a->type == HEX_TYPE_QUOTATION)
    {
        if (a->quotation_size != b->quotation_size)
        {
            return 0;
        }
        else
        {
            for (size_t i = 0; i < a->quotation_size; i++)
            {
                if (!hex_equal(a->data.quotation_value[i], b->data.quotation_value[i]))
                {
                    return 0;
                }
            }
            return 1;
        }
    }
    return 0;
}

static int hex_is_type_symbol(hex_item_t *item)
{
    if (item->type == HEX_TYPE_USER_SYMBOL || item->type == HEX_TYPE_NATIVE_SYMBOL)
    {
        return 1;
    }
    return 0;
}

static int hex_greater(hex_context_t *ctx, hex_item_t *a, hex_item_t *b, char *symbol)
{
    if (a->type == HEX_TYPE_INTEGER && b->type == HEX_TYPE_INTEGER)
    {
        return a->data.int_value > b->data.int_value;
    }
    else if (a->type == HEX_TYPE_STRING && b->type == HEX_TYPE_STRING)
    {
        return strcmp(a->data.str_value, b->data.str_value) > 0;
    }
    else if (a->type == HEX_TYPE_QUOTATION && b->type == HEX_TYPE_QUOTATION)
    {
        // Compare quotations lexicographically
        size_t min_size = a->quotation_size < b->quotation_size ? a->quotation_size : b->quotation_size;
        int is_greater = 0;

        for (size_t i = 0; i < min_size; i++)
        {
            hex_item_t *it_a = a->data.quotation_value[i];
            hex_item_t *it_b = b->data.quotation_value[i];

            // Perform element-wise comparison
            if (it_a->type != it_b->type && !(hex_is_type_symbol(it_a) && hex_is_type_symbol(it_b)))
            {
                // Mismatched types, return false
                return 0;
            }

            if (it_a->type == HEX_TYPE_INTEGER)
            {
                if (it_a->data.int_value != it_b->data.int_value)
                {
                    is_greater = it_a->data.int_value > it_b->data.int_value;
                    break;
                }
            }
            else if (it_a->type == HEX_TYPE_STRING)
            {
                int cmp = strcmp(it_a->data.str_value, it_b->data.str_value);
                if (cmp != 0)
                {
                    is_greater = cmp > 0;
                    break;
                }
            }
            else if (hex_is_type_symbol(it_a))
            {
                int cmp = strcmp(it_a->token->value, it_b->token->value);
                if (cmp != 0)
                {
                    is_greater = cmp > 0;
                    break;
                }
            }
            else
            {
                // Mismatched types, return false
                return 0;
            }
        }

        if (!is_greater)
        {
            // If all compared elements are equal, compare sizes
            return a->quotation_size > b->quotation_size;
        }
        return is_greater;
    }
    else
    {
        hex_error(ctx, "[symbol %s] Two integers, two strings, or two quotations required", symbol);
        return -1;
    }
}

int hex_symbol_equal(hex_context_t *ctx)
{
    HEX_POP(ctx, b);
    if (b->type == HEX_TYPE_INVALID)
    {
        HEX_FREE(ctx, b);
        return 1;
    }
    HEX_POP(ctx, a);
    if (a->type == HEX_TYPE_INVALID)
    {
        HEX_FREE(ctx, a);
        HEX_FREE(ctx, b);
        return 1;
    }
    if ((a->type == HEX_TYPE_INTEGER && b->type == HEX_TYPE_INTEGER) || (a->type == HEX_TYPE_STRING && b->type == HEX_TYPE_STRING) || (a->type == HEX_TYPE_QUOTATION && b->type == HEX_TYPE_QUOTATION))
    {
        int result = hex_push_integer(ctx, hex_equal(a, b));
        if (result != 0)
        {
            HEX_FREE(ctx, a);
            HEX_FREE(ctx, b);
        }
        return result;
    }
    // Different types => false
    int result = hex_push_integer(ctx, 0);
    HEX_FREE(ctx, a);
    HEX_FREE(ctx, b);
    return result;
}

int hex_symbol_notequal(hex_context_t *ctx)
{
    HEX_POP(ctx, b);
    if (b->type == HEX_TYPE_INVALID)
    {
        HEX_FREE(ctx, b);
        return 1;
    }
    HEX_POP(ctx, a);
    if (a->type == HEX_TYPE_INVALID)
    {
        HEX_FREE(ctx, a);
        HEX_FREE(ctx, b);
        return 1;
    }
    if ((a->type == HEX_TYPE_INTEGER && b->type == HEX_TYPE_INTEGER) || (a->type == HEX_TYPE_STRING && b->type == HEX_TYPE_STRING) || (a->type == HEX_TYPE_QUOTATION && b->type == HEX_TYPE_QUOTATION))
    {
        int result = hex_push_integer(ctx, !hex_equal(a, b));
        if (result != 0)
        {
            HEX_FREE(ctx, a);
            HEX_FREE(ctx, b);
        }
        return result;
    }
    // Different types => true
    int result = hex_push_integer(ctx, 1);
    if (result != 0)
    {
        HEX_FREE(ctx, a);
        HEX_FREE(ctx, b);
    }
    return result;
}

int hex_symbol_greater(hex_context_t *ctx)
{
    HEX_POP(ctx, b);
    if (b->type == HEX_TYPE_INVALID)
    {
        HEX_FREE(ctx, b);
        return 1;
    }
    HEX_POP(ctx, a);
    if (a->type == HEX_TYPE_INVALID)
    {
        HEX_FREE(ctx, a);
        HEX_FREE(ctx, b);
        return 1;
    }
    hex_item_t *pa = a;
    hex_item_t *pb = b;
    int result = hex_push_integer(ctx, hex_greater(ctx, pa, pb, ">"));
    if (result != 0)
    {
        HEX_FREE(ctx, a);
        HEX_FREE(ctx, b);
    }
    return result;
}

int hex_symbol_less(hex_context_t *ctx)
{
    HEX_POP(ctx, b);
    if (b->type == HEX_TYPE_INVALID)
    {
        HEX_FREE(ctx, b);
        return 1;
    }
    HEX_POP(ctx, a);
    if (a->type == HEX_TYPE_INVALID)
    {
        HEX_FREE(ctx, a);
        HEX_FREE(ctx, b);
        return 1;
    }
    hex_item_t *pa = a;
    hex_item_t *pb = b;
    int result = hex_push_integer(ctx, hex_greater(ctx, pb, pa, "<"));
    if (result != 0)
    {
        HEX_FREE(ctx, a);
        HEX_FREE(ctx, b);
    }
    return result;
}

int hex_symbol_greaterequal(hex_context_t *ctx)
{
    HEX_POP(ctx, b);
    if (b->type == HEX_TYPE_INVALID)
    {
        HEX_FREE(ctx, b);
        return 1;
    }
    HEX_POP(ctx, a);
    if (a->type == HEX_TYPE_INVALID)
    {
        HEX_FREE(ctx, a);
        HEX_FREE(ctx, b);
        return 1;
    }
    hex_item_t *pa = a;
    hex_item_t *pb = b;
    int result = hex_push_integer(ctx, hex_greater(ctx, pa, pb, ">") || hex_equal(a, b));
    if (result != 0)
    {
        HEX_FREE(ctx, a);
        HEX_FREE(ctx, b);
    }
    return result;
}

int hex_symbol_lessequal(hex_context_t *ctx)
{
    HEX_POP(ctx, b);
    if (b->type == HEX_TYPE_INVALID)
    {
        HEX_FREE(ctx, b);
        return 1;
    }
    HEX_POP(ctx, a);
    if (a->type == HEX_TYPE_INVALID)
    {
        HEX_FREE(ctx, a);
        HEX_FREE(ctx, b);
        return 1;
    }
    hex_item_t *pa = a;
    hex_item_t *pb = b;
    int result = hex_push_integer(ctx, hex_greater(ctx, pb, pa, "<") || hex_equal(a, b));
    if (result != 0)
    {
        HEX_FREE(ctx, a);
        HEX_FREE(ctx, b);
    }
    return result;
}

// Boolean symbols

int hex_symbol_and(hex_context_t *ctx)
{
    HEX_POP(ctx, b);
    if (b->type == HEX_TYPE_INVALID)
    {
        HEX_FREE(ctx, b);
        return 1;
    }
    HEX_POP(ctx, a);
    if (a->type == HEX_TYPE_INVALID)
    {
        HEX_FREE(ctx, a);
        HEX_FREE(ctx, b);
        return 1;
    }
    if (a->type == HEX_TYPE_INTEGER && b->type == HEX_TYPE_INTEGER)
    {
        int result = hex_push_integer(ctx, a->data.int_value && b->data.int_value);
        if (result != 0)
        {
            HEX_FREE(ctx, a);
            HEX_FREE(ctx, b);
        }
        return result;
    }
    hex_error(ctx, "[symbol and] Two integers required");
    HEX_FREE(ctx, a);
    HEX_FREE(ctx, b);
    return 1;
}

int hex_symbol_or(hex_context_t *ctx)
{
    HEX_POP(ctx, b);
    if (b->type == HEX_TYPE_INVALID)
    {
        HEX_FREE(ctx, b);
        return 1;
    }
    HEX_POP(ctx, a);
    if (a->type == HEX_TYPE_INVALID)
    {
        HEX_FREE(ctx, a);
        HEX_FREE(ctx, b);
        return 1;
    }
    if (a->type == HEX_TYPE_INTEGER && b->type == HEX_TYPE_INTEGER)
    {
        int result = hex_push_integer(ctx, a->data.int_value || b->data.int_value);
        if (result != 0)
        {
            HEX_FREE(ctx, a);
            HEX_FREE(ctx, b);
        }
        return result;
    }
    hex_error(ctx, "[symbol or] Two integers required");
    HEX_FREE(ctx, a);
    HEX_FREE(ctx, b);
    return 1;
}

int hex_symbol_not(hex_context_t *ctx)
{
    HEX_POP(ctx, a);
    if (a->type == HEX_TYPE_INVALID)
    {
        HEX_FREE(ctx, a);
        return 1;
    }
    if (a->type == HEX_TYPE_INTEGER)
    {
        int result = hex_push_integer(ctx, !a->data.int_value);
        if (result != 0)
        {
            HEX_FREE(ctx, a);
        }
        return result;
    }
    hex_error(ctx, "[symbol not] Integer required");
    HEX_FREE(ctx, a);
    return 1;
}

int hex_symbol_xor(hex_context_t *ctx)
{
    HEX_POP(ctx, b);
    ;
    if (b->type == HEX_TYPE_INVALID)
    {
        HEX_FREE(ctx, b);
        return 1;
    }
    HEX_POP(ctx, a);
    ;
    if (a->type == HEX_TYPE_INVALID)
    {
        HEX_FREE(ctx, a);
        HEX_FREE(ctx, b);
        return 1;
    }
    if (a->type == HEX_TYPE_INTEGER && b->type == HEX_TYPE_INTEGER)
    {
        int result = hex_push_integer(ctx, a->data.int_value ^ b->data.int_value);
        if (result != 0)
        {
            HEX_FREE(ctx, a);
            HEX_FREE(ctx, b);
        }
        return result;
    }
    hex_error(ctx, "[symbol xor] Two integers required");
    HEX_FREE(ctx, a);
    HEX_FREE(ctx, b);
    return 1;
}

// Quotation and String (List) Symbols
int hex_symbol_cat(hex_context_t *ctx)
{
    HEX_POP(ctx, value);
    if (value->type == HEX_TYPE_INVALID)
    {
        HEX_FREE(ctx, value);
        return 1; // Failed to pop value
    }
    HEX_POP(ctx, list);
    ;
    if (list->type == HEX_TYPE_INVALID)
    {
        HEX_FREE(ctx, list);
        HEX_FREE(ctx, value);
        return 1; // Failed to pop list
    }
    // Strategy: produce a fresh result (deep copies) and consume both inputs.
    // This avoids aliasing between the original quotations and the new one.
    if (list->type == HEX_TYPE_QUOTATION && value->type == HEX_TYPE_QUOTATION)
    {
        size_t new_size = list->quotation_size + value->quotation_size;
        hex_item_t **items = (hex_item_t **)calloc(new_size, sizeof(hex_item_t *));
        if (!items)
        {
            hex_error(ctx, "[symbol cat] Memory allocation failed");
            HEX_FREE(ctx, list);
            HEX_FREE(ctx, value);
            return 1;
        }
        size_t k = 0;
        for (size_t i = 0; i < list->quotation_size; i++)
        {
            items[k] = hex_copy_item(ctx, list->data.quotation_value[i]);
            if (!items[k])
            {
                hex_error(ctx, "[symbol cat] Failed to copy element");
                hex_free_list(ctx, items, k);
                HEX_FREE(ctx, list);
                HEX_FREE(ctx, value);
                return 1;
            }
            k++;
        }
        for (size_t i = 0; i < value->quotation_size; i++)
        {
            items[k] = hex_copy_item(ctx, value->data.quotation_value[i]);
            if (!items[k])
            {
                hex_error(ctx, "[symbol cat] Failed to copy element");
                hex_free_list(ctx, items, k);
                HEX_FREE(ctx, list);
                HEX_FREE(ctx, value);
                return 1;
            }
            k++;
        }
        if (hex_push_quotation(ctx, items, new_size) != 0)
        {
            hex_free_list(ctx, items, new_size);
            HEX_FREE(ctx, list);
            HEX_FREE(ctx, value);
            return 1;
        }
        // Ownership of items array & its elements transferred; DO NOT free(items)
        HEX_FREE(ctx, list);
        HEX_FREE(ctx, value);
        return 0;
    }
    else if (list->type == HEX_TYPE_STRING && value->type == HEX_TYPE_STRING)
    {
        size_t new_len = strlen(list->data.str_value) + strlen(value->data.str_value) + 1;
        char *buf = (char *)malloc(new_len);
        if (!buf)
        {
            hex_error(ctx, "[symbol cat] Memory allocation failed");
            HEX_FREE(ctx, list);
            HEX_FREE(ctx, value);
            return 1;
        }
        strcpy(buf, list->data.str_value);
        strcat(buf, value->data.str_value);
        if (hex_push_string(ctx, buf) != 0)
        {
            free(buf);
            HEX_FREE(ctx, list);
            HEX_FREE(ctx, value);
            return 1;
        }
        free(buf); // temporary buffer
        HEX_FREE(ctx, list);
        HEX_FREE(ctx, value);
        return 0;
    }
    else
    {
        hex_error(ctx, "[symbol cat] Two quotations or two strings required");
        HEX_FREE(ctx, list);
        HEX_FREE(ctx, value);
        return 1;
    }
}

int hex_symbol_len(hex_context_t *ctx)
{
    HEX_POP(ctx, item);
    ;
    if (item->type == HEX_TYPE_INVALID)
    {
        HEX_FREE(ctx, item);
        return 1;
    }
    int result = 0;
    if (item->type == HEX_TYPE_QUOTATION)
    {
        result = hex_push_integer(ctx, item->quotation_size);
    }
    else if (item->type == HEX_TYPE_STRING)
    {
        result = hex_push_integer(ctx, strlen(item->data.str_value));
    }
    else
    {
        hex_error(ctx, "[symbol len] Quotation or string required");
        result = 1;
    }
    if (result != 0)
    {
        HEX_FREE(ctx, item);
    }
    return result;
}

int hex_symbol_get(hex_context_t *ctx)
{
    HEX_POP(ctx, index);
    if (index->type == HEX_TYPE_INVALID)
    {
        HEX_FREE(ctx, index);
        return 1;
    }
    HEX_POP(ctx, list);
    if (list->type == HEX_TYPE_INVALID)
    {
        HEX_FREE(ctx, list);
        HEX_FREE(ctx, index);
        return 1;
    }
    int result = 0;
    hex_item_t *copy = calloc(1, sizeof(hex_item_t));
    if (list->type == HEX_TYPE_QUOTATION)
    {
        if (index->type != HEX_TYPE_INTEGER)
        {
            hex_error(ctx, "[symbol get] Index must be an integer");
            result = 1;
        }
        else if (index->data.int_value < 0 || (size_t)index->data.int_value >= list->quotation_size)
        {
            hex_error(ctx, "[symbol get] Index out of range");
            result = 1;
        }
        else
        {
            copy = hex_copy_item(ctx, list->data.quotation_value[index->data.int_value]);
            result = hex_push(ctx, copy);
        }
    }
    else if (list->type == HEX_TYPE_STRING)
    {
        if (index->type != HEX_TYPE_INTEGER)
        {
            hex_error(ctx, "[symbol get] Index must be an integer");
            result = 1;
        }
        else if (index->data.int_value < 0 || index->data.int_value >= (int)strlen(list->data.str_value))
        {
            hex_error(ctx, "[symbol get] Index out of range");
            result = 1;
        }
        else
        {
            char str[2] = {list->data.str_value[index->data.int_value], '\0'};
            copy = hex_string_item(ctx, str);
            result = hex_push(ctx, copy);
        }
    }
    else
    {
        hex_error(ctx, "[symbol get] Quotation or string required");
        result = 1;
    }
    if (result != 0)
    {
        HEX_FREE(ctx, list);
        HEX_FREE(ctx, index);
    }
    return result;
}

int hex_symbol_index(hex_context_t *ctx)
{
    HEX_POP(ctx, item);
    if (item->type == HEX_TYPE_INVALID)
    {
        HEX_FREE(ctx, item);
        return 1;
    }
    HEX_POP(ctx, list);
    if (list->type == HEX_TYPE_INVALID)
    {
        HEX_FREE(ctx, list);
        HEX_FREE(ctx, item);
        return 1;
    }
    int result = -1;
    if (list->type == HEX_TYPE_QUOTATION)
    {
        for (size_t i = 0; i < list->quotation_size; i++)
        {
            if (hex_equal(list->data.quotation_value[i], item))
            {
                result = i;
                break;
            }
        }
    }
    else if (list->type == HEX_TYPE_STRING)
    {
        char *ptr = strstr(list->data.str_value, item->data.str_value);
        if (ptr)
        {
            result = ptr - list->data.str_value;
        }
    }
    else
    {
        hex_error(ctx, "[symbol index] Quotation or string required");
        HEX_FREE(ctx, list);
        HEX_FREE(ctx, item);
        return 1;
    }
    return hex_push_integer(ctx, result);
}

// String symbols

int hex_symbol_join(hex_context_t *ctx)
{
    HEX_POP(ctx, separator);
    if (separator->type == HEX_TYPE_INVALID)
    {
        HEX_FREE(ctx, separator);
        return 1;
    }
    HEX_POP(ctx, list);
    if (list->type == HEX_TYPE_INVALID)
    {
        HEX_FREE(ctx, list);
        HEX_FREE(ctx, separator);
        return 1;
    }
    int result = 0;
    if (list->type == HEX_TYPE_QUOTATION && separator->type == HEX_TYPE_STRING)
    {
        int length = 0;
        for (size_t i = 0; i < list->quotation_size; i++)
        {
            if (list->data.quotation_value[i]->type == HEX_TYPE_STRING)
            {
                length += strlen(list->data.quotation_value[i]->data.str_value);
            }
            else
            {
                hex_error(ctx, "[symbol join] Quotation must contain only strings");
                HEX_FREE(ctx, list);
                HEX_FREE(ctx, separator);
                return 1;
            }
        }
        if (result == 0)
        {
            length += (list->quotation_size - 1) * strlen(separator->data.str_value);
            char *newStr = (char *)malloc(length + 1);
            if (!newStr)
            {
                hex_error(ctx, "[symbol join] Memory allocation failed");
                HEX_FREE(ctx, list);
                HEX_FREE(ctx, separator);
                return 1;
            }
            newStr[0] = '\0';
            for (size_t i = 0; i < list->quotation_size; i++)
            {
                strcat(newStr, list->data.quotation_value[i]->data.str_value);
                if (i < list->quotation_size - 1)
                {
                    strcat(newStr, separator->data.str_value);
                }
            }
            result = hex_push_string(ctx, newStr);
        }
    }
    else
    {
        hex_error(ctx, "[symbol join] Quotation and string required");
        result = 1;
    }
    if (result != 0)
    {
        HEX_FREE(ctx, list);
        HEX_FREE(ctx, separator);
    }
    return result;
}

int hex_symbol_split(hex_context_t *ctx)
{
    HEX_POP(ctx, separator);
    if (separator->type == HEX_TYPE_INVALID)
    {
        HEX_FREE(ctx, separator);
        return 1;
    }
    HEX_POP(ctx, str);
    if (str->type == HEX_TYPE_INVALID)
    {
        HEX_FREE(ctx, str);
        HEX_FREE(ctx, separator);
        return 1;
    }
    int result = 0;
    if (str->type == HEX_TYPE_STRING && separator->type == HEX_TYPE_STRING)
    {
        if (strlen(separator->data.str_value) == 0)
        {
            // Separator is an empty string: split into individual characters
            size_t size = strlen(str->data.str_value);
            hex_item_t **quotation = (hex_item_t **)calloc(size, sizeof(hex_item_t *));
            if (!quotation)
            {
                hex_error(ctx, "[symbol split] Memory allocation failed");
                result = 1;
            }
            else
            {
                for (size_t i = 0; i < size; i++)
                {
                    quotation[i] = (hex_item_t *)calloc(1, sizeof(hex_item_t));
                    if (!quotation[i])
                    {
                        hex_error(ctx, "[symbol split] Memory allocation failed");
                        result = 1;
                        break;
                    }
                    quotation[i]->type = HEX_TYPE_STRING;
                    quotation[i]->data.str_value = (char *)malloc(2); // Allocate 2 bytes: 1 for the character and 1 for null terminator
                    if (!quotation[i]->data.str_value)
                    {
                        hex_error(ctx, "[symbol split] Memory allocation failed");
                        result = 1;
                        break;
                    }
                    quotation[i]->data.str_value[0] = str->data.str_value[i]; // Copy the single character
                    quotation[i]->data.str_value[1] = '\0';                   // Null-terminate the string
                }
                if (result == 0)
                {
                    result = hex_push_quotation(ctx, quotation, size);
                }
            }
        }
        else
        {
            // Separator is not empty: split as usual
            char *token = strtok(str->data.str_value, separator->data.str_value);
            size_t capacity = 2;
            size_t size = 0;
            hex_item_t **quotation = (hex_item_t **)calloc(capacity, sizeof(hex_item_t *));
            if (!quotation)
            {
                hex_error(ctx, "[symbol split] Memory allocation failed");
                result = 1;
            }
            else
            {
                while (token)
                {
                    if (size >= capacity)
                    {
                        capacity *= 2;
                        hex_item_t **tmp = (hex_item_t **)realloc(quotation, capacity * sizeof(hex_item_t *));
                        if (!tmp)
                        {
                            hex_error(ctx, "[symbol split] Memory allocation failed");
                            result = 1;
                            break;
                        }
                        quotation = tmp;
                        if (!quotation)
                        {
                            hex_error(ctx, "[symbol split] Memory allocation failed");
                            result = 1;
                            break;
                        }
                    }
                    quotation[size] = (hex_item_t *)calloc(1, sizeof(hex_item_t));
                    quotation[size]->type = HEX_TYPE_STRING;
                    quotation[size]->data.str_value = strdup(token);
                    size++;
                    token = strtok(NULL, separator->data.str_value);
                }
                if (result == 0)
                {
                    result = hex_push_quotation(ctx, quotation, size);
                }
            }
        }
    }
    else
    {
        hex_error(ctx, "[symbol split] Two strings required");
        result = 1;
    }
    if (result != 0)
    {
        HEX_FREE(ctx, str);
        HEX_FREE(ctx, separator);
    }
    return result;
}

int hex_symbol_sub(hex_context_t *ctx)
{
    HEX_POP(ctx, replacement);
    if (replacement->type == HEX_TYPE_INVALID)
    {
        HEX_FREE(ctx, replacement);
        return 1;
    }
    HEX_POP(ctx, search);
    if (search->type == HEX_TYPE_INVALID)
    {
        HEX_FREE(ctx, search);
        HEX_FREE(ctx, replacement);
        return 1;
    }
    HEX_POP(ctx, list);
    if (list->type == HEX_TYPE_INVALID)
    {
        HEX_FREE(ctx, list);
        HEX_FREE(ctx, search);
        HEX_FREE(ctx, replacement);
        return 1;
    }
    int result = 0;
    if (list->type == HEX_TYPE_STRING && search->type == HEX_TYPE_STRING && replacement->type == HEX_TYPE_STRING)
    {
        char *str = list->data.str_value;
        char *find = search->data.str_value;
        char *replace = replacement->data.str_value;
        char *ptr = strstr(str, find);
        if (ptr)
        {
            int findLen = strlen(find);
            int replaceLen = strlen(replace);
            int newLen = strlen(str) - findLen + replaceLen + 1;
            char *newStr = (char *)malloc(newLen);
            if (!newStr)
            {
                hex_error(ctx, "[symbol sub] Memory allocation failed");
                result = 1;
            }
            else
            {
                strncpy(newStr, str, ptr - str);
                strcpy(newStr + (ptr - str), replace);
                strcpy(newStr + (ptr - str) + replaceLen, ptr + findLen);
                result = hex_push_string(ctx, newStr);
            }
        }
        else
        {
            result = hex_push_string(ctx, str);
        }
    }
    else
    {
        hex_error(ctx, "[symbol sub] Three strings required");
        result = 1;
    }
    if (result != 0)
    {
        HEX_FREE(ctx, list);
        HEX_FREE(ctx, search);
        HEX_FREE(ctx, replacement);
    }
    return result;
}

// File symbols

int hex_symbol_read(hex_context_t *ctx)
{
    HEX_POP(ctx, filename);
    ;
    if (filename->type == HEX_TYPE_INVALID)
    {
        HEX_FREE(ctx, filename);
        return 1;
    }
    int result = 0;
    if (filename->type == HEX_TYPE_STRING)
    {
        FILE *file = fopen(filename->data.str_value, "rb");
        if (!file)
        {
            hex_error(ctx, "[symbol read] Could not open file for reading: %s", filename->data.str_value);
            result = 1;
        }
        else
        {
            fseek(file, 0, SEEK_END);
            long length = ftell(file);
            fseek(file, 0, SEEK_SET);

            uint8_t *buffer = (uint8_t *)malloc(length);
            if (!buffer)
            {
                hex_error(ctx, "[symbol read] Memory allocation failed");
                result = 1;
            }
            else
            {
                size_t bytesRead = fread(buffer, 1, length, file);
                if (hex_is_binary(buffer, bytesRead))
                {
                    hex_item_t **quotation = (hex_item_t **)calloc(bytesRead, sizeof(hex_item_t *));
                    if (!quotation)
                    {
                        hex_error(ctx, "[symbol read] Memory allocation failed");
                        result = 1;
                    }
                    else
                    {
                        for (size_t i = 0; i < bytesRead; i++)
                        {
                            quotation[i] = (hex_item_t *)calloc(1, sizeof(hex_item_t));
                            quotation[i]->type = HEX_TYPE_INTEGER;
                            quotation[i]->data.int_value = buffer[i];
                        }
                        result = hex_push_quotation(ctx, quotation, bytesRead);
                    }
                }
                else
                {
                    char *str = hex_bytes_to_string(buffer, bytesRead);
                    if (!str)
                    {
                        hex_error(ctx, "[symbol read] Memory allocation failed");
                        result = 1;
                    }
                    else
                    {
                        hex_item_t *item = (hex_item_t *)calloc(1, sizeof(hex_item_t));
                        item->type = HEX_TYPE_STRING;
                        item->data.str_value = hex_process_string(str);
                        result = HEX_PUSH(ctx, item);
                    }
                }
            }
            fclose(file);
        }
    }
    else
    {
        hex_error(ctx, "[symbol read] String required");
        result = 1;
    }
    if (result != 0)
    {
        HEX_FREE(ctx, filename);
    }
    return result;
}

int hex_symbol_write(hex_context_t *ctx)
{
    HEX_POP(ctx, filename);
    if (filename->type == HEX_TYPE_INVALID)
    {
        HEX_FREE(ctx, filename);
        return 1;
    }
    HEX_POP(ctx, data);
    if (data->type == HEX_TYPE_INVALID)
    {
        HEX_FREE(ctx, data);
        HEX_FREE(ctx, filename);
        return 1;
    }
    int result = 0;
    if (filename->type == HEX_TYPE_STRING)
    {
        if (data->type == HEX_TYPE_STRING)
        {
            FILE *file = fopen(filename->data.str_value, "w");
            if (file)
            {
                fputs(data->data.str_value, file);
                fclose(file);
                result = 0;
            }
            else
            {
                hex_error(ctx, "[symbol write] Could not open file for writing: %s", filename->data.str_value);
                result = 1;
            }
        }
        else if (data->type == HEX_TYPE_QUOTATION)
        {
            FILE *file = fopen(filename->data.str_value, "wb");
            if (file)
            {
                for (size_t i = 0; i < data->quotation_size; i++)
                {
                    if (data->data.quotation_value[i]->type != HEX_TYPE_INTEGER)
                    {
                        hex_error(ctx, "[symbol write] Quotation must contain only integers");
                        result = 1;
                        break;
                    }
                    uint8_t byte = (uint8_t)data->data.quotation_value[i]->data.int_value;
                    fwrite(&byte, 1, 1, file);
                }
                fclose(file);
                result = 0;
            }
            else
            {
                hex_error(ctx, "[symbol write] Could not open file for writing: %s", filename->data.str_value);
                result = 1;
            }
        }
        else
        {
            hex_error(ctx, "[symbol write] String or quotation of integers required");
            result = 1;
        }
    }
    else
    {
        hex_error(ctx, "[symbol write] String required");
        result = 1;
    }
    if (result != 0)
    {
        HEX_FREE(ctx, data);
        HEX_FREE(ctx, filename);
    }
    return result;
}

int hex_symbol_append(hex_context_t *ctx)
{
    HEX_POP(ctx, filename);
    if (filename->type == HEX_TYPE_INVALID)
    {
        HEX_FREE(ctx, filename);
        return 1;
    }
    HEX_POP(ctx, data);
    if (data->type == HEX_TYPE_INVALID)
    {
        HEX_FREE(ctx, data);
        HEX_FREE(ctx, filename);
        return 1;
    }
    int result = 0;
    if (filename->type == HEX_TYPE_STRING)
    {
        if (data->type == HEX_TYPE_STRING)
        {
            FILE *file = fopen(filename->data.str_value, "a");
            if (file)
            {
                fputs(data->data.str_value, file);
                fclose(file);
                result = 0;
            }
            else
            {
                hex_error(ctx, "[symbol append] Could not open file for appending: %s", filename->data.str_value);
                result = 1;
            }
        }
        else if (data->type == HEX_TYPE_QUOTATION)
        {
            FILE *file = fopen(filename->data.str_value, "ab");
            if (file)
            {
                for (size_t i = 0; i < data->quotation_size; i++)
                {
                    if (data->data.quotation_value[i]->type != HEX_TYPE_INTEGER)
                    {
                        hex_error(ctx, "[symbol append] Quotation must contain only integers");
                        result = 1;
                        break;
                    }
                    uint8_t byte = (uint8_t)data->data.quotation_value[i]->data.int_value;
                    fwrite(&byte, 1, 1, file);
                }
                fclose(file);
                result = 0;
            }
            else
            {
                hex_error(ctx, "[symbol append] Could not open file for appending: %s", filename->data.str_value);
                result = 1;
            }
        }
        else
        {
            hex_error(ctx, "[symbol append] String or quotation of integers required");
            result = 1;
        }
    }
    else
    {
        hex_error(ctx, "[symbol append] String required");
        result = 1;
    }
    if (result != 0)
    {
        HEX_FREE(ctx, data);
        HEX_FREE(ctx, filename);
    }
    return result;
}

// Shell symbols

int hex_symbol_args(hex_context_t *ctx)
{
    hex_item_t **quotation = (hex_item_t **)calloc(ctx->argc, sizeof(hex_item_t *));
    if (!quotation)
    {
        hex_error(ctx, "[symbol args] Memory allocation failed");
        return 1;
    }
    else
    {
        for (size_t i = 0; i < (size_t)ctx->argc; i++)
        {
            quotation[i] = (hex_item_t *)calloc(1, sizeof(hex_item_t));
            quotation[i]->type = HEX_TYPE_STRING;
            quotation[i]->data.str_value = ctx->argv[i];
        }
        if (hex_push_quotation(ctx, quotation, ctx->argc) != 0)
        {
            hex_free_list(ctx, quotation, ctx->argc);
            return 1;
        }
    }
    return 0;
}

int hex_symbol_exit(hex_context_t *ctx)
{
    HEX_POP(ctx, item);
    ;
    if (item->type == HEX_TYPE_INVALID)
    {
        HEX_FREE(ctx, item);
        return 1;
    }
    if (item->type != HEX_TYPE_INTEGER)
    {
        hex_error(ctx, "[symbol exit] Integer required");
        HEX_FREE(ctx, item);
        return 1;
    }
    int exit_status = item->data.int_value;
    exit(exit_status);
    return 0; // This line will never be reached, but it's here to satisfy the return type
}

int hex_symbol_exec(hex_context_t *ctx)
{
    HEX_POP(ctx, command);
    if (command->type == HEX_TYPE_INVALID)
    {
        HEX_FREE(ctx, command);
        return 1;
    }
    int result = 0;
    if (command->type == HEX_TYPE_STRING)
    {
        int status = system(command->data.str_value);
        result = hex_push_integer(ctx, status);
    }
    else
    {
        hex_error(ctx, "[symbol exec] String required");
        result = 1;
    }
    return result;
}

int hex_symbol_run(hex_context_t *ctx)
{
    HEX_POP(ctx, command);
    if (command->type == HEX_TYPE_INVALID)
    {
        HEX_FREE(ctx, command);
        return 1;
    }
    if (command->type != HEX_TYPE_STRING)
    {
        hex_error(ctx, "[symbol run] String required");
        HEX_FREE(ctx, command);
        return 1;
    }

    char output[8192] = "";
    char error[8192] = "";
    int return_code = 0;

#ifdef _WIN32
    // Windows implementation
    HANDLE hOutputRead, hOutputWrite;
    HANDLE hErrorRead, hErrorWrite;
    SECURITY_ATTRIBUTES sa;
    PROCESS_INFORMATION pi;
    STARTUPINFO si;
    DWORD exitCode;

    sa.nLength = sizeof(SECURITY_ATTRIBUTES);
    sa.lpSecurityDescriptor = NULL;
    sa.bInheritHandle = TRUE;

    // Create pipes for capturing stdout and stderr
    if (!CreatePipe(&hOutputRead, &hOutputWrite, &sa, 0) || !CreatePipe(&hErrorRead, &hErrorWrite, &sa, 0))
    {
        hex_error(ctx, "[symbol run] Failed to create pipes");
        HEX_FREE(ctx, command);
        return 1;
    }

    // Set up STARTUPINFO structure
    ZeroMemory(&si, sizeof(STARTUPINFO));
    si.cb = sizeof(STARTUPINFO);
    si.hStdOutput = hOutputWrite;
    si.hStdError = hErrorWrite;
    si.dwFlags |= STARTF_USESTDHANDLES;

    // Create the child process
    if (!CreateProcess(NULL, command->data.str_value, NULL, NULL, TRUE, 0, NULL, NULL, &si, &pi))
    {
        hex_error(ctx, "[symbol run] Failed to create process");
        HEX_FREE(ctx, command);
        return 1;
    }

    // Close write ends of the pipes
    CloseHandle(hOutputWrite);
    CloseHandle(hErrorWrite);

    // Read stdout
    DWORD bytesRead;
    char buffer[1024];
    while (ReadFile(hOutputRead, buffer, sizeof(buffer) - 1, &bytesRead, NULL) && bytesRead > 0)
    {
        buffer[bytesRead] = '\0';
        strcat(output, buffer);
    }

    // Read stderr
    while (ReadFile(hErrorRead, buffer, sizeof(buffer) - 1, &bytesRead, NULL) && bytesRead > 0)
    {
        buffer[bytesRead] = '\0';
        strcat(error, buffer);
    }

    // Wait for the child process to finish and get the return code
    WaitForSingleObject(pi.hProcess, INFINITE);
    GetExitCodeProcess(pi.hProcess, &exitCode);
    return_code = exitCode;

    // Close handles
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    CloseHandle(hOutputRead);
    CloseHandle(hErrorRead);
#else
    // POSIX implementation (Linux/macOS)
    int stdout_pipe[2];
    int stderr_pipe[2];
    if (pipe(stdout_pipe) != 0 || pipe(stderr_pipe) != 0)
    {
        hex_error(ctx, "[symbol run] Failed to create pipes");
        HEX_FREE(ctx, command);
        return 1;
    }

    pid_t pid = fork();
    if (pid == -1)
    {
        hex_error(ctx, "[symbol run] Failed to fork process");
        HEX_FREE(ctx, command);
        return 1;
    }
    else if (pid == 0)
    {
        // Child process
        close(stdout_pipe[0]);
        close(stderr_pipe[0]);
        dup2(stdout_pipe[1], STDOUT_FILENO);
        dup2(stderr_pipe[1], STDERR_FILENO);
        execl("/bin/sh", "sh", "-c", command->data.str_value, (char *)NULL);
        exit(1);
    }
    else
    {
        // Parent process
        close(stdout_pipe[1]);
        close(stderr_pipe[1]);

        // Read stdout
        FILE *stdout_fp = fdopen(stdout_pipe[0], "r");
        char path[1035];
        while (fgets(path, sizeof(path), stdout_fp) != NULL)
        {
            strcat(output, path);
        }
        fclose(stdout_fp);

        // Read stderr
        FILE *stderr_fp = fdopen(stderr_pipe[0], "r");
        while (fgets(path, sizeof(path), stderr_fp) != NULL)
        {
            strcat(error, path);
        }
        fclose(stderr_fp);

        // Wait for child process to finish and get the return code
        int status;
        waitpid(pid, &status, 0);
        return_code = WEXITSTATUS(status);
    }
#endif

    // Push the return code, output, and error as a quotation
    hex_item_t **quotation = (hex_item_t **)calloc(3, sizeof(hex_item_t *));
    quotation[0] = (hex_item_t *)calloc(1, sizeof(hex_item_t));
    quotation[0]->type = HEX_TYPE_INTEGER;
    quotation[0]->data.int_value = return_code;

    quotation[1] = (hex_item_t *)calloc(1, sizeof(hex_item_t));
    quotation[1]->type = HEX_TYPE_STRING;
    quotation[1]->data.str_value = strdup(output);

    quotation[2] = (hex_item_t *)calloc(1, sizeof(hex_item_t));
    quotation[2]->type = HEX_TYPE_STRING;
    quotation[2]->data.str_value = strdup(error);

    return hex_push_quotation(ctx, quotation, 3);
}

// Control flow symbols

int hex_symbol_if(hex_context_t *ctx)
{
    HEX_POP(ctx, elseBlock);
    ;
    if (elseBlock->type == HEX_TYPE_INVALID)
    {
        HEX_FREE(ctx, elseBlock);
        return 1;
    }
    HEX_POP(ctx, thenBlock);
    ;
    if (thenBlock->type == HEX_TYPE_INVALID)
    {
        HEX_FREE(ctx, thenBlock);
        HEX_FREE(ctx, elseBlock);
        return 1;
    }
    HEX_POP(ctx, condition);
    ;
    if (condition->type == HEX_TYPE_INVALID)
    {
        HEX_FREE(ctx, condition);
        HEX_FREE(ctx, thenBlock);
        HEX_FREE(ctx, elseBlock);
        return 1;
    }

    if (condition->type != HEX_TYPE_QUOTATION || thenBlock->type != HEX_TYPE_QUOTATION || elseBlock->type != HEX_TYPE_QUOTATION)
    {
        hex_error(ctx, "[symbol if] Three quotations required");
        HEX_FREE(ctx, condition);
        HEX_FREE(ctx, thenBlock);
        HEX_FREE(ctx, elseBlock);
        return 1;
    }
    else
    {
        for (size_t i = 0; i < condition->quotation_size; i++)
        {
            if (hex_push(ctx, condition->data.quotation_value[i]) != 0)
            {
                HEX_FREE(ctx, condition);
                HEX_FREE(ctx, thenBlock);
                HEX_FREE(ctx, elseBlock);
                return 1;
            }
        }
        HEX_POP(ctx, evalResult);
        ;
        if (evalResult->type == HEX_TYPE_INTEGER && evalResult->data.int_value > 0)
        {
            for (size_t i = 0; i < thenBlock->quotation_size; i++)
            {
                if (hex_push(ctx, thenBlock->data.quotation_value[i]) != 0)
                {
                    HEX_FREE(ctx, condition);
                    HEX_FREE(ctx, thenBlock);
                    HEX_FREE(ctx, elseBlock);
                    HEX_FREE(ctx, evalResult);
                    return 1;
                }
            }
        }
        else
        {
            for (size_t i = 0; i < elseBlock->quotation_size; i++)
            {
                if (hex_push(ctx, elseBlock->data.quotation_value[i]) != 0)
                {
                    HEX_FREE(ctx, condition);
                    HEX_FREE(ctx, thenBlock);
                    HEX_FREE(ctx, elseBlock);
                    HEX_FREE(ctx, evalResult);
                    return 1;
                }
            }
        }
    }
    return 0;
}

int hex_symbol_while(hex_context_t *ctx)
{
    HEX_POP(ctx, action);
    ;
    if (action->type == HEX_TYPE_INVALID)
    {
        HEX_FREE(ctx, action);
        return 1;
    }
    HEX_POP(ctx, condition);
    ;
    if (condition->type == HEX_TYPE_INVALID)
    {
        HEX_FREE(ctx, action);
        HEX_FREE(ctx, condition);
        return 1;
    }

    if (condition->type != HEX_TYPE_QUOTATION || action->type != HEX_TYPE_QUOTATION)
    {
        hex_error(ctx, "[symbol while] Two quotations required");
        HEX_FREE(ctx, action);
        HEX_FREE(ctx, condition);
        return 1;
    }
    else
    {
        while (1)
        {
            for (size_t i = 0; i < condition->quotation_size; i++)
            {
                // Create a copy to avoid ownership issues
                hex_item_t *copy = hex_copy_item(ctx, condition->data.quotation_value[i]);
                if (!copy || hex_push(ctx, copy) != 0)
                {
                    if (copy)
                        hex_free_item(ctx, copy);
                    HEX_FREE(ctx, action);
                    HEX_FREE(ctx, condition);
                    return 1;
                }
            }
            HEX_POP(ctx, evalResult);
            if (evalResult->type == HEX_TYPE_INTEGER && evalResult->data.int_value == 0)
            {
                // Don't free evalResult here as it might be shared - let normal cleanup handle it
                break;
            }

            hex_item_t *act = hex_copy_item(ctx, action);
            for (size_t i = 0; i < act->quotation_size; i++)
            {
                // Create a copy to avoid ownership issues
                hex_item_t *copy = hex_copy_item(ctx, act->data.quotation_value[i]);
                if (!copy || hex_push(ctx, copy) != 0)
                {
                    if (copy)
                        hex_free_item(ctx, copy);
                    HEX_FREE(ctx, act);
                    HEX_FREE(ctx, action);
                    HEX_FREE(ctx, condition);
                    return 1;
                }
            }
            HEX_FREE(ctx, act); // Free the temporary copy
        }
    }

    // Clean up after successful completion
    HEX_FREE(ctx, action);
    HEX_FREE(ctx, condition);
    return 0;
}

int hex_symbol_error(hex_context_t *ctx)
{

    char *message = strdup(ctx->error);
    ctx->error[0] = '\0';
    return hex_push_string(ctx, message);
}

int hex_symbol_try(hex_context_t *ctx)
{
    HEX_POP(ctx, catch_block);
    if (catch_block->type == HEX_TYPE_INVALID)
    {
        HEX_FREE(ctx, catch_block);
        return 1;
    }
    HEX_POP(ctx, try_block);
    if (try_block->type == HEX_TYPE_INVALID)
    {
        HEX_FREE(ctx, catch_block);
        HEX_FREE(ctx, try_block);
        return 1;
    }

    if (try_block->type != HEX_TYPE_QUOTATION || catch_block->type != HEX_TYPE_QUOTATION)
    {
        hex_error(ctx, "[symbol try] Two quotations required");
        HEX_FREE(ctx, catch_block);
        HEX_FREE(ctx, try_block);
        return 1;
    }
    else
    {
        char prevError[256];
        strncpy(prevError, ctx->error, sizeof(ctx->error));
        ctx->error[0] = '\0';

        ctx->settings->errors_enabled = 0;
        for (size_t i = 0; i < try_block->quotation_size; i++)
        {
            if (hex_push(ctx, try_block->data.quotation_value[i]) != 0)
            {
                ctx->settings->errors_enabled = 1;
            }
        }
        ctx->settings->errors_enabled = 1;

        if (strcmp(ctx->error, "") != 0)
        {
            hex_debug(ctx, "[symbol try] Handling error: %s", ctx->error);
            for (size_t i = 0; i < catch_block->quotation_size; i++)
            {
                if (hex_push(ctx, catch_block->data.quotation_value[i]) != 0)
                {
                    HEX_FREE(ctx, catch_block);
                    HEX_FREE(ctx, try_block);
                    return 1;
                }
            }
        }

        strncpy(ctx->error, prevError, sizeof(ctx->error));
    }
    return 0;
}

int hex_symbol_throw(hex_context_t *ctx)
{
    HEX_POP(ctx, message);
    ;
    if (message->type == HEX_TYPE_INVALID)
    {
        HEX_FREE(ctx, message);
        return 1;
    }
    if (message->type != HEX_TYPE_STRING)
    {
        hex_error(ctx, "[symbol throw] String required");
        HEX_FREE(ctx, message);
        return 1;
    }
    hex_error(ctx, message->data.str_value);
    return 1;
}

// Quotation symbols

int hex_symbol_q(hex_context_t *ctx)
{
    HEX_POP(ctx, item);
    if (item->type == HEX_TYPE_INVALID)
    {
        HEX_FREE(ctx, item);
        return 1;
    }

    // Deep copy the popped item to avoid aliasing its internal pointers.
    hex_item_t *copy = hex_copy_item(ctx, item);
    if (!copy)
    {
        hex_error(ctx, "[symbol '] Failed to copy item");
        HEX_FREE(ctx, item);
        return 1;
    }

    hex_item_t *result = (hex_item_t *)calloc(1, sizeof(hex_item_t));
    if (!result)
    {
        hex_error(ctx, "[symbol '] Memory allocation failed");
        HEX_FREE(ctx, item);
        HEX_FREE(ctx, copy);
        return 1;
    }

    result->type = HEX_TYPE_QUOTATION;
    result->data.quotation_value = (hex_item_t **)calloc(1, sizeof(hex_item_t *));
    if (!result->data.quotation_value)
    {
        hex_error(ctx, "[symbol '] Memory allocation failed");
        HEX_FREE(ctx, item);
        HEX_FREE(ctx, copy);
        HEX_FREE(ctx, result);
        return 1;
    }

    result->data.quotation_value[0] = copy;
    result->quotation_size = 1;

    // Original item no longer needed (we pushed a deep copy)
    HEX_FREE(ctx, item);

    if (HEX_PUSH(ctx, result) != 0)
    {
        HEX_FREE(ctx, result); // will free contained copy via list free
        return 1;
    }
    return 0;
}

int hex_symbol_map(hex_context_t *ctx)
{
    HEX_POP(ctx, action);
    ;
    if (action->type == HEX_TYPE_INVALID)
    {
        HEX_FREE(ctx, action);
        return 1;
    }
    HEX_POP(ctx, list);
    ;
    if (list->type == HEX_TYPE_INVALID)
    {
        HEX_FREE(ctx, action);
        HEX_FREE(ctx, list);
        return 1;
    }

    if (list->type != HEX_TYPE_QUOTATION || action->type != HEX_TYPE_QUOTATION)
    {
        hex_error(ctx, "[symbol map] Two quotations required");
        HEX_FREE(ctx, action);
        HEX_FREE(ctx, list);
        return 1;
    }
    else
    {
        // Allocate result quotation (array of element pointers)
        hex_item_t **quotation = (hex_item_t **)calloc(list->quotation_size, sizeof(hex_item_t *));
        if (!quotation)
        {
            hex_error(ctx, "[symbol map] Memory allocation failed");
            HEX_FREE(ctx, action);
            HEX_FREE(ctx, list);
            return 1;
        }
        for (size_t i = 0; i < list->quotation_size; i++)
        {
            // Push a deep copy of the list element to avoid aliasing
            hex_item_t *elem_copy = hex_copy_item(ctx, list->data.quotation_value[i]);
            if (!elem_copy || hex_push(ctx, elem_copy) != 0)
            {
                if (elem_copy)
                {
                    hex_free_item(ctx, elem_copy);
                }
                HEX_FREE(ctx, action);
                HEX_FREE(ctx, list);
                hex_free_list(ctx, quotation, i);
                return 1;
            }
            // Execute action quotation: push deep copies of its elements
            for (size_t j = 0; j < action->quotation_size; j++)
            {
                hex_item_t *act_elem = hex_copy_item(ctx, action->data.quotation_value[j]);
                if (!act_elem || hex_push(ctx, act_elem) != 0)
                {
                    if (act_elem)
                    {
                        hex_free_item(ctx, act_elem);
                    }
                    HEX_FREE(ctx, action);
                    HEX_FREE(ctx, list);
                    hex_free_list(ctx, quotation, i);
                    return 1;
                }
            }
            // Pop result of action execution, copy into result quotation, free temporary
            hex_item_t *result_item = hex_pop(ctx);
            quotation[i] = hex_copy_item(ctx, result_item);
            HEX_FREE(ctx, result_item);
            if (!quotation[i])
            {
                hex_error(ctx, "[symbol map] Failed to copy result item");
                HEX_FREE(ctx, action);
                HEX_FREE(ctx, list);
                hex_free_list(ctx, quotation, i);
                return 1;
            }
        }
        if (hex_push_quotation(ctx, quotation, list->quotation_size) != 0)
        {
            HEX_FREE(ctx, action);
            HEX_FREE(ctx, list);
            hex_free_list(ctx, quotation, list->quotation_size);
            return 1;
        }
        // Free consumed inputs after success
        HEX_FREE(ctx, action);
        HEX_FREE(ctx, list);
    }

    return 0;
}

// Stack manipulation symbols
int hex_symbol_swap(hex_context_t *ctx)
{
    HEX_POP(ctx, a);
    if (a->type == HEX_TYPE_INVALID)
    {
        HEX_FREE(ctx, a);
        return 1;
    }
    HEX_POP(ctx, b);
    ;
    if (b->type == HEX_TYPE_INVALID)
    {
        HEX_FREE(ctx, b);
        HEX_FREE(ctx, a);
        return 1;
    }
    if (HEX_PUSH(ctx, a) != 0)
    {
        HEX_FREE(ctx, a);
        HEX_FREE(ctx, b);
        return 1;
    }
    if (HEX_PUSH(ctx, b) != 0)
    {
        HEX_FREE(ctx, a);
        HEX_FREE(ctx, b);
        return 1;
    }
    return 0;
}

int hex_symbol_dup(hex_context_t *ctx)
{
    HEX_POP(ctx, item);
    ;
    if (item->type == HEX_TYPE_INVALID)
    {
        HEX_FREE(ctx, item);
        return 1;
    }
    hex_item_t *copy = hex_copy_item(ctx, item);
    if (!copy)
    {
        hex_error(ctx, "[symbol dup] Memory allocation failed");
        HEX_FREE(ctx, item);
        return 1;
    }
    if (HEX_PUSH(ctx, copy) == 0 && HEX_PUSH(ctx, item) == 0)
    {
        return 0;
    }
    HEX_FREE(ctx, item);
    HEX_FREE(ctx, copy);
    return 1;
}

int hex_symbol_stack(hex_context_t *ctx)
{

    hex_item_t **quotation = (hex_item_t **)calloc((ctx->stack->top + 2), sizeof(hex_item_t *));
    if (!quotation)
    {
        hex_error(ctx, "[symbol stack] Memory allocation failed");
        return 1;
    }
    int count = 0;
    if (ctx->stack->top == -1)
    {
        if (hex_push_quotation(ctx, quotation, 0) != 0)
        {
            hex_error(ctx, "[symbol stack] An error occurred while pushing empty quotation");
            return 1;
        }
        return 0;
    }
    for (int i = 0; i <= ctx->stack->top; i++)
    {
        quotation[i] = hex_copy_item(ctx, ctx->stack->entries[i]);
        if (!quotation[i])
        {
            hex_error(ctx, "[symbol stack] Memory allocation failed");
            hex_free_list(ctx, quotation, count);
            return 1;
        }
        count++;
    }

    if (hex_push_quotation(ctx, quotation, count) != 0)
    {
        hex_error(ctx, "[symbol stack] An error occurred while pushing quotation");
        hex_free_list(ctx, quotation, count);
        return 1;
    }
    return 0;
}

int hex_symbol_drop(hex_context_t *ctx)
{
    HEX_POP(ctx, item);
    if (item != NULL)
    {
        HEX_FREE(ctx, item);
    }
    return 0;
}

int hex_symbol_timestamp(hex_context_t *ctx)
{
    static int32_t timestamp[2];
    get_unix_timestamp_sec_usec(timestamp);
    hex_item_t **quotation = (hex_item_t **)calloc(2, sizeof(hex_item_t *));
    if (!quotation)
    {
        hex_error(ctx, "[symbol timestamp] Memory allocation failed");
        return 1;
    }
    quotation[0] = hex_integer_item(ctx, timestamp[0]);
    quotation[1] = hex_integer_item(ctx, timestamp[1]);
    if (hex_push_quotation(ctx, quotation, 2) != 0)
    {
        hex_error(ctx, "[symbol timestamp] An error occurred while pushing quotation");
        hex_free_list(ctx, quotation, 2);
        return 1;
    }
    return 0;
}

#ifdef DEBUG
// Recursively validate that quotations do not share element pointers with siblings.
// Returns 0 on success, 1 if a potential aliasing issue is detected.
int hex_validate_quotation_integrity(hex_context_t *ctx, const hex_item_t *item)
{
    (void)ctx;
    if (!item)
    {
        return 0;
    }
    if (item->type != HEX_TYPE_QUOTATION || item->quotation_size == 0)
    {
        return 0;
    }
    // Simple O(n^2) pointer identity check among siblings; acceptable for debug.
    for (size_t i = 0; i < item->quotation_size; i++)
    {
        hex_item_t *a = item->data.quotation_value[i];
        for (size_t j = i + 1; j < item->quotation_size; j++)
        {
            if (a == item->data.quotation_value[j])
            {
                hex_error(ctx, "[integrity] Quotation has duplicated element pointer at %zu and %zu", i, j);
                return 1;
            }
        }
        // Recurse
        if (hex_validate_quotation_integrity(ctx, a) != 0)
        {
            return 1;
        }
    }
    return 0;
}

int hex_debug_validate_stack(hex_context_t *ctx)
{
    if (!ctx || !ctx->stack)
    {
        return 0;
    }
    for (int i = 0; i <= ctx->stack->top; i++)
    {
        hex_item_t *it = ctx->stack->entries[i];
        if (it && hex_validate_quotation_integrity(ctx, it) != 0)
        {
            hex_error(ctx, "[integrity] Detected potential aliasing at stack index %d", i);
            return 1;
        }
    }
    return 0;
}
#endif

////////////////////////////////////////
// Native Symbol Registration         //
////////////////////////////////////////

void hex_register_symbols(hex_context_t *ctx)
{
    hex_set_native_symbol(ctx, ":", hex_symbol_store);
    hex_set_native_symbol(ctx, "::", hex_symbol_define);
    hex_set_native_symbol(ctx, "#", hex_symbol_free);
    hex_set_native_symbol(ctx, "symbols", hex_symbol_symbols);
    hex_set_native_symbol(ctx, "type", hex_symbol_type);
    hex_set_native_symbol(ctx, ".", hex_symbol_i);
    hex_set_native_symbol(ctx, "!", hex_symbol_eval);
    hex_set_native_symbol(ctx, "puts", hex_symbol_puts);
    hex_set_native_symbol(ctx, "warn", hex_symbol_warn);
    hex_set_native_symbol(ctx, "print", hex_symbol_print);
    hex_set_native_symbol(ctx, "gets", hex_symbol_gets);
    hex_set_native_symbol(ctx, "+", hex_symbol_add);
    hex_set_native_symbol(ctx, "-", hex_symbol_subtract);
    hex_set_native_symbol(ctx, "*", hex_symbol_multiply);
    hex_set_native_symbol(ctx, "/", hex_symbol_divide);
    hex_set_native_symbol(ctx, "%", hex_symbol_modulo);
    hex_set_native_symbol(ctx, "&", hex_symbol_bitand);
    hex_set_native_symbol(ctx, "|", hex_symbol_bitor);
    hex_set_native_symbol(ctx, "^", hex_symbol_bitxor);
    hex_set_native_symbol(ctx, "~", hex_symbol_bitnot);
    hex_set_native_symbol(ctx, "<<", hex_symbol_shiftleft);
    hex_set_native_symbol(ctx, ">>", hex_symbol_shiftright);
    hex_set_native_symbol(ctx, "int", hex_symbol_int);
    hex_set_native_symbol(ctx, "str", hex_symbol_str);
    hex_set_native_symbol(ctx, "dec", hex_symbol_dec);
    hex_set_native_symbol(ctx, "hex", hex_symbol_hex);
    hex_set_native_symbol(ctx, "chr", hex_symbol_chr);
    hex_set_native_symbol(ctx, "ord", hex_symbol_ord);
    hex_set_native_symbol(ctx, "==", hex_symbol_equal);
    hex_set_native_symbol(ctx, "!=", hex_symbol_notequal);
    hex_set_native_symbol(ctx, ">", hex_symbol_greater);
    hex_set_native_symbol(ctx, "<", hex_symbol_less);
    hex_set_native_symbol(ctx, ">=", hex_symbol_greaterequal);
    hex_set_native_symbol(ctx, "<=", hex_symbol_lessequal);
    hex_set_native_symbol(ctx, "and", hex_symbol_and);
    hex_set_native_symbol(ctx, "or", hex_symbol_or);
    hex_set_native_symbol(ctx, "not", hex_symbol_not);
    hex_set_native_symbol(ctx, "xor", hex_symbol_xor);
    hex_set_native_symbol(ctx, "cat", hex_symbol_cat);
    hex_set_native_symbol(ctx, "len", hex_symbol_len);
    hex_set_native_symbol(ctx, "get", hex_symbol_get);
    hex_set_native_symbol(ctx, "index", hex_symbol_index);
    hex_set_native_symbol(ctx, "join", hex_symbol_join);
    hex_set_native_symbol(ctx, "split", hex_symbol_split);
    hex_set_native_symbol(ctx, "sub", hex_symbol_sub);
    hex_set_native_symbol(ctx, "read", hex_symbol_read);
    hex_set_native_symbol(ctx, "write", hex_symbol_write);
    hex_set_native_symbol(ctx, "append", hex_symbol_append);
    hex_set_native_symbol(ctx, "args", hex_symbol_args);
    hex_set_native_symbol(ctx, "exit", hex_symbol_exit);
    hex_set_native_symbol(ctx, "exec", hex_symbol_exec);
    hex_set_native_symbol(ctx, "run", hex_symbol_run);
    hex_set_native_symbol(ctx, "if", hex_symbol_if);
    hex_set_native_symbol(ctx, "while", hex_symbol_while);
    hex_set_native_symbol(ctx, "error", hex_symbol_error);
    hex_set_native_symbol(ctx, "try", hex_symbol_try);
    hex_set_native_symbol(ctx, "throw", hex_symbol_throw);
    hex_set_native_symbol(ctx, "'", hex_symbol_q);
    hex_set_native_symbol(ctx, "map", hex_symbol_map);
    hex_set_native_symbol(ctx, "swap", hex_symbol_swap);
    hex_set_native_symbol(ctx, "dup", hex_symbol_dup);
    hex_set_native_symbol(ctx, "stack", hex_symbol_stack);
    hex_set_native_symbol(ctx, "drop", hex_symbol_drop);
    hex_set_native_symbol(ctx, "timestamp", hex_symbol_timestamp);
}

/* File: src/main.c */
#line 1 "src/main.c"
#ifndef HEX_H
#include "hex.h"
#endif

// Read a file into a buffer
char *hex_read_file(hex_context_t *ctx, const char *filename)
{
    FILE *file = fopen(filename, "r");
    if (file == NULL)
    {
        hex_error(ctx, "[read input file] Failed to open file: %s", filename);
        return NULL;
    }

    // Allocate an initial buffer
    int bufferSize = 1024; // Start with a 1 KB buffer
    char *content = (char *)malloc(bufferSize);
    if (content == NULL)
    {
        hex_error(ctx, "[read input file] Memory allocation failed");
        fclose(file);
        return NULL;
    }

    int bytesReadTotal = 0;
    int bytesRead = 0;

    // Handle hashbang if present
    char hashbangLine[1024];
    if (fgets(hashbangLine, sizeof(hashbangLine), file) != NULL)
    {
        if (strncmp(hashbangLine, "#!", 2) != 0)
        {
            // Not a hashbang line, reset file pointer to the beginning
            fseek(file, 0, SEEK_SET);
            ctx->hashbang = 0;
        }
        else
        {
            ctx->hashbang = 1;
        }
    }

    while ((bytesRead = fread(content + bytesReadTotal, 1, bufferSize - bytesReadTotal, file)) > 0)
    {
        bytesReadTotal += bytesRead;

        // Resize the buffer if necessary
        if (bytesReadTotal == bufferSize)
        {
            bufferSize *= 2; // Double the buffer size
            char *temp = (char *)realloc(content, bufferSize);
            if (temp == NULL)
            {
                hex_error(ctx, "[read input bytecode file] Memory reallocation failed");
                free(content);
                fclose(file);
                return NULL;
            }
            content = temp;
        }
    }

    if (ferror(file))
    {
        hex_error(ctx, "[read input file] Error reading the file");
        free(content);
        fclose(file);
        return NULL;
    }

    // Null-terminate the content
    char *finalContent = (char *)realloc(content, bytesReadTotal + 1);
    if (finalContent == NULL)
    {
        hex_error(ctx, "[read file] Final memory allocation failed");
        free(content);
        fclose(file);
        return NULL;
    }
    content = finalContent;
    content[bytesReadTotal] = '\0';

    fclose(file);
    return content;
}

#if defined(__EMSCRIPTEN__) && defined(BROWSER)
static void prompt()
{
    // no prompt needed on browser
}
#elif defined(__EMSCRIPTEN__) && !defined(BROWSER)
static void prompt()
{
    printf(">\n");
}
#else
static void prompt()
{
    printf("> ");
    fflush(stdout);
}
#endif

#if defined(__EMSCRIPTEN__)
static void do_repl(void *v_ctx)
{
    hex_context_t *ctx = (hex_context_t *)v_ctx;
    prompt();
    char line[HEX_STDIN_BUFFER_SIZE];
    char *p = line;
    p = em_fgets(line, HEX_STDIN_BUFFER_SIZE);
    if (!p)
    {
        printf("Error reading output");
        return;
    }
    // Normalize line endings (remove trailing \r\n or \n)
    line[strcspn(line, "\r\n")] = '\0';

    // Tokenize and process the input
    hex_interpret(ctx, line, "<repl>", 1, 1);
    // Print the top item of the stack
    if (ctx->stack->top >= 0)
    {
        hex_print_item(stdout, ctx->stack->entries[ctx->stack->top]);
        // hex_print_item(stdout, HEX_STACK[HEX_TOP]);
        printf("\n");
    }
    return;
}

#else

static int do_repl(void *v_ctx)
{
    hex_context_t *ctx = (hex_context_t *)v_ctx;
    char line[1024];
    prompt();
    if (fgets(line, sizeof(line), stdin) == NULL)
    {
        printf("\n"); // Handle EOF (Ctrl+D)
        return 1;
    }
    // Normalize line endings (remove trailing \r\n or \n)
    line[strcspn(line, "\r\n")] = '\0';

    // Tokenize and process the input
    hex_interpret(ctx, line, "<repl>", 1, 1);
    // Print the top item of the stack
    if (ctx->stack->top >= 0)
    {
        hex_print_item(stdout, ctx->stack->entries[ctx->stack->top]);
        printf("\n");
    }
    return 0;
}

#endif

// REPL implementation
void hex_repl(hex_context_t *ctx)
{
#if defined(__EMSCRIPTEN__)
    printf("   _*_ _\n");
    printf("  / \\hex\\*\n");
    printf(" *\\_/_/_/  v%s - WASM Build\n", HEX_VERSION);
    printf("      *\n");
    int fps = 0;
    int simulate_infinite_loop = 1;
    emscripten_set_main_loop_arg(do_repl, ctx, fps, simulate_infinite_loop);
#else

    printf("   _*_ _\n");
    printf("  / \\hex\\*\n");
    printf(" *\\_/_/_/  v%s - Press Ctrl+C to exit.\n", HEX_VERSION);
    printf("      *\n");

    while (1)
    {
        if (do_repl(ctx) != 0)
        {
            exit(1);
        }
    }
#endif
}

void hex_handle_sigint(int sig)
{
    (void)sig; // Suppress unused warning
    printf("\n");
    exit(0);
}

// Process piped input from stdin
void hex_process_stdin(hex_context_t *ctx)
{

    char buffer[8192]; // Adjust buffer size as needed
    int bytesRead = fread(buffer, 1, sizeof(buffer) - 1, stdin);
    if (bytesRead == 0)
    {
        hex_error(ctx, "[read stdin] No input provided via stdin.");
        return;
    }

    buffer[bytesRead] = '\0'; // Null-terminate the input
    hex_interpret(ctx, buffer, "<stdin>", 1, 1);
}

void hex_print_help()
{
    printf("   _*_ _\n"
           "  / \\hex\\*\n"
           " *\\_/_/_/  v%s - (c) 2024-2025 Fabio Cevasco\n"
           "      *      \n",
           HEX_VERSION);
    printf("\n"
           "USAGE\n"
           "  hex [options] [file]\n"
           "\n"
           "ARGUMENTS\n"
           "  file            A .hex or .hbx file to interpret\n"
           "\n"
           "OPTIONS\n"
           "  -b, --bytecode  Generate a .hbx bytecode file.\n"
           "  -d, --debug     Enable debug mode.\n"
           "  -h, --help      Display this help message.\n"
           "  -l, --load      Load a .hex or .hbx file before interpreting\n"
           "                  the main file or starting the REPL\n"
           "                  (can be specified multiple times).\n"
           "  -m, --manual    Display the manual.\n"
           "  -v, --version   Display hex version.\n\n");
}

void hex_print_docs(hex_doc_dictionary_t *docs)
{
    printf("\n"
           "   _*_ _\n"
           "  / \\hex\\*\n"
           " *\\_/_/_/  v%s - (c) 2024-2025 Fabio Cevasco\n"
           "      *   \n",
           HEX_VERSION);
    printf("\n"
           "BASICS\n"
           "  hex is a minimalist, slightly-esoteric, concatenative programming language that supports\n"
           "  only integers, strings, symbols, and quotations (lists).\n"
           "\n"
           "  It uses a stack-based execution model and provides 64 native symbols for stack\n"
           "  manipulation, arithmetic operations, control flow, reading and writing\n"
           "  (standard input/output/error and files), executing external processes, and more.\n"
           "\n"
           "  Symbols and literals are separated by whitespace and can be grouped in quotations using\n"
           "  parentheses.\n"
           "\n"
           "  Symbols are evaluated only when they are pushed on the stack, therefore, symbols inside\n"
           "  quotations are not evaluated until the contents of the quotation are pushed on the stack.\n"
           "  You can define your own symbols using the symbol ':' and execute a quotation with '.'.\n"
           "\n"
           "  Oh, and of course all integers are in hexadecimal format! ;)\n"
           "\n"
           "SYMBOLS\n"
           "  +---------+----------------------------+--------------------------------------------------------------------+\n"
           "  | Symbol  | Input -> Output            | Description                                                        |\n"
           "  +---------+----------------------------+--------------------------------------------------------------------+\n");
    for (size_t i = 0; i < docs->size; i++)
    {
        printf("  | ");
        hex_rpad(docs->entries[i]->name, 7);
        printf(" | ");
        hex_lpad(docs->entries[i]->input, 15);
        printf(" -> ");
        hex_rpad(docs->entries[i]->output, 7);
        printf(" | ");
        hex_rpad(docs->entries[i]->description, 66);
        printf(" |\n");
    }
    printf("  +---------+----------------------------+--------------------------------------------------------------------+\n");
}

int hex_write_bytecode_file(hex_context_t *ctx, char *filename, uint8_t *bytecode, size_t size)
{
    FILE *file = fopen(filename, "wb");
    if (file == NULL)
    {
        hex_error(ctx, "[write bytecode] Failed to write file: %s", filename);
        return 1;
    }
    hex_debug(ctx, "Writing bytecode to file: %s", filename);
    uint8_t header[8];
    hex_header(ctx, header);
    fwrite(header, 1, sizeof(header), file);
    uint8_t *symbol_table = NULL;
    size_t symbol_table_size = 0;
    symbol_table = hex_encode_bytecode_symboltable(ctx, &symbol_table_size);
    fwrite(symbol_table, 1, symbol_table_size, file);
    free(symbol_table);
    fwrite(bytecode, 1, size, file);
    fclose(file);
    hex_debug(ctx, "Bytecode file written: %s", filename);
    return 0;
}

int hex_interpret_file(hex_context_t *ctx, const char *file)
{
    int result = 0;
    if (strstr(file, ".hbx") != NULL)
    {
        FILE *bytecode_file = fopen(file, "rb");
        if (bytecode_file == NULL)
        {
            hex_error(ctx, "[open hbx file] Failed to open bytecode file: %s", file);
            return 1;
        }
        fseek(bytecode_file, 0, SEEK_END);
        size_t bytecode_size = ftell(bytecode_file);
        fseek(bytecode_file, 0, SEEK_SET);
        uint8_t *bytecode = (uint8_t *)malloc(bytecode_size);
        if (bytecode == NULL)
        {
            hex_error(ctx, "[read hbx file] Memory allocation failed");
            fclose(bytecode_file);
            return 1;
        }
        fread(bytecode, 1, bytecode_size, bytecode_file);
        fclose(bytecode_file);
        result = hex_interpret_bytecode(ctx, bytecode, bytecode_size, file);
        free(bytecode);
    }
    else
    {
        char *fileContent = hex_read_file(ctx, file);
        if (fileContent == NULL)
        {
            return 1;
        }
        result = hex_interpret(ctx, fileContent, file, 1 + ctx->hashbang, 1);
    }
    return result;
}

////////////////////////////////////////
// Main Program                       //
////////////////////////////////////////

int main(int argc, char *argv[])
{
    // Register SIGINT (Ctrl+C) signal handler
    signal(SIGINT, hex_handle_sigint);

    // Initialize the context
    hex_context_t *ctx = hex_init();
    ctx->argc = argc;
    ctx->argv = argv;

    hex_register_symbols(ctx);
    hex_create_docs(ctx->docs);

    char *file = NULL;
    int generate_bytecode = 0;

    if (argc > 1)
    {
        for (int i = 1; i < argc; i++)
        {
            char *arg = strdup(argv[i]);
            if ((strcmp(arg, "-v") == 0 || strcmp(arg, "--version") == 0))
            {
                printf("%s\n", HEX_VERSION);
                hex_destroy(ctx);
                return 0;
            }
            else if ((strcmp(arg, "-h") == 0 || strcmp(arg, "--help") == 0))
            {
                hex_print_help();
                hex_destroy(ctx);
                return 0;
            }
            else if ((strcmp(arg, "-m") == 0 || strcmp(arg, "--manual") == 0))
            {
                hex_print_docs(ctx->docs);
                hex_destroy(ctx);
                return 0;
            }
            else if ((strcmp(arg, "-d") == 0 || strcmp(arg, "--debug") == 0))
            {
                ctx->settings->debugging_enabled = 1;
                printf("*** Debug mode enabled ***\n");
            }
            else if ((strcmp(arg, "-b") == 0 || strcmp(arg, "--bytecode") == 0))
            {
                generate_bytecode = 1;
            }
            else if ((strcmp(arg, "-l") == 0 || strcmp(arg, "--load") == 0))
            {
                // interpret a library file (e.g. hexlib.hex or hexlib.hbx)
                // before executing the main file
                i++;
                if (i >= argc)
                {
                    hex_error(ctx, "[load] No file specified");
                    hex_destroy(ctx);
                    return 1;
                }
                char *libfile = strdup(argv[i]);
                int load_result = hex_interpret_file(ctx, libfile);
                free(libfile);
                if (load_result != 0)
                {
                    hex_destroy(ctx);
                    return load_result;
                }
            }
            else
            {
                if (!file)
                {
                    file = arg;
                }
                // Ignore extra arguments
            }
        }
        if (file)
        {
            if (generate_bytecode)
            {
                uint8_t *bytecode;
                size_t bytecode_size = 0;
                hex_file_position_t position;
                position.column = 1;
                position.line = 1 + ctx->hashbang;
                position.filename = file;
                char *bytecode_file = strdup(file);
                char *ext = strrchr(bytecode_file, '.');
                char *fileContent = hex_read_file(ctx, file);
                if (ext != NULL)
                {
                    strcpy(ext, ".hbx");
                }
                else
                {
                    strcat(bytecode_file, ".hbx");
                }
                if (hex_bytecode(ctx, fileContent, &bytecode, &bytecode_size, &position) != 0)
                {
                    hex_error(ctx, "[generate bytecode] Failed to generate bytecode");
                    free(fileContent);
                    free(bytecode_file);
                    hex_destroy(ctx);
                    return 1;
                }
                if (hex_write_bytecode_file(ctx, bytecode_file, bytecode, bytecode_size) != 0)
                {
                    free(fileContent);
                    free(bytecode_file);
                    free(bytecode);
                    hex_destroy(ctx);
                    return 1;
                }
                free(fileContent);
                free(bytecode_file);
                free(bytecode);
                hex_destroy(ctx);
                return 0;
            }
            else
            {
                int result = hex_interpret_file(ctx, file);
                hex_destroy(ctx);
                return result;
            }
        }
    }
#if !(__EMSCRIPTEN__)
    if (!isatty(fileno(stdin)))
    {
        // Process piped input from stdin
        hex_process_stdin(ctx);
    }
#endif
    else
    {
        // Start REPL
        hex_repl(ctx);
    }

    hex_destroy(ctx);
    return 0;
}


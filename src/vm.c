#ifndef HEX_H
#include "hex.h"
#endif

////////////////////////////////////////
// Virtual Machine                    //
////////////////////////////////////////

uint8_t HEX_BYTECODE_HEADER[6] = {0x01, 0x48, 0x45, 0x78, 0x01, 0x02};

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
    HEX_OP_FREE = 0x11,

    HEX_OP_IF = 0x12,
    HEX_OP_WHEN = 0x13,
    HEX_OP_WHILE = 0x14,
    HEX_OP_ERROR = 0x15,
    HEX_OP_TRY = 0x16,

    HEX_OP_DUP = 0x17,
    HEX_OP_STACK = 0x18,
    HEX_OP_CLEAR = 0x19,
    HEX_OP_POP = 0x1A,
    HEX_OP_SWAP = 0x1B,

    HEX_OP_I = 0x1C,
    HEX_OP_EVAL = 0x1D,
    HEX_OP_QUOTE = 0x1E,

    HEX_OP_ADD = 0x1F,
    HEX_OP_SUB = 0x20,
    HEX_OP_MUL = 0x21,
    HEX_OP_DIV = 0x22,
    HEX_OP_MOD = 0x23,

    HEX_OP_BITAND = 0x24,
    HEX_OP_BITOR = 0x25,
    HEX_OP_BITXOR = 0x26,
    HEX_OP_BITNOT = 0x27,
    HEX_OP_SHL = 0x28,
    HEX_OP_SHR = 0x29,

    HEX_OP_EQUAL = 0x2A,
    HEX_OP_NOTEQUAL = 0x2B,
    HEX_OP_GREATER = 0x2C,
    HEX_OP_LESS = 0x2D,
    HEX_OP_GREATEREQUAL = 0x2E,
    HEX_OP_LESSEQUAL = 0x2F,

    HEX_OP_AND = 0x30,
    HEX_OP_OR = 0x31,
    HEX_OP_NOT = 0x32,
    HEX_OP_XOR = 0x33,

    HEX_OP_INT = 0x34,
    HEX_OP_STR = 0x35,
    HEX_OP_DEC = 0x36,
    HEX_OP_HEX = 0x37,
    HEX_OP_ORD = 0x38,
    HEX_OP_CHR = 0x39,
    HEX_OP_TYPE = 0x3A,

    HEX_OP_CAT = 0x3B,
    HEX_OP_LEN = 0x3C,
    HEX_OP_GET = 0x3D,
    HEX_OP_INDEX = 0x3E,
    HEX_OP_JOIN = 0x3F,

    HEX_OP_SPLIT = 0x40,
    HEX_OP_REPLACE = 0x41,

    HEX_OP_EACH = 0x42,
    HEX_OP_MAP = 0x43,
    HEX_OP_FILTER = 0x44,

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

static void encode_length(uint8_t **bytecode, size_t *size, size_t *capacity, size_t length)
{
    if (length < 0x80)
    {
        (*bytecode)[(*size)++] = (uint8_t)length;
    }
    else if (length < 0x8000)
    {
        (*bytecode)[(*size)++] = 0x80;
        if (*size + 2 > *capacity)
        {
            *capacity *= 2;
            *bytecode = (uint8_t *)realloc(*bytecode, *capacity);
        }
        (*bytecode)[(*size)++] = (uint8_t)(length >> 8);
        (*bytecode)[(*size)++] = (uint8_t)(length & 0xFF);
    }
    else
    {
        (*bytecode)[(*size)++] = 0x81;
        if (*size + 4 > *capacity)
        {
            *capacity *= 2;
            *bytecode = (uint8_t *)realloc(*bytecode, *capacity);
        }
        (*bytecode)[(*size)++] = (uint8_t)(length >> 24);
        (*bytecode)[(*size)++] = (uint8_t)((length >> 16) & 0xFF);
        (*bytecode)[(*size)++] = (uint8_t)((length >> 8) & 0xFF);
        (*bytecode)[(*size)++] = (uint8_t)(length & 0xFF);
    }
}

int hex_bytecode(hex_context_t *ctx, const char *input, uint8_t **output, size_t *output_size, const char *filename, int line, int column)
{
    hex_file_position_t position = {filename, line, column};
    hex_token_t *token;
    size_t capacity = 128;
    size_t size = 0;
    uint8_t *bytecode = (uint8_t *)malloc(capacity);
    if (!bytecode)
    {
        hex_error(ctx, "Memory allocation failed");
        return 1;
    }

    while ((token = hex_next_token(ctx, &input, &position)) != NULL)
    {
        if (size >= capacity)
        {
            capacity *= 2;
            bytecode = (uint8_t *)realloc(bytecode, capacity);
            if (!bytecode)
            {
                hex_error(ctx, "Memory allocation failed");
                return 1;
            }
        }

        switch (token->type)
        {
        case HEX_TOKEN_INTEGER:
            bytecode[size++] = HEX_OP_PUSHIN;
            int32_t value = hex_parse_integer(token->value);
            encode_length(&bytecode, &size, &capacity, sizeof(int32_t));
            memcpy(&bytecode[size], &value, sizeof(int32_t));
            size += sizeof(int32_t);
            break;

        case HEX_TOKEN_STRING:
            bytecode[size++] = HEX_OP_PUSHST;
            size_t len = strlen(token->value);
            encode_length(&bytecode, &size, &capacity, len);
            memcpy(&bytecode[size], token->value, len);
            size += len;
            break;
        case HEX_TOKEN_SYMBOL:
            if (hex_valid_native_symbol(ctx, token->value))
            {
                size_t sym_len = strlen(token->value);
                encode_length(&bytecode, &size, &capacity, sym_len);
                memcpy(&bytecode[size], token->value, sym_len);
                size += sym_len;
            }
            else
            {
                hex_error(ctx, "(%d,%d) Invalid symbol: %s", position.line, position.column, token->value);
                hex_free_token(token);
                free(bytecode);
                return 1;
            }
            break;
        case HEX_TOKEN_QUOTATION_START:
        {
            bytecode[size++] = HEX_OP_PUSHQT;
            hex_item_t quotation;
            if (hex_parse_quotation(ctx, &input, &quotation, &position) != 0)
            {
                hex_free_token(token);
                free(bytecode);
                return 1;
            }
            // Recursively translate quotation to opcodes
            uint8_t *quotation_bytecode;
            size_t quotation_size;
            if (hex_bytecode(ctx, quotation.data.quotation_value, &quotation_bytecode, &quotation_size, filename, line, column) != 0)
            {
                hex_free_token(token);
                free(bytecode);
                return 1;
            }
            encode_length(&bytecode, &size, &capacity, quotation_size);
            memcpy(&bytecode[size], quotation_bytecode, quotation_size);
            size += quotation_size;
            free(quotation_bytecode);
            break;
        }
        default:
            hex_error(ctx, "(%d,%d) Unexpected token: %s", position.line, position.column, token->value);
            hex_free_token(token);
            free(bytecode);
            return 1;
        }

        hex_free_token(token);
    }

    *output = bytecode;
    *output_size = size;
    return 0;
}

#ifndef HEX_H
#include "hex.h"
#endif

////////////////////////////////////////
// Virtual Machine                    //
////////////////////////////////////////

static void encode_length(uint8_t **bytecode, size_t *size, size_t length)
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

uint8_t hex_symbol_to_opcode(const char *symbol)
{
    // Native Symbols
    if (strcmp(symbol, ":") == 0)
    {
        return HEX_OP_STORE;
    }
    else if (strcmp(symbol, "#") == 0)
    {
        return HEX_OP_FREE;
    }
    else if (strcmp(symbol, "if") == 0)
    {
        return HEX_OP_IF;
    }
    else if (strcmp(symbol, "when") == 0)
    {
        return HEX_OP_WHEN;
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
    else if (strcmp(symbol, "dup") == 0)
    {
        return HEX_OP_DUP;
    }
    else if (strcmp(symbol, "stack") == 0)
    {
        return HEX_OP_STACK;
    }
    else if (strcmp(symbol, "clear") == 0)
    {
        return HEX_OP_CLEAR;
    }
    else if (strcmp(symbol, "pop") == 0)
    {
        return HEX_OP_POP;
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
        return HEX_OP_SUB;
    }
    else if (strcmp(symbol, "*") == 0)
    {
        return HEX_OP_MUL;
    }
    else if (strcmp(symbol, "/") == 0)
    {
        return HEX_OP_DIV;
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
    else if (strcmp(symbol, "replace") == 0)
    {
        return HEX_OP_REPLACE;
    }
    else if (strcmp(symbol, "each") == 0)
    {
        return HEX_OP_EACH;
    }
    else if (strcmp(symbol, "map") == 0)
    {
        return HEX_OP_MAP;
    }
    else if (strcmp(symbol, "filter") == 0)
    {
        return HEX_OP_FILTER;
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
    return 0;
}

const char *hex_opcode_to_symbol(uint8_t opcode)
{
    switch (opcode)
    {
    case HEX_OP_STORE:
        return ":";
    case HEX_OP_FREE:
        return "#";
    case HEX_OP_IF:
        return "if";
    case HEX_OP_WHEN:
        return "when";
    case HEX_OP_WHILE:
        return "while";
    case HEX_OP_ERROR:
        return "error";
    case HEX_OP_TRY:
        return "try";
    case HEX_OP_DUP:
        return "dup";
    case HEX_OP_STACK:
        return "stack";
    case HEX_OP_CLEAR:
        return "clear";
    case HEX_OP_POP:
        return "pop";
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
    case HEX_OP_SUB:
        return "-";
    case HEX_OP_MUL:
        return "*";
    case HEX_OP_DIV:
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
    case HEX_OP_REPLACE:
        return "replace";
    case HEX_OP_EACH:
        return "each";
    case HEX_OP_MAP:
        return "map";
    case HEX_OP_FILTER:
        return "filter";
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
    default:
        return NULL;
    }
}

int hex_bytecode_integer(hex_context_t *ctx, uint8_t **bytecode, size_t *size, size_t *capacity, int32_t value)
{
    hex_debug(ctx, "PUSHIN[%d]: %d", sizeof(int32_t), value);
    // Check if we need to resize the buffer (size + int32_t size + opcode (1) + max encoded length (4))
    if (*size + sizeof(int32_t) + 1 + 4 > *capacity)
    {
        *capacity = (*size + sizeof(int32_t) + 1 + 4 + *capacity) * 2;
        uint8_t *new_bytecode = (uint8_t *)realloc(*bytecode, *capacity);
        if (!new_bytecode)
        {
            hex_error(ctx, "Memory allocation failed");
            return 1;
        }
        *bytecode = new_bytecode;
    }
    (*bytecode)[*size] = HEX_OP_PUSHIN;
    *size += 1; // opcode
    // Encode the length of the integer value
    size_t int_length = 0;
    if (value >= -0x80 && value < 0x80)
    {
        int_length = 1;
    }
    else if (value >= -0x8000 && value < 0x8000)
    {
        int_length = 2;
    }
    else if (value >= -0x800000 && value < 0x800000)
    {
        int_length = 3;
    }
    else
    {
        int_length = 4;
    }
    encode_length(bytecode, size, int_length);
    // Encode the integer value in the minimum number of bytes, in little endian
    if (value >= -0x80 && value < 0x80)
    {
        (*bytecode)[*size] = value & 0xFF;
        *size += 1;
    }
    else if (value >= -0x8000 && value < 0x8000)
    {
        (*bytecode)[*size] = value & 0xFF;
        (*bytecode)[*size + 1] = (value >> 8) & 0xFF;
        *size += 2;
    }
    else if (value >= -0x800000 && value < 0x800000)
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
    size_t len = strlen(value);
    hex_debug(ctx, "PUSHST[%d]: (total size start: %d) %s", len, *size, value);
    // Check if we need to resize the buffer (size + strlen + opcode (1) + max encoded length (4))
    if (*size + len + 1 + 4 > *capacity)
    {
        *capacity = (*size + len + 1 + 4 + *capacity) * 2;
        uint8_t *new_bytecode = (uint8_t *)realloc(*bytecode, *capacity);
        if (!new_bytecode)
        {
            hex_error(ctx, "Memory allocation failed");
            return 1;
        }
        *bytecode = new_bytecode;
    }
    (*bytecode)[*size] = HEX_OP_PUSHST;
    *size += 1; // opcode
    encode_length(bytecode, size, len);
    memcpy(&(*bytecode)[*size], value, len);
    *size += len;
    hex_debug(ctx, "PUSHST[%d]: (total size: %d) %s", len, *size, value);
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
                hex_error(ctx, "Memory allocation failed");
                return 1;
            }
            *bytecode = new_bytecode;
        }
        (*bytecode)[*size] = hex_symbol_to_opcode(value);
        *size += 1; // opcode
        hex_debug(ctx, "NATSYM[1]: (total size: %d) %s", *size, value);
    }
    else
    {
        hex_debug(ctx, "LOOKUP[%d]: %s", strlen(value), value);
        // Check if we need to resize the buffer (size + strlen + opcode (1) + max encoded length (4))
        if (*size + strlen(value) + 1 + 4 > *capacity)
        {
            *capacity = (*size + strlen(value) + 1 + 4) * 2;
            uint8_t *new_bytecode = (uint8_t *)realloc(*bytecode, *capacity);
            if (!new_bytecode)
            {
                hex_error(ctx, "Memory allocation failed");
                return 1;
            }
            *bytecode = new_bytecode;
        }
        (*bytecode)[*size] = HEX_OP_LOOKUP;
        *size += 1; // opcode
        encode_length(bytecode, size, strlen(value));
        memcpy(&(*bytecode)[*size], value, strlen(value));
        *size += strlen(value);
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
    encode_length(bytecode, size, *n_items);
    memcpy(&(*bytecode)[*size], *output, *output_size);
    *size += *output_size;
    hex_debug(ctx, "PUSHQT[%d]: (total size: %d) <end>", *n_items, *output_size);
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
        hex_error(ctx, "Memory allocation failed");
        return 1;
    }
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
            hex_debug(ctx, "PUSHQT[-]: <start>");
            if (hex_generate_quotation_bytecode(ctx, &input, &quotation_bytecode, &quotation_size, &n_items, position) != 0)
            {
                hex_error(ctx, "Failed to generate quotation bytecode (main)");
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
        hex_error(ctx, "Memory allocation failed");
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
            hex_debug(ctx, "PUSHQT[-]: <start>");
            if (hex_generate_quotation_bytecode(ctx, input, &quotation_bytecode, &quotation_size, &n_items, position) != 0)
            {
                hex_error(ctx, "Failed to generate quotation bytecode");
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
            hex_error(ctx, "Bytecode size too small to contain an integer length");
            return 1;
        }
        length |= ((**bytecode & 0x7F) << shift);
        shift += 7;
    } while (*(*bytecode)++ & 0x80);

    *size -= shift / 7;

    if (*size < length)
    {
        hex_error(ctx, "Bytecode size too small to contain the integer value");
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

    hex_debug(ctx, "PUSHIN[%zu]: %d", length, value);
    hex_item_t item = hex_integer_item(ctx, value);
    *result = item;
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
            hex_error(ctx, "Bytecode size too small to contain a string length");
            return 1;
        }
        length |= ((**bytecode & 0x7F) << shift);
        shift += 7;
        (*bytecode)++;
        (*size)--;
    } while (**bytecode & 0x80);

    if (*size < length)
    {
        hex_error(ctx, "Bytecode size too small to contain the string");
        return 1;
    }

    char *value = (char *)malloc(length + 1);
    if (!value)
    {
        hex_error(ctx, "Memory allocation failed");
        return 1;
    }
    memcpy(value, *bytecode, length);
    value[length] = '\0';
    *bytecode += length;
    *size -= length;

    hex_item_t item = hex_string_item(ctx, value);
    *result = item;
    hex_debug(ctx, "PUSHST[%zu]: %s", length, value);
    free(value);
    return 0;
}

int hex_interpret_bytecode_native_symbol(hex_context_t *ctx, uint8_t opcode, size_t position, hex_item_t *result)
{

    const char *symbol = hex_opcode_to_symbol(opcode);
    if (!symbol)
    {
        hex_error(ctx, "Invalid opcode for symbol");
        return 1;
    }

    hex_item_t item;
    item.type = HEX_TYPE_NATIVE_SYMBOL;
    hex_item_t value;
    hex_token_t *token = (hex_token_t *)malloc(sizeof(hex_token_t));
    token->value = (char *)symbol;
    token->position.line = 0;
    token->position.column = position;
    if (hex_get_symbol(ctx, token->value, &value))
    {
        item.token = token;
        item.type = HEX_TYPE_NATIVE_SYMBOL;
        item.data.fn_value = value.data.fn_value;
    }
    else
    {
        hex_error(ctx, "(%d,%d) Unable to reference native symbol: %s (bytecode)", token->position.line, token->position.column, token->value);
        hex_free_token(token);
        return 1;
    }
    hex_debug(ctx, "NATSYM[1]: %d (%s)", opcode, symbol);
    *result = item;
    return 0;
}

int hex_interpret_bytecode_user_symbol(hex_context_t *ctx, uint8_t **bytecode, size_t *size, size_t position, hex_item_t *result)
{
    size_t length = 0;
    int shift = 0;

    // Decode the variable-length integer for the symbol length
    do
    {
        if (*size == 0)
        {
            hex_error(ctx, "Bytecode size too small to contain a symbol length");
            return 1;
        }
        length |= ((**bytecode & 0x7F) << shift);
        shift += 7;
        (*bytecode)++;
        (*size)--;
    } while (**bytecode & 0x80);

    if (*size < length)
    {
        hex_error(ctx, "Bytecode size too small to contain the symbol");
        return 1;
    }

    char *value = (char *)malloc(length + 1);
    if (!value)
    {
        hex_error(ctx, "Memory allocation failed");
        return 1;
    }
    memcpy(value, *bytecode, length);
    value[length] = '\0';
    *bytecode += length;
    *size -= length;

    hex_token_t *token = (hex_token_t *)malloc(sizeof(hex_token_t));
    token->value = (char *)malloc(length + 1);
    strncpy(token->value, value, length + 1);
    token->position.line = 0;
    token->position.column = position;

    hex_item_t item;
    item.type = HEX_TYPE_USER_SYMBOL;
    item.token = token;

    hex_debug(ctx, "LOOKUP[%zu]: %s", length, value);
    free(value);
    *result = item;
    return 0;
}

int hex_interpret_bytecode_quotation(hex_context_t *ctx, uint8_t **bytecode, size_t *size, size_t position, hex_item_t *result)
{
    size_t n_items = 0;
    int shift = 0;

    // Decode the variable-length integer for the number of items
    do
    {
        if (*size == 0)
        {
            hex_error(ctx, "Bytecode size too small to contain a quotation length");
            return 1;
        }
        n_items |= ((**bytecode & 0x7F) << shift);
        shift += 7;
        (*bytecode)++;
        (*size)--;
    } while (**bytecode & 0x80);

    hex_debug(ctx, "PUSHQT[%zu]: <start>", n_items);

    hex_item_t **items = (hex_item_t **)malloc(n_items * sizeof(hex_item_t));
    if (!items)
    {
        hex_error(ctx, "Memory allocation failed");
        return 1;
    }

    for (size_t i = 0; i < n_items; i++)
    {
        uint8_t opcode = **bytecode;
        (*bytecode)++;
        (*size)--;

        hex_item_t *item = (hex_item_t *)malloc(sizeof(hex_item_t));
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
            if (hex_interpret_bytecode_user_symbol(ctx, bytecode, size, position, item) != 0)
            {
                hex_free_list(ctx, items, n_items);
                return 1;
            }
            break;
        case HEX_OP_PUSHQT:
            if (hex_interpret_bytecode_quotation(ctx, bytecode, size, position, item) != 0)
            {
                hex_free_list(ctx, items, n_items);
                return 1;
            }
            break;
        default:
            if (hex_interpret_bytecode_native_symbol(ctx, opcode, *size, item) != 0)
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

    hex_debug(ctx, "PUSHQT[%zu]: <end>", n_items);
    return 0;
}

int hex_interpret_bytecode(hex_context_t *ctx, uint8_t *bytecode, size_t size)
{
    size_t bytecode_size = size;
    size_t position = bytecode_size;
    if (size < 6 || memcmp(bytecode, HEX_BYTECODE_HEADER, 6) != 0)
    {
        hex_error(ctx, "Invalid or missing bytecode header");
        return 1;
    }
    bytecode += 6;
    size -= 6;
    while (size > 0)
    {
        position = bytecode_size - size;
        uint8_t opcode = *bytecode;
        hex_debug(ctx, "Bytecode Position: %zu - opcode: %u", position, opcode);
        bytecode++;
        size--;

        hex_item_t *item = (hex_item_t *)malloc(sizeof(hex_item_t));
        switch (opcode)
        {
        case HEX_OP_PUSHIN:
            if (hex_interpret_bytecode_integer(ctx, &bytecode, &size, item) != 0)
            {
                HEX_FREE(ctx, *item);
                return 1;
            }
            break;
        case HEX_OP_PUSHST:
            if (hex_interpret_bytecode_string(ctx, &bytecode, &size, item) != 0)
            {
                HEX_FREE(ctx, *item);
                return 1;
            }
            break;
        case HEX_OP_LOOKUP:
            if (hex_interpret_bytecode_user_symbol(ctx, &bytecode, &size, position, item) != 0)
            {
                HEX_FREE(ctx, *item);
                return 1;
            }
            break;
        case HEX_OP_PUSHQT:

            if (hex_interpret_bytecode_quotation(ctx, &bytecode, &size, position, item) != 0)
            {
                HEX_FREE(ctx, *item);
                return 1;
            }
            break;
        default:
            if (hex_interpret_bytecode_native_symbol(ctx, opcode, position, item) != 0)
            {
                HEX_FREE(ctx, *item);
                return 1;
            }
            break;
        }
        if (hex_push(ctx, *item) != 0)
        {
            HEX_FREE(ctx, *item);
            return 1;
        }
    }
    return 0;
}

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

int hex_bytecode_integer(hex_context_t *ctx, uint8_t **bytecode, size_t *size, size_t *capacity, int32_t value)
{
    hex_debug(ctx, "PUSHIN: %d", value);
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
    hex_debug(ctx, "PUSHST: \"%s\"", hex_process_string(ctx, value));
    size_t len = strlen(value);
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
    return 0;
}

int hex_bytecode_symbol(hex_context_t *ctx, uint8_t **bytecode, size_t *size, size_t *capacity, const char *value)
{
    if (hex_valid_native_symbol(ctx, value))
    {
        hex_debug(ctx, "NATSYM: %s", value);
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
    }
    else
    {
        // Add to symbol table
        hex_symboltable_set(ctx, value);
        int index = hex_symboltable_get_index(ctx, value);
        hex_debug(ctx, "LOOKUP: #%d -> %s", index, value);
        //  Check if we need to resize the buffer (size + 1 opcode + 2 max index)
        if (*size + 1 + 2 > *capacity)
        {
            *capacity = (*size + 1 + 2) * 2;
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
    encode_length(bytecode, size, *n_items);
    memcpy(&(*bytecode)[*size], *output, *output_size);
    *size += *output_size;
    hex_debug(ctx, "PUSHQT: <end> (items: %d)", *n_items);
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
            hex_debug(ctx, "PUSHQT: <start>");
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
            hex_debug(ctx, "PUSHQT: <start>");
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
    // Get the index of the symbol (one byte)
    if (*size == 0)
    {
        hex_error(ctx, "Bytecode size too small to contain a symbol length");
        return 1;
    }
    size_t index = **bytecode;
    (*bytecode)++;
    (*size)--;

    if (index >= ctx->symbol_table.count)
    {
        hex_error(ctx, "Symbol index out of bounds");
        return 1;
    }
    char *value = hex_symboltable_get_value(ctx, index);
    size_t length = strlen(value);

    if (!value)
    {
        hex_error(ctx, "Memory allocation failed");
        return 1;
    }

    *bytecode += 1;
    *size -= 1;

    hex_token_t *token = (hex_token_t *)malloc(sizeof(hex_token_t));

    token->value = (char *)malloc(length + 1);
    strncpy(token->value, value, length + 1);
    token->position.line = 0;
    token->position.column = position;

    hex_item_t item;
    item.type = HEX_TYPE_USER_SYMBOL;
    item.token = token;

    hex_debug(ctx, "LOOKUP[%zu]: %s", length, value);
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
    uint8_t header[8];
    if (size < 8)
    {
        hex_error(ctx, "Bytecode size too small to contain a header");
        return 1;
    }
    memcpy(header, bytecode, 8);
    int symbol_table_size = hex_validate_header(header);
    hex_debug(ctx, "hex executable file - version: %d - symbols: %d", header[4], symbol_table_size);
    if (symbol_table_size < 0)
    {
        hex_error(ctx, "Invalid bytecode header");
        return 1;
    }
    bytecode += 8;
    size -= 8;
    // Extract the symbol table
    if (symbol_table_size > 0)
    {
        if (hex_decode_bytecode_symboltable(ctx, &bytecode, &size, symbol_table_size) != 0)
        {
            hex_error(ctx, "Failed to decode the symbol table");
            return 1;
        }
    }
    // Debug: Print all symbols in the symbol table
    hex_debug(ctx, "Symbol Table:");
    for (size_t i = 0; i < ctx->symbol_table.count; i++)
    {
        hex_debug(ctx, "Symbol %zu: %s", i, ctx->symbol_table.symbols[i]);
    }
    while (size > 0)
    {
        position = bytecode_size - size;
        uint8_t opcode = *bytecode;
        hex_debug(ctx, "Bytecode Position: %zu - opcode: %02X", position, opcode);
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

void hex_header(hex_context_t *ctx, uint8_t header[8])
{
    header[0] = 0x01;
    header[1] = 'h';
    header[2] = 'e';
    header[3] = 'x';
    header[4] = 0x01; // version
    uint16_t symbol_table_size = (uint16_t)ctx->symbol_table.count;
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

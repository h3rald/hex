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

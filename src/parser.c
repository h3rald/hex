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

    hex_token_t *token = (hex_token_t *)malloc(sizeof(hex_token_t));
    if (!token)
        return NULL;

    token->value = NULL;
    token->type = HEX_TOKEN_INVALID;
    token->quotation_size = 0;
    token->position = (hex_file_position_t *)malloc(sizeof(hex_file_position_t));
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

    quotation = (hex_item_t **)malloc(capacity * sizeof(hex_item_t *));
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
                item = malloc(sizeof(hex_item_t));
                if (item)
                {
                    item->type = HEX_TYPE_NATIVE_SYMBOL;
                    hex_item_t *value = malloc(sizeof(hex_item_t));
                    if (hex_get_symbol(ctx, token->value, value))
                    {
                        item->token = token;
                        item->type = HEX_TYPE_NATIVE_SYMBOL;
                        item->data.fn_value = value->data.fn_value;
                        free(value); // Free the temporary value holder
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
                item = malloc(sizeof(hex_item_t));
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
            item = malloc(sizeof(hex_item_t));
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

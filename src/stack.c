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
        return;
    free(token->value);
    token->value = NULL;
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
    hex_debug_item(ctx, "PUSH", item);
    int result = 0;
    if (item->type == HEX_TYPE_USER_SYMBOL)
    {
        hex_item_t *value = malloc(sizeof(hex_item_t));
        if (value == NULL)
        {
            hex_error(ctx, "[push] Failed to allocate memory for value");
            return 1;
        }
        if (hex_get_symbol(ctx, item->token->value, value))
        {
            if (value->type == HEX_TYPE_QUOTATION && value->operator)
            {
                add_to_stack_trace(ctx, item->token);
                for (size_t i = 0; i < value->quotation_size; i++)
                {
                    if (hex_push(ctx, value->data.quotation_value[i]) != 0)
                    {
                        HEX_FREE(ctx, value);
                        hex_debug_item(ctx, "FAIL", item);
                        return 1;
                    }
                }
            }
            else
            {
                result = hex_push(ctx, value);
            }
        }
        else
        {
            hex_error(ctx, "[push] Undefined user symbol: %s", item->token->value);
            HEX_FREE(ctx, value);
            result = 1;
        }
    }
    else if (item->type == HEX_TYPE_NATIVE_SYMBOL)
    {
        hex_item_t *value = malloc(sizeof(hex_item_t));
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
            HEX_FREE(ctx, value);
            result = 1;
        }
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
    hex_item_t *item = malloc(sizeof(hex_item_t));
    if (item == NULL)
    {
        hex_error(ctx, "[create string] Failed to allocate memory for item");
        free(str);
        return NULL;
    }
    item->type = HEX_TYPE_STRING;
    item->data.str_value = str;
    return item;
}

hex_item_t *hex_integer_item(hex_context_t *ctx, int value)
{
    (void)(ctx);
    hex_item_t *item = malloc(sizeof(hex_item_t));
    if (item == NULL)
    {
        hex_error(ctx, "[create integer] Failed to allocate memory for item");
        return NULL;
    }
    item->type = HEX_TYPE_INTEGER;
    item->data.int_value = value;
    return item;
}

hex_item_t *hex_quotation_item(hex_context_t *ctx, hex_item_t **quotation, size_t size)
{
    hex_item_t *item = malloc(sizeof(hex_item_t));
    if (item == NULL)
    {
        hex_error(ctx, "[create quotation] Failed to allocate memory for item");
        return NULL;
    }
    item->operator= 0;
    item->type = HEX_TYPE_QUOTATION;
    item->data.quotation_value = quotation;
    item->quotation_size = size;
    return item;
}

hex_item_t *hex_symbol_item(hex_context_t *ctx, hex_token_t *token)
{
    hex_item_t *item = malloc(sizeof(hex_item_t));
    if (item == NULL)
    {
        hex_error(ctx, "[create symbol] Failed to allocate memory for item");
        return NULL;
    }
    item->type = hex_valid_native_symbol(ctx, token->value) ? HEX_TYPE_NATIVE_SYMBOL : HEX_TYPE_USER_SYMBOL;
    item->token = token;
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
    hex_item_t *item = malloc(sizeof(hex_item_t));
    if (item == NULL)
    {
        hex_error(ctx, "[pop] Failed to allocate memory for item");
        return NULL;
    }
    if (ctx->stack->top < 0)
    {
        hex_error(ctx, "[pop] Insufficient items on the stack");
        item->type = HEX_TYPE_INVALID;
        return item;
    }
    *item = *ctx->stack->entries[ctx->stack->top];
    hex_debug_item(ctx, " POP", item);
    ctx->stack->top--;
    return item;
}

void hex_free_list(hex_context_t *ctx, hex_item_t **quotation, size_t size)
{
    if (!quotation)
        return;

    for (size_t i = 0; i < size; i++)
    {
        if (quotation[i])
        {
            hex_free_item(ctx, quotation[i]); // Free each item
        }
    }
}

void hex_free_item(hex_context_t *ctx, hex_item_t *item)
{
    if (item == NULL)
        return;

    hex_debug_item(ctx, "FREE", item);

    switch (item->type)
    {
    case HEX_TYPE_STRING:
        if (item->data.str_value)
        {
            free(item->data.str_value);
            item->data.str_value = NULL; // Prevent double free
        }
        break;

    case HEX_TYPE_QUOTATION:
        if (item->data.quotation_value)
        {
            hex_free_list(ctx, item->data.quotation_value, item->quotation_size);
            // free(item->data.quotation_value);
            item->data.quotation_value = NULL; // Prevent double free
        }
        break;

    case HEX_TYPE_NATIVE_SYMBOL:
    case HEX_TYPE_USER_SYMBOL:
        if (item->token)
        {
            hex_free_token(item->token);
            item->token = NULL; // Prevent double free
        }
        break;

    default:
        break;
    }

    free(item); // Free the item itself
    item = NULL;
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
    copy->quotation_size = token->quotation_size;

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

        copy->position->line = token->position->line;
        copy->position->column = token->position->column;
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
    hex_item_t *copy = (hex_item_t *)malloc(sizeof(hex_item_t));
    if (!copy)
    {
        hex_error(ctx, "[copy item] Failed to allocate memory for item copy");
        return NULL;
    }

    // Copy basic fields
    copy->type = item->type;
    copy->operator= item->operator;
    copy->quotation_size = item->quotation_size;

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
        if (item->data.quotation_value && item->quotation_size > 0)
        {
            copy->data.quotation_value = (hex_item_t **)malloc(item->quotation_size * sizeof(hex_item_t *));
            if (!copy->data.quotation_value)
            {
                hex_free_item(ctx, copy);
                hex_error(ctx, "[copy item] Failed to allocate memory for quotation array");
                return NULL;
            }

            for (size_t i = 0; i < item->quotation_size; ++i)
            {
                copy->data.quotation_value[i] = hex_copy_item(ctx, item->data.quotation_value[i]); // Recursively copy each item
                if (!copy->data.quotation_value[i])
                {
                    // Cleanup on failure
                    hex_free_list(ctx, copy->data.quotation_value, i); // Free copied items
                    hex_free_item(ctx, copy);
                    hex_error(ctx, "[copy item] Failed to copy quotation item");
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
        // User symbols do not have a function pointer, so no additional copying here
        break;

    default:
        // Unsupported type
        hex_error(ctx, "[copy item] Unsupported item type: %d", item->type);
        hex_free_item(ctx, copy);
        return NULL;
    }

    // Copy the token field for native and user symbols
    if (item->type == HEX_TYPE_NATIVE_SYMBOL || item->type == HEX_TYPE_USER_SYMBOL)
    {
        copy->token = hex_copy_token(ctx, item->token);
        if (!copy->token)
        {
            hex_free_item(ctx, copy);
            hex_error(ctx, "[copy item] Failed to copy token");
            return NULL;
        }
    }
    else
    {
        copy->token = NULL;
    }

    return copy;
}

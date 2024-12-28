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
            add_to_stack_trace(ctx, item->token);
            if (value->type == HEX_TYPE_QUOTATION && value->operator)
            {
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
                hex_debug_item(ctx, "Value: ", value);
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
    item->operator = 0;
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

hex_item_t *hex_copy_item(hex_context_t *ctx, const hex_item_t *item)
{
    hex_item_t *copy = malloc(sizeof(hex_item_t));
    if (copy == NULL)
    {
        hex_error(ctx, "[copy item] Failed to allocate memory for item");
        return NULL;
    }

    copy->type = item->type;
    switch (item->type)
    {
    case HEX_TYPE_STRING:
        copy->data.str_value = strdup(item->data.str_value);
        if (copy->data.str_value == NULL)
        {
            hex_error(ctx, "[copy item] Failed to allocate memory for string");
            free(copy);
            return NULL;
        }
        break;

    case HEX_TYPE_INTEGER:
        copy->data.int_value = item->data.int_value;
        break;

    case HEX_TYPE_QUOTATION:
        copy->quotation_size = item->quotation_size;
        copy->data.quotation_value = malloc(copy->quotation_size * sizeof(hex_item_t *));
        if (copy->data.quotation_value == NULL)
        {
            hex_error(ctx, "[copy item] Failed to allocate memory for quotation");
            free(copy);
            return NULL;
        }
        for (size_t i = 0; i < copy->quotation_size; i++)
        {
            copy->data.quotation_value[i] = hex_copy_item(ctx, item->data.quotation_value[i]);
            if (copy->data.quotation_value[i] == NULL)
            {
                hex_free_list(ctx, copy->data.quotation_value, i);
                free(copy->data.quotation_value);
                free(copy);
                return NULL;
            }
        }
        break;

    case HEX_TYPE_NATIVE_SYMBOL:
    case HEX_TYPE_USER_SYMBOL:
        copy->token = malloc(sizeof(hex_token_t));
        if (copy->token == NULL)
        {
            hex_error(ctx, "[copy item] Failed to allocate memory for token");
            free(copy);
            return NULL;
        }
        copy->token->value = strdup(item->token->value);
        if (copy->token->value == NULL)
        {
            hex_error(ctx, "[copy item] Failed to allocate memory for token value");
            hex_free_token(copy->token);
            hex_free_item(ctx, copy);
            return NULL;
        }
        break;

    default:
        hex_error(ctx, "[copy item] Failed to copy %s item", hex_type(item->type));
        hex_free_item(ctx, copy);
        return NULL;
    }

    return copy;
}

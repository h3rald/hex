#ifndef HEX_H
#include "hex.h"
#endif

////////////////////////////////////////
// Stack Implementation               //
////////////////////////////////////////

// Free a token
void hex_free_token(hex_token_t *token)
{
    if (token)
    {
        free(token->value);
        free(token);
    }
}

// Push functions
int hex_push(hex_context_t *ctx, hex_item_t item)
{

    if (ctx->stack.top >= HEX_STACK_SIZE - 1)
    {
        hex_error(ctx, "[push] Stack overflow");
        return 1;
    }
    hex_debug_item(ctx, "PUSH", item);
    int result = 0;
    if (item.type == HEX_TYPE_USER_SYMBOL)
    {
        hex_item_t value;
        if (hex_get_symbol(ctx, item.token->value, &value))
        {
            add_to_stack_trace(ctx, item.token);
            if (value.type == HEX_TYPE_QUOTATION && value.operator)
            {
                for (size_t i = 0; i < value.quotation_size; i++)
                {
                    if (hex_push(ctx, *value.data.quotation_value[i]) != 0)
                    {
                        HEX_FREE(ctx, value);
                        hex_debug_item(ctx, "FAIL", item);
                        return 1;
                    }
                }
            }
            else
            {
                result = HEX_PUSH(ctx, value);
            }
        }
        else
        {
            hex_error(ctx, "[push] Undefined user symbol: %s", item.token->value);
            HEX_FREE(ctx, value);
            result = 1;
        }
    }
    else if (item.type == HEX_TYPE_NATIVE_SYMBOL)
    {
        hex_item_t value;
        if (hex_get_symbol(ctx, item.token->value, &value))
        {
            add_to_stack_trace(ctx, item.token);
            hex_debug_item(ctx, "CALL", item);
            result = value.data.fn_value(ctx);
        }
        else
        {
            hex_error(ctx, "[push] Undefined native symbol: %s", item.token->value);
            HEX_FREE(ctx, value);
            result = 1;
        }
    }
    else
    {
        ctx->stack.entries[++ctx->stack.top] = item;
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

hex_item_t hex_string_item(hex_context_t *ctx, const char *value)
{
    char *str = hex_process_string(value);
    if (str == NULL)
    {
        hex_error(ctx, "[create string] Failed to allocate memory for string");
        return (hex_item_t){.type = HEX_TYPE_INVALID};
    }
    hex_item_t item = {.type = HEX_TYPE_STRING, .data.str_value = str};
    return item;
}

hex_item_t hex_integer_item(hex_context_t *ctx, int value)
{
    (void)(ctx);
    hex_item_t item = {.type = HEX_TYPE_INTEGER, .data.int_value = value};
    return item;
}

hex_item_t hex_quotation_item(hex_context_t *ctx, hex_item_t **quotation, size_t size)
{
    (void)(ctx);
    hex_item_t item = {.type = HEX_TYPE_QUOTATION, .data.quotation_value = quotation, .quotation_size = size};
    return item;
}

hex_item_t hex_symbol_item(hex_context_t *ctx, hex_token_t *token)
{
    hex_item_t item = {.type = hex_valid_native_symbol(ctx, token->value) ? HEX_TYPE_NATIVE_SYMBOL : HEX_TYPE_USER_SYMBOL, .token = token};
    return item;
}

int hex_push_string(hex_context_t *ctx, const char *value)
{
    return HEX_PUSH(ctx, hex_string_item(ctx, value));
}

int hex_push_integer(hex_context_t *ctx, int value)
{
    return HEX_PUSH(ctx, hex_integer_item(ctx, value));
}

int hex_push_quotation(hex_context_t *ctx, hex_item_t **quotation, size_t size)
{
    return HEX_PUSH(ctx, hex_quotation_item(ctx, quotation, size));
}

int hex_push_symbol(hex_context_t *ctx, hex_token_t *token)
{
    return HEX_PUSH(ctx, hex_symbol_item(ctx, token));
}

// Pop function
hex_item_t hex_pop(hex_context_t *ctx)
{
    if (ctx->stack.top < 0)
    {
        hex_error(ctx, "[pop] Insufficient items on the stack");
        return (hex_item_t){.type = HEX_TYPE_INVALID};
    }
    hex_debug_item(ctx, " POP", ctx->stack.entries[ctx->stack.top]);
    return ctx->stack.entries[ctx->stack.top--];
}

// Free a stack item
void hex_free_item(hex_context_t *ctx, hex_item_t item)
{
    hex_debug_item(ctx, "FREE", item);
    if (item.type == HEX_TYPE_STRING && item.data.str_value != NULL)
    {
        item.data.str_value = NULL;
        free(item.data.str_value);
    }

    else if (item.type == HEX_TYPE_QUOTATION && item.data.quotation_value != NULL)
    {
        hex_free_list(ctx, item.data.quotation_value, item.quotation_size);
        item.data.quotation_value = NULL;
    }
    else if (item.type == HEX_TYPE_NATIVE_SYMBOL && item.token->value != NULL)
    {
        item.token = NULL;
        hex_free_token(item.token);
    }
    else if (item.type == HEX_TYPE_USER_SYMBOL && item.token->value != NULL)
    {
        item.token = NULL;
        hex_free_token(item.token);
    }
    else
    {
        hex_debug(ctx, "FREE: ** nothing to free");
    }
}

void hex_free_list(hex_context_t *ctx, hex_item_t **quotation, size_t size)
{
    for (size_t i = 0; i < size; i++)
    {
        HEX_FREE(ctx, *quotation[i]);
    }
}

hex_item_t hex_copy_item(const hex_item_t *item)
{
    hex_item_t copy = *item; // Shallow copy the structure

    switch (item->type)
    {
        case HEX_TYPE_STRING:
            if (item->data.str_value)
            {
                copy.data.str_value = strdup(item->data.str_value); // Duplicate the string
            }
            break;

        case HEX_TYPE_QUOTATION:
            if (item->data.quotation_value)
            {
                copy.data.quotation_value = malloc(item->quotation_size * sizeof(hex_item_t *));
                for (size_t i = 0; i < item->quotation_size; i++)
                {
                    copy.data.quotation_value[i] = malloc(sizeof(hex_item_t));
                    *copy.data.quotation_value[i] = hex_copy_item(item->data.quotation_value[i]); // Deep copy each item
                }
            }
            copy.quotation_size = item->quotation_size;
            break;

        case HEX_TYPE_NATIVE_SYMBOL:
        case HEX_TYPE_USER_SYMBOL:
            if (item->token)
            {
                copy.token = malloc(sizeof(hex_token_t));
                *copy.token = *item->token; // Shallow copy the token structure
                if (item->token->value)
                {
                    copy.token->value = strdup(item->token->value); // Deep copy the token's value
                }
            }
            break;

        default:
            break;
    }

    return copy;
}

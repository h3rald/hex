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
        hex_error(ctx, "Stack overflow");
        return 1;
    }
    hex_debug_item(ctx, "PUSH", item);
    int result = 0;
    if (item.type == HEX_TYPE_USER_SYMBOL)
    {
        hex_item_t value;
        if (hex_get_symbol(ctx, item.token->value, &value))
        {
            result = HEX_PUSH(ctx, value);
        }
        else
        {
            hex_error(ctx, "Undefined user symbol: %s", item.token->value);
            HEX_FREE(ctx, value);
            result = 1;
        }
    }
    else if (item.type == HEX_TYPE_NATIVE_SYMBOL)
    {
        hex_debug_item(ctx, "CALL", item);
        result = item.data.fn_value(ctx);
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

char *hex_process_string(hex_context_t *ctx, const char *value)
{
    int len = strlen(value);
    char *processed_str = (char *)malloc(len + 1);
    if (!processed_str)
    {
        hex_error(ctx, "Memory allocation failed");
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
            // case '\\':
            //     *dst++ = '\\';
            //     break;
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

hex_item_t hex_string_item(hex_context_t *ctx, const char *value)
{
    hex_item_t item = {.type = HEX_TYPE_STRING, .data.str_value = hex_process_string(ctx, value)};
    return item;
}

hex_item_t hex_integer_item(hex_context_t *ctx, int value)
{
    (void)(ctx);
    hex_item_t item = {.type = HEX_TYPE_INTEGER, .data.int_value = value};
    return item;
}

hex_item_t hex_quotation_item(hex_context_t *ctx, hex_item_t **quotation, int size)
{
    (void)(ctx);
    hex_item_t item = {.type = HEX_TYPE_QUOTATION, .data.quotation_value = quotation, .quotation_size = size};
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

int hex_push_quotation(hex_context_t *ctx, hex_item_t **quotation, int size)
{
    return HEX_PUSH(ctx, hex_quotation_item(ctx, quotation, size));
}

int hex_push_symbol(hex_context_t *ctx, hex_token_t *token)
{
    add_to_stack_trace(ctx, token);
    hex_item_t value;
    if (hex_get_symbol(ctx, token->value, &value))
    {
        value.token = token;
        return HEX_PUSH(ctx, value);
    }
    else
    {
        hex_error(ctx, "Undefined symbol: %s", token->value);
        return 1;
    }
}

// Pop function
hex_item_t hex_pop(hex_context_t *ctx)
{
    if (ctx->stack.top < 0)
    {
        hex_error(ctx, "Insufficient items on the stack");
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
        hex_debug(ctx, "FREE: ** string start");
        item.data.str_value = NULL;
        free(item.data.str_value);
        hex_debug(ctx, "FREE: ** string end");
    }

    else if (item.type == HEX_TYPE_QUOTATION && item.data.quotation_value != NULL)
    {
        hex_debug(ctx, "FREE: ** quotation start");
        hex_free_list(ctx, item.data.quotation_value, item.quotation_size);
        item.data.quotation_value = NULL;
        hex_debug(ctx, "FREE: ** quotation end");
    }
    else if (item.type == HEX_TYPE_NATIVE_SYMBOL && item.token->value != NULL)
    {
        hex_debug(ctx, "FREE: ** native symbol start");
        item.token = NULL;
        hex_free_token(item.token);
        hex_debug(ctx, "FREE: ** native symbol end");
    }
    else if (item.type == HEX_TYPE_USER_SYMBOL && item.token->value != NULL)
    {
        hex_debug(ctx, "FREE: ** user symbol start");
        item.token = NULL;
        hex_free_token(item.token);
        hex_debug(ctx, "FREE: ** user symbol end");
    }
    else
    {
        hex_debug(ctx, "FREE: ** nothing to free");
    }
}

void hex_free_list(hex_context_t *ctx, hex_item_t **quotation, int size)
{
    for (int i = 0; i < size; i++)
    {
        HEX_FREE(ctx, *quotation[i]);
    }
}

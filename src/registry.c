#ifndef HEX_H
#include "hex.h"
#endif

////////////////////////////////////////
// Registry Implementation            //
////////////////////////////////////////

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

// Add a symbol to the registry
int hex_set_symbol(hex_context_t *ctx, const char *key, hex_item_t value, int native)
{

    if (!native && hex_valid_user_symbol(ctx, key) == 0)
    {
        return 1;
    }
    for (int i = 0; i < ctx->registry.size; i++)
    {
        if (strcmp(ctx->registry.entries[i].key, key) == 0)
        {
            if (ctx->registry.entries[i].value.type == HEX_TYPE_NATIVE_SYMBOL)
            {
                hex_error(ctx, "Cannot overwrite native symbol '%s'", key);
                return 1;
            }
            free(ctx->registry.entries[i].key);
            ctx->registry.entries[i].key = strdup(key);
            ctx->registry.entries[i].value = value;
            return 0;
        }
    }

    if (ctx->registry.size >= HEX_REGISTRY_SIZE)
    {
        hex_error(ctx, "Registry overflow");
        hex_free_token(value.token);
        return 1;
    }

    ctx->registry.entries[ctx->registry.size].key = strdup(key);
    ctx->registry.entries[ctx->registry.size].value = value;
    ctx->registry.size++;
    return 0;
}

// Register a native symbol
void hex_set_native_symbol(hex_context_t *ctx, const char *name, int (*func)())
{
    hex_item_t func_item;
    func_item.type = HEX_TYPE_NATIVE_SYMBOL;
    func_item.data.fn_value = func;

    if (hex_set_symbol(ctx, name, func_item, 1) != 0)
    {
        hex_error(ctx, "Error: Failed to register native symbol '%s'", name);
    }
}

// Get a symbol value from the registry
int hex_get_symbol(hex_context_t *ctx, const char *key, hex_item_t *result)
{

    for (int i = 0; i < ctx->registry.size; i++)
    {
        if (strcmp(ctx->registry.entries[i].key, key) == 0)
        {
            *result = ctx->registry.entries[i].value;
            return 1;
        }
    }
    return 0;
}

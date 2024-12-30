#ifndef HEX_H
#include "hex.h"
#endif

////////////////////////////////////////
// Registry Implementation            //
////////////////////////////////////////

static size_t hash_function(const char *key, size_t bucket_count)
{
    size_t hash = 5381;
    while (*key)
    {
        hash = ((hash << 5) + hash) + (unsigned char)(*key); // hash * 33 + key[i]
        key++;
    }
    return hash % bucket_count;
}


hex_registry_t *hex_registry_create()
{
    hex_registry_t *registry = malloc(sizeof(hex_registry_t));
    if (registry == NULL)
    {
        return NULL;
    }

    registry->bucket_count = HEX_INITIAL_REGISTRY_SIZE;
    registry->size = 0;
    registry->buckets = calloc(HEX_INITIAL_REGISTRY_SIZE, sizeof(hex_registry_entry_t *));
    if (registry->buckets == NULL)
    {
        free(registry);
        return NULL;
    }

    return registry;
}

int hex_registry_resize(hex_context_t *ctx)
{
    hex_registry_t *registry = ctx->registry;
    

    hex_registry_entry_t **new_buckets = calloc(registry->bucket_count * 2, sizeof(hex_registry_entry_t *));
    if (new_buckets == NULL)
    {
        return 1; // Memory allocation failed
    }

    // Rehash all existing entries into the new buckets
    for (size_t i = 0; i < registry->bucket_count; i++)
    {
        hex_registry_entry_t *entry = registry->buckets[i];
        while (entry)
        {
            hex_registry_entry_t *next = entry->next;

            // Recompute hash index
            size_t new_bucket_index = hash_function(entry->key, new_bucket_count);
            entry->next = new_buckets[new_bucket_index];
            new_buckets[new_bucket_index] = entry;

            entry = next;
        }
    }

    // Replace old buckets with new ones
    free(registry->buckets);
    registry->buckets = new_buckets;
    registry->bucket_count = new_bucket_count;

    return 0;
}

void hex_registry_destroy(hex_context_t *ctx)
{
    hex_registry_t *registry = ctx->registry;

    for (size_t i = 0; i < registry->bucket_count; i++)
    {
        hex_registry_entry_t *entry = registry->buckets[i];
        while (entry)
        {
            hex_registry_entry_t *next = entry->next;
            free(entry->key);
            hex_free_item(NULL, entry->value);
            free(entry);
            entry = next;
        }
    }

    free(registry->buckets);
    free(registry);
}


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
    /*if (strlen(symbol) > HEX_MAX_SYMBOL_LENGTH)
    {
        hex_error(ctx, "Symbol name too long: %s", symbol);
        return 0;
    }*/
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

int hex_set_symbol(hex_context_t *ctx, const char *key, hex_item_t *value)
{
    hex_registry_t *registry = ctx->registry;
    if (hex_valid_user_symbol(ctx, key) == 0)
    {
        return 1;
    }

    size_t bucket_index = hash_function(key, registry->bucket_count);
    hex_registry_entry_t *entry = registry->buckets[bucket_index];

    // Search for an existing key in the bucket
    while (entry)
    {
        if (strcmp(entry->key, key) == 0)
        {
            // Key already exists, update its value
            hex_free_item(NULL, entry->value); // Free old value
            entry->value = value;
            return 0;
        }
        entry = entry->next;
    }

    // Add a new entry to the bucket
    hex_registry_entry_t *new_entry = malloc(sizeof(hex_registry_entry_t));
    if (new_entry == NULL)
    {
        return 1; // Memory allocation failed
    }

    new_entry->key = strdup(key);
    new_entry->value = value;
    new_entry->next = registry->buckets[bucket_index];
    registry->buckets[bucket_index] = new_entry;

    registry->size++;

    return 0;
}


// Add a symbol to the registry
int hex_set_symbolOLD(hex_context_t *ctx, const char *key, hex_item_t *value, int native)
{

    if (!native && hex_valid_user_symbol(ctx, key) == 0)
    {
        return 1;
    }
    for (size_t i = 0; i < ctx->registry->size; i++)
    {
        if (strcmp(ctx->registry->entries[i]->key, key) == 0)
        {
            if (ctx->registry->entries[i]->value->type == HEX_TYPE_NATIVE_SYMBOL)
            {
                hex_error(ctx, "[set symbol] Cannot overwrite native symbol '%s'", key);
                return 1;
            }
            free(ctx->registry->entries[i]->key);
            ctx->registry->entries[i]->key = strdup(key);
            ctx->registry->entries[i]->value = value;
            return 0;
        }
    }

    if (ctx->registry->size >= HEX_REGISTRY_SIZE)
    {
        hex_error(ctx, "Registry overflow");
        hex_free_token(value->token);
        return 1;
    }

    ctx->registry->entries[ctx->registry->size] = malloc(sizeof(hex_registry_entry_t));
    if (ctx->registry->entries[ctx->registry->size] == NULL)
    {
        hex_error(ctx, "[set symbol] Memory allocation failed for registry entry");
        return 1;
    }
    ctx->registry->entries[ctx->registry->size]->key = strdup(key);
    ctx->registry->entries[ctx->registry->size]->value = value;
    ctx->registry->size++;
    return 0;
}

// Register a native symbol
void hex_set_native_symbol(hex_context_t *ctx, const char *name, int (*func)())
{
    hex_item_t *func_item = malloc(sizeof(hex_item_t));
    if (func_item == NULL)
    {
        hex_error(ctx, "[set native symbol] Memory allocation failed for native symbol '%s'", name);
        return;
    }
    func_item->type = HEX_TYPE_NATIVE_SYMBOL;
    func_item->data.fn_value = func;
    // Need to create a fake token for native symbols as well.
    func_item->token = malloc(sizeof(hex_token_t));
    func_item->token->type = HEX_TOKEN_SYMBOL;
    func_item->token->value = strdup(name);
    func_item->token->position = NULL;

    if (hex_set_symbol(ctx, name, func_item, 1) != 0)
    {
        hex_error(ctx, "Error: Failed to register native symbol '%s'", name);
        hex_free_item(ctx, func_item);
    }
}

// Get a symbol value from the registry
int hex_get_symbol(hex_context_t *ctx, const char *key, hex_item_t *result)
{
    for (size_t i = 0; i < ctx->registry->size; i++)
    {
        if (strcmp(ctx->registry->entries[i]->key, key) == 0)
        {
            if (ctx->registry->entries[i]->value == NULL)
            {
                hex_error(ctx, "[get symbol] Registry entry value is NULL for key: %s", key);
                return 0;
            }
            hex_debug(ctx, "LOOK: %s", key);
            hex_item_t *item = hex_copy_item(ctx, ctx->registry->entries[i]->value);
            if (item == NULL)
            {
                hex_error(ctx, "[get symbol] Failed to copy item for key: %s", key);
                return 0;
            }
            *result = *item;
            hex_debug_item(ctx, "DONE", result);
            return 1;
        }
    }
    return 0;
}

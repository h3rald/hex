#ifndef HEX_H
#include "hex.h"
#endif

void hex_symboltable_init(hex_context_t *ctx)
{
    ctx->symbol_table.count = 0;
    ctx->symbol_table.symbols = malloc(HEX_MAX_USER_SYMBOLS * sizeof(char *));
}

void hex_symboltable_free(hex_context_t *ctx)
{
    for (uint16_t i = 0; i < ctx->symbol_table.count; ++i)
    {
        free(ctx->symbol_table.symbols[i]);
    }
    free(ctx->symbol_table.symbols);
    ctx->symbol_table.count = 0;
}

// Add a symbol to the table if it does not already exist
// Returns 0 on success, -1 if the symbol is too long or table is full
int hex_symboltable_set(hex_context_t *ctx, const char *symbol)
{
    hex_symbol_table_t *table = &ctx->symbol_table;
    size_t len = strlen(symbol);

    // Check symbol length
    if (len > HEX_MAX_SYMBOL_LENGTH)
    {
        return -1; // Symbol too long
    }

    // Check if table is full
    if (table->count >= HEX_MAX_USER_SYMBOLS)
    {
        return -1; // Table full
    }

    // Check if symbol already exists
    for (uint16_t i = 0; i < table->count; ++i)
    {
        if (strcmp(table->symbols[i], symbol) == 0)
        {
            return 0; // Symbol already exists, no-op
        }
    }

    // Add the symbol
    table->symbols[table->count] = strdup(symbol);
    table->count++;
    return 0;
}

// Get the index of a symbol in the table, or -1 if not found
int hex_symboltable_get(hex_context_t *ctx, const char *symbol)
{
    hex_symbol_table_t *table = &ctx->symbol_table;
    for (uint16_t i = 0; i < table->count; ++i)
    {
        if (strcmp(table->symbols[i], symbol) == 0)
        {
            return i;
        }
    }
    return -1; // Symbol not found
}

// Decode a bytecode's symbol table into the hex_symbol_table_t structure
// Assumes input is well-formed
void hex_decode_bytecode_symboltable(hex_context_t *ctx, const uint8_t *bytecode, size_t size)
{
    hex_symbol_table_t *table = &ctx->symbol_table;
    table->count = 0;
    size_t offset = 0;

    while (offset < size)
    {
        if (table->count >= HEX_MAX_USER_SYMBOLS)
        {
            break; // Prevent overflow
        }

        uint8_t str_len = bytecode[offset++];
        char *symbol = malloc(str_len + 1);
        memcpy(symbol, bytecode + offset, str_len);
        symbol[str_len] = '\0';
        offset += str_len;

        hex_symboltable_set(ctx, symbol);
        free(symbol);
    }
}

// Encode the symbol table into a bytecode representation
// Returns bytecode buffer and sets out_size to the bytecode length
uint8_t *hex_encode_bytecode_symboltable(hex_context_t *ctx, size_t *out_size)
{
    hex_symbol_table_t *table = &ctx->symbol_table;
    size_t total_size = 0;

    // Calculate total size
    for (uint16_t i = 0; i < table->count; ++i)
    {
        total_size += 1 + strlen(table->symbols[i]);
    }

    uint8_t *bytecode = malloc(total_size);
    size_t offset = 0;

    for (uint16_t i = 0; i < table->count; ++i)
    {
        size_t len = strlen(table->symbols[i]);
        bytecode[offset++] = (uint8_t)len;
        memcpy(bytecode + offset, table->symbols[i], len);
        offset += len;
    }

    *out_size = total_size;
    return bytecode;
}

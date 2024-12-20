#ifndef HEX_H
#include "hex.h"
#endif

int hex_symboltable_set(hex_context_t *ctx, const char *symbol)
{
    hex_symbol_table_t *table = &ctx->symbol_table;
    size_t len = strlen(symbol);

    if (len > HEX_MAX_SYMBOL_LENGTH)
    {
        return -1;
    }

    if (table->count >= HEX_MAX_USER_SYMBOLS)
    {
        return -1; // Table full
    }

    for (uint16_t i = 0; i < table->count; ++i)
    {
        if (strcmp(table->symbols[i], symbol) == 0)
        {
            return 0;
        }
    }

    table->symbols[table->count] = strdup(symbol);
    table->count++;
    return 0;
}

int hex_symboltable_get_index(hex_context_t *ctx, const char *symbol)
{
    hex_symbol_table_t *table = &ctx->symbol_table;
    for (uint16_t i = 0; i < table->count; ++i)
    {
        if (strcmp(table->symbols[i], symbol) == 0)
        {
            return i;
        }
    }
    return -1;
}

char *hex_symboltable_get_value(hex_context_t *ctx, uint16_t index)
{
    if (index >= ctx->symbol_table.count)
    {
        return NULL;
    }
    return ctx->symbol_table.symbols[index];
}

int hex_decode_bytecode_symboltable(hex_context_t *ctx, uint8_t **bytecode, size_t *size, size_t total)
{
    hex_symbol_table_t *table = &ctx->symbol_table;
    table->count = 0;

    for (size_t i = 0; i < total; i++)
    {
        hex_debug(ctx, "Decoding symbol %zu", i);
        size_t len = (size_t)(*bytecode)[0];
        (*bytecode)++;
        *size -= 1;

        char *symbol = malloc(len + 1);
        if (symbol == NULL)
        {
            hex_error(ctx, "[decode symbol table] Memory allocation failed");
            // Handle memory allocation failure
            return -1;
        }
        memcpy(symbol, *bytecode, len);
        symbol[len] = '\0';
        hex_symboltable_set(ctx, symbol);
        free(symbol);
        *bytecode += len;
        *size -= len;
    }
    return 0;
}

uint8_t *hex_encode_bytecode_symboltable(hex_context_t *ctx, size_t *out_size)
{
    hex_symbol_table_t *table = &ctx->symbol_table;
    size_t total_size = 0;

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

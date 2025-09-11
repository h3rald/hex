#ifndef HEX_H
#include "hex.h"
#endif

////////////////////////////////////////
// Hex Interpreter Implementation     //
////////////////////////////////////////

hex_context_t *hex_init()
{
    hex_context_t *context = malloc(sizeof(hex_context_t));
    if (!context)
        return NULL;

    context->argc = 0;
    context->argv = NULL;
    context->registry = hex_registry_create();
    context->docs = malloc(sizeof(hex_doc_dictionary_t));
    if (context->docs)
    {
        context->docs->entries = malloc(HEX_NATIVE_SYMBOLS * sizeof(hex_doc_entry_t));
        context->docs->size = 0;
    }
    context->stack = malloc(sizeof(hex_stack_t));
    if (context->stack)
    {
        context->stack->entries = malloc(HEX_STACK_SIZE * sizeof(hex_item_t *));
        context->stack->top = -1;
        context->stack->capacity = HEX_STACK_SIZE;
        // Initialize all entries to NULL
        for (int i = 0; i < HEX_STACK_SIZE; i++)
        {
            context->stack->entries[i] = NULL;
        }
    }
    context->stack_trace = malloc(sizeof(hex_stack_trace_t));
    if (context->stack_trace)
    {
        context->stack_trace->entries = malloc(HEX_STACK_TRACE_SIZE * sizeof(hex_token_t *));
        context->stack_trace->start = 0;
        context->stack_trace->size = 0;
        // Initialize all entries to NULL
        for (int i = 0; i < HEX_STACK_TRACE_SIZE; i++)
        {
            context->stack_trace->entries[i] = NULL;
        }
    }
    context->settings = malloc(sizeof(hex_settings_t));
    if (context->settings)
    {
        context->settings->debugging_enabled = 0;
        context->settings->errors_enabled = 1;
        context->settings->stack_trace_enabled = 1;
    }
    context->symbol_table = malloc(sizeof(hex_symbol_table_t));
    if (context->symbol_table)
    {
        context->symbol_table->count = 0;
        context->symbol_table->symbols = malloc(HEX_MAX_USER_SYMBOLS * sizeof(char *));
        // Initialize all symbols to NULL
        for (int i = 0; i < HEX_MAX_USER_SYMBOLS; i++)
        {
            context->symbol_table->symbols[i] = NULL;
        }
    }
    return context;
}

int hex_interpret(hex_context_t *ctx, const char *code, const char *filename, int line, int column)
{

    const char *input = code;
    hex_file_position_t position = {filename, line, column};
    hex_token_t *token = hex_next_token(ctx, &input, &position);

    while (token != NULL && token->type != HEX_TOKEN_INVALID)
    {
        int result = 0;
        
        if (token->type == HEX_TOKEN_INTEGER)
        {
            result = hex_push_integer(ctx, hex_parse_integer(token->value));
        }
        else if (token->type == HEX_TOKEN_STRING)
        {
            result = hex_push_string(ctx, token->value);
        }
        else if (token->type == HEX_TOKEN_SYMBOL)
        {
            if (token->position && filename)
            {
                if (token->position->filename)
                {
                    free((void *)token->position->filename);
                }
                token->position->filename = strdup(filename);
            }
            result = hex_push_symbol(ctx, token);
            // hex_push_symbol copies the token (from Patch 2), so we free the original
        }
        else if (token->type == HEX_TOKEN_QUOTATION_END)
        {
            hex_error(ctx, "(%d,%d) Unexpected end of quotation", position.line, position.column);
            result = 1;
        }
        else if (token->type == HEX_TOKEN_QUOTATION_START)
        {
            hex_item_t *quotationItem = (hex_item_t *)malloc(sizeof(hex_item_t));
            if (!quotationItem)
            {
                hex_error(ctx, "(%d,%d) Failed to allocate memory for quotation", position.line, position.column);
                result = 1;
            }
            else if (hex_parse_quotation(ctx, &input, quotationItem, &position) != 0)
            {
                hex_error(ctx, "(%d,%d) Failed to parse quotation", position.line, position.column);
                free(quotationItem);
                result = 1;
            }
            else
            {
                result = hex_push_quotation(ctx, quotationItem->data.quotation_value, quotationItem->quotation_size);
                // Don't free quotationItem here as its content is now owned by the stack
                free(quotationItem); // Only free the wrapper, not the contents
            }
        }

        if (result != 0)
        {
            hex_error(ctx, "[interpret] Unable to push: %s", token->value);
            hex_free_token(token);
            print_stack_trace(ctx);
            return result;
        }

        // Always free the token after processing since no function consumes it
        // All push functions copy the token data they need, so we own the original
        hex_free_token(token);

        token = hex_next_token(ctx, &input, &position);
    }
    if (token != NULL && token->type == HEX_TOKEN_INVALID)
    {
        token->position->filename = strdup(filename);
        add_to_stack_trace(ctx, token);
        print_stack_trace(ctx);
        hex_free_token(token); // Make sure to free the invalid token too
        return 1;
    }
    return 0;
}

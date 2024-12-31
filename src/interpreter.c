#ifndef HEX_H
#include "hex.h"
#endif

////////////////////////////////////////
// Hex Interpreter Implementation     //
////////////////////////////////////////

hex_context_t *hex_init()
{
    hex_context_t *context = malloc(sizeof(hex_context_t));
    context->argc = 0;
    context->argv = NULL;
    context->registry = hex_registry_create();
    context->docs = malloc(sizeof(hex_doc_dictionary_t));
    context->docs->entries = malloc(HEX_NATIVE_SYMBOLS * sizeof(hex_doc_entry_t));
    context->docs->size = 0;
    context->stack = malloc(sizeof(hex_stack_t));
    context->stack->entries = malloc(HEX_STACK_SIZE * sizeof(hex_item_t));
    context->stack->top = -1;
    context->stack_trace = malloc(sizeof(hex_stack_trace_t));
    context->stack_trace->entries = malloc(HEX_STACK_TRACE_SIZE * sizeof(hex_token_t));
    context->stack_trace->start = 0;
    context->stack_trace->size = 0;
    context->settings = malloc(sizeof(hex_settings_t));
    context->settings->debugging_enabled = 0;
    context->settings->errors_enabled = 1;
    context->settings->stack_trace_enabled = 1;
    context->symbol_table = malloc(sizeof(hex_symbol_table_t));
    context->symbol_table->count = 0;
    context->symbol_table->symbols = malloc(HEX_MAX_USER_SYMBOLS * sizeof(char *));
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
            token->position->filename = strdup(filename);
            result = hex_push_symbol(ctx, token);
        }
        else if (token->type == HEX_TOKEN_QUOTATION_END)
        {
            hex_error(ctx, "(%d,%d) Unexpected end of quotation", position.line, position.column);
            result = 1;
        }
        else if (token->type == HEX_TOKEN_QUOTATION_START)
        {
            hex_item_t *quotationItem = (hex_item_t *)malloc(sizeof(hex_item_t));
            if (hex_parse_quotation(ctx, &input, quotationItem, &position) != 0)
            {
                hex_error(ctx, "(%d,%d) Failed to parse quotation", position.line, position.column);
                result = 1;
            }
            else
            {
                hex_item_t **quotation = quotationItem->data.quotation_value;
                int quotation_size = quotationItem->quotation_size;
                result = hex_push_quotation(ctx, quotation, quotation_size);
            }
        }

        if (result != 0)
        {
            hex_error(ctx, "[interpret] Unable to push: %s", token->value);
            hex_free_token(token);
            print_stack_trace(ctx);
            return result;
        }

        token = hex_next_token(ctx, &input, &position);
    }
    if (token != NULL && token->type == HEX_TOKEN_INVALID)
    {
        token->position->filename = strdup(filename);
        add_to_stack_trace(ctx, token);
        print_stack_trace(ctx);
        return 1;
    }
    return 0;
}

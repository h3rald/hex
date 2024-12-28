#ifndef HEX_H
#include "hex.h"
#endif

////////////////////////////////////////
// Error & Debugging                  //
////////////////////////////////////////

void hex_error(hex_context_t *ctx, const char *format, ...)
{
    va_list args;
    va_start(args, format);
    vsnprintf(ctx->error, sizeof(ctx->error), format, args);
    if (ctx->settings->errors_enabled) /// FC
    {
        fprintf(stderr, "ERROR: ");
        fprintf(stderr, "%s\n", ctx->error);
    }
    va_end(args);
}

void hex_debug(hex_context_t *ctx, const char *format, ...)
{
    if (ctx->settings->debugging_enabled)
    {
        va_list args;
        va_start(args, format);
        fprintf(stdout, "*** ");
        vfprintf(stdout, format, args);
        fprintf(stdout, "\n");
        va_end(args);
    }
}

char *hex_type(hex_item_type_t type)
{
    switch (type)
    {
    case HEX_TYPE_INTEGER:
        return "integer";
    case HEX_TYPE_STRING:
        return "string";
    case HEX_TYPE_QUOTATION:
        return "quotation";
    case HEX_TYPE_NATIVE_SYMBOL:
        return "native-symbol";
    case HEX_TYPE_USER_SYMBOL:
        return "user-symbol";
    case HEX_TYPE_INVALID:
        return "invalid";
    default:
        return "unknown";
    }
}

void hex_debug_item(hex_context_t *ctx, const char *message, hex_item_t *item)
{
    if (ctx->settings->debugging_enabled)
    {
        fprintf(stdout, "*** %s: ", message);
        hex_print_item(stdout, item);
        fprintf(stdout, "\n");
    }
}

////////////////////////////////////////
// Stack trace implementation         //
////////////////////////////////////////

// Add an entry to the circular stack trace
void add_to_stack_trace(hex_context_t *ctx, hex_token_t *token)
{
    int index = (ctx->stack_trace->start + ctx->stack_trace->size) % HEX_STACK_TRACE_SIZE;

    if (ctx->stack_trace->size < HEX_STACK_TRACE_SIZE)
    {
        // Buffer is not full; add item
        ctx->stack_trace->entries[index] = token;
        ctx->stack_trace->size++;
    }
    else
    {
        // Buffer is full; overwrite the oldest item
        ctx->stack_trace->entries[index] = token;
        ctx->stack_trace->start = (ctx->stack_trace->start + 1) % HEX_STACK_TRACE_SIZE;
    }
}

// Print the stack trace
void print_stack_trace(hex_context_t *ctx)
{
    if (!ctx->settings->stack_trace_enabled || !ctx->settings->errors_enabled || ctx->stack_trace->size <= 0)
    {
        return;
    }
    fprintf(stderr, "[stack trace] (most recent symbol first):\n");

    for (size_t i = 0; i < ctx->stack_trace->size; i++)
    {
        int index = (ctx->stack_trace->start + ctx->stack_trace->size - 1 - i) % HEX_STACK_TRACE_SIZE;
        hex_token_t *token = malloc(sizeof(hex_token_t));
        token = ctx->stack_trace->entries[index];
        fprintf(stderr, "  %s (%s:%d:%d)\n", token->value, token->position->filename, token->position->line, token->position->column);
    }
}

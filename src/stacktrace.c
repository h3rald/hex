#ifndef HEX_H
#include "hex.h"
#endif

////////////////////////////////////////
// Stack trace implementation         //
////////////////////////////////////////

// Add an entry to the circular stack trace
void add_to_stack_trace(hex_context_t *ctx, hex_token_t *token)
{
    int index = (ctx->stack_trace.start + ctx->stack_trace.size) % HEX_STACK_TRACE_SIZE;

    if (ctx->stack_trace.size < HEX_STACK_TRACE_SIZE)
    {
        // Buffer is not full; add item
        ctx->stack_trace.entries[index] = *token;
        ctx->stack_trace.size++;
    }
    else
    {
        // Buffer is full; overwrite the oldest item
        ctx->stack_trace.entries[index] = *token;
        ctx->stack_trace.start = (ctx->stack_trace.start + 1) % HEX_STACK_TRACE_SIZE;
    }
}

// Print the stack trace
void print_stack_trace(hex_context_t *ctx)
{
    if (!ctx->settings.stack_trace_enabled || !ctx->settings.errors_enabled || ctx->stack_trace.size <= 0)
    {
        return;
    }
    fprintf(stderr, "[stack trace] (most recent symbol first):\n");

    for (size_t i = 0; i < ctx->stack_trace.size; i++)
    {
        int index = (ctx->stack_trace.start + ctx->stack_trace.size - 1 - i) % HEX_STACK_TRACE_SIZE;
        hex_token_t token = ctx->stack_trace.entries[index];
        fprintf(stderr, "  %s (%s:%d:%d)\n", token.value, token.position.filename, token.position.line, token.position.column);
    }
}

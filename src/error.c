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
    if (ctx->settings.errors_enabled) /// FC
    {
        fprintf(stderr, "[error] ");
        fprintf(stderr, "%s\n", ctx->error);
    }
    va_end(args);
}

void hex_debug(hex_context_t *ctx, const char *format, ...)
{
    if (ctx->settings.debugging_enabled)
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

void hex_debug_item(hex_context_t *ctx, const char *message, hex_item_t item)
{
    if (ctx->settings.debugging_enabled)
    {
        fprintf(stdout, "*** %s: ", message);
        hex_print_item(stdout, item);
        fprintf(stdout, "\n");
    }
}

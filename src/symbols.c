#ifndef HEX_H
#include "hex.h"
#endif

////////////////////////////////////////
// Native Symbol Implementations      //
////////////////////////////////////////

// Definition symbols

int hex_symbol_store(hex_context_t *ctx)
{

    HEX_POP(ctx, name);
    if (name.type == HEX_TYPE_INVALID)
    {
        HEX_FREE(ctx, name);
        return 1;
    }
    HEX_POP(ctx, value);
    if (value.type == HEX_TYPE_INVALID)
    {
        HEX_FREE(ctx, name);
        HEX_FREE(ctx, value);
        return 1;
    }
    if (name.type != HEX_TYPE_STRING)
    {
        hex_error(ctx, "[symbol :] Symbol name must be a string");
        HEX_FREE(ctx, name);
        HEX_FREE(ctx, value);
        return 1;
    }
    if (hex_set_symbol(ctx, name.data.str_value, value, 0) != 0)
    {
        hex_error(ctx, "[symbol :] Failed to store symbol '%s'", name.data.str_value);
        HEX_FREE(ctx, name);
        HEX_FREE(ctx, value);
        return 1;
    }
    return 0;
}

int hex_symbol_define(hex_context_t *ctx)
{

    HEX_POP(ctx, name);
    if (name.type == HEX_TYPE_INVALID)
    {
        HEX_FREE(ctx, name);
        return 1;
    }
    HEX_POP(ctx, value);
    if (value.type == HEX_TYPE_INVALID)
    {
        HEX_FREE(ctx, name);
        HEX_FREE(ctx, value);
        return 1;
    }
    if (name.type != HEX_TYPE_STRING)
    {
        hex_error(ctx, "[symbol ::] Symbol name must be a string");
        HEX_FREE(ctx, name);
        HEX_FREE(ctx, value);
        return 1;
    }
    if (value.type == HEX_TYPE_QUOTATION)
    {
        value.immediate = 1;
    }
    if (hex_set_symbol(ctx, name.data.str_value, value, 0) != 0)
    {
        hex_error(ctx, "[symbol ::] Failed to store symbol '%s'", name.data.str_value);
        HEX_FREE(ctx, name);
        HEX_FREE(ctx, value);
        return 1;
    }
    return 0;
}

int hex_symbol_free(hex_context_t *ctx)
{

    HEX_POP(ctx, item);
    if (item.type == HEX_TYPE_INVALID)
    {
        HEX_FREE(ctx, item);
        return 1;
    }
    if (item.type != HEX_TYPE_STRING)
    {
        HEX_FREE(ctx, item);
        hex_error(ctx, "[symbol #] Symbol name must be a string");
        return 1;
    }
    if (hex_valid_native_symbol(ctx, item.data.str_value))
    {
        hex_error(ctx, "[symbol #] Cannot free native symbol '%s'", item.data.str_value);
        HEX_FREE(ctx, item);
        return 1;
    }
    for (size_t i = 0; i < ctx->registry.size; i++)
    {
        if (strcmp(ctx->registry.entries[i].key, item.data.str_value) == 0)
        {
            free(ctx->registry.entries[i].key);
            HEX_FREE(ctx, ctx->registry.entries[i].value);
            for (size_t j = i; j < ctx->registry.size - 1; j++)
            {
                ctx->registry.entries[j] = ctx->registry.entries[j + 1];
            }
            ctx->registry.size--;
            HEX_FREE(ctx, item);
            return 0;
        }
    }
    HEX_FREE(ctx, item);
    return 0;
}

int hex_symbol_type(hex_context_t *ctx)
{

    HEX_POP(ctx, item);
    if (item.type == HEX_TYPE_INVALID)
    {
        HEX_FREE(ctx, item);
        return 1;
    }
    return hex_push_string(ctx, hex_type(item.type));
}

// Evaluation symbols

int hex_symbol_i(hex_context_t *ctx)
{

    HEX_POP(ctx, item);
    if (item.type == HEX_TYPE_INVALID)
    {
        HEX_FREE(ctx, item);
        return 1;
    }
    if (item.type != HEX_TYPE_QUOTATION)
    {
        hex_error(ctx, "[symbol .] Quotation required");
        HEX_FREE(ctx, item);
        return 1;
    }
    for (size_t i = 0; i < item.quotation_size; i++)
    {
        if (hex_push(ctx, *item.data.quotation_value[i]) != 0)
        {
            HEX_FREE(ctx, item);
            return 1;
        }
    }
    return 0;
}

// evaluate a string or bytecode array
int hex_symbol_eval(hex_context_t *ctx)
{
    HEX_POP(ctx, item);
    if (item.type == HEX_TYPE_INVALID)
    {
        HEX_FREE(ctx, item);
        return 1;
    }
    if (item.type == HEX_TYPE_STRING)
    {
        return hex_interpret(ctx, item.data.str_value, "<!>", 1, 1);
    }
    else if (item.type == HEX_TYPE_QUOTATION)
    {
        for (size_t i = 0; i < item.quotation_size; i++)
        {
            if (item.data.quotation_value[i]->type != HEX_TYPE_INTEGER)
            {
                hex_error(ctx, "[symbol !] Quotation must contain only integers");
                HEX_FREE(ctx, item);
                return 1;
            }
        }
        uint8_t *bytecode = (uint8_t *)malloc(item.quotation_size * sizeof(uint8_t));
        if (!bytecode)
        {
            hex_error(ctx, "[symbol !] Memory allocation failed");
            HEX_FREE(ctx, item);
            return 1;
        }
        for (size_t i = 0; i < item.quotation_size; i++)
        {
            if (item.data.quotation_value[i]->type != HEX_TYPE_INTEGER)
            {
                hex_error(ctx, "[symbol !] Quotation must contain only integers");
                free(bytecode);
                HEX_FREE(ctx, item);
                return 1;
            }
            bytecode[i] = (uint8_t)item.data.quotation_value[i]->data.int_value;
        }
        int result = hex_interpret_bytecode(ctx, bytecode, item.quotation_size);
        free(bytecode);
        return result;
    }
    else
    {
        hex_error(ctx, "[symbol !] String or a quotation of integers required");
        HEX_FREE(ctx, item);
        return 1;
    }
}

// IO Symbols

int hex_symbol_puts(hex_context_t *ctx)
{

    HEX_POP(ctx, item);
    if (item.type == HEX_TYPE_INVALID)
    {
        HEX_FREE(ctx, item);
        return 1;
    }
    hex_raw_print_item(stdout, item);
    printf("\n");
    return 0;
}

int hex_symbol_warn(hex_context_t *ctx)
{

    HEX_POP(ctx, item);
    if (item.type == HEX_TYPE_INVALID)
    {
        HEX_FREE(ctx, item);
        return 1;
    }
    hex_raw_print_item(stderr, item);
    printf("\n");
    return 0;
}

int hex_symbol_print(hex_context_t *ctx)
{

    HEX_POP(ctx, item);
    if (item.type == HEX_TYPE_INVALID)
    {
        HEX_FREE(ctx, item);
        return 1;
    }
    hex_raw_print_item(stdout, item);
    fflush(stdout);
    return 0;
}

int hex_symbol_gets(hex_context_t *ctx)
{

    char input[HEX_STDIN_BUFFER_SIZE]; // Buffer to store the input (adjust size if needed)
    char *p = input;
#if defined(__EMSCRIPTEN__)
    p = em_fgets(input, 1024);
#else
    p = fgets(input, sizeof(input), stdin);
#endif

    if (p != NULL)
    {
        // Strip the newline character at the end of the string
        input[strcspn(input, "\n")] = '\0';

        // Push the input string onto the stack
        return hex_push_string(ctx, input);
    }
    else
    {
        hex_error(ctx, "[symbol gets] Failed to read input");
        return 1;
    }
}

// Mathematical symbols
int hex_symbol_add(hex_context_t *ctx)
{
    HEX_POP(ctx, b);
    if (b.type == HEX_TYPE_INVALID)
    {
        HEX_FREE(ctx, b);
        return 1;
    }
    HEX_POP(ctx, a);
    if (a.type == HEX_TYPE_INVALID)
    {
        HEX_FREE(ctx, a);
        HEX_FREE(ctx, b);
        return 1;
    }
    if (a.type == HEX_TYPE_INTEGER && b.type == HEX_TYPE_INTEGER)
    {
        return hex_push_integer(ctx, a.data.int_value + b.data.int_value);
    }
    hex_error(ctx, "[symbol +] Two integers required");
    HEX_FREE(ctx, a);
    HEX_FREE(ctx, b);
    return 1;
}

int hex_symbol_subtract(hex_context_t *ctx)
{

    HEX_POP(ctx, b);
    if (b.type == HEX_TYPE_INVALID)
    {
        HEX_FREE(ctx, b);
        return 1;
    }
    HEX_POP(ctx, a);
    if (a.type == HEX_TYPE_INVALID)
    {
        HEX_FREE(ctx, a);
        HEX_FREE(ctx, b);
        return 1;
    }
    if (a.type == HEX_TYPE_INTEGER && b.type == HEX_TYPE_INTEGER)
    {
        return hex_push_integer(ctx, a.data.int_value - b.data.int_value);
    }
    hex_error(ctx, "[symbol -] Two integers required");
    HEX_FREE(ctx, a);
    HEX_FREE(ctx, b);
    return 1;
}

int hex_symbol_multiply(hex_context_t *ctx)
{

    HEX_POP(ctx, b);
    if (b.type == HEX_TYPE_INVALID)
    {
        HEX_FREE(ctx, b);
        return 1;
    }
    HEX_POP(ctx, a);
    if (a.type == HEX_TYPE_INVALID)
    {
        HEX_FREE(ctx, a);
        HEX_FREE(ctx, b);
        return 1;
    }
    if (a.type == HEX_TYPE_INTEGER && b.type == HEX_TYPE_INTEGER)
    {
        return hex_push_integer(ctx, a.data.int_value * b.data.int_value);
    }
    hex_error(ctx, "[symbol *] Two integers required");
    HEX_FREE(ctx, a);
    HEX_FREE(ctx, b);
    return 1;
}

int hex_symbol_divide(hex_context_t *ctx)
{

    HEX_POP(ctx, b);
    if (b.type == HEX_TYPE_INVALID)
    {
        HEX_FREE(ctx, b);
        return 1;
    }
    HEX_POP(ctx, a);
    if (a.type == HEX_TYPE_INVALID)
    {
        HEX_FREE(ctx, a);
        HEX_FREE(ctx, b);
        return 1;
    }
    if (a.type == HEX_TYPE_INTEGER && b.type == HEX_TYPE_INTEGER)
    {
        if (b.data.int_value == 0)
        {
            hex_error(ctx, "[symbol /] Division by zero");
            return 1;
        }
        return hex_push_integer(ctx, a.data.int_value / b.data.int_value);
    }
    hex_error(ctx, "[symbol /] Two integers required");
    HEX_FREE(ctx, a);
    HEX_FREE(ctx, b);
    return 1;
}

int hex_symbol_modulo(hex_context_t *ctx)
{

    HEX_POP(ctx, b);
    if (b.type == HEX_TYPE_INVALID)
    {
        HEX_FREE(ctx, b);
        return 1;
    }
    HEX_POP(ctx, a);
    if (a.type == HEX_TYPE_INVALID)
    {
        HEX_FREE(ctx, a);
        HEX_FREE(ctx, b);
        return 1;
    }
    if (a.type == HEX_TYPE_INTEGER && b.type == HEX_TYPE_INTEGER)
    {
        if (b.data.int_value == 0)
        {
            hex_error(ctx, "[symbol %%] Division by zero");
            return 1;
        }
        return hex_push_integer(ctx, a.data.int_value % b.data.int_value);
    }
    hex_error(ctx, "[symbol %%] Two integers required");
    HEX_FREE(ctx, a);
    HEX_FREE(ctx, b);
    return 1;
}

// Bit symbols

int hex_symbol_bitand(hex_context_t *ctx)
{

    HEX_POP(ctx, right);
    if (right.type == HEX_TYPE_INVALID)
    {
        HEX_FREE(ctx, right);
        return 1;
    }
    HEX_POP(ctx, left);
    if (left.type == HEX_TYPE_INVALID)
    {
        HEX_FREE(ctx, left);
        HEX_FREE(ctx, right);
        return 1;
    }
    if (left.type == HEX_TYPE_INTEGER && right.type == HEX_TYPE_INTEGER)
    {
        return hex_push_integer(ctx, left.data.int_value & right.data.int_value);
    }
    hex_error(ctx, "[symbol &] Two integers required");
    HEX_FREE(ctx, left);
    HEX_FREE(ctx, right);
    return 1;
}

int hex_symbol_bitor(hex_context_t *ctx)
{

    HEX_POP(ctx, right);
    if (right.type == HEX_TYPE_INVALID)
    {
        HEX_FREE(ctx, right);
        return 1;
    }
    HEX_POP(ctx, left);
    if (left.type == HEX_TYPE_INVALID)
    {
        HEX_FREE(ctx, left);
        HEX_FREE(ctx, right);
        return 1;
    }
    if (left.type == HEX_TYPE_INTEGER && right.type == HEX_TYPE_INTEGER)
    {
        return hex_push_integer(ctx, left.data.int_value | right.data.int_value);
    }
    hex_error(ctx, "[symbol |] Two integers required");
    HEX_FREE(ctx, left);
    HEX_FREE(ctx, right);
    return 1;
}

int hex_symbol_bitxor(hex_context_t *ctx)
{

    HEX_POP(ctx, right);
    if (right.type == HEX_TYPE_INVALID)
    {
        HEX_FREE(ctx, right);
        return 1;
    }
    HEX_POP(ctx, left);
    if (left.type == HEX_TYPE_INVALID)
    {
        HEX_FREE(ctx, left);
        HEX_FREE(ctx, right);
        return 1;
    }
    if (left.type == HEX_TYPE_INTEGER && right.type == HEX_TYPE_INTEGER)
    {
        return hex_push_integer(ctx, left.data.int_value ^ right.data.int_value);
    }
    hex_error(ctx, "[symbol ^] Two integers required");
    HEX_FREE(ctx, left);
    HEX_FREE(ctx, right);
    return 1;
}

int hex_symbol_shiftleft(hex_context_t *ctx)
{

    HEX_POP(ctx, right);
    if (right.type == HEX_TYPE_INVALID)
    {
        HEX_FREE(ctx, right);
        return 1;
    }
    HEX_POP(ctx, left);
    if (left.type == HEX_TYPE_INVALID)
    {
        HEX_FREE(ctx, left);
        HEX_FREE(ctx, right);
        return 1;
    }
    if (left.type == HEX_TYPE_INTEGER && right.type == HEX_TYPE_INTEGER)
    {
        return hex_push_integer(ctx, left.data.int_value << right.data.int_value);
    }
    hex_error(ctx, "[symbol <<] Two integers required");
    HEX_FREE(ctx, left);
    HEX_FREE(ctx, right);
    return 1;
}

int hex_symbol_shiftright(hex_context_t *ctx)
{

    HEX_POP(ctx, right);
    if (right.type == HEX_TYPE_INVALID)
    {
        HEX_FREE(ctx, right);
        return 1;
    }
    HEX_POP(ctx, left);
    if (left.type == HEX_TYPE_INVALID)
    {
        HEX_FREE(ctx, left);
        HEX_FREE(ctx, right);
        return 1;
    }
    if (left.type == HEX_TYPE_INTEGER && right.type == HEX_TYPE_INTEGER)
    {
        return hex_push_integer(ctx, left.data.int_value >> right.data.int_value);
    }
    hex_error(ctx, "[symbol >>] Two integers required");
    HEX_FREE(ctx, left);
    HEX_FREE(ctx, right);
    return 1;
}

int hex_symbol_bitnot(hex_context_t *ctx)
{

    HEX_POP(ctx, item);
    if (item.type == HEX_TYPE_INVALID)
    {
        HEX_FREE(ctx, item);
        return 1;
    }
    if (item.type == HEX_TYPE_INTEGER)
    {
        return hex_push_integer(ctx, ~item.data.int_value);
    }
    hex_error(ctx, "[symbol ~] Integer required");
    HEX_FREE(ctx, item);
    return 1;
}

// Conversion symbols

int hex_symbol_int(hex_context_t *ctx)
{

    HEX_POP(ctx, a);
    if (a.type == HEX_TYPE_INVALID)
    {
        HEX_FREE(ctx, a);
        return 1;
    }
    if (a.type == HEX_TYPE_STRING)
    {
        return hex_push_integer(ctx, strtol(a.data.str_value, NULL, 16));
    }
    hex_error(ctx, "[symbol int] String representing a hexadecimal integer required");
    HEX_FREE(ctx, a);
    return 1;
}

int hex_symbol_str(hex_context_t *ctx)
{

    HEX_POP(ctx, a);
    if (a.type == HEX_TYPE_INVALID)
    {
        HEX_FREE(ctx, a);
        return 1;
    }
    if (a.type == HEX_TYPE_INTEGER)
    {
        return hex_push_string(ctx, hex_itoa_hex(a.data.int_value));
    }
    hex_error(ctx, "[symbol str] Integer required");
    HEX_FREE(ctx, a);
    return 1;
}

int hex_symbol_dec(hex_context_t *ctx)
{
    HEX_POP(ctx, a);
    if (a.type == HEX_TYPE_INVALID)
    {
        HEX_FREE(ctx, a);
        return 1;
    }
    if (a.type == HEX_TYPE_INTEGER)
    {
        char buffer[32];
        snprintf(buffer, sizeof(buffer), "%d", a.data.int_value);
        return hex_push_string(ctx, buffer);
    }
    hex_error(ctx, "[symbol dec] Integer required");
    HEX_FREE(ctx, a);
    return 1;
}

int hex_symbol_hex(hex_context_t *ctx)
{

    HEX_POP(ctx, item);
    if (item.type == HEX_TYPE_INVALID)
    {
        HEX_FREE(ctx, item);
        return 1;
    }
    if (item.type == HEX_TYPE_STRING)
    {
        return hex_push_integer(ctx, strtol(item.data.str_value, NULL, 10));
    }
    hex_error(ctx, "[symbol hex] String representing a decimal integer required");
    HEX_FREE(ctx, item);
    return 1;
}

int hex_symbol_ord(hex_context_t *ctx)
{
    HEX_POP(ctx, item);
    if (item.type == HEX_TYPE_INVALID)
    {
        HEX_FREE(ctx, item);
        return 1;
    }
    if (item.type == HEX_TYPE_STRING)
    {
        if (strlen(item.data.str_value) > 1)
        {
            return hex_push_integer(ctx, -1);
        }
        unsigned char *str = (unsigned char *)item.data.str_value;
        if (str[0] < 128)
        {
            return hex_push_integer(ctx, str[0]);
        }
        else
        {
            return hex_push_integer(ctx, -1);
        }
    }
    hex_error(ctx, "[symbol ord] String required");
    HEX_FREE(ctx, item);
    return 1;
}

int hex_symbol_chr(hex_context_t *ctx)
{
    HEX_POP(ctx, item);
    if (item.type == HEX_TYPE_INVALID)
    {
        HEX_FREE(ctx, item);
        return 1;
    }
    if (item.type == HEX_TYPE_INTEGER)
    {
        if (item.data.int_value >= 0 && item.data.int_value < 128)
        {
            char str[2] = {(char)item.data.int_value, '\0'};
            return hex_push_string(ctx, str);
        }
        else
        {
            return hex_push_string(ctx, "");
        }
    }
    hex_error(ctx, "[symbol chr] Integer required");
    HEX_FREE(ctx, item);
    return 1;
}

// Comparison symbols

static int hex_equal(hex_item_t a, hex_item_t b)
{
    if (a.type == HEX_TYPE_INVALID || b.type == HEX_TYPE_INVALID)
    {
        return 0;
    }
    if (a.type == HEX_TYPE_NATIVE_SYMBOL || a.type == HEX_TYPE_USER_SYMBOL)
    {
        return (strcmp(a.token->value, b.token->value) == 0);
    }
    if (a.type != b.type)
    {
        return 0;
    }
    if (a.type == HEX_TYPE_INTEGER)
    {
        return a.data.int_value == b.data.int_value;
    }
    if (a.type == HEX_TYPE_STRING)
    {
        return (strcmp(a.data.str_value, b.data.str_value) == 0);
    }
    if (a.type == HEX_TYPE_QUOTATION)
    {
        if (a.quotation_size != b.quotation_size)
        {
            return 0;
        }
        else
        {
            for (size_t i = 0; i < a.quotation_size; i++)
            {
                if (!hex_equal(*a.data.quotation_value[i], *b.data.quotation_value[i]))
                {
                    return 0;
                }
            }
            return 1;
        }
    }
    return 0;
}

static int hex_is_type_symbol(hex_item_t *item)
{
    if (item->type == HEX_TYPE_USER_SYMBOL || item->type == HEX_TYPE_NATIVE_SYMBOL)
    {
        return 1;
    }
    return 0;
}

static int hex_greater(hex_context_t *ctx, hex_item_t *a, hex_item_t *b, char *symbol)
{
    if (a->type == HEX_TYPE_INTEGER && b->type == HEX_TYPE_INTEGER)
    {
        return a->data.int_value > b->data.int_value;
    }
    else if (a->type == HEX_TYPE_STRING && b->type == HEX_TYPE_STRING)
    {
        return strcmp(a->data.str_value, b->data.str_value) > 0;
    }
    else if (a->type == HEX_TYPE_QUOTATION && b->type == HEX_TYPE_QUOTATION)
    {
        // Compare quotations lexicographically
        size_t min_size = a->quotation_size < b->quotation_size ? a->quotation_size : b->quotation_size;
        int is_greater = 0;

        for (size_t i = 0; i < min_size; i++)
        {
            hex_item_t *it_a = a->data.quotation_value[i];
            hex_item_t *it_b = b->data.quotation_value[i];

            // Perform element-wise comparison
            if (it_a->type != it_b->type && !(hex_is_type_symbol(it_a) && hex_is_type_symbol(it_b)))
            {
                // Mismatched types, return false
                return 0;
            }

            if (it_a->type == HEX_TYPE_INTEGER)
            {
                if (it_a->data.int_value != it_b->data.int_value)
                {
                    is_greater = it_a->data.int_value > it_b->data.int_value;
                    break;
                }
            }
            else if (it_a->type == HEX_TYPE_STRING)
            {
                int cmp = strcmp(it_a->data.str_value, it_b->data.str_value);
                if (cmp != 0)
                {
                    is_greater = cmp > 0;
                    break;
                }
            }
            else if (hex_is_type_symbol(it_a))
            {
                int cmp = strcmp(it_a->token->value, it_b->token->value);
                if (cmp != 0)
                {
                    is_greater = cmp > 0;
                    break;
                }
            }
            else
            {
                // Mismatched types, return false
                return 0;
            }
        }

        if (!is_greater)
        {
            // If all compared elements are equal, compare sizes
            return a->quotation_size > b->quotation_size;
        }
        return is_greater;
    }
    else
    {
        hex_error(ctx, "[symbol %s] Two integers, two strings, or two quotations required", symbol);
        return -1;
    }
}

int hex_symbol_equal(hex_context_t *ctx)
{

    HEX_POP(ctx, b);
    if (b.type == HEX_TYPE_INVALID)
    {
        HEX_FREE(ctx, b);
        return 1;
    }
    HEX_POP(ctx, a);
    if (a.type == HEX_TYPE_INVALID)
    {
        HEX_FREE(ctx, a);
        HEX_FREE(ctx, b);
        return 1;
    }
    if ((a.type == HEX_TYPE_INTEGER && b.type == HEX_TYPE_INTEGER) || (a.type == HEX_TYPE_STRING && b.type == HEX_TYPE_STRING) || (a.type == HEX_TYPE_QUOTATION && b.type == HEX_TYPE_QUOTATION))
    {
        return hex_push_integer(ctx, hex_equal(a, b));
    }
    // Different types => false
    return hex_push_integer(ctx, 0);
}

int hex_symbol_notequal(hex_context_t *ctx)
{

    HEX_POP(ctx, b);
    if (b.type == HEX_TYPE_INVALID)
    {
        HEX_FREE(ctx, b);
        return 1;
    }
    HEX_POP(ctx, a);
    if (a.type == HEX_TYPE_INVALID)
    {
        HEX_FREE(ctx, a);
        HEX_FREE(ctx, b);
        return 1;
    }
    if ((a.type == HEX_TYPE_INTEGER && b.type == HEX_TYPE_INTEGER) || (a.type == HEX_TYPE_STRING && b.type == HEX_TYPE_STRING) || (a.type == HEX_TYPE_QUOTATION && b.type == HEX_TYPE_QUOTATION))
    {
        return hex_push_integer(ctx, !hex_equal(a, b));
    }
    // Different types => true
    return hex_push_integer(ctx, 1);
}

int hex_symbol_greater(hex_context_t *ctx)
{
    HEX_POP(ctx, b);
    if (b.type == HEX_TYPE_INVALID)
    {
        HEX_FREE(ctx, b);
        return 1;
    }

    HEX_POP(ctx, a);
    if (a.type == HEX_TYPE_INVALID)
    {
        HEX_FREE(ctx, a);
        HEX_FREE(ctx, b);
        return 1;
    }
    hex_item_t *pa = &a;
    hex_item_t *pb = &b;
    hex_push_integer(ctx, hex_greater(ctx, pa, pb, ">"));
    return 0;
}

int hex_symbol_less(hex_context_t *ctx)
{

    HEX_POP(ctx, b);
    if (b.type == HEX_TYPE_INVALID)
    {
        HEX_FREE(ctx, b);
        return 1;
    }
    HEX_POP(ctx, a);
    if (a.type == HEX_TYPE_INVALID)
    {
        HEX_FREE(ctx, a);
        HEX_FREE(ctx, b);
        return 1;
    }
    hex_item_t *pa = &a;
    hex_item_t *pb = &b;
    hex_push_integer(ctx, hex_greater(ctx, pb, pa, "<"));
    return 0;
}

int hex_symbol_greaterequal(hex_context_t *ctx)
{

    HEX_POP(ctx, b);
    if (b.type == HEX_TYPE_INVALID)
    {
        HEX_FREE(ctx, b);
        return 1;
    }
    HEX_POP(ctx, a);
    if (a.type == HEX_TYPE_INVALID)
    {
        HEX_FREE(ctx, a);
        HEX_FREE(ctx, b);
        return 1;
    }
    hex_item_t *pa = &a;
    hex_item_t *pb = &b;
    hex_push_integer(ctx, hex_greater(ctx, pa, pb, ">") || hex_equal(a, b));
    return 0;
}

int hex_symbol_lessequal(hex_context_t *ctx)
{

    HEX_POP(ctx, b);
    if (b.type == HEX_TYPE_INVALID)
    {
        HEX_FREE(ctx, b);
        return 1;
    }
    HEX_POP(ctx, a);
    if (a.type == HEX_TYPE_INVALID)
    {
        HEX_FREE(ctx, a);
        HEX_FREE(ctx, b);
        return 1;
    }
    hex_item_t *pa = &a;
    hex_item_t *pb = &b;
    hex_push_integer(ctx, !hex_greater(ctx, pb, pa, "<") || hex_equal(a, b));
    return 0;
}

// Boolean symbols

int hex_symbol_and(hex_context_t *ctx)
{

    HEX_POP(ctx, b);
    if (b.type == HEX_TYPE_INVALID)
    {
        HEX_FREE(ctx, b);
        return 1;
    }
    HEX_POP(ctx, a);
    if (a.type == HEX_TYPE_INVALID)
    {
        HEX_FREE(ctx, a);
        HEX_FREE(ctx, b);
        return 1;
    }
    if (a.type == HEX_TYPE_INTEGER && b.type == HEX_TYPE_INTEGER)
    {
        return hex_push_integer(ctx, a.data.int_value && b.data.int_value);
    }
    hex_error(ctx, "[symbol and] Two integers required");
    HEX_FREE(ctx, a);
    HEX_FREE(ctx, b);
    return 1;
}

int hex_symbol_or(hex_context_t *ctx)
{

    HEX_POP(ctx, b);
    if (b.type == HEX_TYPE_INVALID)
    {
        HEX_FREE(ctx, b);
        return 1;
    }
    HEX_POP(ctx, a);
    if (a.type == HEX_TYPE_INVALID)
    {
        HEX_FREE(ctx, a);
        HEX_FREE(ctx, b);
        return 1;
    }
    if (a.type == HEX_TYPE_INTEGER && b.type == HEX_TYPE_INTEGER)
    {
        return hex_push_integer(ctx, a.data.int_value || b.data.int_value);
    }
    hex_error(ctx, "[symbol or] Two integers required");
    HEX_FREE(ctx, a);
    HEX_FREE(ctx, b);
    return 1;
}

int hex_symbol_not(hex_context_t *ctx)
{

    HEX_POP(ctx, a);
    if (a.type == HEX_TYPE_INVALID)
    {
        HEX_FREE(ctx, a);
        return 1;
    }
    if (a.type == HEX_TYPE_INTEGER)
    {
        return hex_push_integer(ctx, !a.data.int_value);
    }
    hex_error(ctx, "[symbol not] Integer required");
    HEX_FREE(ctx, a);
    return 1;
}

int hex_symbol_xor(hex_context_t *ctx)
{

    HEX_POP(ctx, b);
    if (b.type == HEX_TYPE_INVALID)
    {
        HEX_FREE(ctx, b);
        return 1;
    }
    HEX_POP(ctx, a);
    if (a.type == HEX_TYPE_INVALID)
    {
        HEX_FREE(ctx, a);
        HEX_FREE(ctx, b);
        return 1;
    }
    if (a.type == HEX_TYPE_INTEGER && b.type == HEX_TYPE_INTEGER)
    {
        return hex_push_integer(ctx, a.data.int_value ^ b.data.int_value);
    }
    hex_error(ctx, "[symbol xor] Two integers required");
    HEX_FREE(ctx, a);
    HEX_FREE(ctx, b);
    return 1;
}

// Quotation and String (List) Symbols

int hex_symbol_cat(hex_context_t *ctx)
{

    HEX_POP(ctx, value);
    if (value.type == HEX_TYPE_INVALID)
    {
        HEX_FREE(ctx, value);
        return 1; // Failed to pop value
    }

    HEX_POP(ctx, list);
    if (list.type == HEX_TYPE_INVALID)
    {
        HEX_FREE(ctx, list);
        HEX_FREE(ctx, value);
        return 1; // Failed to pop list
    }

    int result = 0;

    if (list.type == HEX_TYPE_QUOTATION && value.type == HEX_TYPE_QUOTATION)
    {
        // Concatenate two quotations
        size_t newSize = list.quotation_size + value.quotation_size;
        hex_item_t **newQuotation = (hex_item_t **)realloc(
            list.data.quotation_value, newSize * sizeof(hex_item_t *));
        if (!newQuotation)
        {
            hex_error(ctx, "[symbol cat] Memory allocation failed");
            result = 1;
        }
        else
        {
            // Append items from the second quotation
            for (size_t i = 0; i < (size_t)value.quotation_size; i++)
            {
                newQuotation[list.quotation_size + i] = value.data.quotation_value[i];
            }

            list.data.quotation_value = newQuotation;
            list.quotation_size = newSize;
            result = hex_push_quotation(ctx, list.data.quotation_value, newSize);
        }
    }
    else if (list.type == HEX_TYPE_STRING && value.type == HEX_TYPE_STRING)
    {
        // Concatenate two strings
        size_t newLength = strlen(list.data.str_value) + strlen(value.data.str_value) + 1;
        char *newStr = (char *)malloc(newLength);
        if (!newStr)
        {
            hex_error(ctx, "[symbol cat] Memory allocation failed");
            result = 1;
        }
        else
        {
            strcpy(newStr, list.data.str_value);
            strcat(newStr, value.data.str_value);
            result = hex_push_string(ctx, newStr);
        }
    }
    else
    {
        hex_error(ctx, "[symbol cat] Two quotations or two strings required");
        result = 1;
    }

    // Free resources if the operation fails
    if (result != 0)
    {
        HEX_FREE(ctx, list);
        HEX_FREE(ctx, value);
    }

    return result;
}

int hex_symbol_len(hex_context_t *ctx)
{

    HEX_POP(ctx, item);
    if (item.type == HEX_TYPE_INVALID)
    {
        HEX_FREE(ctx, item);
        return 1;
    }
    int result = 0;
    if (item.type == HEX_TYPE_QUOTATION)
    {
        result = hex_push_integer(ctx, item.quotation_size);
    }
    else if (item.type == HEX_TYPE_STRING)
    {
        result = hex_push_integer(ctx, strlen(item.data.str_value));
    }
    else
    {
        hex_error(ctx, "[symbol len] Quotation or string required");
        result = 1;
    }
    if (result != 0)
    {
        HEX_FREE(ctx, item);
    }
    return result;
}

int hex_symbol_get(hex_context_t *ctx)
{

    HEX_POP(ctx, index);
    if (index.type == HEX_TYPE_INVALID)
    {
        HEX_FREE(ctx, index);
        return 1;
    }
    HEX_POP(ctx, list);
    if (list.type == HEX_TYPE_INVALID)
    {
        HEX_FREE(ctx, list);
        HEX_FREE(ctx, index);
        return 1;
    }
    int result = 0;
    if (list.type == HEX_TYPE_QUOTATION)
    {
        if (index.type != HEX_TYPE_INTEGER)
        {
            hex_error(ctx, "[symbol get] Index must be an integer");
            result = 1;
        }
        else if (index.data.int_value < 0 || (size_t)index.data.int_value >= list.quotation_size)
        {
            hex_error(ctx, "[symbol get] Index out of range");
            result = 1;
        }
        else
        {
            result = hex_push(ctx, *list.data.quotation_value[index.data.int_value]);
        }
    }
    else if (list.type == HEX_TYPE_STRING)
    {
        if (index.type != HEX_TYPE_INTEGER)
        {
            hex_error(ctx, "[symbol get] Index must be an integer");
            result = 1;
        }
        else if (index.data.int_value < 0 || index.data.int_value >= (int)strlen(list.data.str_value))
        {
            hex_error(ctx, "[symbol get] Index out of range");
            result = 1;
        }
        else
        {
            char str[2] = {list.data.str_value[index.data.int_value], '\0'};
            result = hex_push_string(ctx, str);
        }
    }
    else
    {
        hex_error(ctx, "[symbol get] Quotation or string required");
        result = 1;
    }
    if (result != 0)
    {

        HEX_FREE(ctx, list);
        HEX_FREE(ctx, index);
    }
    return result;
}

int hex_symbol_index(hex_context_t *ctx)
{

    HEX_POP(ctx, item);
    if (item.type == HEX_TYPE_INVALID)
    {
        HEX_FREE(ctx, item);
        return 1;
    }
    HEX_POP(ctx, list);
    if (list.type == HEX_TYPE_INVALID)
    {
        HEX_FREE(ctx, list);
        HEX_FREE(ctx, item);
        return 1;
    }
    int result = -1;
    if (list.type == HEX_TYPE_QUOTATION)
    {
        for (size_t i = 0; i < list.quotation_size; i++)
        {
            if (hex_equal(*list.data.quotation_value[i], item))
            {
                result = i;
                break;
            }
        }
    }
    else if (list.type == HEX_TYPE_STRING)
    {
        char *ptr = strstr(list.data.str_value, item.data.str_value);
        if (ptr)
        {
            result = ptr - list.data.str_value;
        }
    }
    else
    {
        hex_error(ctx, "[symbol index] Quotation or string required");
        HEX_FREE(ctx, list);
        HEX_FREE(ctx, item);
        return 1;
    }
    return hex_push_integer(ctx, result);
}

// String symbols

int hex_symbol_join(hex_context_t *ctx)
{

    HEX_POP(ctx, separator);
    if (separator.type == HEX_TYPE_INVALID)
    {
        HEX_FREE(ctx, separator);
        return 1;
    }
    HEX_POP(ctx, list);
    if (list.type == HEX_TYPE_INVALID)
    {
        HEX_FREE(ctx, list);
        HEX_FREE(ctx, separator);
        return 1;
    }
    int result = 0;
    if (list.type == HEX_TYPE_QUOTATION && separator.type == HEX_TYPE_STRING)
    {
        int length = 0;
        for (size_t i = 0; i < list.quotation_size; i++)
        {
            if (list.data.quotation_value[i]->type == HEX_TYPE_STRING)
            {
                length += strlen(list.data.quotation_value[i]->data.str_value);
            }
            else
            {
                hex_error(ctx, "[symbol join] Quotation must contain only strings");
                HEX_FREE(ctx, list);
                HEX_FREE(ctx, separator);
                return 1;
            }
        }
        if (result == 0)
        {
            length += (list.quotation_size - 1) * strlen(separator.data.str_value);
            char *newStr = (char *)malloc(length + 1);
            if (!newStr)
            {
                hex_error(ctx, "[symbol join] Memory allocation failed");
                HEX_FREE(ctx, list);
                HEX_FREE(ctx, separator);
                return 1;
            }
            newStr[0] = '\0';
            for (size_t i = 0; i < list.quotation_size; i++)
            {
                strcat(newStr, list.data.quotation_value[i]->data.str_value);
                if (i < list.quotation_size - 1)
                {
                    strcat(newStr, separator.data.str_value);
                }
            }
            result = hex_push_string(ctx, newStr);
        }
    }
    else
    {
        hex_error(ctx, "[symbol join] Quotation and string required");
        result = 1;
    }
    if (result != 0)
    {
        HEX_FREE(ctx, list);
        HEX_FREE(ctx, separator);
    }
    return result;
}

int hex_symbol_split(hex_context_t *ctx)
{
    HEX_POP(ctx, separator);
    if (separator.type == HEX_TYPE_INVALID)
    {
        HEX_FREE(ctx, separator);
        return 1;
    }
    HEX_POP(ctx, str);
    if (str.type == HEX_TYPE_INVALID)
    {
        HEX_FREE(ctx, str);
        HEX_FREE(ctx, separator);
        return 1;
    }
    int result = 0;
    if (str.type == HEX_TYPE_STRING && separator.type == HEX_TYPE_STRING)
    {
        if (strlen(separator.data.str_value) == 0)
        {
            // Separator is an empty string: split into individual characters
            size_t size = strlen(str.data.str_value);
            hex_item_t **quotation = (hex_item_t **)malloc(size * sizeof(hex_item_t *));
            if (!quotation)
            {
                hex_error(ctx, "[symbol split] Memory allocation failed");
                result = 1;
            }
            else
            {
                for (size_t i = 0; i < size; i++)
                {
                    quotation[i] = (hex_item_t *)malloc(sizeof(hex_item_t));
                    if (!quotation[i])
                    {
                        hex_error(ctx, "[symbol split] Memory allocation failed");
                        result = 1;
                        break;
                    }
                    quotation[i]->type = HEX_TYPE_STRING;
                    quotation[i]->data.str_value = (char *)malloc(2); // Allocate 2 bytes: 1 for the character and 1 for null terminator
                    if (!quotation[i]->data.str_value)
                    {
                        hex_error(ctx, "[symbol split] Memory allocation failed");
                        result = 1;
                        break;
                    }
                    quotation[i]->data.str_value[0] = str.data.str_value[i]; // Copy the single character
                    quotation[i]->data.str_value[1] = '\0';                  // Null-terminate the string
                }
                if (result == 0)
                {
                    result = hex_push_quotation(ctx, quotation, size);
                }
            }
        }
        else
        {
            // Separator is not empty: split as usual
            char *token = strtok(str.data.str_value, separator.data.str_value);
            size_t capacity = 2;
            size_t size = 0;
            hex_item_t **quotation = (hex_item_t **)malloc(capacity * sizeof(hex_item_t *));
            if (!quotation)
            {
                hex_error(ctx, "[symbol split] Memory allocation failed");
                result = 1;
            }
            else
            {
                while (token)
                {
                    if (size >= capacity)
                    {
                        capacity *= 2;
                        quotation = (hex_item_t **)realloc(quotation, capacity * sizeof(hex_item_t *));
                        if (!quotation)
                        {
                            hex_error(ctx, "[symbol split] Memory allocation failed");
                            result = 1;
                            break;
                        }
                    }
                    quotation[size] = (hex_item_t *)malloc(sizeof(hex_item_t));
                    quotation[size]->type = HEX_TYPE_STRING;
                    quotation[size]->data.str_value = strdup(token);
                    size++;
                    token = strtok(NULL, separator.data.str_value);
                }
                if (result == 0)
                {
                    result = hex_push_quotation(ctx, quotation, size);
                }
            }
        }
    }
    else
    {
        hex_error(ctx, "[symbol split] Two strings required");
        result = 1;
    }
    if (result != 0)
    {
        HEX_FREE(ctx, str);
        HEX_FREE(ctx, separator);
    }
    return result;
}

int hex_symbol_replace(hex_context_t *ctx)
{

    HEX_POP(ctx, replacement);
    if (replacement.type == HEX_TYPE_INVALID)
    {
        HEX_FREE(ctx, replacement);
        return 1;
    }
    HEX_POP(ctx, search);
    if (search.type == HEX_TYPE_INVALID)
    {
        HEX_FREE(ctx, search);
        HEX_FREE(ctx, replacement);
        return 1;
    }
    HEX_POP(ctx, list);
    if (list.type == HEX_TYPE_INVALID)
    {
        HEX_FREE(ctx, list);
        HEX_FREE(ctx, search);
        HEX_FREE(ctx, replacement);
        return 1;
    }
    int result = 0;
    if (list.type == HEX_TYPE_STRING && search.type == HEX_TYPE_STRING && replacement.type == HEX_TYPE_STRING)
    {
        char *str = list.data.str_value;
        char *find = search.data.str_value;
        char *replace = replacement.data.str_value;
        char *ptr = strstr(str, find);
        if (ptr)
        {
            int findLen = strlen(find);
            int replaceLen = strlen(replace);
            int newLen = strlen(str) - findLen + replaceLen + 1;
            char *newStr = (char *)malloc(newLen);
            if (!newStr)
            {
                hex_error(ctx, "[symbol replace] Memory allocation failed");
                result = 1;
            }
            else
            {
                strncpy(newStr, str, ptr - str);
                strcpy(newStr + (ptr - str), replace);
                strcpy(newStr + (ptr - str) + replaceLen, ptr + findLen);
                result = hex_push_string(ctx, newStr);
            }
        }
        else
        {
            result = hex_push_string(ctx, str);
        }
    }
    else
    {
        hex_error(ctx, "[symbol replace] Three strings required");
        result = 1;
    }
    if (result != 0)
    {
        HEX_FREE(ctx, list);
        HEX_FREE(ctx, search);
        HEX_FREE(ctx, replacement);
    }
    return result;
}

// File symbols

int hex_symbol_read(hex_context_t *ctx)
{
    HEX_POP(ctx, filename);
    if (filename.type == HEX_TYPE_INVALID)
    {
        HEX_FREE(ctx, filename);
        return 1;
    }
    int result = 0;
    if (filename.type == HEX_TYPE_STRING)
    {
        FILE *file = fopen(filename.data.str_value, "rb");
        if (!file)
        {
            hex_error(ctx, "[symbol read] Could not open file for reading: %s", filename.data.str_value);
            result = 1;
        }
        else
        {
            fseek(file, 0, SEEK_END);
            long length = ftell(file);
            fseek(file, 0, SEEK_SET);

            uint8_t *buffer = (uint8_t *)malloc(length);
            if (!buffer)
            {
                hex_error(ctx, "[symbol read] Memory allocation failed");
                result = 1;
            }
            else
            {
                size_t bytesRead = fread(buffer, 1, length, file);
                if (hex_is_binary(buffer, bytesRead))
                {
                    hex_item_t **quotation = (hex_item_t **)malloc(bytesRead * sizeof(hex_item_t *));
                    if (!quotation)
                    {
                        hex_error(ctx, "[symbol read] Memory allocation failed");
                        result = 1;
                    }
                    else
                    {
                        for (size_t i = 0; i < bytesRead; i++)
                        {
                            quotation[i] = (hex_item_t *)malloc(sizeof(hex_item_t));
                            quotation[i]->type = HEX_TYPE_INTEGER;
                            quotation[i]->data.int_value = buffer[i];
                        }
                        result = hex_push_quotation(ctx, quotation, bytesRead);
                    }
                }
                else
                {
                    char *str = hex_bytes_to_string(buffer, bytesRead);
                    if (!str)
                    {
                        hex_error(ctx, "[symbol read] Memory allocation failed");
                        result = 1;
                    }
                    else
                    {
                        hex_item_t item = {.type = HEX_TYPE_STRING, .data.str_value = str};
                        result = HEX_PUSH(ctx, item);
                    }
                }
                free(buffer);
            }
            fclose(file);
        }
    }
    else
    {
        hex_error(ctx, "[symbol read] String required");
        result = 1;
    }
    if (result != 0)
    {
        HEX_FREE(ctx, filename);
    }
    return result;
}

int hex_symbol_write(hex_context_t *ctx)
{

    HEX_POP(ctx, filename);
    if (filename.type == HEX_TYPE_INVALID)
    {
        HEX_FREE(ctx, filename);
        return 1;
    }
    HEX_POP(ctx, data);
    if (data.type == HEX_TYPE_INVALID)
    {
        HEX_FREE(ctx, data);
        HEX_FREE(ctx, filename);
        return 1;
    }
    int result = 0;
    if (filename.type == HEX_TYPE_STRING)
    {
        if (data.type == HEX_TYPE_STRING)
        {
            FILE *file = fopen(filename.data.str_value, "w");
            if (file)
            {
                fputs(data.data.str_value, file);
                fclose(file);
                result = 0;
            }
            else
            {
                hex_error(ctx, "[symbol write] Could not open file for writing: %s", filename.data.str_value);
                result = 1;
            }
        }
        else if (data.type == HEX_TYPE_QUOTATION)
        {
            FILE *file = fopen(filename.data.str_value, "wb");
            if (file)
            {
                for (size_t i = 0; i < data.quotation_size; i++)
                {
                    if (data.data.quotation_value[i]->type != HEX_TYPE_INTEGER)
                    {
                        hex_error(ctx, "[symbol write] Quotation must contain only integers");
                        result = 1;
                        break;
                    }
                    uint8_t byte = (uint8_t)data.data.quotation_value[i]->data.int_value;
                    fwrite(&byte, 1, 1, file);
                }
                fclose(file);
                result = 0;
            }
            else
            {
                hex_error(ctx, "[symbol write] Could not open file for writing: %s", filename.data.str_value);
                result = 1;
            }
        }
        else
        {
            hex_error(ctx, "[symbol write] String or quotation of integers required");
            result = 1;
        }
    }
    else
    {
        hex_error(ctx, "[symbol write] String required");
        result = 1;
    }
    if (result != 0)
    {
        HEX_FREE(ctx, data);
        HEX_FREE(ctx, filename);
    }
    return result;
}

int hex_symbol_append(hex_context_t *ctx)
{

    HEX_POP(ctx, filename);
    if (filename.type == HEX_TYPE_INVALID)
    {
        HEX_FREE(ctx, filename);
        return 1;
    }
    HEX_POP(ctx, data);
    if (data.type == HEX_TYPE_INVALID)
    {
        HEX_FREE(ctx, data);
        HEX_FREE(ctx, filename);
        return 1;
    }
    int result = 0;
    if (filename.type == HEX_TYPE_STRING)
    {
        if (data.type == HEX_TYPE_STRING)
        {
            FILE *file = fopen(filename.data.str_value, "a");
            if (file)
            {
                fputs(data.data.str_value, file);
                fclose(file);
                result = 0;
            }
            else
            {
                hex_error(ctx, "[symbol append] Could not open file for appending: %s", filename.data.str_value);
                result = 1;
            }
        }
        else if (data.type == HEX_TYPE_QUOTATION)
        {
            FILE *file = fopen(filename.data.str_value, "ab");
            if (file)
            {
                for (size_t i = 0; i < data.quotation_size; i++)
                {
                    if (data.data.quotation_value[i]->type != HEX_TYPE_INTEGER)
                    {
                        hex_error(ctx, "[symbol append] Quotation must contain only integers");
                        result = 1;
                        break;
                    }
                    uint8_t byte = (uint8_t)data.data.quotation_value[i]->data.int_value;
                    fwrite(&byte, 1, 1, file);
                }
                fclose(file);
                result = 0;
            }
            else
            {
                hex_error(ctx, "[symbol append] Could not open file for appending: %s", filename.data.str_value);
                result = 1;
            }
        }
        else
        {
            hex_error(ctx, "[symbol append] String or quotation of integers required");
            result = 1;
        }
    }
    else
    {
        hex_error(ctx, "[symbol append] String required");
        result = 1;
    }
    if (result != 0)
    {
        HEX_FREE(ctx, data);
        HEX_FREE(ctx, filename);
    }
    return result;
}

// Shell symbols

int hex_symbol_args(hex_context_t *ctx)
{

    hex_item_t **quotation = (hex_item_t **)malloc(ctx->argc * sizeof(hex_item_t *));
    if (!quotation)
    {
        hex_error(ctx, "[symbol args] Memory allocation failed");
        return 1;
    }
    else
    {
        for (size_t i = 0; i < (size_t)ctx->argc; i++)
        {
            quotation[i] = (hex_item_t *)malloc(sizeof(hex_item_t));
            quotation[i]->type = HEX_TYPE_STRING;
            quotation[i]->data.str_value = ctx->argv[i];
        }
        if (hex_push_quotation(ctx, quotation, ctx->argc) != 0)
        {
            hex_free_list(ctx, quotation, ctx->argc);
            return 1;
        }
    }
    return 0;
}

int hex_symbol_exit(hex_context_t *ctx)
{

    HEX_POP(ctx, item);
    if (item.type == HEX_TYPE_INVALID)
    {
        HEX_FREE(ctx, item);
        return 1;
    }
    if (item.type != HEX_TYPE_INTEGER)
    {
        hex_error(ctx, "[symbol exit] Integer required");
        HEX_FREE(ctx, item);
        return 1;
    }
    int exit_status = item.data.int_value;
    exit(exit_status);
    return 0; // This line will never be reached, but it's here to satisfy the return type
}

int hex_symbol_exec(hex_context_t *ctx)
{

    HEX_POP(ctx, command);
    if (command.type == HEX_TYPE_INVALID)
    {
        HEX_FREE(ctx, command);
        return 1;
    }
    int result = 0;
    if (command.type == HEX_TYPE_STRING)
    {
        int status = system(command.data.str_value);
        result = hex_push_integer(ctx, status);
    }
    else
    {
        hex_error(ctx, "[symbol exec] String required");
        result = 1;
    }
    if (result != 0)
    {
        HEX_FREE(ctx, command);
    }
    return result;
}

int hex_symbol_run(hex_context_t *ctx)
{

    HEX_POP(ctx, command);
    if (command.type == HEX_TYPE_INVALID)
    {
        HEX_FREE(ctx, command);
        return 1;
    }
    if (command.type != HEX_TYPE_STRING)
    {
        hex_error(ctx, "[symbol run] String required");
        HEX_FREE(ctx, command);
        return 1;
    }

    char output[8192] = "";
    char error[8192] = "";
    int return_code = 0;

#ifdef _WIN32
    // Windows implementation
    HANDLE hOutputRead, hOutputWrite;
    HANDLE hErrorRead, hErrorWrite;
    SECURITY_ATTRIBUTES sa;
    PROCESS_INFORMATION pi;
    STARTUPINFO si;
    DWORD exitCode;

    sa.nLength = sizeof(SECURITY_ATTRIBUTES);
    sa.lpSecurityDescriptor = NULL;
    sa.bInheritHandle = TRUE;

    // Create pipes for capturing stdout and stderr
    if (!CreatePipe(&hOutputRead, &hOutputWrite, &sa, 0) || !CreatePipe(&hErrorRead, &hErrorWrite, &sa, 0))
    {
        hex_error(ctx, "[symbol run] Failed to create pipes");
        HEX_FREE(ctx, command);
        return 1;
    }

    // Set up STARTUPINFO structure
    ZeroMemory(&si, sizeof(STARTUPINFO));
    si.cb = sizeof(STARTUPINFO);
    si.hStdOutput = hOutputWrite;
    si.hStdError = hErrorWrite;
    si.dwFlags |= STARTF_USESTDHANDLES;

    // Create the child process
    if (!CreateProcess(NULL, command.data.str_value, NULL, NULL, TRUE, 0, NULL, NULL, &si, &pi))
    {
        hex_error(ctx, "[symbol run] Failed to create process");
        HEX_FREE(ctx, command);
        return 1;
    }

    // Close write ends of the pipes
    CloseHandle(hOutputWrite);
    CloseHandle(hErrorWrite);

    // Read stdout
    DWORD bytesRead;
    char buffer[1024];
    while (ReadFile(hOutputRead, buffer, sizeof(buffer) - 1, &bytesRead, NULL) && bytesRead > 0)
    {
        buffer[bytesRead] = '\0';
        strcat(output, buffer);
    }

    // Read stderr
    while (ReadFile(hErrorRead, buffer, sizeof(buffer) - 1, &bytesRead, NULL) && bytesRead > 0)
    {
        buffer[bytesRead] = '\0';
        strcat(error, buffer);
    }

    // Wait for the child process to finish and get the return code
    WaitForSingleObject(pi.hProcess, INFINITE);
    GetExitCodeProcess(pi.hProcess, &exitCode);
    return_code = exitCode;

    // Close handles
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    CloseHandle(hOutputRead);
    CloseHandle(hErrorRead);
#else
    // POSIX implementation (Linux/macOS)
    int stdout_pipe[2];
    int stderr_pipe[2];
    if (pipe(stdout_pipe) != 0 || pipe(stderr_pipe) != 0)
    {
        hex_error(ctx, "[symbol run] Failed to create pipes");
        HEX_FREE(ctx, command);
        return 1;
    }

    pid_t pid = fork();
    if (pid == -1)
    {
        hex_error(ctx, "[symbol run] Failed to fork process");
        HEX_FREE(ctx, command);
        return 1;
    }
    else if (pid == 0)
    {
        // Child process
        close(stdout_pipe[0]);
        close(stderr_pipe[0]);
        dup2(stdout_pipe[1], STDOUT_FILENO);
        dup2(stderr_pipe[1], STDERR_FILENO);
        execl("/bin/sh", "sh", "-c", command.data.str_value, (char *)NULL);
        exit(1);
    }
    else
    {
        // Parent process
        close(stdout_pipe[1]);
        close(stderr_pipe[1]);

        // Read stdout
        FILE *stdout_fp = fdopen(stdout_pipe[0], "r");
        char path[1035];
        while (fgets(path, sizeof(path), stdout_fp) != NULL)
        {
            strcat(output, path);
        }
        fclose(stdout_fp);

        // Read stderr
        FILE *stderr_fp = fdopen(stderr_pipe[0], "r");
        while (fgets(path, sizeof(path), stderr_fp) != NULL)
        {
            strcat(error, path);
        }
        fclose(stderr_fp);

        // Wait for child process to finish and get the return code
        int status;
        waitpid(pid, &status, 0);
        return_code = WEXITSTATUS(status);
    }
#endif

    // Push the return code, output, and error as a quotation
    hex_item_t **quotation = (hex_item_t **)malloc(3 * sizeof(hex_item_t *));
    quotation[0] = (hex_item_t *)malloc(sizeof(hex_item_t));
    quotation[0]->type = HEX_TYPE_INTEGER;
    quotation[0]->data.int_value = return_code;

    quotation[1] = (hex_item_t *)malloc(sizeof(hex_item_t));
    quotation[1]->type = HEX_TYPE_STRING;
    quotation[1]->data.str_value = strdup(output);

    quotation[2] = (hex_item_t *)malloc(sizeof(hex_item_t));
    quotation[2]->type = HEX_TYPE_STRING;
    quotation[2]->data.str_value = strdup(error);

    return hex_push_quotation(ctx, quotation, 3);
}

// Control flow symbols

int hex_symbol_if(hex_context_t *ctx)
{

    HEX_POP(ctx, elseBlock);
    if (elseBlock.type == HEX_TYPE_INVALID)
    {
        HEX_FREE(ctx, elseBlock);
        return 1;
    }
    HEX_POP(ctx, thenBlock);
    if (thenBlock.type == HEX_TYPE_INVALID)
    {
        HEX_FREE(ctx, thenBlock);
        HEX_FREE(ctx, elseBlock);
        return 1;
    }
    HEX_POP(ctx, condition);
    if (condition.type == HEX_TYPE_INVALID)
    {
        HEX_FREE(ctx, condition);
        HEX_FREE(ctx, thenBlock);
        HEX_FREE(ctx, elseBlock);
        return 1;
    }
    if (condition.type != HEX_TYPE_QUOTATION || thenBlock.type != HEX_TYPE_QUOTATION || elseBlock.type != HEX_TYPE_QUOTATION)
    {
        hex_error(ctx, "[symbol if] Three quotations required");
        return 1;
    }
    else
    {
        for (size_t i = 0; i < condition.quotation_size; i++)
        {
            if (hex_push(ctx, *condition.data.quotation_value[i]) != 0)
            {
                HEX_FREE(ctx, condition);
                HEX_FREE(ctx, thenBlock);
                HEX_FREE(ctx, elseBlock);
                return 1;
            }
        }
        HEX_POP(ctx, evalResult);
        if (evalResult.type == HEX_TYPE_INTEGER && evalResult.data.int_value > 0)
        {
            for (size_t i = 0; i < thenBlock.quotation_size; i++)
            {
                if (hex_push(ctx, *thenBlock.data.quotation_value[i]) != 0)
                {
                    HEX_FREE(ctx, condition);
                    HEX_FREE(ctx, thenBlock);
                    HEX_FREE(ctx, elseBlock);
                    return 1;
                }
            }
        }
        else
        {
            for (size_t i = 0; i < elseBlock.quotation_size; i++)
            {
                if (hex_push(ctx, *elseBlock.data.quotation_value[i]) != 0)
                {
                    HEX_FREE(ctx, condition);
                    HEX_FREE(ctx, thenBlock);
                    HEX_FREE(ctx, elseBlock);
                    return 1;
                }
            }
        }
    }
    return 0;
}

int hex_symbol_when(hex_context_t *ctx)
{

    HEX_POP(ctx, action);
    if (action.type == HEX_TYPE_INVALID)
    {
        HEX_FREE(ctx, action);
        return 1;
    }
    HEX_POP(ctx, condition);
    if (condition.type == HEX_TYPE_INVALID)
    {
        HEX_FREE(ctx, action);
        HEX_FREE(ctx, condition);
        return 1;
    }
    int result = 0;
    if (condition.type != HEX_TYPE_QUOTATION || action.type != HEX_TYPE_QUOTATION)
    {
        hex_error(ctx, "[symbol when] Two quotations required");
        result = 1;
    }
    else
    {
        for (size_t i = 0; i < condition.quotation_size; i++)
        {
            if (hex_push(ctx, *condition.data.quotation_value[i]) != 0)
            {
                result = 1;
                break; // Break if pushing the item failed
            }
        }
        HEX_POP(ctx, evalResult);
        if (evalResult.type == HEX_TYPE_INTEGER && evalResult.data.int_value > 0)
        {
            for (size_t i = 0; i < action.quotation_size; i++)
            {
                if (hex_push(ctx, *action.data.quotation_value[i]) != 0)
                {
                    result = 1;
                    break;
                }
            }
        }
    }
    if (result != 0)
    {
        HEX_FREE(ctx, action);
        HEX_FREE(ctx, condition);
    }
    return result;
}

int hex_symbol_while(hex_context_t *ctx)
{

    HEX_POP(ctx, action);
    if (action.type == HEX_TYPE_INVALID)
    {
        HEX_FREE(ctx, action);
        return 1;
    }
    HEX_POP(ctx, condition);
    if (condition.type == HEX_TYPE_INVALID)
    {
        HEX_FREE(ctx, action);
        HEX_FREE(ctx, condition);
        return 1;
    }
    if (condition.type != HEX_TYPE_QUOTATION || action.type != HEX_TYPE_QUOTATION)
    {
        hex_error(ctx, "[symbol while] Two quotations required");
        HEX_FREE(ctx, action);
        HEX_FREE(ctx, condition);
        return 1;
    }
    else
    {
        while (1)
        {
            for (size_t i = 0; i < condition.quotation_size; i++)
            {
                if (hex_push(ctx, *condition.data.quotation_value[i]) != 0)
                {
                    HEX_FREE(ctx, action);
                    HEX_FREE(ctx, condition);
                    return 1;
                }
            }
            HEX_POP(ctx, evalResult);
            if (evalResult.type == HEX_TYPE_INTEGER && evalResult.data.int_value == 0)
            {
                break;
            }
            for (size_t i = 0; i < action.quotation_size; i++)
            {
                if (hex_push(ctx, *action.data.quotation_value[i]) != 0)
                {
                    HEX_FREE(ctx, action);
                    HEX_FREE(ctx, condition);
                    return 1;
                }
            }
        }
    }
    return 0;
}

/*int hex_symbol_each(hex_context_t *ctx)
{

    HEX_POP(ctx, action);
    if (action.type == HEX_TYPE_INVALID)
    {
        HEX_FREE(ctx, action);
        return 1;
    }
    HEX_POP(ctx, list);
    if (list.type == HEX_TYPE_INVALID)
    {
        HEX_FREE(ctx, action);
        HEX_FREE(ctx, list);
        return 1;
    }
    if (list.type != HEX_TYPE_QUOTATION || action.type != HEX_TYPE_QUOTATION)
    {
        hex_error(ctx, "[symbol each] Two quotations required");
        HEX_FREE(ctx, action);
        HEX_FREE(ctx, list);
        return 1;
    }
    else
    {
        for (size_t i = 0; i < list.quotation_size; i++)
        {
            if (hex_push(ctx, *list.data.quotation_value[i]) != 0)
            {
                HEX_FREE(ctx, action);
                HEX_FREE(ctx, list);
                return 1;
            }
            for (size_t j = 0; j < action.quotation_size; j++)
            {
                if (hex_push(ctx, *action.data.quotation_value[j]) != 0)
                {
                    HEX_FREE(ctx, action);
                    HEX_FREE(ctx, list);
                    return 1;
                }
            }
        }
    }
    return 0;
}*/

int hex_symbol_error(hex_context_t *ctx)
{

    char *message = strdup(ctx->error);
    ctx->error[0] = '\0';
    return hex_push_string(ctx, message);
}

int hex_symbol_try(hex_context_t *ctx)
{

    HEX_POP(ctx, catch_block);
    if (catch_block.type == HEX_TYPE_INVALID)
    {
        HEX_FREE(ctx, catch_block);
        return 1;
    }
    HEX_POP(ctx, try_block);
    if (try_block.type == HEX_TYPE_INVALID)
    {
        HEX_FREE(ctx, catch_block);
        HEX_FREE(ctx, try_block);
        return 1;
    }
    if (try_block.type != HEX_TYPE_QUOTATION || catch_block.type != HEX_TYPE_QUOTATION)
    {
        hex_error(ctx, "[symbol try] Two quotations required");
        HEX_FREE(ctx, catch_block);
        HEX_FREE(ctx, try_block);
        return 1;
    }
    else
    {
        char prevError[256];
        strncpy(prevError, ctx->error, sizeof(ctx->error));
        ctx->error[0] = '\0';

        ctx->settings.errors_enabled = 0;
        for (size_t i = 0; i < try_block.quotation_size; i++)
        {
            if (hex_push(ctx, *try_block.data.quotation_value[i]) != 0)
            {
                ctx->settings.errors_enabled = 1;
            }
        }
        ctx->settings.errors_enabled = 1;

        if (strcmp(ctx->error, ""))
        {
            for (size_t i = 0; i < catch_block.quotation_size; i++)
            {
                if (hex_push(ctx, *catch_block.data.quotation_value[i]) != 0)
                {
                    HEX_FREE(ctx, catch_block);
                    HEX_FREE(ctx, try_block);
                    return 1;
                }
            }
        }

        strncpy(ctx->error, prevError, sizeof(ctx->error));
    }
    return 0;
}

// Quotation symbols

int hex_symbol_q(hex_context_t *ctx)
{

    HEX_POP(ctx, item);
    if (item.type == HEX_TYPE_INVALID)
    {
        HEX_FREE(ctx, item);
        return 1;
    }

    hex_item_t *quotation = (hex_item_t *)malloc(sizeof(hex_item_t));
    if (!quotation)
    {
        hex_error(ctx, "[symbol '] Memory allocation failed");
        HEX_FREE(ctx, item);
        return 1;
    }

    *quotation = item;

    hex_item_t result;
    result.type = HEX_TYPE_QUOTATION;
    result.data.quotation_value = (hex_item_t **)malloc(sizeof(hex_item_t *));
    if (!result.data.quotation_value)
    {
        HEX_FREE(ctx, item);
        free(quotation);
        hex_error(ctx, "[symbol '] Memory allocation failed");
        return 1;
    }

    result.data.quotation_value[0] = quotation;
    result.quotation_size = 1;

    if (HEX_PUSH(ctx, result) != 0)
    {
        HEX_FREE(ctx, item);
        free(quotation);
        free(result.data.quotation_value);
        return 1;
    }

    return 0;
}

int hex_symbol_map(hex_context_t *ctx)
{

    HEX_POP(ctx, action);
    if (action.type == HEX_TYPE_INVALID)
    {
        HEX_FREE(ctx, action);
        return 1;
    }
    HEX_POP(ctx, list);
    if (list.type == HEX_TYPE_INVALID)
    {
        HEX_FREE(ctx, action);
        HEX_FREE(ctx, list);
        return 1;
    }
    if (list.type != HEX_TYPE_QUOTATION || action.type != HEX_TYPE_QUOTATION)
    {
        hex_error(ctx, "[symbol map] Two quotations required");
        HEX_FREE(ctx, action);
        HEX_FREE(ctx, list);
        return 1;
    }
    else
    {
        hex_item_t **quotation = (hex_item_t **)malloc(list.quotation_size * sizeof(hex_item_t *));
        if (!quotation)
        {
            hex_error(ctx, "[symbol map] Memory allocation failed");
            HEX_FREE(ctx, action);
            HEX_FREE(ctx, list);
            return 1;
        }
        for (size_t i = 0; i < list.quotation_size; i++)
        {
            if (hex_push(ctx, *list.data.quotation_value[i]) != 0)
            {
                HEX_FREE(ctx, action);
                HEX_FREE(ctx, list);
                hex_free_list(ctx, quotation, i);
                return 1;
            }
            for (size_t j = 0; j < action.quotation_size; j++)
            {
                if (hex_push(ctx, *action.data.quotation_value[j]) != 0)
                {
                    HEX_FREE(ctx, action);
                    HEX_FREE(ctx, list);
                    hex_free_list(ctx, quotation, i);
                    return 1;
                }
            }
            quotation[i] = (hex_item_t *)malloc(sizeof(hex_item_t));
            *quotation[i] = hex_pop(ctx);
        }
        if (hex_push_quotation(ctx, quotation, list.quotation_size) != 0)
        {
            HEX_FREE(ctx, action);
            HEX_FREE(ctx, list);
            hex_free_list(ctx, quotation, list.quotation_size);
            return 1;
        }
    }
    return 0;
}

int hex_symbol_filter(hex_context_t *ctx)
{

    HEX_POP(ctx, action);
    if (action.type == HEX_TYPE_INVALID)
    {
        HEX_FREE(ctx, action);
        return 1;
    }
    HEX_POP(ctx, list);
    if (list.type == HEX_TYPE_INVALID)
    {
        HEX_FREE(ctx, action);
        HEX_FREE(ctx, list);
        return 1;
    }
    if (list.type != HEX_TYPE_QUOTATION || action.type != HEX_TYPE_QUOTATION)
    {
        hex_error(ctx, "[symbol filter] Two quotations required");
        HEX_FREE(ctx, action);
        HEX_FREE(ctx, list);
        return 1;
    }
    else
    {
        hex_item_t **quotation = (hex_item_t **)malloc(list.quotation_size * sizeof(hex_item_t *));
        if (!quotation)
        {
            hex_error(ctx, "[symbol filter] Memory allocation failed");
            HEX_FREE(ctx, action);
            HEX_FREE(ctx, list);
            return 1;
        }
        size_t count = 0;
        for (size_t i = 0; i < list.quotation_size; i++)
        {
            if (hex_push(ctx, *list.data.quotation_value[i]) != 0)
            {
                HEX_FREE(ctx, action);
                HEX_FREE(ctx, list);
                hex_free_list(ctx, quotation, count);
                return 1;
            }
            for (size_t j = 0; j < action.quotation_size; j++)
            {
                if (hex_push(ctx, *action.data.quotation_value[j]) != 0)
                {
                    HEX_FREE(ctx, action);
                    HEX_FREE(ctx, list);
                    hex_free_list(ctx, quotation, count);
                    return 1;
                }
            }
            HEX_POP(ctx, evalResult);
            if (evalResult.type == HEX_TYPE_INTEGER && evalResult.data.int_value > 0)
            {
                quotation[count] = (hex_item_t *)malloc(sizeof(hex_item_t));
                if (!quotation[count])
                {
                    hex_error(ctx, "[symbol filter] Memory allocation failed");
                    HEX_FREE(ctx, action);
                    HEX_FREE(ctx, list);
                    hex_free_list(ctx, quotation, count);
                    return 1;
                }
                *quotation[count] = *list.data.quotation_value[i];
                count++;
            }
        }
        if (hex_push_quotation(ctx, quotation, count) != 0)
        {
            hex_error(ctx, "[symbol filter] An error occurred while pushing quotation");
            HEX_FREE(ctx, action);
            HEX_FREE(ctx, list);
            for (size_t i = 0; i < count; i++)
            {
                HEX_FREE(ctx, *quotation[i]);
            }
            return 1;
        }
    }
    return 0;
}

// Stack manipulation symbols

int hex_symbol_swap(hex_context_t *ctx)
{

    HEX_POP(ctx, a);
    if (a.type == HEX_TYPE_INVALID)
    {
        HEX_FREE(ctx, a);
        return 1;
    }
    HEX_POP(ctx, b);
    if (b.type == HEX_TYPE_INVALID)
    {
        HEX_FREE(ctx, b);
        HEX_FREE(ctx, a);
        return 1;
    }
    if (HEX_PUSH(ctx, a) != 0)
    {
        HEX_FREE(ctx, a);
        HEX_FREE(ctx, b);
        return 1;
    }
    if (HEX_PUSH(ctx, b) != 0)
    {
        HEX_FREE(ctx, a);
        HEX_FREE(ctx, b);
        return 1;
    }
    return 0;
}

int hex_symbol_dup(hex_context_t *ctx)
{

    HEX_POP(ctx, item);
    if (item.type == HEX_TYPE_INVALID)
    {
        HEX_FREE(ctx, item);
        return 1;
    }
    if (HEX_PUSH(ctx, item) == 0 && HEX_PUSH(ctx, item) == 0)
    {
        return 0;
    }
    HEX_FREE(ctx, item);
    return 1;
}

int hex_symbol_stack(hex_context_t *ctx)
{

    hex_item_t **quotation = (hex_item_t **)malloc((ctx->stack.top + 1) * sizeof(hex_item_t *));
    if (!quotation)
    {
        hex_error(ctx, "[symbol stack] Memory allocation failed");
        return 1;
    }
    int count = 0;
    for (size_t i = 0; i <= (size_t)ctx->stack.top + 1; i++)
    {
        quotation[i] = (hex_item_t *)malloc(sizeof(hex_item_t));
        if (!quotation[i])
        {
            hex_error(ctx, "[symbol stack] Memory allocation failed");
            hex_free_list(ctx, quotation, count);
            return 1;
        }
        *quotation[i] = ctx->stack.entries[i];
        count++;
    }

    if (hex_push_quotation(ctx, quotation, ctx->stack.top + 1) != 0)
    {
        hex_error(ctx, "[symbol stack] An error occurred while pushing quotation");
        hex_free_list(ctx, quotation, count);
        return 1;
    }
    return 0;
}

int hex_symbol_clear(hex_context_t *ctx)
{
    for (size_t i = 0; i <= (size_t)ctx->stack.top; i++)
    {
        HEX_FREE(ctx, ctx->stack.entries[i]);
    }
    ctx->stack.top = -1;
    return 0;
}

int hex_symbol_pop(hex_context_t *ctx)
{

    HEX_POP(ctx, item);
    HEX_FREE(ctx, item);
    return 0;
}

////////////////////////////////////////
// Native Symbol Registration         //
////////////////////////////////////////

void hex_register_symbols(hex_context_t *ctx)
{
    hex_set_native_symbol(ctx, ":", hex_symbol_store);
    hex_set_native_symbol(ctx, "::", hex_symbol_define);
    hex_set_native_symbol(ctx, "#", hex_symbol_free);
    hex_set_native_symbol(ctx, "type", hex_symbol_type);
    hex_set_native_symbol(ctx, ".", hex_symbol_i);
    hex_set_native_symbol(ctx, "!", hex_symbol_eval);
    hex_set_native_symbol(ctx, "puts", hex_symbol_puts);
    hex_set_native_symbol(ctx, "warn", hex_symbol_warn);
    hex_set_native_symbol(ctx, "print", hex_symbol_print);
    hex_set_native_symbol(ctx, "gets", hex_symbol_gets);
    hex_set_native_symbol(ctx, "+", hex_symbol_add);
    hex_set_native_symbol(ctx, "-", hex_symbol_subtract);
    hex_set_native_symbol(ctx, "*", hex_symbol_multiply);
    hex_set_native_symbol(ctx, "/", hex_symbol_divide);
    hex_set_native_symbol(ctx, "%", hex_symbol_modulo);
    hex_set_native_symbol(ctx, "&", hex_symbol_bitand);
    hex_set_native_symbol(ctx, "|", hex_symbol_bitor);
    hex_set_native_symbol(ctx, "^", hex_symbol_bitxor);
    hex_set_native_symbol(ctx, "~", hex_symbol_bitnot);
    hex_set_native_symbol(ctx, "<<", hex_symbol_shiftleft);
    hex_set_native_symbol(ctx, ">>", hex_symbol_shiftright);
    hex_set_native_symbol(ctx, "int", hex_symbol_int);
    hex_set_native_symbol(ctx, "str", hex_symbol_str);
    hex_set_native_symbol(ctx, "dec", hex_symbol_dec);
    hex_set_native_symbol(ctx, "hex", hex_symbol_hex);
    hex_set_native_symbol(ctx, "chr", hex_symbol_chr);
    hex_set_native_symbol(ctx, "ord", hex_symbol_ord);
    hex_set_native_symbol(ctx, "==", hex_symbol_equal);
    hex_set_native_symbol(ctx, "!=", hex_symbol_notequal);
    hex_set_native_symbol(ctx, ">", hex_symbol_greater);
    hex_set_native_symbol(ctx, "<", hex_symbol_less);
    hex_set_native_symbol(ctx, ">=", hex_symbol_greaterequal);
    hex_set_native_symbol(ctx, "<=", hex_symbol_lessequal);
    hex_set_native_symbol(ctx, "and", hex_symbol_and);
    hex_set_native_symbol(ctx, "or", hex_symbol_or);
    hex_set_native_symbol(ctx, "not", hex_symbol_not);
    hex_set_native_symbol(ctx, "xor", hex_symbol_xor);
    hex_set_native_symbol(ctx, "cat", hex_symbol_cat);
    hex_set_native_symbol(ctx, "len", hex_symbol_len);
    hex_set_native_symbol(ctx, "get", hex_symbol_get);
    hex_set_native_symbol(ctx, "index", hex_symbol_index);
    hex_set_native_symbol(ctx, "join", hex_symbol_join);
    hex_set_native_symbol(ctx, "split", hex_symbol_split);
    hex_set_native_symbol(ctx, "replace", hex_symbol_replace);
    hex_set_native_symbol(ctx, "read", hex_symbol_read);
    hex_set_native_symbol(ctx, "write", hex_symbol_write);
    hex_set_native_symbol(ctx, "append", hex_symbol_append);
    hex_set_native_symbol(ctx, "args", hex_symbol_args);
    hex_set_native_symbol(ctx, "exit", hex_symbol_exit);
    hex_set_native_symbol(ctx, "exec", hex_symbol_exec);
    hex_set_native_symbol(ctx, "run", hex_symbol_run);
    hex_set_native_symbol(ctx, "if", hex_symbol_if);
    hex_set_native_symbol(ctx, "when", hex_symbol_when);
    hex_set_native_symbol(ctx, "while", hex_symbol_while);
    hex_set_native_symbol(ctx, "error", hex_symbol_error);
    hex_set_native_symbol(ctx, "try", hex_symbol_try);
    hex_set_native_symbol(ctx, "'", hex_symbol_q);
    hex_set_native_symbol(ctx, "map", hex_symbol_map);
    hex_set_native_symbol(ctx, "filter", hex_symbol_filter);
    hex_set_native_symbol(ctx, "swap", hex_symbol_swap);
    hex_set_native_symbol(ctx, "dup", hex_symbol_dup);
    hex_set_native_symbol(ctx, "stack", hex_symbol_stack);
    hex_set_native_symbol(ctx, "clear", hex_symbol_clear);
    hex_set_native_symbol(ctx, "pop", hex_symbol_pop);
}

#ifndef HEX_H
#include "hex.h"
#endif

////////////////////////////////////////
// Helper Functions                   //
////////////////////////////////////////

#if defined(__EMSCRIPTEN__)
#include <emscripten.h>

EM_ASYNC_JS(char *, em_fgets, (const char *buf, size_t bufsize), {
    return await new Promise(function(resolve, reject) {
               if (Module.pending_lines.length > 0)
               {
                   resolve(Module.pending_lines.shift());
               }
               else
               {
                   Module.pending_fgets.push(resolve);
               }
           })
        .then(function(s) {
            // convert JS string to WASM string
            let l = s.length + 1;
            if (l >= bufsize)
            {
                // truncate
                l = bufsize - 1;
            }
            Module.stringToUTF8(s.slice(0, l), buf, l);
            return buf;
        });
});

#endif

static void hex_rpad(const char *str, int total_length)
{
    int len = strlen(str);
    printf("%s", str);
    for (int i = len; i < total_length; i++)
    {
        printf(" ");
    }
}
static void hex_lpad(const char *str, int total_length)
{
    int len = strlen(str);
    for (int i = len; i < total_length; i++)
    {
        printf(" ");
    }
    printf("%s", str);
}

char *hex_itoa(int num, int base)
{
    static char str[20];
    int i = 0;

    // Handle 0 explicitly, otherwise empty string is printed
    if (num == 0)
    {
        str[i++] = '0';
        str[i] = '\0';
        return str;
    }

    // Process each digit
    while (num != 0)
    {
        int rem = num % base;
        str[i++] = (rem > 9) ? (rem - 10) + 'a' : rem + '0';
        num = num / base;
    }

    str[i] = '\0'; // Null-terminate string

    // Reverse the string
    int start = 0;
    int end = i - 1;
    while (start < end)
    {
        char temp = str[start];
        str[start] = str[end];
        str[end] = temp;
        start++;
        end--;
    }

    return str;
}

char *hex_itoa_dec(int num)
{
    return hex_itoa(num, 10);
}

char *hex_itoa_hex(int num)
{
    return hex_itoa(num, 16);
}

void hex_raw_print_item(FILE *stream, hex_item_t item)
{
    switch (item.type)
    {
    case HEX_TYPE_INTEGER:
        fprintf(stream, "0x%x", item.data.int_value);
        break;
    case HEX_TYPE_STRING:
        fprintf(stream, "%s", item.data.str_value);
        break;
    case HEX_TYPE_USER_SYMBOL:
    case HEX_TYPE_NATIVE_SYMBOL:
        fprintf(stream, "%s", item.token->data.value);
        break;
    case HEX_TYPE_QUOTATION:
        fprintf(stream, "(");
        for (size_t i = 0; i < item.quotation_size; i++)
        {
            if (i > 0)
            {
                fprintf(stream, " ");
            }
            hex_print_item(stream, *item.data.quotation_value[i]);
        }
        fprintf(stream, ")");
        break;

    case HEX_TYPE_INVALID:
        fprintf(stream, "<invalid>");
        break;
    default:
        fprintf(stream, "<unknown>");
        break;
    }
}

void hex_print_item(FILE *stream, hex_item_t item)
{
    switch (item.type)
    {
    case HEX_TYPE_INTEGER:
        fprintf(stream, "0x%x", item.data.int_value);
        break;

    case HEX_TYPE_STRING:
        fprintf(stream, "\"");
        for (char *c = item.data.str_value; *c != '\0'; c++)
        {
            switch (*c)
            {
            case '\n':
                fprintf(stream, "\\n");
                break;
            case '\t':
                fprintf(stream, "\\t");
                break;
            case '\r':
                fprintf(stream, "\\r");
                break;
            case '\b':
                fprintf(stream, "\\b");
                break;
            case '\f':
                fprintf(stream, "\\f");
                break;
            case '\v':
                fprintf(stream, "\\v");
                break;
            case '\\':
                fprintf(stream, "\\\\");
                break;
            case '\"':
                fprintf(stream, "\\\"");
                break;
            default:
                if ((unsigned char)*c < 32 || (unsigned char)*c > 126)
                {
                    // Escape non-printable characters as hex (e.g., \x1F)
                    fprintf(stream, "\\x%02x", (unsigned char)*c);
                }
                else
                {
                    fputc(*c, stream);
                }
                break;
            }
        }
        fprintf(stream, "\"");
        break;

    case HEX_TYPE_USER_SYMBOL:
    case HEX_TYPE_NATIVE_SYMBOL:
        fprintf(stream, "%s", item.token->data.value);
        break;

    case HEX_TYPE_QUOTATION:
        fprintf(stream, "(");
        for (size_t i = 0; i < item.quotation_size; i++)
        {
            if (i > 0)
            {
                fprintf(stream, " ");
            }
            hex_print_item(stream, *item.data.quotation_value[i]);
        }
        fprintf(stream, ")");
        break;

    case HEX_TYPE_INVALID:
        fprintf(stream, "<invalid>");
        break;
    default:
        fprintf(stream, "<unknown>");
        break;
    }
}

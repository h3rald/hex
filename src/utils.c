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

void hex_rpad(const char *str, int total_length)
{
    int len = strlen(str);
    printf("%s", str);
    for (int i = len; i < total_length; i++)
    {
        printf(" ");
    }
}
void hex_lpad(const char *str, int total_length)
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
        fprintf(stream, "%s", item.token->value);
        break;
    case HEX_TYPE_QUOTATION:
        fprintf(stream, "(");
        for (size_t i = 0; i < item.quotation_size; i++)
        {
            if (i > 0)
            {
                fprintf(stream, " ");
            }
            hex_print_item(stream, item.data.quotation_value[i]);
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

void hex_encode_length(uint8_t **bytecode, size_t *size, size_t length)
{
    while (length >= 0x80)
    {
        (*bytecode)[*size] = (length & 0x7F) | 0x80;
        length >>= 7;
        (*size)++;
    }
    (*bytecode)[*size] = length & 0x7F;
    (*size)++;
}

int hex_is_binary(const uint8_t *data, size_t size)
{
    const double binary_threshold = 0.1; // 10% of bytes being non-printable
    size_t non_printable_count = 0;
    for (size_t i = 0; i < size; i++)
    {
        uint8_t byte = data[i];
        // Check if the byte is a printable ASCII character or a common control character.
        if (!((byte >= 32 && byte <= 126) || byte == 9 || byte == 10 || byte == 13))
        {
            non_printable_count++;
        }
        // Early exit if the threshold is exceeded.
        if ((double)non_printable_count / size > binary_threshold)
        {
            return 1;
        }
    }
    return 0;
}

void hex_print_string(FILE *stream, char *value)
{
    fprintf(stream, "\"");
    for (char *c = value; *c != '\0'; c++)
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
            fputc(*c, stream);
            break;
        }
    }
    fprintf(stream, "\"");
}

void hex_print_item(FILE *stream, hex_item_t *item)
{
    switch (item->type)
    {
    case HEX_TYPE_INTEGER:
        fprintf(stream, "0x%x", item->data.int_value);
        break;

    case HEX_TYPE_STRING:
        hex_print_string(stream, item->data.str_value);
        break;

    case HEX_TYPE_USER_SYMBOL:
    case HEX_TYPE_NATIVE_SYMBOL:
        fprintf(stream, "%s", item->token->value);
        break;

    case HEX_TYPE_QUOTATION:
        fprintf(stream, "(");
        for (size_t i = 0; i < item->quotation_size; i++)
        {
            if (i > 0)
            {
                fprintf(stream, " ");
            }
            hex_print_item(stream, item->data.quotation_value[i]);
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

char *hex_bytes_to_string(const uint8_t *bytes, size_t size)
{
    char *str = (char *)malloc(size * 6 + 1); // Allocate enough space for worst case (\uXXXX format)
    if (!str)
    {
        return NULL; // Allocation failed
    }

    char *ptr = str;
    for (size_t i = 0; i < size; i++)
    {
        uint8_t byte = bytes[i];
        switch (byte)
        {
        case '\n':
            *ptr++ = '\\';
            *ptr++ = 'n';
            break;
        case '\t':
            *ptr++ = '\\';
            *ptr++ = 't';
            break;
        case '\r':
            if (i + 1 < size && bytes[i + 1] == '\n')
            {
                i++; // Skip the '\n' part of the '\r\n' sequence
            }
            *ptr++ = '\\';
            *ptr++ = 'n';
            break;
        case '\b':
            *ptr++ = '\\';
            *ptr++ = 'b';
            break;
        case '\f':
            *ptr++ = '\\';
            *ptr++ = 'f';
            break;
        case '\v':
            *ptr++ = '\\';
            *ptr++ = 'v';
            break;
        case '\\':
            *ptr++ = '\\';
            break;
        case '\"':
            *ptr++ = '\\';
            *ptr++ = '\"';
            break;
        default:
            *ptr++ = byte;
            break;
        }
    }
    *ptr = '\0';
    return str;
}

char *hex_process_string(const char *value)
{
    int len = strlen(value);
    char *processed_str = (char *)malloc(len + 1);
    if (!processed_str)
    {
        return NULL;
    }

    char *dst = processed_str;
    const char *src = value;
    while (*src)
    {
        if (*src == '\\' && *(src + 1))
        {
            src++;
            switch (*src)
            {
            case 'n':
                *dst++ = '\n';
                break;
            case 't':
                *dst++ = '\t';
                break;
            case 'r':
                *dst++ = '\r';
                break;
            case 'b':
                *dst++ = '\b';
                break;
            case 'f':
                *dst++ = '\f';
                break;
            case 'v':
                *dst++ = '\v';
                break;
            case '\"':
                *dst++ = '\"';
                break;
            default:
                *dst++ = '\\';
                *dst++ = *src;
                break;
            }
        }
        else
        {
            *dst++ = *src;
        }
        src++;
    }
    *dst = '\0';
    return processed_str;
}

size_t hex_min_bytes_to_encode_integer(int32_t value)
{
    // If value is negative, we need to return 4 bytes because we must preserve the sign bits.
    if (value < 0)
    {
        return 4;
    }

    // For positive values, check the minimal number of bytes needed.
    for (int bytes = 1; bytes <= 4; bytes++)
    {
        int32_t mask = (1 << (bytes * 8)) - 1;
        int32_t truncated_value = value & mask;

        // If the truncated value is equal to the original, this is the minimal byte size
        if (truncated_value == value)
        {
            return bytes;
        }
    }

    return 4; // Default to 4 bytes if no smaller size is found.
}

char *hex_unescape_string(const char *input)
{
    size_t len = strlen(input);
    char *unescaped = (char *)malloc(len + 1);

    size_t j = 0;
    for (size_t i = 0; i < len; i++)
    {
        if (input[i] == '\\' && i + 1 < len)
        {
            switch (input[i + 1])
            {
            case 'n':
                unescaped[j++] = '\n';
                i++;
                break;
            case 't':
                unescaped[j++] = '\t';
                i++;
                break;
            case 'r':
                unescaped[j++] = '\r';
                i++;
                break;
            case '\\':
                unescaped[j++] = '\\';
                i++;
                break;
            case '\"':
                unescaped[j++] = '\"';
                i++;
                break;
            case '\'':
                unescaped[j++] = '\'';
                i++;
                break;
            case 'v':
                unescaped[j++] = '\v';
                i++;
                break;
            case 'f':
                unescaped[j++] = '\f';
                i++;
                break;
            case 'a':
                unescaped[j++] = '\a';
                i++;
                break;
            case 'b':
                unescaped[j++] = '\b';
                i++;
                break;
            default:
                unescaped[j++] = input[i];
            }
        }
        else
        {
            unescaped[j++] = input[i];
        }
    }
    unescaped[j] = '\0'; // Null-terminate the string
    return unescaped;
}

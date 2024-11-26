#include "hex.h"

// Common operations
#define POP(x) hex_item_t x = hex_pop()
#define FREE(x) hex_free_element(x)
#define PUSH(x) hex_push(x)

// Global variables
int HEX_DEBUG = 0;
char HEX_ERROR[256] = "";
char **HEX_ARGV;
int HEX_ARGC = 0;
int HEX_ERRORS = 1;
int HEX_STACK_TRACE = 0;
hex_item_t HEX_STACK[HEX_STACK_SIZE];
int HEX_TOP = -1;
hex_registry_entry_t HEX_REGISTRY[HEX_REGISTRY_SIZE];
int HEX_REGISTRY_COUNT = 0;

char *HEX_NATIVE_SYMBOLS[] = {
    "store",
    "free",
    "type",
    "i",
    "eval",
    "puts",
    "warn",
    "print",
    "gets",
    "+",
    "-",
    "*",
    "/",
    "%",
    "&",
    "|",
    "^",
    "~",
    "<<",
    ">>",
    "int",
    "str",
    "dec",
    "hex",
    "==",
    "!=",
    ">",
    "<",
    ">=",
    "<=",
    "and",
    "or",
    "not",
    "xor",
    "cat",
    "slice",
    "len",
    "get",
    "insert",
    "index",
    "join",
    "split",
    "replace",
    "read",
    "write",
    "append",
    "args",
    "exit",
    "exec",
    "run",
    "if",
    "when",
    "while",
    "each",
    "error",
    "try",
    "q",
    "map",
    "filter",
    "swap",
    "dup",
    "stack",
    "clear",
    "pop"};

////////////////////////////////////////
// Context Implementation             //
////////////////////////////////////////

hex_context_t hex_init()
{
    hex_context_t context;
    context.argc = 0;
    context.argv = NULL;
    context.error = NULL;
    context.registry.size = 0;
    context.stack.top = -1;
    context.stack_trace.start = 0;
    context.stack_trace.size = 0;
    context.settings.debugging_enabled = 0;
    context.settings.errors_enabled = 1;
    context.settings.stack_trace_enabled = 1;
    return context;
}

////////////////////////////////////////
// Registry Implementation            //
////////////////////////////////////////

int hex_valid_user_symbol(const char *symbol)
{
    // Check that key starts with a letter, or underscore
    // and subsequent characters (if any) are letters, numbers, or underscores
    if (strlen(symbol) == 0)
    {
        hex_error("Symbol name cannot be an empty string");
        return 0;
    }
    if (!isalpha(symbol[0]) && symbol[0] != '_')
    {
        hex_error("Invalid symbol: %s", symbol);
        return 0;
    }
    for (size_t j = 1; j < strlen(symbol); j++)
    {
        if (!isalnum(symbol[j]) && symbol[j] != '_' && symbol[j] != '-')
        {
            hex_error("Invalid symbol: %s", symbol);
            return 0;
        }
    }
    return 1;
}

// Add a symbol to the registry
int hex_set_symbol(const char *key, hex_item_t value, int native)
{
    if (!native && hex_valid_user_symbol(key) == 0)
    {
        return 1;
    }
    for (int i = 0; i < HEX_REGISTRY_COUNT; i++)
    {
        if (strcmp(HEX_REGISTRY[i].key, key) == 0)
        {
            if (HEX_REGISTRY[i].value.type == HEX_TYPE_NATIVE_SYMBOL)
            {
                hex_error("Cannot overwrite native symbol %s", key);
                return 1;
            }
            free(HEX_REGISTRY[i].key);
            HEX_REGISTRY[i].key = strdup(key);
            HEX_REGISTRY[i].value = value;
            return 0;
        }
    }

    if (HEX_REGISTRY_COUNT >= HEX_REGISTRY_SIZE)
    {
        hex_error("Registry overflow");
        hex_free_token(value.token);
        return 1;
    }

    HEX_REGISTRY[HEX_REGISTRY_COUNT].key = strdup(key);
    HEX_REGISTRY[HEX_REGISTRY_COUNT].value = value;
    HEX_REGISTRY_COUNT++;
    return 0;
}

// Register a native symbol
void hex_set_native_symbol(const char *name, int (*func)())
{
    hex_item_t funcElement;
    funcElement.type = HEX_TYPE_NATIVE_SYMBOL;
    funcElement.data.functionPointer = func;

    if (hex_set_symbol(name, funcElement, 1) != 0)
    {
        hex_error("Error: Failed to register native symbol '%s'", name);
    }
}

// Get a symbol value from the registry
int hex_get_symbol(const char *key, hex_item_t *result)
{
    for (int i = 0; i < HEX_REGISTRY_COUNT; i++)
    {
        if (strcmp(HEX_REGISTRY[i].key, key) == 0)
        {
            *result = HEX_REGISTRY[i].value;
            return 1;
        }
    }
    return 0;
}

////////////////////////////////////////
// Stack Implementation               //
////////////////////////////////////////

// Push functions
int hex_push(hex_item_t element)
{
    if (HEX_TOP >= HEX_STACK_SIZE - 1)
    {
        hex_error("Stack overflow");
        return 1;
    }
    hex_debug_element("PUSH", element);
    int result = 0;
    if (element.type == HEX_TYPE_USER_SYMBOL)
    {
        hex_item_t value;
        if (hex_get_symbol(element.token->value, &value))
        {
            result = PUSH(value);
        }
        else
        {
            hex_error("Undefined user symbol: %s", element.token->value);
            FREE(value);
            result = 1;
        }
    }
    else if (element.type == HEX_TYPE_NATIVE_SYMBOL)
    {
        hex_debug_element("CALL", element);
        add_to_stack_trace(element.token);
        result = element.data.functionPointer();
    }
    else
    {
        HEX_STACK[++HEX_TOP] = element;
    }
    if (result == 0)
    {
        hex_debug_element("DONE", element);
    }
    else
    {
        hex_debug_element("FAIL", element);
    }
    return result;
}

int hex_push_int(int value)
{
    hex_item_t element = {.type = HEX_TYPE_INTEGER, .data.intValue = value};
    return PUSH(element);
}

char *hex_process_string(const char *value)
{
    int len = strlen(value);
    char *processedStr = (char *)malloc(len + 1);
    if (!processedStr)
    {
        hex_error("Memory allocation failed");
        return NULL;
    }

    char *dst = processedStr;
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
            case '\\':
                *dst++ = '\\';
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
    return processedStr;
}

int hex_push_string(const char *value)
{
    char *processedStr = hex_process_string(value);
    hex_item_t element = {.type = HEX_TYPE_STRING, .data.strValue = processedStr};
    return PUSH(element);
}

int hex_push_quotation(hex_item_t **quotation, int size)
{
    hex_item_t element = {.type = HEX_TYPE_QUOTATION, .data.quotationValue = quotation, .quotationSize = size};
    return PUSH(element);
}

int hex_push_symbol(hex_token_t *token)
{
    add_to_stack_trace(token);
    hex_item_t value;
    if (hex_get_symbol(token->value, &value))
    {
        value.token = token;
        return PUSH(value);
    }
    else
    {
        hex_error("Undefined symbol: %s", token->value);
        return 1;
    }
}

// Pop function
hex_item_t hex_pop()
{
    if (HEX_TOP < 0)
    {
        hex_error("Insufficient elements on the stack");
        return (hex_item_t){.type = HEX_TYPE_INVALID};
    }
    hex_debug_element(" POP", HEX_STACK[HEX_TOP]);
    return HEX_STACK[HEX_TOP--];
}

// Free a stack element
void hex_free_element(hex_item_t element)
{
    hex_debug_element("FREE", element);
    if (element.type == HEX_TYPE_STRING && element.data.strValue != NULL)
    {
        free(element.data.strValue);
        element.data.strValue = NULL;
    }
    else if (element.type == HEX_TYPE_QUOTATION && element.data.quotationValue != NULL)
    {
        for (int i = 0; i < element.quotationSize; i++)
        {
            if (element.data.quotationValue[i] != NULL)
            {
                FREE(*element.data.quotationValue[i]);
                // free(element.data.quotationValue[i]);
                // element.data.quotationValue[i] = NULL;
            }
        }
        free(element.data.quotationValue);
        element.data.quotationValue = NULL;
    }
    else if (element.type == HEX_TYPE_NATIVE_SYMBOL && element.token->value != NULL)
    {
        hex_free_token(element.token);
    }
    else if (element.type == HEX_TYPE_USER_SYMBOL && element.token->value != NULL)
    {
        hex_free_token(element.token);
    }
}

void hex_free_list(hex_item_t **quotation, int size)
{
    hex_error("An error occurred while filtering the list");
    for (int i = 0; i < size; i++)
    {
        FREE(*quotation[i]);
    }
}

////////////////////////////////////////
// Error & Debugging                  //
////////////////////////////////////////

void hex_error(const char *format, ...)
{
    va_list args;
    va_start(args, format);
    vsnprintf(HEX_ERROR, sizeof(HEX_ERROR), format, args);
    if (HEX_ERRORS)
    {
        fprintf(stderr, "[error] ");
        fprintf(stderr, "%s\n", HEX_ERROR);
    }
    va_end(args);
}

void hex_debug(const char *format, ...)
{
    if (HEX_DEBUG)
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
        return "native symbol";
    case HEX_TYPE_USER_SYMBOL:
        return "user symbol";
    case HEX_TYPE_INVALID:
        return "invalid";
    default:
        return "unknown";
    }
}

void hex_debug_element(const char *message, hex_item_t element)
{
    if (HEX_DEBUG)
    {
        fprintf(stdout, "*** %s: ", message);
        hex_print_element(stdout, element);
        fprintf(stdout, "\n");
    }
}

////////////////////////////////////////
// Tokenizer Implementation           //
////////////////////////////////////////

// Process a token from the input
hex_token_t *hex_next_token(const char **input, hex_file_position_t *position)
{
    const char *ptr = *input;

    // Skip whitespace
    while (isspace(*ptr))
    {
        if (*ptr == '\n')
        {
            position->line++;
            position->column = 1;
        }
        else
        {
            position->column++;
        }
        ptr++;
    }

    if (*ptr == '\0')
    {
        return NULL; // End of input
    }

    hex_token_t *token = (hex_token_t *)malloc(sizeof(hex_token_t));
    token->value = NULL;
    token->position.line = position->line;
    token->position.column = position->column;

    if (*ptr == ';')
    {
        // Comment token
        const char *start = ptr;
        while (*ptr != '\0' && *ptr != '\n')
        {
            ptr++;
            position->column++;
        }
        int len = ptr - start;
        token->value = (char *)malloc(len + 1);
        strncpy(token->value, start, len);
        token->value[len] = '\0';
        token->type = HEX_TOKEN_COMMENT;
    }
    else if (*ptr == '"')
    {
        // String token
        ptr++;
        const char *start = ptr;
        int len = 0;

        while (*ptr != '\0')
        {
            if (*ptr == '\\' && *(ptr + 1) == '"')
            {
                ptr += 2;
                len++;
                position->column += 2;
            }
            else if (*ptr == '"')
            {
                break;
            }
            else if (*ptr == '\n')
            {
                hex_error("Unescaped new line in string");
                token->value = "<newline>";
                token->type = HEX_TOKEN_INVALID;
                token->position.line = position->line;
                token->position.column = position->column;
                return token;
            }
            else
            {
                ptr++;
                len++;
                position->column++;
            }
        }

        if (*ptr != '"')
        {
            hex_error("Unterminated string");
            token->type = HEX_TOKEN_INVALID;
            token->value = strdup(ptr);
            token->position.line = position->line;
            token->position.column = position->column;
            return token;
        }

        token->value = (char *)malloc(len + 1);
        char *dst = token->value;

        ptr = start;
        while (*ptr != '\0' && *ptr != '"')
        {
            if (*ptr == '\\' && *(ptr + 1) == '"')
            {
                *dst++ = '"';
                ptr += 2;
            }
            else
            {
                *dst++ = *ptr++;
            }
        }
        *dst = '\0';
        ptr++;
        position->column++;
        token->type = HEX_TOKEN_STRING;
    }
    else if (strncmp(ptr, "0x", 2) == 0 || strncmp(ptr, "0X", 2) == 0)
    {
        // Hexadecimal integer token
        const char *start = ptr;
        ptr += 2; // Skip the "0x" prefix
        position->column += 2;
        while (isxdigit(*ptr))
        {
            ptr++;
            position->column++;
        }
        int len = ptr - start;
        token->value = (char *)malloc(len + 1);
        strncpy(token->value, start, len);
        token->value[len] = '\0';
        token->type = HEX_TOKEN_INTEGER;
    }
    else if (*ptr == '(')
    {
        token->type = HEX_TOKEN_QUOTATION_START;
        ptr++;
        position->column++;
    }
    else if (*ptr == ')')
    {
        token->type = HEX_TOKEN_QUOTATION_END;
        ptr++;
        position->column++;
    }
    else
    {
        // Symbol token
        const char *start = ptr;
        while (*ptr != '\0' && !isspace(*ptr) && *ptr != ';' && *ptr != '(' && *ptr != ')' && *ptr != '"')
        {
            ptr++;
            position->column++;
        }

        int len = ptr - start;
        token->value = (char *)malloc(len + 1);
        strncpy(token->value, start, len);
        token->value[len] = '\0';
        if (hex_valid_native_symbol(token->value) || hex_valid_user_symbol(token->value))
        {
            token->type = HEX_TOKEN_SYMBOL;
        }
        else
        {
            token->type = HEX_TOKEN_INVALID;
            token->position.line = position->line;
            token->position.column = position->column;
        }
    }

    *input = ptr;
    return token;
}

// Free a token
void hex_free_token(hex_token_t *token)
{
    if (token)
    {
        free(token->value);
        free(token);
    }
}

int hex_valid_native_symbol(const char *symbol)
{
    for (size_t i = 0; i < sizeof(HEX_NATIVE_SYMBOLS) / sizeof(HEX_NATIVE_SYMBOLS[0]); i++)
    {
        if (strcmp(symbol, HEX_NATIVE_SYMBOLS[i]) == 0)
        {
            return 1;
        }
    }
    return 0;
}

int32_t hex_parse_integer(const char *hex_str)
{
    // Parse the hexadecimal string as an unsigned 32-bit integer
    uint32_t unsigned_value = (uint32_t)strtoul(hex_str, NULL, 16);

    // Cast the unsigned value to a signed 32-bit integer
    return (int32_t)unsigned_value;
}

int hex_parse_quotation(const char **input, hex_item_t *result, hex_file_position_t *position)
{
    hex_item_t **quotation = NULL;
    int capacity = 2;
    int size = 0;
    int balanced = 1;

    quotation = (hex_item_t **)malloc(capacity * sizeof(hex_item_t *));
    if (!quotation)
    {
        hex_error("Memory allocation failed");
        return 1;
    }

    hex_token_t *token;
    while ((token = hex_next_token(input, position)) != NULL)
    {
        if (token->type == HEX_TOKEN_QUOTATION_END)
        {
            balanced--;
            break;
        }

        if (size >= capacity)
        {
            capacity *= 2;
            quotation = (hex_item_t **)realloc(quotation, capacity * sizeof(hex_item_t *));
            if (!quotation)
            {
                hex_error("Memory allocation failed");
                return 1;
            }
        }

        hex_item_t *element = (hex_item_t *)malloc(sizeof(hex_item_t));
        if (token->type == HEX_TOKEN_INTEGER)
        {
            element->type = HEX_TYPE_INTEGER;
            element->data.intValue = hex_parse_integer(token->value);
        }
        else if (token->type == HEX_TOKEN_STRING)
        {
            char *processedStr = hex_process_string(token->value);
            element->type = HEX_TYPE_STRING;
            element->data.strValue = strdup(processedStr);
        }
        else if (token->type == HEX_TOKEN_SYMBOL)
        {
            if (hex_valid_native_symbol(token->value))
            {
                element->type = HEX_TYPE_NATIVE_SYMBOL;
                hex_item_t value;
                if (hex_get_symbol(token->value, &value))
                {
                    element->token = token;
                    element->type = HEX_TYPE_NATIVE_SYMBOL;
                    element->data.functionPointer = value.data.functionPointer;
                }
                else
                {
                    hex_error("Unable to reference native symbol: %s", token->value);
                    hex_free_token(token);
                    hex_free_list(quotation, size);
                    return 1;
                }
            }
            else
            {
                element->type = HEX_TYPE_USER_SYMBOL;
            }
            token->position.filename = strdup(position->filename);
            element->token = token;
        }
        else if (token->type == HEX_TOKEN_QUOTATION_START)
        {
            element->type = HEX_TYPE_QUOTATION;
            if (hex_parse_quotation(input, element, position) != 0)
            {
                hex_free_token(token);
                hex_free_list(quotation, size);
                return 1;
            }
        }
        else if (token->type == HEX_TOKEN_COMMENT)
        {
            // Ignore comments
        }
        else
        {
            hex_error("Unexpected token in quotation: %d", token->value);
            hex_free_token(token);
            hex_free_list(quotation, size);
            return 1;
        }

        quotation[size] = element;
        size++;
    }

    if (balanced != 0)
    {
        hex_error("Unterminated quotation");
        hex_free_token(token);
        hex_free_list(quotation, size);
        return 1;
    }

    result->type = HEX_TYPE_QUOTATION;
    result->data.quotationValue = quotation;
    result->quotationSize = size;
    hex_free_token(token);
    return 0;
}

////////////////////////////////////////
// Stack trace implementation         //
////////////////////////////////////////

hex_stack_trace_t stackTrace = {.start = 0, .size = 0};

// Add an entry to the circular stack trace
void add_to_stack_trace(hex_token_t *token)
{
    int index = (stackTrace.start + stackTrace.size) % HEX_STACK_TRACE_SIZE;

    if (stackTrace.size < HEX_STACK_TRACE_SIZE)
    {
        // Buffer is not full; add element
        stackTrace.entries[index] = *token;
        stackTrace.size++;
    }
    else
    {
        // Buffer is full; overwrite the oldest element
        stackTrace.entries[index] = *token;
        stackTrace.start = (stackTrace.start + 1) % HEX_STACK_TRACE_SIZE;
    }
}

// Print the stack trace
void print_stack_trace()
{
    if (!HEX_STACK_TRACE || !HEX_ERRORS)
    {
        return;
    }
    fprintf(stderr, "[stack trace] (most recent symbol first):\n");

    for (int i = 0; i < stackTrace.size; i++)
    {
        int index = (stackTrace.start + stackTrace.size - 1 - i) % HEX_STACK_TRACE_SIZE;
        hex_token_t token = stackTrace.entries[index];
        fprintf(stderr, "  %s (%s:%d:%d)\n", token.value, token.position.filename, token.position.line, token.position.column);
    }
}

////////////////////////////////////////
// Helper Functions                   //
////////////////////////////////////////

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

void hex_raw_print_element(FILE *stream, hex_item_t element)
{
    switch (element.type)
    {
    case HEX_TYPE_INTEGER:
        fprintf(stream, "0x%xx", element.data.intValue);
        break;
    case HEX_TYPE_STRING:
        fprintf(stream, "%s", element.data.strValue);
        break;
    case HEX_TYPE_USER_SYMBOL:
    case HEX_TYPE_NATIVE_SYMBOL:
        fprintf(stream, "%s", element.token->value);
        break;
    case HEX_TYPE_QUOTATION:
        fprintf(stream, "(");
        for (int i = 0; i < element.quotationSize; i++)
        {
            if (i > 0)
            {
                fprintf(stream, " ");
            }
            hex_print_element(stream, *element.data.quotationValue[i]);
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

void hex_print_element(FILE *stream, hex_item_t element)
{
    switch (element.type)
    {
    case HEX_TYPE_INTEGER:
        fprintf(stream, "0x%x", element.data.intValue);
        break;

    case HEX_TYPE_STRING:
        fprintf(stream, "\"");
        for (char *c = element.data.strValue; *c != '\0'; c++)
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
        fprintf(stream, "%s", element.token->value);
        break;

    case HEX_TYPE_QUOTATION:
        fprintf(stream, "(");
        for (int i = 0; i < element.quotationSize; i++)
        {
            if (i > 0)
            {
                fprintf(stream, " ");
            }
            hex_print_element(stream, *element.data.quotationValue[i]);
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

int hex_is_symbol(hex_token_t *token, char *value)
{
    return strcmp(token->value, value) == 0;
}

////////////////////////////////////////
// Native Symbol Implementations      //
////////////////////////////////////////

// Definition symbols

int hex_symbol_store()
{
    // POP(name);
    POP(name);
    if (name.type == HEX_TYPE_INVALID)
    {
        FREE(name);
        return 1;
    }
    POP(value);
    if (value.type == HEX_TYPE_INVALID)
    {
        FREE(name);
        FREE(value);
        return 1;
    }
    if (name.type != HEX_TYPE_STRING)
    {
        hex_error("Symbol name must be a string");
        FREE(name);
        FREE(value);
        return 1;
    }
    if (hex_set_symbol(name.data.strValue, value, 0) != 0)
    {
        hex_error("Failed to store variable");
        FREE(name);
        FREE(value);
        return 1;
    }
    return 0;
}

int hex_symbol_free()
{
    POP(element);
    if (element.type == HEX_TYPE_INVALID)
    {
        FREE(element);
        return 1;
    }
    if (element.type != HEX_TYPE_STRING)
    {
        FREE(element);
        hex_error("Variable name must be a string");
        return 1;
    }
    for (int i = 0; i < HEX_REGISTRY_COUNT; i++)
    {
        if (strcmp(HEX_REGISTRY[i].key, element.data.strValue) == 0)
        {
            free(HEX_REGISTRY[i].key);
            FREE(HEX_REGISTRY[i].value);
            for (int j = i; j < HEX_REGISTRY_COUNT - 1; j++)
            {
                HEX_REGISTRY[j] = HEX_REGISTRY[j + 1];
            }
            HEX_REGISTRY_COUNT--;
            FREE(element);
            return 0;
        }
    }
    FREE(element);
    return 0;
}

int hex_symbol_type()
{
    POP(element);
    if (element.type == HEX_TYPE_INVALID)
    {
        FREE(element);
        return 1;
    }
    return hex_push_string(hex_type(element.type));
}

// Evaluation symbols

int hex_symbol_i()
{
    POP(element);
    if (element.type == HEX_TYPE_INVALID)
    {
        FREE(element);
        return 1;
    }
    if (element.type != HEX_TYPE_QUOTATION)
    {
        hex_error("'i' symbol requires a quotation");
        FREE(element);
        return 1;
    }
    for (int i = 0; i < element.quotationSize; i++)
    {
        if (hex_push(*element.data.quotationValue[i]) != 0)
        {
            FREE(element);
            return 1;
        }
    }
    return 0;
}

int hex_interpret(const char *code, char *filename, int line, int column);

// evaluate a string
int hex_symbol_eval()
{
    POP(element);
    if (element.type == HEX_TYPE_INVALID)
    {
        FREE(element);
        return 1;
    }
    if (element.type != HEX_TYPE_STRING)
    {
        hex_error("'eval' symbol requires a string");
        FREE(element);
        return 1;
    }
    return hex_interpret(element.data.strValue, "<eval>", 1, 1);
}

// IO Symbols

int hex_symbol_puts()
{
    POP(element);
    if (element.type == HEX_TYPE_INVALID)
    {
        FREE(element);
        return 1;
    }
    hex_raw_print_element(stdout, element);
    printf("\n");
    return 0;
}

int hex_symbol_warn()
{
    POP(element);
    if (element.type == HEX_TYPE_INVALID)
    {
        FREE(element);
        return 1;
    }
    hex_raw_print_element(stderr, element);
    printf("\n");
    return 0;
}

int hex_symbol_print()
{
    POP(element);
    if (element.type == HEX_TYPE_INVALID)
    {
        FREE(element);
        return 1;
    }
    hex_raw_print_element(stdout, element);
    return 0;
}

int hex_symbol_gets()
{
    char input[HEX_STDIN_BUFFER_SIZE]; // Buffer to store the input (adjust size if needed)

    if (fgets(input, sizeof(input), stdin) != NULL)
    {
        // Strip the newline character at the end of the string
        input[strcspn(input, "\n")] = '\0';

        // Push the input string onto the stack
        return hex_push_string(input);
    }
    else
    {
        hex_error("Failed to read input");
        return 1;
    }
}

// Mathematical symbols
int hex_symbol_add()
{
    POP(b);
    if (b.type == HEX_TYPE_INVALID)
    {
        FREE(b);
        return 1;
    }
    POP(a);
    if (a.type == HEX_TYPE_INVALID)
    {
        FREE(a);
        FREE(b);
        return 1;
    }
    if (a.type == HEX_TYPE_INTEGER && b.type == HEX_TYPE_INTEGER)
    {
        return hex_push_int(a.data.intValue + b.data.intValue);
    }
    hex_error("'+' symbol requires two integers");
    FREE(a);
    FREE(b);
    return 1;
}

int hex_symbol_subtract()
{
    POP(b);
    if (b.type == HEX_TYPE_INVALID)
    {
        FREE(b);
        return 1;
    }
    POP(a);
    if (a.type == HEX_TYPE_INVALID)
    {
        FREE(a);
        FREE(b);
        return 1;
    }
    if (a.type == HEX_TYPE_INTEGER && b.type == HEX_TYPE_INTEGER)
    {
        return hex_push_int(a.data.intValue - b.data.intValue);
    }
    hex_error("'-' symbol requires two integers");
    FREE(a);
    FREE(b);
    return 1;
}

int hex_symbol_multiply()
{
    POP(b);
    if (b.type == HEX_TYPE_INVALID)
    {
        FREE(b);
        return 1;
    }
    POP(a);
    if (a.type == HEX_TYPE_INVALID)
    {
        FREE(a);
        FREE(b);
        return 1;
    }
    if (a.type == HEX_TYPE_INTEGER && b.type == HEX_TYPE_INTEGER)
    {
        return hex_push_int(a.data.intValue * b.data.intValue);
    }
    hex_error("'*' symbol requires two integers");
    FREE(a);
    FREE(b);
    return 1;
}

int hex_symbol_divide()
{
    POP(b);
    if (b.type == HEX_TYPE_INVALID)
    {
        FREE(b);
        return 1;
    }
    POP(a);
    if (a.type == HEX_TYPE_INVALID)
    {
        FREE(a);
        FREE(b);
        return 1;
    }
    if (a.type == HEX_TYPE_INTEGER && b.type == HEX_TYPE_INTEGER)
    {
        if (b.data.intValue == 0)
        {
            hex_error("Division by zero");
            return 1;
        }
        return hex_push_int(a.data.intValue / b.data.intValue);
    }
    hex_error("'/' symbol requires two integers");
    FREE(a);
    FREE(b);
    return 1;
}

int hex_symbol_modulo()
{
    POP(b);
    if (b.type == HEX_TYPE_INVALID)
    {
        FREE(b);
        return 1;
    }
    POP(a);
    if (a.type == HEX_TYPE_INVALID)
    {
        FREE(a);
        FREE(b);
        return 1;
    }
    if (a.type == HEX_TYPE_INTEGER && b.type == HEX_TYPE_INTEGER)
    {
        if (b.data.intValue == 0)
        {
            hex_error("Division by zero");
        }
        return hex_push_int(a.data.intValue % b.data.intValue);
    }
    hex_error("'%%' symbol requires two integers");
    FREE(a);
    FREE(b);
    return 1;
}

// Bit symbols

int hex_symbol_bitand()
{
    POP(right);
    if (right.type == HEX_TYPE_INVALID)
    {
        FREE(right);
        return 1;
    }
    POP(left);
    if (left.type == HEX_TYPE_INVALID)
    {
        FREE(left);
        FREE(right);
        return 1;
    }
    if (left.type == HEX_TYPE_INTEGER && right.type == HEX_TYPE_INTEGER)
    {
        return hex_push_int(left.data.intValue & right.data.intValue);
    }
    hex_error("'&' symbol requires two integers");
    FREE(left);
    FREE(right);
    return 1;
}

int hex_symbol_bitor()
{
    POP(right);
    if (right.type == HEX_TYPE_INVALID)
    {
        FREE(right);
        return 1;
    }
    POP(left);
    if (left.type == HEX_TYPE_INVALID)
    {
        FREE(left);
        FREE(right);
        return 1;
    }
    if (left.type == HEX_TYPE_INTEGER && right.type == HEX_TYPE_INTEGER)
    {
        return hex_push_int(left.data.intValue | right.data.intValue);
    }
    hex_error("'|' symbol requires two integers");
    FREE(left);
    FREE(right);
    return 1;
}

int hex_symbol_bitxor()
{
    POP(right);
    if (right.type == HEX_TYPE_INVALID)
    {
        FREE(right);
        return 1;
    }
    POP(left);
    if (left.type == HEX_TYPE_INVALID)
    {
        FREE(left);
        FREE(right);
        return 1;
    }
    if (left.type == HEX_TYPE_INTEGER && right.type == HEX_TYPE_INTEGER)
    {
        return hex_push_int(left.data.intValue ^ right.data.intValue);
    }
    hex_error("'^' symbol requires two integers");
    FREE(left);
    FREE(right);
    return 1;
}

int hex_symbol_shiftleft()
{
    POP(right);
    if (right.type == HEX_TYPE_INVALID)
    {
        FREE(right);
        return 1;
    }
    POP(left);
    if (left.type == HEX_TYPE_INVALID)
    {
        FREE(left);
        FREE(right);
        return 1;
    }
    if (left.type == HEX_TYPE_INTEGER && right.type == HEX_TYPE_INTEGER)
    {
        return hex_push_int(left.data.intValue << right.data.intValue);
    }
    hex_error("'<<' symbol requires two integers");
    FREE(left);
    FREE(right);
    return 1;
}

int hex_symbol_shiftright()
{
    POP(right);
    if (right.type == HEX_TYPE_INVALID)
    {
        FREE(right);
        return 1;
    }
    POP(left);
    if (left.type == HEX_TYPE_INVALID)
    {
        FREE(left);
        FREE(right);
        return 1;
    }
    if (left.type == HEX_TYPE_INTEGER && right.type == HEX_TYPE_INTEGER)
    {
        return hex_push_int(left.data.intValue >> right.data.intValue);
    }
    hex_error("'>>' symbol requires two integers");
    FREE(left);
    FREE(right);
    return 1;
}

int hex_symbol_bitnot()
{
    POP(element);
    if (element.type == HEX_TYPE_INVALID)
    {
        FREE(element);
        return 1;
    }
    if (element.type == HEX_TYPE_INTEGER)
    {
        return hex_push_int(~element.data.intValue);
    }
    hex_error("'~' symbol requires one integer");
    FREE(element);
    return 1;
}

// Conversion symbols

int hex_symbol_int()
{
    POP(a);
    if (a.type == HEX_TYPE_INVALID)
    {
        FREE(a);
        return 1;
    }
    if (a.type == HEX_TYPE_QUOTATION)
    {
        hex_error("Cannot convert a quotation to an integer");
        FREE(a);
        return 1;
    }
    if (a.type == HEX_TYPE_INTEGER)
    {
        return hex_push_int(a.data.intValue);
    }
    if (a.type == HEX_TYPE_STRING)
    {
        return hex_push_int(strtol(a.data.strValue, NULL, 16));
    }
    hex_error("Unsupported data type: %s", hex_type(a.type));
    FREE(a);
    return 1;
}

int hex_symbol_str()
{
    POP(a);
    if (a.type == HEX_TYPE_INVALID)
    {
        FREE(a);
        return 1;
    }
    if (a.type == HEX_TYPE_QUOTATION)
    {
        hex_error("Cannot convert a quotation to a string");
        FREE(a);
        return 1;
    }
    if (a.type == HEX_TYPE_INTEGER)
    {
        return hex_push_string(hex_itoa_hex(a.data.intValue));
    }
    if (a.type == HEX_TYPE_STRING)
    {
        return hex_push_string(a.data.strValue);
    }
    hex_error("Unsupported data type: %s", hex_type(a.type));
    FREE(a);
    return 1;
}

int hex_symbol_dec()
{
    POP(a);
    if (a.type == HEX_TYPE_INVALID)
    {
        FREE(a);
        return 1;
    }
    if (a.type == HEX_TYPE_INTEGER)
    {
        return hex_push_string(hex_itoa_dec(a.data.intValue));
    }
    hex_error("An integer is required");
    FREE(a);
    return 1;
}

int hex_symbol_hex()
{
    POP(element);
    if (element.type == HEX_TYPE_INVALID)
    {
        FREE(element);
        return 1;
    }
    if (element.type == HEX_TYPE_STRING)
    {
        return hex_push_int(strtol(element.data.strValue, NULL, 10));
    }
    hex_error("'hex' symbol requires a string representing a decimal integer");
    FREE(element);
    return 1;
}

// Comparison symbols

int hex_equal(hex_item_t a, hex_item_t b)
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
        return a.data.intValue == b.data.intValue;
    }
    if (a.type == HEX_TYPE_STRING)
    {
        return (strcmp(a.data.strValue, b.data.strValue) == 0);
    }
    if (a.type == HEX_TYPE_QUOTATION)
    {
        if (a.quotationSize != b.quotationSize)
        {
            return 0;
        }
        else
        {
            for (int i = 0; i < a.quotationSize; i++)
            {
                if (!hex_equal(*a.data.quotationValue[i], *b.data.quotationValue[i]))
                {
                    return 0;
                }
            }
            return 1;
        }
    }
    return 0;
}

int hex_symbol_equal()
{
    POP(b);
    if (b.type == HEX_TYPE_INVALID)
    {
        FREE(b);
        return 1;
    }
    POP(a);
    if (a.type == HEX_TYPE_INVALID)
    {
        FREE(a);
        FREE(b);
        return 1;
    }
    if ((a.type == HEX_TYPE_INTEGER && b.type == HEX_TYPE_INTEGER) || (a.type == HEX_TYPE_STRING && b.type == HEX_TYPE_STRING) || (a.type == HEX_TYPE_QUOTATION && b.type == HEX_TYPE_QUOTATION))
    {
        return hex_push_int(hex_equal(a, b));
    }
    hex_error("'==' symbol requires two integers, two strings, or two quotations");
    FREE(a);
    FREE(b);
    return 1;
}

int hex_symbol_notequal()
{
    POP(b);
    if (b.type == HEX_TYPE_INVALID)
    {
        FREE(b);
        return 1;
    }
    POP(a);
    if (a.type == HEX_TYPE_INVALID)
    {
        FREE(a);
        FREE(b);
        return 1;
    }
    if ((a.type == HEX_TYPE_INTEGER && b.type == HEX_TYPE_INTEGER) || (a.type == HEX_TYPE_STRING && b.type == HEX_TYPE_STRING) || (a.type == HEX_TYPE_QUOTATION && b.type == HEX_TYPE_QUOTATION))
    {
        return hex_push_int(!hex_equal(a, b));
    }
    hex_error("'!=' symbol requires two integers, two strings, or two quotations");
    FREE(a);
    FREE(b);
    return 1;
}

int hex_symbol_greater()
{
    POP(b);
    if (b.type == HEX_TYPE_INVALID)
    {
        FREE(b);
        return 1;
    }
    POP(a);
    if (a.type == HEX_TYPE_INVALID)
    {
        FREE(a);
        FREE(b);
        return 1;
    }
    if (a.type == HEX_TYPE_INTEGER && b.type == HEX_TYPE_INTEGER)
    {
        return hex_push_int(a.data.intValue > b.data.intValue);
    }
    else if (a.type == HEX_TYPE_STRING && b.type == HEX_TYPE_STRING)
    {
        return hex_push_int(strcmp(a.data.strValue, b.data.strValue) > 0);
    }
    hex_error("'>' symbol requires two integers or two strings");
    FREE(a);
    FREE(b);
    return 1;
}

int hex_symbol_less()
{
    POP(b);
    if (b.type == HEX_TYPE_INVALID)
    {
        FREE(b);
        return 1;
    }
    POP(a);
    if (a.type == HEX_TYPE_INVALID)
    {
        FREE(a);
        FREE(b);
        return 1;
    }
    if (a.type == HEX_TYPE_INTEGER && b.type == HEX_TYPE_INTEGER)
    {
        return hex_push_int(a.data.intValue < b.data.intValue);
    }
    if (a.type == HEX_TYPE_STRING && b.type == HEX_TYPE_STRING)
    {
        return hex_push_int(strcmp(a.data.strValue, b.data.strValue) < 0);
    }
    hex_error("'<' symbol requires two integers or two strings");
    FREE(a);
    FREE(b);
    return 1;
}

int hex_symbol_greaterequal()
{
    POP(b);
    if (b.type == HEX_TYPE_INVALID)
    {
        FREE(b);
        return 1;
    }
    POP(a);
    if (a.type == HEX_TYPE_INVALID)
    {
        FREE(a);
        FREE(b);
        return 1;
    }
    if (a.type == HEX_TYPE_INTEGER && b.type == HEX_TYPE_INTEGER)
    {
        return hex_push_int(a.data.intValue >= b.data.intValue);
    }
    if (a.type == HEX_TYPE_STRING && b.type == HEX_TYPE_STRING)
    {
        return hex_push_int(strcmp(a.data.strValue, b.data.strValue) >= 0);
    }
    hex_error("'>=' symbol requires two integers or two strings");
    FREE(a);
    FREE(b);
    return 1;
}

int hex_symbol_lessequal()
{
    POP(b);
    if (b.type == HEX_TYPE_INVALID)
    {
        FREE(b);
        return 1;
    }
    POP(a);
    if (a.type == HEX_TYPE_INVALID)
    {
        FREE(a);
        FREE(b);
        return 1;
    }
    if (a.type == HEX_TYPE_INTEGER && b.type == HEX_TYPE_INTEGER)
    {
        return hex_push_int(a.data.intValue <= b.data.intValue);
    }
    if (a.type == HEX_TYPE_STRING && b.type == HEX_TYPE_STRING)
    {
        return hex_push_int(strcmp(a.data.strValue, b.data.strValue) <= 0);
    }
    hex_error("'<=' symbol requires two integers or two strings");
    FREE(a);
    FREE(b);
    return 1;
}

// Boolean symbols

int hex_symbol_and()
{
    POP(b);
    if (b.type == HEX_TYPE_INVALID)
    {
        FREE(b);
        return 1;
    }
    POP(a);
    if (a.type == HEX_TYPE_INVALID)
    {
        FREE(a);
        FREE(b);
        return 1;
    }
    if (a.type == HEX_TYPE_INTEGER && b.type == HEX_TYPE_INTEGER)
    {
        return hex_push_int(a.data.intValue && b.data.intValue);
    }
    hex_error("'and' symbol requires two integers");
    FREE(a);
    FREE(b);
    return 1;
}

int hex_symbol_or()
{
    POP(b);
    if (b.type == HEX_TYPE_INVALID)
    {
        FREE(b);
        return 1;
    }
    POP(a);
    if (a.type == HEX_TYPE_INVALID)
    {
        FREE(a);
        FREE(b);
        return 1;
    }
    if (a.type == HEX_TYPE_INTEGER && b.type == HEX_TYPE_INTEGER)
    {
        return hex_push_int(a.data.intValue || b.data.intValue);
    }
    hex_error("'or' symbol requires two integers");
    FREE(a);
    FREE(b);
    return 1;
}

int hex_symbol_not()
{
    POP(a);
    if (a.type == HEX_TYPE_INVALID)
    {
        FREE(a);
        return 1;
    }
    if (a.type == HEX_TYPE_INTEGER)
    {
        return hex_push_int(!a.data.intValue);
    }
    hex_error("'not' symbol requires an integer");
    FREE(a);
    return 1;
}

int hex_symbol_xor()
{
    POP(b);
    if (b.type == HEX_TYPE_INVALID)
    {
        FREE(b);
        return 1;
    }
    POP(a);
    if (a.type == HEX_TYPE_INVALID)
    {
        FREE(a);
        FREE(b);
        return 1;
    }
    if (a.type == HEX_TYPE_INTEGER && b.type == HEX_TYPE_INTEGER)
    {
        return hex_push_int(a.data.intValue ^ b.data.intValue);
    }
    hex_error("'xor' symbol requires two integers");
    FREE(a);
    FREE(b);
    return 1;
}

// Quotation and String (List) Symbols

int hex_symbol_cat()
{
    POP(value);
    if (value.type == HEX_TYPE_INVALID)
    {
        FREE(value);
        return 1; // Failed to pop value
    }

    POP(list);
    if (list.type == HEX_TYPE_INVALID)
    {
        FREE(list);
        FREE(value);
        return 1; // Failed to pop list
    }

    int result = 0;

    if (list.type == HEX_TYPE_QUOTATION && value.type == HEX_TYPE_QUOTATION)
    {
        // Concatenate two quotations
        size_t newSize = list.quotationSize + value.quotationSize;
        hex_item_t **newQuotation = (hex_item_t **)realloc(
            list.data.quotationValue, newSize * sizeof(hex_item_t *));
        if (!newQuotation)
        {
            hex_error("Memory allocation failed");
            result = 1;
        }
        else
        {
            // Append elements from the second quotation
            for (size_t i = 0; i < (size_t)value.quotationSize; i++)
            {
                newQuotation[list.quotationSize + i] = value.data.quotationValue[i];
            }

            list.data.quotationValue = newQuotation;
            list.quotationSize = newSize;
            result = hex_push_quotation(list.data.quotationValue, newSize);
        }
    }
    else if (list.type == HEX_TYPE_STRING && value.type == HEX_TYPE_STRING)
    {
        // Concatenate two strings
        size_t newLength = strlen(list.data.strValue) + strlen(value.data.strValue) + 1;
        char *newStr = (char *)malloc(newLength);
        if (!newStr)
        {
            hex_error("Memory allocation failed");
            result = 1;
        }
        else
        {
            strcpy(newStr, list.data.strValue);
            strcat(newStr, value.data.strValue);
            result = hex_push_string(newStr);
        }
    }
    else
    {
        hex_error("Symbol 'cat' requires two quotations or two strings");
        result = 1;
    }

    // Free resources if the operation fails
    if (result != 0)
    {
        FREE(list);
        FREE(value);
    }

    return result;
}

int hex_symbol_slice()
{
    POP(end);
    if (end.type == HEX_TYPE_INVALID)
    {
        FREE(end);
        return 1;
    }
    POP(start);
    if (start.type == HEX_TYPE_INVALID)
    {
        FREE(start);
        FREE(end);
        return 1;
    }
    POP(list);
    if (list.type == HEX_TYPE_INVALID)
    {
        FREE(list);
        FREE(start);
        FREE(end);
        return 1;
    }
    int result = 0;
    if (list.type == HEX_TYPE_QUOTATION)
    {
        if (start.type != HEX_TYPE_INTEGER || end.type != HEX_TYPE_INTEGER)
        {
            hex_error("Slice indices must be integers");
            result = 1;
        }
        else if (start.data.intValue < 0 || start.data.intValue >= list.quotationSize || end.data.intValue < 0 || end.data.intValue >= list.quotationSize)
        {
            hex_error("Slice indices out of range");
            result = 1;
        }
        else
        {
            int newSize = end.data.intValue - start.data.intValue + 1;
            hex_item_t **newQuotation = (hex_item_t **)malloc(newSize * sizeof(hex_item_t *));
            if (!newQuotation)
            {
                hex_error("Memory allocation failed");
                result = 1;
            }
            else
            {
                for (int i = 0; i < newSize; i++)
                {
                    newQuotation[i] = (hex_item_t *)malloc(sizeof(hex_item_t));
                    *newQuotation[i] = *list.data.quotationValue[start.data.intValue + i];
                }
                result = hex_push_quotation(newQuotation, newSize);
            }
        }
    }
    else if (list.type == HEX_TYPE_STRING)
    {
        if (start.type != HEX_TYPE_INTEGER || end.type != HEX_TYPE_INTEGER)
        {
            hex_error("Slice indices must be integers");
            result = 1;
        }
        else if (start.data.intValue < 0 || start.data.intValue >= (int)strlen(list.data.strValue) || end.data.intValue < 0 || end.data.intValue >= (int)strlen(list.data.strValue))
        {
            hex_error("Slice indices out of range");
            result = 1;
        }
        else
        {
            int newSize = end.data.intValue - start.data.intValue + 1;
            char *newStr = (char *)malloc(newSize + 1);
            if (!newStr)
            {
                hex_error("Memory allocation failed");
                result = 1;
            }
            else
            {
                strncpy(newStr, list.data.strValue + start.data.intValue, newSize);
                newStr[newSize] = '\0';
                result = hex_push_string(newStr);
            }
        }
    }
    else
    {
        hex_error("Symbol 'slice' requires a quotation or a string");
        result = 1;
    }
    if (result != 0)
    {
        FREE(list);
        FREE(start);
        FREE(end);
    }
    return result;
}

int hex_symbol_len()
{
    POP(element);
    if (element.type == HEX_TYPE_INVALID)
    {
        FREE(element);
        return 1;
    }
    int result = 0;
    if (element.type == HEX_TYPE_QUOTATION)
    {
        result = hex_push_int(element.quotationSize);
    }
    else if (element.type == HEX_TYPE_STRING)
    {
        result = hex_push_int(strlen(element.data.strValue));
    }
    else
    {
        hex_error("Symbol 'len' requires a quotation or a string");
        result = 1;
    }
    if (result != 0)
    {
        FREE(element);
    }
    return result;
}

int hex_symbol_get()
{
    POP(index);
    if (index.type == HEX_TYPE_INVALID)
    {
        FREE(index);
        return 1;
    }
    POP(list);
    if (list.type == HEX_TYPE_INVALID)
    {
        FREE(list);
        FREE(index);
        return 1;
    }
    int result = 0;
    if (list.type == HEX_TYPE_QUOTATION)
    {
        if (index.type != HEX_TYPE_INTEGER)
        {
            hex_error("Index must be an integer");
            result = 1;
        }
        else if (index.data.intValue < 0 || index.data.intValue >= list.quotationSize)
        {
            hex_error("Index out of range");
            result = 1;
        }
        else
        {
            result = hex_push(*list.data.quotationValue[index.data.intValue]);
        }
    }
    else if (list.type == HEX_TYPE_STRING)
    {
        if (index.type != HEX_TYPE_INTEGER)
        {
            hex_error("Index must be an integer");
            result = 1;
        }
        else if (index.data.intValue < 0 || index.data.intValue >= (int)strlen(list.data.strValue))
        {
            hex_error("Index out of range");
            result = 1;
        }
        else
        {
            char str[2] = {list.data.strValue[index.data.intValue], '\0'};
            result = hex_push_string(str);
        }
    }
    else
    {
        hex_error("Symbol 'get' requires a quotation or a string");
        result = 1;
    }
    if (result != 0)
    {

        FREE(list);
        FREE(index);
    }
    return result;
}

int hex_symbol_insert(void)
{
    POP(index);
    if (index.type == HEX_TYPE_INVALID)
    {
        FREE(index);
        return 1;
    }
    POP(value);
    if (value.type == HEX_TYPE_INVALID)
    {
        FREE(index);
        FREE(value);
        return 1;
    }

    POP(target);
    if (target.type == HEX_TYPE_INVALID)
    {
        FREE(target);
        FREE(index);
        FREE(value);
        return 1;
    }

    if (index.type != HEX_TYPE_INTEGER)
    {
        hex_error("Index must be an integer");
        FREE(target);
        FREE(index);
        FREE(value);
        return 1;
    }

    if (target.type == HEX_TYPE_STRING)
    {
        if (value.type != HEX_TYPE_STRING)
        {
            hex_error("Value must be a string when inserting into a string");
            FREE(target);
            FREE(index);
            FREE(value);
            return 1;
        }

        size_t target_len = strlen(target.data.strValue);
        size_t value_len = strlen(value.data.strValue);
        size_t pos = index.data.intValue;

        if (pos > target_len)
        {
            pos = target_len; // Append at the end if position exceeds target length
        }

        char *new_str = (char *)malloc(target_len + value_len + 1);
        if (!new_str)
        {
            hex_error("Memory allocation failed");
            FREE(target);
            FREE(index);
            FREE(value);
            return 1;
        }

        strncpy(new_str, target.data.strValue, pos);
        strcpy(new_str + pos, value.data.strValue);
        strcpy(new_str + pos + value_len, target.data.strValue + pos);

        free(target.data.strValue);
        target.data.strValue = new_str;

        if (PUSH(target) != 0)
        {
            free(new_str);
            FREE(target);
            FREE(index);
            FREE(value);
            return 1; // Failed to push the result onto the stack
        }

        return 0;
    }
    else if (target.type == HEX_TYPE_QUOTATION)
    {
        if (index.data.intValue < 0 || index.data.intValue > target.quotationSize)
        {
            hex_error("Invalid index for quotation");
            FREE(target);
            FREE(index);
            FREE(value);
            return 1;
        }

        hex_item_t **new_quotation = (hex_item_t **)realloc(
            target.data.quotationValue,
            (target.quotationSize + 1) * sizeof(hex_item_t *));
        if (!new_quotation)
        {
            hex_error("Memory allocation failed");
            FREE(target);
            FREE(index);
            FREE(value);
            return 1;
        }

        for (size_t i = target.quotationSize; i > (size_t)index.data.intValue; --i)
        {
            new_quotation[i] = new_quotation[i - 1];
        }
        new_quotation[index.data.intValue] = (hex_item_t *)malloc(sizeof(hex_item_t));
        if (!new_quotation[index.data.intValue])
        {
            hex_error("Memory allocation failed");
            FREE(target);
            FREE(index);
            FREE(value);
            return 1;
        }

        *new_quotation[index.data.intValue] = value;
        target.data.quotationValue = new_quotation;
        target.quotationSize++;

        if (PUSH(target) != 0)
        {
            free(new_quotation[index.data.intValue]);
            FREE(target);
            FREE(index);
            FREE(value);
            return 1; // Failed to push the result onto the stack
        }

        return 0;
    }
    else
    {
        hex_error("Target must be a string or quotation");
        FREE(target);
        FREE(index);
        FREE(value);
        return 1;
    }
}

int hex_symbol_index()
{
    POP(element);
    if (element.type == HEX_TYPE_INVALID)
    {
        FREE(element);
        return 1;
    }
    POP(list);
    if (list.type == HEX_TYPE_INVALID)
    {
        FREE(list);
        FREE(element);
        return 1;
    }
    int result = -1;
    if (list.type == HEX_TYPE_QUOTATION)
    {
        for (int i = 0; i < list.quotationSize; i++)
        {
            if (hex_equal(*list.data.quotationValue[i], element))
            {
                result = i;
                break;
            }
        }
    }
    else if (list.type == HEX_TYPE_STRING)
    {
        char *ptr = strstr(list.data.strValue, element.data.strValue);
        if (ptr)
        {
            result = ptr - list.data.strValue;
        }
    }
    else
    {
        hex_error("Symbol 'index' requires a quotation or a string");
        FREE(list);
        FREE(element);
        return 1;
    }
    return hex_push_int(result);
}

// String symbols

int hex_symbol_join()
{
    POP(separator);
    if (separator.type == HEX_TYPE_INVALID)
    {
        FREE(separator);
        return 1;
    }
    POP(list);
    if (list.type == HEX_TYPE_INVALID)
    {
        FREE(list);
        FREE(separator);
        return 1;
    }
    int result = 0;
    if (list.type == HEX_TYPE_QUOTATION && separator.type == HEX_TYPE_STRING)
    {
        int length = 0;
        for (int i = 0; i < list.quotationSize; i++)
        {
            if (list.data.quotationValue[i]->type == HEX_TYPE_STRING)
            {
                length += strlen(list.data.quotationValue[i]->data.strValue);
            }
            else
            {
                hex_error("Quotation must contain only strings");
                FREE(list);
                FREE(separator);
                return 1;
            }
        }
        if (result == 0)
        {
            length += (list.quotationSize - 1) * strlen(separator.data.strValue);
            char *newStr = (char *)malloc(length + 1);
            if (!newStr)
            {
                hex_error("Memory allocation failed");
                FREE(list);
                FREE(separator);
                return 1;
            }
            newStr[0] = '\0';
            for (int i = 0; i < list.quotationSize; i++)
            {
                strcat(newStr, list.data.quotationValue[i]->data.strValue);
                if (i < list.quotationSize - 1)
                {
                    strcat(newStr, separator.data.strValue);
                }
            }
            result = hex_push_string(newStr);
        }
    }
    else
    {
        hex_error("Symbol 'join' requires a quotation and a string");
        result = 1;
    }
    if (result != 0)
    {
        FREE(list);
        FREE(separator);
    }
    return result;
}

int hex_symbol_split()
{
    POP(separator);
    if (separator.type == HEX_TYPE_INVALID)
    {
        FREE(separator);
        return 1;
    }
    POP(str);
    if (str.type == HEX_TYPE_INVALID)
    {
        FREE(str);
        FREE(separator);
        return 1;
    }
    int result = 0;
    if (str.type == HEX_TYPE_STRING && separator.type == HEX_TYPE_STRING)
    {
        char *token = strtok(str.data.strValue, separator.data.strValue);
        int capacity = 2;
        int size = 0;
        hex_item_t **quotation = (hex_item_t **)malloc(capacity * sizeof(hex_item_t *));
        if (!quotation)
        {
            hex_error("Memory allocation failed");
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
                        hex_error("Memory allocation failed");
                        result = 1;
                        break;
                    }
                }
                quotation[size] = (hex_item_t *)malloc(sizeof(hex_item_t));
                quotation[size]->type = HEX_TYPE_STRING;
                quotation[size]->data.strValue = strdup(token);
                size++;
                token = strtok(NULL, separator.data.strValue);
            }
            if (result == 0)
            {
                result = hex_push_quotation(quotation, size);
            }
        }
    }
    else
    {
        hex_error("Symbol 'split' requires two strings");
        result = 1;
    }
    if (result != 0)
    {
        FREE(str);
        FREE(separator);
    }
    return result;
}

int hex_symbol_replace()
{
    POP(replacement);
    if (replacement.type == HEX_TYPE_INVALID)
    {
        FREE(replacement);
        return 1;
    }
    POP(search);
    if (search.type == HEX_TYPE_INVALID)
    {
        FREE(search);
        FREE(replacement);
        return 1;
    }
    POP(list);
    if (list.type == HEX_TYPE_INVALID)
    {
        FREE(list);
        FREE(search);
        FREE(replacement);
        return 1;
    }
    int result = 0;
    if (list.type == HEX_TYPE_STRING && search.type == HEX_TYPE_STRING && replacement.type == HEX_TYPE_STRING)
    {
        char *str = list.data.strValue;
        char *find = search.data.strValue;
        char *replace = replacement.data.strValue;
        char *ptr = strstr(str, find);
        if (ptr)
        {
            int findLen = strlen(find);
            int replaceLen = strlen(replace);
            int newLen = strlen(str) - findLen + replaceLen + 1;
            char *newStr = (char *)malloc(newLen);
            if (!newStr)
            {
                hex_error("Memory allocation failed");
                result = 1;
            }
            else
            {
                strncpy(newStr, str, ptr - str);
                strcpy(newStr + (ptr - str), replace);
                strcpy(newStr + (ptr - str) + replaceLen, ptr + findLen);
                result = hex_push_string(newStr);
            }
        }
        else
        {
            result = hex_push_string(str);
        }
    }
    else
    {
        hex_error("Symbol 'replace' requires three strings");
        result = 1;
    }
    if (result != 0)
    {
        FREE(list);
        FREE(search);
        FREE(replacement);
    }
    return result;
}

// File symbols

int hex_symbol_read()
{
    POP(filename);
    if (filename.type == HEX_TYPE_INVALID)
    {
        FREE(filename);
        return 1;
    }
    int result = 0;
    if (filename.type == HEX_TYPE_STRING)
    {
        FILE *file = fopen(filename.data.strValue, "r");
        if (!file)
        {
            hex_error("Could not open file: %s", filename.data.strValue);
            result = 1;
        }
        else
        {
            fseek(file, 0, SEEK_END);
            long length = ftell(file);
            fseek(file, 0, SEEK_SET);

            char *buffer = (char *)malloc(length + 1);
            if (!buffer)
            {
                hex_error("Memory allocation failed");
                result = 1;
            }
            else
            {
                fread(buffer, 1, length, file);
                buffer[length] = '\0';
                result = hex_push_string(buffer);
                free(buffer);
            }
            fclose(file);
        }
    }
    else
    {
        hex_error("Symbol 'read' requires a string");
        result = 1;
    }
    if (result != 0)
    {
        FREE(filename);
    }
    return result;
}

int hex_symbol_write()
{
    POP(filename);
    if (filename.type == HEX_TYPE_INVALID)
    {
        FREE(filename);
        return 1;
    }
    POP(data);
    if (data.type == HEX_TYPE_INVALID)
    {
        FREE(data);
        FREE(filename);
        return 1;
    }
    int result = 0;
    if (filename.type == HEX_TYPE_STRING)
    {
        if (data.type == HEX_TYPE_STRING)
        {
            FILE *file = fopen(filename.data.strValue, "w");
            if (file)
            {
                fputs(data.data.strValue, file);
                fclose(file);
                result = 0;
            }
            else
            {
                hex_error("Could not open file for writing: %s", filename.data.strValue);
                result = 1;
            }
        }
        else
        {
            hex_error("Symbol 'write' requires a string");
            result = 1;
        }
    }
    else
    {
        hex_error("Symbol 'write' requires a string");
        result = 1;
    }
    if (result != 0)
    {
        FREE(data);
        FREE(filename);
    }
    return result;
}

int hex_symbol_append()
{
    POP(filename);
    if (filename.type == HEX_TYPE_INVALID)
    {
        FREE(filename);
        return 1;
    }
    POP(data);
    if (data.type == HEX_TYPE_INVALID)
    {
        FREE(data);
        FREE(filename);
        return 1;
    }
    int result = 0;
    if (filename.type == HEX_TYPE_STRING)
    {
        if (data.type == HEX_TYPE_STRING)
        {
            FILE *file = fopen(filename.data.strValue, "a");
            if (file)
            {
                fputs(data.data.strValue, file);
                fclose(file);
                result = 0;
            }
            else
            {
                hex_error("Could not open file for appending: %s", filename.data.strValue);
                result = 1;
            }
        }
        else
        {
            hex_error("Symbol 'append' requires a string");
            result = 1;
        }
    }
    else
    {
        hex_error("Symbol 'append' requires a string");
        result = 1;
    }
    if (result != 0)
    {
        FREE(data);
        FREE(filename);
    }
    return result;
}

// Shell symbols

int hex_symbol_args()
{
    hex_item_t **quotation = (hex_item_t **)malloc(HEX_ARGC * sizeof(hex_item_t *));
    if (!quotation)
    {
        hex_error("Memory allocation failed");
        return 1;
    }
    else
    {
        for (int i = 0; i < HEX_ARGC; i++)
        {
            quotation[i] = (hex_item_t *)malloc(sizeof(hex_item_t));
            quotation[i]->type = HEX_TYPE_STRING;
            quotation[i]->data.strValue = HEX_ARGV[i];
        }
        if (hex_push_quotation(quotation, HEX_ARGC) != 0)
        {
            hex_free_list(quotation, HEX_ARGC);
            return 1;
        }
    }
    return 0;
}

int hex_symbol_exit()
{
    POP(element);
    if (element.type == HEX_TYPE_INVALID)
    {
        FREE(element);
        return 1;
    }
    if (element.type != HEX_TYPE_INTEGER)
    {
        hex_error("Exit status must be an integer");
        FREE(element);
        return 1;
    }
    int exit_status = element.data.intValue;
    exit(exit_status);
    return 0; // This line will never be reached, but it's here to satisfy the return type
}

int hex_symbol_exec()
{
    POP(command);
    if (command.type == HEX_TYPE_INVALID)
    {
        FREE(command);
        return 1;
    }
    int result = 0;
    if (command.type == HEX_TYPE_STRING)
    {
        int status = system(command.data.strValue);
        result = hex_push_int(status);
    }
    else
    {
        hex_error("Symbol 'exec' requires a string");
        result = 1;
    }
    if (result != 0)
    {
        FREE(command);
    }
    return result;
}

int hex_symbol_run()
{
    POP(command);
    if (command.type == HEX_TYPE_INVALID)
    {
        FREE(command);
        return 1;
    }
    if (command.type != HEX_TYPE_STRING)
    {
        hex_error("Symbol 'run' requires a string");
        FREE(command);
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
        hex_error("Failed to create pipes");
        FREE(command);
        return 1;
    }

    // Set up STARTUPINFO structure
    ZeroMemory(&si, sizeof(STARTUPINFO));
    si.cb = sizeof(STARTUPINFO);
    si.hStdOutput = hOutputWrite;
    si.hStdError = hErrorWrite;
    si.dwFlags |= STARTF_USESTDHANDLES;

    // Create the child process
    if (!CreateProcess(NULL, command.data.strValue, NULL, NULL, TRUE, 0, NULL, NULL, &si, &pi))
    {
        hex_error("Failed to create process");
        FREE(command);
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
        hex_error("Failed to create pipes");
        FREE(command);
        return 1;
    }

    pid_t pid = fork();
    if (pid == -1)
    {
        hex_error("Failed to fork process");
        FREE(command);
        return 1;
    }
    else if (pid == 0)
    {
        // Child process
        close(stdout_pipe[0]);
        close(stderr_pipe[0]);
        dup2(stdout_pipe[1], STDOUT_FILENO);
        dup2(stderr_pipe[1], STDERR_FILENO);
        execl("/bin/sh", "sh", "-c", command.data.strValue, (char *)NULL);
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
    quotation[0]->data.intValue = return_code;

    quotation[1] = (hex_item_t *)malloc(sizeof(hex_item_t));
    quotation[1]->type = HEX_TYPE_STRING;
    quotation[1]->data.strValue = strdup(output);

    quotation[2] = (hex_item_t *)malloc(sizeof(hex_item_t));
    quotation[2]->type = HEX_TYPE_STRING;
    quotation[2]->data.strValue = strdup(error);

    return hex_push_quotation(quotation, 3);
}

// Control flow symbols

int hex_symbol_if()
{
    POP(elseBlock);
    if (elseBlock.type == HEX_TYPE_INVALID)
    {
        FREE(elseBlock);
        return 1;
    }
    POP(thenBlock);
    if (thenBlock.type == HEX_TYPE_INVALID)
    {
        FREE(thenBlock);
        FREE(elseBlock);
        return 1;
    }
    POP(condition);
    if (condition.type == HEX_TYPE_INVALID)
    {
        FREE(condition);
        FREE(thenBlock);
        FREE(elseBlock);
        return 1;
    }
    if (condition.type != HEX_TYPE_QUOTATION || thenBlock.type != HEX_TYPE_QUOTATION || elseBlock.type != HEX_TYPE_QUOTATION)
    {
        hex_error("'if' symbol requires three quotations");
        return 1;
    }
    else
    {
        for (int i = 0; i < condition.quotationSize; i++)
        {
            if (hex_push(*condition.data.quotationValue[i]) != 0)
            {
                FREE(condition);
                FREE(thenBlock);
                FREE(elseBlock);
                return 1;
            }
        }
        POP(evalResult);
        if (evalResult.type == HEX_TYPE_INTEGER && evalResult.data.intValue > 0)
        {
            for (int i = 0; i < thenBlock.quotationSize; i++)
            {
                if (hex_push(*thenBlock.data.quotationValue[i]) != 0)
                {
                    FREE(condition);
                    FREE(thenBlock);
                    FREE(elseBlock);
                    return 1;
                }
            }
        }
        else
        {
            for (int i = 0; i < elseBlock.quotationSize; i++)
            {
                if (hex_push(*elseBlock.data.quotationValue[i]) != 0)
                {
                    FREE(condition);
                    FREE(thenBlock);
                    FREE(elseBlock);
                    return 1;
                }
            }
        }
    }
    return 0;
}

int hex_symbol_when()
{

    POP(action);
    if (action.type == HEX_TYPE_INVALID)
    {
        FREE(action);
        return 1;
    }
    POP(condition);
    if (condition.type == HEX_TYPE_INVALID)
    {
        FREE(action);
        FREE(condition);
        return 1;
    }
    int result = 0;
    if (condition.type != HEX_TYPE_QUOTATION || action.type != HEX_TYPE_QUOTATION)
    {
        hex_error("'when' symbol requires two quotations");
        result = 1;
    }
    else
    {
        for (int i = 0; i < condition.quotationSize; i++)
        {
            if (hex_push(*condition.data.quotationValue[i]) != 0)
            {
                result = 1;
                break; // Break if pushing the element failed
            }
        }
        POP(evalResult);
        if (evalResult.type == HEX_TYPE_INTEGER && evalResult.data.intValue > 0)
        {
            for (int i = 0; i < action.quotationSize; i++)
            {
                if (hex_push(*action.data.quotationValue[i]) != 0)
                {
                    result = 1;
                    break;
                }
            }
        }
    }
    if (result != 0)
    {
        FREE(action);
        FREE(condition);
    }
    return result;
}

int hex_symbol_while()
{
    POP(action);
    if (action.type == HEX_TYPE_INVALID)
    {
        FREE(action);
        return 1;
    }
    POP(condition);
    if (condition.type == HEX_TYPE_INVALID)
    {
        FREE(action);
        FREE(condition);
        return 1;
    }
    if (condition.type != HEX_TYPE_QUOTATION || action.type != HEX_TYPE_QUOTATION)
    {
        hex_error("'while' symbol requires two quotations");
        FREE(action);
        FREE(condition);
        return 1;
    }
    else
    {
        while (1)
        {
            for (int i = 0; i < condition.quotationSize; i++)
            {
                if (hex_push(*condition.data.quotationValue[i]) != 0)
                {
                    FREE(action);
                    FREE(condition);
                    return 1;
                }
            }
            POP(evalResult);
            if (evalResult.type == HEX_TYPE_INTEGER && evalResult.data.intValue == 0)
            {
                break;
            }
            for (int i = 0; i < action.quotationSize; i++)
            {
                if (hex_push(*action.data.quotationValue[i]) != 0)
                {
                    FREE(action);
                    FREE(condition);
                    return 1;
                }
            }
        }
    }
    return 0;
}

int hex_symbol_each()
{
    POP(action);
    if (action.type == HEX_TYPE_INVALID)
    {
        FREE(action);
        return 1;
    }
    POP(list);
    if (list.type == HEX_TYPE_INVALID)
    {
        FREE(action);
        FREE(list);
        return 1;
    }
    if (list.type != HEX_TYPE_QUOTATION || action.type != HEX_TYPE_QUOTATION)
    {
        hex_error("'each' symbol requires two quotations");
        FREE(action);
        FREE(list);
        return 1;
    }
    else
    {
        for (int i = 0; i < list.quotationSize; i++)
        {
            if (hex_push(*list.data.quotationValue[i]) != 0)
            {
                FREE(action);
                FREE(list);
                return 1;
            }
            for (int j = 0; j < action.quotationSize; j++)
            {
                if (hex_push(*action.data.quotationValue[j]) != 0)
                {
                    FREE(action);
                    FREE(list);
                    return 1;
                }
            }
        }
    }
    return 0;
}

int hex_symbol_error()
{
    char *message = strdup(HEX_ERROR);
    HEX_ERROR[0] = '\0';
    return hex_push_string(message);
}

int hex_symbol_try()
{
    POP(catchBlock);
    if (catchBlock.type == HEX_TYPE_INVALID)
    {
        FREE(catchBlock);
        return 1;
    }
    POP(tryBlock);
    if (tryBlock.type == HEX_TYPE_INVALID)
    {
        FREE(catchBlock);
        FREE(tryBlock);
        return 1;
    }
    if (tryBlock.type != HEX_TYPE_QUOTATION || catchBlock.type != HEX_TYPE_QUOTATION)
    {
        hex_error("'try' symbol requires two quotations");
        FREE(catchBlock);
        FREE(tryBlock);
        return 1;
    }
    else
    {
        char prevError[256];
        strncpy(prevError, HEX_ERROR, sizeof(HEX_ERROR));
        HEX_ERROR[0] = '\0';

        HEX_ERRORS = 0;
        for (int i = 0; i < tryBlock.quotationSize; i++)
        {
            if (hex_push(*tryBlock.data.quotationValue[i]) != 0)
            {
                FREE(catchBlock);
                FREE(tryBlock);
                HEX_ERRORS = 1;
                return 1;
            }
        }
        HEX_ERRORS = 1;

        if (strcmp(HEX_ERROR, ""))
        {
            for (int i = 0; i < catchBlock.quotationSize; i++)
            {
                if (hex_push(*catchBlock.data.quotationValue[i]) != 0)
                {
                    FREE(catchBlock);
                    FREE(tryBlock);
                    return 1;
                }
            }
        }

        strncpy(HEX_ERROR, prevError, sizeof(HEX_ERROR));
    }
    return 0;
}

// Quotation symbols

int hex_symbol_q(void)
{
    POP(element);
    if (element.type == HEX_TYPE_INVALID)
    {
        FREE(element);
        return 1;
    }

    hex_item_t *quotation = (hex_item_t *)malloc(sizeof(hex_item_t));
    if (!quotation)
    {
        hex_error("Memory allocation failed");
        FREE(element);
        return 1;
    }

    *quotation = element;

    hex_item_t result;
    result.type = HEX_TYPE_QUOTATION;
    result.data.quotationValue = (hex_item_t **)malloc(sizeof(hex_item_t *));
    if (!result.data.quotationValue)
    {
        FREE(element);
        free(quotation);
        hex_error("Memory allocation failed");
        return 1;
    }

    result.data.quotationValue[0] = quotation;
    result.quotationSize = 1;

    if (PUSH(result) != 0)
    {
        FREE(element);
        free(quotation);
        free(result.data.quotationValue);
        return 1;
    }

    return 0;
}

int hex_symbol_map()
{
    POP(action);
    if (action.type == HEX_TYPE_INVALID)
    {
        FREE(action);
        return 1;
    }
    POP(list);
    if (list.type == HEX_TYPE_INVALID)
    {
        FREE(action);
        FREE(list);
        return 1;
    }
    if (list.type != HEX_TYPE_QUOTATION || action.type != HEX_TYPE_QUOTATION)
    {
        hex_error("'map' symbol requires two quotations");
        FREE(action);
        FREE(list);
        return 1;
    }
    else
    {
        hex_item_t **quotation = (hex_item_t **)malloc(list.quotationSize * sizeof(hex_item_t *));
        if (!quotation)
        {
            hex_error("Memory allocation failed");
            FREE(action);
            FREE(list);
            return 1;
        }
        for (int i = 0; i < list.quotationSize; i++)
        {
            if (hex_push(*list.data.quotationValue[i]) != 0)
            {
                FREE(action);
                FREE(list);
                hex_free_list(quotation, i);
                return 1;
            }
            for (int j = 0; j < action.quotationSize; j++)
            {
                if (hex_push(*action.data.quotationValue[j]) != 0)
                {
                    FREE(action);
                    FREE(list);
                    hex_free_list(quotation, i);
                    return 1;
                }
            }
            quotation[i] = (hex_item_t *)malloc(sizeof(hex_item_t));
            *quotation[i] = hex_pop();
        }
        if (hex_push_quotation(quotation, list.quotationSize) != 0)
        {
            FREE(action);
            FREE(list);
            hex_free_list(quotation, list.quotationSize);
            return 1;
        }
    }
    return 0;
}

int hex_symbol_filter()
{
    POP(action);
    if (action.type == HEX_TYPE_INVALID)
    {
        FREE(action);
        return 1;
    }
    POP(list);
    if (list.type == HEX_TYPE_INVALID)
    {
        FREE(action);
        FREE(list);
        return 1;
    }
    if (list.type != HEX_TYPE_QUOTATION || action.type != HEX_TYPE_QUOTATION)
    {
        hex_error("'filter' symbol requires two quotations");
        FREE(action);
        FREE(list);
        return 1;
    }
    else
    {
        hex_item_t **quotation = (hex_item_t **)malloc(list.quotationSize * sizeof(hex_item_t *));
        if (!quotation)
        {
            hex_error("Memory allocation failed");
            FREE(action);
            FREE(list);
            return 1;
        }
        int count = 0;
        for (int i = 0; i < list.quotationSize; i++)
        {
            if (hex_push(*list.data.quotationValue[i]) != 0)
            {
                FREE(action);
                FREE(list);
                hex_free_list(quotation, count);
                return 1;
            }
            for (int j = 0; j < action.quotationSize; j++)
            {
                if (hex_push(*action.data.quotationValue[j]) != 0)
                {
                    FREE(action);
                    FREE(list);
                    hex_free_list(quotation, count);
                    return 1;
                }
            }
            POP(evalResult);
            if (evalResult.type == HEX_TYPE_INTEGER && evalResult.data.intValue > 0)
            {
                quotation[count] = (hex_item_t *)malloc(sizeof(hex_item_t));
                if (!quotation[count])
                {
                    hex_error("Memory allocation failed");
                    FREE(action);
                    FREE(list);
                    hex_free_list(quotation, count);
                    return 1;
                }
                *quotation[count] = *list.data.quotationValue[i];
                count++;
            }
        }
        if (hex_push_quotation(quotation, count) != 0)
        {
            FREE(action);
            FREE(list);
            return 1;
        }
        hex_error("An error occurred while filtering the list");
        for (int i = 0; i < count; i++)
        {
            FREE(*quotation[i]);
        }
        return 1;
    }
    return 0;
}

// Stack manipulation symbols

int hex_symbol_swap()
{
    POP(a);
    if (a.type == HEX_TYPE_INVALID)
    {
        FREE(a);
        return 1;
    }
    POP(b);
    if (b.type == HEX_TYPE_INVALID)
    {
        FREE(b);
        FREE(a);
        return 1;
    }
    if (PUSH(a) != 0)
    {
        FREE(a);
        FREE(b);
        return 1;
    }
    if (PUSH(b) != 0)
    {
        FREE(a);
        FREE(b);
        return 1;
    }
    return 0;
}

int hex_symbol_dup()
{
    POP(element);
    if (element.type == HEX_TYPE_INVALID)
    {
        FREE(element);
        return 1;
    }
    if (PUSH(element) == 0 && PUSH(element) == 0)
    {
        return 0;
    }
    FREE(element);
    return 1;
}

int hex_symbol_stack()
{
    hex_item_t **quotation = (hex_item_t **)malloc((HEX_TOP + 1) * sizeof(hex_item_t *));
    if (!quotation)
    {
        hex_error("Memory allocation failed");
        return 1;
    }
    int count = 0;
    for (int i = 0; i <= HEX_TOP; i++)
    {
        quotation[i] = (hex_item_t *)malloc(sizeof(hex_item_t));
        if (!quotation[i])
        {
            hex_error("Memory allocation failed");
            hex_free_list(quotation, count);
            return 1;
        }
        *quotation[i] = HEX_STACK[i];
        count++;
    }

    if (hex_push_quotation(quotation, HEX_TOP + 1) != 0)
    {
        hex_error("An error occurred while fetching stack contents");
        hex_free_list(quotation, count);
        return 1;
    }
    return 0;
}

int hex_symbol_clear()
{
    while (HEX_TOP >= 0)
    {
        FREE(HEX_STACK[HEX_TOP--]);
    }
    HEX_TOP = -1;
    return 0;
}

int hex_symbol_pop()
{
    POP(element);
    FREE(element);
    return 0;
}

////////////////////////////////////////
// Native Symbol Registration         //
////////////////////////////////////////

void hex_register_symbols()
{
    hex_set_native_symbol("store", hex_symbol_store);
    hex_set_native_symbol("free", hex_symbol_free);
    hex_set_native_symbol("type", hex_symbol_type);
    hex_set_native_symbol("i", hex_symbol_i);
    hex_set_native_symbol("eval", hex_symbol_eval);
    hex_set_native_symbol("puts", hex_symbol_puts);
    hex_set_native_symbol("warn", hex_symbol_warn);
    hex_set_native_symbol("print", hex_symbol_print);
    hex_set_native_symbol("gets", hex_symbol_gets);
    hex_set_native_symbol("+", hex_symbol_add);
    hex_set_native_symbol("-", hex_symbol_subtract);
    hex_set_native_symbol("*", hex_symbol_multiply);
    hex_set_native_symbol("/", hex_symbol_divide);
    hex_set_native_symbol("%", hex_symbol_modulo);
    hex_set_native_symbol("&", hex_symbol_bitand);
    hex_set_native_symbol("|", hex_symbol_bitor);
    hex_set_native_symbol("^", hex_symbol_bitxor);
    hex_set_native_symbol("~", hex_symbol_bitnot);
    hex_set_native_symbol("<<", hex_symbol_shiftleft);
    hex_set_native_symbol(">>", hex_symbol_shiftright);
    hex_set_native_symbol("int", hex_symbol_int);
    hex_set_native_symbol("str", hex_symbol_str);
    hex_set_native_symbol("dec", hex_symbol_dec);
    hex_set_native_symbol("hex", hex_symbol_hex);
    hex_set_native_symbol("==", hex_symbol_equal);
    hex_set_native_symbol("!=", hex_symbol_notequal);
    hex_set_native_symbol(">", hex_symbol_greater);
    hex_set_native_symbol("<", hex_symbol_less);
    hex_set_native_symbol(">=", hex_symbol_greaterequal);
    hex_set_native_symbol("<=", hex_symbol_lessequal);
    hex_set_native_symbol("and", hex_symbol_and);
    hex_set_native_symbol("or", hex_symbol_or);
    hex_set_native_symbol("not", hex_symbol_not);
    hex_set_native_symbol("xor", hex_symbol_xor);
    hex_set_native_symbol("cat", hex_symbol_cat);
    hex_set_native_symbol("slice", hex_symbol_slice);
    hex_set_native_symbol("len", hex_symbol_len);
    hex_set_native_symbol("get", hex_symbol_get);
    hex_set_native_symbol("insert", hex_symbol_insert);
    hex_set_native_symbol("index", hex_symbol_index);
    hex_set_native_symbol("join", hex_symbol_join);
    hex_set_native_symbol("split", hex_symbol_split);
    hex_set_native_symbol("replace", hex_symbol_replace);
    hex_set_native_symbol("read", hex_symbol_read);
    hex_set_native_symbol("write", hex_symbol_write);
    hex_set_native_symbol("append", hex_symbol_append);
    hex_set_native_symbol("args", hex_symbol_args);
    hex_set_native_symbol("exit", hex_symbol_exit);
    hex_set_native_symbol("exec", hex_symbol_exec);
    hex_set_native_symbol("run", hex_symbol_run);
    hex_set_native_symbol("if", hex_symbol_if);
    hex_set_native_symbol("when", hex_symbol_when);
    hex_set_native_symbol("while", hex_symbol_while);
    hex_set_native_symbol("each", hex_symbol_each);
    hex_set_native_symbol("error", hex_symbol_error);
    hex_set_native_symbol("try", hex_symbol_try);
    hex_set_native_symbol("q", hex_symbol_q);
    hex_set_native_symbol("map", hex_symbol_map);
    hex_set_native_symbol("filter", hex_symbol_filter);
    hex_set_native_symbol("swap", hex_symbol_swap);
    hex_set_native_symbol("dup", hex_symbol_dup);
    hex_set_native_symbol("stack", hex_symbol_stack);
    hex_set_native_symbol("clear", hex_symbol_clear);
    hex_set_native_symbol("pop", hex_symbol_pop);
}

////////////////////////////////////////
// Hex Interpreter Implementation     //
////////////////////////////////////////

int hex_interpret(const char *code, char *filename, int line, int column)
{
    const char *input = code;
    hex_file_position_t position = {filename, line, column};
    hex_token_t *token = hex_next_token(&input, &position);

    while (token != NULL && token->type != HEX_TOKEN_INVALID)
    {
        int result = 0;
        if (token->type == HEX_TOKEN_INTEGER)
        {
            result = hex_push_int(hex_parse_integer(token->value));
        }
        else if (token->type == HEX_TOKEN_STRING)
        {
            result = hex_push_string(token->value);
        }
        else if (token->type == HEX_TOKEN_SYMBOL)
        {
            token->position.filename = strdup(filename);
            result = hex_push_symbol(token);
        }
        else if (token->type == HEX_TOKEN_QUOTATION_END)
        {
            hex_error("Unexpected end of quotation");
            result = 1;
        }
        else if (token->type == HEX_TOKEN_QUOTATION_START)
        {
            hex_item_t *quotationElement = (hex_item_t *)malloc(sizeof(hex_item_t));
            if (hex_parse_quotation(&input, quotationElement, &position) != 0)
            {
                hex_error("Failed to parse quotation");
                result = 1;
            }
            else
            {
                hex_item_t **quotation = quotationElement->data.quotationValue;
                int quotationSize = quotationElement->quotationSize;
                result = hex_push_quotation(quotation, quotationSize);
            }
            free(quotationElement);
        }

        if (result != 0)
        {
            hex_free_token(token);
            print_stack_trace();
            return result;
        }

        token = hex_next_token(&input, &position);
    }
    if (token != NULL && token->type == HEX_TOKEN_INVALID)
    {
        token->position.filename = strdup(filename);
        add_to_stack_trace(token);
        print_stack_trace();
        return 1;
    }
    hex_free_token(token);
    return 0;
}

// Read a file into a buffer
char *hex_read_file(const char *filename)
{
    FILE *file = fopen(filename, "r");
    if (file == NULL)
    {
        hex_error("Failed to open file");
        return NULL;
    }

    // Allocate an initial buffer
    int bufferSize = 1024; // Start with a 1 KB buffer
    char *content = (char *)malloc(bufferSize);
    if (content == NULL)
    {
        hex_error("Memory allocation failed");
        fclose(file);
        return NULL;
    }

    int bytesReadTotal = 0;
    int bytesRead = 0;

    while ((bytesRead = fread(content + bytesReadTotal, 1, bufferSize - bytesReadTotal, file)) > 0)
    {
        bytesReadTotal += bytesRead;

        // Resize the buffer if necessary
        if (bytesReadTotal == bufferSize)
        {
            bufferSize *= 2; // Double the buffer size
            char *temp = (char *)realloc(content, bufferSize);
            if (temp == NULL)
            {
                hex_error("Memory reallocation failed");
                free(content);
                fclose(file);
                return NULL;
            }
            content = temp;
        }
    }

    if (ferror(file))
    {
        hex_error("Error reading the file");
        free(content);
        fclose(file);
        return NULL;
    }

    // Null-terminate the content
    char *finalContent = (char *)realloc(content, bytesReadTotal + 1);
    if (finalContent == NULL)
    {
        hex_error("Final memory allocation failed");
        free(content);
        fclose(file);
        return NULL;
    }
    content = finalContent;
    content[bytesReadTotal] = '\0';

    fclose(file);
    return content;
}

// REPL implementation
void hex_repl()
{
    char line[1024];

    printf("   _*_ _\n");
    printf("  / \\hex\\*\n");
    printf(" *\\_/_/_/ v%s - Press Ctrl+C to exit.\n", HEX_VERSION);
    printf("      *\n");

    while (1)
    {
        printf("> "); // Prompt
        if (fgets(line, sizeof(line), stdin) == NULL)
        {
            printf("\n"); // Handle EOF (Ctrl+D)
            break;
        }

        // Normalize line endings (remove trailing \r\n or \n)
        line[strcspn(line, "\r\n")] = '\0';

        // Tokenize and process the input
        hex_interpret(line, "<repl>", 1, 1);
        // Print the top element of the stack
        if (HEX_TOP >= 0)
        {
            hex_print_element(stdout, HEX_STACK[HEX_TOP]);
            printf("\n");
        }
    }
}

void hex_handle_sigint(int sig)
{
    (void)sig; // Suppress unused warning
    printf("\n");
    exit(0);
}

// Process piped input from stdin
void hex_process_stdin()
{
    char buffer[8192]; // Adjust buffer size as needed
    int bytesRead = fread(buffer, 1, sizeof(buffer) - 1, stdin);
    if (bytesRead == 0)
    {
        hex_error("Error: No input provided via stdin.");
        return;
    }

    buffer[bytesRead] = '\0'; // Null-terminate the input
    hex_interpret(buffer, "<stdin>", 1, 1);
}

////////////////////////////////////////
// Main Program                       //
////////////////////////////////////////

int main(int argc, char *argv[])
{
    // store argv to global variable
    HEX_ARGV = argv;
    HEX_ARGC = argc;
    // Register SIGINT (Ctrl+C) signal handler
    signal(SIGINT, hex_handle_sigint);

    hex_register_symbols();

    if (argc > 1)
    {

        for (int i = 1; i < argc; i++)
        {
            char *arg = strdup(argv[i]);
            if ((strcmp(arg, "-v") == 0 || strcmp(arg, "--version") == 0))
            {
                printf("%s\n", HEX_VERSION);
                return 0;
            }
            else if ((strcmp(arg, "-d") == 0 || strcmp(arg, "--debug") == 0))
            {
                HEX_DEBUG = 1;
                printf("*** Debug mode enabled ***\n");
            }
            else
            {
                // Process a file
                char *fileContent = hex_read_file(arg);
                if (!fileContent)
                {
                    return 1;
                }
                hex_interpret(fileContent, arg, 1, 1);
                return 0;
            }
        }
    }
    if (!isatty(fileno(stdin)))
    {
        HEX_STACK_TRACE = 0;
        // Process piped input from stdin
        hex_process_stdin();
    }
    else
    {
        HEX_STACK_TRACE = 0;
        // Start REPL
        hex_repl();
    }

    return 0;
}

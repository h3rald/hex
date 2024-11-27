#include "hex.h"

// Common operations
#define POP(ctx, x) hex_item_t x = hex_pop(ctx)
#define FREE(ctx, x) hex_free_element(ctx, x)
#define PUSH(ctx, x) hex_push(ctx, x)

static char *HEX_NATIVE_SYMBOLS_LIST[] = {
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
// Registry Implementation            //
////////////////////////////////////////

int hex_valid_user_symbol(hex_context_t *ctx, const char *symbol)
{
    // Check that key starts with a letter, or underscore
    // and subsequent characters (if any) are letters, numbers, or underscores
    if (strlen(symbol) == 0)
    {
        hex_error(ctx, "Symbol name cannot be an empty string");
        return 0;
    }
    if (!isalpha(symbol[0]) && symbol[0] != '_')
    {
        hex_error(ctx, "Invalid symbol: %s", symbol);
        return 0;
    }
    for (size_t j = 1; j < strlen(symbol); j++)
    {
        if (!isalnum(symbol[j]) && symbol[j] != '_' && symbol[j] != '-')
        {
            hex_error(ctx, "Invalid symbol: %s", symbol);
            return 0;
        }
    }
    return 1;
}

// Add a symbol to the registry
int hex_set_symbol(hex_context_t *ctx, const char *key, hex_item_t value, int native)
{
    (void)(ctx);
    if (!native && hex_valid_user_symbol(ctx, key) == 0)
    {
        return 1;
    }
    for (int i = 0; i < ctx->registry.size; i++)
    {
        if (strcmp(ctx->registry.entries[i].key, key) == 0)
        {
            if (ctx->registry.entries[i].value.type == HEX_TYPE_NATIVE_SYMBOL)
            {
                hex_error(ctx, "Cannot overwrite native symbol %s", key);
                return 1;
            }
            free(ctx->registry.entries[i].key);
            ctx->registry.entries[i].key = strdup(key);
            ctx->registry.entries[i].value = value;
            return 0;
        }
    }

    if (ctx->registry.size >= HEX_REGISTRY_SIZE)
    {
        hex_error(ctx, "Registry overflow");
        hex_free_token(value.token);
        return 1;
    }

    ctx->registry.entries[ctx->registry.size].key = strdup(key);
    ctx->registry.entries[ctx->registry.size].value = value;
    ctx->registry.size++;
    return 0;
}

// Register a native symbol
void hex_set_native_symbol(hex_context_t *ctx, const char *name, int (*func)())
{
    hex_item_t funcElement;
    funcElement.type = HEX_TYPE_NATIVE_SYMBOL;
    funcElement.data.fnValue = func;

    if (hex_set_symbol(ctx, name, funcElement, 1) != 0)
    {
        hex_error(ctx, "Error: Failed to register native symbol '%s'", name);
    }
}

// Get a symbol value from the registry
int hex_get_symbol(hex_context_t *ctx, const char *key, hex_item_t *result)
{
    (void)(ctx);
    for (int i = 0; i < ctx->registry.size; i++)
    {
        if (strcmp(ctx->registry.entries[i].key, key) == 0)
        {
            *result = ctx->registry.entries[i].value;
            return 1;
        }
    }
    return 0;
}

////////////////////////////////////////
// Stack Implementation               //
////////////////////////////////////////

// Push functions
int hex_push(hex_context_t *ctx, hex_item_t element)
{
    (void)(ctx);
    if (ctx->stack.top >= HEX_STACK_SIZE - 1)
    {
        hex_error(ctx, "Stack overflow");
        return 1;
    }
    hex_debug_element(ctx, "PUSH", element);
    int result = 0;
    if (element.type == HEX_TYPE_USER_SYMBOL)
    {
        hex_item_t value;
        if (hex_get_symbol(ctx, element.token->value, &value))
        {
            result = PUSH(ctx, value);
        }
        else
        {
            hex_error(ctx, "Undefined user symbol: %s", element.token->value);
            FREE(ctx, value);
            result = 1;
        }
    }
    else if (element.type == HEX_TYPE_NATIVE_SYMBOL)
    {
        hex_debug_element(ctx, "CALL", element);
        add_to_stack_trace(ctx, element.token);
        result = element.data.fnValue(ctx);
    }
    else
    {
        ctx->stack.entries[++ctx->stack.top] = element;
    }
    if (result == 0)
    {
        hex_debug_element(ctx, "DONE", element);
    }
    else
    {
        hex_debug_element(ctx, "FAIL", element);
    }
    return result;
}

int hex_push_int(hex_context_t *ctx, int value)
{
    hex_item_t element = {.type = HEX_TYPE_INTEGER, .data.intValue = value};
    return PUSH(ctx, element);
}

char *hex_process_string(hex_context_t *ctx, const char *value)
{
    int len = strlen(value);
    char *processedStr = (char *)malloc(len + 1);
    if (!processedStr)
    {
        hex_error(ctx, "Memory allocation failed");
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

int hex_push_string(hex_context_t *ctx, const char *value)
{
    char *processedStr = hex_process_string(ctx, value);
    hex_item_t element = {.type = HEX_TYPE_STRING, .data.strValue = processedStr};
    return PUSH(ctx, element);
}

int hex_push_quotation(hex_context_t *ctx, hex_item_t **quotation, int size)
{
    hex_item_t element = {.type = HEX_TYPE_QUOTATION, .data.quotationValue = quotation, .quotationSize = size};
    return PUSH(ctx, element);
}

int hex_push_symbol(hex_context_t *ctx, hex_token_t *token)
{
    add_to_stack_trace(ctx, token);
    hex_item_t value;
    if (hex_get_symbol(ctx, token->value, &value))
    {
        value.token = token;
        return PUSH(ctx, value);
    }
    else
    {
        hex_error(ctx, "Undefined symbol: %s", token->value);
        return 1;
    }
}

// Pop function
hex_item_t hex_pop(hex_context_t *ctx)
{
    if (ctx->stack.top < 0)
    {
        hex_error(ctx, "Insufficient elements on the stack");
        return (hex_item_t){.type = HEX_TYPE_INVALID};
    }
    hex_debug_element(ctx, " POP", ctx->stack.entries[ctx->stack.top]);
    return ctx->stack.entries[ctx->stack.top--];
}

// Free a stack element
void hex_free_element(hex_context_t *ctx, hex_item_t element)
{
    hex_debug_element(ctx, "FREE", element);
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
                FREE(ctx, *element.data.quotationValue[i]);
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

void hex_free_list(hex_context_t *ctx, hex_item_t **quotation, int size)
{
    hex_error(ctx, "An error occurred while filtering the list");
    for (int i = 0; i < size; i++)
    {
        FREE(ctx, *quotation[i]);
    }
}

////////////////////////////////////////
// Error & Debugging                  //
////////////////////////////////////////

void hex_error(hex_context_t *ctx, const char *format, ...)
{
    va_list args;
    va_start(args, format);
    vsnprintf(ctx->error, sizeof(ctx->error), format, args);
    if (ctx->settings.errors_enabled)
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
        return "native symbol";
    case HEX_TYPE_USER_SYMBOL:
        return "user symbol";
    case HEX_TYPE_INVALID:
        return "invalid";
    default:
        return "unknown";
    }
}

void hex_debug_element(hex_context_t *ctx, const char *message, hex_item_t element)
{
    if (ctx->settings.debugging_enabled)
    {
        fprintf(stdout, "*** %s: ", message);
        hex_print_element(stdout, element);
        fprintf(stdout, "\n");
    }
}

////////////////////////////////////////
// Help System                        //
////////////////////////////////////////

// Hash function (simple djb2 hash)
unsigned int hex_doc_hash(const char *str)
{
    unsigned long hash = 5381;
    int c;
    while ((c = *str++))
    {
        hash = ((hash << 5) + hash) + c; // hash * 33 + c
    }
    return hash % HEX_NATIVE_SYMBOLS;
}

// Insert a DocEntry into the hash table
void hex_doc(hex_doc_dictionary_t *dict, const char *name, const char *description, const char *input, const char *output)
{
    hex_doc_entry_t doc = {.name = name, .description = description, .signature.input = input, .signature.output = output};
    unsigned int index = hex_doc_hash(doc.name);
    // Handle collisions using linear probing
    while (dict->entries[index].name != NULL)
    {
        index = (index + 1) % HEX_NATIVE_SYMBOLS;
    }
    dict->entries[index] = doc;
}

// Lookup a DocEntry by name
hex_doc_entry_t *hex_get_doc(hex_doc_dictionary_t *dict, const char *name)
{
    unsigned int index = hex_doc_hash(name);
    unsigned int start_index = index; // Save the starting index for detection of loops
    while (dict->entries[index].name != NULL)
    {
        if (strcmp(dict->entries[index].name, name) == 0)
        {
            return &dict->entries[index];
        }
        index = (index + 1) % HEX_NATIVE_SYMBOLS;
        if (index == start_index)
        {
            break; // Avoid infinite loops
        }
    }
    return NULL; // Not found
}

hex_doc_dictionary_t hex_create_docs()
{
    hex_doc_dictionary_t docs;
    hex_doc(&docs, "!=", "-", "-", "-");
    hex_doc(&docs, "%%", "-", "-", "-");
    hex_doc(&docs, "&", "-", "-", "-");
    hex_doc(&docs, "*", "-", "-", "-");
    hex_doc(&docs, "+", "-", "-", "-");
    hex_doc(&docs, "-", "-", "-", "-");
    hex_doc(&docs, "/", "-", "-", "-");
    hex_doc(&docs, "<", "-", "-", "-");
    hex_doc(&docs, "<<", "-", "-", "-");
    hex_doc(&docs, "<=", "-", "-", "-");
    hex_doc(&docs, "==", "-", "-", "-");
    hex_doc(&docs, ">", "-", "-", "-");
    hex_doc(&docs, ">=", "-", "-", "-");
    hex_doc(&docs, ">>", "-", "-", "-");
    hex_doc(&docs, "^", "-", "-", "-");
    hex_doc(&docs, "and", "-", "-", "-");
    hex_doc(&docs, "append", "-", "-", "-");
    hex_doc(&docs, "args", "-", "-", "-");
    hex_doc(&docs, "cat", "-", "-", "-");
    hex_doc(&docs, "clear", "-", "-", "-");
    hex_doc(&docs, "dec", "-", "-", "-");
    hex_doc(&docs, "dup", "-", "-", "-");
    hex_doc(&docs, "each", "-", "-", "-");
    hex_doc(&docs, "error", "-", "-", "-");
    hex_doc(&docs, "eval", "-", "-", "-");
    hex_doc(&docs, "exec", "-", "-", "-");
    hex_doc(&docs, "exit", "-", "-", "-");
    hex_doc(&docs, "filter", "-", "-", "-");
    hex_doc(&docs, "free", "-", "-", "-");
    hex_doc(&docs, "get", "-", "-", "-");
    hex_doc(&docs, "gets", "-", "-", "-");
    hex_doc(&docs, "hex", "-", "-", "-");
    hex_doc(&docs, "i", "-", "-", "-");
    hex_doc(&docs, "if", "-", "-", "-");
    hex_doc(&docs, "index", "-", "-", "-");
    hex_doc(&docs, "insert", "-", "-", "-");
    hex_doc(&docs, "int", "-", "-", "-");
    hex_doc(&docs, "join", "-", "-", "-");
    hex_doc(&docs, "len", "-", "-", "-");
    hex_doc(&docs, "map", "-", "-", "-");
    hex_doc(&docs, "not", "-", "-", "-");
    hex_doc(&docs, "or", "-", "-", "-");
    hex_doc(&docs, "pop", "-", "-", "-");
    hex_doc(&docs, "print", "-", "-", "-");
    hex_doc(&docs, "puts", "-", "-", "-");
    hex_doc(&docs, "q", "-", "-", "-");
    hex_doc(&docs, "read", "-", "-", "-");
    hex_doc(&docs, "replace", "-", "-", "-");
    hex_doc(&docs, "run", "-", "-", "-");
    hex_doc(&docs, "slice", "-", "-", "-");
    hex_doc(&docs, "split", "-", "-", "-");
    hex_doc(&docs, "stack", "-", "-", "-");
    hex_doc(&docs, "store", "-", "-", "-");
    hex_doc(&docs, "str", "-", "-", "-");
    hex_doc(&docs, "swap", "-", "-", "-");
    hex_doc(&docs, "try", "-", "-", "-");
    hex_doc(&docs, "type", "-", "-", "-");
    hex_doc(&docs, "warn", "-", "-", "-");
    hex_doc(&docs, "when", "-", "-", "-");
    hex_doc(&docs, "while", "-", "-", "-");
    hex_doc(&docs, "write", "-", "-", "-");
    hex_doc(&docs, "xor", "-", "-", "-");
    hex_doc(&docs, "|", "-", "-", "-");
    hex_doc(&docs, "~", "-", "-", "-");
    return docs;
}

////////////////////////////////////////
// Tokenizer Implementation           //
////////////////////////////////////////

// Process a token from the input
hex_token_t *hex_next_token(hex_context_t *ctx, const char **input, hex_file_position_t *position)
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
                hex_error(ctx, "Unescaped new line in string");
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
            hex_error(ctx, "Unterminated string");
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
        if (hex_valid_native_symbol(token->value) || hex_valid_user_symbol(ctx, token->value))
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
    for (size_t i = 0; i < sizeof(HEX_NATIVE_SYMBOLS_LIST) / sizeof(HEX_NATIVE_SYMBOLS_LIST[0]); i++)
    {
        if (strcmp(symbol, HEX_NATIVE_SYMBOLS_LIST[i]) == 0)
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

int hex_parse_quotation(hex_context_t *ctx, const char **input, hex_item_t *result, hex_file_position_t *position)
{
    hex_item_t **quotation = NULL;
    int capacity = 2;
    int size = 0;
    int balanced = 1;

    quotation = (hex_item_t **)malloc(capacity * sizeof(hex_item_t *));
    if (!quotation)
    {
        hex_error(ctx, "Memory allocation failed");
        return 1;
    }

    hex_token_t *token;
    while ((token = hex_next_token(ctx, input, position)) != NULL)
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
                hex_error(ctx, "Memory allocation failed");
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
            char *processedStr = hex_process_string(ctx, token->value);
            element->type = HEX_TYPE_STRING;
            element->data.strValue = strdup(processedStr);
        }
        else if (token->type == HEX_TOKEN_SYMBOL)
        {
            if (hex_valid_native_symbol(token->value))
            {
                element->type = HEX_TYPE_NATIVE_SYMBOL;
                hex_item_t value;
                if (hex_get_symbol(ctx, token->value, &value))
                {
                    element->token = token;
                    element->type = HEX_TYPE_NATIVE_SYMBOL;
                    element->data.fnValue = value.data.fnValue;
                }
                else
                {
                    hex_error(ctx, "Unable to reference native symbol: %s", token->value);
                    hex_free_token(token);
                    hex_free_list(ctx, quotation, size);
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
            if (hex_parse_quotation(ctx, input, element, position) != 0)
            {
                hex_free_token(token);
                hex_free_list(ctx, quotation, size);
                return 1;
            }
        }
        else if (token->type == HEX_TOKEN_COMMENT)
        {
            // Ignore comments
        }
        else
        {
            hex_error(ctx, "Unexpected token in quotation: %d", token->value);
            hex_free_token(token);
            hex_free_list(ctx, quotation, size);
            return 1;
        }

        quotation[size] = element;
        size++;
    }

    if (balanced != 0)
    {
        hex_error(ctx, "Unterminated quotation");
        hex_free_token(token);
        hex_free_list(ctx, quotation, size);
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

// Add an entry to the circular stack trace
void add_to_stack_trace(hex_context_t *ctx, hex_token_t *token)
{
    int index = (ctx->stack_trace.start + ctx->stack_trace.size) % HEX_STACK_TRACE_SIZE;

    if (ctx->stack_trace.size < HEX_STACK_TRACE_SIZE)
    {
        // Buffer is not full; add element
        ctx->stack_trace.entries[index] = *token;
        ctx->stack_trace.size++;
    }
    else
    {
        // Buffer is full; overwrite the oldest element
        ctx->stack_trace.entries[index] = *token;
        ctx->stack_trace.start = (ctx->stack_trace.start + 1) % HEX_STACK_TRACE_SIZE;
    }
}

// Print the stack trace
void print_stack_trace(hex_context_t *ctx)
{
    if (!ctx->settings.stack_trace_enabled || !ctx->settings.errors_enabled)
    {
        return;
    }
    fprintf(stderr, "[stack trace] (most recent symbol first):\n");

    for (int i = 0; i < ctx->stack_trace.size; i++)
    {
        int index = (ctx->stack_trace.start + ctx->stack_trace.size - 1 - i) % HEX_STACK_TRACE_SIZE;
        hex_token_t token = ctx->stack_trace.entries[index];
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

int hex_symbol_store(hex_context_t *ctx)
{
    (void)(ctx);
    POP(ctx, name);
    if (name.type == HEX_TYPE_INVALID)
    {
        FREE(ctx, name);
        return 1;
    }
    POP(ctx, value);
    if (value.type == HEX_TYPE_INVALID)
    {
        FREE(ctx, name);
        FREE(ctx, value);
        return 1;
    }
    if (name.type != HEX_TYPE_STRING)
    {
        hex_error(ctx, "Symbol name must be a string");
        FREE(ctx, name);
        FREE(ctx, value);
        return 1;
    }
    if (hex_set_symbol(ctx, name.data.strValue, value, 0) != 0)
    {
        hex_error(ctx, "Failed to store variable");
        FREE(ctx, name);
        FREE(ctx, value);
        return 1;
    }
    return 0;
}

int hex_symbol_free(hex_context_t *ctx)
{
    (void)(ctx);

    POP(ctx, element);
    if (element.type == HEX_TYPE_INVALID)
    {
        FREE(ctx, element);
        return 1;
    }
    if (element.type != HEX_TYPE_STRING)
    {
        FREE(ctx, element);
        hex_error(ctx, "Variable name must be a string");
        return 1;
    }
    for (int i = 0; i < ctx->registry.size; i++)
    {
        if (strcmp(ctx->registry.entries[i].key, element.data.strValue) == 0)
        {
            free(ctx->registry.entries[i].key);
            FREE(ctx, ctx->registry.entries[i].value);
            for (int j = i; j < ctx->registry.size - 1; j++)
            {
                ctx->registry.entries[j] = ctx->registry.entries[j + 1];
            }
            ctx->registry.size--;
            FREE(ctx, element);
            return 0;
        }
    }
    FREE(ctx, element);
    return 0;
}

int hex_symbol_type(hex_context_t *ctx)
{
    (void)(ctx);

    POP(ctx, element);
    if (element.type == HEX_TYPE_INVALID)
    {
        FREE(ctx, element);
        return 1;
    }
    return hex_push_string(ctx, hex_type(element.type));
}

// Evaluation symbols

int hex_symbol_i(hex_context_t *ctx)
{
    (void)(ctx);

    POP(ctx, element);
    if (element.type == HEX_TYPE_INVALID)
    {
        FREE(ctx, element);
        return 1;
    }
    if (element.type != HEX_TYPE_QUOTATION)
    {
        hex_error(ctx, "'i' symbol requires a quotation");
        FREE(ctx, element);
        return 1;
    }
    for (int i = 0; i < element.quotationSize; i++)
    {
        if (hex_push(ctx, *element.data.quotationValue[i]) != 0)
        {
            FREE(ctx, element);
            return 1;
        }
    }
    return 0;
}

// evaluate a string
int hex_symbol_eval(hex_context_t *ctx)
{
    (void)(ctx);

    POP(ctx, element);
    if (element.type == HEX_TYPE_INVALID)
    {
        FREE(ctx, element);
        return 1;
    }
    if (element.type != HEX_TYPE_STRING)
    {
        hex_error(ctx, "'eval' symbol requires a string");
        FREE(ctx, element);
        return 1;
    }
    return hex_interpret(ctx, element.data.strValue, "<eval>", 1, 1);
}

// IO Symbols

int hex_symbol_puts(hex_context_t *ctx)
{
    (void)(ctx);

    POP(ctx, element);
    if (element.type == HEX_TYPE_INVALID)
    {
        FREE(ctx, element);
        return 1;
    }
    hex_raw_print_element(stdout, element);
    printf("\n");
    return 0;
}

int hex_symbol_warn(hex_context_t *ctx)
{
    (void)(ctx);

    POP(ctx, element);
    if (element.type == HEX_TYPE_INVALID)
    {
        FREE(ctx, element);
        return 1;
    }
    hex_raw_print_element(stderr, element);
    printf("\n");
    return 0;
}

int hex_symbol_print(hex_context_t *ctx)
{
    (void)(ctx);

    POP(ctx, element);
    if (element.type == HEX_TYPE_INVALID)
    {
        FREE(ctx, element);
        return 1;
    }
    hex_raw_print_element(stdout, element);
    return 0;
}

int hex_symbol_gets(hex_context_t *ctx)
{
    (void)(ctx);

    char input[HEX_STDIN_BUFFER_SIZE]; // Buffer to store the input (adjust size if needed)

    if (fgets(input, sizeof(input), stdin) != NULL)
    {
        // Strip the newline character at the end of the string
        input[strcspn(input, "\n")] = '\0';

        // Push the input string onto the stack
        return hex_push_string(ctx, input);
    }
    else
    {
        hex_error(ctx, "Failed to read input");
        return 1;
    }
}

// Mathematical symbols
int hex_symbol_add(hex_context_t *ctx)
{
    POP(ctx, b);
    if (b.type == HEX_TYPE_INVALID)
    {
        FREE(ctx, b);
        return 1;
    }
    POP(ctx, a);
    if (a.type == HEX_TYPE_INVALID)
    {
        FREE(ctx, a);
        FREE(ctx, b);
        return 1;
    }
    if (a.type == HEX_TYPE_INTEGER && b.type == HEX_TYPE_INTEGER)
    {
        return hex_push_int(ctx, a.data.intValue + b.data.intValue);
    }
    hex_error(ctx, "'+' symbol requires two integers");
    FREE(ctx, a);
    FREE(ctx, b);
    return 1;
}

int hex_symbol_subtract(hex_context_t *ctx)
{
    (void)(ctx);

    POP(ctx, b);
    if (b.type == HEX_TYPE_INVALID)
    {
        FREE(ctx, b);
        return 1;
    }
    POP(ctx, a);
    if (a.type == HEX_TYPE_INVALID)
    {
        FREE(ctx, a);
        FREE(ctx, b);
        return 1;
    }
    if (a.type == HEX_TYPE_INTEGER && b.type == HEX_TYPE_INTEGER)
    {
        return hex_push_int(ctx, a.data.intValue - b.data.intValue);
    }
    hex_error(ctx, "'-' symbol requires two integers");
    FREE(ctx, a);
    FREE(ctx, b);
    return 1;
}

int hex_symbol_multiply(hex_context_t *ctx)
{
    (void)(ctx);

    POP(ctx, b);
    if (b.type == HEX_TYPE_INVALID)
    {
        FREE(ctx, b);
        return 1;
    }
    POP(ctx, a);
    if (a.type == HEX_TYPE_INVALID)
    {
        FREE(ctx, a);
        FREE(ctx, b);
        return 1;
    }
    if (a.type == HEX_TYPE_INTEGER && b.type == HEX_TYPE_INTEGER)
    {
        return hex_push_int(ctx, a.data.intValue * b.data.intValue);
    }
    hex_error(ctx, "'*' symbol requires two integers");
    FREE(ctx, a);
    FREE(ctx, b);
    return 1;
}

int hex_symbol_divide(hex_context_t *ctx)
{
    (void)(ctx);

    POP(ctx, b);
    if (b.type == HEX_TYPE_INVALID)
    {
        FREE(ctx, b);
        return 1;
    }
    POP(ctx, a);
    if (a.type == HEX_TYPE_INVALID)
    {
        FREE(ctx, a);
        FREE(ctx, b);
        return 1;
    }
    if (a.type == HEX_TYPE_INTEGER && b.type == HEX_TYPE_INTEGER)
    {
        if (b.data.intValue == 0)
        {
            hex_error(ctx, "Division by zero");
            return 1;
        }
        return hex_push_int(ctx, a.data.intValue / b.data.intValue);
    }
    hex_error(ctx, "'/' symbol requires two integers");
    FREE(ctx, a);
    FREE(ctx, b);
    return 1;
}

int hex_symbol_modulo(hex_context_t *ctx)
{
    (void)(ctx);

    POP(ctx, b);
    if (b.type == HEX_TYPE_INVALID)
    {
        FREE(ctx, b);
        return 1;
    }
    POP(ctx, a);
    if (a.type == HEX_TYPE_INVALID)
    {
        FREE(ctx, a);
        FREE(ctx, b);
        return 1;
    }
    if (a.type == HEX_TYPE_INTEGER && b.type == HEX_TYPE_INTEGER)
    {
        if (b.data.intValue == 0)
        {
            hex_error(ctx, "Division by zero");
        }
        return hex_push_int(ctx, a.data.intValue % b.data.intValue);
    }
    hex_error(ctx, "'%%' symbol requires two integers");
    FREE(ctx, a);
    FREE(ctx, b);
    return 1;
}

// Bit symbols

int hex_symbol_bitand(hex_context_t *ctx)
{
    (void)(ctx);

    POP(ctx, right);
    if (right.type == HEX_TYPE_INVALID)
    {
        FREE(ctx, right);
        return 1;
    }
    POP(ctx, left);
    if (left.type == HEX_TYPE_INVALID)
    {
        FREE(ctx, left);
        FREE(ctx, right);
        return 1;
    }
    if (left.type == HEX_TYPE_INTEGER && right.type == HEX_TYPE_INTEGER)
    {
        return hex_push_int(ctx, left.data.intValue & right.data.intValue);
    }
    hex_error(ctx, "'&' symbol requires two integers");
    FREE(ctx, left);
    FREE(ctx, right);
    return 1;
}

int hex_symbol_bitor(hex_context_t *ctx)
{
    (void)(ctx);

    POP(ctx, right);
    if (right.type == HEX_TYPE_INVALID)
    {
        FREE(ctx, right);
        return 1;
    }
    POP(ctx, left);
    if (left.type == HEX_TYPE_INVALID)
    {
        FREE(ctx, left);
        FREE(ctx, right);
        return 1;
    }
    if (left.type == HEX_TYPE_INTEGER && right.type == HEX_TYPE_INTEGER)
    {
        return hex_push_int(ctx, left.data.intValue | right.data.intValue);
    }
    hex_error(ctx, "'|' symbol requires two integers");
    FREE(ctx, left);
    FREE(ctx, right);
    return 1;
}

int hex_symbol_bitxor(hex_context_t *ctx)
{
    (void)(ctx);

    POP(ctx, right);
    if (right.type == HEX_TYPE_INVALID)
    {
        FREE(ctx, right);
        return 1;
    }
    POP(ctx, left);
    if (left.type == HEX_TYPE_INVALID)
    {
        FREE(ctx, left);
        FREE(ctx, right);
        return 1;
    }
    if (left.type == HEX_TYPE_INTEGER && right.type == HEX_TYPE_INTEGER)
    {
        return hex_push_int(ctx, left.data.intValue ^ right.data.intValue);
    }
    hex_error(ctx, "'^' symbol requires two integers");
    FREE(ctx, left);
    FREE(ctx, right);
    return 1;
}

int hex_symbol_shiftleft(hex_context_t *ctx)
{
    (void)(ctx);

    POP(ctx, right);
    if (right.type == HEX_TYPE_INVALID)
    {
        FREE(ctx, right);
        return 1;
    }
    POP(ctx, left);
    if (left.type == HEX_TYPE_INVALID)
    {
        FREE(ctx, left);
        FREE(ctx, right);
        return 1;
    }
    if (left.type == HEX_TYPE_INTEGER && right.type == HEX_TYPE_INTEGER)
    {
        return hex_push_int(ctx, left.data.intValue << right.data.intValue);
    }
    hex_error(ctx, "'<<' symbol requires two integers");
    FREE(ctx, left);
    FREE(ctx, right);
    return 1;
}

int hex_symbol_shiftright(hex_context_t *ctx)
{
    (void)(ctx);

    POP(ctx, right);
    if (right.type == HEX_TYPE_INVALID)
    {
        FREE(ctx, right);
        return 1;
    }
    POP(ctx, left);
    if (left.type == HEX_TYPE_INVALID)
    {
        FREE(ctx, left);
        FREE(ctx, right);
        return 1;
    }
    if (left.type == HEX_TYPE_INTEGER && right.type == HEX_TYPE_INTEGER)
    {
        return hex_push_int(ctx, left.data.intValue >> right.data.intValue);
    }
    hex_error(ctx, "'>>' symbol requires two integers");
    FREE(ctx, left);
    FREE(ctx, right);
    return 1;
}

int hex_symbol_bitnot(hex_context_t *ctx)
{
    (void)(ctx);

    POP(ctx, element);
    if (element.type == HEX_TYPE_INVALID)
    {
        FREE(ctx, element);
        return 1;
    }
    if (element.type == HEX_TYPE_INTEGER)
    {
        return hex_push_int(ctx, ~element.data.intValue);
    }
    hex_error(ctx, "'~' symbol requires one integer");
    FREE(ctx, element);
    return 1;
}

// Conversion symbols

int hex_symbol_int(hex_context_t *ctx)
{
    (void)(ctx);

    POP(ctx, a);
    if (a.type == HEX_TYPE_INVALID)
    {
        FREE(ctx, a);
        return 1;
    }
    if (a.type == HEX_TYPE_QUOTATION)
    {
        hex_error(ctx, "Cannot convert a quotation to an integer");
        FREE(ctx, a);
        return 1;
    }
    if (a.type == HEX_TYPE_INTEGER)
    {
        return hex_push_int(ctx, a.data.intValue);
    }
    if (a.type == HEX_TYPE_STRING)
    {
        return hex_push_int(ctx, strtol(a.data.strValue, NULL, 16));
    }
    hex_error(ctx, "Unsupported data type: %s", hex_type(a.type));
    FREE(ctx, a);
    return 1;
}

int hex_symbol_str(hex_context_t *ctx)
{
    (void)(ctx);

    POP(ctx, a);
    if (a.type == HEX_TYPE_INVALID)
    {
        FREE(ctx, a);
        return 1;
    }
    if (a.type == HEX_TYPE_QUOTATION)
    {
        hex_error(ctx, "Cannot convert a quotation to a string");
        FREE(ctx, a);
        return 1;
    }
    if (a.type == HEX_TYPE_INTEGER)
    {
        return hex_push_string(ctx, hex_itoa_hex(a.data.intValue));
    }
    if (a.type == HEX_TYPE_STRING)
    {
        return hex_push_string(ctx, a.data.strValue);
    }
    hex_error(ctx, "Unsupported data type: %s", hex_type(a.type));
    FREE(ctx, a);
    return 1;
}

int hex_symbol_dec(hex_context_t *ctx)
{
    (void)(ctx);

    POP(ctx, a);
    if (a.type == HEX_TYPE_INVALID)
    {
        FREE(ctx, a);
        return 1;
    }
    if (a.type == HEX_TYPE_INTEGER)
    {
        return hex_push_string(ctx, hex_itoa_dec(a.data.intValue));
    }
    hex_error(ctx, "An integer is required");
    FREE(ctx, a);
    return 1;
}

int hex_symbol_hex(hex_context_t *ctx)
{
    (void)(ctx);

    POP(ctx, element);
    if (element.type == HEX_TYPE_INVALID)
    {
        FREE(ctx, element);
        return 1;
    }
    if (element.type == HEX_TYPE_STRING)
    {
        return hex_push_int(ctx, strtol(element.data.strValue, NULL, 10));
    }
    hex_error(ctx, "'hex' symbol requires a string representing a decimal integer");
    FREE(ctx, element);
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

int hex_symbol_equal(hex_context_t *ctx)
{
    (void)(ctx);

    POP(ctx, b);
    if (b.type == HEX_TYPE_INVALID)
    {
        FREE(ctx, b);
        return 1;
    }
    POP(ctx, a);
    if (a.type == HEX_TYPE_INVALID)
    {
        FREE(ctx, a);
        FREE(ctx, b);
        return 1;
    }
    if ((a.type == HEX_TYPE_INTEGER && b.type == HEX_TYPE_INTEGER) || (a.type == HEX_TYPE_STRING && b.type == HEX_TYPE_STRING) || (a.type == HEX_TYPE_QUOTATION && b.type == HEX_TYPE_QUOTATION))
    {
        return hex_push_int(ctx, hex_equal(a, b));
    }
    hex_error(ctx, "'==' symbol requires two integers, two strings, or two quotations");
    FREE(ctx, a);
    FREE(ctx, b);
    return 1;
}

int hex_symbol_notequal(hex_context_t *ctx)
{
    (void)(ctx);

    POP(ctx, b);
    if (b.type == HEX_TYPE_INVALID)
    {
        FREE(ctx, b);
        return 1;
    }
    POP(ctx, a);
    if (a.type == HEX_TYPE_INVALID)
    {
        FREE(ctx, a);
        FREE(ctx, b);
        return 1;
    }
    if ((a.type == HEX_TYPE_INTEGER && b.type == HEX_TYPE_INTEGER) || (a.type == HEX_TYPE_STRING && b.type == HEX_TYPE_STRING) || (a.type == HEX_TYPE_QUOTATION && b.type == HEX_TYPE_QUOTATION))
    {
        return hex_push_int(ctx, !hex_equal(a, b));
    }
    hex_error(ctx, "'!=' symbol requires two integers, two strings, or two quotations");
    FREE(ctx, a);
    FREE(ctx, b);
    return 1;
}

int hex_symbol_greater(hex_context_t *ctx)
{
    (void)(ctx);

    POP(ctx, b);
    if (b.type == HEX_TYPE_INVALID)
    {
        FREE(ctx, b);
        return 1;
    }
    POP(ctx, a);
    if (a.type == HEX_TYPE_INVALID)
    {
        FREE(ctx, a);
        FREE(ctx, b);
        return 1;
    }
    if (a.type == HEX_TYPE_INTEGER && b.type == HEX_TYPE_INTEGER)
    {
        return hex_push_int(ctx, a.data.intValue > b.data.intValue);
    }
    else if (a.type == HEX_TYPE_STRING && b.type == HEX_TYPE_STRING)
    {
        return hex_push_int(ctx, strcmp(a.data.strValue, b.data.strValue) > 0);
    }
    hex_error(ctx, "'>' symbol requires two integers or two strings");
    FREE(ctx, a);
    FREE(ctx, b);
    return 1;
}

int hex_symbol_less(hex_context_t *ctx)
{
    (void)(ctx);

    POP(ctx, b);
    if (b.type == HEX_TYPE_INVALID)
    {
        FREE(ctx, b);
        return 1;
    }
    POP(ctx, a);
    if (a.type == HEX_TYPE_INVALID)
    {
        FREE(ctx, a);
        FREE(ctx, b);
        return 1;
    }
    if (a.type == HEX_TYPE_INTEGER && b.type == HEX_TYPE_INTEGER)
    {
        return hex_push_int(ctx, a.data.intValue < b.data.intValue);
    }
    if (a.type == HEX_TYPE_STRING && b.type == HEX_TYPE_STRING)
    {
        return hex_push_int(ctx, strcmp(a.data.strValue, b.data.strValue) < 0);
    }
    hex_error(ctx, "'<' symbol requires two integers or two strings");
    FREE(ctx, a);
    FREE(ctx, b);
    return 1;
}

int hex_symbol_greaterequal(hex_context_t *ctx)
{
    (void)(ctx);

    POP(ctx, b);
    if (b.type == HEX_TYPE_INVALID)
    {
        FREE(ctx, b);
        return 1;
    }
    POP(ctx, a);
    if (a.type == HEX_TYPE_INVALID)
    {
        FREE(ctx, a);
        FREE(ctx, b);
        return 1;
    }
    if (a.type == HEX_TYPE_INTEGER && b.type == HEX_TYPE_INTEGER)
    {
        return hex_push_int(ctx, a.data.intValue >= b.data.intValue);
    }
    if (a.type == HEX_TYPE_STRING && b.type == HEX_TYPE_STRING)
    {
        return hex_push_int(ctx, strcmp(a.data.strValue, b.data.strValue) >= 0);
    }
    hex_error(ctx, "'>=' symbol requires two integers or two strings");
    FREE(ctx, a);
    FREE(ctx, b);
    return 1;
}

int hex_symbol_lessequal(hex_context_t *ctx)
{
    (void)(ctx);

    POP(ctx, b);
    if (b.type == HEX_TYPE_INVALID)
    {
        FREE(ctx, b);
        return 1;
    }
    POP(ctx, a);
    if (a.type == HEX_TYPE_INVALID)
    {
        FREE(ctx, a);
        FREE(ctx, b);
        return 1;
    }
    if (a.type == HEX_TYPE_INTEGER && b.type == HEX_TYPE_INTEGER)
    {
        return hex_push_int(ctx, a.data.intValue <= b.data.intValue);
    }
    if (a.type == HEX_TYPE_STRING && b.type == HEX_TYPE_STRING)
    {
        return hex_push_int(ctx, strcmp(a.data.strValue, b.data.strValue) <= 0);
    }
    hex_error(ctx, "'<=' symbol requires two integers or two strings");
    FREE(ctx, a);
    FREE(ctx, b);
    return 1;
}

// Boolean symbols

int hex_symbol_and(hex_context_t *ctx)
{
    (void)(ctx);

    POP(ctx, b);
    if (b.type == HEX_TYPE_INVALID)
    {
        FREE(ctx, b);
        return 1;
    }
    POP(ctx, a);
    if (a.type == HEX_TYPE_INVALID)
    {
        FREE(ctx, a);
        FREE(ctx, b);
        return 1;
    }
    if (a.type == HEX_TYPE_INTEGER && b.type == HEX_TYPE_INTEGER)
    {
        return hex_push_int(ctx, a.data.intValue && b.data.intValue);
    }
    hex_error(ctx, "'and' symbol requires two integers");
    FREE(ctx, a);
    FREE(ctx, b);
    return 1;
}

int hex_symbol_or(hex_context_t *ctx)
{
    (void)(ctx);

    POP(ctx, b);
    if (b.type == HEX_TYPE_INVALID)
    {
        FREE(ctx, b);
        return 1;
    }
    POP(ctx, a);
    if (a.type == HEX_TYPE_INVALID)
    {
        FREE(ctx, a);
        FREE(ctx, b);
        return 1;
    }
    if (a.type == HEX_TYPE_INTEGER && b.type == HEX_TYPE_INTEGER)
    {
        return hex_push_int(ctx, a.data.intValue || b.data.intValue);
    }
    hex_error(ctx, "'or' symbol requires two integers");
    FREE(ctx, a);
    FREE(ctx, b);
    return 1;
}

int hex_symbol_not(hex_context_t *ctx)
{
    (void)(ctx);

    POP(ctx, a);
    if (a.type == HEX_TYPE_INVALID)
    {
        FREE(ctx, a);
        return 1;
    }
    if (a.type == HEX_TYPE_INTEGER)
    {
        return hex_push_int(ctx, !a.data.intValue);
    }
    hex_error(ctx, "'not' symbol requires an integer");
    FREE(ctx, a);
    return 1;
}

int hex_symbol_xor(hex_context_t *ctx)
{
    (void)(ctx);

    POP(ctx, b);
    if (b.type == HEX_TYPE_INVALID)
    {
        FREE(ctx, b);
        return 1;
    }
    POP(ctx, a);
    if (a.type == HEX_TYPE_INVALID)
    {
        FREE(ctx, a);
        FREE(ctx, b);
        return 1;
    }
    if (a.type == HEX_TYPE_INTEGER && b.type == HEX_TYPE_INTEGER)
    {
        return hex_push_int(ctx, a.data.intValue ^ b.data.intValue);
    }
    hex_error(ctx, "'xor' symbol requires two integers");
    FREE(ctx, a);
    FREE(ctx, b);
    return 1;
}

// Quotation and String (List) Symbols

int hex_symbol_cat(hex_context_t *ctx)
{
    (void)(ctx);

    POP(ctx, value);
    if (value.type == HEX_TYPE_INVALID)
    {
        FREE(ctx, value);
        return 1; // Failed to pop value
    }

    POP(ctx, list);
    if (list.type == HEX_TYPE_INVALID)
    {
        FREE(ctx, list);
        FREE(ctx, value);
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
            hex_error(ctx, "Memory allocation failed");
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
            result = hex_push_quotation(ctx, list.data.quotationValue, newSize);
        }
    }
    else if (list.type == HEX_TYPE_STRING && value.type == HEX_TYPE_STRING)
    {
        // Concatenate two strings
        size_t newLength = strlen(list.data.strValue) + strlen(value.data.strValue) + 1;
        char *newStr = (char *)malloc(newLength);
        if (!newStr)
        {
            hex_error(ctx, "Memory allocation failed");
            result = 1;
        }
        else
        {
            strcpy(newStr, list.data.strValue);
            strcat(newStr, value.data.strValue);
            result = hex_push_string(ctx, newStr);
        }
    }
    else
    {
        hex_error(ctx, "Symbol 'cat' requires two quotations or two strings");
        result = 1;
    }

    // Free resources if the operation fails
    if (result != 0)
    {
        FREE(ctx, list);
        FREE(ctx, value);
    }

    return result;
}

int hex_symbol_slice(hex_context_t *ctx)
{
    (void)(ctx);

    POP(ctx, end);
    if (end.type == HEX_TYPE_INVALID)
    {
        FREE(ctx, end);
        return 1;
    }
    POP(ctx, start);
    if (start.type == HEX_TYPE_INVALID)
    {
        FREE(ctx, start);
        FREE(ctx, end);
        return 1;
    }
    POP(ctx, list);
    if (list.type == HEX_TYPE_INVALID)
    {
        FREE(ctx, list);
        FREE(ctx, start);
        FREE(ctx, end);
        return 1;
    }
    int result = 0;
    if (list.type == HEX_TYPE_QUOTATION)
    {
        if (start.type != HEX_TYPE_INTEGER || end.type != HEX_TYPE_INTEGER)
        {
            hex_error(ctx, "Slice indices must be integers");
            result = 1;
        }
        else if (start.data.intValue < 0 || start.data.intValue >= list.quotationSize || end.data.intValue < 0 || end.data.intValue >= list.quotationSize)
        {
            hex_error(ctx, "Slice indices out of range");
            result = 1;
        }
        else
        {
            int newSize = end.data.intValue - start.data.intValue + 1;
            hex_item_t **newQuotation = (hex_item_t **)malloc(newSize * sizeof(hex_item_t *));
            if (!newQuotation)
            {
                hex_error(ctx, "Memory allocation failed");
                result = 1;
            }
            else
            {
                for (int i = 0; i < newSize; i++)
                {
                    newQuotation[i] = (hex_item_t *)malloc(sizeof(hex_item_t));
                    *newQuotation[i] = *list.data.quotationValue[start.data.intValue + i];
                }
                result = hex_push_quotation(ctx, newQuotation, newSize);
            }
        }
    }
    else if (list.type == HEX_TYPE_STRING)
    {
        if (start.type != HEX_TYPE_INTEGER || end.type != HEX_TYPE_INTEGER)
        {
            hex_error(ctx, "Slice indices must be integers");
            result = 1;
        }
        else if (start.data.intValue < 0 || start.data.intValue >= (int)strlen(list.data.strValue) || end.data.intValue < 0 || end.data.intValue >= (int)strlen(list.data.strValue))
        {
            hex_error(ctx, "Slice indices out of range");
            result = 1;
        }
        else
        {
            int newSize = end.data.intValue - start.data.intValue + 1;
            char *newStr = (char *)malloc(newSize + 1);
            if (!newStr)
            {
                hex_error(ctx, "Memory allocation failed");
                result = 1;
            }
            else
            {
                strncpy(newStr, list.data.strValue + start.data.intValue, newSize);
                newStr[newSize] = '\0';
                result = hex_push_string(ctx, newStr);
            }
        }
    }
    else
    {
        hex_error(ctx, "Symbol 'slice' requires a quotation or a string");
        result = 1;
    }
    if (result != 0)
    {
        FREE(ctx, list);
        FREE(ctx, start);
        FREE(ctx, end);
    }
    return result;
}

int hex_symbol_len(hex_context_t *ctx)
{
    (void)(ctx);

    POP(ctx, element);
    if (element.type == HEX_TYPE_INVALID)
    {
        FREE(ctx, element);
        return 1;
    }
    int result = 0;
    if (element.type == HEX_TYPE_QUOTATION)
    {
        result = hex_push_int(ctx, element.quotationSize);
    }
    else if (element.type == HEX_TYPE_STRING)
    {
        result = hex_push_int(ctx, strlen(element.data.strValue));
    }
    else
    {
        hex_error(ctx, "Symbol 'len' requires a quotation or a string");
        result = 1;
    }
    if (result != 0)
    {
        FREE(ctx, element);
    }
    return result;
}

int hex_symbol_get(hex_context_t *ctx)
{
    (void)(ctx);

    POP(ctx, index);
    if (index.type == HEX_TYPE_INVALID)
    {
        FREE(ctx, index);
        return 1;
    }
    POP(ctx, list);
    if (list.type == HEX_TYPE_INVALID)
    {
        FREE(ctx, list);
        FREE(ctx, index);
        return 1;
    }
    int result = 0;
    if (list.type == HEX_TYPE_QUOTATION)
    {
        if (index.type != HEX_TYPE_INTEGER)
        {
            hex_error(ctx, "Index must be an integer");
            result = 1;
        }
        else if (index.data.intValue < 0 || index.data.intValue >= list.quotationSize)
        {
            hex_error(ctx, "Index out of range");
            result = 1;
        }
        else
        {
            result = hex_push(ctx, *list.data.quotationValue[index.data.intValue]);
        }
    }
    else if (list.type == HEX_TYPE_STRING)
    {
        if (index.type != HEX_TYPE_INTEGER)
        {
            hex_error(ctx, "Index must be an integer");
            result = 1;
        }
        else if (index.data.intValue < 0 || index.data.intValue >= (int)strlen(list.data.strValue))
        {
            hex_error(ctx, "Index out of range");
            result = 1;
        }
        else
        {
            char str[2] = {list.data.strValue[index.data.intValue], '\0'};
            result = hex_push_string(ctx, str);
        }
    }
    else
    {
        hex_error(ctx, "Symbol 'get' requires a quotation or a string");
        result = 1;
    }
    if (result != 0)
    {

        FREE(ctx, list);
        FREE(ctx, index);
    }
    return result;
}

int hex_symbol_insert(hex_context_t *ctx)
{
    (void)(ctx);
    POP(ctx, index);
    if (index.type == HEX_TYPE_INVALID)
    {
        FREE(ctx, index);
        return 1;
    }
    POP(ctx, value);
    if (value.type == HEX_TYPE_INVALID)
    {
        FREE(ctx, index);
        FREE(ctx, value);
        return 1;
    }

    POP(ctx, target);
    if (target.type == HEX_TYPE_INVALID)
    {
        FREE(ctx, target);
        FREE(ctx, index);
        FREE(ctx, value);
        return 1;
    }

    if (index.type != HEX_TYPE_INTEGER)
    {
        hex_error(ctx, "Index must be an integer");
        FREE(ctx, target);
        FREE(ctx, index);
        FREE(ctx, value);
        return 1;
    }

    if (target.type == HEX_TYPE_STRING)
    {
        if (value.type != HEX_TYPE_STRING)
        {
            hex_error(ctx, "Value must be a string when inserting into a string");
            FREE(ctx, target);
            FREE(ctx, index);
            FREE(ctx, value);
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
            hex_error(ctx, "Memory allocation failed");
            FREE(ctx, target);
            FREE(ctx, index);
            FREE(ctx, value);
            return 1;
        }

        strncpy(new_str, target.data.strValue, pos);
        strcpy(new_str + pos, value.data.strValue);
        strcpy(new_str + pos + value_len, target.data.strValue + pos);

        free(target.data.strValue);
        target.data.strValue = new_str;

        if (PUSH(ctx, target) != 0)
        {
            free(new_str);
            FREE(ctx, target);
            FREE(ctx, index);
            FREE(ctx, value);
            return 1; // Failed to push the result onto the stack
        }

        return 0;
    }
    else if (target.type == HEX_TYPE_QUOTATION)
    {
        if (index.data.intValue < 0 || index.data.intValue > target.quotationSize)
        {
            hex_error(ctx, "Invalid index for quotation");
            FREE(ctx, target);
            FREE(ctx, index);
            FREE(ctx, value);
            return 1;
        }

        hex_item_t **new_quotation = (hex_item_t **)realloc(
            target.data.quotationValue,
            (target.quotationSize + 1) * sizeof(hex_item_t *));
        if (!new_quotation)
        {
            hex_error(ctx, "Memory allocation failed");
            FREE(ctx, target);
            FREE(ctx, index);
            FREE(ctx, value);
            return 1;
        }

        for (size_t i = target.quotationSize; i > (size_t)index.data.intValue; --i)
        {
            new_quotation[i] = new_quotation[i - 1];
        }
        new_quotation[index.data.intValue] = (hex_item_t *)malloc(sizeof(hex_item_t));
        if (!new_quotation[index.data.intValue])
        {
            hex_error(ctx, "Memory allocation failed");
            FREE(ctx, target);
            FREE(ctx, index);
            FREE(ctx, value);
            return 1;
        }

        *new_quotation[index.data.intValue] = value;
        target.data.quotationValue = new_quotation;
        target.quotationSize++;

        if (PUSH(ctx, target) != 0)
        {
            free(new_quotation[index.data.intValue]);
            FREE(ctx, target);
            FREE(ctx, index);
            FREE(ctx, value);
            return 1; // Failed to push the result onto the stack
        }

        return 0;
    }
    else
    {
        hex_error(ctx, "Target must be a string or quotation");
        FREE(ctx, target);
        FREE(ctx, index);
        FREE(ctx, value);
        return 1;
    }
}

int hex_symbol_index(hex_context_t *ctx)
{
    (void)(ctx);

    POP(ctx, element);
    if (element.type == HEX_TYPE_INVALID)
    {
        FREE(ctx, element);
        return 1;
    }
    POP(ctx, list);
    if (list.type == HEX_TYPE_INVALID)
    {
        FREE(ctx, list);
        FREE(ctx, element);
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
        hex_error(ctx, "Symbol 'index' requires a quotation or a string");
        FREE(ctx, list);
        FREE(ctx, element);
        return 1;
    }
    return hex_push_int(ctx, result);
}

// String symbols

int hex_symbol_join(hex_context_t *ctx)
{
    (void)(ctx);

    POP(ctx, separator);
    if (separator.type == HEX_TYPE_INVALID)
    {
        FREE(ctx, separator);
        return 1;
    }
    POP(ctx, list);
    if (list.type == HEX_TYPE_INVALID)
    {
        FREE(ctx, list);
        FREE(ctx, separator);
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
                hex_error(ctx, "Quotation must contain only strings");
                FREE(ctx, list);
                FREE(ctx, separator);
                return 1;
            }
        }
        if (result == 0)
        {
            length += (list.quotationSize - 1) * strlen(separator.data.strValue);
            char *newStr = (char *)malloc(length + 1);
            if (!newStr)
            {
                hex_error(ctx, "Memory allocation failed");
                FREE(ctx, list);
                FREE(ctx, separator);
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
            result = hex_push_string(ctx, newStr);
        }
    }
    else
    {
        hex_error(ctx, "Symbol 'join' requires a quotation and a string");
        result = 1;
    }
    if (result != 0)
    {
        FREE(ctx, list);
        FREE(ctx, separator);
    }
    return result;
}

int hex_symbol_split(hex_context_t *ctx)
{
    (void)(ctx);

    POP(ctx, separator);
    if (separator.type == HEX_TYPE_INVALID)
    {
        FREE(ctx, separator);
        return 1;
    }
    POP(ctx, str);
    if (str.type == HEX_TYPE_INVALID)
    {
        FREE(ctx, str);
        FREE(ctx, separator);
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
            hex_error(ctx, "Memory allocation failed");
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
                        hex_error(ctx, "Memory allocation failed");
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
                result = hex_push_quotation(ctx, quotation, size);
            }
        }
    }
    else
    {
        hex_error(ctx, "Symbol 'split' requires two strings");
        result = 1;
    }
    if (result != 0)
    {
        FREE(ctx, str);
        FREE(ctx, separator);
    }
    return result;
}

int hex_symbol_replace(hex_context_t *ctx)
{
    (void)(ctx);

    POP(ctx, replacement);
    if (replacement.type == HEX_TYPE_INVALID)
    {
        FREE(ctx, replacement);
        return 1;
    }
    POP(ctx, search);
    if (search.type == HEX_TYPE_INVALID)
    {
        FREE(ctx, search);
        FREE(ctx, replacement);
        return 1;
    }
    POP(ctx, list);
    if (list.type == HEX_TYPE_INVALID)
    {
        FREE(ctx, list);
        FREE(ctx, search);
        FREE(ctx, replacement);
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
                hex_error(ctx, "Memory allocation failed");
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
        hex_error(ctx, "Symbol 'replace' requires three strings");
        result = 1;
    }
    if (result != 0)
    {
        FREE(ctx, list);
        FREE(ctx, search);
        FREE(ctx, replacement);
    }
    return result;
}

// File symbols

int hex_symbol_read(hex_context_t *ctx)
{
    (void)(ctx);

    POP(ctx, filename);
    if (filename.type == HEX_TYPE_INVALID)
    {
        FREE(ctx, filename);
        return 1;
    }
    int result = 0;
    if (filename.type == HEX_TYPE_STRING)
    {
        FILE *file = fopen(filename.data.strValue, "r");
        if (!file)
        {
            hex_error(ctx, "Could not open file: %s", filename.data.strValue);
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
                hex_error(ctx, "Memory allocation failed");
                result = 1;
            }
            else
            {
                fread(buffer, 1, length, file);
                buffer[length] = '\0';
                result = hex_push_string(ctx, buffer);
                free(buffer);
            }
            fclose(file);
        }
    }
    else
    {
        hex_error(ctx, "Symbol 'read' requires a string");
        result = 1;
    }
    if (result != 0)
    {
        FREE(ctx, filename);
    }
    return result;
}

int hex_symbol_write(hex_context_t *ctx)
{
    (void)(ctx);

    POP(ctx, filename);
    if (filename.type == HEX_TYPE_INVALID)
    {
        FREE(ctx, filename);
        return 1;
    }
    POP(ctx, data);
    if (data.type == HEX_TYPE_INVALID)
    {
        FREE(ctx, data);
        FREE(ctx, filename);
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
                hex_error(ctx, "Could not open file for writing: %s", filename.data.strValue);
                result = 1;
            }
        }
        else
        {
            hex_error(ctx, "Symbol 'write' requires a string");
            result = 1;
        }
    }
    else
    {
        hex_error(ctx, "Symbol 'write' requires a string");
        result = 1;
    }
    if (result != 0)
    {
        FREE(ctx, data);
        FREE(ctx, filename);
    }
    return result;
}

int hex_symbol_append(hex_context_t *ctx)
{
    (void)(ctx);

    POP(ctx, filename);
    if (filename.type == HEX_TYPE_INVALID)
    {
        FREE(ctx, filename);
        return 1;
    }
    POP(ctx, data);
    if (data.type == HEX_TYPE_INVALID)
    {
        FREE(ctx, data);
        FREE(ctx, filename);
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
                hex_error(ctx, "Could not open file for appending: %s", filename.data.strValue);
                result = 1;
            }
        }
        else
        {
            hex_error(ctx, "Symbol 'append' requires a string");
            result = 1;
        }
    }
    else
    {
        hex_error(ctx, "Symbol 'append' requires a string");
        result = 1;
    }
    if (result != 0)
    {
        FREE(ctx, data);
        FREE(ctx, filename);
    }
    return result;
}

// Shell symbols

int hex_symbol_args(hex_context_t *ctx)
{
    (void)(ctx);

    hex_item_t **quotation = (hex_item_t **)malloc(ctx->argc * sizeof(hex_item_t *));
    if (!quotation)
    {
        hex_error(ctx, "Memory allocation failed");
        return 1;
    }
    else
    {
        for (int i = 0; i < ctx->argc; i++)
        {
            quotation[i] = (hex_item_t *)malloc(sizeof(hex_item_t));
            quotation[i]->type = HEX_TYPE_STRING;
            quotation[i]->data.strValue = ctx->argv[i];
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
    (void)(ctx);

    POP(ctx, element);
    if (element.type == HEX_TYPE_INVALID)
    {
        FREE(ctx, element);
        return 1;
    }
    if (element.type != HEX_TYPE_INTEGER)
    {
        hex_error(ctx, "Exit status must be an integer");
        FREE(ctx, element);
        return 1;
    }
    int exit_status = element.data.intValue;
    exit(exit_status);
    return 0; // This line will never be reached, but it's here to satisfy the return type
}

int hex_symbol_exec(hex_context_t *ctx)
{
    (void)(ctx);

    POP(ctx, command);
    if (command.type == HEX_TYPE_INVALID)
    {
        FREE(ctx, command);
        return 1;
    }
    int result = 0;
    if (command.type == HEX_TYPE_STRING)
    {
        int status = system(command.data.strValue);
        result = hex_push_int(ctx, status);
    }
    else
    {
        hex_error(ctx, "Symbol 'exec' requires a string");
        result = 1;
    }
    if (result != 0)
    {
        FREE(ctx, command);
    }
    return result;
}

int hex_symbol_run(hex_context_t *ctx)
{
    (void)(ctx);

    POP(ctx, command);
    if (command.type == HEX_TYPE_INVALID)
    {
        FREE(ctx, command);
        return 1;
    }
    if (command.type != HEX_TYPE_STRING)
    {
        hex_error(ctx, "Symbol 'run' requires a string");
        FREE(ctx, command);
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
        hex_error(ctx, "Failed to create pipes");
        FREE(ctx, command);
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
        hex_error(ctx, "Failed to create process");
        FREE(ctx, command);
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
        hex_error(ctx, "Failed to create pipes");
        FREE(ctx, command);
        return 1;
    }

    pid_t pid = fork();
    if (pid == -1)
    {
        hex_error(ctx, "Failed to fork process");
        FREE(ctx, command);
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

    return hex_push_quotation(ctx, quotation, 3);
}

// Control flow symbols

int hex_symbol_if(hex_context_t *ctx)
{
    (void)(ctx);

    POP(ctx, elseBlock);
    if (elseBlock.type == HEX_TYPE_INVALID)
    {
        FREE(ctx, elseBlock);
        return 1;
    }
    POP(ctx, thenBlock);
    if (thenBlock.type == HEX_TYPE_INVALID)
    {
        FREE(ctx, thenBlock);
        FREE(ctx, elseBlock);
        return 1;
    }
    POP(ctx, condition);
    if (condition.type == HEX_TYPE_INVALID)
    {
        FREE(ctx, condition);
        FREE(ctx, thenBlock);
        FREE(ctx, elseBlock);
        return 1;
    }
    if (condition.type != HEX_TYPE_QUOTATION || thenBlock.type != HEX_TYPE_QUOTATION || elseBlock.type != HEX_TYPE_QUOTATION)
    {
        hex_error(ctx, "'if' symbol requires three quotations");
        return 1;
    }
    else
    {
        for (int i = 0; i < condition.quotationSize; i++)
        {
            if (hex_push(ctx, *condition.data.quotationValue[i]) != 0)
            {
                FREE(ctx, condition);
                FREE(ctx, thenBlock);
                FREE(ctx, elseBlock);
                return 1;
            }
        }
        POP(ctx, evalResult);
        if (evalResult.type == HEX_TYPE_INTEGER && evalResult.data.intValue > 0)
        {
            for (int i = 0; i < thenBlock.quotationSize; i++)
            {
                if (hex_push(ctx, *thenBlock.data.quotationValue[i]) != 0)
                {
                    FREE(ctx, condition);
                    FREE(ctx, thenBlock);
                    FREE(ctx, elseBlock);
                    return 1;
                }
            }
        }
        else
        {
            for (int i = 0; i < elseBlock.quotationSize; i++)
            {
                if (hex_push(ctx, *elseBlock.data.quotationValue[i]) != 0)
                {
                    FREE(ctx, condition);
                    FREE(ctx, thenBlock);
                    FREE(ctx, elseBlock);
                    return 1;
                }
            }
        }
    }
    return 0;
}

int hex_symbol_when(hex_context_t *ctx)
{
    (void)(ctx);

    POP(ctx, action);
    if (action.type == HEX_TYPE_INVALID)
    {
        FREE(ctx, action);
        return 1;
    }
    POP(ctx, condition);
    if (condition.type == HEX_TYPE_INVALID)
    {
        FREE(ctx, action);
        FREE(ctx, condition);
        return 1;
    }
    int result = 0;
    if (condition.type != HEX_TYPE_QUOTATION || action.type != HEX_TYPE_QUOTATION)
    {
        hex_error(ctx, "'when' symbol requires two quotations");
        result = 1;
    }
    else
    {
        for (int i = 0; i < condition.quotationSize; i++)
        {
            if (hex_push(ctx, *condition.data.quotationValue[i]) != 0)
            {
                result = 1;
                break; // Break if pushing the element failed
            }
        }
        POP(ctx, evalResult);
        if (evalResult.type == HEX_TYPE_INTEGER && evalResult.data.intValue > 0)
        {
            for (int i = 0; i < action.quotationSize; i++)
            {
                if (hex_push(ctx, *action.data.quotationValue[i]) != 0)
                {
                    result = 1;
                    break;
                }
            }
        }
    }
    if (result != 0)
    {
        FREE(ctx, action);
        FREE(ctx, condition);
    }
    return result;
}

int hex_symbol_while(hex_context_t *ctx)
{
    (void)(ctx);

    POP(ctx, action);
    if (action.type == HEX_TYPE_INVALID)
    {
        FREE(ctx, action);
        return 1;
    }
    POP(ctx, condition);
    if (condition.type == HEX_TYPE_INVALID)
    {
        FREE(ctx, action);
        FREE(ctx, condition);
        return 1;
    }
    if (condition.type != HEX_TYPE_QUOTATION || action.type != HEX_TYPE_QUOTATION)
    {
        hex_error(ctx, "'while' symbol requires two quotations");
        FREE(ctx, action);
        FREE(ctx, condition);
        return 1;
    }
    else
    {
        while (1)
        {
            for (int i = 0; i < condition.quotationSize; i++)
            {
                if (hex_push(ctx, *condition.data.quotationValue[i]) != 0)
                {
                    FREE(ctx, action);
                    FREE(ctx, condition);
                    return 1;
                }
            }
            POP(ctx, evalResult);
            if (evalResult.type == HEX_TYPE_INTEGER && evalResult.data.intValue == 0)
            {
                break;
            }
            for (int i = 0; i < action.quotationSize; i++)
            {
                if (hex_push(ctx, *action.data.quotationValue[i]) != 0)
                {
                    FREE(ctx, action);
                    FREE(ctx, condition);
                    return 1;
                }
            }
        }
    }
    return 0;
}

int hex_symbol_each(hex_context_t *ctx)
{
    (void)(ctx);

    POP(ctx, action);
    if (action.type == HEX_TYPE_INVALID)
    {
        FREE(ctx, action);
        return 1;
    }
    POP(ctx, list);
    if (list.type == HEX_TYPE_INVALID)
    {
        FREE(ctx, action);
        FREE(ctx, list);
        return 1;
    }
    if (list.type != HEX_TYPE_QUOTATION || action.type != HEX_TYPE_QUOTATION)
    {
        hex_error(ctx, "'each' symbol requires two quotations");
        FREE(ctx, action);
        FREE(ctx, list);
        return 1;
    }
    else
    {
        for (int i = 0; i < list.quotationSize; i++)
        {
            if (hex_push(ctx, *list.data.quotationValue[i]) != 0)
            {
                FREE(ctx, action);
                FREE(ctx, list);
                return 1;
            }
            for (int j = 0; j < action.quotationSize; j++)
            {
                if (hex_push(ctx, *action.data.quotationValue[j]) != 0)
                {
                    FREE(ctx, action);
                    FREE(ctx, list);
                    return 1;
                }
            }
        }
    }
    return 0;
}

int hex_symbol_error(hex_context_t *ctx)
{
    (void)(ctx);

    char *message = strdup(ctx->error);
    ctx->error[0] = '\0';
    return hex_push_string(ctx, message);
}

int hex_symbol_try(hex_context_t *ctx)
{
    (void)(ctx);

    POP(ctx, catchBlock);
    if (catchBlock.type == HEX_TYPE_INVALID)
    {
        FREE(ctx, catchBlock);
        return 1;
    }
    POP(ctx, tryBlock);
    if (tryBlock.type == HEX_TYPE_INVALID)
    {
        FREE(ctx, catchBlock);
        FREE(ctx, tryBlock);
        return 1;
    }
    if (tryBlock.type != HEX_TYPE_QUOTATION || catchBlock.type != HEX_TYPE_QUOTATION)
    {
        hex_error(ctx, "'try' symbol requires two quotations");
        FREE(ctx, catchBlock);
        FREE(ctx, tryBlock);
        return 1;
    }
    else
    {
        char prevError[256];
        strncpy(prevError, ctx->error, sizeof(ctx->error));
        ctx->error[0] = '\0';

        ctx->settings.errors_enabled = 0;
        for (int i = 0; i < tryBlock.quotationSize; i++)
        {
            if (hex_push(ctx, *tryBlock.data.quotationValue[i]) != 0)
            {
                FREE(ctx, catchBlock);
                FREE(ctx, tryBlock);
                ctx->settings.errors_enabled = 1;
                return 1;
            }
        }
        ctx->settings.errors_enabled = 1;

        if (strcmp(ctx->error, ""))
        {
            for (int i = 0; i < catchBlock.quotationSize; i++)
            {
                if (hex_push(ctx, *catchBlock.data.quotationValue[i]) != 0)
                {
                    FREE(ctx, catchBlock);
                    FREE(ctx, tryBlock);
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
    (void)(ctx);
    POP(ctx, element);
    if (element.type == HEX_TYPE_INVALID)
    {
        FREE(ctx, element);
        return 1;
    }

    hex_item_t *quotation = (hex_item_t *)malloc(sizeof(hex_item_t));
    if (!quotation)
    {
        hex_error(ctx, "Memory allocation failed");
        FREE(ctx, element);
        return 1;
    }

    *quotation = element;

    hex_item_t result;
    result.type = HEX_TYPE_QUOTATION;
    result.data.quotationValue = (hex_item_t **)malloc(sizeof(hex_item_t *));
    if (!result.data.quotationValue)
    {
        FREE(ctx, element);
        free(quotation);
        hex_error(ctx, "Memory allocation failed");
        return 1;
    }

    result.data.quotationValue[0] = quotation;
    result.quotationSize = 1;

    if (PUSH(ctx, result) != 0)
    {
        FREE(ctx, element);
        free(quotation);
        free(result.data.quotationValue);
        return 1;
    }

    return 0;
}

int hex_symbol_map(hex_context_t *ctx)
{
    (void)(ctx);

    POP(ctx, action);
    if (action.type == HEX_TYPE_INVALID)
    {
        FREE(ctx, action);
        return 1;
    }
    POP(ctx, list);
    if (list.type == HEX_TYPE_INVALID)
    {
        FREE(ctx, action);
        FREE(ctx, list);
        return 1;
    }
    if (list.type != HEX_TYPE_QUOTATION || action.type != HEX_TYPE_QUOTATION)
    {
        hex_error(ctx, "'map' symbol requires two quotations");
        FREE(ctx, action);
        FREE(ctx, list);
        return 1;
    }
    else
    {
        hex_item_t **quotation = (hex_item_t **)malloc(list.quotationSize * sizeof(hex_item_t *));
        if (!quotation)
        {
            hex_error(ctx, "Memory allocation failed");
            FREE(ctx, action);
            FREE(ctx, list);
            return 1;
        }
        for (int i = 0; i < list.quotationSize; i++)
        {
            if (hex_push(ctx, *list.data.quotationValue[i]) != 0)
            {
                FREE(ctx, action);
                FREE(ctx, list);
                hex_free_list(ctx, quotation, i);
                return 1;
            }
            for (int j = 0; j < action.quotationSize; j++)
            {
                if (hex_push(ctx, *action.data.quotationValue[j]) != 0)
                {
                    FREE(ctx, action);
                    FREE(ctx, list);
                    hex_free_list(ctx, quotation, i);
                    return 1;
                }
            }
            quotation[i] = (hex_item_t *)malloc(sizeof(hex_item_t));
            *quotation[i] = hex_pop(ctx);
        }
        if (hex_push_quotation(ctx, quotation, list.quotationSize) != 0)
        {
            FREE(ctx, action);
            FREE(ctx, list);
            hex_free_list(ctx, quotation, list.quotationSize);
            return 1;
        }
    }
    return 0;
}

int hex_symbol_filter(hex_context_t *ctx)
{
    (void)(ctx);

    POP(ctx, action);
    if (action.type == HEX_TYPE_INVALID)
    {
        FREE(ctx, action);
        return 1;
    }
    POP(ctx, list);
    if (list.type == HEX_TYPE_INVALID)
    {
        FREE(ctx, action);
        FREE(ctx, list);
        return 1;
    }
    if (list.type != HEX_TYPE_QUOTATION || action.type != HEX_TYPE_QUOTATION)
    {
        hex_error(ctx, "'filter' symbol requires two quotations");
        FREE(ctx, action);
        FREE(ctx, list);
        return 1;
    }
    else
    {
        hex_item_t **quotation = (hex_item_t **)malloc(list.quotationSize * sizeof(hex_item_t *));
        if (!quotation)
        {
            hex_error(ctx, "Memory allocation failed");
            FREE(ctx, action);
            FREE(ctx, list);
            return 1;
        }
        int count = 0;
        for (int i = 0; i < list.quotationSize; i++)
        {
            if (hex_push(ctx, *list.data.quotationValue[i]) != 0)
            {
                FREE(ctx, action);
                FREE(ctx, list);
                hex_free_list(ctx, quotation, count);
                return 1;
            }
            for (int j = 0; j < action.quotationSize; j++)
            {
                if (hex_push(ctx, *action.data.quotationValue[j]) != 0)
                {
                    FREE(ctx, action);
                    FREE(ctx, list);
                    hex_free_list(ctx, quotation, count);
                    return 1;
                }
            }
            POP(ctx, evalResult);
            if (evalResult.type == HEX_TYPE_INTEGER && evalResult.data.intValue > 0)
            {
                quotation[count] = (hex_item_t *)malloc(sizeof(hex_item_t));
                if (!quotation[count])
                {
                    hex_error(ctx, "Memory allocation failed");
                    FREE(ctx, action);
                    FREE(ctx, list);
                    hex_free_list(ctx, quotation, count);
                    return 1;
                }
                *quotation[count] = *list.data.quotationValue[i];
                count++;
            }
        }
        if (hex_push_quotation(ctx, quotation, count) != 0)
        {
            FREE(ctx, action);
            FREE(ctx, list);
            return 1;
        }
        hex_error(ctx, "An error occurred while filtering the list");
        for (int i = 0; i < count; i++)
        {
            FREE(ctx, *quotation[i]);
        }
        return 1;
    }
    return 0;
}

// Stack manipulation symbols

int hex_symbol_swap(hex_context_t *ctx)
{
    (void)(ctx);

    POP(ctx, a);
    if (a.type == HEX_TYPE_INVALID)
    {
        FREE(ctx, a);
        return 1;
    }
    POP(ctx, b);
    if (b.type == HEX_TYPE_INVALID)
    {
        FREE(ctx, b);
        FREE(ctx, a);
        return 1;
    }
    if (PUSH(ctx, a) != 0)
    {
        FREE(ctx, a);
        FREE(ctx, b);
        return 1;
    }
    if (PUSH(ctx, b) != 0)
    {
        FREE(ctx, a);
        FREE(ctx, b);
        return 1;
    }
    return 0;
}

int hex_symbol_dup(hex_context_t *ctx)
{
    (void)(ctx);

    POP(ctx, element);
    if (element.type == HEX_TYPE_INVALID)
    {
        FREE(ctx, element);
        return 1;
    }
    if (PUSH(ctx, element) == 0 && PUSH(ctx, element) == 0)
    {
        return 0;
    }
    FREE(ctx, element);
    return 1;
}

int hex_symbol_stack(hex_context_t *ctx)
{
    (void)(ctx);

    hex_item_t **quotation = (hex_item_t **)malloc((ctx->stack.top + 1) * sizeof(hex_item_t *));
    if (!quotation)
    {
        hex_error(ctx, "Memory allocation failed");
        return 1;
    }
    int count = 0;
    for (int i = 0; i <= ctx->stack.top; i++)
    {
        quotation[i] = (hex_item_t *)malloc(sizeof(hex_item_t));
        if (!quotation[i])
        {
            hex_error(ctx, "Memory allocation failed");
            hex_free_list(ctx, quotation, count);
            return 1;
        }
        *quotation[i] = ctx->stack.entries[i];
        //*quotation[i] = HEX_STACK[i];
        count++;
    }

    if (hex_push_quotation(ctx, quotation, ctx->stack.top + 1) != 0)
    {
        hex_error(ctx, "An error occurred while fetching stack contents");
        hex_free_list(ctx, quotation, count);
        return 1;
    }
    return 0;
}

int hex_symbol_clear(hex_context_t *ctx)
{
    (void)(ctx);

    while (ctx->stack.top >= 0)
    {
        FREE(ctx, ctx->stack.entries[ctx->stack.top--]);
        // FREE(ctx, HEX_STACK[HEX_TOP--]);
    }
    ctx->stack.top = -1;
    return 0;
}

int hex_symbol_pop(hex_context_t *ctx)
{
    (void)(ctx);

    POP(ctx, element);
    FREE(ctx, element);
    return 0;
}

////////////////////////////////////////
// Native Symbol Registration         //
////////////////////////////////////////

void hex_register_symbols(hex_context_t *ctx)
{
    hex_set_native_symbol(ctx, "store", hex_symbol_store);
    hex_set_native_symbol(ctx, "free", hex_symbol_free);
    hex_set_native_symbol(ctx, "type", hex_symbol_type);
    hex_set_native_symbol(ctx, "i", hex_symbol_i);
    hex_set_native_symbol(ctx, "eval", hex_symbol_eval);
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
    hex_set_native_symbol(ctx, "slice", hex_symbol_slice);
    hex_set_native_symbol(ctx, "len", hex_symbol_len);
    hex_set_native_symbol(ctx, "get", hex_symbol_get);
    hex_set_native_symbol(ctx, "insert", hex_symbol_insert);
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
    hex_set_native_symbol(ctx, "each", hex_symbol_each);
    hex_set_native_symbol(ctx, "error", hex_symbol_error);
    hex_set_native_symbol(ctx, "try", hex_symbol_try);
    hex_set_native_symbol(ctx, "q", hex_symbol_q);
    hex_set_native_symbol(ctx, "map", hex_symbol_map);
    hex_set_native_symbol(ctx, "filter", hex_symbol_filter);
    hex_set_native_symbol(ctx, "swap", hex_symbol_swap);
    hex_set_native_symbol(ctx, "dup", hex_symbol_dup);
    hex_set_native_symbol(ctx, "stack", hex_symbol_stack);
    hex_set_native_symbol(ctx, "clear", hex_symbol_clear);
    hex_set_native_symbol(ctx, "pop", hex_symbol_pop);
}

////////////////////////////////////////
// Hex Interpreter Implementation     //
////////////////////////////////////////

hex_context_t hex_init()
{
    hex_context_t context;
    context.argc = 0;
    context.argv = NULL;
    context.registry.size = 0;
    context.stack.top = -1;
    context.stack_trace.start = 0;
    context.stack_trace.size = 0;
    context.settings.debugging_enabled = 0;
    context.settings.errors_enabled = 1;
    context.settings.stack_trace_enabled = 1;
    return context;
}

int hex_interpret(hex_context_t *ctx, const char *code, const char *filename, int line, int column)
{
    (void)(ctx);
    const char *input = code;
    hex_file_position_t position = {filename, line, column};
    hex_token_t *token = hex_next_token(ctx, &input, &position);

    while (token != NULL && token->type != HEX_TOKEN_INVALID)
    {
        int result = 0;
        if (token->type == HEX_TOKEN_INTEGER)
        {
            result = hex_push_int(ctx, hex_parse_integer(token->value));
        }
        else if (token->type == HEX_TOKEN_STRING)
        {
            result = hex_push_string(ctx, token->value);
        }
        else if (token->type == HEX_TOKEN_SYMBOL)
        {
            token->position.filename = strdup(filename);
            result = hex_push_symbol(ctx, token);
        }
        else if (token->type == HEX_TOKEN_QUOTATION_END)
        {
            hex_error(ctx, "Unexpected end of quotation");
            result = 1;
        }
        else if (token->type == HEX_TOKEN_QUOTATION_START)
        {
            hex_item_t *quotationElement = (hex_item_t *)malloc(sizeof(hex_item_t));
            if (hex_parse_quotation(ctx, &input, quotationElement, &position) != 0)
            {
                hex_error(ctx, "Failed to parse quotation");
                result = 1;
            }
            else
            {
                hex_item_t **quotation = quotationElement->data.quotationValue;
                int quotationSize = quotationElement->quotationSize;
                result = hex_push_quotation(ctx, quotation, quotationSize);
            }
            free(quotationElement);
        }

        if (result != 0)
        {
            hex_free_token(token);
            print_stack_trace(ctx);
            return result;
        }

        token = hex_next_token(ctx, &input, &position);
    }
    if (token != NULL && token->type == HEX_TOKEN_INVALID)
    {
        token->position.filename = strdup(filename);
        add_to_stack_trace(ctx, token);
        print_stack_trace(ctx);
        return 1;
    }
    hex_free_token(token);
    return 0;
}

// Read a file into a buffer
char *hex_read_file(hex_context_t *ctx, const char *filename)
{
    FILE *file = fopen(filename, "r");
    if (file == NULL)
    {
        hex_error(ctx, "Failed to open file");
        return NULL;
    }

    // Allocate an initial buffer
    int bufferSize = 1024; // Start with a 1 KB buffer
    char *content = (char *)malloc(bufferSize);
    if (content == NULL)
    {
        hex_error(ctx, "Memory allocation failed");
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
                hex_error(ctx, "Memory reallocation failed");
                free(content);
                fclose(file);
                return NULL;
            }
            content = temp;
        }
    }

    if (ferror(file))
    {
        hex_error(ctx, "Error reading the file");
        free(content);
        fclose(file);
        return NULL;
    }

    // Null-terminate the content
    char *finalContent = (char *)realloc(content, bytesReadTotal + 1);
    if (finalContent == NULL)
    {
        hex_error(ctx, "Final memory allocation failed");
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
void hex_repl(hex_context_t *ctx)
{
    (void)(ctx);
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
        hex_interpret(ctx, line, "<repl>", 1, 1);
        // Print the top element of the stack
        if (ctx->stack.top >= 0)
        {
            hex_print_element(stdout, ctx->stack.entries[ctx->stack.top]);
            // hex_print_element(stdout, HEX_STACK[HEX_TOP]);
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
void hex_process_stdin(hex_context_t *ctx)
{
    (void)(ctx);
    char buffer[8192]; // Adjust buffer size as needed
    int bytesRead = fread(buffer, 1, sizeof(buffer) - 1, stdin);
    if (bytesRead == 0)
    {
        hex_error(ctx, "Error: No input provided via stdin.");
        return;
    }

    buffer[bytesRead] = '\0'; // Null-terminate the input
    hex_interpret(ctx, buffer, "<stdin>", 1, 1);
}

void hex_print_help(hex_doc_dictionary_t *docs)
{
    printf("   _*_ _\n");
    printf("  / \\hex\\*\n");
    printf(" *\\_/_/_/ v%s\n", HEX_VERSION);
    printf("      *\n");
    (void)(docs);
}

void hex_print_doc(hex_doc_entry_t *doc)
{
    printf("  Symbol '%s'\n", doc->name);
    printf("  %s => %s\n", doc->signature.input, doc->signature.output);
    printf("  %s\n", doc->description);
}

////////////////////////////////////////
// Main Program                       //
////////////////////////////////////////

int main(int argc, char *argv[])
{
    // Register SIGINT (Ctrl+C) signal handler
    signal(SIGINT, hex_handle_sigint);

    // Initialize the context
    hex_context_t ctx = hex_init();
    ctx.argc = argc;
    ctx.argv = argv;

    hex_register_symbols(&ctx);
    hex_doc_dictionary_t docs = hex_create_docs();

    if (argc > 1)
    {
        int help = 0;
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
                ctx.settings.debugging_enabled = 1;
                printf("*** Debug mode enabled ***\n");
            }
            else if ((strcmp(arg, "-h") == 0 || strcmp(arg, "--help") == 0))
            {
                help = 1;
            }
            else
            {
                if (help)
                {
                    // Lookup symbol
                    hex_doc_entry_t *doc = hex_get_doc(&docs, arg);
                    help = 0;
                    if (doc)
                    {
                        hex_print_doc(doc);
                        return 0;
                    }
                    else
                    {
                        fprintf(stderr, "Symbol '%s' does not exist.\n", arg);
                        return 1;
                    }
                }
                else
                {
                    // Process a file
                    char *fileContent = hex_read_file(&ctx, arg);
                    if (!fileContent)
                    {
                        return 1;
                    }
                    hex_interpret(&ctx, fileContent, arg, 1, 1);
                    return 0;
                }
            }
        }
        if (help)
        {
            // TODO print help
            hex_print_help(&docs);
            return 0;
        }
    }
    if (!isatty(fileno(stdin)))
    {
        ctx.settings.stack_trace_enabled = 0;
        // Process piped input from stdin
        hex_process_stdin(&ctx);
    }
    else
    {
        ctx.settings.stack_trace_enabled = 0;
        // Start REPL
        hex_repl(&ctx);
    }

    return 0;
}

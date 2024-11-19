#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <signal.h>
#include <stdarg.h>
#ifdef _WIN32
#include <windows.h>
#include <io.h>
int isatty(int fd)
{
    HANDLE h = (HANDLE)_get_osfhandle(fd);
    return (GetFileType(h) == FILE_TYPE_CHAR);
}
#else
#include <unistd.h>
#include <sys/wait.h>
#endif

#define HEX_STDIN_BUFFER_SIZE 256
#define HEX_REGISTRY_SIZE 1024
#define HEX_STACK_SIZE 100

int HEX_DEBUG = 0;
char HEX_ERROR[256] = "";
char **HEX_ARGV;
int HEX_ARGC = 0;
volatile sig_atomic_t HEX_KEEP_RUNNING = 1;
int HEX_ERRORS = 1;
int HEX_QUOTATION_BALANCE = 0;

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

// Enum to represent the type of stack elements
typedef enum
{
    HEX_TYPE_INTEGER,
    HEX_TYPE_STRING,
    HEX_TYPE_QUOTATION,
    HEX_TYPE_NATIVE_SYMBOL,
    HEX_TYPE_USER_SYMBOL,
    HEX_TYPE_INVALID
} HEX_ElementType;

// Unified Stack Element
typedef struct HEX_StackElement
{
    HEX_ElementType type;
    union
    {
        int intValue;
        char *strValue;
        int (*functionPointer)();
        struct HEX_StackElement **quotationValue;
    } data;
    char *symbolName;     // Symbol name (valid for HEX_TYPE_NATIVE_SYMBOL and HEX_TYPE_USER_SYMBOL)
    size_t quotationSize; // Size of the quotation (valid for HEX_TYPE_QUOTATION)
} HEX_StackElement;

////////////////////////////////////////
// Registry Implementation            //
////////////////////////////////////////

// Registry Entry
typedef struct
{
    char *key;
    HEX_StackElement value;
} HEX_RegistryEntry;

HEX_RegistryEntry hex_registry[HEX_REGISTRY_SIZE];
int hex_dictCount = 0;

void hex_free_element(HEX_StackElement element);

int hex_valid_symbol(const char *symbol)
{
    // Check that key starts with a letter, or underscore
    // and subsequent characters (if any) are letters, numbers, or underscores
    if (strlen(symbol) == 0)
    {
        hex_error("Symbol name cannot be an empty string");
        return 1;
    }
    if (!isalpha(symbol[0]) && symbol[0] != '_')
    {
        hex_error("Invalid symbol name: %s", symbol);
        return 1;
    }
    for (int j = 1; j < strlen(symbol); j++)
    {
        if (!isalnum(symbol[j]) && symbol[j] != '_')
        {
            hex_error("Invalid symbol name: %s", symbol);
            return 1;
        }
    }
    return 0;
}

// Add a symbol to the registry
int hex_set_symbol(const char *key, HEX_StackElement value, int native)
{
    if (!native && hex_valid_symbol(key) != 0)
    {
        return 1;
    }
    for (int i = 0; i < hex_dictCount; i++)
    {
        if (strcmp(hex_registry[i].key, key) == 0)
        {
            if (hex_registry[i].value.type == HEX_TYPE_NATIVE_SYMBOL)
            {
                hex_error("Cannot overwrite native symbol %s", key);
                return 1;
            }
            free(hex_registry[i].key);
            hex_free_element(hex_registry[i].value);
            value.symbolName = strdup(key);
            hex_registry[i].key = strdup(key);
            hex_registry[i].value = value;
            return 0;
        }
    }

    if (hex_dictCount >= HEX_REGISTRY_SIZE)
    {
        hex_error("Registry overflow");
        return 1;
    }

    hex_registry[hex_dictCount].key = strdup(key);
    hex_registry[hex_dictCount].value = value;
    hex_dictCount++;
    return 0;
}

// Register a native symbol
void hex_set_native_symbol(const char *name, int (*func)())
{
    HEX_StackElement funcElement;
    funcElement.type = HEX_TYPE_NATIVE_SYMBOL;
    funcElement.data.functionPointer = func;
    funcElement.symbolName = strdup(name);

    if (hex_set_symbol(name, funcElement, 1) != 0)
    {
        hex_error("Error: Failed to register native symbol '%s'", name);
    }
}

// Get a symbol value from the registry
int hex_get_symbol(const char *key, HEX_StackElement *result)
{
    for (int i = 0; i < hex_dictCount; i++)
    {
        if (strcmp(hex_registry[i].key, key) == 0)
        {
            *result = hex_registry[i].value;
            return 1;
        }
    }
    return 0;
}

////////////////////////////////////////
// Stack Implementation               //
////////////////////////////////////////

void hex_debug_element(const char *message, HEX_StackElement element);

HEX_StackElement hex_stack[HEX_STACK_SIZE];
int hex_top = -1;

// Push functions
int hex_push(HEX_StackElement element)
{
    if (hex_top >= HEX_STACK_SIZE - 1)
    {
        hex_error("Stack overflow");
        return 1;
    }
    hex_debug_element("PUSH", element);
    // Note: from a parser perspective, there is no way to determine if a symbol is a native symbol or a user symbol
    if (element.type == HEX_TYPE_USER_SYMBOL)
    {
        HEX_StackElement value;
        int result = 0;
        if (hex_get_symbol(element.symbolName, &value))
        {
            if (value.type == HEX_TYPE_NATIVE_SYMBOL)
            {
                result = value.data.functionPointer();
            }
            else
            {
                result = hex_push(value);
            }
        }
        else
        {
            hex_error("Undefined symbol: %s", element.symbolName);
            result = 1;
        }
        hex_free_element(value);
        return result;
    }
    else if (element.type == HEX_TYPE_NATIVE_SYMBOL)
    {
        return element.data.functionPointer();
    }
    hex_stack[++hex_top] = element;
    return 0;
}

int hex_push_int(int value)
{
    HEX_StackElement element = {.type = HEX_TYPE_INTEGER, .data.intValue = value};
    return hex_push(element);
}

int hex_push_string(const char *value)
{
    HEX_StackElement element = {.type = HEX_TYPE_STRING, .data.strValue = strdup(value)};
    return hex_push(element);
}

int hex_push_quotation(HEX_StackElement **quotation, size_t size)
{
    HEX_StackElement element = {.type = HEX_TYPE_QUOTATION, .data.quotationValue = quotation, .quotationSize = size};
    return hex_push(element);
}

int hex_push_symbol(const char *name)
{
    HEX_StackElement value;
    if (hex_get_symbol(name, &value))
    {
        return hex_push(value);
    }
    else
    {
        hex_error("Undefined symbol: %s", name);
        return 1;
    }
}

// Pop function
HEX_StackElement hex_pop()
{
    if (hex_top < 0)
    {
        hex_error("Insufficient elements on the stack");
        return (HEX_StackElement){.type = HEX_TYPE_INVALID};
    }
    hex_debug_element(" POP", hex_stack[hex_top]);
    return hex_stack[hex_top--];
}

// Free a stack element
void hex_free_element(HEX_StackElement element)
{
    if (element.type == HEX_TYPE_STRING)
    {
        free(element.data.strValue);
    }
    else if (element.type == HEX_TYPE_QUOTATION)
    {
        for (size_t i = 0; i < element.quotationSize; i++)
        {
            hex_free_element(*element.data.quotationValue[i]);
            free(element.data.quotationValue[i]);
        }
        free(element.data.quotationValue);
    }
}

////////////////////////////////////////
// Debugging                          //
////////////////////////////////////////

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

void hex_print_element(FILE *stream, HEX_StackElement element);

char *hex_type(HEX_ElementType type)
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
        return "symbol";
    case HEX_TYPE_USER_SYMBOL:
        return "symbol";
    case HEX_TYPE_INVALID:
        return "invalid";
    default:
        return "unknown";
    }
}

void hex_debug_element(const char *message, HEX_StackElement element)
{
    if (HEX_DEBUG)
    {
        fprintf(stdout, "*** %s: ", message);
        hex_print_element(stdout, element);
        // printf(" [%s]", hex_type(element.type));
        fprintf(stdout, "\n");
    }
}

////////////////////////////////////////
// Tokenizer Implementation           //
////////////////////////////////////////

// Token Types
typedef enum
{
    HEX_TOKEN_NUMBER,
    HEX_TOKEN_STRING,
    HEX_TOKEN_SYMBOL,
    HEX_TOKEN_QUOTATION_START,
    HEX_TOKEN_QUOTATION_END,
    HEX_TOKEN_INVALID
} HEX_TokenType;

typedef struct
{
    HEX_TokenType type;
    char *value;
} HEX_Token;

// Process a token from the input
HEX_Token *hex_next_token(const char **input) {
    const char *ptr = *input;

    // Static balance counter to track nested quotations
    static int quotation_balance = 0;

    // Skip whitespace and comments
    while (isspace(*ptr) || *ptr == ';') {
        if (*ptr == ';') {
            while (*ptr != '\0' && *ptr != '\n') {
                ptr++;
            }
        }
        ptr++;
    }

    if (*ptr == '\0') {
        if (quotation_balance != 0) {
            hex_error("Unbalanced quotations at end of input");
        }
        return NULL; // End of input
    }

    HEX_Token *token = (HEX_Token *)malloc(sizeof(HEX_Token));
    token->value = NULL;

    if (*ptr == '"') {
        // String token
        ptr++;
        const char *start = ptr;
        size_t len = 0;

        while (*ptr != '\0') {
            if (*ptr == '\\' && *(ptr + 1) == '"') {
                ptr += 2;
                len++;
            } else if (*ptr == '"') {
                break;
            } else {
                ptr++;
                len++;
            }
        }

        if (*ptr != '"') {
            hex_error("Unterminated string");
            token->type = HEX_TOKEN_INVALID;
            return token;
        }

        token->value = (char *)malloc(len + 1);
        char *dst = token->value;

        ptr = start;
        while (*ptr != '\0' && *ptr != '"') {
            if (*ptr == '\\' && *(ptr + 1) == '"') {
                *dst++ = '"';
                ptr += 2;
            } else {
                *dst++ = *ptr++;
            }
        }
        *dst = '\0';
        ptr++;
        token->type = HEX_TOKEN_STRING;
    } else if (strncmp(ptr, "0x", 2) == 0 || strncmp(ptr, "0X", 2) == 0) {
        // Hexadecimal integer token
        const char *start = ptr;
        ptr += 2; // Skip the "0x" prefix
        while (isxdigit(*ptr)) {
            ptr++;
        }
        size_t len = ptr - start;
        token->value = (char *)malloc(len + 1);
        strncpy(token->value, start, len);
        token->value[len] = '\0';
        token->type = HEX_TOKEN_NUMBER;
    } else if (*ptr == '(') {
        // Quotation start
        quotation_balance++;
        token->type = HEX_TOKEN_QUOTATION_START;
        ptr++;
    } else if (*ptr == ')') {
        // Quotation end
        if (quotation_balance == 0) {
            hex_error("Unexpected closing parenthesis");
            token->type = HEX_TOKEN_INVALID;
            return token;
        }
        quotation_balance--;
        token->type = HEX_TOKEN_QUOTATION_END;
        ptr++;
    } else {
        // Symbol token
        const char *start = ptr;
        while (*ptr != '\0' && !isspace(*ptr) && *ptr != ';' && *ptr != '(' && *ptr != ')' && *ptr != '"') {
            ptr++;
        }
        size_t len = ptr - start;
        token->value = (char *)malloc(len + 1);
        strncpy(token->value, start, len);
        token->value[len] = '\0';
        token->type = HEX_TOKEN_SYMBOL;
    }

    *input = ptr;
    return token;
}

// Free a token
void hex_free_token(HEX_Token *token)
{
    if (token)
    {
        free(token->value);
        free(token);
    }
}

int hex_parse_quotation(const char **input, HEX_StackElement *result, int balance) {
    HEX_StackElement **quotation = NULL;
    size_t capacity = 2;
    size_t size = 0;

    quotation = (HEX_StackElement **)malloc(capacity * sizeof(HEX_StackElement *));
    if (!quotation) {
        hex_error("Memory allocation failed");
        return 1;
    }

    HEX_Token *token;
    while ((token = hex_next_token(input)) != NULL) {
        if (token->type == HEX_TOKEN_QUOTATION_END) {
            balance--;
            if (balance < 0) {
                hex_error("Unexpected closing parenthesis");
                hex_free_token(token);
                free(quotation);
                return 1;
            }
            hex_free_token(token);
            break;
        }

        if (size >= capacity) {
            capacity *= 2;
            quotation = (HEX_StackElement **)realloc(quotation, capacity * sizeof(HEX_StackElement *));
            if (!quotation) {
                hex_error("Memory allocation failed");
                return 1;
            }
        }

        HEX_StackElement *element = (HEX_StackElement *)malloc(sizeof(HEX_StackElement));
        if (token->type == HEX_TOKEN_NUMBER) {
            element->type = HEX_TYPE_INTEGER;
            element->data.intValue = (int)strtol(token->value, NULL, 16);
        } else if (token->type == HEX_TOKEN_STRING) {
            element->type = HEX_TYPE_STRING;
            element->data.strValue = strdup(token->value);
        } else if (token->type == HEX_TOKEN_SYMBOL) {
            element->type = HEX_TYPE_USER_SYMBOL;
            element->symbolName = strdup(token->value);
        } else if (token->type == HEX_TOKEN_QUOTATION_START) {
            balance++;
            element->type = HEX_TYPE_QUOTATION;
            if (hex_parse_quotation(input, element, balance) != 0) {
                hex_free_token(token);
                free(quotation);
                return 1;
            }
        } else {
            hex_error("Unexpected token in quotation");
            hex_free_token(token);
            free(quotation);
            return 1;
        }

        quotation[size] = element;
        size++;
        hex_free_token(token);
    }

    if (balance != 0) {
        hex_error("Unbalanced parentheses at end of quotation");
        free(quotation);
        return 1;
    }

    result->type = HEX_TYPE_QUOTATION;
    result->data.quotationValue = quotation;
    result->quotationSize = size;
    return 0;
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

void hex_print_element(FILE *stream, HEX_StackElement element)
{
    switch (element.type)
    {
    case HEX_TYPE_INTEGER:
        fprintf(stream, "0x%x", element.data.intValue);
        break;
    case HEX_TYPE_STRING:
        fprintf(stream, "\"%s\"", element.data.strValue);
        break;
    case HEX_TYPE_USER_SYMBOL:
        fprintf(stream, "%s", element.symbolName);
        break;
    case HEX_TYPE_NATIVE_SYMBOL:
        fprintf(stream, "%s", element.symbolName);
        break;
    case HEX_TYPE_QUOTATION:
    {
        fprintf(stream, "(");
        for (size_t i = 0; i < element.quotationSize; i++)
        {
            if (i > 0)
                fprintf(stream, " "); // Add space between elements
            hex_print_element(stream, *element.data.quotationValue[i]);
        }
        fprintf(stream, ")");
        break;
    }
    default:
    }
}

int hex_is_symbol(HEX_Token *token, char *value)
{
    return strcmp(token->value, value) == 0;
}

////////////////////////////////////////
// Native Symbol Implementations      //
////////////////////////////////////////

// Definition symbols

int hex_symbol_store()
{
    HEX_StackElement name = hex_pop();
    if (name.type == HEX_TYPE_INVALID)
    {
        hex_free_element(name);
        return 1;
    }
    int result = 0;
    HEX_StackElement value = hex_pop();
    if (value.type == HEX_TYPE_INVALID)
    {
        result = 1;
    }
    else if (name.type != HEX_TYPE_STRING)
    {
        hex_error("Symbol name must be a string");
        result = 1;
    }
    else if (hex_set_symbol(name.data.strValue, value, 0) != 0)
    {
        hex_error("Failed to store variable");
        result = 1;
    }
    hex_free_element(name);
    hex_free_element(value);
    return result;
}

int hex_symbol_free()
{
    HEX_StackElement element = hex_pop();
    if (element.type == HEX_TYPE_INVALID)
    {
        hex_free_element(element);
        return 1;
    }
    if (element.type != HEX_TYPE_STRING)
    {
        hex_free_element(element);
        hex_error("Variable name must be a string");
        return 1;
    }
    for (int i = 0; i < hex_dictCount; i++)
    {
        if (strcmp(hex_registry[i].key, element.data.strValue) == 0)
        {
            free(hex_registry[i].key);
            hex_free_element(hex_registry[i].value);
            for (int j = i; j < hex_dictCount - 1; j++)
            {
                hex_registry[j] = hex_registry[j + 1];
            }
            hex_dictCount--;
            hex_free_element(element);
            return 0;
        }
    }
    hex_free_element(element);
    return 0;
}

int hex_symbol_type()
{
    HEX_StackElement element = hex_pop();
    if (element.type == HEX_TYPE_INVALID)
    {
        hex_free_element(element);
        return 1;
    }
    int result = 0;
    result = hex_push_string(hex_type(element.type));
    hex_free_element(element);
    return result;
}

// Evaluation symbols

int hex_symbol_i()
{
    HEX_StackElement element = hex_pop();
    if (element.type == HEX_TYPE_INVALID)
    {
        hex_free_element(element);
        return 1;
    }
    int result = 0;
    if (element.type != HEX_TYPE_QUOTATION)
    {
        hex_error("'i' symbol requires a quotation");
        result = 1;
    }
    for (int i = 0; i < element.quotationSize; i++)
    {
        if (hex_push(*element.data.quotationValue[i]) != 0)
        {
            result = 1;
            break;
        }
    }
    hex_free_element(element);
    return result;
}

int hex_interpret(const char *code);

// evaluate a string
int hex_symbol_eval()
{
    HEX_StackElement element = hex_pop();
    if (element.type == HEX_TYPE_INVALID)
    {
        return 1;
    }
    int result = 0;
    if (element.type != HEX_TYPE_STRING)
    {
        hex_error("'eval' symbol requires a string");
        result = 1;
    }
    result = hex_interpret(element.data.strValue);
    hex_free_element(element);
    return result;
}

// IO Symbols

int hex_symbol_puts()
{
    HEX_StackElement element = hex_pop();
    if (element.type == HEX_TYPE_INVALID)
    {
        hex_free_element(element);
        return 1;
    }
    hex_print_element(stdout, element);
    printf("\n");
    return 0;
}

int hex_symbol_warn()
{
    HEX_StackElement element = hex_pop();
    if (element.type == HEX_TYPE_INVALID)
    {
        hex_free_element(element);
        return 1;
    }
    hex_print_element(stderr, element);
    printf("\n");
    hex_free_element(element);
    return 0;
}

int hex_symbol_print()
{
    HEX_StackElement element = hex_pop();
    if (element.type == HEX_TYPE_INVALID)
    {
        hex_free_element(element);
        return 1;
    }
    hex_print_element(stdout, element);
    hex_free_element(element);
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
    HEX_StackElement b = hex_pop();
    if (b.type == HEX_TYPE_INVALID)
    {
        hex_free_element(b);
        return 1;
    }
    HEX_StackElement a = hex_pop();
    if (a.type == HEX_TYPE_INVALID)
    {
        hex_free_element(a);
        hex_free_element(b);
        return 1;
    }
    int result = 0;
    if (a.type == HEX_TYPE_INTEGER && b.type == HEX_TYPE_INTEGER)
    {
        result = hex_push_int(a.data.intValue + b.data.intValue);
    }
    else
    {
        hex_error("'+' symbol requires two integers");
        result = 1;
    }
    hex_free_element(a);
    hex_free_element(b);
    return result;
}

int hex_symbol_subtract()
{
    HEX_StackElement b = hex_pop();
    if (b.type == HEX_TYPE_INVALID)
    {
        hex_free_element(b);
        return 1;
    }
    HEX_StackElement a = hex_pop();
    if (a.type == HEX_TYPE_INVALID)
    {
        hex_free_element(a);
        hex_free_element(b);
        return 1;
    }
    int result = 0;
    if (a.type == HEX_TYPE_INTEGER && b.type == HEX_TYPE_INTEGER)
    {
        result = hex_push_int(a.data.intValue - b.data.intValue);
    }
    else
    {
        hex_error("'-' symbol requires two integers");
        result = 1;
    }
    hex_free_element(a);
    hex_free_element(b);
    return result;
}

int hex_symbol_multiply()
{
    HEX_StackElement b = hex_pop();
    if (b.type == HEX_TYPE_INVALID)
    {
        hex_free_element(b);
        return 1;
    }
    HEX_StackElement a = hex_pop();
    if (a.type == HEX_TYPE_INVALID)
    {
        hex_free_element(a);
        hex_free_element(b);
        return 1;
    }
    int result = 0;
    if (a.type == HEX_TYPE_INTEGER && b.type == HEX_TYPE_INTEGER)
    {
        result = hex_push_int(a.data.intValue * b.data.intValue);
    }
    else
    {
        hex_error("'*' symbol requires two integers");
    }
    hex_free_element(a);
    hex_free_element(b);
    return result;
}

int hex_symbol_divide()
{
    HEX_StackElement b = hex_pop();
    if (b.type == HEX_TYPE_INVALID)
    {
        hex_free_element(b);
        return 1;
    }
    HEX_StackElement a = hex_pop();
    if (a.type == HEX_TYPE_INVALID)
    {
        hex_free_element(a);
        hex_free_element(b);
        return 1;
    }
    int result = 0;
    if (a.type == HEX_TYPE_INTEGER && b.type == HEX_TYPE_INTEGER)
    {
        if (b.data.intValue == 0)
        {
            hex_error("Division by zero");
        }
        result = hex_push_int(a.data.intValue / b.data.intValue);
    }
    else
    {
        hex_error("'/' symbol requires two integers");
        result = 1;
    }
    hex_free_element(a);
    hex_free_element(b);
    return result;
}

int hex_symbol_modulo()
{
    HEX_StackElement b = hex_pop();
    if (b.type == HEX_TYPE_INVALID)
    {
        hex_free_element(b);
        return 1;
    }
    HEX_StackElement a = hex_pop();
    if (a.type == HEX_TYPE_INVALID)
    {
        hex_free_element(a);
        hex_free_element(b);
        return 1;
    }
    int result = 0;
    if (a.type == HEX_TYPE_INTEGER && b.type == HEX_TYPE_INTEGER)
    {
        if (b.data.intValue == 0)
        {
            hex_error("Division by zero");
        }
        result = hex_push_int(a.data.intValue % b.data.intValue);
    }
    else
    {
        hex_error("'%%' symbol requires two integers");
        result = 1;
    }
    hex_free_element(a);
    hex_free_element(b);
    return result;
}

// Bit symbols

int hex_symbol_bitand()
{
    HEX_StackElement right = hex_pop();
    if (right.type == HEX_TYPE_INVALID)
    {
        hex_free_element(right);
        return 1;
    }
    HEX_StackElement left = hex_pop();
    if (left.type == HEX_TYPE_INVALID)
    {
        hex_free_element(left);
        hex_free_element(right);
        return 1;
    }
    int result = 0;
    if (left.type == HEX_TYPE_INTEGER && right.type == HEX_TYPE_INTEGER)
    {
        int value = left.data.intValue & right.data.intValue;
        result = hex_push_int(value);
    }
    else
    {
        hex_error("Bitwise AND requires two integers");
        result = 1;
    }
    hex_free_element(left);
    hex_free_element(right);
    return result;
}

int hex_symbol_bitor()
{
    HEX_StackElement right = hex_pop();
    if (right.type == HEX_TYPE_INVALID)
    {
        hex_free_element(right);
        return 1;
    }
    HEX_StackElement left = hex_pop();
    if (left.type == HEX_TYPE_INVALID)
    {
        hex_free_element(left);
        hex_free_element(right);
        return 1;
    }
    int result = 0;
    if (left.type == HEX_TYPE_INTEGER && right.type == HEX_TYPE_INTEGER)
    {
        int value = left.data.intValue | right.data.intValue;
        result = hex_push_int(value);
    }
    else
    {
        hex_error("Bitwise OR requires two integers");
        result = 1;
    }
    hex_free_element(left);
    hex_free_element(right);
    return result;
}

int hex_symbol_bitxor()
{
    HEX_StackElement right = hex_pop();
    if (right.type == HEX_TYPE_INVALID)
    {
        hex_free_element(right);
        return 1;
    }
    HEX_StackElement left = hex_pop();
    if (left.type == HEX_TYPE_INVALID)
    {
        hex_free_element(left);
        hex_free_element(right);
        return 1;
    }
    int result = 0;
    if (left.type == HEX_TYPE_INTEGER && right.type == HEX_TYPE_INTEGER)
    {
        int value = left.data.intValue ^ right.data.intValue;
        result = hex_push_int(value);
    }
    else
    {
        hex_error("Bitwise XOR requires two integers");
        result = 1;
    }
    hex_free_element(left);
    hex_free_element(right);
    return result;
}

int hex_symbol_shiftleft()
{
    HEX_StackElement right = hex_pop();
    if (right.type == HEX_TYPE_INVALID)
    {
        hex_free_element(right);
        return 1;
    }
    HEX_StackElement left = hex_pop();
    if (left.type == HEX_TYPE_INVALID)
    {
        hex_free_element(left);
        hex_free_element(right);
        return 1;
    }
    int result = 0;
    if (left.type == HEX_TYPE_INTEGER && right.type == HEX_TYPE_INTEGER)
    {
        int value = left.data.intValue << right.data.intValue;
        result = hex_push_int(value);
    }
    else
    {
        hex_error("Left shift requires two integers");
        result = 1;
    }
    hex_free_element(left);
    hex_free_element(right);
    return result;
}

int hex_symbol_shiftright()
{
    HEX_StackElement right = hex_pop();
    if (right.type == HEX_TYPE_INVALID)
    {
        hex_free_element(right);
        return 1;
    }
    HEX_StackElement left = hex_pop();
    if (left.type == HEX_TYPE_INVALID)
    {
        hex_free_element(left);
        hex_free_element(right);
        return 1;
    }
    int result = 0;
    if (left.type == HEX_TYPE_INTEGER && right.type == HEX_TYPE_INTEGER)
    {
        int value = left.data.intValue >> right.data.intValue;
        result = hex_push_int(value);
    }
    else
    {
        hex_error("Right shift requires two integers");
        result = 1;
    }
    hex_free_element(left);
    hex_free_element(right);
    return result;
}

int hex_symbol_bitnot()
{
    HEX_StackElement element = hex_pop();
    if (element.type == HEX_TYPE_INVALID)
    {
        hex_free_element(element);
        return 1;
    }
    int result = 0;
    if (element.type == HEX_TYPE_INTEGER)
    {
        int value = ~element.data.intValue;
        result = hex_push_int(value);
    }
    else
    {
        hex_error("Bitwise NOT requires one integer");
        result = 1;
    }
    hex_free_element(element);
    return result;
}

// Conversion symbols

int hex_symbol_int()
{
    HEX_StackElement a = hex_pop();
    if (a.type == HEX_TYPE_INVALID)
    {
        hex_free_element(a);
        return 1;
    }
    int result = 0;
    if (a.type == HEX_TYPE_QUOTATION)
    {
        hex_error("Cannot convert a quotation to an integer");
        result = 1;
    }
    else if (a.type == HEX_TYPE_INTEGER)
    {
        result = hex_push_int(a.data.intValue);
    }
    else if (a.type == HEX_TYPE_STRING)
    {
        result = hex_push_int(strtol(a.data.strValue, NULL, 16));
    }
    hex_free_element(a);
    return result;
}

int hex_symbol_str()
{
    HEX_StackElement a = hex_pop();
    if (a.type == HEX_TYPE_INVALID)
    {
        hex_free_element(a);
        return 1;
    }
    int result = 0;
    if (a.type == HEX_TYPE_QUOTATION)
    {
        hex_error("Cannot convert a quotation to a string");
        result = 1;
    }
    else if (a.type == HEX_TYPE_INTEGER)
    {
        result = hex_push_string(hex_itoa_hex(a.data.intValue));
    }
    else if (a.type == HEX_TYPE_STRING)
    {
        result = hex_push_string(a.data.strValue);
    }
    hex_free_element(a);
    return result;
}

int hex_symbol_dec()
{
    HEX_StackElement a = hex_pop();
    if (a.type == HEX_TYPE_INVALID)
    {
        hex_free_element(a);
        return 1;
    }
    int result = 0;
    if (a.type == HEX_TYPE_INTEGER)
    {
        result = hex_push_string(hex_itoa_dec(a.data.intValue));
    }
    else
    {
        hex_error("An integer is required");
        result = 1;
    }
    hex_free_element(a);
    return result;
}

int hex_symbol_hex()
{
    HEX_StackElement element = hex_pop();
    if (element.type == HEX_TYPE_INVALID)
    {
        hex_free_element(element);
        return 1;
    }
    int result = 0;
    if (element.type == HEX_TYPE_STRING)
    {
        int value = strtol(element.data.strValue, NULL, 10);
        result = hex_push_int(value);
    }
    else
    {
        hex_error("'hex' symbol requires a string representing a decimal integer");
        result = 1;
    }
    hex_free_element(element);
    return result;
}

// Comparison symbols

int hex_equal(HEX_StackElement a, HEX_StackElement b)
{
    int result = 0;
    if (a.type == HEX_TYPE_INTEGER && b.type == HEX_TYPE_INTEGER)
    {
        result = a.data.intValue == b.data.intValue;
    }
    else if (a.type == HEX_TYPE_STRING && b.type == HEX_TYPE_STRING)
    {
        result = (strcmp(a.data.strValue, b.data.strValue) == 0);
    }
    else if (a.type == HEX_TYPE_QUOTATION && b.type == HEX_TYPE_QUOTATION)
    {
        if (a.quotationSize != b.quotationSize)
        {
            result = 0;
        }
        else
        {
            result = 1;
            for (size_t i = 0; i < a.quotationSize; i++)
            {
                if (a.data.quotationValue[i] != b.data.quotationValue[i])
                {
                    result = 0;
                    break;
                }
            }
        }
    }
    return result;
}

int hex_symbol_equal()
{
    HEX_StackElement b = hex_pop();
    if (b.type == HEX_TYPE_INVALID)
    {
        hex_free_element(b);
        return 1;
    }
    HEX_StackElement a = hex_pop();
    if (a.type == HEX_TYPE_INVALID)
    {
        hex_free_element(a);
        hex_free_element(b);
        return 1;
    }
    int result = 0;
    if ((a.type == HEX_TYPE_INTEGER && b.type == HEX_TYPE_INTEGER) || (a.type == HEX_TYPE_STRING && b.type == HEX_TYPE_STRING) || (a.type == HEX_TYPE_QUOTATION && b.type == HEX_TYPE_QUOTATION))
    {
        result = hex_push_int(hex_equal(a, b));
    }
    else
    {
        hex_error("'==' symbol requires two integers, two strings, or two quotations");
        result = 1;
    }
    hex_free_element(a);
    hex_free_element(b);
    return result;
}

int hex_symbol_notequal()
{
    HEX_StackElement b = hex_pop();
    if (b.type == HEX_TYPE_INVALID)
    {
        hex_free_element(b);
        return 1;
    }
    HEX_StackElement a = hex_pop();
    if (a.type == HEX_TYPE_INVALID)
    {
        hex_free_element(a);
        hex_free_element(b);
        return 1;
    }
    int result = 0;
    if ((a.type == HEX_TYPE_INTEGER && b.type == HEX_TYPE_INTEGER) || (a.type == HEX_TYPE_STRING && b.type == HEX_TYPE_STRING) || (a.type == HEX_TYPE_QUOTATION && b.type == HEX_TYPE_QUOTATION))
    {
        result = hex_push_int(!hex_equal(a, b));
    }
    else
    {
        hex_error("'!=' symbol requires two integers, two strings, or two quotations");
        result = 1;
    }
    hex_free_element(a);
    hex_free_element(b);
    return result;
}

int hex_symbol_greater()
{
    HEX_StackElement b = hex_pop();
    if (b.type == HEX_TYPE_INVALID)
    {
        hex_free_element(b);
        return 1;
    }
    HEX_StackElement a = hex_pop();
    if (a.type == HEX_TYPE_INVALID)
    {
        hex_free_element(a);
        hex_free_element(b);
        return 1;
    }
    int result = 0;
    if (a.type == HEX_TYPE_INTEGER && b.type == HEX_TYPE_INTEGER)
    {
        result = hex_push_int(a.data.intValue > b.data.intValue);
    }
    else if (a.type == HEX_TYPE_STRING && b.type == HEX_TYPE_STRING)
    {
        result = hex_push_int(strcmp(a.data.strValue, b.data.strValue) > 0);
    }
    else
    {
        hex_error("'>' symbol requires two integers or two strings");
        result = 1;
    }
    hex_free_element(a);
    hex_free_element(b);
    return result;
}

int hex_symbol_less()
{
    HEX_StackElement b = hex_pop();
    if (b.type == HEX_TYPE_INVALID)
    {
        hex_free_element(b);
        return 1;
    }
    HEX_StackElement a = hex_pop();
    if (a.type == HEX_TYPE_INVALID)
    {
        hex_free_element(a);
        hex_free_element(b);
        return 1;
    }
    int result = 0;
    if (a.type == HEX_TYPE_INTEGER && b.type == HEX_TYPE_INTEGER)
    {
        result = hex_push_int(a.data.intValue < b.data.intValue);
    }
    else if (a.type == HEX_TYPE_STRING && b.type == HEX_TYPE_STRING)
    {
        result = hex_push_int(strcmp(a.data.strValue, b.data.strValue) < 0);
    }
    else
    {
        hex_error("'<' symbol requires two integers or two strings");
        result = 1;
    }
    hex_free_element(a);
    hex_free_element(b);
    return result;
}

int hex_symbol_greaterequal()
{
    HEX_StackElement b = hex_pop();
    if (b.type == HEX_TYPE_INVALID)
    {
        hex_free_element(b);
        return 1;
    }
    HEX_StackElement a = hex_pop();
    if (a.type == HEX_TYPE_INVALID)
    {
        hex_free_element(a);
        hex_free_element(b);
        return 1;
    }
    int result = 0;
    if (a.type == HEX_TYPE_INTEGER && b.type == HEX_TYPE_INTEGER)
    {
        result = hex_push_int(a.data.intValue >= b.data.intValue);
    }
    else if (a.type == HEX_TYPE_STRING && b.type == HEX_TYPE_STRING)
    {
        result = hex_push_int(strcmp(a.data.strValue, b.data.strValue) >= 0);
    }
    else
    {
        hex_error("'>=' symbol requires two integers or two strings");
        result = 1;
    }
    hex_free_element(a);
    hex_free_element(b);
    return result;
}

int hex_symbol_lessequal()
{
    HEX_StackElement b = hex_pop();
    if (b.type == HEX_TYPE_INVALID)
    {
        hex_free_element(b);
        return 1;
    }
    HEX_StackElement a = hex_pop();
    if (a.type == HEX_TYPE_INVALID)
    {
        hex_free_element(a);
        hex_free_element(b);
        return 1;
    }
    int result = 0;
    if (a.type == HEX_TYPE_INTEGER && b.type == HEX_TYPE_INTEGER)
    {
        result = hex_push_int(a.data.intValue <= b.data.intValue);
    }
    else if (a.type == HEX_TYPE_STRING && b.type == HEX_TYPE_STRING)
    {
        result = hex_push_int(strcmp(a.data.strValue, b.data.strValue) <= 0);
    }
    else
    {
        hex_error("'<=' symbol requires two integers or two strings");
        result = 1;
    }
    hex_free_element(a);
    hex_free_element(b);
    return result;
}

// Boolean symbols

int hex_symbol_and()
{
    HEX_StackElement b = hex_pop();
    if (b.type == HEX_TYPE_INVALID)
    {
        hex_free_element(b);
        return 1;
    }
    HEX_StackElement a = hex_pop();
    if (a.type == HEX_TYPE_INVALID)
    {
        hex_free_element(a);
        hex_free_element(b);
        return 1;
    }
    int result = 0;
    if (a.type == HEX_TYPE_INTEGER && b.type == HEX_TYPE_INTEGER)
    {
        result = hex_push_int(a.data.intValue && b.data.intValue);
    }
    else
    {
        hex_error("'and' symbol requires two integers");
        result = 1;
    }
    hex_free_element(a);
    hex_free_element(b);
    return result;
}

int hex_symbol_or()
{
    HEX_StackElement b = hex_pop();
    if (b.type == HEX_TYPE_INVALID)
    {
        hex_free_element(b);
        return 1;
    }
    HEX_StackElement a = hex_pop();
    if (a.type == HEX_TYPE_INVALID)
    {
        hex_free_element(a);
        hex_free_element(b);
        return 1;
    }
    int result = 0;
    if (a.type == HEX_TYPE_INTEGER && b.type == HEX_TYPE_INTEGER)
    {
        result = hex_push_int(a.data.intValue || b.data.intValue);
    }
    else
    {
        hex_error("'or' symbol requires two integers");
        result = 1;
    }
    hex_free_element(a);
    hex_free_element(b);
    return result;
}

int hex_symbol_not()
{
    HEX_StackElement a = hex_pop();
    if (a.type == HEX_TYPE_INVALID)
    {
        hex_free_element(a);
        return 1;
    }
    int result = 0;
    if (a.type == HEX_TYPE_INTEGER)
    {
        result = hex_push_int(!a.data.intValue);
    }
    else
    {
        hex_error("'not' symbol requires an integer");
        result = 1;
    }
    hex_free_element(a);
    return result;
}

int hex_symbol_xor()
{
    HEX_StackElement b = hex_pop();
    if (b.type == HEX_TYPE_INVALID)
    {
        hex_free_element(b);
        return 1;
    }
    HEX_StackElement a = hex_pop();
    if (a.type == HEX_TYPE_INVALID)
    {
        hex_free_element(a);
        hex_free_element(b);
        return 1;
    }
    int result = 0;
    if (a.type == HEX_TYPE_INTEGER && b.type == HEX_TYPE_INTEGER)
    {
        result = hex_push_int(a.data.intValue ^ b.data.intValue);
    }
    else
    {
        hex_error("'xor' symbol requires two integers");
        result = 1;
    }
    hex_free_element(a);
    hex_free_element(b);
    return result;
}

////////////////////////////////////////
// Quotation/String symbols           //
////////////////////////////////////////

int hex_symbol_concat()
{
    HEX_StackElement value = hex_pop();
    if (value.type == HEX_TYPE_INVALID)
    {
        hex_free_element(value);
        return 1;
    }
    HEX_StackElement list = hex_pop();
    if (list.type == HEX_TYPE_INVALID)
    {
        hex_free_element(list);
        hex_free_element(value);
        return 1;
    }
    int result = 0;
    if (list.type == HEX_TYPE_QUOTATION)
    {
        size_t newSize = list.quotationSize + 1;
        HEX_StackElement **newQuotation = (HEX_StackElement **)realloc(list.data.quotationValue, newSize * sizeof(HEX_StackElement *));
        if (!newQuotation)
        {
            hex_error("Memory allocation failed");
            result = 1;
        }
        else
        {
            HEX_StackElement *element = (HEX_StackElement *)malloc(sizeof(HEX_StackElement));
            *element = value;
            newQuotation[newSize - 1] = element;
            list.data.quotationValue = newQuotation;
            list.quotationSize = newSize;
            result = hex_push_quotation(list.data.quotationValue, newSize);
        }
    }
    else if (list.type == HEX_TYPE_STRING)
    {
        char *newStr = (char *)malloc(strlen(list.data.strValue) + strlen(value.data.strValue) + 1);
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
        hex_error("Symbol 'append' requires a quotation or a string");
        result = 1;
    }
    return result;
}

int hex_symbol_slice()
{
    HEX_StackElement end = hex_pop();
    if (end.type == HEX_TYPE_INVALID)
    {
        hex_free_element(end);
        return 1;
    }
    HEX_StackElement start = hex_pop();
    if (start.type == HEX_TYPE_INVALID)
    {
        hex_free_element(start);
        hex_free_element(end);
        return 1;
    }
    HEX_StackElement list = hex_pop();
    if (list.type == HEX_TYPE_INVALID)
    {
        hex_free_element(list);
        hex_free_element(start);
        hex_free_element(end);
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
            size_t newSize = end.data.intValue - start.data.intValue + 1;
            HEX_StackElement **newQuotation = (HEX_StackElement **)malloc(newSize * sizeof(HEX_StackElement *));
            if (!newQuotation)
            {
                hex_error("Memory allocation failed");
                result = 1;
            }
            else
            {
                for (size_t i = 0; i < newSize; i++)
                {
                    newQuotation[i] = (HEX_StackElement *)malloc(sizeof(HEX_StackElement));
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
        else if (start.data.intValue < 0 || start.data.intValue >= strlen(list.data.strValue) || end.data.intValue < 0 || end.data.intValue >= strlen(list.data.strValue))
        {
            hex_error("Slice indices out of range");
            result = 1;
        }
        else
        {
            size_t newSize = end.data.intValue - start.data.intValue + 1;
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
    hex_free_element(list);
    hex_free_element(start);
    hex_free_element(end);
    return result;
}

int hex_symbol_len()
{
    HEX_StackElement element = hex_pop();
    if (element.type == HEX_TYPE_INVALID)
    {
        hex_free_element(element);
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
    hex_free_element(element);
    return result;
}

int hex_symbol_get()
{
    HEX_StackElement index = hex_pop();
    if (index.type == HEX_TYPE_INVALID)
    {
        hex_free_element(index);
        return 1;
    }
    HEX_StackElement list = hex_pop();
    if (list.type == HEX_TYPE_INVALID)
    {
        hex_free_element(list);
        hex_free_element(index);
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
        else if (index.data.intValue < 0 || index.data.intValue >= strlen(list.data.strValue))
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
    hex_free_element(list);
    hex_free_element(index);
    return result;
}

int hex_symbol_set()
{
    HEX_StackElement value = hex_pop();
    if (value.type == HEX_TYPE_INVALID)
    {
        hex_free_element(value);
        return 1;
    }
    HEX_StackElement index = hex_pop();
    if (index.type == HEX_TYPE_INVALID)
    {
        hex_free_element(index);
        hex_free_element(value);
        return 1;
    }
    HEX_StackElement list = hex_pop();
    if (list.type == HEX_TYPE_INVALID)
    {
        hex_free_element(list);
        hex_free_element(index);
        hex_free_element(value);
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
            hex_free_element(*list.data.quotationValue[index.data.intValue]);
            *list.data.quotationValue[index.data.intValue] = value;
            result = 0;
        }
    }
    else if (list.type == HEX_TYPE_STRING)
    {
        if (index.type != HEX_TYPE_INTEGER)
        {
            hex_error("Index must be an integer");
            result = 1;
        }
        else if (index.data.intValue < 0 || index.data.intValue >= strlen(list.data.strValue))
        {
            hex_error("Index out of range");
            result = 1;
        }
        else
        {
            list.data.strValue[index.data.intValue] = value.data.strValue[0];
            result = 0;
        }
    }
    else
    {
        hex_error("Symbol 'set' requires a quotation or a string");
        result = 1;
    }
    hex_free_element(list);
    hex_free_element(index);
    hex_free_element(value);
    return result;
}

int hex_symbol_index()
{
    HEX_StackElement element = hex_pop();
    if (element.type == HEX_TYPE_INVALID)
    {
        hex_free_element(element);
        return 1;
    }
    HEX_StackElement list = hex_pop();
    if (list.type == HEX_TYPE_INVALID)
    {
        hex_free_element(list);
        hex_free_element(element);
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
    }
    hex_free_element(element);
    hex_free_element(list);
    return hex_push_int(result);
}

////////////////////////////////////////
// String symbols                     //
////////////////////////////////////////

int hex_symbol_split()
{
    HEX_StackElement separator = hex_pop();
    if (separator.type == HEX_TYPE_INVALID)
    {
        hex_free_element(separator);
        return 1;
    }
    HEX_StackElement list = hex_pop();
    if (list.type == HEX_TYPE_INVALID)
    {
        hex_free_element(list);
        hex_free_element(separator);
        return 1;
    }
    int result = 0;
    if (list.type == HEX_TYPE_STRING && separator.type == HEX_TYPE_STRING)
    {
        char *str = list.data.strValue;
        char *sep = separator.data.strValue;
        char *token = strtok(str, sep);
        while (token)
        {
            result = hex_push_string(token);
            if (result != 0)
            {
                break;
            }
            token = strtok(NULL, sep);
        }
    }
    else
    {
        hex_error("Symbol 'split' requires two strings");
        result = 1;
    }
    hex_free_element(list);
    hex_free_element(separator);
    return result;
}

int hex_symbol_replace()
{
    HEX_StackElement replacement = hex_pop();
    if (replacement.type == HEX_TYPE_INVALID)
    {
        hex_free_element(replacement);
        return 1;
    }
    HEX_StackElement search = hex_pop();
    if (search.type == HEX_TYPE_INVALID)
    {
        hex_free_element(search);
        hex_free_element(replacement);
        return 1;
    }
    HEX_StackElement list = hex_pop();
    if (list.type == HEX_TYPE_INVALID)
    {
        hex_free_element(list);
        hex_free_element(search);
        hex_free_element(replacement);
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
            size_t findLen = strlen(find);
            size_t replaceLen = strlen(replace);
            size_t newLen = strlen(str) - findLen + replaceLen + 1;
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
    hex_free_element(list);
    hex_free_element(search);
    hex_free_element(replacement);
    return result;
}

////////////////////////////////////////
// File symbols                       //
////////////////////////////////////////

int hex_symbol_read()
{
    HEX_StackElement filename = hex_pop();
    if (filename.type == HEX_TYPE_INVALID)
    {
        hex_free_element(filename);
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
    hex_free_element(filename);
    return result;
}

int hex_symbol_write()
{
    HEX_StackElement filename = hex_pop();
    if (filename.type == HEX_TYPE_INVALID)
    {
        hex_free_element(filename);
        return 1;
    }
    HEX_StackElement data = hex_pop();
    if (data.type == HEX_TYPE_INVALID)
    {
        hex_free_element(data);
        hex_free_element(filename);
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
    hex_free_element(data);
    hex_free_element(filename);
    return result;
}

int hex_symbol_append()
{
    HEX_StackElement filename = hex_pop();
    if (filename.type == HEX_TYPE_INVALID)
    {
        hex_free_element(filename);
        return 1;
    }
    HEX_StackElement data = hex_pop();
    if (data.type == HEX_TYPE_INVALID)
    {
        hex_free_element(data);
        hex_free_element(filename);
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
    hex_free_element(data);
    hex_free_element(filename);
    return result;
}

////////////////////////////////////////
// Shell symbols                      //
////////////////////////////////////////

int hex_symbol_args()
{
    int result = 0;
    if (HEX_ARGV)
    {
        HEX_StackElement **quotation = (HEX_StackElement **)malloc(HEX_ARGC * sizeof(HEX_StackElement *));
        if (!quotation)
        {
            hex_error("Memory allocation failed");
            result = 1;
        }
        else
        {
            for (int i = 0; i < HEX_ARGC; i++)
            {
                quotation[i] = (HEX_StackElement *)malloc(sizeof(HEX_StackElement));
                quotation[i]->type = HEX_TYPE_STRING;
                quotation[i]->data.strValue = HEX_ARGV[i];
            }
            result = hex_push_quotation(quotation, HEX_ARGC);
        }
    }
    else
    {
        result = 1;
    }
    return result;
}

int hex_symbol_exit()
{
    HEX_KEEP_RUNNING = 0;
    return 0;
}

int hex_symbol_exec()
{
    HEX_StackElement command = hex_pop();
    if (command.type == HEX_TYPE_INVALID)
    {
        hex_free_element(command);
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
    hex_free_element(command);
    return result;
}

int hex_symbol_run()
{
    HEX_StackElement command = hex_pop();
    if (command.type == HEX_TYPE_INVALID)
    {
        hex_free_element(command);
        return 1;
    }
    if (command.type != HEX_TYPE_STRING)
    {
        hex_error("Symbol 'run' requires a string");
        hex_free_element(command);
        return 1;
    }

    FILE *fp;
    char path[1035];
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
        hex_free_element(command);
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
        hex_free_element(command);
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
        hex_free_element(command);
        return 1;
    }

    pid_t pid = fork();
    if (pid == -1)
    {
        hex_error("Failed to fork process");
        hex_free_element(command);
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
    HEX_StackElement **quotation = (HEX_StackElement **)malloc(3 * sizeof(HEX_StackElement *));
    quotation[0] = (HEX_StackElement *)malloc(sizeof(HEX_StackElement));
    quotation[0]->type = HEX_TYPE_INTEGER;
    quotation[0]->data.intValue = return_code;

    quotation[1] = (HEX_StackElement *)malloc(sizeof(HEX_StackElement));
    quotation[1]->type = HEX_TYPE_STRING;
    quotation[1]->data.strValue = strdup(output);

    quotation[2] = (HEX_StackElement *)malloc(sizeof(HEX_StackElement));
    quotation[2]->type = HEX_TYPE_STRING;
    quotation[2]->data.strValue = strdup(error);

    hex_free_element(command);
    return hex_push_quotation(quotation, 3);
}

////////////////////////////////////////
// Control flow symbols               //
////////////////////////////////////////

int hex_symbol_when()
{

    HEX_StackElement action = hex_pop();
    if (action.type == HEX_TYPE_INVALID)
    {
        hex_free_element(action);
        return 1;
    }
    HEX_StackElement condition = hex_pop();
    if (condition.type == HEX_TYPE_INVALID)
    {
        hex_free_element(action);
        hex_free_element(condition);
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
        for (size_t i = 0; i < condition.quotationSize; i++)
        {
            if (hex_push(*condition.data.quotationValue[i]) != 0)
            {
                result = 1;
                break; // Break if pushing the element failed
            }
        }
        HEX_StackElement evalResult = hex_pop();
        if (evalResult.type == HEX_TYPE_INTEGER && evalResult.data.intValue > 0)
        {
            for (size_t i = 0; i < action.quotationSize; i++)
            {
                if (hex_push(*action.data.quotationValue[i]) != 0)
                {
                    result = 1;
                    break;
                }
            }
        }
        hex_free_element(evalResult);
    }
    hex_free_element(condition);
    hex_free_element(action);
    return result;
}

int hex_symbol_unless()
{
    HEX_StackElement action = hex_pop();
    if (action.type == HEX_TYPE_INVALID)
    {
        hex_free_element(action);
        return 1;
    }
    HEX_StackElement condition = hex_pop();
    if (condition.type == HEX_TYPE_INVALID)
    {
        hex_free_element(action);
        hex_free_element(condition);
        return 1;
    }
    int result = 0;
    if (condition.type != HEX_TYPE_QUOTATION || action.type != HEX_TYPE_QUOTATION)
    {
        hex_error("'unless' symbol requires two quotations");
        result = 1;
    }
    else
    {
        for (size_t i = 0; i < condition.quotationSize; i++)
        {
            if (hex_push(*condition.data.quotationValue[i]) != 0)
            {
                result = 1;
                break; // Break if pushing the element failed
            }
        }
        HEX_StackElement evalResult = hex_pop();
        if (evalResult.type == HEX_TYPE_INTEGER && evalResult.data.intValue == 0)
        {
            for (size_t i = 0; i < action.quotationSize; i++)
            {
                if (hex_push(*action.data.quotationValue[i]) != 0)
                {
                    result = 1;
                    break;
                }
            }
        }
        hex_free_element(evalResult);
    }
    hex_free_element(condition);
    hex_free_element(action);
    return result;
}

int hex_symbol_while()
{
    HEX_StackElement action = hex_pop();
    if (action.type == HEX_TYPE_INVALID)
    {
        hex_free_element(action);
        return 1;
    }
    HEX_StackElement condition = hex_pop();
    if (condition.type == HEX_TYPE_INVALID)
    {
        hex_free_element(action);
        hex_free_element(condition);
        return 1;
    }
    int result = 0;
    if (condition.type != HEX_TYPE_QUOTATION || action.type != HEX_TYPE_QUOTATION)
    {
        hex_error("'while' symbol requires two quotations");
        result = 1;
    }
    else
    {
        while (1)
        {
            for (size_t i = 0; i < condition.quotationSize; i++)
            {
                if (hex_push(*condition.data.quotationValue[i]) != 0)
                {
                    result = 1;
                    break; // Break if pushing the element failed
                }
            }
            HEX_StackElement evalResult = hex_pop();
            if (evalResult.type == HEX_TYPE_INTEGER && evalResult.data.intValue == 0)
            {
                break;
            }
            for (size_t i = 0; i < action.quotationSize; i++)
            {
                if (hex_push(*action.data.quotationValue[i]) != 0)
                {
                    result = 1;
                    break;
                }
            }
            hex_free_element(evalResult);
        }
    }
    hex_free_element(condition);
    hex_free_element(action);
    return result;
}

int hex_symbol_each()
{
    HEX_StackElement action = hex_pop();
    if (action.type == HEX_TYPE_INVALID)
    {
        hex_free_element(action);
        return 1;
    }
    HEX_StackElement list = hex_pop();
    if (list.type == HEX_TYPE_INVALID)
    {
        hex_free_element(action);
        hex_free_element(list);
        return 1;
    }
    int result = 0;
    if (list.type != HEX_TYPE_QUOTATION || action.type != HEX_TYPE_QUOTATION)
    {
        hex_error("'each' symbol requires two quotations");
        result = 1;
    }
    else
    {
        for (size_t i = 0; i < list.quotationSize; i++)
        {
            if (hex_push(*list.data.quotationValue[i]) != 0)
            {
                result = 1;
                break; // Break if pushing the element failed
            }
            for (size_t j = 0; j < action.quotationSize; j++)
            {
                if (hex_push(*action.data.quotationValue[j]) != 0)
                {
                    result = 1;
                    break;
                }
            }
        }
    }
    hex_free_element(list);
    hex_free_element(action);
    return result;
}

int hex_symbol_times()
{
    HEX_StackElement count = hex_pop();
    if (count.type == HEX_TYPE_INVALID)
    {
        hex_free_element(count);
        return 1;
    }
    HEX_StackElement action = hex_pop();
    if (action.type == HEX_TYPE_INVALID)
    {
        hex_free_element(action);
        hex_free_element(count);
        return 1;
    }
    int result = 0;
    if (count.type != HEX_TYPE_INTEGER || action.type != HEX_TYPE_QUOTATION)
    {
        hex_error("'times' symbol requires an integer and a quotation");
        result = 1;
    }
    else
    {
        for (int i = 0; i < count.data.intValue; i++)
        {
            for (size_t j = 0; j < action.quotationSize; j++)
            {
                if (hex_push(*action.data.quotationValue[j]) != 0)
                {
                    result = 1;
                    break;
                }
            }
        }
    }
    hex_free_element(count);
    hex_free_element(action);
    return result;
}

int hex_symbol_error()
{
    char *message = strdup(HEX_ERROR);
    HEX_ERROR[0] = '\0';
    return hex_push_string(message);
}

int hex_symbol_try()
{
    HEX_StackElement tryBlock = hex_pop();
    if (tryBlock.type == HEX_TYPE_INVALID)
    {
        hex_free_element(tryBlock);
        return 1;
    }
    HEX_StackElement catchBlock = hex_pop();
    if (catchBlock.type == HEX_TYPE_INVALID)
    {
        hex_free_element(catchBlock);
        hex_free_element(tryBlock);
        return 1;
    }
    int result = 0;
    if (tryBlock.type != HEX_TYPE_QUOTATION || catchBlock.type != HEX_TYPE_QUOTATION)
    {
        hex_error("'try' symbol requires two quotations");
        result = 1;
    }
    else
    {
        char prevError[256];
        strncpy(prevError, HEX_ERROR, sizeof(HEX_ERROR));
        HEX_ERROR[0] = '\0';

        HEX_ERRORS = 0;
        for (size_t i = 0; i < tryBlock.quotationSize; i++)
        {
            if (hex_push(*tryBlock.data.quotationValue[i]) != 0)
            {
                result = 1;
                break;
            }
        }
        HEX_ERRORS = 1;

        if (HEX_ERROR != "")
        {
            for (size_t i = 0; i < catchBlock.quotationSize; i++)
            {
                if (hex_push(*catchBlock.data.quotationValue[i]) != 0)
                {
                    result = 1;
                    break;
                }
            }
        }

        strncpy(HEX_ERROR, prevError, sizeof(HEX_ERROR));
    }
    hex_free_element(tryBlock);
    hex_free_element(catchBlock);
    return result;
}

////////////////////////////////////////
// List symbols                       //
////////////////////////////////////////

int hex_symbol_map()
{
    HEX_StackElement action = hex_pop();
    if (action.type == HEX_TYPE_INVALID)
    {
        hex_free_element(action);
        return 1;
    }
    HEX_StackElement list = hex_pop();
    if (list.type == HEX_TYPE_INVALID)
    {
        hex_free_element(action);
        hex_free_element(list);
        return 1;
    }
    int result = 0;
    if (list.type != HEX_TYPE_QUOTATION || action.type != HEX_TYPE_QUOTATION)
    {
        hex_error("'map' symbol requires two quotations");
        result = 1;
    }
    else
    {
        HEX_StackElement **quotation = (HEX_StackElement **)malloc(list.quotationSize * sizeof(HEX_StackElement *));
        if (!quotation)
        {
            hex_error("Memory allocation failed");
            result = 1;
        }
        else
        {
            for (size_t i = 0; i < list.quotationSize; i++)
            {
                if (hex_push(*list.data.quotationValue[i]) != 0)
                {
                    result = 1;
                    break;
                }
                for (size_t j = 0; j < action.quotationSize; j++)
                {
                    if (hex_push(*action.data.quotationValue[j]) != 0)
                    {
                        result = 1;
                        break;
                    }
                }
                quotation[i] = (HEX_StackElement *)malloc(sizeof(HEX_StackElement));
                *quotation[i] = hex_pop();
            }
            result = hex_push_quotation(quotation, list.quotationSize);
        }
    }
    hex_free_element(list);
    hex_free_element(action);
    return result;
}

int hex_symbol_filter()
{
    HEX_StackElement action = hex_pop();
    if (action.type == HEX_TYPE_INVALID)
    {
        hex_free_element(action);
        return 1;
    }
    HEX_StackElement list = hex_pop();
    if (list.type == HEX_TYPE_INVALID)
    {
        hex_free_element(action);
        hex_free_element(list);
        return 1;
    }
    int result = 0;
    if (list.type != HEX_TYPE_QUOTATION || action.type != HEX_TYPE_QUOTATION)
    {
        hex_error("'filter' symbol requires two quotations");
        result = 1;
    }
    else
    {
        HEX_StackElement **quotation = (HEX_StackElement **)malloc(list.quotationSize * sizeof(HEX_StackElement *));
        if (!quotation)
        {
            hex_error("Memory allocation failed");
            result = 1;
        }
        else
        {
            size_t count = 0;
            for (size_t i = 0; i < list.quotationSize; i++)
            {
                if (hex_push(*list.data.quotationValue[i]) != 0)
                {
                    result = 1;
                    break;
                }
                for (size_t j = 0; j < action.quotationSize; j++)
                {
                    if (hex_push(*action.data.quotationValue[j]) != 0)
                    {
                        result = 1;
                        break;
                    }
                }
                HEX_StackElement evalResult = hex_pop();
                if (evalResult.type == HEX_TYPE_INTEGER && evalResult.data.intValue > 0)
                {
                    quotation[count] = (HEX_StackElement *)malloc(sizeof(HEX_StackElement));
                    *quotation[count] = *list.data.quotationValue[i];
                    count++;
                }
                hex_free_element(evalResult);
            }
            result = hex_push_quotation(quotation, count);
        }
    }
    hex_free_element(list);
    hex_free_element(action);
    return result;
}

////////////////////////////////////////
// Stack manipulation symbols         //
////////////////////////////////////////

int hex_symbol_swap()
{
    HEX_StackElement a = hex_pop();
    if (a.type == HEX_TYPE_INVALID)
    {
        hex_free_element(a);
        return 1;
    }
    HEX_StackElement b = hex_pop();
    if (b.type == HEX_TYPE_INVALID)
    {
        hex_free_element(b);
        hex_free_element(a);
        return 1;
    }
    int result = hex_push(a);
    if (result == 0)
    {
        result = hex_push(b);
    }
    hex_free_element(a);
    hex_free_element(b);
    return result;
}

int hex_symbol_dup()
{
    HEX_StackElement element = hex_pop();
    if (element.type == HEX_TYPE_INVALID)
    {
        hex_free_element(element);
        return 1;
    }
    int result = hex_push(element);
    if (result == 0)
    {
        result = hex_push(element);
    }
    hex_free_element(element);
    return result;
}

int hex_symbol_stack()
{
    HEX_StackElement **quotation = (HEX_StackElement **)malloc((hex_top + 1) * sizeof(HEX_StackElement *));
    if (!quotation)
    {
        hex_error("Memory allocation failed");
        return 1;
    }

    for (int i = 0; i <= hex_top; i++)
    {
        quotation[i] = (HEX_StackElement *)malloc(sizeof(HEX_StackElement));
        if (!quotation[i])
        {
            hex_error("Memory allocation failed");
            return 1;
        }
        *quotation[i] = hex_stack[i];
    }

    return hex_push_quotation(quotation, hex_top + 1);
}

int hex_symbol_clear()
{
    while (hex_top >= 0)
    {
        hex_free_element(hex_stack[hex_top--]);
    }
    return 0;
}

int hex_symbol_pop()
{
    HEX_StackElement element = hex_pop();
    if (element.type == HEX_TYPE_INVALID)
    {
        hex_free_element(element);
        return 1;
    }
    hex_free_element(element);
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
    hex_set_native_symbol("concat", hex_symbol_concat);
    hex_set_native_symbol("slice", hex_symbol_slice);
    hex_set_native_symbol("len", hex_symbol_len);
    hex_set_native_symbol("get", hex_symbol_get);
    hex_set_native_symbol("set", hex_symbol_set);
    hex_set_native_symbol("index", hex_symbol_index);
    hex_set_native_symbol("split", hex_symbol_split);
    hex_set_native_symbol("replace", hex_symbol_replace);
    hex_set_native_symbol("read", hex_symbol_read);
    hex_set_native_symbol("write", hex_symbol_write);
    hex_set_native_symbol("append", hex_symbol_append);
    hex_set_native_symbol("args", hex_symbol_args);
    hex_set_native_symbol("exit", hex_symbol_exit);
    hex_set_native_symbol("exec", hex_symbol_exec);
    hex_set_native_symbol("run", hex_symbol_run);
    hex_set_native_symbol("when", hex_symbol_when);
    hex_set_native_symbol("unless", hex_symbol_unless);
    hex_set_native_symbol("while", hex_symbol_while);
    hex_set_native_symbol("each", hex_symbol_each);
    hex_set_native_symbol("times", hex_symbol_times);
    hex_set_native_symbol("error", hex_symbol_error);
    hex_set_native_symbol("try", hex_symbol_try);
    hex_set_native_symbol("map", hex_symbol_map);
    hex_set_native_symbol("filter", hex_symbol_filter);
    hex_set_native_symbol("swap", hex_symbol_swap);
    hex_set_native_symbol("dup", hex_symbol_dup);
    hex_set_native_symbol("stack", hex_symbol_stack);
    hex_set_native_symbol("clear", hex_symbol_clear);
}

////////////////////////////////////////
// Hex Interpreter Implementation     //
////////////////////////////////////////

int hex_interpret(const char *code)
{
    const char *input = code;
    HEX_Token *token = hex_next_token(&input);

    while (token != NULL && token->type != HEX_TOKEN_INVALID)
    {
        if (token->type == HEX_TOKEN_NUMBER)
        {
            if (hex_push_int((int)strtol(token->value, NULL, 16)) != 0)
            {
                hex_free_token(token);
                return 1;
            }
        }
        else if (token->type == HEX_TOKEN_STRING)
        {
            HEX_StackElement symbolValue;
            if (hex_get_symbol(token->value, &symbolValue))
            {
                if (hex_push(symbolValue) != 0)
                {
                    hex_free_token(token);
                    return 1;
                }
            }
            else
            {
                if (hex_push_string(token->value) != 0)
                {
                    hex_free_token(token);
                    return 1;
                }
            }
        }
        else if (token->type == HEX_TOKEN_SYMBOL)
        {
            if (hex_push_symbol(token->value) != 0)
            {
                hex_free_token(token);
                return 1;
            }
        }
        else if (token->type == HEX_TOKEN_QUOTATION_START)
        {
            HEX_StackElement *quotationElement =                       (HEX_StackElement *)malloc(sizeof(HEX_StackElement));    
            if (hex_parse_quotation(&input, quotationElement, 1) != 0) {
                hex_error("Failed to parse quotation");
                return 1;
            }
            HEX_StackElement **quotation = quotationElement->data.quotationValue;
            size_t quotationSize = quotationElement->quotationSize;
            if (hex_push_quotation(quotation, quotationSize) != 0)
            {
                hex_free_token(token);
                return 1;
            }
        }
        token = hex_next_token(&input);
    }
    hex_free_token(token);
}

// Read a file into a buffer
char *hex_read_file(const char *filename)
{
    FILE *file = fopen(filename, "r");
    if (!file)
    {
        hex_error("Could not open file: %s", filename);
        exit(1);
    }

    fseek(file, 0, SEEK_END);
    long length = ftell(file);
    fseek(file, 0, SEEK_SET);

    char *buffer = (char *)malloc(length + 1);
    if (!buffer)
    {
        hex_error("Memory allocation failed");
    }

    fread(buffer, 1, length, file);
    buffer[length] = '\0';
    fclose(file);
    return buffer;
}

// REPL implementation
void hex_repl()
{
    char line[1024];

    printf("hex Interactive REPL. Type 'exit' to quit or press Ctrl+C.\n");

    while (HEX_KEEP_RUNNING)
    {
        printf("> "); // Prompt
        if (fgets(line, sizeof(line), stdin) == NULL)
        {
            printf("\n"); // Handle EOF (Ctrl+D)
            break;
        }

        // Normalize line endings (remove trailing \r\n or \n)
        line[strcspn(line, "\r\n")] = '\0';

        // Exit command
        if (strcmp(line, "exit") == 0)
        {
            break;
        }

        // Tokenize and process the input
        hex_interpret(line);
        // Print the top element of the stack
        if (hex_top >= 0)
        {
            hex_print_element(stdout, hex_stack[hex_top]);
            printf("\n");
        }
    }
}

void hex_handle_sigint(int sig)
{
    (void)sig; // Suppress unused warning
    HEX_KEEP_RUNNING = 0;
    printf("\nExiting hex REPL. Goodbye!\n");
}

// Process piped input from stdin
void hex_process_stdin()
{
    char buffer[8192]; // Adjust buffer size as needed
    size_t bytesRead = fread(buffer, 1, sizeof(buffer) - 1, stdin);
    if (bytesRead == 0)
    {
        hex_error("Error: No input provided via stdin.");
        return;
    }

    buffer[bytesRead] = '\0'; // Null-terminate the input
    hex_interpret(buffer);
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
            // Set HEX_DEBUG to 1 if -d or --debug flag is provided
            if ((strcmp(argv[i], "-d") == 0 || strcmp(argv[i], "--debug") == 0))
            {
                HEX_DEBUG = 1;
                printf("*** Debug mode enabled ***\n");
            }
            else
            {
                // Process a file
                const char *filename = argv[i];
                char *fileContent = hex_read_file(filename);
                if (!fileContent)
                {
                    return 1;
                }

                hex_interpret(fileContent);
                free(fileContent); // Free the allocated memory
                return 0;
            }
        }
    }
    if (!isatty(fileno(stdin)))
    {
        // Process piped input from stdin
        hex_process_stdin();
    }
    else
    {
        // Start REPL
        hex_repl();
    }

    return 0;
}

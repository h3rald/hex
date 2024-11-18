#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <signal.h>
#ifdef _WIN32
#include <windows.h>
#include <io.h>
int hex_isatty(int fd)
{
    HANDLE h = (HANDLE)_get_osfhandle(fd);
    return (GetFileType(h) == FILE_TYPE_CHAR);
}
#else
#include <unistd.h>
#endif

// Size of STDIN buffer (gets symbol)
#define HEX_STDIN_BUFFER_SIZE 256
// Global Registry for Variables
#define HEX_REGISTRY_SIZE 1024
// Stack Definition
#define HEX_STACK_SIZE 100

int HEX_DEBUG = 0;

void hex_error(const char *format, ...)
{
    va_list args;
    va_start(args, format);
    vfprintf(stderr, format, args);
    fprintf(stderr, "\n");
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
    hex_debug_element("Pushing element", element);
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
        return "native symbol";
    case HEX_TYPE_USER_SYMBOL:
        return "user symbol";
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
        printf(" [%s]", hex_type(element.type));
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
    HEX_TOKEN_QUOTATION_END
} HEX_TokenType;

typedef struct
{
    HEX_TokenType type;
    char *value;
} HEX_Token;

// Process a token from the input
HEX_Token *hex_next_token(const char **input)
{
    const char *ptr = *input;

    // Skip whitespace and comments
    while (isspace(*ptr) || *ptr == ';')
    {
        if (*ptr == ';')
        {
            while (*ptr != '\0' && *ptr != '\n')
            {
                ptr++;
            }
        }
        ptr++;
    }

    if (*ptr == '\0')
    {
        return NULL; // End of input
    }

    HEX_Token *token = (HEX_Token *)malloc(sizeof(HEX_Token));
    token->value = NULL;

    if (*ptr == '"')
    {
        // String token
        ptr++;
        const char *start = ptr;
        size_t len = 0;

        while (*ptr != '\0')
        {
            if (*ptr == '\\' && *(ptr + 1) == '"')
            {
                ptr += 2;
                len++;
            }
            else if (*ptr == '"')
            {
                break;
            }
            else
            {
                ptr++;
                len++;
            }
        }

        if (*ptr != '"')
        {
            hex_error("Unterminated string");
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
        token->type = HEX_TOKEN_STRING;
    }
    else if (strncmp(ptr, "0x", 2) == 0 || strncmp(ptr, "0X", 2) == 0)
    {
        // Hexadecimal integer token
        const char *start = ptr;
        ptr += 2; // Skip the "0x" prefix
        while (isxdigit(*ptr))
        {
            ptr++;
        }
        size_t len = ptr - start;
        token->value = (char *)malloc(len + 1);
        strncpy(token->value, start, len);
        token->value[len] = '\0';
        token->type = HEX_TOKEN_NUMBER;
    }
    else if (*ptr == '(')
    {
        token->type = HEX_TOKEN_QUOTATION_START;
        ptr++;
    }
    else if (*ptr == ')')
    {
        token->type = HEX_TOKEN_QUOTATION_END;
        ptr++;
    }
    else
    {
        const char *start = ptr;
        while (*ptr != '\0' && !isspace(*ptr) && *ptr != ';' && *ptr != '(' && *ptr != ')' && *ptr != '"')
        {
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

// Recursive quotation parsing
HEX_StackElement **hex_parse_quotation(const char **input, size_t *size)
{
    HEX_StackElement **quotation = NULL;
    size_t capacity = 2;
    *size = 0;

    quotation = (HEX_StackElement **)malloc(capacity * sizeof(HEX_StackElement *));
    if (!quotation)
    {
        hex_error("Memory allocation failed");
    }

    HEX_Token *token;
    while ((token = hex_next_token(input)) != NULL)
    {
        if (token->type == HEX_TOKEN_QUOTATION_END)
        {
            hex_free_token(token);
            break;
        }

        if (*size >= capacity)
        {
            capacity *= 2;
            quotation = (HEX_StackElement **)realloc(quotation, capacity * sizeof(HEX_StackElement *));
            if (!quotation)
            {
                hex_error("Memory allocation failed");
            }
        }

        HEX_StackElement *element = (HEX_StackElement *)malloc(sizeof(HEX_StackElement));
        if (token->type == HEX_TOKEN_NUMBER)
        {
            element->type = HEX_TYPE_INTEGER;
            element->data.intValue = (int)strtol(token->value, NULL, 16);
        }
        else if (token->type == HEX_TOKEN_STRING)
        {
            element->type = HEX_TYPE_STRING;
            element->data.strValue = strdup(token->value);
        }
        // TODO: check if non-native symbols are handled correctly
        else if (token->type == HEX_TOKEN_SYMBOL)
        {
            element->type = HEX_TYPE_USER_SYMBOL;
            element->symbolName = strdup(token->value);
        }
        else if (token->type == HEX_TOKEN_QUOTATION_START)
        {
            element->type = HEX_TYPE_QUOTATION;
            element->data.quotationValue = hex_parse_quotation(input, &element->quotationSize);
        }
        else
        {
            hex_error("Unexpected token in quotation");
        }

        quotation[*size] = element;
        (*size)++;
        hex_free_token(token);
    }

    return quotation;
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
        fprintf(stream, element.symbolName);
        break;
    case HEX_TYPE_NATIVE_SYMBOL:
        fprintf(stream, element.symbolName);
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
    HEX_StackElement value = hex_pop();
    if (name.type != HEX_TYPE_STRING)
    {
        hex_error("Symbol name must be a string");
        return 1;
    }
    if (hex_set_symbol(name.data.strValue, value, 0))
    {
        hex_error("Failed to store variable");
        return 1;
    }
    hex_free_element(name);
    return 0;
}

int hex_symbol_free()
{
    HEX_StackElement element = hex_pop();
    if (element.type != HEX_TYPE_STRING)
    {
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
    int result = 0;
    result = hex_push_string(hex_type(element.type));
    hex_free_element(element);
    return result;
}

// Evaluation symbols

int hex_symbol_i()
{
    HEX_StackElement element = hex_pop();
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
    hex_print_element(stdout, element);
    printf("\n");
    hex_free_element(element);
    return 0;
}

int hex_symbol_warn()
{
    HEX_StackElement element = hex_pop();
    hex_print_element(stderr, element);
    printf("\n");
    hex_free_element(element);
    return 0;
}

int hex_symbol_print()
{
    HEX_StackElement element = hex_pop();
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
    HEX_StackElement a = hex_pop();
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
    HEX_StackElement a = hex_pop();
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
    HEX_StackElement a = hex_pop();
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
    HEX_StackElement a = hex_pop();
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

// Bit symbols

int hex_symbol_bitand()
{
    HEX_StackElement right = hex_pop(); // Pop the second operand
    HEX_StackElement left = hex_pop();  // Pop the first operand
    int result = 0;
    if (left.type == HEX_TYPE_INTEGER && right.type == HEX_TYPE_INTEGER)
    {
        int value = left.data.intValue & right.data.intValue; // Bitwise AND
        result = hex_push_int(value);                         // Push the result back onto the stack
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
    HEX_StackElement left = hex_pop();
    int result = 0;
    if (left.type == HEX_TYPE_INTEGER && right.type == HEX_TYPE_INTEGER)
    {
        int value = left.data.intValue | right.data.intValue; // Bitwise OR
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
    HEX_StackElement left = hex_pop();
    int result = 0;
    if (left.type == HEX_TYPE_INTEGER && right.type == HEX_TYPE_INTEGER)
    {
        int value = left.data.intValue ^ right.data.intValue; // Bitwise XOR
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
    HEX_StackElement right = hex_pop(); // The number of positions to shift
    HEX_StackElement left = hex_pop();  // The value to shift
    int result = 0;
    if (left.type == HEX_TYPE_INTEGER && right.type == HEX_TYPE_INTEGER)
    {
        int value = left.data.intValue << right.data.intValue; // Left shift
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
    HEX_StackElement right = hex_pop(); // The number of positions to shift
    HEX_StackElement left = hex_pop();  // The value to shift
    int result = 0;
    if (left.type == HEX_TYPE_INTEGER && right.type == HEX_TYPE_INTEGER)
    {
        int value = left.data.intValue >> right.data.intValue; // Right shift
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
    HEX_StackElement element = hex_pop(); // Only one operand for bitwise NOT
    int result = 0;
    if (element.type == HEX_TYPE_INTEGER)
    {
        int value = ~element.data.intValue; // Bitwise NOT (complement)
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

// Comparison symbols

int hex_symbol_equal()
{
    HEX_StackElement b = hex_pop();
    HEX_StackElement a = hex_pop();
    int result = 0;
    if (a.type == HEX_TYPE_INTEGER && b.type == HEX_TYPE_INTEGER)
    {
        result = hex_push_int(a.data.intValue == b.data.intValue);
    }
    else if (a.type == HEX_TYPE_STRING && b.type == HEX_TYPE_STRING)
    {
        result = hex_push_int(strcmp(a.data.strValue, b.data.strValue) == 0);
    }
    else
    {
        hex_error("'==' symbol requires two integers or two strings");
        result = 1;
    }
    hex_free_element(a);
    hex_free_element(b);
    return result;
}

int hex_symbol_notequal()
{
    HEX_StackElement b = hex_pop();
    HEX_StackElement a = hex_pop();
    int result = 0;
    if (a.type == HEX_TYPE_INTEGER && b.type == HEX_TYPE_INTEGER)
    {
        result = hex_push_int(a.data.intValue != b.data.intValue);
    }
    else if (a.type == HEX_TYPE_STRING && b.type == HEX_TYPE_STRING)
    {
        result = hex_push_int(strcmp(a.data.strValue, b.data.strValue) != 0);
    }
    else
    {
        hex_error("'!=' symbol requires two integers or two strings");
        result = 1;
    }
    hex_free_element(a);
    hex_free_element(b);
    return result;
}

int hex_symbol_greater()
{
    HEX_StackElement b = hex_pop();
    HEX_StackElement a = hex_pop();
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
    HEX_StackElement a = hex_pop();
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
    HEX_StackElement a = hex_pop();
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
    HEX_StackElement a = hex_pop();
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
    HEX_StackElement a = hex_pop();
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
    HEX_StackElement a = hex_pop();
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
    HEX_StackElement a = hex_pop();
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
// Quotation/String symbols                      //
////////////////////////////////////////

int hex_symbol_append()
{
    HEX_StackElement value = hex_pop();
    HEX_StackElement list = hex_pop();
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

int hex_symbol_prepend()
{
    HEX_StackElement value = hex_pop();
    HEX_StackElement list = hex_pop();
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
            for (size_t i = newSize - 1; i > 0; i--)
            {
                newQuotation[i] = newQuotation[i - 1];
            }
            HEX_StackElement *element = (HEX_StackElement *)malloc(sizeof(HEX_StackElement));
            *element = value;
            newQuotation[0] = element;
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
            strcpy(newStr, value.data.strValue);
            strcat(newStr, list.data.strValue);
            result = hex_push_string(newStr);
        }
    }
    else
    {
        hex_error("Symbol 'prepend' requires a quotation or a string");
        result = 1;
    }
    return result;
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
    hex_set_native_symbol("&", hex_symbol_bitand);
    hex_set_native_symbol("|", hex_symbol_bitor);
    hex_set_native_symbol("^", hex_symbol_bitxor);
    hex_set_native_symbol("~", hex_symbol_bitnot);
    hex_set_native_symbol("<<", hex_symbol_shiftleft);
    hex_set_native_symbol(">>", hex_symbol_shiftright);
    hex_set_native_symbol("int", hex_symbol_int);
    hex_set_native_symbol("str", hex_symbol_str);
    hex_set_native_symbol("dec", hex_symbol_dec);
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
    hex_set_native_symbol("append", hex_symbol_append);
    hex_set_native_symbol("prepend", hex_symbol_prepend);
}

////////////////////////////////////////
// Hex Interpreter Implementation     //
////////////////////////////////////////

int hex_interpret(const char *code)
{
    const char *input = code;
    HEX_Token *token;

    while ((token = hex_next_token(&input)) != NULL)
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
            size_t quotationSize;
            HEX_StackElement **quotation = hex_parse_quotation(&input, &quotationSize);
            if (hex_push_quotation(quotation, quotationSize) != 0)
            {
                hex_free_token(token);
                return 1;
            }
        }
        hex_free_token(token);
    }
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

volatile sig_atomic_t hex_keep_running = 1;

// REPL implementation
void hex_repl()
{
    char line[1024];

    printf("hex Interactive REPL. Type 'exit' to quit or press Ctrl+C.\n");

    while (hex_keep_running)
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
    hex_keep_running = 0;
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
    if (!hex_isatty(fileno(stdin)))
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

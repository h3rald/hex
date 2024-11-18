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

// Enum to represent the type of stack elements
typedef enum
{
    HEX_TYPE_INTEGER,
    HEX_TYPE_STRING,
    HEX_TYPE_QUOTATION,
    HEX_TYPE_FUNCTION
} HEX_ElementType;

// Unified Stack Element
typedef struct HEX_StackElement
{
    HEX_ElementType type;
    union
    {
        int intValue;
        char *strValue;
        void (*functionPointer)();
        struct HEX_StackElement **quotationValue;
    } data;
    size_t quotationSize; // Size of the quotation (valid for HEX_TYPE_QUOTATION)
} HEX_StackElement;

// Dictionary Entry
typedef struct
{
    char *key;
    HEX_StackElement value;
} HEX_DictionaryEntry;

// Size of STDIN buffer (gets operator)
#define HEX_STDIN_BUFFER_SIZE 256

// Global Dictionary for Variables
#define HEX_DICTIONARY_SIZE 1024
HEX_DictionaryEntry hex_dictionary[HEX_DICTIONARY_SIZE];
int hex_dictCount = 0;

void hex_free_element(HEX_StackElement element);

void hex_fail(char *message)
{
    fprintf(stderr, "%s\n", message);
}

// Function to add a variable to the dictionary
int hex_set_variable(const char *key, HEX_StackElement value)
{
    for (int i = 0; i < hex_dictCount; i++)
    {
        if (strcmp(hex_dictionary[i].key, key) == 0)
        {
            if (hex_dictionary[i].value.type == HEX_TYPE_FUNCTION)
            {
                fprintf(stderr, "Cannot overwrite native symbol %s", key);
                return 0;
            }
            free(hex_dictionary[i].key);
            hex_free_element(hex_dictionary[i].value);
            hex_dictionary[i].key = strdup(key);
            hex_dictionary[i].value = value;
            return 0;
        }
    }

    if (hex_dictCount >= HEX_DICTIONARY_SIZE)
    {
        hex_fail("Dictionary overflow");
        return 1;
    }

    hex_dictionary[hex_dictCount].key = strdup(key);
    hex_dictionary[hex_dictCount].value = value;
    hex_dictCount++;
    return 1;
}

void hex_register_symbol(const char *name, void (*func)())
{
    HEX_StackElement funcElement;
    funcElement.type = HEX_TYPE_FUNCTION;
    funcElement.data.functionPointer = func;

    if (!hex_set_variable(name, funcElement))
    {
        fprintf(stderr, "Error: Failed to register native symbol '%s'.\n", name);
    }
}

// Function to get a variable value from the dictionary
int hex_get_variable(const char *key, HEX_StackElement *result)
{
    for (int i = 0; i < hex_dictCount; i++)
    {
        if (strcmp(hex_dictionary[i].key, key) == 0)
        {
            *result = hex_dictionary[i].value;
            return 1;
        }
    }
    return 0;
}

// Stack Definition
#define HEX_STACK_SIZE 100
HEX_StackElement hex_stack[HEX_STACK_SIZE];
int hex_top = -1;

// Push functions
void hex_push(HEX_StackElement element)
{
    if (hex_top >= HEX_STACK_SIZE - 1)
    {
        hex_fail("Stack overflow");
    }
    hex_stack[++hex_top] = element;
}

void hex_push_int(int value)
{
    HEX_StackElement element = {.type = HEX_TYPE_INTEGER, .data.intValue = value};
    hex_push(element);
}

void hex_push_string(const char *value)
{
    HEX_StackElement element = {.type = HEX_TYPE_STRING, .data.strValue = strdup(value)};
    hex_push(element);
}

void hex_push_quotation(HEX_StackElement **quotation, size_t size)
{
    HEX_StackElement element = {.type = HEX_TYPE_QUOTATION, .data.quotationValue = quotation, .quotationSize = size};
    hex_push(element);
}

// Pop function
HEX_StackElement hex_pop()
{
    if (hex_top < 0)
    {
        hex_fail("Stack underflow");
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

// Token Types
typedef enum
{
    HEX_TOKEN_NUMBER,
    HEX_TOKEN_STRING,
    HEX_TOKEN_OPERATOR,
    HEX_TOKEN_QUOTATION_START,
    HEX_TOKEN_QUOTATION_END
} HEX_TokenType;

typedef struct
{
    HEX_TokenType type;
    char *value;
} HEX_Token;

// Tokenizer Implementation
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
            hex_fail("Unterminated string");
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
        while (*ptr != '\0' && !isspace(*ptr) && *ptr != ';')
        {
            ptr++;
        }
        size_t len = ptr - start;
        token->value = (char *)malloc(len + 1);
        strncpy(token->value, start, len);
        token->value[len] = '\0';
        token->type = HEX_TOKEN_OPERATOR;
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
        hex_fail("Memory allocation failed");
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
                hex_fail("Memory allocation failed");
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
        else if (token->type == HEX_TOKEN_QUOTATION_START)
        {
            element->type = HEX_TYPE_QUOTATION;
            element->data.quotationValue = hex_parse_quotation(input, &element->quotationSize);
        }
        else
        {
            hex_fail("Unexpected token in quotation");
        }

        quotation[*size] = element;
        (*size)++;
        hex_free_token(token);
    }

    return quotation;
}

char *hex_itoa(int num)
{
    static char str[20];
    int i = 0;
    int base = 16;

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
    case HEX_TYPE_FUNCTION:
        fprintf(stream, "<native>");
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
        hex_fail("Unknown element type");
    }
}

int hex_is_operator(HEX_Token *token, char *value)
{
    return strcmp(token->value, value) == 0;
}

void hex_operator_define()
{
    HEX_StackElement name = hex_pop();
    HEX_StackElement value = hex_pop();
    if (name.type != HEX_TYPE_STRING)
    {
        hex_fail("Variable name must be a string");
    }
    if (hex_set_variable(name.data.strValue, value))
    {
    }
    hex_free_element(name);
}

// IO Operators

void hex_operator_puts()
{
    HEX_StackElement element = hex_pop();
    hex_print_element(stdout, element);
    printf("\n");
    hex_free_element(element);
}

void hex_operator_warn()
{
    HEX_StackElement element = hex_pop();
    hex_print_element(stderr, element);
    printf("\n");
    hex_free_element(element);
}

void hex_operator_print()
{
    HEX_StackElement element = hex_pop();
    hex_print_element(stdout, element);
    hex_free_element(element);
}

void hex_operator_gets()
{
    char input[HEX_STDIN_BUFFER_SIZE]; // Buffer to store the input (adjust size if needed)

    if (fgets(input, sizeof(input), stdin) != NULL)
    {
        // Strip the newline character at the end of the string
        input[strcspn(input, "\n")] = '\0';

        // Push the input string onto the stack
        hex_push_string(input);
    }
    else
    {
        hex_fail("Failed to read input");
    }
}

// Mathematical operators
void hex_operator_add()
{
    HEX_StackElement b = hex_pop();
    HEX_StackElement a = hex_pop();
    if (a.type == HEX_TYPE_INTEGER && b.type == HEX_TYPE_INTEGER)
    {
        hex_push_int(a.data.intValue + b.data.intValue);
    }
    else
    {
        hex_fail("'+' operator requires two integers");
    }
    hex_free_element(a);
    hex_free_element(b);
}

void hex_operator_subtract()
{
    HEX_StackElement b = hex_pop();
    HEX_StackElement a = hex_pop();
    if (a.type == HEX_TYPE_INTEGER && b.type == HEX_TYPE_INTEGER)
    {
        hex_push_int(a.data.intValue - b.data.intValue);
    }
    else
    {
        hex_fail("'-' operator requires two integers");
    }
    hex_free_element(a);
    hex_free_element(b);
}

void hex_operator_multiply()
{
    HEX_StackElement b = hex_pop();
    HEX_StackElement a = hex_pop();
    if (a.type == HEX_TYPE_INTEGER && b.type == HEX_TYPE_INTEGER)
    {
        hex_push_int(a.data.intValue * b.data.intValue);
    }
    else
    {
        hex_fail("'*' operator requires two integers");
    }
    hex_free_element(a);
    hex_free_element(b);
}

void hex_operator_divide()
{
    HEX_StackElement b = hex_pop();
    HEX_StackElement a = hex_pop();
    if (a.type == HEX_TYPE_INTEGER && b.type == HEX_TYPE_INTEGER)
    {
        if (b.data.intValue == 0)
        {
            hex_fail("Division by zero");
        }
        hex_push_int(a.data.intValue / b.data.intValue);
    }
    else
    {
        hex_fail("'/' operator requires two integers");
    }
    hex_free_element(a);
    hex_free_element(b);
}

// Bit operators

void hex_operator_bitand()
{
    HEX_StackElement right = hex_pop(); // Pop the second operand
    HEX_StackElement left = hex_pop();  // Pop the first operand
    if (left.type == HEX_TYPE_INTEGER && right.type == HEX_TYPE_INTEGER)
    {
        int result = left.data.intValue & right.data.intValue; // Bitwise AND
        hex_push_int(result);                                  // Push the result back onto the stack
    }
    else
    {
        hex_fail("Bitwise AND requires two integers");
    }
}

void hex_operator_bitor()
{
    HEX_StackElement right = hex_pop();
    HEX_StackElement left = hex_pop();
    if (left.type == HEX_TYPE_INTEGER && right.type == HEX_TYPE_INTEGER)
    {
        int result = left.data.intValue | right.data.intValue; // Bitwise OR
        hex_push_int(result);
    }
    else
    {
        hex_fail("Bitwise OR requires two integers");
    }
}

void hex_operator_bitxor()
{
    HEX_StackElement right = hex_pop();
    HEX_StackElement left = hex_pop();
    if (left.type == HEX_TYPE_INTEGER && right.type == HEX_TYPE_INTEGER)
    {
        int result = left.data.intValue ^ right.data.intValue; // Bitwise XOR
        hex_push_int(result);
    }
    else
    {
        hex_fail("Bitwise XOR requires two integers");
    }
}

void hex_operator_shiftleft()
{
    HEX_StackElement right = hex_pop(); // The number of positions to shift
    HEX_StackElement left = hex_pop();  // The value to shift
    if (left.type == HEX_TYPE_INTEGER && right.type == HEX_TYPE_INTEGER)
    {
        int result = left.data.intValue << right.data.intValue; // Left shift
        hex_push_int(result);
    }
    else
    {
        hex_fail("Left shift requires two integers");
    }
}

void hex_operator_shiftright()
{
    HEX_StackElement right = hex_pop(); // The number of positions to shift
    HEX_StackElement left = hex_pop();  // The value to shift
    if (left.type == HEX_TYPE_INTEGER && right.type == HEX_TYPE_INTEGER)
    {
        int result = left.data.intValue >> right.data.intValue; // Right shift
        hex_push_int(result);
    }
    else
    {
        hex_fail("Right shift requires two integers");
    }
}

void hex_operator_bitnot()
{
    HEX_StackElement element = hex_pop(); // Only one operand for bitwise NOT
    if (element.type == HEX_TYPE_INTEGER)
    {
        int result = ~element.data.intValue; // Bitwise NOT (complement)
        hex_push_int(result);
    }
    else
    {
        hex_fail("Bitwise NOT requires one integer");
    }
}

// Converter operators
void hex_operator_int()
{
    HEX_StackElement a = hex_pop();
    if (a.type == HEX_TYPE_QUOTATION)
    {
        hex_fail("Cannot convert a quotation to an integer");
    }
    else if (a.type == HEX_TYPE_INTEGER)
    {
        hex_push_int(a.data.intValue);
    }
    else if (a.type == HEX_TYPE_STRING)
    {
        hex_push_int(strtol(a.data.strValue, NULL, 16));
    }
}

void hex_operator_str()
{
    HEX_StackElement a = hex_pop();
    if (a.type == HEX_TYPE_QUOTATION)
    {
        hex_fail("Cannot convert a quotation to a string");
    }
    else if (a.type == HEX_TYPE_INTEGER)
    {
        hex_push_string(hex_itoa(a.data.intValue));
    }
    else if (a.type == HEX_TYPE_STRING)
    {
        hex_push_string(a.data.strValue);
    }
}

void hex_process(const char *code)
{
    const char *input = code;
    HEX_Token *token;

    while ((token = hex_next_token(&input)) != NULL)
    {
        if (token->type == HEX_TOKEN_NUMBER)
        {
            hex_push_int((int)strtol(token->value, NULL, 16));
        }
        else if (token->type == HEX_TOKEN_STRING)
        {
            HEX_StackElement variableValue;
            if (hex_get_variable(token->value, &variableValue))
            {
                hex_push(variableValue);
            }
            else
            {
                hex_push_string(token->value);
            }
        }
        else if (token->type == HEX_TOKEN_OPERATOR)
        {
            if (strcmp(token->value, "define") == 0)
            {
                hex_operator_define();
            }
            else if (hex_is_operator(token, "+"))
            {
                hex_operator_add();
            }
            else if (hex_is_operator(token, "puts"))
            {
                hex_operator_puts();
            }
            else if (hex_is_operator(token, "gets"))
            {
                hex_operator_gets();
            }
            else if (hex_is_operator(token, "&"))
            {
                hex_operator_bitand();
            }
            else if (hex_is_operator(token, "|"))
            {
                hex_operator_bitor();
            }
            else if (hex_is_operator(token, "^"))
            {
                hex_operator_bitxor();
            }
            else if (hex_is_operator(token, "~"))
            {
                hex_operator_bitnot();
            }
            else if (hex_is_operator(token, "<<"))
            {
                hex_operator_shiftleft();
            }
            else if (hex_is_operator(token, ">>"))
            {
                hex_operator_shiftright();
            }
            else if (hex_is_operator(token, "-"))
            {
                hex_operator_subtract();
            }
            else if (hex_is_operator(token, "*"))
            {
                hex_operator_multiply();
            }
            else if (hex_is_operator(token, "/"))
            {
                hex_operator_divide();
            }
            else if (hex_is_operator(token, "int"))
            {
                hex_operator_int();
            }
            else if (hex_is_operator(token, "str"))
            {
                hex_operator_str();
            }
            else
            {
                fprintf(stderr, "Unknown operator: %s\n", token->value);
                exit(EXIT_FAILURE);
            }
        }
        else if (token->type == HEX_TOKEN_QUOTATION_START)
        {
            size_t quotationSize;
            HEX_StackElement **quotation = hex_parse_quotation(&input, &quotationSize);
            hex_push_quotation(quotation, quotationSize);
        }
        hex_free_token(token);
    }
}

// Main Function
char *hex_read_file(const char *filename)
{
    FILE *file = fopen(filename, "r");
    if (!file)
    {
        fprintf(stderr, "Could not open file: %s\n", filename);
        exit(EXIT_FAILURE);
    }

    fseek(file, 0, SEEK_END);
    long length = ftell(file);
    fseek(file, 0, SEEK_SET);

    char *buffer = (char *)malloc(length + 1);
    if (!buffer)
    {
        hex_fail("Memory allocation failed");
    }

    fread(buffer, 1, length, file);
    buffer[length] = '\0';
    fclose(file);
    return buffer;
}

volatile sig_atomic_t hex_keep_running = 1;

void hex_repl()
{
    char line[1024];
    printf("Hex Interactive REPL. Type 'exit' to quit or press Ctrl+C.\n");

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
        hex_process(line);
    }
}

void hex_handle_sigint(int sig)
{
    (void)sig; // Suppress unused warning
    hex_keep_running = 0;
    printf("\nExiting Hex REPL. Goodbye!\n");
}

void hex_process_stdin()
{
    char buffer[8192]; // Adjust buffer size as needed
    size_t bytesRead = fread(buffer, 1, sizeof(buffer) - 1, stdin);
    if (bytesRead == 0)
    {
        fprintf(stderr, "Error: No input provided via stdin.\n");
        return;
    }

    buffer[bytesRead] = '\0'; // Null-terminate the input
    hex_process(buffer);
}

int main(int argc, char *argv[])
{
    // Register SIGINT (Ctrl+C) signal handler
    signal(SIGINT, hex_handle_sigint);

    if (argc == 2)
    {
        // Process a file
        const char *filename = argv[1];
        char *fileContent = hex_read_file(filename);
        if (!fileContent)
        {
            return EXIT_FAILURE;
        }

        hex_process(fileContent);
        free(fileContent); // Free the allocated memory
    }
    else if (!hex_isatty(fileno(stdin)))
    {
        // Process piped input from stdin
        hex_process_stdin();
    }
    else
    {
        // Start REPL
        hex_repl();
    }

    return EXIT_SUCCESS;
}

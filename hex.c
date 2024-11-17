#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>
#include <signal.h>
#ifdef _WIN32
#include <windows.h>
#include <io.h>
int isatty(int fd) {
    HANDLE h = (HANDLE)_get_osfhandle(fd);
    return (GetFileType(h) == FILE_TYPE_CHAR);
}
#else
#include <unistd.h>
#endif


// Enum to represent the type of stack elements
typedef enum
{
    TYPE_INTEGER,
    TYPE_STRING,
    TYPE_QUOTATION
    TYPE_FUNCTION
} ElementType;

// Unified Stack Element
typedef struct StackElement
{
    ElementType type;
    union
    {
        int intValue;                     
        char *strValue;                   
        void (*functionPointer)()
        struct StackElement **quotationValue;
    } data;
    size_t quotationSize; // Size of the quotation (valid for TYPE_QUOTATION)
} StackElement;

// Dictionary Entry
typedef struct
{
    char *key;
    StackElement value;
} DictionaryEntry;

// Size of STDIN buffer (gets operator)
#define STDIN_BUFFER_SIZE 256

// Global Dictionary for Variables
#define DICTIONARY_SIZE 1024
DictionaryEntry dictionary[DICTIONARY_SIZE];
int dictCount = 0;

void free_element(StackElement element);

void fail(char *message)
{
    fprintf(stderr, "%s\n", message);
}

// Function to add a variable to the dictionary
int set_variable(const char *key, StackElement value)
{
    for (int i = 0; i < dictCount; i++)
    {
        if (strcmp(dictionary[i].key, key) == 0)
        {
            if (dictionary[i].value.type == TYPE_FUNCTION){
                fprintf(stderr, "Cannot overwrite native symbol %s", key);
                return 0;
            }
            free(dictionary[i].key);
            free_element(dictionary[i].value);
            dictionary[i].key = strdup(key);
            dictionary[i].value = value;
            return 0;
        }
    }

    if (dictCount >= DICTIONARY_SIZE)
    {
        fail("Dictionary overflow");
        return 1;
    }

    dictionary[dictCount].key = strdup(key);
    dictionary[dictCount].value = value;
    dictCount++;
    return 1;
}

void add_function(const char *name, void (*func)()) {
    StackElement funcElement;
    funcElement.type = TYPE_FUNCTION;
    funcElement.data.functionPointer = func;

    if (!add_variable(name, funcElement)) {
        fprintf(stderr, "Error: Failed to register function '%s'.\n", name);
    }
}

// Function to get a variable value from the dictionary
bool get_variable(const char *key, StackElement *result)
{
    for (int i = 0; i < dictCount; i++)
    {
        if (strcmp(dictionary[i].key, key) == 0)
        {
            *result = dictionary[i].value;
            return true;
        }
    }
    return false;
}

// Stack Definition
#define STACK_SIZE 100
StackElement stack[STACK_SIZE];
int top = -1;

// Push functions
void push(StackElement element)
{
    if (top >= STACK_SIZE - 1)
    {
        fail("Stack overflow");
    }
    stack[++top] = element;
}

void push_int(int value)
{
    StackElement element = {.type = TYPE_INTEGER, .data.intValue = value};
    push(element);
}

void push_string(const char *value)
{
    StackElement element = {.type = TYPE_STRING, .data.strValue = strdup(value)};
    push(element);
}

void push_quotation(StackElement **quotation, size_t size)
{
    StackElement element = {.type = TYPE_QUOTATION, .data.quotationValue = quotation, .quotationSize = size};
    push(element);
}

// Pop function
StackElement pop()
{
    if (top < 0)
    {
        fail("Stack underflow");
    }
    return stack[top--];
}

// Free a stack element
void free_element(StackElement element)
{
    if (element.type == TYPE_STRING)
    {
        free(element.data.strValue);
    }
    else if (element.type == TYPE_QUOTATION)
    {
        for (size_t i = 0; i < element.quotationSize; i++)
        {
            free_element(*element.data.quotationValue[i]);
            free(element.data.quotationValue[i]);
        }
        free(element.data.quotationValue);
    }
}

// Token Types
typedef enum
{
    TOKEN_NUMBER,
    TOKEN_STRING,
    TOKEN_OPERATOR,
    TOKEN_QUOTATION_START,
    TOKEN_QUOTATION_END
} TokenType;

typedef struct
{
    TokenType type;
    char *value;
} Token;

// Tokenizer Implementation
Token *next_token(const char **input)
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

    Token *token = (Token *)malloc(sizeof(Token));
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
            fail("Unterminated string");
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
        token->type = TOKEN_STRING;
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
        token->type = TOKEN_NUMBER;
    }
    else if (*ptr == '(')
    {
        token->type = TOKEN_QUOTATION_START;
        ptr++;
    }
    else if (*ptr == ')')
    {
        token->type = TOKEN_QUOTATION_END;
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
        token->type = TOKEN_OPERATOR;
    }

    *input = ptr;
    return token;
}

// Free a token
void free_token(Token *token)
{
    if (token)
    {
        free(token->value);
        free(token);
    }
}

// Recursive quotation parsing
StackElement **parse_quotation(const char **input, size_t *size)
{
    StackElement **quotation = NULL;
    size_t capacity = 2;
    *size = 0;

    quotation = (StackElement **)malloc(capacity * sizeof(StackElement *));
    if (!quotation)
    {
        fail("Memory allocation failed");
    }

    Token *token;
    while ((token = next_token(input)) != NULL)
    {
        if (token->type == TOKEN_QUOTATION_END)
        {
            free_token(token);
            break;
        }

        if (*size >= capacity)
        {
            capacity *= 2;
            quotation = (StackElement **)realloc(quotation, capacity * sizeof(StackElement *));
            if (!quotation)
            {
                fail("Memory allocation failed");
            }
        }

        StackElement *element = (StackElement *)malloc(sizeof(StackElement));
        if (token->type == TOKEN_NUMBER)
        {
            element->type = TYPE_INTEGER;
            element->data.intValue = (int)strtol(token->value, NULL, 16);
        }
        else if (token->type == TOKEN_STRING)
        {
            element->type = TYPE_STRING;
            element->data.strValue = strdup(token->value);
        }
        else if (token->type == TOKEN_QUOTATION_START)
        {
            element->type = TYPE_QUOTATION;
            element->data.quotationValue = parse_quotation(input, &element->quotationSize);
        }
        else
        {
            fail("Unexpected token in quotation");
        }

        quotation[*size] = element;
        (*size)++;
        free_token(token);
    }

    return quotation;
}

char *itoa(int num)
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

void print_element(FILE *stream, StackElement element)
{
    switch (element.type)
    {
    case TYPE_INTEGER:
        fprintf(stream, "0x%x", element.data.intValue);
        break;
    case TYPE_STRING:
        fprintf(stream, "\"%s\"", element.data.strValue);
        break;
    case TYPE_FUNCTION:
        fprintf(stream, "<native>")
        break;
    case TYPE_QUOTATION:
    {
        fprintf(stream, "(");
        for (size_t i = 0; i < element.quotationSize; i++)
        {
            if (i > 0)
                fprintf(stream, " "); // Add space between elements
            print_element(stream, *element.data.quotationValue[i]);
        }
        fprintf(stream, ")");
        break;
    }
    default:
        fail("Unknown element type");
    }
}

int is_operator(Token *token, char *value)
{
    return strcmp(token->value, value) == 0;
}

void operator_define()
{
    StackElement name = pop();
    StackElement value = pop();
    if (name.type != TYPE_STRING)
    {
        fail("Variable name must be a string");
    }
    if (set_variable(name.data.strValue, value)) {}
    free_element(name);
}

// IO Operators

void operator_puts()
{
    StackElement element = pop();
    print_element(stdout, element);
    printf("\n");
    free_element(element);
}

void operator_warn()
{
    StackElement element = pop();
    print_element(stderr, element);
    printf("\n");
    free_element(element);
}

void operator_print()
{
    StackElement element = pop();
    print_element(stdout, element);
    free_element(element);
}

void operator_gets()
{
    char input[STDIN_BUFFER_SIZE]; // Buffer to store the input (adjust size if needed)

    if (fgets(input, sizeof(input), stdin) != NULL)
    {
        // Strip the newline character at the end of the string
        input[strcspn(input, "\n")] = '\0';

        // Push the input string onto the stack
        push_string(input);
    }
    else
    {
        fail("Failed to read input");
    }
}

// Mathematical operators
void operator_add()
{
    StackElement b = pop();
    StackElement a = pop();
    if (a.type == TYPE_INTEGER && b.type == TYPE_INTEGER)
    {
        push_int(a.data.intValue + b.data.intValue);
    }
    else
    {
        fail("'+' operator requires two integers");
    }
    free_element(a);
    free_element(b);
}

void operator_subtract()
{
    StackElement b = pop();
    StackElement a = pop();
    if (a.type == TYPE_INTEGER && b.type == TYPE_INTEGER)
    {
        push_int(a.data.intValue - b.data.intValue);
    }
    else
    {
        fail("'-' operator requires two integers");
    }
    free_element(a);
    free_element(b);
}

void operator_multiply()
{
    StackElement b = pop();
    StackElement a = pop();
    if (a.type == TYPE_INTEGER && b.type == TYPE_INTEGER)
    {
        push_int(a.data.intValue * b.data.intValue);
    }
    else
    {
        fail("'*' operator requires two integers");
    }
    free_element(a);
    free_element(b);
}

void operator_divide()
{
    StackElement b = pop();
    StackElement a = pop();
    if (a.type == TYPE_INTEGER && b.type == TYPE_INTEGER)
    {
        if (b.data.intValue == 0)
        {
            fail("Division by zero");
        }
        push_int(a.data.intValue / b.data.intValue);
    }
    else
    {
        fail("'/' operator requires two integers");
    }
    free_element(a);
    free_element(b);
}

// Bit operators

void operator_bitand()
{
    StackElement right = pop();  // Pop the second operand
    StackElement left = pop();   // Pop the first operand
    if (left.type == TYPE_INTEGER && right.type == TYPE_INTEGER) {
        int result = left.data.intValue & right.data.intValue;  // Bitwise AND
        push_int(result);  // Push the result back onto the stack
    } else {
        fail("Bitwise AND requires two integers");
    }
}

void operator_bitor()
{
    StackElement right = pop();
    StackElement left = pop();
    if (left.type == TYPE_INTEGER && right.type == TYPE_INTEGER) {
        int result = left.data.intValue | right.data.intValue;  // Bitwise OR
        push_int(result);
    } else {
        fail("Bitwise OR requires two integers");
    }
}

void operator_bitxor()
{
    StackElement right = pop();
    StackElement left = pop();
    if (left.type == TYPE_INTEGER && right.type == TYPE_INTEGER) {
        int result = left.data.intValue ^ right.data.intValue;  // Bitwise XOR
        push_int(result);
    } else {
        fail("Bitwise XOR requires two integers");
    }
}

void operator_shiftleft()
{
    StackElement right = pop();  // The number of positions to shift
    StackElement left = pop();   // The value to shift
    if (left.type == TYPE_INTEGER && right.type == TYPE_INTEGER) {
        int result = left.data.intValue << right.data.intValue;  // Left shift
        push_int(result);
    } else {
        fail("Left shift requires two integers");
    }
}

void operator_shiftright()
{
    StackElement right = pop();  // The number of positions to shift
    StackElement left = pop();   // The value to shift
    if (left.type == TYPE_INTEGER && right.type == TYPE_INTEGER) {
        int result = left.data.intValue >> right.data.intValue;  // Right shift
        push_int(result);
    } else {
        fail("Right shift requires two integers");
    }
}

void operator_bitnot()
{
    StackElement element = pop();  // Only one operand for bitwise NOT
    if (element.type == TYPE_INTEGER) {
        int result = ~element.data.intValue;  // Bitwise NOT (complement)
        push_int(result);
    } else {
        fail("Bitwise NOT requires one integer");
    }
}

// Converter operators
void operator_int()
{
    StackElement a = pop();
    if (a.type == TYPE_QUOTATION)
    {
        fail("Cannot convert a quotation to an integer");
    }
    else if (a.type == TYPE_INTEGER)
    {
        push_int(a.data.intValue);
    }
    else if (a.type == TYPE_STRING)
    {
        push_int(strtol(a.data.strValue, NULL, 16));
    }
}

void operator_str()
{
    StackElement a = pop();
    if (a.type == TYPE_QUOTATION)
    {
        fail("Cannot convert a quotation to a string");
    }
    else if (a.type == TYPE_INTEGER)
    {
        push_string(itoa(a.data.intValue));
    }
    else if (a.type == TYPE_STRING)
    {
        push_string(a.data.strValue);
    }
}

// Process code
void process(const char *code)
{
    const char *input = code;
    Token *token;

    while ((token = next_token(&input)) != NULL)
    {
        if (token->type == TOKEN_NUMBER)
        {
            push_int((int)strtol(token->value, NULL, 16));
        }
        else if (token->type == TOKEN_STRING)
        {
            StackElement variableValue;
            if (get_variable(token->value, &variableValue))
            {
                push(variableValue);
            }
            else
            {
                push_string(token->value);
            }
        }
        else if (token->type == TOKEN_OPERATOR)
        {
            if (strcmp(token->value, "define") == 0)
            {
                operator_define();
            }
            else if (is_operator(token, "+"))
            {
                operator_add();
            }
            else if (is_operator(token, "puts"))
            {
                operator_puts();
            }
            else if (is_operator(token, "gets"))
            {
                operator_gets();
            }
            else if (is_operator(token, "&"))
            {
                operator_bitand();
            }
            else if (is_operator(token, "|"))
            {
                operator_bitor();
            }
            else if (is_operator(token, "^"))
            {
                operator_bitxor();
            }
            else if (is_operator(token, "~"))
            {
                operator_bitnot();
            }
            else if (is_operator(token, "<<"))
            {
                operator_shiftleft();
            }
            else if (is_operator(token, ">>"))
            {
                operator_shiftright();
            }
            else if (is_operator(token, "-"))
            {
                operator_subtract();
            }
            else if (is_operator(token, "*"))
            {
                operator_multiply();
            }
            else if (is_operator(token, "/"))
            {
                operator_divide();
            }
            else if (is_operator(token, "int"))
            {
                operator_int();
            }
            else if (is_operator(token, "str"))
            {
                operator_str();
            }
            else
            {
                fprintf(stderr, "Unknown operator: %s\n", token->value);
                exit(EXIT_FAILURE);
            }
        }
        else if (token->type == TOKEN_QUOTATION_START)
        {
            size_t quotationSize;
            StackElement **quotation = parse_quotation(&input, &quotationSize);
            push_quotation(quotation, quotationSize);
        }
        free_token(token);
    }
}

// Main Function
char *read_file(const char *filename)
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
        fail("Memory allocation failed");
    }

    fread(buffer, 1, length, file);
    buffer[length] = '\0';
    fclose(file);
    return buffer;
}

volatile sig_atomic_t keep_running = 1;

void repl() {
    char line[1024];
    printf("hex Interactive REPL. Type 'exit' to quit or press Ctrl+C.\n");

    while (keep_running) {
        printf("> ");  // Prompt
        if (fgets(line, sizeof(line), stdin) == NULL) {
            printf("\n");  // Handle EOF (Ctrl+D)
            break;
        }

        // Normalize line endings (remove trailing \r\n or \n)
        line[strcspn(line, "\r\n")] = '\0';

        // Exit command
        if (strcmp(line, "exit") == 0) {
            break;
        }

        // Tokenize and process the input
        process(line);
    }
}


void handle_sigint(int sig) {
    (void)sig;  // Suppress unused warning
    keep_running = 0;
    printf("\nExiting hex REPL. Goodbye!\n");
}

void process_stdin() {
    char buffer[8192];  // Adjust buffer size as needed
    size_t bytesRead = fread(buffer, 1, sizeof(buffer) - 1, stdin);
    if (bytesRead == 0) {
        fprintf(stderr, "Error: No input provided via stdin.\n");
        return;
    }

    buffer[bytesRead] = '\0';  // Null-terminate the input
    process(buffer);
}

int main(int argc, char *argv[]) {
    // Register SIGINT (Ctrl+C) signal handler
    signal(SIGINT, handle_sigint);

    if (argc == 2) {
        // Process a file
        const char *filename = argv[1];
        char *fileContent = read_file(filename);
        if (!fileContent) {
            return EXIT_FAILURE;
        }

        process(fileContent);
        free(fileContent);  // Free the allocated memory
    } else if (!isatty(fileno(stdin))) {
        // Process piped input from stdin
        process_stdin();
    } else {
        // Start REPL
        repl();
    }

    return EXIT_SUCCESS;
}



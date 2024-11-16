#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>

// Enum to represent the type of stack elements
typedef enum
{
    TYPE_INTEGER,
    TYPE_STRING,
    TYPE_ARRAY
} ElementType;

// Unified Stack Element
typedef struct StackElement
{
    ElementType type;
    union
    {
        int intValue;                     // For integers
        char *strValue;                   // For strings
        struct StackElement **arrayValue; // For arrays
    } data;
    size_t arraySize; // Size of the array (valid for TYPE_ARRAY)
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
#define DICTIONARY_SIZE 100
DictionaryEntry dictionary[DICTIONARY_SIZE];
int dictCount = 0;

void free_element(StackElement element);

// Function to add a variable to the dictionary
void set_variable(const char *key, StackElement value)
{
    for (int i = 0; i < dictCount; i++)
    {
        if (strcmp(dictionary[i].key, key) == 0)
        {
            free(dictionary[i].key);
            free_element(dictionary[i].value);
            dictionary[i].key = strdup(key);
            dictionary[i].value = value;
            return;
        }
    }

    if (dictCount >= DICTIONARY_SIZE)
    {
        fprintf(stderr, "Error: Dictionary overflow\n");
        exit(EXIT_FAILURE);
    }

    dictionary[dictCount].key = strdup(key);
    dictionary[dictCount].value = value;
    dictCount++;
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
        fprintf(stderr, "Error: Stack overflow\n");
        exit(EXIT_FAILURE);
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

void push_array(StackElement **array, size_t size)
{
    StackElement element = {.type = TYPE_ARRAY, .data.arrayValue = array, .arraySize = size};
    push(element);
}

// Pop function
StackElement pop()
{
    if (top < 0)
    {
        fprintf(stderr, "Error: Stack underflow\n");
        exit(EXIT_FAILURE);
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
    else if (element.type == TYPE_ARRAY)
    {
        for (size_t i = 0; i < element.arraySize; i++)
        {
            free_element(*element.data.arrayValue[i]);
            free(element.data.arrayValue[i]);
        }
        free(element.data.arrayValue);
    }
}

// Token Types
typedef enum
{
    TOKEN_NUMBER,
    TOKEN_STRING,
    TOKEN_OPERATOR,
    TOKEN_ARRAY_START,
    TOKEN_ARRAY_END
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
            fprintf(stderr, "Error: Unterminated string\n");
            exit(EXIT_FAILURE);
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
        token->type = TOKEN_ARRAY_START;
        ptr++;
    }
    else if (*ptr == ')')
    {
        token->type = TOKEN_ARRAY_END;
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

// Recursive array parsing
StackElement **parse_array(const char **input, size_t *size)
{
    StackElement **array = NULL;
    size_t capacity = 2;
    *size = 0;

    array = (StackElement **)malloc(capacity * sizeof(StackElement *));
    if (!array)
    {
        fprintf(stderr, "Error: Memory allocation failed\n");
        exit(EXIT_FAILURE);
    }

    Token *token;
    while ((token = next_token(input)) != NULL)
    {
        if (token->type == TOKEN_ARRAY_END)
        {
            free_token(token);
            break;
        }

        if (*size >= capacity)
        {
            capacity *= 2;
            array = (StackElement **)realloc(array, capacity * sizeof(StackElement *));
            if (!array)
            {
                fprintf(stderr, "Error: Memory allocation failed\n");
                exit(EXIT_FAILURE);
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
        else if (token->type == TOKEN_ARRAY_START)
        {
            element->type = TYPE_ARRAY;
            element->data.arrayValue = parse_array(input, &element->arraySize);
        }
        else
        {
            fprintf(stderr, "Error: Unexpected token in array\n");
            exit(EXIT_FAILURE);
        }

        array[*size] = element;
        (*size)++;
        free_token(token);
    }

    return array;
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
    case TYPE_ARRAY:
    {
        fprintf(stream, "(");
        for (size_t i = 0; i < element.arraySize; i++)
        {
            if (i > 0)
                fprintf(stream, " "); // Add space between elements
            print_element(stream, *element.data.arrayValue[i]);
        }
        fprintf(stream, ")");
        break;
    }
    default:
        fprintf(stderr, "Error: Unknown element type\n");
        exit(EXIT_FAILURE);
    }
}

void fail(char *message)
{
    fprintf(stderr, "ERROR: %s\n", message);
    exit(EXIT_FAILURE);
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
        fprintf(stderr, "Error: Variable name must be a string\n");
        exit(EXIT_FAILURE);
    }
    set_variable(name.data.strValue, value);
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
        fprintf(stderr, "Error: Failed to read input\n");
        exit(EXIT_FAILURE);
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
        fprintf(stderr, "Error: '+' operator requires two integers\n");
        exit(EXIT_FAILURE);
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
        fprintf(stderr, "Error: '-' operator requires two integers\n");
        exit(EXIT_FAILURE);
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
        fprintf(stderr, "Error: '*' operator requires two integers\n");
        exit(EXIT_FAILURE);
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
            fprintf(stderr, "Error: Division by zero\n");
            exit(EXIT_FAILURE);
        }
        push_int(a.data.intValue / b.data.intValue);
    }
    else
    {
        fprintf(stderr, "Error: '/' operator requires two integers\n");
        exit(EXIT_FAILURE);
    }
    free_element(a);
    free_element(b);
}

// Converter operators
void operator_int()
{
    StackElement a = pop();
    if (a.type == TYPE_ARRAY)
    {
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
            else if (strcmp(token->value, "-") == 0)
            {
                operator_subtract();
            }
            else if (strcmp(token->value, "*") == 0)
            {
                operator_multiply();
            }
            else if (strcmp(token->value, "/") == 0)
            {
                operator_divide();
            }
            else
            {
                fprintf(stderr, "Error: Unknown operator: %s\n", token->value);
                exit(EXIT_FAILURE);
            }
        }
        else if (token->type == TOKEN_ARRAY_START)
        {
            size_t arraySize;
            StackElement **array = parse_array(&input, &arraySize);
            push_array(array, arraySize);
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
        fprintf(stderr, "Error: Could not open file: %s\n", filename);
        exit(EXIT_FAILURE);
    }

    fseek(file, 0, SEEK_END);
    long length = ftell(file);
    fseek(file, 0, SEEK_SET);

    char *buffer = (char *)malloc(length + 1);
    if (!buffer)
    {
        fprintf(stderr, "Error: Memory allocation failed\n");
        exit(EXIT_FAILURE);
    }

    fread(buffer, 1, length, file);
    buffer[length] = '\0';
    fclose(file);
    return buffer;
}

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        fprintf(stderr, "Usage: %s <filename>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    char *code = read_file(argv[1]);
    process(code);
    free(code);

    // Cleanup dictionary
    for (int i = 0; i < dictCount; i++)
    {
        free(dictionary[i].key);
        free_element(dictionary[i].value);
    }

    return 0;
}

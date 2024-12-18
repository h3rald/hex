#ifndef HEX_H
#include "hex.h"
#endif

// Read a file into a buffer
char *hex_read_file(hex_context_t *ctx, const char *filename)
{
    FILE *file = fopen(filename, "r");
    if (file == NULL)
    {
        hex_error(ctx, "Failed to open file: %s", filename);
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

    // Handle hashbang if present
    char hashbangLine[1024];
    if (fgets(hashbangLine, sizeof(hashbangLine), file) != NULL)
    {
        if (strncmp(hashbangLine, "#!", 2) != 0)
        {
            // Not a hashbang line, reset file pointer to the beginning
            fseek(file, 0, SEEK_SET);
            ctx->hashbang = 0;
        }
        else
        {
            ctx->hashbang = 1;
        }
    }

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

#if defined(__EMSCRIPTEN__) && defined(BROWSER)
static void prompt()
{
    // no prompt needed on browser
}
#elif defined(__EMSCRIPTEN__) && !defined(BROWSER)
static void prompt()
{
    printf(">\n");
}
#else
static void prompt()
{
    printf("> ");
    fflush(stdout);
}
#endif

#if defined(__EMSCRIPTEN__)
static void do_repl(void *v_ctx)
{
    hex_context_t *ctx = (hex_context_t *)v_ctx;
    prompt();
    char line[1024];
    char *p = line;
    p = em_fgets(line, 1024);
    if (!p)
    {
        printf("Error reading output");
        return;
    }
    // Normalize line endings (remove trailing \r\n or \n)
    line[strcspn(line, "\r\n")] = '\0';

    // Tokenize and process the input
    hex_interpret(ctx, line, "<repl>", 1, 1);
    // Print the top item of the stack
    if (ctx->stack.top >= 0)
    {
        hex_print_item(stdout, ctx->stack.entries[ctx->stack.top]);
        // hex_print_item(stdout, HEX_STACK[HEX_TOP]);
        printf("\n");
    }
    return;
}

#else

static int do_repl(void *v_ctx)
{
    hex_context_t *ctx = (hex_context_t *)v_ctx;
    char line[1024];
    prompt();
    if (fgets(line, sizeof(line), stdin) == NULL)
    {
        printf("\n"); // Handle EOF (Ctrl+D)
        return 1;
    }
    // Normalize line endings (remove trailing \r\n or \n)
    line[strcspn(line, "\r\n")] = '\0';

    // Tokenize and process the input
    hex_interpret(ctx, line, "<repl>", 1, 1);
    // Print the top item of the stack
    hex_print_item(stdout, ctx->stack.entries[ctx->stack.top]);
    printf("\n");
    return 0;
}

#endif

// REPL implementation
void hex_repl(hex_context_t *ctx)
{
#if defined(__EMSCRIPTEN__)
    printf("   _*_ _\n");
    printf("  / \\hex\\*\n");
    printf(" *\\_/_/_/  v%s - WASM Build\n", HEX_VERSION);
    printf("      *\n");
    int fps = 0;
    int simulate_infinite_loop = 1;
    emscripten_set_main_loop_arg(do_repl, ctx, fps, simulate_infinite_loop);
#else

    printf("   _*_ _\n");
    printf("  / \\hex\\*\n");
    printf(" *\\_/_/_/  v%s - Press Ctrl+C to exit.\n", HEX_VERSION);
    printf("      *\n");

    while (1)
    {
        if (do_repl(ctx) != 0)
        {
            exit(1);
        }
    }
#endif
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

void hex_print_help()
{
    printf("   _*_ _\n"
           "  / \\hex\\*\n"
           " *\\_/_/_/  v%s - (c) 2024 Fabio Cevasco\n"
           "      *      \n",
           HEX_VERSION);
    printf("\n"
           "USAGE\n"
           "  hex [options] [file]\n"
           "\n"
           "ARGUMENTS\n"
           "  file            A .hex file to interpret\n"
           "\n"
           "OPTIONS\n"
           "  -b, --bytecode  Generate bytecode file.\n"
           "  -d, --debug     Enable debug mode.\n"
           "  -h, --help      Display this help message.\n"
           "  -m, --manual    Display the manual.\n"
           "  -v, --version   Display hex version.\n\n");
}

void hex_print_docs(hex_doc_dictionary_t *docs)
{
    printf("\n"
           "   _*_ _\n"
           "  / \\hex\\*\n"
           " *\\_/_/_/  v%s - (c) 2024 Fabio Cevasco\n"
           "      *   \n",
           HEX_VERSION);
    printf("\n"
           "BASICS\n"
           "  hex is a minimalist, slightly-esoteric, concatenative programming language that supports\n"
           "  only integers, strings, symbols, and quotations (lists).\n"
           "\n"
           "  It uses a stack-based execution model and provides 64 native symbols for stack\n"
           "  manipulation, arithmetic operations, control flow, reading and writing\n"
           "  (standard input/output/error and files), executing external processes, and more.\n"
           "\n"
           "  Symbols and literals are separated by whitespace and can be grouped in quotations using\n"
           "  parentheses.\n"
           "\n"
           "  Symbols are evaluated only when they are pushed on the stack, therefore, symbols inside\n"
           "  quotations are not evaluated until the contents of the quotation are pushed on the stack.\n"
           "  You can define your own symbols using the symbol ':' and execute a quotation with '.'.\n"
           "\n"
           "  Oh, and of course all integers are in hexadecimal format! ;)\n"
           "\n"
           "SYMBOLS\n"
           "  +---------+----------------------------+-------------------------------------------------+\n"
           "  | Symbol  | Input -> Output            | Description                                     |\n"
           "  +---------+----------------------------+-------------------------------------------------+\n");
    for (size_t i = 0; i < docs->size; i++)
    {
        printf("  | ");
        hex_rpad(docs->entries[i].name, 7);
        printf(" | ");
        hex_lpad(docs->entries[i].input, 15);
        printf(" -> ");
        hex_rpad(docs->entries[i].output, 7);
        printf(" | ");
        hex_rpad(docs->entries[i].description, 47);
        printf(" |\n");
    }
    printf("  +---------+----------------------------+-------------------------------------------------+\n");
}

int hex_write_bytecode_file(hex_context_t *ctx, char *filename, uint8_t *bytecode, size_t size)
{
    FILE *file = fopen(filename, "wb");
    if (file == NULL)
    {
        hex_error(ctx, "Failed to write file: %s", filename);
        return 1;
    }
    hex_debug(ctx, "Writing bytecode to file: %s", filename);
    fwrite(HEX_BYTECODE_HEADER, 1, 6, file);
    fwrite(bytecode, 1, size, file);
    fclose(file);
    hex_debug(ctx, "Bytecode file written: %s", filename);
    return 0;
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
    hex_create_docs(&ctx.docs);

    char *file;
    int generate_bytecode = 0;

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
            else if ((strcmp(arg, "-h") == 0 || strcmp(arg, "--help") == 0))
            {
                hex_print_help();
                return 0;
            }
            else if ((strcmp(arg, "-m") == 0 || strcmp(arg, "--manual") == 0))
            {
                hex_print_docs(&ctx.docs);
                return 0;
            }
            else if ((strcmp(arg, "-d") == 0 || strcmp(arg, "--debug") == 0))
            {
                ctx.settings.debugging_enabled = 1;
                printf("*** Debug mode enabled ***\n");
            }
            else if ((strcmp(arg, "-b") == 0 || strcmp(arg, "--bytecode") == 0))
            {
                generate_bytecode = 1;
            }
            else
            {
                file = arg;
            }
        }
        if (file)
        {
            if (strstr(file, ".hbx") != NULL)
            {
                FILE *bytecode_file = fopen(file, "rb");
                if (bytecode_file == NULL)
                {
                    hex_error(&ctx, "Failed to open bytecode file: %s", file);
                    return 1;
                }
                fseek(bytecode_file, 0, SEEK_END);
                size_t bytecode_size = ftell(bytecode_file);
                fseek(bytecode_file, 0, SEEK_SET);
                uint8_t *bytecode = (uint8_t *)malloc(bytecode_size);
                if (bytecode == NULL)
                {
                    hex_error(&ctx, "Memory allocation failed");
                    fclose(bytecode_file);
                    return 1;
                }
                fread(bytecode, 1, bytecode_size, bytecode_file);
                fclose(bytecode_file);
                hex_interpret_bytecode(&ctx, bytecode, bytecode_size);
                free(bytecode);
            }
            else
            {
                char *fileContent = hex_read_file(&ctx, file);
                if (generate_bytecode)
                {
                    uint8_t *bytecode;
                    size_t bytecode_size = 0;
                    hex_file_position_t position;
                    position.column = 1;
                    position.line = 1 + ctx.hashbang;
                    position.filename = file;
                    char *bytecode_file = strdup(file);
                    char *ext = strrchr(bytecode_file, '.');
                    if (ext != NULL)
                    {
                        strcpy(ext, ".hbx");
                    }
                    else
                    {
                        strcat(bytecode_file, ".hbx");
                    }
                    if (hex_bytecode(&ctx, fileContent, &bytecode, &bytecode_size, &position) != 0)
                    {
                        hex_error(&ctx, "Failed to generate bytecode");
                        return 1;
                    }
                    if (hex_write_bytecode_file(&ctx, bytecode_file, bytecode, bytecode_size) != 0)
                    {
                        return 1;
                    }
                }
                else
                {
                    hex_interpret(&ctx, fileContent, file, 1 + ctx.hashbang, 1);
                }
            }
            return 0;
        }
    }
#if !(__EMSCRIPTEN__)
    if (!isatty(fileno(stdin)))
    {
        ctx.settings.stack_trace_enabled = 0;
        // Process piped input from stdin
        hex_process_stdin(&ctx);
    }
#endif
    else
    {
        ctx.settings.stack_trace_enabled = 0;
        // Start REPL
        hex_repl(&ctx);
    }

    return 0;
}

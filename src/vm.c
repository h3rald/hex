#ifndef HEX_H
#include "hex.h"
#endif

////////////////////////////////////////
// Virtual Machine                    //
////////////////////////////////////////

static void encode_length(uint8_t **bytecode, size_t *size, size_t *capacity, size_t length)
{
    if (length < 0x80)
    {
        (*bytecode)[(*size)++] = (uint8_t)length;
    }
    else if (length < 0x8000)
    {
        (*bytecode)[(*size)++] = 0x80;
        if (*size + 2 > *capacity)
        {
            *capacity *= 2;
            *bytecode = (uint8_t *)realloc(*bytecode, *capacity);
        }
        (*bytecode)[(*size)++] = (uint8_t)(length >> 8);
        (*bytecode)[(*size)++] = (uint8_t)(length & 0xFF);
    }
    else
    {
        (*bytecode)[(*size)++] = 0x81;
        if (*size + 4 > *capacity)
        {
            *capacity *= 2;
            *bytecode = (uint8_t *)realloc(*bytecode, *capacity);
        }
        (*bytecode)[(*size)++] = (uint8_t)(length >> 24);
        (*bytecode)[(*size)++] = (uint8_t)((length >> 16) & 0xFF);
        (*bytecode)[(*size)++] = (uint8_t)((length >> 8) & 0xFF);
        (*bytecode)[(*size)++] = (uint8_t)(length & 0xFF);
    }
}

static uint8_t get_opcode(char *symbol)
{
    // Native Symbols
    if (strcmp(symbol, ":") == 0)
    {
        return HEX_OP_STORE;
    }
    else if (strcmp(symbol, "#") == 0)
    {
        return HEX_OP_FREE;
    }
    else if (strcmp(symbol, "if") == 0)
    {
        return HEX_OP_IF;
    }
    else if (strcmp(symbol, "when") == 0)
    {
        return HEX_OP_WHEN;
    }
    else if (strcmp(symbol, "while") == 0)
    {
        return HEX_OP_WHILE;
    }
    else if (strcmp(symbol, "error") == 0)
    {
        return HEX_OP_ERROR;
    }
    else if (strcmp(symbol, "try") == 0)
    {
        return HEX_OP_TRY;
    }
    else if (strcmp(symbol, "dup") == 0)
    {
        return HEX_OP_DUP;
    }
    else if (strcmp(symbol, "stack") == 0)
    {
        return HEX_OP_STACK;
    }
    else if (strcmp(symbol, "clear") == 0)
    {
        return HEX_OP_CLEAR;
    }
    else if (strcmp(symbol, "pop") == 0)
    {
        return HEX_OP_POP;
    }
    else if (strcmp(symbol, "swap") == 0)
    {
        return HEX_OP_SWAP;
    }
    else if (strcmp(symbol, ".") == 0)
    {
        return HEX_OP_I;
    }
    else if (strcmp(symbol, "!") == 0)
    {
        return HEX_OP_EVAL;
    }
    else if (strcmp(symbol, "'") == 0)
    {
        return HEX_OP_QUOTE;
    }
    else if (strcmp(symbol, "+") == 0)
    {
        return HEX_OP_ADD;
    }
    else if (strcmp(symbol, "-") == 0)
    {
        return HEX_OP_SUB;
    }
    else if (strcmp(symbol, "*") == 0)
    {
        return HEX_OP_MUL;
    }
    else if (strcmp(symbol, "/") == 0)
    {
        return HEX_OP_DIV;
    }
    else if (strcmp(symbol, "%") == 0)
    {
        return HEX_OP_MOD;
    }
    else if (strcmp(symbol, "&") == 0)
    {
        return HEX_OP_BITAND;
    }
    else if (strcmp(symbol, "|") == 0)
    {
        return HEX_OP_BITOR;
    }
    else if (strcmp(symbol, "^") == 0)
    {
        return HEX_OP_BITXOR;
    }
    else if (strcmp(symbol, "~") == 0)
    {
        return HEX_OP_BITNOT;
    }
    else if (strcmp(symbol, "<<") == 0)
    {
        return HEX_OP_SHL;
    }
    else if (strcmp(symbol, ">>") == 0)
    {
        return HEX_OP_SHR;
    }
    else if (strcmp(symbol, "==") == 0)
    {
        return HEX_OP_EQUAL;
    }
    else if (strcmp(symbol, "!=") == 0)
    {
        return HEX_OP_NOTEQUAL;
    }
    else if (strcmp(symbol, ">") == 0)
    {
        return HEX_OP_GREATER;
    }
    else if (strcmp(symbol, "<") == 0)
    {
        return HEX_OP_LESS;
    }
    else if (strcmp(symbol, ">=") == 0)
    {
        return HEX_OP_GREATEREQUAL;
    }
    else if (strcmp(symbol, "<=") == 0)
    {
        return HEX_OP_LESSEQUAL;
    }
    else if (strcmp(symbol, "and") == 0)
    {
        return HEX_OP_AND;
    }
    else if (strcmp(symbol, "or") == 0)
    {
        return HEX_OP_OR;
    }
    else if (strcmp(symbol, "not") == 0)
    {
        return HEX_OP_NOT;
    }
    else if (strcmp(symbol, "xor") == 0)
    {
        return HEX_OP_XOR;
    }
    else if (strcmp(symbol, "int") == 0)
    {
        return HEX_OP_INT;
    }
    else if (strcmp(symbol, "str") == 0)
    {
        return HEX_OP_STR;
    }
    else if (strcmp(symbol, "dec") == 0)
    {
        return HEX_OP_DEC;
    }
    else if (strcmp(symbol, "hex") == 0)
    {
        return HEX_OP_HEX;
    }
    else if (strcmp(symbol, "ord") == 0)
    {
        return HEX_OP_ORD;
    }
    else if (strcmp(symbol, "chr") == 0)
    {
        return HEX_OP_CHR;
    }
    else if (strcmp(symbol, "type") == 0)
    {
        return HEX_OP_TYPE;
    }
    else if (strcmp(symbol, "cat") == 0)
    {
        return HEX_OP_CAT;
    }
    else if (strcmp(symbol, "len") == 0)
    {
        return HEX_OP_LEN;
    }
    else if (strcmp(symbol, "get") == 0)
    {
        return HEX_OP_GET;
    }
    else if (strcmp(symbol, "index") == 0)
    {
        return HEX_OP_INDEX;
    }
    else if (strcmp(symbol, "join") == 0)
    {
        return HEX_OP_JOIN;
    }
    else if (strcmp(symbol, "split") == 0)
    {
        return HEX_OP_SPLIT;
    }
    else if (strcmp(symbol, "replace") == 0)
    {
        return HEX_OP_REPLACE;
    }
    else if (strcmp(symbol, "each") == 0)
    {
        return HEX_OP_EACH;
    }
    else if (strcmp(symbol, "map") == 0)
    {
        return HEX_OP_MAP;
    }
    else if (strcmp(symbol, "filter") == 0)
    {
        return HEX_OP_FILTER;
    }
    else if (strcmp(symbol, "puts") == 0)
    {
        return HEX_OP_PUTS;
    }
    else if (strcmp(symbol, "warn") == 0)
    {
        return HEX_OP_WARN;
    }
    else if (strcmp(symbol, "print") == 0)
    {
        return HEX_OP_PRINT;
    }
    else if (strcmp(symbol, "gets") == 0)
    {
        return HEX_OP_GETS;
    }
    else if (strcmp(symbol, "read") == 0)
    {
        return HEX_OP_READ;
    }
    else if (strcmp(symbol, "write") == 0)
    {
        return HEX_OP_WRITE;
    }
    else if (strcmp(symbol, "append") == 0)
    {
        return HEX_OP_APPEND;
    }
    else if (strcmp(symbol, "args") == 0)
    {
        return HEX_OP_ARGS;
    }
    else if (strcmp(symbol, "exit") == 0)
    {
        return HEX_OP_EXIT;
    }
    else if (strcmp(symbol, "exec") == 0)
    {
        return HEX_OP_EXEC;
    }
    else if (strcmp(symbol, "run") == 0)
    {
        return HEX_OP_RUN;
    }
    return 0;
}

int hex_bytecode(hex_context_t *ctx, const char *input, uint8_t **output, size_t *output_size, hex_file_position_t *position, int *open_quotations)
{
    hex_token_t *token;
    size_t capacity = 128;
    size_t size = 0;
    uint8_t *bytecode = (uint8_t *)malloc(capacity);
    if (!bytecode)
    {
        hex_error(ctx, "Memory allocation failed");
        return 1;
    }
    printf("-------\n");
    printf("%s", input);
    printf("-------\n");
    while ((token = hex_next_token(ctx, &input, position)) != NULL)
    {
        if (size >= capacity)
        {
            capacity *= 2;
            bytecode = (uint8_t *)realloc(bytecode, capacity);
            if (!bytecode)
            {
                hex_error(ctx, "Memory allocation failed");
                return 1;
            }
        }

        switch (token->type)
        {
        case HEX_TOKEN_INTEGER:
            bytecode[size++] = HEX_OP_PUSHIN;
            int32_t value = hex_parse_integer(token->data.value);
            encode_length(&bytecode, &size, &capacity, sizeof(int32_t));
            memcpy(&bytecode[size], &value, sizeof(int32_t));
            size += sizeof(int32_t);
            hex_debug(ctx, "PUSHIN[%d]: %d", sizeof(int32_t), value);
            break;

        case HEX_TOKEN_STRING:
            bytecode[size++] = HEX_OP_PUSHST;
            size_t len = strlen(token->data.value);
            encode_length(&bytecode, &size, &capacity, len);
            memcpy(&bytecode[size], token->data.value, len);
            hex_debug(ctx, "PUSHST[%d]: %s", len, token->data.value);
            size += len;
            break;
        case HEX_TOKEN_SYMBOL:
            if (hex_valid_native_symbol(ctx, token->data.value))
            {
                char *symbol = token->data.value;
                bytecode[size++] = get_opcode(symbol);
                hex_debug(ctx, "NATSYM[1]: %s", symbol);
            }
            else
            {
                // Lookup user symbol
                bytecode[size++] = HEX_OP_LOOKUP;
                size_t sym_len = strlen(token->data.value);
                encode_length(&bytecode, &size, &capacity, sym_len);
                memcpy(&bytecode[size], token->data.value, sym_len);
                size += sym_len;
                hex_debug(ctx, "LOOKUP[%d]: %s", sym_len, token->data.value);
            }
            break;
        case HEX_TOKEN_QUOTATION_START:
        {
            bytecode[size++] = HEX_OP_PUSHQT;
            hex_token_t quotation;
            hex_tokenize_quotation(ctx, &input, &quotation, position);
            hex_debug(ctx, "PUSHQT[%d]: <start>", quotation.quotation_size);
            encode_length(&bytecode, &size, &capacity, quotation.quotation_size);
            // TODO:  Incorrect, here we already tokenized the quotation input.
            for (size_t i = 0; i < quotation.quotation_size; i++)
            {
                if (hex_bytecode(ctx, input, output, output_size, position, open_quotations) != 0)
                {
                    hex_error(ctx, "Failed to generate bytecode for quotation");
                    hex_free_token(token);
                    free(bytecode);
                    return 1;
                }
            }
            hex_debug(ctx, "PUSHQT[%d]: <end>", quotation.quotation_size);
            break;
        }
        case HEX_TOKEN_QUOTATION_END:
            open_quotations--;
            break;
        default:
            // Ignore other tokens
            break;
        }
        hex_free_token(token);
    }
    *output = bytecode;
    *output_size = size;
    return 0;
}

int hex_tokenize_quotation(hex_context_t *ctx, const char **input, hex_token_t *result, hex_file_position_t *position)
{
    size_t capacity = 2;
    size_t size = 0;
    hex_token_t **quotation = (hex_token_t **)malloc(capacity * sizeof(hex_token_t *));
    int balanced = 1;
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
            quotation = (hex_token_t **)realloc(quotation, capacity * sizeof(hex_token_t *));
            if (!quotation)
            {
                hex_error(ctx, "(%d,%d), Memory allocation failed", position->line, position->column);
                return 1;
            }
        }

        if (token->type == HEX_TOKEN_INTEGER || token->type == HEX_TOKEN_STRING)
        {

            quotation[size] = token;
            size++;
        }
        else if (token->type == HEX_TOKEN_SYMBOL)
        {
            token->position.filename = strdup(position->filename);
            quotation[size] = token;
            size++;
        }
        else if (token->type == HEX_TOKEN_QUOTATION_START)
        {
            hex_token_t nested_result;
            if (hex_tokenize_quotation(ctx, input, &nested_result, position) != 0)
            {
                hex_free_token(token);
                return 1;
            }
            quotation[size]->data.quotation_value = nested_result.data.quotation_value;
            quotation[size]->quotation_size = nested_result.quotation_size;
            size++;
        }
        else if (token->type == HEX_TOKEN_COMMENT)
        {
            // Ignore comments
        }
        else
        {
            hex_error(ctx, "(%d,%d) Unexpected token in quotation: %d", position->line, position->column, token->data.value);
            hex_free_token(token);
            return 1;
        }
    }

    if (balanced != 0)
    {
        hex_error(ctx, "(%d,%d) Unterminated quotation", position->line, position->column);
        hex_free_token(token);
        return 1;
    }

    result->data.quotation_value = quotation;
    result->quotation_size = size;
    return 0;
}

/*
int hex_quotation_bytecode(hex_context_t *ctx, hex_item_t *quotation, const char *input, hex_file_position_t *position, uint8_t **bytecode, size_t *size, size_t *capacity)
{
    for (size_t i = 0; i < quotation->quotation_size; ++i)
    {
        hex_item_t *item = quotation->data.quotation_value[i];
        switch (item->type)
        {
        case HEX_TYPE_INTEGER:
            (*bytecode)[(*size)++] = HEX_OP_PUSHIN;
            encode_length(bytecode, size, capacity, sizeof(int32_t));
            memcpy(&(*bytecode)[*size], &item->data.int_value, sizeof(int32_t));
            *size += sizeof(int32_t);
            hex_debug(ctx, "PUSHIN[%d]: %d", sizeof(int32_t), item->data.int_value);
            break;
        case HEX_TYPE_STRING:
            (*bytecode)[(*size)++] = HEX_OP_PUSHST;
            size_t len = strlen(item->data.str_value);
            encode_length(bytecode, size, capacity, len);
            memcpy(&(*bytecode)[*size], item->data.str_value, len);
            *size += len;
            hex_debug(ctx, "PUSHST[%d]: %s", len, item->data.str_value);
            break;
        case HEX_TYPE_NATIVE_SYMBOL:
            char *symbol = item->token->data.value;
            (*bytecode)[(*size)++] = get_opcode(symbol);
            hex_debug(ctx, "NATSYM[1]: %s", symbol);
            break;
        case HEX_TYPE_USER_SYMBOL:
            // Lookup user symbol
            (*bytecode)[(*size)++] = HEX_OP_LOOKUP;
            size_t sym_len = strlen(item->token->data.value);
            encode_length(bytecode, size, capacity, sym_len);
            memcpy(&(*bytecode)[*size], item->token->data.value, sym_len);
            *size += sym_len;
            hex_debug(ctx, "LOOKUP[%d]: %s", sym_len, item->token->data.value);
            break;
        case HEX_TYPE_QUOTATION:
            (*bytecode)[(*size)++] = HEX_OP_PUSHQT;
            hex_item_t *quotation = item;
            hex_debug(ctx, "PUSHQT[%d]: <start>", quotation->quotation_size);
            encode_length(bytecode, size, capacity, quotation->quotation_size);
            if (hex_quotation_bytecode(ctx, quotation, input, position, bytecode, size, capacity) != 0)
            {
                hex_error(ctx, "Failed to generate bytecode for quotation");
                return 1;
            }
            hex_debug(ctx, "PUSHQT[%d]: <end>", quotation->quotation_size);
            break;
        default:
            // Ignore other types
            // Review in case of invalid items
            break;
        }
    }
    return 0;
}
*/

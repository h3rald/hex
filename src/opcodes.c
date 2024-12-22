#ifndef HEX_H
#include "hex.h"
#endif

uint8_t hex_symbol_to_opcode(const char *symbol)
{
    // Native Symbols
    if (strcmp(symbol, ":") == 0)
    {
        return HEX_OP_STORE;
    }
    else if (strcmp(symbol, "::") == 0)
    {
        return HEX_OP_DEFINE;
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

const char *hex_opcode_to_symbol(uint8_t opcode)
{
    switch (opcode)
    {
    case HEX_OP_STORE:
        return ":";
    case HEX_OP_DEFINE:
        return "::";
    case HEX_OP_FREE:
        return "#";
    case HEX_OP_IF:
        return "if";
    case HEX_OP_WHEN:
        return "when";
    case HEX_OP_WHILE:
        return "while";
    case HEX_OP_ERROR:
        return "error";
    case HEX_OP_TRY:
        return "try";
    case HEX_OP_DUP:
        return "dup";
    case HEX_OP_STACK:
        return "stack";
    case HEX_OP_CLEAR:
        return "clear";
    case HEX_OP_POP:
        return "pop";
    case HEX_OP_SWAP:
        return "swap";
    case HEX_OP_I:
        return ".";
    case HEX_OP_EVAL:
        return "!";
    case HEX_OP_QUOTE:
        return "'";
    case HEX_OP_ADD:
        return "+";
    case HEX_OP_SUB:
        return "-";
    case HEX_OP_MUL:
        return "*";
    case HEX_OP_DIV:
        return "/";
    case HEX_OP_MOD:
        return "%";
    case HEX_OP_BITAND:
        return "&";
    case HEX_OP_BITOR:
        return "|";
    case HEX_OP_BITXOR:
        return "^";
    case HEX_OP_BITNOT:
        return "~";
    case HEX_OP_SHL:
        return "<<";
    case HEX_OP_SHR:
        return ">>";
    case HEX_OP_EQUAL:
        return "==";
    case HEX_OP_NOTEQUAL:
        return "!=";
    case HEX_OP_GREATER:
        return ">";
    case HEX_OP_LESS:
        return "<";
    case HEX_OP_GREATEREQUAL:
        return ">=";
    case HEX_OP_LESSEQUAL:
        return "<=";
    case HEX_OP_AND:
        return "and";
    case HEX_OP_OR:
        return "or";
    case HEX_OP_NOT:
        return "not";
    case HEX_OP_XOR:
        return "xor";
    case HEX_OP_INT:
        return "int";
    case HEX_OP_STR:
        return "str";
    case HEX_OP_DEC:
        return "dec";
    case HEX_OP_HEX:
        return "hex";
    case HEX_OP_ORD:
        return "ord";
    case HEX_OP_CHR:
        return "chr";
    case HEX_OP_TYPE:
        return "type";
    case HEX_OP_CAT:
        return "cat";
    case HEX_OP_LEN:
        return "len";
    case HEX_OP_GET:
        return "get";
    case HEX_OP_INDEX:
        return "index";
    case HEX_OP_JOIN:
        return "join";
    case HEX_OP_SPLIT:
        return "split";
    case HEX_OP_REPLACE:
        return "replace";
    case HEX_OP_EACH:
        return "each";
    case HEX_OP_MAP:
        return "map";
    case HEX_OP_FILTER:
        return "filter";
    case HEX_OP_PUTS:
        return "puts";
    case HEX_OP_WARN:
        return "warn";
    case HEX_OP_PRINT:
        return "print";
    case HEX_OP_GETS:
        return "gets";
    case HEX_OP_READ:
        return "read";
    case HEX_OP_WRITE:
        return "write";
    case HEX_OP_APPEND:
        return "append";
    case HEX_OP_ARGS:
        return "args";
    case HEX_OP_EXIT:
        return "exit";
    case HEX_OP_EXEC:
        return "exec";
    case HEX_OP_RUN:
        return "run";
    default:
        return NULL;
    }
}

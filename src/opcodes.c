#ifndef HEX_H
#include "hex.h"
#endif

// Opcodes
typedef enum hex_opcode_t
{
    // Core Operations: <op> [prefix] <len> <data>
    HEX_OP_LOOKUP = 0x00,
    HEX_OP_PUSHIN = 0x01,
    HEX_OP_PUSHST = 0x02,
    HEX_OP_PUSHQT = 0x03,

    // Native Symbols
    HEX_OP_STORE = 0x10,
    HEX_OP_FREE = 0x11,

    HEX_OP_IF = 0x12,
    HEX_OP_WHEN = 0x13,
    HEX_OP_WHILE = 0x14,
    HEX_OP_ERROR = 0x15,
    HEX_OP_TRY = 0x16,

    HEX_OP_DUP = 0x17,
    HEX_OP_STACK = 0x18,
    HEX_OP_CLEAR = 0x19,
    HEX_OP_POP = 0x1A,
    HEX_OP_SWAP = 0x1B,

    HEX_OP_I = 0x1C,
    HEX_OP_EVAL = 0x1D,
    HEX_OP_QUOTE = 0x1E,

    HEX_OP_ADD = 0x1F,
    HEX_OP_SUB = 0x20,
    HEX_OP_MUL = 0x21,
    HEX_OP_DIV = 0x22,
    HEX_OP_MOD = 0x23,

    HEX_OP_BITAND = 0x24,
    HEX_OP_BITOR = 0x25,
    HEX_OP_BITXOR = 0x26,
    HEX_OP_BITNOT = 0x27,
    HEX_OP_SHL = 0x28,
    HEX_OP_SHR = 0x29,

    HEX_OP_EQUAL = 0x2A,
    HEX_OP_NOTEQUAL = 0x2B,
    HEX_OP_GREATER = 0x2C,
    HEX_OP_LESS = 0x2D,
    HEX_OP_GREATEREQUAL = 0x2E,
    HEX_OP_LESSEQUAL = 0x2F,

    HEX_OP_AND = 0x30,
    HEX_OP_OR = 0x31,
    HEX_OP_NOT = 0x32,
    HEX_OP_XOR = 0x33,

    HEX_OP_INT = 0x34,
    HEX_OP_STR = 0x35,
    HEX_OP_DEC = 0x36,
    HEX_OP_HEX = 0x37,
    HEX_OP_ORD = 0x38,
    HEX_OP_CHR = 0x39,
    HEX_OP_TYPE = 0x3A,

    HEX_OP_CAT = 0x3B,
    HEX_OP_LEN = 0x3C,
    HEX_OP_GET = 0x3D,
    HEX_OP_INDEX = 0x3E,
    HEX_OP_JOIN = 0x3F,

    HEX_OP_SPLIT = 0x40,
    HEX_OP_REPLACE = 0x41,

    HEX_OP_EACH = 0x42,
    HEX_OP_MAP = 0x43,
    HEX_OP_FILTER = 0x44,

    HEX_OP_PUTS = 0x45,
    HEX_OP_WARN = 0x46,
    HEX_OP_PRINT = 0x47,
    HEX_OP_GETS = 0x48,

    HEX_OP_READ = 0x49,
    HEX_OP_WRITE = 0x4A,
    HEX_OP_APPEND = 0x4B,
    HEX_OP_ARGS = 0x4C,
    HEX_OP_EXIT = 0x4D,
    HEX_OP_EXEC = 0x4E,
    HEX_OP_RUN = 0x4F,

} hex_opcode_t;

uint8_t hex_symbol_to_opcode(const char *symbol)
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

const char *hex_opcode_to_symbol(uint8_t opcode)
{
    switch (opcode)
    {
    case HEX_OP_STORE:
        return ":";
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

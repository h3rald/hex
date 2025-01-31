#ifndef HEX_H
#include "hex.h"
#endif

////////////////////////////////////////
// Help System                        //
////////////////////////////////////////

void hex_set_doc(hex_doc_dictionary_t *dict, const char *name, const char *input, const char *output, const char *description)
{
    hex_doc_entry_t *doc = malloc(sizeof(hex_doc_entry_t));
    if (doc == NULL)
    {
        return;
    }
    doc->name = name;
    doc->description = description;
    doc->input = input;
    doc->output = output;
    dict->entries[dict->size] = doc;
    dict->size++;
}

int hex_get_doc(hex_doc_dictionary_t *docs, const char *key, hex_doc_entry_t *result)
{
    for (size_t i = 0; i < docs->size; i++)
    {
        if (strcmp(docs->entries[i]->name, key) == 0)
        {
            *result = *(docs->entries[i]);
            return 1;
        }
    }
    return 0;
}

void hex_create_docs(hex_doc_dictionary_t *docs)
{
    // Memory
    hex_set_doc(docs, ":", "a s", "", "Stores 'a' as the literal symbol 's'.");
    hex_set_doc(docs, "::", "a s", "", "Defines 'a' as the operator symbol 's'.");
    hex_set_doc(docs, "#", "s", "", "Deletes user symbol 's'.");
    hex_set_doc(docs, "symbols", "", "q", "Pushes a quotation containing all registered symbols on the stack.");

    // Control flow
    hex_set_doc(docs, "if", "q q q", "*", "If 'q1' is not 0x0, executes 'q2', else 'q3'.");
    hex_set_doc(docs, "while", "q1 q2", "*", "While 'q1' is not 0x0, executes 'q2'.");
    hex_set_doc(docs, "error", "", "s", "Returns the last error message.");
    hex_set_doc(docs, "try", "q1 q2", "*", "If 'q1' fails, executes 'q2'.");
    hex_set_doc(docs, "throw", "s", "", "Throws error 's'.");

    // Stack
    hex_set_doc(docs, "dup", "a", "a a", "Duplicates 'a'.");
    hex_set_doc(docs, "pop", "a", "", "Removes the top item from the stack.");
    hex_set_doc(docs, "swap", "a1 a2", "a2 a1", "Swaps 'a2' with 'a1'.");
    hex_set_doc(docs, "stack", "", "q", "Returns the contents of the stack.");

    // Evaluation
    hex_set_doc(docs, ".", "q", "*", "Pushes each item of 'q' on the stack.");
    hex_set_doc(docs, "!", "(s|q)", "*", "Evaluates 's' as a hex program or 'q' as hex bytecode.");
    hex_set_doc(docs, "'", "a", "q", "Wraps 'a' in a quotation.");
    hex_set_doc(docs, "debug", "q", "*", "Enables debug mode and pushes each item of 'q' on the stack.");

    // Arithmetic
    hex_set_doc(docs, "+", "i1 12", "i", "Adds two integers.");
    hex_set_doc(docs, "-", "i1 12", "i", "Subtracts 'i2' from 'i1'.");
    hex_set_doc(docs, "*", "i1 12", "i", "Multiplies two integers.");
    hex_set_doc(docs, "/", "i1 12", "i", "Divides 'i1' by 'i2'.");
    hex_set_doc(docs, "%", "i1 12", "i", "Calculates the modulo of 'i1' divided by 'i2'.");

    // Bitwise
    hex_set_doc(docs, "&", "i1 12", "i", "Calculates the bitwise AND of two integers.");
    hex_set_doc(docs, "|", "i1 12", "i", "Calculates the bitwise OR of two integers.");
    hex_set_doc(docs, "^", "i1 12", "i", "Calculates the bitwise XOR of two integers.");
    hex_set_doc(docs, "~", "i", "i", "Calculates the bitwise NOT of an integer.");
    hex_set_doc(docs, "<<", "i1 12", "i", "Shifts 'i1' by 'i2' bytes to the left.");
    hex_set_doc(docs, ">>", "i1 12", "i", "Shifts 'i1' by 'i2' bytes to the right.");

    // Comparison
    hex_set_doc(docs, "==", "a1 a2", "i", "Returns 0x1 if 'a1' == 'a2', 0x0 otherwise.");
    hex_set_doc(docs, "!=", "a1 a2", "i", "Returns 0x1 if 'a1' != 'a2', 0x0 otherwise.");
    hex_set_doc(docs, ">", "a1 a2", "i", "Returns 0x1 if 'a1' > 'a2', 0x0 otherwise.");
    hex_set_doc(docs, "<", "a1 a2", "i", "Returns 0x1 if 'a1' < 'a2', 0x0 otherwise.");
    hex_set_doc(docs, ">=", "a1 a2", "i", "Returns 0x1 if 'a1' >= 'a2', 0x0 otherwise.");
    hex_set_doc(docs, "<=", "a1 a2", "i", "Returns 0x1 if 'a1' <= 'a2', 0x0 otherwise.");

    // Logical
    hex_set_doc(docs, "and", "i1 i2", "i", "Returns 0x1 if both 'i1' and 'i2' are not 0x0.");
    hex_set_doc(docs, "or", "i1 i2", "i", "Returns 0x1 if either 'i1' or 'i2' are not 0x0.");
    hex_set_doc(docs, "not", "i", "i", "Returns 0x1 if 'i' is 0x0, 0x0 otherwise.");
    hex_set_doc(docs, "xor", "i1 i2", "i", "Returns 0x1 if only one item is not 0x0.");

    // Type
    hex_set_doc(docs, "int", "s", "i", "Converts a string to a hex integer.");
    hex_set_doc(docs, "str", "i", "s", "Converts a hex integer to a string.");
    hex_set_doc(docs, "dec", "i", "s", "Converts a hex integer to a decimal string.");
    hex_set_doc(docs, "hex", "s", "i", "Converter a decimal string to a hex integer.");
    hex_set_doc(docs, "chr", "i", "s", "Converts an integer to a single-character.");
    hex_set_doc(docs, "ord", "s", "i", "Converts a single-character to an integer.");
    hex_set_doc(docs, "type", "a", "s", "Pushes the data type of 'a' on the stack.");

    // List
    hex_set_doc(docs, "cat", "(s1 s2|q1 q2) ", "(s3|q3)", "Concatenates two quotations or two strings.");
    hex_set_doc(docs, "len", "(s|q)", "i ", "Returns the length of 's' or 'q'.");
    hex_set_doc(docs, "get", "(s|q)", "a", "Gets the item at position 'i' in 's' or 'q'.");
    hex_set_doc(docs, "index", "(s a|q a)", "i", "Returns the index of 'a' within 's' or 'q'.");
    hex_set_doc(docs, "join", "q s", "s", "Joins the strings in 'q' using separator 's'.");
    hex_set_doc(docs, "split", "s1 s2", "q", "Splits 's1' using separator 's2'.");
    hex_set_doc(docs, "replace", "s1 s2 s3", "s", "Replaces 's2' with 's3' within 's1'.");
    hex_set_doc(docs, "map", "q1 q2", "q3", "Applies 'q2' to 'q1' items and returns results.");

    // I/O
    hex_set_doc(docs, "puts", "a", "", "Prints 'a' and a new line to standard output.");
    hex_set_doc(docs, "warn", "a", "", "Prints 'a' and a new line to standard error.");
    hex_set_doc(docs, "print", "a", "", "Prints 'a' to standard output.");
    hex_set_doc(docs, "gets", "", "s", "Gets a string from standard input.");

    // File
    hex_set_doc(docs, "read", "s1", "(s2|q)", "Returns the contents of the specified file.");
    hex_set_doc(docs, "write", "(s1|q) s2", "", "Writes 's1' or 'q' to the file 's2'.");
    hex_set_doc(docs, "append", "(s1|q) s2", "", "Appends 's1' or 'q' to the file 's2'.");

    // Shell
    hex_set_doc(docs, "args", "", "q", "Returns the program arguments.");
    hex_set_doc(docs, "exit", "i", "", "Exits the program with exit code 'i'.");
    hex_set_doc(docs, "exec", "s", "i", "Executes the command 's'.");
    hex_set_doc(docs, "run", "s", "q", "Executes 's' and returns code, stdout, stderr.");
}

#!/bin/bash

# Files to combine
header_file="src/hex.h"
source_files=(
    "src/stack.c" 
    "src/registry.c" 
    "src/error.c" 
    "src/help.c" 
    "src/stacktrace.c" 
    "src/parser.c" 
    "src/symboltable.c"
    "src/opcodes.c"
    "src/vm.c"
    "src/interpreter.c" 
    "src/helpers.c" 
    "src/symbols.c" 
    "src/main.c"
)
output_file="src/hex.c"

# Start with a clean output file
echo "/* *** hex amalgamation *** */" > "$output_file"

# Add the header file with a #line directive
echo "/* File: $header_file */" >> "$output_file"
echo "#line 1 \"$header_file\"" >> "$output_file"
cat "$header_file" >> "$output_file"
echo "" >> "$output_file"

# Add each source file with #line directives
for file in "${source_files[@]}"; do
    echo "/* File: $file */" >> "$output_file"
    echo "#line 1 \"$file\"" >> "$output_file"
    cat "$file" >> "$output_file"
    echo "" >> "$output_file"
done

echo "Amalgamation file created: $output_file"

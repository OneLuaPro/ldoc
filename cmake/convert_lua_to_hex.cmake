# cmake/convert_lua_to_hex.cmake
# Essentially reads ldoc.lua (the main script) and converts it into a C-header
# file for inclusion in ldoc.cpp. The contents of the generated header are pure
# hex values.

# Read the file content
file(READ "${INPUT_FILE}" CONTENT)

# Remove Shebang line (starts with #!) if it exists to prevent Lua syntax errors
string(REGEX REPLACE "^#![^\n]*\n" "" CLEAN_CONTENT "${CONTENT}")

# Convert cleaned content to hex
file(WRITE "${OUTPUT_FILE}.tmp" "${CLEAN_CONTENT}")
file(READ "${OUTPUT_FILE}.tmp" RAW_HEX HEX)
file(REMOVE "${OUTPUT_FILE}.tmp")

string(REGEX REPLACE "([0-9a-f][0-9a-f])" "0x\\1," FORMATTED_HEX "${RAW_HEX}")

file(WRITE "${OUTPUT_FILE}" 
"#pragma once\n\n"
"static const unsigned char ldoc_source_bytes[] = {\n${FORMATTED_HEX}\n0x00\n};\n\n"
"static const unsigned int ldoc_source_size = sizeof(ldoc_source_bytes) - 1;\n"
)

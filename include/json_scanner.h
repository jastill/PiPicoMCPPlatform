#pragma once
#include "mcp_request.h"
#include <cstddef>

// ─────────────────────────────────────────────────────────────────────────────
// JsonScanner — extracts MCP JSON-RPC request fields from a raw JSON string.
//
// Does NOT build a parse tree or use heap memory.  Makes a single linear pass
// over the input, descending at most two levels into nested objects (params
// and params.arguments).  Unknown keys at every level are skipped via
// skip_value() which correctly handles arbitrary JSON nesting.
//
// Returns true if the minimum required fields (jsonrpc, method) were found.
// ─────────────────────────────────────────────────────────────────────────────
class JsonScanner {
public:
    bool scan(const char* json, size_t len, McpRequest& out);

private:
    // ── Low-level character-level helpers ─────────────────────────────────────

    // Advance past whitespace characters.
    const char* skip_ws(const char* p, const char* end);

    // Skip any single JSON value (string, number, boolean, null, object, array).
    // Returns pointer to the character after the value, or end on error.
    const char* skip_value(const char* p, const char* end);

    // Helpers used by skip_value:
    const char* skip_string(const char* p, const char* end); // p points AFTER opening "
    const char* skip_object(const char* p, const char* end); // p points AFTER opening {
    const char* skip_array (const char* p, const char* end); // p points AFTER opening [

    // Read a JSON string value into dst (null-terminated).
    // p must point to the character AFTER the opening '"'.
    // Returns pointer to the character after the closing '"', or nullptr on error.
    const char* read_string(const char* p, const char* end,
                            char* dst, size_t dst_len);

    // ── Object traversal ──────────────────────────────────────────────────────

    // Parse a top-level JSON object and populate McpRequest fields.
    // p must point to the character AFTER the opening '{'.
    bool scan_object(const char* p, const char* end, McpRequest& out);

    // Parse the params object and populate relevant McpRequest fields.
    // p must point to the character AFTER the opening '{'.
    bool scan_params(const char* p, const char* end, McpRequest& out);

    // Parse the arguments object inside params.
    // p must point to the character AFTER the opening '{'.
    bool scan_arguments(const char* p, const char* end, McpRequest& out);
};

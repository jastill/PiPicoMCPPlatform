#pragma once
#include "platform_config.h"

// ─────────────────────────────────────────────────────────────────────────────
// McpRequest — flat POD struct populated by JsonScanner from one JSON-RPC line.
//
// Only the fields used by the MCP tools implemented here are extracted.
// Unknown fields are skipped.
// ─────────────────────────────────────────────────────────────────────────────
struct McpRequest {
    // JSON-RPC envelope
    char jsonrpc[8];                      // expected "2.0"
    char method[MAX_METHOD_LEN];          // e.g. "tools/call"
    char id[MAX_ID_LEN];                  // request id stored as string
    bool id_is_number;                    // true → emit id as integer, false → as string
    bool id_present;                      // false for JSON-RPC notifications (no id)

    // params.name — the tool name when method == "tools/call"
    char tool_name[MAX_METHOD_LEN];

    // Tool arguments (flat; all tools use at most two argument fields)
    char arg_name[MAX_PARAM_NAME_LEN];   // used by: get_sensor_reading, get_output_state,
                                          //           set_output
    char arg_value[MAX_ARG_VAL_LEN];     // used by: set_output (the new state value)
    bool arg_has_name;
    bool arg_has_value;

    void clear();
};

#pragma once
#include "sensor_registry.h"
#include "actuator_registry.h"
#include "json_builder.h"
#include "json_scanner.h"
#include "mcp_request.h"
#include "platform_config.h"

// ─────────────────────────────────────────────────────────────────────────────
// McpServer — JSON-RPC 2.0 / MCP protocol dispatcher.
//
// Accepts one null-terminated line of JSON at a time via process_line(),
// parses it, dispatches to the appropriate handler, and writes the response
// into the caller-supplied tx_buf.
//
// All memory is on the stack or in static storage — no heap allocations.
// ─────────────────────────────────────────────────────────────────────────────
class McpServer {
public:
    McpServer(SensorRegistry& sensors, ActuatorRegistry& actuators);

    // Called once at startup (currently a no-op; kept for future setup).
    void init();

    // Process one complete JSON-RPC line (null-terminated, no trailing \n).
    // Writes the response (including trailing \n) into tx_buf[0..tx_buf_size).
    // Returns the number of bytes written (0 if the message is a notification
    // that requires no response, or if an internal error prevents any output).
    size_t process_line(const char* line, size_t line_len,
                        char* tx_buf, size_t tx_buf_size);

private:
    // ── JSON-RPC method handlers ──────────────────────────────────────────────
    void handle_initialize (const McpRequest& req, JsonBuilder& b);
    void handle_tools_list (const McpRequest& req, JsonBuilder& b);
    void handle_tools_call (const McpRequest& req, JsonBuilder& b);

    // ── MCP tool implementations ──────────────────────────────────────────────
    // Each writes the inner result JSON into 'inner' and then embeds it as a
    // string in the MCP content envelope using 'b'.
    void tool_list_sensors        (const McpRequest& req, JsonBuilder& b);
    void tool_get_sensor_readings (const McpRequest& req, JsonBuilder& b);
    void tool_get_sensor_reading  (const McpRequest& req, JsonBuilder& b);
    void tool_list_outputs        (const McpRequest& req, JsonBuilder& b);
    void tool_set_output          (const McpRequest& req, JsonBuilder& b);
    void tool_get_output_state    (const McpRequest& req, JsonBuilder& b);

    // ── inputSchema emitters (used in tools/list) ─────────────────────────────
    void emit_schema_no_args       (JsonBuilder& b);
    void emit_schema_name_required (JsonBuilder& b);
    void emit_schema_set_output    (JsonBuilder& b);

    // ── JSON-RPC envelope helpers ─────────────────────────────────────────────
    // Emits: { "jsonrpc":"2.0", "id":<id>, "result":
    void emit_result_header(JsonBuilder& b, const McpRequest& req);

    // Emits a full JSON-RPC error response (including closing braces + \n).
    void emit_error(JsonBuilder& b, const McpRequest& req,
                    int32_t code, const char* message);

    // Wraps inner JSON text in the MCP content envelope:
    //   "content":[{"type":"text","text":"<escaped inner>"}],"isError":false
    void emit_content_text(JsonBuilder& b, const char* inner_json);
    void emit_content_error(JsonBuilder& b, const char* message);

    // ── Tool descriptor helper ────────────────────────────────────────────────
    // Emits one tool entry: {"name":...,"description":...,"inputSchema":{...}}
    // Schema is emitted by calling schema_fn(b).
    void emit_tool_entry(JsonBuilder& b, const char* name, const char* description,
                         void (McpServer::*schema_fn)(JsonBuilder&));

    SensorRegistry&  sensors_;
    ActuatorRegistry& actuators_;
    JsonScanner       scanner_;

    // Reusable inner buffer for building tool result payloads.
    char inner_buf_[INNER_BUF_SIZE];
};

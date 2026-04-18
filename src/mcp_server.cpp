#include "mcp_server.h"
#include "sensor.h"
#include "actuator.h"
#include <cstring>
#include <cstdlib>

// ── Constructor / init ────────────────────────────────────────────────────────

McpServer::McpServer(SensorRegistry& sensors, ActuatorRegistry& actuators)
    : sensors_(sensors), actuators_(actuators) {}

void McpServer::init() {
    // Reserved for future hardware / SDK setup.
}

// ── Public entry point ────────────────────────────────────────────────────────

size_t McpServer::process_line(const char* line, size_t line_len,
                               char* tx_buf, size_t tx_buf_size) {
    McpRequest req;
    if (!scanner_.scan(line, line_len, req)) {
        // Malformed JSON — emit parse error (only if we can't even tell whether
        // there's an id, use null id per JSON-RPC spec).
        JsonBuilder b(tx_buf, tx_buf_size);
        emit_error(b, req, -32700, "Parse error");
        return b.finish();
    }

    // JSON-RPC notifications have no id — no response required.
    if (!req.id_present) {
        return 0;
    }

    JsonBuilder b(tx_buf, tx_buf_size);

    if (strncmp(req.method, "initialize", MAX_METHOD_LEN) == 0) {
        handle_initialize(req, b);
    } else if (strncmp(req.method, "tools/list", MAX_METHOD_LEN) == 0) {
        handle_tools_list(req, b);
    } else if (strncmp(req.method, "tools/call", MAX_METHOD_LEN) == 0) {
        handle_tools_call(req, b);
    } else {
        emit_error(b, req, -32601, "Method not found");
    }

    if (b.overflowed()) {
        // TX buffer was not large enough — emit a compact error instead.
        b.reset();
        emit_error(b, req, -32603, "Response too large for buffer");
    }

    return b.finish();
}

// ── JSON-RPC envelope helpers ─────────────────────────────────────────────────

void McpServer::emit_result_header(JsonBuilder& b, const McpRequest& req) {
    b.begin_object();
    b.kv_str("jsonrpc", "2.0");
    b.key("id");
    if (req.id_is_number) {
        b.value_raw(req.id);    // emit numeric id without quotes
    } else if (req.id[0] != '\0') {
        b.value_str(req.id);
    } else {
        b.value_null();
    }
    b.key("result");
}

void McpServer::emit_error(JsonBuilder& b, const McpRequest& req,
                           int32_t code, const char* message) {
    b.begin_object();
    b.kv_str("jsonrpc", "2.0");
    b.key("id");
    if (req.id_is_number && req.id[0] != '\0') {
        b.value_raw(req.id);
    } else if (req.id[0] != '\0') {
        b.value_str(req.id);
    } else {
        b.value_null();
    }
    b.key("error");
    b.begin_object();
    b.kv_int("code", code);
    b.kv_str("message", message);
    b.end_object();
    b.end_object();
}

void McpServer::emit_content_text(JsonBuilder& b, const char* inner_json) {
    b.key("content");
    b.begin_array();
    b.begin_object();
    b.kv_str("type", "text");
    b.kv_str("text", inner_json);   // inner JSON is a string value (double-escaped)
    b.end_object();
    b.end_array();
    b.kv_bool("isError", false);
}

void McpServer::emit_content_error(JsonBuilder& b, const char* message) {
    b.key("content");
    b.begin_array();
    b.begin_object();
    b.kv_str("type", "text");
    b.kv_str("text", message);
    b.end_object();
    b.end_array();
    b.kv_bool("isError", true);
}

// ── Tool schema helpers ───────────────────────────────────────────────────────

void McpServer::emit_schema_no_args(JsonBuilder& b) {
    b.key("inputSchema");
    b.begin_object();
    b.kv_str("type", "object");
    b.key("properties"); b.begin_object(); b.end_object();
    b.key("required");   b.begin_array();  b.end_array();
    b.end_object();
}

void McpServer::emit_schema_name_required(JsonBuilder& b) {
    b.key("inputSchema");
    b.begin_object();
    b.kv_str("type", "object");
    b.key("properties");
    b.begin_object();
    b.key("name");
    b.begin_object();
    b.kv_str("type", "string");
    b.kv_str("description", "Name of the sensor or output");
    b.end_object();
    b.end_object();
    b.key("required");
    b.begin_array();
    b.value_str("name");
    b.end_array();
    b.end_object();
}

void McpServer::emit_schema_set_output(JsonBuilder& b) {
    b.key("inputSchema");
    b.begin_object();
    b.kv_str("type", "object");
    b.key("properties");
    b.begin_object();
    b.key("name");
    b.begin_object();
    b.kv_str("type", "string");
    b.kv_str("description", "Output name");
    b.end_object();
    b.key("value");
    b.begin_object();
    b.kv_str("type", "number");
    b.kv_str("description",
              "State value: 0 or 1 for relay/LED; 0-100 for PWM duty cycle");
    b.end_object();
    b.end_object();
    b.key("required");
    b.begin_array();
    b.value_str("name");
    b.value_str("value");
    b.end_array();
    b.end_object();
}

void McpServer::emit_tool_entry(JsonBuilder& b, const char* name,
                                const char* description,
                                void (McpServer::*schema_fn)(JsonBuilder&)) {
    b.begin_object();
    b.kv_str("name", name);
    b.kv_str("description", description);
    (this->*schema_fn)(b);
    b.end_object();
}

// ── Method handlers ───────────────────────────────────────────────────────────

void McpServer::handle_initialize(const McpRequest& req, JsonBuilder& b) {
    emit_result_header(b, req);
    b.begin_object();
    b.kv_str("protocolVersion", MCP_PROTOCOL_VERSION);
    b.key("capabilities");
    b.begin_object();
    b.key("tools"); b.begin_object(); b.end_object();
    b.end_object();
    b.key("serverInfo");
    b.begin_object();
    b.kv_str("name",    MCP_SERVER_NAME);
    b.kv_str("version", MCP_SERVER_VERSION);
    b.end_object();
    b.end_object();  // result
    b.end_object();  // top-level
}

void McpServer::handle_tools_list(const McpRequest& req, JsonBuilder& b) {
    emit_result_header(b, req);
    b.begin_object();
    b.key("tools");
    b.begin_array();

    emit_tool_entry(b, "list_sensors",
        "List all registered sensors with their name, type, unit and description",
        &McpServer::emit_schema_no_args);

    emit_tool_entry(b, "get_sensor_readings",
        "Get current readings from all registered sensors",
        &McpServer::emit_schema_no_args);

    emit_tool_entry(b, "get_sensor_reading",
        "Get the current reading from a specific sensor by name",
        &McpServer::emit_schema_name_required);

    emit_tool_entry(b, "list_outputs",
        "List all registered outputs (relays, LEDs, PWM actuators)",
        &McpServer::emit_schema_no_args);

    emit_tool_entry(b, "set_output",
        "Set the state of an output. Use 0/1 for relay or LED; 0-100 for PWM",
        &McpServer::emit_schema_set_output);

    emit_tool_entry(b, "get_output_state",
        "Get the current state of a specific output by name",
        &McpServer::emit_schema_name_required);

    b.end_array();
    b.end_object();  // result
    b.end_object();  // top-level
}

void McpServer::handle_tools_call(const McpRequest& req, JsonBuilder& b) {
    if (strncmp(req.tool_name, "list_sensors", MAX_METHOD_LEN) == 0) {
        tool_list_sensors(req, b);
    } else if (strncmp(req.tool_name, "get_sensor_readings", MAX_METHOD_LEN) == 0) {
        tool_get_sensor_readings(req, b);
    } else if (strncmp(req.tool_name, "get_sensor_reading", MAX_METHOD_LEN) == 0) {
        tool_get_sensor_reading(req, b);
    } else if (strncmp(req.tool_name, "list_outputs", MAX_METHOD_LEN) == 0) {
        tool_list_outputs(req, b);
    } else if (strncmp(req.tool_name, "set_output", MAX_METHOD_LEN) == 0) {
        tool_set_output(req, b);
    } else if (strncmp(req.tool_name, "get_output_state", MAX_METHOD_LEN) == 0) {
        tool_get_output_state(req, b);
    } else {
        emit_error(b, req, -32602, "Unknown tool name");
    }
}

// ── Tool implementations ──────────────────────────────────────────────────────

void McpServer::tool_list_sensors(const McpRequest& req, JsonBuilder& b) {
    // Build inner JSON payload
    JsonBuilder inner(inner_buf_, sizeof(inner_buf_));
    inner.begin_object();
    inner.key("sensors");
    inner.begin_array();
    for (size_t i = 0; i < sensors_.count(); ++i) {
        ISensor* s = sensors_.get(i);
        const SensorInfo& info = s->info();
        inner.begin_object();
        inner.kv_str("name",        info.name);
        inner.kv_str("type",        sensor_type_str(info.type));
        inner.kv_str("unit",        info.unit);
        inner.kv_str("description", info.description);
        inner.end_object();
    }
    inner.end_array();
    inner.kv_int("count", (int32_t)sensors_.count());
    inner.end_object();
    // Null-terminate without appending newline
    if (inner.length() < sizeof(inner_buf_)) {
        inner_buf_[inner.length()] = '\0';
    }

    emit_result_header(b, req);
    b.begin_object();
    emit_content_text(b, inner_buf_);
    b.end_object();
    b.end_object();
}

void McpServer::tool_get_sensor_readings(const McpRequest& req, JsonBuilder& b) {
    JsonBuilder inner(inner_buf_, sizeof(inner_buf_));
    inner.begin_object();
    inner.key("readings");
    inner.begin_array();
    for (size_t i = 0; i < sensors_.count(); ++i) {
        ISensor* s = sensors_.get(i);
        const SensorInfo& info    = s->info();
        SensorReading     reading = s->read();
        inner.begin_object();
        inner.kv_str("name",  info.name);
        inner.kv_str("type",  sensor_type_str(info.type));
        inner.kv_str("unit",  info.unit);
        if (reading.valid) {
            inner.kv_float("value", reading.value, 3);
        } else {
            inner.kv_null("value");
        }
        inner.kv_bool("valid", reading.valid);
        inner.end_object();
    }
    inner.end_array();
    inner.kv_int("count", (int32_t)sensors_.count());
    inner.end_object();
    if (inner.length() < sizeof(inner_buf_)) {
        inner_buf_[inner.length()] = '\0';
    }

    emit_result_header(b, req);
    b.begin_object();
    emit_content_text(b, inner_buf_);
    b.end_object();
    b.end_object();
}

void McpServer::tool_get_sensor_reading(const McpRequest& req, JsonBuilder& b) {
    if (!req.arg_has_name || req.arg_name[0] == '\0') {
        emit_error(b, req, -32602, "Missing required argument: name");
        return;
    }

    ISensor* s = sensors_.find(req.arg_name);
    if (!s) {
        emit_error(b, req, -32602, "Sensor not found");
        return;
    }

    const SensorInfo& info    = s->info();
    SensorReading     reading = s->read();

    JsonBuilder inner(inner_buf_, sizeof(inner_buf_));
    inner.begin_object();
    inner.kv_str("name",  info.name);
    inner.kv_str("type",  sensor_type_str(info.type));
    inner.kv_str("unit",  info.unit);
    if (reading.valid) {
        inner.kv_float("value", reading.value, 3);
    } else {
        inner.kv_null("value");
    }
    inner.kv_bool("valid", reading.valid);
    inner.end_object();
    if (inner.length() < sizeof(inner_buf_)) {
        inner_buf_[inner.length()] = '\0';
    }

    emit_result_header(b, req);
    b.begin_object();
    emit_content_text(b, inner_buf_);
    b.end_object();
    b.end_object();
}

void McpServer::tool_list_outputs(const McpRequest& req, JsonBuilder& b) {
    JsonBuilder inner(inner_buf_, sizeof(inner_buf_));
    inner.begin_object();
    inner.key("outputs");
    inner.begin_array();
    for (size_t i = 0; i < actuators_.count(); ++i) {
        IActuator* a = actuators_.get(i);
        const ActuatorInfo& info  = a->info();
        ActuatorState       state = a->get();
        inner.begin_object();
        inner.kv_str("name",        info.name);
        inner.kv_str("type",        actuator_type_str(info.type));
        inner.kv_str("description", info.description);
        if (state.valid) {
            inner.kv_float("state", state.value, 1);
        } else {
            inner.kv_null("state");
        }
        inner.end_object();
    }
    inner.end_array();
    inner.kv_int("count", (int32_t)actuators_.count());
    inner.end_object();
    if (inner.length() < sizeof(inner_buf_)) {
        inner_buf_[inner.length()] = '\0';
    }

    emit_result_header(b, req);
    b.begin_object();
    emit_content_text(b, inner_buf_);
    b.end_object();
    b.end_object();
}

void McpServer::tool_set_output(const McpRequest& req, JsonBuilder& b) {
    if (!req.arg_has_name || req.arg_name[0] == '\0') {
        emit_error(b, req, -32602, "Missing required argument: name");
        return;
    }
    if (!req.arg_has_value || req.arg_value[0] == '\0') {
        emit_error(b, req, -32602, "Missing required argument: value");
        return;
    }

    IActuator* a = actuators_.find(req.arg_name);
    if (!a) {
        emit_error(b, req, -32602, "Output not found");
        return;
    }

    // Parse value — accept "true"/"false" as well as numbers
    float set_val;
    if (strncmp(req.arg_value, "true", 5) == 0) {
        set_val = 1.0f;
    } else if (strncmp(req.arg_value, "false", 6) == 0) {
        set_val = 0.0f;
    } else {
        char* end;
        set_val = strtof(req.arg_value, &end);
        if (end == req.arg_value) {
            emit_error(b, req, -32602, "Invalid value: must be a number or boolean");
            return;
        }
    }

    // Clamp to valid range depending on actuator type
    const ActuatorInfo& info = a->info();
    if (info.type == ActuatorType::RELAY || info.type == ActuatorType::LED) {
        set_val = (set_val != 0.0f) ? 1.0f : 0.0f;
    } else if (info.type == ActuatorType::PWM) {
        if (set_val < 0.0f)   set_val = 0.0f;
        if (set_val > 100.0f) set_val = 100.0f;
    }

    bool ok = a->set(set_val);

    JsonBuilder inner(inner_buf_, sizeof(inner_buf_));
    inner.begin_object();
    inner.kv_str("name",    info.name);
    inner.kv_str("type",    actuator_type_str(info.type));
    inner.kv_float("value", set_val, 1);
    inner.kv_bool("success", ok);
    inner.end_object();
    if (inner.length() < sizeof(inner_buf_)) {
        inner_buf_[inner.length()] = '\0';
    }

    emit_result_header(b, req);
    b.begin_object();
    if (ok) {
        emit_content_text(b, inner_buf_);
    } else {
        emit_content_error(b, "Failed to set output");
    }
    b.end_object();
    b.end_object();
}

void McpServer::tool_get_output_state(const McpRequest& req, JsonBuilder& b) {
    if (!req.arg_has_name || req.arg_name[0] == '\0') {
        emit_error(b, req, -32602, "Missing required argument: name");
        return;
    }

    IActuator* a = actuators_.find(req.arg_name);
    if (!a) {
        emit_error(b, req, -32602, "Output not found");
        return;
    }

    const ActuatorInfo& info  = a->info();
    ActuatorState       state = a->get();

    JsonBuilder inner(inner_buf_, sizeof(inner_buf_));
    inner.begin_object();
    inner.kv_str("name", info.name);
    inner.kv_str("type", actuator_type_str(info.type));
    if (state.valid) {
        inner.kv_float("value", state.value, 1);
    } else {
        inner.kv_null("value");
    }
    inner.kv_bool("valid", state.valid);
    inner.end_object();
    if (inner.length() < sizeof(inner_buf_)) {
        inner_buf_[inner.length()] = '\0';
    }

    emit_result_header(b, req);
    b.begin_object();
    emit_content_text(b, inner_buf_);
    b.end_object();
    b.end_object();
}

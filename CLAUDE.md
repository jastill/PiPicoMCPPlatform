# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Build

This is a Raspberry Pi Pico (RP2040) project using the Pico SDK and CMake. There is no way to run or test on the host machine — the output is a `.uf2` firmware image.

```bash
export PICO_SDK_PATH=/path/to/pico-sdk

mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)
# Output: build/mcp_server.uf2
```

To flash: hold BOOTSEL on the Pico, plug it in (mounts as USB mass storage), copy `mcp_server.uf2` to it.

**IDE note:** Clang LSP will show "file not found" errors for all Pico SDK and project headers until `cmake` has generated `build/compile_commands.json`. This is expected — the build itself is correct.

## Architecture

This is a bare-metal MCP (Model Context Protocol) server. An AI client connects over USB serial and calls tools that read sensors or control actuators. All communication is newline-delimited JSON-RPC 2.0.

### Data flow

```
USB serial (CDC)
    → UsbCdcTransport::read_byte()          [main.cpp accumulates into rx_buf until '\n']
    → McpServer::process_line()
        → JsonScanner::scan()               → McpRequest (flat POD)
        → handle_initialize / handle_tools_list / handle_tools_call
            → tool_*() methods
                → SensorRegistry / ActuatorRegistry  → ISensor / IActuator
                → JsonBuilder (inner_buf_)           [builds inner JSON payload]
            → JsonBuilder (tx_buf)                   [wraps payload in MCP content envelope]
    → UsbCdcTransport::write()              [response + '\n' flushed immediately]
```

### Key design constraint: zero heap allocation

Every buffer is statically sized. `new`/`malloc` are never called. All sensor and actuator objects in `app/main.cpp` are `static`. The registries hold raw pointers to these statics. `McpServer` owns `inner_buf_[INNER_BUF_SIZE]` as a member for building tool result payloads before embedding them as strings in the outer response.

### Two-level JSON encoding in tool responses

Tool results follow the MCP spec: the actual data is a JSON object serialised *as a string* inside `content[0].text`. This means the inner JSON is double-escaped. The flow is:

1. A `JsonBuilder inner(inner_buf_, ...)` builds the data object (e.g. `{"readings":[...]}`)
2. `emit_content_text(b, inner_buf_)` passes that string to `b.value_str()`, which JSON-escapes it into the outer response

### JsonBuilder comma logic

`JsonBuilder` tracks comma placement with an `after_key_` flag and per-depth `needs_comma_[]` / `in_array_[]` arrays. The critical rule: `key()` sets `after_key_ = true`, which suppresses the comma check in the immediately following value emitter or `begin_object()`/`begin_array()`. This prevents `"key":,{...}` bugs. Always call `key()` before a value when inside an object; call value emitters directly when inside an array.

### JsonScanner field extraction

`JsonScanner` makes one linear pass and populates `McpRequest` (a flat POD). It descends at most two levels: top-level object → `params` → `params.arguments`. All argument fields for every tool are stored flat in `McpRequest` (`arg_name`, `arg_value`). If you add a tool that needs a third argument field, add it to `McpRequest` and teach `scan_arguments()` to extract it.

## Extending the platform

### Adding a new sensor

1. Create `sensors/include/my_sensor.h` and `sensors/src/my_sensor.cpp`
2. Subclass `ISensor`, populate `info_` in the constructor, implement `read()`
3. Add the `.cpp` to `PLATFORM_SOURCES` in `CMakeLists.txt`
4. Declare a `static MyCustomSensor` in `app/main.cpp` and call `sensors.add(&it)`

No other files need changing — `McpServer` reads from the registry dynamically.

### Adding a new actuator

Same pattern as sensors, but subclass `IActuator` from `actuators/include/`. The `set(float value)` convention: `0`/`1` for binary types, `0`–`100` for PWM. `McpServer::tool_set_output()` enforces the clamping before calling `set()`.

### Adding a new MCP tool

1. Add a `tool_my_tool()` private method to `McpServer`
2. Dispatch it in `handle_tools_call()` by matching `req.tool_name`
3. Advertise it in `handle_tools_list()` via `emit_tool_entry()` with an appropriate schema emitter
4. If the tool needs new argument fields, add them to `McpRequest` and `JsonScanner::scan_arguments()`

## Configuration

All compile-time limits are in `include/platform_config.h`. The ones most likely to need adjustment:

- `TX_BUFFER_SIZE` (4096) — increase if `tools/list` with many sensors overflows
- `INNER_BUF_SIZE` (2048) — increase if a tool's data payload overflows  
- `MAX_SENSORS` / `MAX_ACTUATORS` (16 each) — registry capacity
- `MAX_DESC_LEN` (64) — maximum sensor/actuator description length

If `JsonBuilder::overflowed()` returns true after building a response, `McpServer::process_line()` discards it and sends a `-32603` error instead.

## MCP protocol notes

- JSON-RPC notifications (no `id` field) receive no response — `process_line()` returns 0
- The `id` field may be a JSON number or string; `McpRequest::id_is_number` controls how it is re-emitted
- The server only handles `initialize`, `tools/list`, and `tools/call`; all other methods return `-32601`
- `notifications/initialized` (sent by clients after `initialize`) is silently consumed as a notification

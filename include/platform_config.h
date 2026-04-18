#pragma once
#include <cstddef>
#include <cstdint>

// ─────────────────────────────────────────────────────────────────────────────
// Registry limits — increase if you need more sensors/actuators.
// Each slot is a pointer (4 bytes on RP2040), so cost is minimal.
// ─────────────────────────────────────────────────────────────────────────────
static constexpr size_t MAX_SENSORS   = 16;
static constexpr size_t MAX_ACTUATORS = 16;

// ─────────────────────────────────────────────────────────────────────────────
// Buffer sizes
// RX_BUFFER_SIZE: must hold one complete inbound JSON-RPC line
// TX_BUFFER_SIZE: must hold one complete outbound JSON-RPC response
// INNER_BUF_SIZE: temporary buffer used inside tool handlers to build the
//                 inner JSON payload before embedding it as a string in the
//                 MCP content envelope
// ─────────────────────────────────────────────────────────────────────────────
static constexpr size_t RX_BUFFER_SIZE  = 512;
static constexpr size_t TX_BUFFER_SIZE  = 4096;
static constexpr size_t INNER_BUF_SIZE  = 2048;

// ─────────────────────────────────────────────────────────────────────────────
// String field lengths (includes null terminator)
// ─────────────────────────────────────────────────────────────────────────────
static constexpr size_t MAX_NAME_LEN       = 32;
static constexpr size_t MAX_UNIT_LEN       = 16;
static constexpr size_t MAX_DESC_LEN       = 64;
static constexpr size_t MAX_METHOD_LEN     = 48;
static constexpr size_t MAX_PARAM_NAME_LEN = 32;
static constexpr size_t MAX_ID_LEN         = 16;
static constexpr size_t MAX_ARG_VAL_LEN    = 32;

// ─────────────────────────────────────────────────────────────────────────────
// USB transport
// ─────────────────────────────────────────────────────────────────────────────
// read_byte() timeout in microseconds — 0 = non-blocking poll
static constexpr uint32_t USB_CHAR_TIMEOUT_US = 0;

// ─────────────────────────────────────────────────────────────────────────────
// MCP protocol version advertised during initialize
// ─────────────────────────────────────────────────────────────────────────────
static constexpr const char* MCP_PROTOCOL_VERSION = "2024-11-05";
static constexpr const char* MCP_SERVER_NAME      = "PiPicoMCPServer";
static constexpr const char* MCP_SERVER_VERSION   = "1.0.0";

// ─────────────────────────────────────────────────────────────────────────────
// JsonBuilder nesting depth limit
// ─────────────────────────────────────────────────────────────────────────────
static constexpr uint8_t JSON_MAX_DEPTH = 10;

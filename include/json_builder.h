#pragma once
#include "platform_config.h"
#include <cstddef>
#include <cstdint>

// ─────────────────────────────────────────────────────────────────────────────
// JsonBuilder — zero-allocation, buffer-based JSON serialiser.
//
// Usage:
//   char buf[512];
//   JsonBuilder b(buf, sizeof(buf));
//   b.begin_object()
//    .kv_str("jsonrpc", "2.0")
//    .kv_int("id", 1)
//    .key("result").begin_object()
//      .kv_str("status", "ok")
//    .end_object()
//    .end_object();
//   size_t len = b.finish();  // null-terminates, appends '\n', returns byte count
//
// Rules:
//   • Inside an object: call key() before each value emitter.
//   • Inside an array:  call value emitters directly (no key()).
//   • kv_* helpers combine key() + value() for convenience.
//   • On buffer overflow writes are silently dropped; overflowed() returns true.
// ─────────────────────────────────────────────────────────────────────────────
class JsonBuilder {
public:
    JsonBuilder(char* buf, size_t capacity);

    // Reset so the same buffer can be reused.
    void reset();

    // ── Structural ────────────────────────────────────────────────────────────
    JsonBuilder& begin_object();
    JsonBuilder& end_object();
    JsonBuilder& begin_array();
    JsonBuilder& end_array();

    // Emit a quoted key followed by ':'.
    JsonBuilder& key(const char* k);

    // ── Value emitters ────────────────────────────────────────────────────────
    JsonBuilder& value_str  (const char* s);
    JsonBuilder& value_int  (int32_t v);
    JsonBuilder& value_float(float v, int decimals = 3);
    JsonBuilder& value_bool (bool v);
    JsonBuilder& value_null ();
    // Pre-serialised JSON fragment (inserted verbatim, no quoting).
    JsonBuilder& value_raw  (const char* raw);

    // ── Convenience key+value helpers ─────────────────────────────────────────
    JsonBuilder& kv_str  (const char* k, const char* v);
    JsonBuilder& kv_int  (const char* k, int32_t v);
    JsonBuilder& kv_float(const char* k, float v, int decimals = 3);
    JsonBuilder& kv_bool (const char* k, bool v);
    JsonBuilder& kv_null (const char* k);

    // ── Finalise ──────────────────────────────────────────────────────────────
    // Appends '\n', null-terminates, returns total bytes written (incl. '\n').
    size_t finish();

    bool        overflowed() const;
    const char* data()       const;
    size_t      length()     const;

private:
    void append_char  (char c);
    void append_raw   (const char* s);    // raw bytes, no quoting
    void append_quoted(const char* s);    // JSON string-escaped with surrounding quotes

    // Emit ',' if needed before the current item.
    // Suppressed when the previous emit was a key (colon).
    void maybe_comma();

    // Called after emitting a complete value (or before structural open)
    // to update comma-tracking state.
    void after_value();

    char*   buf_;
    size_t  cap_;
    size_t  pos_;
    bool    overflow_;

    // after_key_: true between a key() call and the subsequent value.
    // Suppresses comma in maybe_comma() and skips needs_comma_ update.
    bool    after_key_;

    // Per-depth comma and container-type tracking.
    // Index is the depth BEFORE entering that container
    // (so depth_ is the current level, depth_-1 is the parent).
    bool    needs_comma_[JSON_MAX_DEPTH];  // needs comma before next item
    bool    in_array_[JSON_MAX_DEPTH];     // true = array, false = object
    uint8_t depth_;
};

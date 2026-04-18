#include "json_builder.h"
#include <cstring>
#include <cstdio>

// ── Constructor / reset ───────────────────────────────────────────────────────

JsonBuilder::JsonBuilder(char* buf, size_t capacity)
    : buf_(buf), cap_(capacity), pos_(0), overflow_(false),
      after_key_(false), depth_(0) {
    if (cap_ > 0) buf_[0] = '\0';
    for (uint8_t i = 0; i < JSON_MAX_DEPTH; ++i) {
        needs_comma_[i] = false;
        in_array_[i]    = false;
    }
}

void JsonBuilder::reset() {
    pos_       = 0;
    overflow_  = false;
    after_key_ = false;
    depth_     = 0;
    if (cap_ > 0) buf_[0] = '\0';
    for (uint8_t i = 0; i < JSON_MAX_DEPTH; ++i) {
        needs_comma_[i] = false;
        in_array_[i]    = false;
    }
}

// ── Low-level character output ────────────────────────────────────────────────

void JsonBuilder::append_char(char c) {
    if (pos_ + 1 < cap_) {
        buf_[pos_++] = c;
        buf_[pos_]   = '\0';
    } else {
        overflow_ = true;
    }
}

void JsonBuilder::append_raw(const char* s) {
    while (*s) append_char(*s++);
}

void JsonBuilder::append_quoted(const char* s) {
    append_char('"');
    while (*s) {
        char c = *s++;
        switch (c) {
            case '"':  append_char('\\'); append_char('"');  break;
            case '\\': append_char('\\'); append_char('\\'); break;
            case '\n': append_char('\\'); append_char('n');  break;
            case '\r': append_char('\\'); append_char('r');  break;
            case '\t': append_char('\\'); append_char('t');  break;
            default:
                if ((unsigned char)c < 0x20) {
                    char esc[8];
                    snprintf(esc, sizeof(esc), "\\u%04x", (unsigned char)c);
                    append_raw(esc);
                } else {
                    append_char(c);
                }
                break;
        }
    }
    append_char('"');
}

// ── Comma logic ───────────────────────────────────────────────────────────────
//
// maybe_comma(): emit ',' if the current container has had a previous item
//   AND we are not immediately after a key (key: value needs no comma).
//
// after_value(): update the parent container's needs_comma_ flag after
//   a complete value has been emitted (only for arrays; objects track this
//   via key()).

void JsonBuilder::maybe_comma() {
    if (!after_key_ && depth_ > 0 && needs_comma_[depth_ - 1]) {
        append_char(',');
    }
}

void JsonBuilder::after_value() {
    // After a value, clear the after_key_ flag.
    // If we are directly inside an array (not following a key), mark that
    // the array now needs a comma before the next element.
    if (!after_key_ && depth_ > 0 && in_array_[depth_ - 1]) {
        needs_comma_[depth_ - 1] = true;
    }
    after_key_ = false;
}

// ── Structural ────────────────────────────────────────────────────────────────

JsonBuilder& JsonBuilder::begin_object() {
    maybe_comma();
    // If we're inside an array (not following a key), mark that array needs comma.
    if (!after_key_ && depth_ > 0 && in_array_[depth_ - 1]) {
        needs_comma_[depth_ - 1] = true;
    }
    after_key_ = false;
    append_char('{');
    if (depth_ < JSON_MAX_DEPTH) {
        needs_comma_[depth_] = false;
        in_array_[depth_]    = false;
        ++depth_;
    }
    return *this;
}

JsonBuilder& JsonBuilder::end_object() {
    if (depth_ > 0) --depth_;
    after_key_ = false;
    append_char('}');
    return *this;
}

JsonBuilder& JsonBuilder::begin_array() {
    maybe_comma();
    if (!after_key_ && depth_ > 0 && in_array_[depth_ - 1]) {
        needs_comma_[depth_ - 1] = true;
    }
    after_key_ = false;
    append_char('[');
    if (depth_ < JSON_MAX_DEPTH) {
        needs_comma_[depth_] = false;
        in_array_[depth_]    = true;
        ++depth_;
    }
    return *this;
}

JsonBuilder& JsonBuilder::end_array() {
    if (depth_ > 0) --depth_;
    after_key_ = false;
    append_char(']');
    return *this;
}

JsonBuilder& JsonBuilder::key(const char* k) {
    // Emit comma if there were previous members in this object.
    maybe_comma();
    // Mark that this object now has at least one member — next key needs comma.
    if (depth_ > 0) needs_comma_[depth_ - 1] = true;
    append_quoted(k);
    append_char(':');
    after_key_ = true;
    return *this;
}

// ── Value emitters ────────────────────────────────────────────────────────────

JsonBuilder& JsonBuilder::value_str(const char* s) {
    maybe_comma();
    append_quoted(s ? s : "");
    after_value();
    return *this;
}

JsonBuilder& JsonBuilder::value_int(int32_t v) {
    maybe_comma();
    char tmp[16];
    snprintf(tmp, sizeof(tmp), "%ld", (long)v);
    append_raw(tmp);
    after_value();
    return *this;
}

JsonBuilder& JsonBuilder::value_float(float v, int decimals) {
    maybe_comma();
    char tmp[32];
    snprintf(tmp, sizeof(tmp), "%.*f", decimals, (double)v);
    append_raw(tmp);
    after_value();
    return *this;
}

JsonBuilder& JsonBuilder::value_bool(bool v) {
    maybe_comma();
    append_raw(v ? "true" : "false");
    after_value();
    return *this;
}

JsonBuilder& JsonBuilder::value_null() {
    maybe_comma();
    append_raw("null");
    after_value();
    return *this;
}

JsonBuilder& JsonBuilder::value_raw(const char* raw) {
    maybe_comma();
    append_raw(raw ? raw : "null");
    after_value();
    return *this;
}

// ── Convenience helpers ───────────────────────────────────────────────────────

JsonBuilder& JsonBuilder::kv_str(const char* k, const char* v) {
    key(k); return value_str(v);
}

JsonBuilder& JsonBuilder::kv_int(const char* k, int32_t v) {
    key(k); return value_int(v);
}

JsonBuilder& JsonBuilder::kv_float(const char* k, float v, int decimals) {
    key(k); return value_float(v, decimals);
}

JsonBuilder& JsonBuilder::kv_bool(const char* k, bool v) {
    key(k); return value_bool(v);
}

JsonBuilder& JsonBuilder::kv_null(const char* k) {
    key(k); return value_null();
}

// ── Finalise ──────────────────────────────────────────────────────────────────

size_t JsonBuilder::finish() {
    if (pos_ + 1 < cap_) {
        buf_[pos_++] = '\n';
        buf_[pos_]   = '\0';
    } else if (cap_ > 0) {
        // Force newline at end even on overflow
        buf_[cap_ - 1] = '\n';
        pos_            = cap_ - 1;
        overflow_       = true;
    }
    return pos_;
}

bool JsonBuilder::overflowed() const { return overflow_; }
const char* JsonBuilder::data()   const { return buf_; }
size_t      JsonBuilder::length() const { return pos_; }

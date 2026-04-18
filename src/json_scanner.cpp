#include "json_scanner.h"
#include <cstring>
#include <cstdlib>

// ── Helper utilities ──────────────────────────────────────────────────────────

void McpRequest::clear() {
    jsonrpc[0]      = '\0';
    method[0]       = '\0';
    id[0]           = '\0';
    id_is_number    = false;
    id_present      = false;
    tool_name[0]    = '\0';
    arg_name[0]     = '\0';
    arg_value[0]    = '\0';
    arg_has_name    = false;
    arg_has_value   = false;
}

// ── JsonScanner implementation ────────────────────────────────────────────────

const char* JsonScanner::skip_ws(const char* p, const char* end) {
    while (p < end && (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r')) {
        ++p;
    }
    return p;
}

const char* JsonScanner::skip_string(const char* p, const char* end) {
    // p points to the character AFTER the opening '"'
    while (p < end) {
        char c = *p++;
        if (c == '\\') {
            if (p < end) ++p; // skip escaped character
        } else if (c == '"') {
            return p; // past closing '"'
        }
    }
    return end; // unterminated string
}

const char* JsonScanner::skip_object(const char* p, const char* end) {
    // p points AFTER the opening '{'
    int depth = 1;
    while (p < end && depth > 0) {
        char c = *p++;
        if (c == '"') {
            p = skip_string(p, end);
        } else if (c == '{') {
            depth++;
        } else if (c == '}') {
            depth--;
        }
    }
    return p;
}

const char* JsonScanner::skip_array(const char* p, const char* end) {
    // p points AFTER the opening '['
    int depth = 1;
    while (p < end && depth > 0) {
        char c = *p++;
        if (c == '"') {
            p = skip_string(p, end);
        } else if (c == '[') {
            depth++;
        } else if (c == ']') {
            depth--;
        }
    }
    return p;
}

const char* JsonScanner::skip_value(const char* p, const char* end) {
    p = skip_ws(p, end);
    if (p >= end) return end;

    char c = *p;
    if (c == '"') {
        return skip_string(p + 1, end);
    } else if (c == '{') {
        return skip_object(p + 1, end);
    } else if (c == '[') {
        return skip_array(p + 1, end);
    } else {
        // number, true, false, null — advance until delimiter
        while (p < end) {
            char ch = *p;
            if (ch == ',' || ch == '}' || ch == ']' ||
                ch == ' ' || ch == '\t' || ch == '\n' || ch == '\r') {
                break;
            }
            ++p;
        }
        return p;
    }
}

const char* JsonScanner::read_string(const char* p, const char* end,
                                      char* dst, size_t dst_len) {
    // p points AFTER the opening '"'
    size_t out = 0;
    while (p < end) {
        char c = *p++;
        if (c == '\\') {
            if (p >= end) break;
            char esc = *p++;
            char decoded = esc;
            switch (esc) {
                case 'n':  decoded = '\n'; break;
                case 'r':  decoded = '\r'; break;
                case 't':  decoded = '\t'; break;
                case '"':  decoded = '"';  break;
                case '\\': decoded = '\\'; break;
                // \uXXXX and other escapes: just copy the letter
                default:   decoded = esc; break;
            }
            if (out + 1 < dst_len) dst[out++] = decoded;
        } else if (c == '"') {
            break; // closing quote
        } else {
            if (out + 1 < dst_len) dst[out++] = c;
        }
    }
    dst[out] = '\0';
    return p;
}

// ── Object traversal ──────────────────────────────────────────────────────────

bool JsonScanner::scan_arguments(const char* p, const char* end, McpRequest& out) {
    // p points AFTER the opening '{'
    while (p < end) {
        p = skip_ws(p, end);
        if (p >= end || *p == '}') break;
        if (*p == ',') { ++p; continue; }
        if (*p != '"') break; // malformed

        // Read key
        char key[MAX_PARAM_NAME_LEN];
        p = read_string(p + 1, end, key, sizeof(key));
        p = skip_ws(p, end);
        if (p >= end || *p != ':') break;
        ++p;
        p = skip_ws(p, end);

        if (strcmp(key, "name") == 0) {
            if (p < end && *p == '"') {
                p = read_string(p + 1, end, out.arg_name, MAX_PARAM_NAME_LEN);
                out.arg_has_name = true;
            } else {
                p = skip_value(p, end);
            }
        } else if (strcmp(key, "value") == 0 || strcmp(key, "state") == 0) {
            // Accept "value" or "state" as the actuator value parameter
            char tmp[MAX_ARG_VAL_LEN];
            if (p < end && *p == '"') {
                p = read_string(p + 1, end, tmp, sizeof(tmp));
            } else {
                // Capture raw token (number or boolean)
                const char* start = p;
                p = skip_value(p, end);
                size_t len = (size_t)(p - start);
                if (len >= MAX_ARG_VAL_LEN) len = MAX_ARG_VAL_LEN - 1;
                strncpy(tmp, start, len);
                tmp[len] = '\0';
            }
            strncpy(out.arg_value, tmp, MAX_ARG_VAL_LEN - 1);
            out.arg_value[MAX_ARG_VAL_LEN - 1] = '\0';
            out.arg_has_value = true;
        } else {
            p = skip_value(p, end);
        }
    }
    return true;
}

bool JsonScanner::scan_params(const char* p, const char* end, McpRequest& out) {
    // p points AFTER the opening '{'
    while (p < end) {
        p = skip_ws(p, end);
        if (p >= end || *p == '}') break;
        if (*p == ',') { ++p; continue; }
        if (*p != '"') break;

        char key[MAX_METHOD_LEN];
        p = read_string(p + 1, end, key, sizeof(key));
        p = skip_ws(p, end);
        if (p >= end || *p != ':') break;
        ++p;
        p = skip_ws(p, end);

        if (strcmp(key, "name") == 0) {
            if (p < end && *p == '"') {
                p = read_string(p + 1, end, out.tool_name, MAX_METHOD_LEN);
            } else {
                p = skip_value(p, end);
            }
        } else if (strcmp(key, "arguments") == 0) {
            if (p < end && *p == '{') {
                scan_arguments(p + 1, end, out);
                p = skip_object(p + 1, end);
            } else {
                p = skip_value(p, end);
            }
        } else {
            p = skip_value(p, end);
        }
    }
    return true;
}

bool JsonScanner::scan_object(const char* p, const char* end, McpRequest& out) {
    // p points AFTER the opening '{'
    while (p < end) {
        p = skip_ws(p, end);
        if (p >= end || *p == '}') break;
        if (*p == ',') { ++p; continue; }
        if (*p != '"') break;

        char key[MAX_METHOD_LEN];
        p = read_string(p + 1, end, key, sizeof(key));
        p = skip_ws(p, end);
        if (p >= end || *p != ':') break;
        ++p;
        p = skip_ws(p, end);

        if (strcmp(key, "jsonrpc") == 0) {
            if (p < end && *p == '"') {
                p = read_string(p + 1, end, out.jsonrpc, sizeof(out.jsonrpc));
            } else {
                p = skip_value(p, end);
            }
        } else if (strcmp(key, "method") == 0) {
            if (p < end && *p == '"') {
                p = read_string(p + 1, end, out.method, MAX_METHOD_LEN);
            } else {
                p = skip_value(p, end);
            }
        } else if (strcmp(key, "id") == 0) {
            out.id_present = true;
            if (p < end && *p == '"') {
                // String id
                p = read_string(p + 1, end, out.id, MAX_ID_LEN);
                out.id_is_number = false;
            } else if (p < end && (*p == 'n')) {
                // null id — treat as absent
                p = skip_value(p, end);
                out.id_present = false;
            } else {
                // Numeric id — capture raw digits
                const char* start = p;
                p = skip_value(p, end);
                size_t len = (size_t)(p - start);
                if (len >= MAX_ID_LEN) len = MAX_ID_LEN - 1;
                strncpy(out.id, start, len);
                out.id[len] = '\0';
                out.id_is_number = true;
            }
        } else if (strcmp(key, "params") == 0) {
            if (p < end && *p == '{') {
                scan_params(p + 1, end, out);
                p = skip_object(p + 1, end);
            } else {
                p = skip_value(p, end);
            }
        } else {
            p = skip_value(p, end);
        }
    }
    return (out.method[0] != '\0');
}

bool JsonScanner::scan(const char* json, size_t len, McpRequest& out) {
    out.clear();
    if (!json || len == 0) return false;

    const char* p   = json;
    const char* end = json + len;

    p = skip_ws(p, end);
    if (p >= end || *p != '{') return false;

    return scan_object(p + 1, end, out);
}

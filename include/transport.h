#pragma once
#include <cstddef>

// Abstract transport layer for the MCP server.
// Concrete implementations: UsbCdcTransport (USB serial), UartTransport, etc.
class ITransport {
public:
    virtual ~ITransport() = default;

    // Initialise the underlying hardware / SDK subsystem.
    virtual void init() = 0;

    // Read one byte.  Returns the byte value (0–255) or -1 if none available.
    virtual int read_byte() = 0;

    // Write len bytes from buf.  Returns number of bytes actually written.
    virtual size_t write(const char* buf, size_t len) = 0;

    // Optional: returns true when a host is connected and ready to receive data.
    // Implementations that cannot determine this should return true always.
    virtual bool connected() = 0;
};

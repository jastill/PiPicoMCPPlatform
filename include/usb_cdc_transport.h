#pragma once
#include "transport.h"

// USB CDC (USB serial) transport using the Pico SDK stdio layer.
//
// Prerequisites in CMakeLists.txt:
//   pico_enable_stdio_usb(target 1)
//   pico_enable_stdio_uart(target 0)
//
// The SDK registers the USB CDC ACM class automatically when PICO_STDIO_USB=1.
class UsbCdcTransport : public ITransport {
public:
    // Calls stdio_init_all().  Must be called before any other method.
    void   init()      override;

    // Non-blocking read via getchar_timeout_us(0).
    // Returns -1 (PICO_ERROR_TIMEOUT) when no byte is ready.
    int    read_byte() override;

    // Writes buf[0..len) to stdout and flushes.  Flush is essential:
    // without it the CDC driver buffers output and the host stalls.
    size_t write(const char* buf, size_t len) override;

    // Returns true when a USB host has the virtual COM port open.
    bool   connected() override;
};

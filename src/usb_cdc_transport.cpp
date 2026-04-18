#include "usb_cdc_transport.h"
#include "pico/stdlib.h"
#include "pico/stdio_usb.h"
#include <cstdio>

void UsbCdcTransport::init() {
    stdio_init_all();
}

int UsbCdcTransport::read_byte() {
    // Non-blocking: returns PICO_ERROR_TIMEOUT (-1) immediately if no byte ready.
    return getchar_timeout_us(USB_CHAR_TIMEOUT_US);
}

size_t UsbCdcTransport::write(const char* buf, size_t len) {
    size_t written = fwrite(buf, 1, len, stdout);
    fflush(stdout);  // Critical: flush CDC driver buffer so host sees the response.
    return written;
}

bool UsbCdcTransport::connected() {
    return stdio_usb_connected();
}

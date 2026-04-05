// Register libusb enum types with Emscripten embind so that
// emval_create_invoker can resolve them at runtime.

#include <emscripten/bind.h>
#include <libusb.h>

EMSCRIPTEN_BINDINGS(libusb_types) {
    emscripten::enum_<libusb_transfer_status>("libusb_transfer_status")
        .value("LIBUSB_TRANSFER_COMPLETED", LIBUSB_TRANSFER_COMPLETED)
        .value("LIBUSB_TRANSFER_ERROR", LIBUSB_TRANSFER_ERROR)
        .value("LIBUSB_TRANSFER_TIMED_OUT", LIBUSB_TRANSFER_TIMED_OUT)
        .value("LIBUSB_TRANSFER_CANCELLED", LIBUSB_TRANSFER_CANCELLED)
        .value("LIBUSB_TRANSFER_STALL", LIBUSB_TRANSFER_STALL)
        .value("LIBUSB_TRANSFER_NO_DEVICE", LIBUSB_TRANSFER_NO_DEVICE)
        .value("LIBUSB_TRANSFER_OVERFLOW", LIBUSB_TRANSFER_OVERFLOW);
}

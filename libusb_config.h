/* config.h - manual config for Emscripten/WebAssembly build of libusb 1.0.27 */

#define DEFAULT_VISIBILITY __attribute__ ((visibility ("default")))
#define PRINTF_FORMAT(a, b) __attribute__ ((__format__ (__printf__, a, b)))

#define PLATFORM_POSIX 1
#define _GNU_SOURCE 1
#define ENABLE_LOGGING 1

#define HAVE_SYS_TIME_H 1
#define HAVE_NFDS_T 1
#define HAVE_CLOCK_GETTIME 1

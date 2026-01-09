/**
 * Greenleaf Communications Library (GCL) Compatibility Stubs
 *
 * GCL was a serial/modem communications library used for multiplayer
 * in the DOS era. This functionality is completely obsolete.
 *
 * Modern multiplayer uses TCP/IP via enet (platform layer).
 * All GCL functions return failure to indicate "modem not available".
 */

#ifndef COMPAT_GCL_H
#define COMPAT_GCL_H

#include <cstdint>

// =============================================================================
// GCL Return Codes
// =============================================================================

#define GCL_SUCCESS          0
#define GCL_ERROR           -1
#define GCL_NO_CARRIER      -2
#define GCL_NO_MODEM        -3
#define GCL_BUSY            -4
#define GCL_NO_DIALTONE     -5
#define GCL_TIMEOUT         -6
#define GCL_INVALID_PORT    -7
#define GCL_PORT_IN_USE     -8
#define GCL_NO_MEMORY       -9
#define GCL_NOT_INITIALIZED -10

// =============================================================================
// GCL Types
// =============================================================================

typedef int32_t GCL_HANDLE;
typedef int32_t GCL_PORT;

// Port identifiers
#define GCL_COM1    0
#define GCL_COM2    1
#define GCL_COM3    2
#define GCL_COM4    3

// Baud rates
#define GCL_BAUD_300    300
#define GCL_BAUD_1200   1200
#define GCL_BAUD_2400   2400
#define GCL_BAUD_9600   9600
#define GCL_BAUD_14400  14400
#define GCL_BAUD_19200  19200
#define GCL_BAUD_28800  28800
#define GCL_BAUD_38400  38400
#define GCL_BAUD_57600  57600

// =============================================================================
// GCL Structures
// =============================================================================

typedef struct _GCL_CONFIG {
    GCL_PORT port;           // COM port
    int32_t  baudRate;       // Baud rate
    int32_t  dataBits;       // Data bits (7 or 8)
    int32_t  stopBits;       // Stop bits (1 or 2)
    int32_t  parity;         // Parity (0=none, 1=odd, 2=even)
    int32_t  flowControl;    // Flow control (0=none, 1=hardware, 2=software)
} GCL_CONFIG;

typedef struct _GCL_MODEM_INIT {
    char* initString;        // Modem init string (e.g., "ATZ")
    char* dialPrefix;        // Dial prefix (e.g., "ATDT")
    char* hangupString;      // Hangup string (e.g., "ATH0")
} GCL_MODEM_INIT;

// =============================================================================
// GCL Function Stubs
// =============================================================================
// All functions return failure - modem/serial is not supported

#ifdef __cplusplus
extern "C" {
#endif

// Initialization - Always fails
static inline int32_t gcl_init(void) {
    return GCL_NOT_INITIALIZED;
}

static inline int32_t gcl_shutdown(void) {
    return GCL_SUCCESS; // OK to "shut down" nothing
}

// Port management - Always fails
static inline GCL_HANDLE gcl_open_port(GCL_PORT port, GCL_CONFIG* config) {
    (void)port; (void)config;
    return GCL_NO_MODEM;
}

static inline int32_t gcl_close_port(GCL_HANDLE handle) {
    (void)handle;
    return GCL_SUCCESS;
}

static inline int32_t gcl_set_config(GCL_HANDLE handle, GCL_CONFIG* config) {
    (void)handle; (void)config;
    return GCL_ERROR;
}

// Modem operations - Always fail
static inline int32_t gcl_modem_init(GCL_HANDLE handle, GCL_MODEM_INIT* init) {
    (void)handle; (void)init;
    return GCL_NO_MODEM;
}

static inline int32_t gcl_dial(GCL_HANDLE handle, const char* phoneNumber) {
    (void)handle; (void)phoneNumber;
    return GCL_NO_MODEM;
}

static inline int32_t gcl_answer(GCL_HANDLE handle) {
    (void)handle;
    return GCL_NO_MODEM;
}

static inline int32_t gcl_hangup(GCL_HANDLE handle) {
    (void)handle;
    return GCL_SUCCESS; // OK to "hang up" nothing
}

static inline int32_t gcl_get_carrier(GCL_HANDLE handle) {
    (void)handle;
    return 0; // No carrier
}

// Data transfer - Always fail
static inline int32_t gcl_send(GCL_HANDLE handle, const void* data, int32_t length) {
    (void)handle; (void)data; (void)length;
    return GCL_ERROR;
}

static inline int32_t gcl_receive(GCL_HANDLE handle, void* buffer, int32_t maxLength) {
    (void)handle; (void)buffer; (void)maxLength;
    return 0; // No data received
}

static inline int32_t gcl_bytes_available(GCL_HANDLE handle) {
    (void)handle;
    return 0; // No data available
}

static inline int32_t gcl_flush(GCL_HANDLE handle) {
    (void)handle;
    return GCL_SUCCESS;
}

// Status
static inline int32_t gcl_get_error(GCL_HANDLE handle) {
    (void)handle;
    return GCL_NO_MODEM;
}

static inline const char* gcl_error_string(int32_t error) {
    switch (error) {
        case GCL_SUCCESS:          return "Success";
        case GCL_ERROR:            return "Error";
        case GCL_NO_CARRIER:       return "No carrier";
        case GCL_NO_MODEM:         return "Modem not available";
        case GCL_BUSY:             return "Line busy";
        case GCL_NO_DIALTONE:      return "No dial tone";
        case GCL_TIMEOUT:          return "Timeout";
        case GCL_INVALID_PORT:     return "Invalid port";
        case GCL_PORT_IN_USE:      return "Port in use";
        case GCL_NO_MEMORY:        return "Out of memory";
        case GCL_NOT_INITIALIZED:  return "Not initialized";
        default:                   return "Unknown error";
    }
}

#ifdef __cplusplus
}
#endif

// =============================================================================
// Macro stubs for common patterns
// =============================================================================

#define GCL_INIT()           gcl_init()
#define GCL_DIAL(num)        GCL_NO_MODEM
#define GCL_ANSWER()         GCL_NO_MODEM
#define GCL_HANGUP()         GCL_SUCCESS
#define GCL_SEND(d, l)       GCL_ERROR
#define GCL_RECEIVE(b, l)    0

// =============================================================================
// End of GCL Compatibility Header
// =============================================================================

#endif // COMPAT_GCL_H

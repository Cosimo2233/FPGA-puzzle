#ifndef PUZZLE_BRIDGE_H
#define PUZZLE_BRIDGE_H
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define BR_PAYLOAD_MAX 128u
#define BR_RAW_MAX (BR_PAYLOAD_MAX + 8u)
#define BR_WIRE_MAX (2u * BR_RAW_MAX + 2u)
#define BR_LEASE_MS 1000u
#define BR_FRAME_TIMEOUT_MS 100u

enum {
    BR_STATUS = 1, BR_ARM = 2, BR_DISARM = 3, BR_HEARTBEAT = 4,
    BR_DIAGNOSTICS = 5,
    BR_DATA = 0x10, BR_MAGNET = 0x11,
    BR_ACK = 0x80, BR_RX = 0x90
};
enum { BR_OK, BR_BAD_COMMAND, BR_NOT_ARMED, BR_BUSY };
enum { BR_FAULT_NONE, BR_FAULT_TIMEOUT, BR_FAULT_IO, BR_FAULT_TX };

/* Called only in main context. send must enqueue the entire buffer or none.
 * Port 0 is the framed host link; 1/2 are raw motor UARTs.
 * stop cancels pending motor TX, never sends a motor command. */
typedef struct {
    bool (*send)(uint8_t port, const uint8_t *data, size_t size);
    void (*magnet)(bool enabled);
    void (*stop)(void);
} bridge_io_t;

typedef struct {
    bridge_io_t io;
    /* Optional read-only snapshot; existing ten-byte STATUS is unchanged. */
    size_t (*diagnostics)(uint8_t *out, size_t capacity);
    uint8_t raw[BR_RAW_MAX];
    size_t used;
    bool receiving, escaped, armed, magnet_on;
    uint8_t fault, event_seq;
    uint32_t last_heartbeat, last_byte, malformed;
} bridge_t;

uint16_t bridge_crc(const uint8_t *data, size_t size);
size_t bridge_encode(uint8_t *out, uint8_t type, uint8_t channel, uint8_t seq,
                     const uint8_t *payload, size_t size);
void bridge_init(bridge_t *b, bridge_io_t io);
void bridge_tick(bridge_t *b, uint32_t now);
void bridge_fault(bridge_t *b, uint8_t reason);
void bridge_host_byte(bridge_t *b, uint8_t value, uint32_t now);
void bridge_motor_rx(bridge_t *b, uint8_t channel,
                     const uint8_t *data, size_t size);
#endif

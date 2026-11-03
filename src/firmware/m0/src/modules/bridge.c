#include "bridge.h"
#include <string.h>

uint16_t bridge_crc(const uint8_t *data, size_t size)
{
    uint16_t crc = 0xffffu;
    for (size_t i = 0; i < size; ++i) {
        crc ^= (uint16_t)data[i] << 8;
        for (unsigned bit = 0; bit < 8; ++bit)
            crc = (uint16_t)((crc << 1) ^ ((crc & 0x8000u) ? 0x1021u : 0u));
    }
    return crc;
}

size_t bridge_encode(uint8_t *out, uint8_t type, uint8_t channel, uint8_t seq,
                     const uint8_t *payload, size_t size)
{
    uint8_t raw[BR_RAW_MAX];
    if (size > BR_PAYLOAD_MAX || (size && !payload)) return 0;
    raw[0] = 1;
    raw[1] = type;
    raw[2] = channel;
    raw[3] = seq;
    raw[4] = (uint8_t)size;
    raw[5] = 0;
    if (size) memcpy(raw + 6, payload, size);
    uint16_t crc = bridge_crc(raw, size + 6);
    raw[size + 6] = (uint8_t)crc;
    raw[size + 7] = (uint8_t)(crc >> 8);
    size_t n = 0;
    out[n++] = 0x7e;
    for (size_t i = 0; i < size + 8; ++i) {
        if (raw[i] == 0x7e || raw[i] == 0x7d) {
            out[n++] = 0x7d;
            out[n++] = raw[i] ^ 0x20;
        } else {
            out[n++] = raw[i];
        }
    }
    out[n++] = 0x7e;
    return n;
}

void bridge_fault(bridge_t *b, uint8_t reason)
{
    b->armed = false;
    b->magnet_on = false;
    b->fault = reason;
    b->io.magnet(false);
    b->io.stop();
}

void bridge_init(bridge_t *b, bridge_io_t io)
{
    memset(b, 0, sizeof(*b));
    b->io = io;
    bridge_fault(b, BR_FAULT_NONE);
}

void bridge_tick(bridge_t *b, uint32_t now)
{
    if (b->armed && (uint32_t)(now - b->last_heartbeat) >= BR_LEASE_MS)
        bridge_fault(b, BR_FAULT_TIMEOUT);
    if (b->receiving && (b->used || b->escaped) &&
        (uint32_t)(now - b->last_byte) >= BR_FRAME_TIMEOUT_MS) {
        b->receiving = false;
        b->escaped = false;
        b->used = 0;
        ++b->malformed;
    }
}

static void reply(bridge_t *b, uint8_t command, uint8_t channel, uint8_t seq,
                  uint8_t result)
{
    uint8_t payload[BR_PAYLOAD_MAX] = {command, result, b->armed, b->magnet_on, b->fault,
                          1, 0, 0, 0, 0};
    size_t size = 10;
    uint8_t wire[BR_WIRE_MAX];
    for (unsigned i = 0; i < 4; ++i)
        payload[6 + i] = (uint8_t)(b->malformed >> (8 * i));
    if (command == BR_DIAGNOSTICS && result == BR_OK && b->diagnostics) {
        size_t extra = b->diagnostics(payload + size, sizeof(payload) - size);
        if (!extra || extra > sizeof(payload) - size) payload[1] = BR_BAD_COMMAND;
        else size += extra;
    }
    size_t n = bridge_encode(wire, BR_ACK, channel, seq, payload, size);
    if (!b->io.send(0, wire, n)) bridge_fault(b, BR_FAULT_TX);
}

static void dispatch(bridge_t *b, uint32_t now)
{
    uint8_t *p = b->raw;
    if (b->used < 8) { ++b->malformed; return; }
    size_t len = p[4] | ((size_t)p[5] << 8);
    if (p[0] != 1 || len > BR_PAYLOAD_MAX || b->used != len + 8 ||
        bridge_crc(p, len + 6) != (uint16_t)(p[len + 6] | (p[len + 7] << 8))) {
        ++b->malformed;
        return;
    }
    uint8_t type = p[1], channel = p[2], seq = p[3], result = BR_BAD_COMMAND;
    const uint8_t *data = p + 6;
    if (type == BR_DATA) {
        if ((channel == 1 || channel == 2) && len) {
            result = !b->armed ? BR_NOT_ARMED :
                (b->io.send(channel, data, len) ? BR_OK : BR_BUSY);
        }
    } else if (channel == 0) {
        switch (type) {
        case BR_STATUS:
            if (!len) result = BR_OK;
            break;
        case BR_DIAGNOSTICS:
            if (!len && b->diagnostics) result = BR_OK;
            break;
        case BR_ARM:
            if (len == 4 && memcmp(data, "ARM!", 4) == 0) {
                if (b->armed) {
                    result = BR_BUSY;
                } else {
                    bridge_fault(b, BR_FAULT_NONE);
                    b->last_heartbeat = now;
                    b->armed = true;
                    result = BR_OK;
                }
            }
            break;
        case BR_DISARM:
            if (!len) { bridge_fault(b, BR_FAULT_NONE); result = BR_OK; }
            break;
        case BR_HEARTBEAT:
            if (!len) {
                result = b->armed ? BR_OK : BR_NOT_ARMED;
                if (b->armed) b->last_heartbeat = now;
            }
            break;
        case BR_MAGNET:
            if (len == 1 && data[0] <= 1) {
                result = (data[0] && !b->armed) ? BR_NOT_ARMED : BR_OK;
                if (result == BR_OK) {
                    b->magnet_on = data[0] != 0;
                    b->io.magnet(b->magnet_on);
                }
            }
            break;
        default:
            break;
        }
    }
    reply(b, type, channel, seq, result);
}

void bridge_host_byte(bridge_t *b, uint8_t value, uint32_t now)
{
    bridge_tick(b, now); /* Expire BEFORE processing a delayed command. */
    if (value == 0x7e) {
        if (b->receiving && b->used) {
            if (b->escaped) ++b->malformed;
            else dispatch(b, now);
        }
        b->receiving = true;
        b->escaped = false;
        b->used = 0;
    } else if (b->receiving) {
        if (b->escaped) {
            /* Only the two reserved bytes may be escaped. */
            if (value != 0x5e && value != 0x5d) {
                b->receiving = false;
                ++b->malformed;
                return;
            }
            value ^= 0x20;
            b->escaped = false;
        } else if (value == 0x7d) {
            b->escaped = true;
            b->last_byte = now;
            return;
        }
        if (b->used == sizeof(b->raw)) {
            b->receiving = false;
            ++b->malformed;
        } else b->raw[b->used++] = value;
    }
    b->last_byte = now;
}

void bridge_motor_rx(bridge_t *b, uint8_t channel,
                     const uint8_t *data, size_t size)
{
    uint8_t wire[BR_WIRE_MAX];
    if (!size || size > BR_PAYLOAD_MAX || (channel != 1 && channel != 2)) return;
    size_t n = bridge_encode(wire, BR_RX, channel, b->event_seq++, data, size);
    if (!b->io.send(0, wire, n)) bridge_fault(b, BR_FAULT_TX);
}

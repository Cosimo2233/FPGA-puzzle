#include "bridge.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

static bridge_t b;
static uint8_t sent[3][BR_WIRE_MAX];
static size_t sent_size[3];
static unsigned calls[3], stops;
static bool magnet, fail_host, fail_motor;

static bool send_data(uint8_t port, const uint8_t *data, size_t size)
{
    assert(port < 3 && size <= BR_WIRE_MAX);
    if ((port == 0 && fail_host) || (port != 0 && fail_motor)) return false;
    memcpy(sent[port], data, size);
    sent_size[port] = size;
    ++calls[port];
    return true;
}
static void set_magnet(bool on) { magnet = on; }
static void stop(void) { ++stops; }
static void reset(void)
{
    memset(calls, 0, sizeof(calls));
    fail_host = fail_motor = false;
    bridge_init(&b, (bridge_io_t){send_data, set_magnet, stop});
}
static void feed(const uint8_t *wire, size_t len, uint32_t now)
{
    for (size_t i = 0; i < len; ++i) bridge_host_byte(&b, wire[i], now);
}
static void command(uint8_t type, uint8_t channel, const uint8_t *p,
                    size_t len, uint32_t now)
{
    uint8_t wire[BR_WIRE_MAX];
    size_t n = bridge_encode(wire, type, channel, 17, p, len);
    assert(n);
    feed(wire, n, now);
}
static void arm(uint32_t now)
{
    command(BR_ARM, 0, (const uint8_t *)"ARM!", 4, now);
    assert(b.armed && !magnet);
}
static size_t unframe(const uint8_t *wire, size_t n, uint8_t *raw)
{
    assert(n > 2 && wire[0] == 0x7e && wire[n - 1] == 0x7e);
    size_t count = 0;
    for (size_t i = 1; i < n - 1; ++i) {
        uint8_t v = wire[i];
        if (v == 0x7d) v = wire[++i] ^ 0x20;
        raw[count++] = v;
    }
    return count;
}
static uint8_t result(void)
{
    uint8_t raw[BR_RAW_MAX];
    assert(unframe(sent[0], sent_size[0], raw) == 18);
    assert(raw[1] == BR_ACK && raw[3] == 17);
    return raw[7];
}

int main(void)
{
    uint8_t on = 1, off = 0, wire[BR_WIRE_MAX], payload[128], raw[BR_RAW_MAX];
    for (unsigned i = 0; i < sizeof(payload); ++i) payload[i] = (uint8_t)i;
    assert(bridge_crc((const uint8_t *)"123456789", 9) == 0x29b1);
    reset();
    assert(!b.armed && !magnet);
    command(BR_DATA, 1, payload, 4, 0);
    assert(result() == BR_NOT_ARMED && calls[1] == 0);
    command(BR_MAGNET, 0, &on, 1, 0);
    assert(result() == BR_NOT_ARMED && !magnet);
    command(BR_STATUS, 0, NULL, 0, 0);
    assert(result() == BR_OK && !b.armed);
    /* Optional diagnostics must reject cleanly when no hardware hook exists. */
    command(BR_DIAGNOSTICS, 0, NULL, 0, 0);
    assert(result() == BR_BAD_COMMAND && !b.armed && !magnet);

    arm(10);
    for (uint8_t channel = 1; channel <= 2; ++channel) {
        command(BR_DATA, channel, payload, sizeof(payload), 11);
        assert(result() == BR_OK && calls[channel] == 1);
        assert(sent_size[channel] == sizeof(payload));
        assert(memcmp(sent[channel], payload, sizeof(payload)) == 0);
        bridge_motor_rx(&b, channel, payload, sizeof(payload));
        assert(unframe(sent[0], sent_size[0], raw) == BR_RAW_MAX);
        assert(raw[1] == BR_RX && raw[2] == channel);
        assert(memcmp(raw + 6, payload, sizeof(payload)) == 0);
    }
    command(BR_DATA, 3, payload, 4, 12);
    assert(result() == BR_BAD_COMMAND);
    command(BR_MAGNET, 0, &on, 1, 12);
    assert(magnet && result() == BR_OK);
    command(BR_HEARTBEAT, 0, NULL, 0, 900);
    bridge_tick(&b, 1899);
    assert(b.armed && magnet);
    bridge_tick(&b, 1900);
    assert(!b.armed && !magnet && b.fault == BR_FAULT_TIMEOUT);
    command(BR_HEARTBEAT, 0, NULL, 0, 1901);
    assert(result() == BR_NOT_ARMED);
    command(BR_MAGNET, 0, &off, 1, 1902);
    assert(result() == BR_OK);

    reset();
    arm(UINT32_MAX - 500u);
    bridge_tick(&b, 498);
    assert(b.armed);
    bridge_tick(&b, 499);
    assert(!b.armed);
    arm(1000);
    /* A late heartbeat cannot revive an expired session. */
    command(BR_HEARTBEAT, 0, NULL, 0, 2000);
    assert(result() == BR_NOT_ARMED);

    reset();
    arm(0);
    size_t n = bridge_encode(wire, BR_MAGNET, 0, 17, &on, 1);
    wire[2] ^= 1;
    feed(wire, n, 1);
    assert(!magnet && b.malformed == 1);
    n = bridge_encode(wire, BR_MAGNET, 0, 17, &on, 1);
    feed(wire, n / 2, 2);
    bridge_tick(&b, 102);
    feed(wire + n / 2, n - n / 2, 103);
    assert(!magnet);
    command(BR_MAGNET, 0, &on, 1, 104);
    assert(magnet);
    command(BR_DISARM, 0, NULL, 0, 105);
    assert(!magnet && !b.armed);

    reset();
    bridge_host_byte(&b, 0x7e, 0);
    for (unsigned i = 0; i < 1000; ++i) bridge_host_byte(&b, 0x55, 0);
    assert(b.malformed == 1 && !b.armed);
    arm(1);
    /* Every possible byte value survives escaping, including 7d/7e. */
    for (unsigned chunk = 0; chunk < 2; ++chunk) {
        for (unsigned i = 0; i < 128; ++i) payload[i] = (uint8_t)(chunk * 128 + i);
        command(BR_DATA, 1, payload, 128, 2);
        assert(memcmp(sent[1], payload, 128) == 0);
    }
    fail_motor = true;
    unsigned previous = calls[1];
    command(BR_DATA, 1, payload, 10, 3);
    assert(result() == BR_BUSY && calls[1] == previous);
    fail_motor = false;
    command(BR_MAGNET, 0, &on, 1, 4);
    fail_host = true;
    command(BR_STATUS, 0, NULL, 0, 5);
    assert(!b.armed && !magnet && b.fault == BR_FAULT_TX);

    reset();
    arm(0);
    command(BR_MAGNET, 0, &on, 1, 1);
    bridge_fault(&b, BR_FAULT_IO);
    assert(!b.armed && !magnet);
    /* Deterministic random garbage may never arm or drive any motor. */
    reset();
    uint32_t random = 12345;
    for (unsigned i = 0; i < 100000; ++i) {
        random = random * 1664525u + 1013904223u;
        bridge_host_byte(&b, (uint8_t)(random >> 24), i / 10);
    }
    assert(!b.armed && !magnet && !calls[1] && !calls[2]);
    assert(stops > 0);
    puts("PASS: CRC, binary routing, replies, arming, lease/wraparound, corrupt/"
         "oversize/truncated frames, backpressure, faults, 100000 noise bytes");
    return 0;
}

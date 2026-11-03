/* Exercise actual main.c queue/IRQ/fault code against fake hardware registers. */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#define main firmware_main
#include "../src/main.c"
#undef main

UART_Regs fake_uarts[3];
bool fake_magnet;

static void reset(void)
{
    memset(fake_uarts, 0, sizeof(fake_uarts));
    memset(rx, 0, sizeof(rx));
    memset(tx, 0, sizeof(tx));
    io_faults = 0;
    idle_rx_faults = 0;
    memset((void *)&uart_diagnostics, 0, sizeof(uart_diagnostics));
    milliseconds = 0;
    bridge_init(&bridge, (bridge_io_t){enqueue, magnet_output, stop_motors});
    bridge.diagnostics = diagnostic_snapshot;
}

static void error_on(unsigned port)
{
    fake_uarts[port].RXDATA = UART_RXDATA_FRMERR_SET;
    fake_uarts[port].rx_checks = 0;
    uart_rx_irq(port);
}

static void check_status_ack(uint8_t fault)
{
    uint8_t raw[64];
    size_t n = 0;
    bool escaped = false;
    UART_Regs *u = HOST_INST;
    assert(u->sent_count >= 2 && u->sent[0] == 0x7e);
    assert(u->sent[u->sent_count - 1] == 0x7e);
    for (size_t i = 1; i + 1 < u->sent_count; ++i) {
        uint8_t b = u->sent[i];
        if (escaped) { raw[n++] = b ^ 0x20; escaped = false; }
        else if (b == 0x7d) escaped = true;
        else raw[n++] = b;
    }
    assert(n == 18 && !escaped);
    assert(raw[1] == BR_ACK && raw[3] == 7 && raw[4] == 10);
    assert(raw[6] == BR_STATUS && raw[7] == BR_OK);
    assert(raw[8] == 0 && raw[9] == 0 && raw[10] == fault);
    assert(bridge_crc(raw, 16) == (uint16_t)(raw[16] | raw[17] << 8));
}

static uint32_t u32(const uint8_t *p)
{
    return (uint32_t)p[0] | (uint32_t)p[1] << 8 |
        (uint32_t)p[2] << 16 | (uint32_t)p[3] << 24;
}

static void check_diagnostics(void)
{
    reset();
    milliseconds = 0;
    error_on(0); /* first error may happen at time zero */
    service_faults();
    milliseconds = 123;
    bridge.armed = true;
    ROT_INST->RXDATA = UART_RXDATA_OVRERR_SET | UART_RXDATA_BRKERR_SET |
        UART_RXDATA_PARERR_SET | UART_RXDATA_FRMERR_SET;
    ROT_INST->rx_checks = 0;
    rx[2].head = RX_SIZE - 1; /* independently exercise software overflow */
    uart_rx_irq(2);
    service_faults();
    milliseconds = 130;
    error_on(0);
    service_faults();
    uint8_t wire[BR_WIRE_MAX], raw[BR_RAW_MAX];
    size_t size = bridge_encode(wire, BR_DIAGNOSTICS, 0, 42, NULL, 0);
    for (size_t i = 0; i < size; ++i) bridge_host_byte(&bridge, wire[i], 130);
    service_tx();
    size_t used = 0;
    for (size_t i = 1; i + 1 < HOST_INST->sent_count; ++i) {
        uint8_t b = HOST_INST->sent[i];
        if (b == 0x7d) b = HOST_INST->sent[++i] ^ 0x20;
        raw[used++] = b;
    }
    assert(used == 132 && raw[1] == BR_ACK && raw[3] == 42 && raw[4] == 124);
    assert(bridge_crc(raw, used - 2) == (uint16_t)(raw[used - 2] | raw[used - 1] << 8));
    assert(raw[6] == BR_DIAGNOSTICS && raw[7] == BR_OK && raw[10] == BR_FAULT_IO);
    const uint8_t *d = raw + 16;
    assert(d[0] == 1 && d[1] == 5 && u32(d + 2) == 130);
    const uint8_t *host = d + 6, *xyz = host + 36, *rot = xyz + 36;
    assert(u32(host) == 2 && u32(host + 16) == 2);
    assert(u32(host + 24) == 0 && u32(host + 28) == 130 && u32(host + 32) == 0);
    for (unsigned i = 0; i < 36; ++i) assert(xyz[i] == 0);
    for (unsigned i = 0; i < 6; ++i) assert(u32(rot + i * 4) == 1);
    assert(u32(rot + 24) == 123 && u32(rot + 28) == 123 && u32(rot + 32) == 1);
    assert(!bridge.armed && !fake_magnet && bridge.fault == BR_FAULT_IO);
    assert(!XYZ_INST->sent_count && !ROT_INST->sent_count);
    /* Query and fault clearing must not destroy lifetime diagnostic evidence. */
    bridge_fault(&bridge, BR_FAULT_NONE);
    assert(uart_diagnostics.ports[0].events == 2);
    /* Reading diagnostics cannot renew an armed lease or clear its history. */
    bridge.armed = true;
    bridge.last_heartbeat = 100;
    for (size_t i = 0; i < size; ++i) bridge_host_byte(&bridge, wire[i], 150);
    assert(bridge.armed && bridge.last_heartbeat == 100);
    assert(uart_diagnostics.ports[0].events == 2);
    bridge_tick(&bridge, 1100);
    assert(!bridge.armed && bridge.fault == BR_FAULT_TIMEOUT);
    uart_diagnostics.ports[0].events = UINT32_MAX;
    error_on(0);
    assert(uart_diagnostics.ports[0].events == UINT32_MAX);
    assert(diagnostic_snapshot(raw, UART_DIAG_BODY_SIZE - 1) == 0);
}

int main(void)
{
    check_diagnostics();
    uint8_t status[BR_WIRE_MAX];
    size_t size = bridge_encode(status, BR_STATUS, 0, 7, NULL, 0);
    reset();
    /* Floating/unused XYZ RX cannot enqueue bytes or latch a bridge fault. */
    error_on(1);
    assert(!io_faults && rx[1].head == rx[1].tail);
    for (size_t i = 0; i < size; ++i)
        bridge_host_byte(&bridge, status[i], 0);
    service_tx();
    check_status_ack(BR_FAULT_NONE);
    /* TX remains available after every downstream reset. Simulated wire only. */
    stop_motors();
    assert(enqueue(1, (const uint8_t *)"xyz", 3));
    service_tx();
    assert(XYZ_INST->sent_count == 3);
    assert(memcmp(XYZ_INST->sent, "xyz", 3) == 0);
    /* Rotation RX still delivers data to the software queue. */
    ROT_INST->RXDATA = 0x5a;
    ROT_INST->rx_checks = 0;
    uart_rx_irq(2);
    uint8_t received;
    assert(pop_rx(2, &received) && received == 0x5a);
    for (unsigned port = 2; port < 3; ++port) {
        reset();
        bridge.armed = true;
        fake_magnet = true;
        assert(enqueue(1, (const uint8_t *)"motion", 6));
        for (size_t i = 0; i < size / 2; ++i)
            bridge_host_byte(&bridge, status[i], 0);
        error_on(port);
        assert(!fake_magnet);
        assert(!enqueue(2, (const uint8_t *)"motion", 6));
        /* Host bytes arriving while a different UART faults are retained. */
        HOST_INST->RXDATA = status[size / 2];
        HOST_INST->rx_checks = 0;
        uart_rx_irq(0);
        service_faults();
        uint8_t value;
        assert(pop_rx(0, &value));
        bridge_host_byte(&bridge, value, 0);
        for (size_t i = size / 2 + 1; i < size; ++i)
            bridge_host_byte(&bridge, status[i], 0);
        /* Repeated errors while the ACK is queued may not truncate it. */
        for (unsigned i = 0; i < 10; ++i) {
            error_on(port);
            service_faults();
        }
        service_tx();
        check_status_ack(BR_FAULT_IO);
        assert(!bridge.armed && !fake_magnet);
        assert(!XYZ_INST->sent_count && !ROT_INST->sent_count);
    }
    reset();
    for (size_t i = 0; i < size / 2; ++i)
        bridge_host_byte(&bridge, status[i], 0);
    error_on(0);
    service_faults();
    assert(!bridge.receiving && !bridge.used);
    for (size_t i = 0; i < size; ++i)
        bridge_host_byte(&bridge, status[i], 0);
    service_tx();
    check_status_ack(BR_FAULT_NONE);
    /* Idle boot/open-port BREAK/framing must not block the first task. Both
     * ports retain evidence; damaged queues and partial commands are dropped. */
    for (unsigned port = 0; port < 3; port += 2) {
        reset();
        milliseconds = port ? 25 : 23000;
        rx[port].head = 1;
        if (port == 2) motor_used[1] = 1;
        for (size_t i = 0; i < size / 2; ++i)
            bridge_host_byte(&bridge, status[i], milliseconds);
        fake_uarts[port].RXDATA = UART_RXDATA_BRKERR_SET | UART_RXDATA_FRMERR_SET;
        fake_uarts[port].rx_checks = 0;
        uart_rx_irq(port);
        assert(!io_faults && idle_rx_faults == (1u << port));
        service_faults();
        assert(!bridge.armed && !fake_magnet && bridge.fault == BR_FAULT_NONE);
        assert(rx[port].head == rx[port].tail);
        assert(!XYZ_INST->sent_count && !ROT_INST->sent_count);
        if (port == 0) assert(!bridge.receiving && !bridge.used);
        else assert(motor_used[1] == 0);
        assert(uart_diagnostics.ports[port].events == 1);
        assert(uart_diagnostics.ports[port].armed_events == 0);
        uint8_t arm[BR_WIRE_MAX];
        size_t arm_size = bridge_encode(arm, BR_ARM, 0, 8, (const uint8_t *)"ARM!", 4);
        for (size_t i = 0; i < arm_size; ++i)
            bridge_host_byte(&bridge, arm[i], milliseconds);
        assert(bridge.armed && bridge.fault == BR_FAULT_NONE);
        error_on(port); /* Same physical error after ARM must latch. */
        assert(io_faults && !fake_magnet);
        service_faults();
        assert(!bridge.armed && bridge.fault == BR_FAULT_IO);
        error_on(port); /* Disarmed-by-fault is not the healthy idle state. */
        service_faults();
        assert(bridge.fault == BR_FAULT_IO);
        assert(uart_diagnostics.ports[port].armed_events == 1);
    }
    reset();
    error_on(0);
    bridge.armed = true; /* IRQ arrived inside ARM dispatch before assignment. */
    assert(!enqueue(1, (const uint8_t *)"motion", 6));
    service_faults();
    assert(!bridge.armed && bridge.fault == BR_FAULT_IO);
    puts("PASS: idle UART noise is recorded/resynchronized; armed and ARM-boundary errors latch; "
         "downstream IRQ faults preserve host command and complete STATUS ACK; "
         "outputs stop, host RX faults discard partial commands; "
         "XYZ RX ignored, XYZ TX and rotation RX preserved");
    return 0;
}

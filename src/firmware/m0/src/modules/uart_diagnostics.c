#include "uart_diagnostics.h"

static void increment(volatile uint32_t *value)
{
    if (*value != UINT32_MAX) ++*value;
}

void uart_diagnostics_record(volatile uart_diagnostics_t *d, unsigned port,
                             uint32_t flags, uint32_t now, bool armed)
{
    if (port > 2 || !(flags & 31u)) return;
    volatile uart_diagnostic_port_t *p = &d->ports[port];
    if (!p->events) p->first_ms = now;
    increment(&p->events);
    p->last_ms = now;
    if (flags & UART_DIAG_OVERRUN) increment(&p->overrun);
    if (flags & UART_DIAG_BREAK) increment(&p->break_error);
    if (flags & UART_DIAG_PARITY) increment(&p->parity);
    if (flags & UART_DIAG_FRAMING) increment(&p->framing);
    if (flags & UART_DIAG_QUEUE_FULL) increment(&p->queue_full);
    if (armed) increment(&p->armed_events);
}

static void put_u32(uint8_t *out, uint32_t value)
{
    for (unsigned i = 0; i < 4; ++i) out[i] = (uint8_t)(value >> (8u * i));
}

size_t uart_diagnostics_encode(const uart_diagnostics_t *d, uint32_t now,
                              uint8_t *out, size_t capacity)
{
    if (capacity < UART_DIAG_BODY_SIZE) return 0;
    out[0] = 1; /* diagnostic layout revision, independent of frame protocol */
    out[1] = 5; /* HOST and rotation RX enabled, XYZ remains TX-only */
    put_u32(out + 2, now);
    for (unsigned i = 0; i < 3; ++i) {
        const uart_diagnostic_port_t *p = &d->ports[i];
        const uint32_t values[] = {p->events, p->overrun, p->break_error,
            p->parity, p->framing, p->queue_full, p->first_ms, p->last_ms,
            p->armed_events};
        for (unsigned j = 0; j < 9; ++j)
            put_u32(out + 6 + i * 36 + j * 4, values[j]);
    }
    return UART_DIAG_BODY_SIZE;
}

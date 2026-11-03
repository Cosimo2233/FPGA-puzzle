#ifndef PUZZLE_UART_DIAGNOSTICS_H
#define PUZZLE_UART_DIAGNOSTICS_H
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

enum {
    UART_DIAG_OVERRUN = 1u, UART_DIAG_BREAK = 2u,
    UART_DIAG_PARITY = 4u, UART_DIAG_FRAMING = 8u,
    UART_DIAG_QUEUE_FULL = 16u
};
#define UART_DIAG_BODY_SIZE 114u
typedef struct {
    uint32_t events, overrun, break_error, parity, framing, queue_full;
    uint32_t first_ms, last_ms, armed_events;
} uart_diagnostic_port_t;
typedef struct { uart_diagnostic_port_t ports[3]; } uart_diagnostics_t;

/* IRQ writer; caller masks interrupts while copying a snapshot for encoding.
 * Lifetime counters survive DISARM/ARM and clear only on MCU restart. */
void uart_diagnostics_record(volatile uart_diagnostics_t *d, unsigned port,
                             uint32_t flags, uint32_t now, bool armed);
size_t uart_diagnostics_encode(const uart_diagnostics_t *d, uint32_t now,
                              uint8_t *out, size_t capacity);
#endif

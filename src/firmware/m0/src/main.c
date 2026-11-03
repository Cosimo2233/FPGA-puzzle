/*
 * Copyright (c) 2021, Texas Instruments Incorporated
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * *  Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * *  Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * *  Neither the name of Texas Instruments Incorporated nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "ti_msp_dl_config.h"
#include "bridge.h"
#include "uart_diagnostics.h"

#define RX_SIZE 512u
#define TX_SIZE 2048u
#define RX_ERRORS (UART_RXDATA_OVRERR_SET | UART_RXDATA_BRKERR_SET | \
                   UART_RXDATA_PARERR_SET | UART_RXDATA_FRMERR_SET)

typedef struct {
    volatile uint16_t head, tail;
    uint8_t data[RX_SIZE];
} rx_queue_t;
typedef struct {
    uint16_t head, tail;
    uint8_t data[TX_SIZE];
} tx_queue_t;

static UART_Regs *const uarts[3] = {HOST_INST, XYZ_INST, ROT_INST};
static rx_queue_t rx[3];
static tx_queue_t tx[3];
static volatile uint32_t milliseconds;
/* One bit per UART; downstream errors must not destroy host diagnostics. */
static volatile uint8_t io_faults;
/* Idle RX noise requires resynchronization, not a latched motion fault. */
static volatile uint8_t idle_rx_faults;
static bridge_t bridge;
static volatile uart_diagnostics_t uart_diagnostics;
static uint8_t motor_rx[2][64];
static size_t motor_used[2];
static uint32_t motor_sent_at[2];

void SysTick_Handler(void) { ++milliseconds; }

static void magnet_output(bool on)
{
    uint32_t primask = __get_PRIMASK();
    __disable_irq();
    if (on && !io_faults) DL_GPIO_setPins(MAGNET_PORT, MAGNET_OUT_PIN);
    else DL_GPIO_clearPins(MAGNET_PORT, MAGNET_OUT_PIN);
    __set_PRIMASK(primask);
}

static void stop_motors(void)
{
    /* Discard pending TX, including hardware FIFOs. Already transmitted bytes
     * and motion accepted by the motor cannot be recalled by this bridge. */
    uint32_t primask = __get_PRIMASK();
    __disable_irq();
    for (unsigned i = 1; i < 3; ++i) {
        tx[i].head = tx[i].tail = 0;
        rx[i].head = rx[i].tail = 0;
        motor_used[i - 1] = 0;
        DL_UART_Main_reset(uarts[i]);
        DL_UART_Main_enablePower(uarts[i]);
    }
    delay_cycles(POWER_STARTUP_DELAY);
    SYSCFG_DL_XYZ_init();
    SYSCFG_DL_ROT_init();
    /* XYZ remains TX-only even after ARM, DISARM or fault recovery. */
    DL_UART_Main_setRXFIFOThreshold(ROT_INST, DL_UART_RX_FIFO_LEVEL_ONE_ENTRY);
    NVIC_ClearPendingIRQ(ROT_INST_INT_IRQN);
    __set_PRIMASK(primask);
}

static bool enqueue(uint8_t port, const uint8_t *data, size_t size)
{
    if (port > 2 || (port != 0 && (io_faults | idle_rx_faults))) return false;
    tx_queue_t *q = &tx[port];
    size_t free_space = (q->tail - q->head - 1u) & (TX_SIZE - 1u);
    if (size > free_space) return false;
    for (size_t i = 0; i < size; ++i) {
        q->data[q->head] = data[i];
        q->head = (q->head + 1u) & (TX_SIZE - 1u);
    }
    return true;
}

static void uart_rx_irq(unsigned port)
{
    /* No XYZ receive path: PA9 may be unconnected in the actual machine. */
    if (port == 1) return;
    UART_Regs *uart = uarts[port];
    rx_queue_t *q = &rx[port];
    DL_UART_Main_clearInterruptStatus(uart, DL_UART_MAIN_INTERRUPT_RX);
    while (!DL_UART_Main_isRXFIFOEmpty(uart)) {
        /* RXDATA must be read exactly once: it pops data AND error flags. */
        uint32_t word = uart->RXDATA;
        uint16_t next = (q->head + 1u) & (RX_SIZE - 1u);
        if ((word & RX_ERRORS) || next == q->tail) {
            uint32_t flags = 0;
            if (word & UART_RXDATA_OVRERR_SET) flags |= UART_DIAG_OVERRUN;
            if (word & UART_RXDATA_BRKERR_SET) flags |= UART_DIAG_BREAK;
            if (word & UART_RXDATA_PARERR_SET) flags |= UART_DIAG_PARITY;
            if (word & UART_RXDATA_FRMERR_SET) flags |= UART_DIAG_FRAMING;
            if (next == q->tail) flags |= UART_DIAG_QUEUE_FULL;
            uart_diagnostics_record(&uart_diagnostics, port, flags,
                                    milliseconds, bridge.armed);
            if (bridge.armed || bridge.fault != BR_FAULT_NONE || io_faults)
                io_faults |= (uint8_t)(1u << port);
            else
                idle_rx_faults |= (uint8_t)(1u << port);
            DL_GPIO_clearPins(MAGNET_PORT, MAGNET_OUT_PIN);
        } else if (!((io_faults | idle_rx_faults) & (1u << port))) {
            q->data[q->head] = (uint8_t)word;
            __DMB();
            q->head = next;
        }
    }
}

void HOST_INST_IRQHandler(void) { uart_rx_irq(0); }
void ROT_INST_IRQHandler(void) { uart_rx_irq(2); }

static bool pop_rx(unsigned port, uint8_t *value)
{
    rx_queue_t *q = &rx[port];
    if (q->head == q->tail) return false;
    __DMB();
    *value = q->data[q->tail];
    __DMB();
    q->tail = (q->tail + 1u) & (RX_SIZE - 1u);
    return true;
}

static void service_tx(void)
{
    for (unsigned port = 0; port < 3; ++port) {
        tx_queue_t *q = &tx[port];
        while ((port == 0 || !(io_faults | idle_rx_faults)) && q->head != q->tail &&
               !DL_UART_Main_isTXFIFOFull(uarts[port])) {
            DL_UART_Main_transmitData(uarts[port], q->data[q->tail]);
            q->tail = (q->tail + 1u) & (TX_SIZE - 1u);
        }
    }
}

static void service_faults(void)
{
    if (!(io_faults | idle_rx_faults)) return;
    uint32_t primask = __get_PRIMASK();
    __disable_irq();
    uint8_t faults = io_faults;
    uint8_t idle_faults = idle_rx_faults;
    /* An IRQ may interrupt dispatch just before ARM sets armed. Treat that
     * boundary conservatively: pending errors must never survive into motion. */
    if (bridge.armed) faults |= idle_faults;
    if (faults) bridge_fault(&bridge, BR_FAULT_IO);
    /* Before ARM there are no accepted motor commands. Retain lifetime
     * diagnostics, drop damaged RX and require a fresh CRC-valid host frame.
     * Never clear an already latched fault, even after it has disarmed output.
     */
    for (unsigned port = 0; port < 3; ++port) {
        if (idle_faults & (1u << port)) {
            rx[port].tail = rx[port].head;
            if (port) motor_used[port - 1] = 0;
        }
    }
    /* stop_motors has already discarded/reset both downstream ports. Only a
     * HOST RX error invalidates a partially received host command. Preserve
     * queued HOST TX bytes, otherwise repeated downstream faults truncate ACKs.
     * Subsequent STATUS reports the latched fault without enabling anything.
     */
    if ((faults | idle_faults) & 1u) {
        rx[0].tail = rx[0].head;
        bridge.receiving = false;
        bridge.used = 0;
        bridge.escaped = false;
    }
    io_faults = 0;
    idle_rx_faults = 0;
    __set_PRIMASK(primask);
}

static size_t diagnostic_snapshot(uint8_t *out, size_t capacity)
{
    uint32_t primask = __get_PRIMASK();
    __disable_irq();
    uart_diagnostics_t snapshot = uart_diagnostics;
    uint32_t now = milliseconds;
    __set_PRIMASK(primask);
    return uart_diagnostics_encode(&snapshot, now, out, capacity);
}

int main(void)
{
    SYSCFG_DL_init();
    bridge_init(&bridge, (bridge_io_t){enqueue, magnet_output, stop_motors});
    bridge.diagnostics = diagnostic_snapshot;
    /* One-entry threshold also handles 1/3-byte replies without RX timeout. */
    DL_UART_Main_setRXFIFOThreshold(HOST_INST, DL_UART_RX_FIFO_LEVEL_ONE_ENTRY);
    DL_UART_Main_setRXFIFOThreshold(ROT_INST, DL_UART_RX_FIFO_LEVEL_ONE_ENTRY);
    SysTick_Config(CPUCLK_FREQ / 1000u);
    NVIC_ClearPendingIRQ(HOST_INST_INT_IRQN);
    NVIC_EnableIRQ(HOST_INST_INT_IRQN);
    NVIC_EnableIRQ(ROT_INST_INT_IRQN);

    while (1) {
        service_faults();
        bridge_tick(&bridge, milliseconds);
        uint8_t value;
        /* Bounded work per UART prevents one channel starving the others. */
        for (unsigned n = 0; n < 64 && !(io_faults | idle_rx_faults) && pop_rx(0, &value); ++n)
            bridge_host_byte(&bridge, value, milliseconds);
        /* Only the rotation axis returns data; XYZ is send-only. */
        for (unsigned port = 2; port < 3 && !(io_faults | idle_rx_faults); ++port) {
            unsigned index = port - 1;
            while (motor_used[index] < sizeof(motor_rx[index]) &&
                   pop_rx(port, &motor_rx[index][motor_used[index]]))
                ++motor_used[index];
            uint32_t now = milliseconds;
            /* Batch bytes to avoid multiplying USB traffic by frame overhead. */
            if (motor_used[index] && (motor_used[index] == sizeof(motor_rx[index]) ||
                (uint32_t)(now - motor_sent_at[index]) >= 2u)) {
                bridge_motor_rx(&bridge, (uint8_t)port,
                                motor_rx[index], motor_used[index]);
                motor_used[index] = 0;
                motor_sent_at[index] = now;
            }
        }
        service_tx();
    }
}

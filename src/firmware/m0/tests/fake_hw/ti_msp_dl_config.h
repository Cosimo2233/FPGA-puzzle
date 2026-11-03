/* Native fake registers used only by test_uart_faults.c, never target builds. */
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
typedef struct {
    uint32_t RXDATA;
    unsigned rx_checks;
    uint8_t sent[4096];
    size_t sent_count;
} UART_Regs;
extern UART_Regs fake_uarts[3];
extern bool fake_magnet;
#define HOST_INST (&fake_uarts[0])
#define XYZ_INST (&fake_uarts[1])
#define ROT_INST (&fake_uarts[2])
#define HOST_INST_IRQHandler fake_host_irq
#define XYZ_INST_IRQHandler fake_xyz_irq
#define ROT_INST_IRQHandler fake_rot_irq
#define HOST_INST_INT_IRQN 0
#define XYZ_INST_INT_IRQN 1
#define ROT_INST_INT_IRQN 2
#define MAGNET_PORT 0
#define MAGNET_OUT_PIN 1
#define POWER_STARTUP_DELAY 16
#define CPUCLK_FREQ 32000000
#define UART_RXDATA_OVRERR_SET (1u << 8)
#define UART_RXDATA_BRKERR_SET (1u << 9)
#define UART_RXDATA_PARERR_SET (1u << 10)
#define UART_RXDATA_FRMERR_SET (1u << 11)
#define DL_UART_RX_FIFO_LEVEL_ONE_ENTRY 0
#define DL_UART_MAIN_INTERRUPT_RX 0
#define __get_PRIMASK() 0u
#define __disable_irq() ((void)0)
#define __set_PRIMASK(x) ((void)(x))
#define __DMB() ((void)0)
#define delay_cycles(x) ((void)(x))
#define NVIC_ClearPendingIRQ(x) ((void)(x))
#define NVIC_EnableIRQ(x) ((void)(x))
#define SysTick_Config(x) ((void)(x))
#define SYSCFG_DL_init() ((void)0)
#define SYSCFG_DL_XYZ_init() ((void)0)
#define SYSCFG_DL_ROT_init() ((void)0)
#define DL_GPIO_setPins(port, pin) ((void)(port), (void)(pin), fake_magnet = true)
#define DL_GPIO_clearPins(port, pin) ((void)(port), (void)(pin), fake_magnet = false)
#define DL_UART_Main_reset(u) ((void)(u))
#define DL_UART_Main_enablePower(u) ((void)(u))
#define DL_UART_Main_setRXFIFOThreshold(u, n) ((void)(u), (void)(n))
#define DL_UART_Main_clearInterruptStatus(u, n) ((void)(u), (void)(n))
static inline bool DL_UART_Main_isRXFIFOEmpty(UART_Regs *u) {
    return u->rx_checks++ != 0;
}
static inline bool DL_UART_Main_isTXFIFOFull(UART_Regs *u) {
    (void)u;
    return false;
}
static inline void DL_UART_Main_transmitData(UART_Regs *u, uint8_t byte) {
    if (u->sent_count < sizeof(u->sent)) u->sent[u->sent_count++] = byte;
}

#include <ts7200.h>
#include <kernel/uart.h>

void initUART()
{
    // Note: high registers must be written last
    int *uart1Mid  = (int *)(UART1_BASE + UART_LCRM_OFFSET);
    int *uart1Low  = (int *)(UART1_BASE + UART_LCRL_OFFSET);
    int *uart1High = (int *)(UART1_BASE + UART_LCRH_OFFSET);
    int *uart1Ctrl = (int *)(UART1_BASE + UART_CTLR_OFFSET);
    int *uart2High = (int *)(UART2_BASE + UART_LCRH_OFFSET);
    int *uart2Ctrl = (int *)(UART2_BASE + UART_CTLR_OFFSET);

    // Set uart1 speed to 2400
    *uart1Low = 191;
    *uart1Mid = 0x0;

    // Enable uart 1,2 modem, rcv, xmit interrupts
    *uart2Ctrl |= TIEN_MASK | RIEN_MASK | MSIEN_MASK;

    // Set uart1 to 2 stop bits + no fifo
    int temp = *uart1High;
    temp = (temp | STP2_MASK) & ~FEN_MASK ;
    *uart1High = temp;

    // Set uart2 to no fifo
    temp = *uart2High;
    temp = temp & ~FEN_MASK;
    *uart2High = temp;
}

unsigned int getUARTStatus(int port)
{
    switch (port) {
    case 0:
        return *(unsigned int *)(UART1_BASE + UART_INTR_OFFSET);
    case 1:
        return *(unsigned int *)(UART2_BASE + UART_INTR_OFFSET);
    default:
        return 0;
    }
}

char getUARTData(int port)
{
    switch (port) {
    case 0:
        return *(char *)(UART1_BASE + UART_DATA_OFFSET);
    case 1:
        return *(char *)(UART2_BASE + UART_DATA_OFFSET);
    default:
        return '\0';
    }
}

void resetUART()
{
    *(int *)(UART1_BASE + UART_CTLR_OFFSET) = 0;
    *(int *)(UART2_BASE + UART_CTLR_OFFSET) = 0;
}

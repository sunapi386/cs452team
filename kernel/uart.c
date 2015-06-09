#include <ts7200.h>
#include <kernel/uart.h>

void initUART()
{
    // Note: high registers must be written last
    int *uart1Mid  = (int *)(UART1_BASE + UART_LCRM_OFFSET);
    int *uart1Low  = (int *)(UART1_BASE + UART_LCRL_OFFSET);
    int *uart1High = (int *)(UART1_BASE + UART_LCRH_OFFSET);
    int *uart2High = (int *)(UART2_BASE + UART_LCRH_OFFSET);

    // Set uart1 speed to 2400
    *uart1Low = 191;
    *uart1Mid = 0x0;

    // Set uart1 to 2 stop bits + no fifo
    int temp = *uart1High;
    temp = (temp | STP2_MASK) & ~FEN_MASK ;
    *uart1High = temp;

    // Set uart2 to no fifo
    temp = *uart2High;
    temp = temp & ~FEN_MASK;
    *uart2High = temp;
}

void resetUART()
{

}

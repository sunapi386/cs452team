#include <ts7200.h>
#include <events.h>
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
    *uart1Low = 0xbf;
    *uart1Mid = 0x0;
    *uart1Ctrl = *uart1Ctrl | MSIEN_MASK;// | RIEN_MASK ;

    // Enable uart 2 rcv, xmit interrupts
    //*uart2Ctrl |= TIEN_MASK | RIEN_MASK;// | MSIEN_MASK;
    // Set uart1 to 2 stop bits + no fifo
    int temp = *uart1High;
    temp = (temp | STP2_MASK) & ~FEN_MASK ;
    *uart1High = temp;
    // Set uart2 to no fifo
    temp = *uart2High;
    temp = temp & ~FEN_MASK;
    *uart2High = temp;
}

void setUARTCtrl(int event, int val)
{
    int *ctrl = 0;
    int mask = 0;

    // set control parameters
    switch (event) {
    case UART1_RECV_EVENT:
        ctrl = (int *)(UART1_BASE + UART_CTLR_OFFSET);
        mask = RIEN_MASK;
        break;
    case UART1_XMIT_EVENT:
        ctrl = (int *)(UART1_BASE + UART_CTLR_OFFSET);
        mask = TIEN_MASK;
        break;
    case UART2_RECV_EVENT:
        ctrl = (int *)(UART2_BASE + UART_CTLR_OFFSET);
        mask = RIEN_MASK;
        break;
    case UART2_XMIT_EVENT:
        ctrl = (int *)(UART2_BASE + UART_CTLR_OFFSET);
        mask = TIEN_MASK;
        break;
    default:
        return;
    }

    // read the control
    int temp = *ctrl;

    // negate the bit if val is 0; else assert it
    *ctrl = (val == 0) ? (temp & ~mask) : (temp | mask);
}

int getUARTIntStatus(int port)
{
    switch (port) {
    case 0:
        return *(int *)(UART1_BASE + UART_INTR_OFFSET);
    case 1:
        return *(int *)(UART2_BASE + UART_INTR_OFFSET);
    default:
        return 0;
    }
}

char getUARTData(int port)
{
    switch (port) {
    case 0:
        return (char)(*(int *)(UART1_BASE + UART_DATA_OFFSET));
    case 1:
        return (char)(*(int *)(UART2_BASE + UART_DATA_OFFSET));
    default:
        return '\0';
    }
}

void resetUART()
{
    *(int *)(UART1_BASE + UART_CTLR_OFFSET) = 0;
    *(int *)(UART2_BASE + UART_CTLR_OFFSET) = 0;
}

/*
    Modem interrupt is specific to UART1
*/
int getUART1ModemStatus()
{
    return *(int *)(UART1_BASE + UART_MDMSTS_OFFSET);
}

void enableUART1ModemInterrupt()
{
    clearUART1ModemInterrupt();

    int temp = *(int *)(UART1_BASE + UART_CTLR_OFFSET);
    *(int *)(UART1_BASE + UART_CTLR_OFFSET) = temp | MSIEN_MASK;
}

void clearUART1ModemInterrupt()
{
    // Modem interrupt is cleared by writing
    // anything to UART1IntIDIntClr register
    *(int *)(UART1_BASE + UART_INTR_OFFSET) = 0;
}

void disableUART1ModemInterrupt()
{
    clearUART1ModemInterrupt();

    // Disable the interrupt in UART unit
    int temp = *(int *)(UART1_BASE + UART_CTLR_OFFSET);
    *(int *)(UART1_BASE + UART_CTLR_OFFSET) = temp & ~MSIEN_MASK;
}

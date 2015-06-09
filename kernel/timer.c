#include <ts7200.h>
#include <kernel/timer.h>

void initTimer() {
    clearTimerInterrupt();

	// 10 ms interval for 508kHz
    *(int *)(TIMER3_BASE + LDR_OFFSET) = 5080;

    // 508kHz | enable | periodic mode
	*(int *)(TIMER3_BASE + CRTL_OFFSET)
        = CLKSEL_MASK | ENABLE_MASK | MODE_MASK;
}

void clearTimerInterrupt()
{
    // Write anything to CLR_OFFSET to clear interrupt
    *(int *)(TIMER3_BASE + CLR_OFFSET) = 1;
}

void resetTimer()
{
    clearTimerInterrupt();
    *(int *)(TIMER3_BASE + CRTL_OFFSET) = 0;
}

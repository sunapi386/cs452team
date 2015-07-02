#include <ts7200.h>
#include <kernel/timer.h>

void initTimer() {
    clearTimerInterrupt();

	// 10 ms per interrupt, timer 3 runs at 508.4689KHz.
    *(int *)(TIMER3_BASE + LDR_OFFSET) = 5084;

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

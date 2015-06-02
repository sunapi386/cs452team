#include <ts7200.h>
#include <kernel/timer.h>

void InitTimer(volatile unsigned int **count) {
	volatile unsigned int *ctrl = (unsigned int *)(TIMER3_BASE + CRTL_OFFSET);
	volatile unsigned int *load = (unsigned int *)(TIMER3_BASE + LDR_OFFSET);
	*count = (unsigned int *)(TIMER3_BASE + VAL_OFFSET);
	*load = 0xffffffff;
	*ctrl = CLKSEL_MASK | ENABLE_MASK;
}

void UpdateTimer(unsigned int *minutes, unsigned int *seconds, unsigned int *ticks) {
	if (*ticks + 1 < 10)
	{
		++*ticks;
	}
	else
	{
		*ticks = 0;
		if (*seconds + 1 < 60)
		{
			++*seconds;
		}
		else
		{
			*seconds = 0;
			++*minutes;
		}
	}
}


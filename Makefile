#
# Makefile for kernel-side
#
CC     = gcc
CFLAGS  = -c -fPIC -Wall -Wextra -mcpu=arm920t -msoft-float -std=gnu99 -I. -Iinclude
# -g: include hooks for gdb
# -c: only compile
# -mcpu=arm920t: generate code for the 920t architecture
# -fpic: emit position-independent code
# -Wall: report all warnings
# -msoft-float: use software for floating point

AS	= as
ASFLAGS	= -mcpu=arm920t -mapcs-32
# -mapcs-32: always create a complete stack frame


LD  = ld
LDFLAGS = -init main -Map kernel.map -N -T linker.ld -L/u/wbcowan/gnuarm-4.0.2/lib/gcc/arm-elf/4.0.2

# .PRECIOUS: if make is interrupted during execution, the target is not deleted.
.PRECIOUS: %.s

# .PHONY: make will run its recipe unconditionally
.PHONY: all clean


all = kernel.elf

sources := $(wildcard *.c)
assembled_sources := $(patsubst %c,%s,$(sources))
hand_assemblies := $(filter-out $(assembled_sources),$(wildcard *.s))
objects := $(patsubst %.c,%.o,$(sources)) $(patsubst %.s,%.o,$(hand_assemblies))

kernel.elf: $(objects) linker.ld
	$(LD) $(LDFLAGS) -o $@ $(filter-out linker.ld,$^) -lgcc
	cp kernel.elf /u/cs452/tftp/ARM/${USER}/k1.elf
	chmod 755 /u/cs452/tftp/ARM/${USER}/k1.elf

%.s: %.c
	$(CC) -S $(CFLAGS) $<

%.o: %.s
	$(AS) $(ASFLAGS) -o $@ $<

%.d : %.c
	@set -e; rm -f $@; \
	    $(CC) -MM $(CFLAGS) $< | sed 's,\($*\)\.o[ :]*,\1.s $@ : ,g' > $@;

clean:
	-rm -f kernel.elf $(objects) $(assembled_sources) $(sources:.c=.d) kernel.map

-include $(sources:.c=.d)

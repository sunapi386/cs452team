#
# Makefile for kernel-side
#
XCC     = gcc
AS	= as
AR	= ar
LD  = ld
CFLAGS  = -O2 -c -fPIC -Wall -mcpu=arm920t -msoft-float -I. -I./include -I./src
# -g: include hooks for gdb
# -c: only compile
# -mcpu=arm920t: generate code for the 920t architecture
# -fpic: emit position-independent code
# -Wall: report all warnings
# -msoft-float: use software for floating point

ASFLAGS	= -mcpu=arm920t -mapcs-32
# -mapcs-32: always create a complete stack frame

ARFLAGS = rcs

LDFLAGS = -init main -Map kernel.map -N  -T orex.ld -L/u/wbcowan/gnuarm-4.0.2/lib/gcc/arm-elf/4.0.2 -L../lib

all: kernel.s kernel.elf


###################################
# bwio
#

bwio.s: src/bwio.c
	$(XCC) -S $(CFLAGS) src/bwio.c

bwio.o: bwio.s
	$(AS) $(ASFLAGS) -o bwio.o bwio.s


###################################
# scheduler
#

scheduler.s: src/scheduler.c
	$(XCC) -S $(CFLAGS) src/scheduler.c

scheduler.o: scheduler.s
	$(AS) $(ASFLAGS) -o scheduler.o scheduler.s


###################################
# kernel
#

kernel.s: src/kernel.c
	$(XCC) -S $(CFLAGS) src/kernel.c

kernel.o: kernel.s
	$(AS) $(ASFLAGS) -o kernel.o kernel.s

kernel.elf: kernel.o bwio.o
	# $(LD) $(LDFLAGS) -o $@ kernel.o $(LIBS) # libraries -lbwio
	$(LD) $(LDFLAGS) -o $@ kernel.o bwio.o -lgcc



clean:
	-rm -f *.s *.a *.o

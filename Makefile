#
# Makefile for kernel-side
#
CC     = gcc
CFLAGS  = -O2 -c -fPIC -fno-builtin -Wall -Wextra -mcpu=arm920t -msoft-float -mpoke-function-name -std=gnu99 -I. -Iinclude
# -c: only compile
# -mcpu=arm920t: generate code for the 920t architecture
# -fpic: emit position-independent code
# -Wall: report all warnings
# -msoft-float: use software for floating point
# -mpoke-function-name: https://gcc.gnu.org/onlinedocs/gcc-4.1.2/gcc/ARM-Options.html

AS	= as
ASFLAGS	= -mcpu=arm920t -mapcs-32
# -mapcs-32: always create a complete stack frame


LD  = ld
LDFLAGS = -init main -Map kernel.map -N -T linker.ld -L/u/wbcowan/gnuarm-4.0.2/lib/gcc/arm-elf/4.0.2

# .PRECIOUS: if make is interrupted during execution, the target is not deleted.
.DEFAULT_GOAL := deploy
.PRECIOUS: %.s
# .PHONY: make will run its recipe unconditionally
.PHONY: clean deploy
src_dirs = kernel user
vpath %.h include include/kernel include/user
vpath %.c $(src_dirs)
generated_sources = user/track_data.c
generated_headers = include/user/track_data.h
sources := $(foreach sdir,$(src_dirs),$(wildcard $(sdir)/*.c)) $(generated_sources)
assembled_sources := $(patsubst %c,%s,$(sources))
# hand_assemblies := $(filter-out $(assembled_sources),$(wildcard *.s))
hand_assemblies := kernel/context_switch.s
objects := $(patsubst %.s,%.o,$(hand_assemblies)) $(patsubst %.c,%.o,$(sources))

deploy: kernel.elf
	install -m 755 -g cs452_sf kernel.elf /u/cs452/tftp/ARM/sc/${USER}.elf

kernel.elf: $(objects) linker.ld
	$(LD) $(LDFLAGS) -o $@ $(filter-out linker.ld,$^) -lgcc

user/track_data.c: track/parse_track.py track/tracka track/trackb
	track/parse_track.py -C user/track_data.c -H include/user/track_data.h track/tracka track/trackb

%.s: %.c
	$(CC) -o $@ -S $(CFLAGS) $<

%.o: %.s
	$(AS) $(ASFLAGS) -o $@ $<

%.d : %.c
	@set -e; rm -f $@; \
	    $(CC) -MM $(CFLAGS) $< | sed 's,\($*\)\.o[ :]*,\1.s $@ : ,g' > $@;

clean:
	-rm -f kernel.elf $(objects) $(assembled_sources) $(sources:.c=.d) kernel.map $(generated_sources) $(generated_headers)

prod: clean check
	make CFLAGS="$(CFLAGS) -DPRODUCTION"


k5: prod
	install -m 755 -g cs452_sf kernel.elf /u/cs452/tftp/ARM/sc/k5.elf

check:
	git grep -n FIXME
	git grep -n TODO

-include $(sources:.c=.d)

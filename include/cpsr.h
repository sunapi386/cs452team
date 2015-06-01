#ifndef __CPSR_H
#define __CPSR_H

#define NegativeMask (1<<31)
#define ZeroMask (1<<30)
#define CarryMask (1<<29)
#define OverflowMask (1<<28)

#define ModeMask 0x1f
    #define UserMode 0x10
    #define FIQMode 0x11
    #define IRQMode 0x12
    #define SprMode 0x13
    #define AbtMode 0x17
    #define UdfMode 0x1b
    #define SysMode 0x1f

#define DisableIRQ (1<<7)
#define DisableFIQ (1<<6)


#define Thumb (1<<5)

#endif /*_ CPSR_H */

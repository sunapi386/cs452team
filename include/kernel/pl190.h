#ifndef __PL190_H
#define __PL190_H

#define VIC1 0x800B0000
#define VIC2 0x800C0000

#define IRQ_STATUS     0x00
#define RAW_INTR       0x08
#define INT_SELECT     0x0c // Wirte 0 for IRQ
#define INT_ENABLE     0x10 // 1 enable, 0 disable
#define INT_CLEAR      0x14
#define SOFT_INT       0x18
#define SOFT_INT_CLEAR 0x1c
#define VECT_ADDR      0x30

#endif// __PL190_H

#ifndef __PL190_H
#define __PL190_H

// Refer to EP93xx_User_Guide_UM1.pdf page 165-170

#define VIC1 0x800B0000
#define VIC2 0x800C0000

#define VIC_IRQ_STATUS      0x00
#define VIC_RAW_INTR        0x08
#define VIC_INT_SELECT      0x0c // Wirte 0 for IRQ
#define VIC_INT_ENABLE      0x10 // 1 enable, 0 disable
#define VIC_INT_CLEAR       0x14
#define VIC_SOFT_INT        0x18
#define VIC_SOFT_INT_CLEAR  0x1c
#define VIC_VECT_ADDR       0x30

//

#endif// __PL190_H

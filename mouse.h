#ifndef __MOUSE_H
#define __MOUSE_H

#include "types.h"
#include "interrupts.h"
#include "port.h"
class MouseDriver : public InterruptHandler
{
    Port8Bit dataport;      // port 0x60: the scancode arrives here
    Port8Bit commandport;   // port 0x64: status / commands

    uint8_t buffer[3];        // mouse sends 3 bytes for every event
    uint8_t offset;          // how many bytes have we received so far?
    uint8_t buttons;         // which buttons are currently pressed?

public:
    MouseDriver(InterruptManager* manager);
    ~MouseDriver();
    virtual uint32_t HandleInterrupt(uint32_t esp);
};

#endif
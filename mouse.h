#ifndef __MOUSE_H
#define __MOUSE_H

#include "types.h"
#include "port.h"
#include "driver.h"
#include "interrupts.h"

// Anything that wants mouse events (a console cursor, a desktop, ...)
// inherits from this and overrides what it cares about.
//   button: 1 = left, 2 = right, 3 = middle
//   dx > 0 = moved right, dy > 0 = moved DOWN (screen direction)
class MouseEventHandler
{
    public:
        MouseEventHandler();

        virtual void OnActivate();
        virtual void OnMouseDown(uint8_t button);
        virtual void OnMouseUp(uint8_t button);
        virtual void OnMouseMove(int dx, int dy);
};

class MouseDriver : public InterruptHandler, public Driver
{
    Port8Bit dataport;      // port 0x60: the mouse bytes arrive here
    Port8Bit commandport;   // port 0x64: status / commands

    uint8_t buffer[3];      // mouse sends 3 bytes for every event
    uint8_t offset;         // how many bytes have we received so far?
    uint8_t buttons;        // which buttons are currently pressed?

    MouseEventHandler* handler;

public:
    MouseDriver(InterruptManager* manager, MouseEventHandler* handler);
    ~MouseDriver();
    virtual uint32_t HandleInterrupt(uint32_t esp);
    virtual void Activate();
};

#endif
#ifndef __KEYBOARD_H
#define __KEYBOARD_H

#include "types.h"
#include "interrupts.h"
#include "driver.h"
#include "port.h"

class KeyboardEventHandler
{
    public:
        KeyboardEventHandler();

        virtual void OnKeyDown(char);
        virtual void OnKeyUp(char);
};


// Keyboard driver. It is an InterruptHandler for interrupt 0x21 (IRQ1),
// so HandleInterrupt() runs every time a key is pressed or released.
class KeyboardDriver : public InterruptHandler, public Driver
{
    Port8Bit dataport;      // port 0x60: the scancode arrives here
    Port8Bit commandport;   // port 0x64: status / commands
    
    bool shiftHeld;         // is Shift being held down?
    bool capsLock;          // is Caps Lock switched on?
    bool extended;          // did we just see the 0xE0 prefix?

    KeyboardEventHandler* handler; 
public:
    KeyboardDriver(InterruptManager* manager, KeyboardEventHandler* handler);
    ~KeyboardDriver();
    virtual uint32_t HandleInterrupt(uint32_t esp);
    virtual void Activate();
};

#endif
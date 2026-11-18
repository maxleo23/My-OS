#ifndef __KEYBOARD_H
#define __KEYBOARD_H

#include "types.h"

// Called from the IRQ1 handler. Reads port 0x60, decodes the scancode
// and prints the character on screen.
void HandleKeyboardInterrupt();

#endif
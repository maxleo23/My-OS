#ifndef __MOUSE_H
#define __MOUSE_H

#include "types.h"

// Called from the IRQ12 handler. Reads port 0x60, decodes the mouse packet
void HandleMouseInterrupt();

#endif
// and prints the character on screen.
void HandleKeyboardInterrupt();

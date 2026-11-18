
#include "keyboard.h"
#include "port.h"
 
void printf(char* str);
 
// Scancode set 1 -> ASCII, indices 0x00..0x39. '\0' = no printable character.
static const char normalMap[] =
    "\0\0"              // 00, 01 (Esc)
    "1234567890-="      // 02-0D
    "\b\t"              // 0E Backspace, 0F Tab
    "qwertyuiop[]"      // 10-1B
    "\n\0"              // 1C Enter, 1D Ctrl
    "asdfghjkl;'`"      // 1E-29
    "\0\\"              // 2A LShift, 2B
    "zxcvbnm,./"        // 2C-35
    "\0*\0 ";           // 36 RShift, 37 keypad *, 38 Alt, 39 Space
 
static const char shiftMap[] =
    "\0\0"
    "!@#$%^&*()_+"
    "\b\t"
    "QWERTYUIOP{}"
    "\n\0"
    "ASDFGHJKL:\"~"
    "\0|"
    "ZXCVBNM<>?"
    "\0*\0 ";
 
static_assert(sizeof(normalMap) == 0x3A + 1, "normalMap must cover 0x00-0x39");
static_assert(sizeof(shiftMap)  == 0x3A + 1, "shiftMap must cover 0x00-0x39");
 
static Port8Bit dataPort(0x60);
 
static bool shiftHeld = false;
static bool capsLock  = false;
static bool extended  = false;   // previous byte was the 0xE0 prefix
 
void HandleKeyboardInterrupt()
{
    uint8_t scancode = dataPort.Read();   // must always be read
 
    // 0xE0 prefix = extended key (arrows, right ctrl, ...). Skip it.
    if(scancode == 0xE0) { extended = true; return; }
    if(extended)          { extended = false; return; }
 
    bool released = scancode & 0x80;
    uint8_t key = scancode & 0x7F;
 
    // Shift (0x2A left, 0x36 right)
    if(key == 0x2A || key == 0x36) { shiftHeld = !released; return; }
 
    // Caps Lock toggles on press
    if(key == 0x3A) { if(!released) capsLock = !capsLock; return; }
 
    if(released || key > 0x39) return;
 
    char c = (shiftHeld ? shiftMap : normalMap)[key];
 
    // Caps Lock only flips letters
    if(capsLock)
    {
        if(c >= 'a' && c <= 'z')      c -= 32;
        else if(c >= 'A' && c <= 'Z') c += 32;
    }
 
    if(c == 0) return;
 
    char out[2] = { c, '\0' };
    printf(out);
}
 
#include "keyboard.h"

void printf(char* str);

// Two lookup tables: "scancode number -> character".
// The keyboard does not send letters, it sends numbers (scancodes).
// Example: pressing 'A' sends 0x1E, so normalMap[0x1E] is 'a'.
// '\0' means "this key has no character" (Ctrl, Alt, Esc, ...).

// Used when Shift is NOT held.
static const char normalMap[] =
    "\0\0"              // 0x00, 0x01 (Esc)
    "1234567890-="      // 0x02 - 0x0D
    "\b\t"              // 0x0E Backspace, 0x0F Tab
    "qwertyuiop[]"      // 0x10 - 0x1B
    "\n\0"              // 0x1C Enter, 0x1D Ctrl
    "asdfghjkl;'`"      // 0x1E - 0x29
    "\0\\"              // 0x2A Left Shift, 0x2B
    "zxcvbnm,./"        // 0x2C - 0x35
    "\0*\0 ";           // 0x36 Right Shift, 0x37 keypad *, 0x38 Alt, 0x39 Space

// Used when Shift IS held.
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


KeyboardDriver::KeyboardDriver(InterruptManager* manager)
: InterruptHandler(0x21, manager),   // "call me for interrupt 0x21"
  dataport(0x60),
  commandport(0x64)
{
    shiftHeld = false;
    capsLock  = false;
    extended  = false;

    // Throw away any old bytes already waiting in the keyboard buffer.
    while(commandport.Read() & 0x01)
        dataport.Read();
}

KeyboardDriver::~KeyboardDriver()
{
}

// Runs every time the keyboard sends something.
uint32_t KeyboardDriver::HandleInterrupt(uint32_t esp)
{
    // Step 1: read the scancode. This is REQUIRED. If we don't read it,
    // the keyboard will never send us another key.
    uint8_t scancode = dataport.Read();

    // Step 2: skip extended keys (arrows, right Ctrl, ...). They send 0xE0
    // first, then a second byte. We ignore both.
    if(scancode == 0xE0) { extended = true; return esp; }
    if(extended)         { extended = false; return esp; }

    // Step 3: the top bit (0x80) means "key was RELEASED".
    // The lower 7 bits are the key itself.
    bool released = scancode & 0x80;
    uint8_t key   = scancode & 0x7F;

    // Step 4: Shift keys. Remember whether they are down or up.
    if(key == 0x2A || key == 0x36)
    {
        shiftHeld = !released;
        return esp;
    }

    // Step 5: Caps Lock toggles every time it is pressed.
    if(key == 0x3A)
    {
        if(!released) capsLock = !capsLock;
        return esp;
    }

    // Step 6: we only print when a key is pressed (not released),
    // and only for keys our tables know about.
    if(released || key > 0x39)
        return esp;

    // Step 7: look up the character (use the shift table if Shift is held).
    char c = shiftHeld ? shiftMap[key] : normalMap[key];

    // Step 8: Caps Lock flips the case of letters only.
    if(capsLock)
    {
        if(c >= 'a' && c <= 'z')      c -= 32;   // make uppercase
        else if(c >= 'A' && c <= 'Z') c += 32;   // make lowercase
    }

    // Step 9: print it (skip keys with no character).
    if(c != 0)
    {
        char text[2] = { c, '\0' };
        printf(text);
    }

    return esp;
}
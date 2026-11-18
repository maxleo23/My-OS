#include "types.h"
#include "gdt.h"
#include "interrupts.h"
#include "driver.h"
#include "keyboard.h"
#include "mouse.h"

// ---- mouse cursor on the text screen ---------------------------------------
// The cursor is drawn by swapping the foreground/background colours of one
// cell. printf() hides it while scrolling so the inverted cell does not
// get copied up with the text.
static int  cursorX = 40, cursorY = 12;
static bool cursorShown = false;

static void InvertCell(int x, int y)
{
    uint16_t* VideoMemory = (uint16_t*)0xb8000;
    uint16_t cell = VideoMemory[80*y+x];
    VideoMemory[80*y+x] = ((cell & 0xF000) >> 4)
                        | ((cell & 0x0F00) << 4)
                        | ( cell & 0x00FF);
}

static void HideCursor()
{
    if(cursorShown) { InvertCell(cursorX, cursorY); cursorShown = false; }
}

static void ShowCursor()
{
    if(!cursorShown) { InvertCell(cursorX, cursorY); cursorShown = true; }
}

// ---- text output -----------------------------------------------------------
void printf(char* str)
{
    static uint16_t* VideoMemory = (uint16_t*)0xb8000;
    static int x = 0, y = 0;

    for(int i = 0; str[i] != '\0'; ++i)
    {
        if(str[i] == '\n')
        {
            y++; x = 0;
        }
        else if(str[i] == '\b')
        {
            if(x > 0)      x--;
            else if(y > 0) { y--; x = 79; }   // wrap back to the end of the previous line
            else           continue;          // top-left corner: nothing to erase
            VideoMemory[80*y+x] = (VideoMemory[80*y+x] & 0xFF00) | ' ';
        }
        else if(str[i] == '\t')
        {
            x = (x + 4) & ~3;
        }
        else
        {
            VideoMemory[80*y+x] = (VideoMemory[80*y+x] & 0xFF00) | (uint8_t)str[i];
            x++;
        }

        if(x >= 80) { y++; x = 0; }

        if(y >= 25)
        {
            // Scroll: move every row up by one and blank the last row.
            bool wasShown = cursorShown;
            HideCursor();

            for(int row = 1; row < 25; row++)
                for(int col = 0; col < 80; col++)
                    VideoMemory[80*(row-1)+col] = VideoMemory[80*row+col];

            for(int col = 0; col < 80; col++)
                VideoMemory[80*24+col] = (VideoMemory[80*24+col] & 0xFF00) | ' ';

            y = 24;
            if(wasShown) ShowCursor();
        }
    }
}

void clearScreen()
{
    uint16_t* VideoMemory = (uint16_t*)0xb8000;
    for(int i = 0; i < 80*25; i++)
        VideoMemory[i] = 0x0720;
    cursorShown = false;
}

typedef void (*constructor)();
extern "C" constructor start_ctors;
extern "C" constructor end_ctors;
extern "C" void callConstructors() {
    for(constructor* i = &start_ctors; i != &end_ctors; i++)
    (*i)();
}


// Temporary listener: prints every key press, exactly like the old driver did.
// Later, replace this with your desktop / window manager.
class PrintfKeyboardEventHandler : public KeyboardEventHandler
{
public:
    void OnKeyDown(char c)
    {
        char text[2] = { c, '\0' };
        printf(text);
    }
};

// Temporary listener: moves the on-screen cursor. Later, replace this with
// your desktop / window manager (OnMouseDown / OnMouseUp are already called).
class MouseToConsole : public MouseEventHandler
{
public:
    void OnActivate()
    {
        ShowCursor();
    }

    void OnMouseMove(int dx, int dy)
    {
        HideCursor();
        cursorX += dx;
        cursorY += dy;
        if(cursorX < 0)  cursorX = 0;
        if(cursorX >= 80) cursorX = 79;
        if(cursorY < 0)  cursorY = 0;
        if(cursorY >= 25) cursorY = 24;
        ShowCursor();
    }
};

extern "C" void kernelMain(const void* multiboot_structure, uint32_t /*multiboot_magic*/)
{   
    clearScreen();
    
    printf("Hello World!\n");
    
    GlobalDescriptorTable gdt;
    InterruptManager interrupts(&gdt);

    printf("initializing Hardware, stage 1\n");
    
    DriverManager drvManager;

        PrintfKeyboardEventHandler kbhandler;
        KeyboardDriver keyboard(&interrupts, &kbhandler);
        drvManager.AddDriver(&keyboard);

        MouseToConsole mousehandler;
        MouseDriver mouse(&interrupts, &mousehandler);
        drvManager.AddDriver(&mouse);

    printf("initializing Hardware, stage 2\n");
        drvManager.ActivateAll();

    printf("initializing Hardware, stage 3\n");
    interrupts.Activate();     // now allow interrupts to happen

    while(1);
}
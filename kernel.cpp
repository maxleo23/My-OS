#include "types.h"
#include "gdt.h"
#include "interrupts.h"
#include "keyboard.h"
#include "mouse.h"

void printf(char* str)
{
    static uint16_t* VideoMemory = (uint16_t*)0xb8000;
    static uint8_t x = 0, y = 0;

    for(int i = 0; str[i] != '\0'; ++i)
    {
        if(str[i] == '\n')  { y++; x = 0; }
        else if(str[i] == '\b') { if(x > 0) { x--; VideoMemory[80*y+x] = (VideoMemory[80*y+x] & 0xFF00) | ' '; } }
        else if(str[i] == '\t') { x = (x + 4) & ~3; }
        else {VideoMemory[80*y+x] = (VideoMemory[80*y+x] & 0xFF00) | str[i]; x++;}

        if(x >= 80) { y++; x = 0; }

        if(y >= 25)
        {
            for(y = 0; y < 25; y++)
                for (x = 0; x < 80; x++)
                VideoMemory[80*y+x] = (VideoMemory[80*y+x] & 0xFF00) | ' '; 
            x = 0; y = 0;                
        }
    }
}

void clearScreen()
{
    uint16_t* VideoMemory = (uint16_t*)0xb8000;
    for(int i = 0; i < 80*25; i++)
        VideoMemory[i] = 0x0720;
}

typedef void (*constructor)();
extern "C" constructor start_ctors;
extern "C" constructor end_ctors;
extern "C" void callConstructors() {
    for(constructor* i = &start_ctors; i != &end_ctors; i++)
    (*i)();
}


extern "C" void kernelMain(const void* multiboot_structure, uint32_t /*multiboot_magic*/)
{   
    clearScreen();
    
    printf("Hello World!\n");
    printf("Hello Leo");
    
    GlobalDescriptorTable gdt;
    InterruptManager interrupts(&gdt);

    // Create the drivers. Each one registers itself with the
    // InterruptManager for its own interrupt number.
    KeyboardDriver keyboard(&interrupts);
    MouseDriver mouse(&interrupts);

    mouse.Activate();          // switch the mouse on
    interrupts.Activate();     // now allow interrupts to happen

    while(1);
}
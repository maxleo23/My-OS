#include "mouse.h"

MouseDriver::MouseDriver(InterruptManager* manager)
: InterruptHandler(0x2C, manager),
dataport(0x60),
commandport(0x64)
{
    offset = 0;
    buttons = 0;

    uint16_t* VideoMemory = (uint16_t*)0xb8000;
    VideoMemory[80*12+40] = ((VideoMemory[80*12+40] & 0xF000) >> 4)
                            | ((VideoMemory[80*12+40] & 0x0F00) << 4)
                            | ((VideoMemory[80*12+40] & 0x00FF));
                            

    commandport.Write(0xA8); // activate the mouse interrupt
    commandport.Write(0x20); // tell the mouse we want to set the default settings
    uint8_t status = (dataport.Read() | 2) & ~0x20; 
    commandport.Write(0x60); // set sate
    dataport.Write(status); // write the status byte

    commandport.Write(0xD4); // tell the mouse we are sending a command
    dataport.Write(0xF4); // enable the mouse
    dataport.Read(); // empty the mouse's output buffer
}

MouseDriver::~MouseDriver()
{
}

void printf(char*);

uint32_t MouseDriver::HandleInterrupt(uint32_t esp)
{

    uint8_t status = commandport.Read();
    if(!(status & 0x20)) 
        return esp; // if the 5th bit is not set, then the mouse has not sent any data

        static int8_t x = 40, y = 12; // the mouse's position on the screen

    buffer[offset] = dataport.Read();
    if(offset == 0 && !(buffer[0] & 0x08))
        return esp;
    offset = (offset + 1) % 3;
    
    if(offset == 0)
    {
        static uint16_t* VideoMemory = (uint16_t*)0xb8000;

        VideoMemory[80*y+x] = ((VideoMemory[80*y+x] & 0xF000) >> 4)
                            | ((VideoMemory[80*y+x] & 0x0F00) << 4)
                            | ((VideoMemory[80*y+x] & 0x00FF));

        x += buffer[1];
        if(x < 0) x = 0;
        if(x >= 80) x = 79;

        y -= buffer[2];
        if(y < 0) y = 0;
        if(y >= 25) y = 24;


        VideoMemory[80*y+x] = ((VideoMemory[80*y+x] & 0xF000) >> 4)
                            | ((VideoMemory[80*y+x] & 0x0F00) << 4)
                            | ((VideoMemory[80*y+x] & 0x00FF));

        for(uint8_t i = 0; i < 3; i++)
        {
            if((buffer[0] & (0x01 << i)) != (buttons & (0x01 << i)))
            {
                VideoMemory[80*y+x] = ((VideoMemory[80*y+x] & 0xF000) >> 4)
                            | ((VideoMemory[80*y+x] & 0x0F00) << 4)
                            | ((VideoMemory[80*y+x] & 0x00FF));
            }
        }
        buttons = buffer[0];
    }

    return esp;
}

#include "mouse.h"

MouseEventHandler::MouseEventHandler()
{
}

void MouseEventHandler::OnActivate()
{
}

void MouseEventHandler::OnMouseDown(uint8_t)
{
}

void MouseEventHandler::OnMouseUp(uint8_t)
{
}

void MouseEventHandler::OnMouseMove(int, int)
{
}

MouseDriver::MouseDriver(InterruptManager* manager, MouseEventHandler* handler)
: InterruptHandler(0x2C, manager),
  dataport(0x60),
  commandport(0x64)
{
    this->handler = handler;
    offset = 0;
    buttons = 0;
}

MouseDriver::~MouseDriver()
{
}

void MouseDriver::Activate()
{
    offset = 0;
    buttons = 0;

    commandport.Write(0xA8); // activate the mouse interrupt
    commandport.Write(0x20); // read the controller command byte
    uint8_t status = (dataport.Read() | 2) & ~0x20; // enable mouse IRQ, enable mouse clock
    commandport.Write(0x60); // set the controller command byte
    dataport.Write(status);

    commandport.Write(0xD4); // next byte goes to the mouse
    dataport.Write(0xF4);    // enable the mouse
    dataport.Read();         // read the acknowledge

    if(handler != 0)
        handler->OnActivate();
}

uint32_t MouseDriver::HandleInterrupt(uint32_t esp)
{
    uint8_t status = commandport.Read();
    if(!(status & 0x20))
        return esp;     // bit 5 not set: this byte is not from the mouse

    buffer[offset] = dataport.Read();

    // The first byte of every packet always has bit 3 set. If not, we are
    // out of sync: drop the byte and wait for a valid first byte.
    if(offset == 0 && !(buffer[0] & 0x08))
        return esp;

    offset = (offset + 1) % 3;
    if(offset != 0)
        return esp;     // packet not complete yet

    // ---- packet complete: buffer[0] = flags, buffer[1] = X, buffer[2] = Y ----

    // X and Y are 9-bit signed numbers: the 8 data bits + a sign bit in buffer[0].
    int dx = buffer[1];
    int dy = buffer[2];
    if(buffer[0] & 0x10) dx -= 256;
    if(buffer[0] & 0x20) dy -= 256;
    if(buffer[0] & 0xC0) { dx = 0; dy = 0; }   // overflow bits: values are garbage

    if(handler != 0)
    {
        if(dx != 0 || dy != 0)
            handler->OnMouseMove(dx, -dy);     // PS/2 Y points up, screen Y points down

        for(uint8_t i = 0; i < 3; i++)
        {
            uint8_t bit = 0x01 << i;
            if((buffer[0] & bit) != (buttons & bit))
            {
                if(buffer[0] & bit) handler->OnMouseDown(i + 1);
                else                handler->OnMouseUp(i + 1);
            }
        }
    }
    buttons = buffer[0] & 0x07;

    return esp;
}
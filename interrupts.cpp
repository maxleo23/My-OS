#include "interrupts.h"

void printf(char* str);

InterruptManager::GateDescriptor InterruptManager::interruptsDescriptorTable[256];

InterruptManager* InterruptManager::ActiveInterruptManager = 0;

// ---- InterruptHandler: register / unregister a driver ---------------------

InterruptHandler::InterruptHandler(uint8_t interruptNumber, InterruptManager* interruptManager)
{
    this->interruptNumber = interruptNumber;
    this->interruptManager = interruptManager;
    interruptManager->handlers[interruptNumber] = this;   // "call me for this number"
}

InterruptHandler::~InterruptHandler()
{
    if(interruptManager->handlers[interruptNumber] == this)
        interruptManager->handlers[interruptNumber] = 0;
}

uint32_t InterruptHandler::HandleInterrupt(uint32_t esp)
{
    return esp;   // default: do nothing
}

void InterruptManager::SetInterruptDescriptorTableEntry(
    uint8_t interruptNumber,
    uint16_t codeSegmentSelectorOffset,
    void (*handler)(),
    uint8_t DescriptorPrivilegeLevel,
    uint8_t DescriptorType)
{

    const uint8_t IDT_DESC_PRESENT = 0x80;

    interruptsDescriptorTable[interruptNumber].handlerAddressLowBits = ((uint32_t)handler) & 0xFFFF;
    interruptsDescriptorTable[interruptNumber].handlerAddressHighBits = (((uint32_t)handler) >> 16) & 0xFFFF;
    interruptsDescriptorTable[interruptNumber].gdt_codeSegmentSelector = codeSegmentSelectorOffset;
    interruptsDescriptorTable[interruptNumber].access = IDT_DESC_PRESENT | DescriptorType | ((DescriptorPrivilegeLevel&3) << 5);
    interruptsDescriptorTable[interruptNumber].reserved = 0;
}

InterruptManager::InterruptManager(GlobalDescriptorTable* gdt)
: picMasterCommand(0x20),
  picMasterData(0x21),
  picSlaveCommand(0xA0),
  picSlaveData(0xA1)
{
    uint16_t CodeSegment = gdt->CodeSegmentSelector();
    const uint8_t IDT_INTERRUPT_GATE = 0xE;

    for(uint16_t i = 0; i < 256; i++)
    {
        handlers[i] = 0;
        SetInterruptDescriptorTableEntry(i, CodeSegment, &IgnoreInterruptRequest, 0, IDT_INTERRUPT_GATE);
    }

    SetInterruptDescriptorTableEntry(0x20, CodeSegment, &HandleInterruptRequest0x00, 0, IDT_INTERRUPT_GATE);
    SetInterruptDescriptorTableEntry(0x21, CodeSegment, &HandleInterruptRequest0x01, 0, IDT_INTERRUPT_GATE);
    SetInterruptDescriptorTableEntry(0x2C, CodeSegment, &HandleInterruptRequest0x0C, 0, IDT_INTERRUPT_GATE);

    picMasterCommand.Write(0x11);
    picSlaveCommand.Write(0x11);

    picMasterData.Write(0x20);
    picSlaveData.Write(0x28);

    picMasterData.Write(0x04);
    picSlaveData.Write(0x02);

    picMasterData.Write(0x01);
    picSlaveData.Write(0x01);

    picMasterData.Write(0xF8);   // was 0x00: allow IRQ0, IRQ1, IRQ2 (cascade)
    picSlaveData.Write(0xEF);    // was 0x00: allow IRQ12 only  

    InterruptsDescriptorTablePointer idt;
    idt.size = 256 * sizeof(GateDescriptor) -1; 
    idt.base = (uint32_t)interruptsDescriptorTable;
    asm volatile("lidt %0" : : "m" (idt));
}

InterruptManager::~InterruptManager()
{
}

void InterruptManager::Activate()
{
    if(ActiveInterruptManager != 0) 
        ActiveInterruptManager->Deactivate();
    ActiveInterruptManager = this;
    asm("sti");
}

void InterruptManager::Deactivate()
{
    if(ActiveInterruptManager == this) 
    {
        ActiveInterruptManager = 0;
        asm("cli");
    }
}

uint32_t InterruptManager::handleInterrupt(uint8_t interruptNumber, uint32_t esp)
{
    if(ActiveInterruptManager != 0)
        return ActiveInterruptManager->DoHandleInterrupt(interruptNumber, esp);
    return esp;
}

uint32_t InterruptManager::DoHandleInterrupt(uint8_t interruptNumber, uint32_t esp)
{
    if(handlers[interruptNumber] != 0)
    {
        // A driver registered for this interrupt: let it deal with it.
        esp = handlers[interruptNumber]->HandleInterrupt(esp);
    }
    else if(interruptNumber != 0x20)   // 0x20 is the timer, it fires all the time
    {
        const char* hex = "0123456789ABCDEF";
        char msg[] = "UNHANDLED INTERRUPT 0x00 ";
        msg[22] = hex[(interruptNumber >> 4) & 0xF];
        msg[23] = hex[interruptNumber & 0xF];
        printf(msg);
    }

    // Hardware IRQs (0x20-0x2F) must be acknowledged with an EOI
    if(0x20 <= interruptNumber && interruptNumber < 0x30)
    {
        picMasterCommand.Write(0x20);
        if(interruptNumber >= 0x28)
            picSlaveCommand.Write(0x20);
    }

    return esp;
}
# My_Os

A hobby 32-bit x86 operating system kernel written in C++ and assembly. It boots through GRUB (Multiboot), prints text to the screen, sets up its own GDT and interrupt table, and reacts to keyboard interrupts.

The long-term goal is a simple command-line OS with a couple of small apps.

## Current features

- Multiboot-compliant loader (`loader.s`) that sets up a 2 MB kernel stack and runs global constructors before `kernelMain`
- VGA text-mode output (`printf`, `clearScreen`) written straight to `0xB8000`
- Global Descriptor Table (GDT) with null, unused, code and data segments, and segment registers reloaded after `lgdt`
- Port I/O classes (`Port8Bit`, `Port8BitSlow`, `Port16Bit`, `Port32Bit`)
- Interrupt Descriptor Table (IDT) with a default "ignore" handler for all 256 vectors
- PIC (8259) remapping, so IRQs 0-15 land on vectors `0x20`-`0x2F`
- Timer (IRQ0) and keyboard (IRQ1) handlers, with EOI sent to the PIC
- Keyboard handler reads port `0x60` and prints on key press (scancodes below `0x80`)

## Project layout

| File | Purpose |
|------|---------|
| `loader.s` | Multiboot header, stack setup, entry point (`loader`) |
| `kernel.cpp` | `kernelMain`, `printf`, `clearScreen`, global constructor caller |
| `gdt.h` / `gdt.cpp` | `GlobalDescriptorTable` and `SegmentDescriptor` |
| `interrupts.h` / `interrupts.cpp` | `InterruptManager`: IDT setup, PIC remap, interrupt handler |
| `interruptstubs.s` | Assembly entry stubs for each interrupt, calls `handleInterrupt` |
| `port.h` / `port.cpp` | Wrappers around the x86 `in` / `out` instructions |
| `types.h` | Fixed-size integer typedefs (no standard library available) |
| `linker.ld` | Linker script, kernel loads at `0x100000` |
| `makefile` | Builds the kernel and ISO |

## How it boots

1. GRUB loads `mykernel.bin` and jumps to `loader`.
2. `loader` sets the stack, calls `callConstructors`, then `kernelMain`.
3. `kernelMain` clears the screen, prints a greeting, creates the `GlobalDescriptorTable`, then the `InterruptManager`.
4. `InterruptManager` builds the IDT, remaps the PIC and loads the table with `lidt`.
5. `Activate()` runs `sti` and the kernel loops, handling interrupts as they arrive.

## Requirements

- Linux (or WSL) with `gcc` able to build 32-bit code (`gcc-multilib`)
- `binutils` (`as`, `ld`)
- `grub-mkrescue`, `xorriso` and `mtools` (for building the ISO)
- A VM such as VirtualBox or QEMU to run the result

## Build and run

```sh
make mykernel.bin      # build the kernel only
make mykernel.iso      # build a bootable ISO
make run               # build the ISO and copy it to the shared folder
make clean             # remove build output
```

The `run` target copies `mykernel.iso` to the `DEST` folder set at the top of the makefile and then removes the local copy.

The makefile does not track header dependencies. After editing a `.h` file, run `make clean` first.

To test in QEMU instead:

```sh
qemu-system-i386 -cdrom mykernel.iso
```

In VirtualBox, create a new VM (Type: Other, Version: Other/Unknown 32-bit), attach `mykernel.iso` as the optical disk and start it.

## Compiler flags

The kernel is built freestanding: `-m32 -ffreestanding -nostdlib -fno-pic -fno-pie -fno-rtti -fno-exceptions -fno-stack-protector -fno-use-cxa-atexit -fno-leading-underscore`. `-fno-leading-underscore` matters because `interruptstubs.s` refers to C++ mangled names such as `_ZN16InterruptManager15handleInterruptEhj`. If you rename a function or change its parameters, update the mangled name in the stub file.

## Known limitations

- Keyboard output is raw: it prints on key press but does not translate scancodes into characters yet
- `printf` handles plain strings only (no formatting) and clears the screen instead of scrolling
- No CPU exception handlers; all unhandled vectors just `iret`
- No memory management, multitasking or drivers beyond the keyboard

## Roadmap

- Scancode-to-character table for the keyboard
- Hex/number printing helper for debugging
- Proper screen scrolling
- CPU exception handlers (page fault, general protection fault)
- Simple command-line shell
- A couple of small apps

## Notes

Inspired by the "Write your own operating system" tutorial series. Written for learning.

GPPPARAMS = -m32 -fno-pic -ffreestanding -Wall -Wextra -fno-pie -fno-stack-protector -fno-use-cxa-atexit -nostdlib -fno-builtin -fno-rtti -fno-exceptions -fno-leading-underscore -Wno-write-strings
ASPARAMS = --32
LDPARAMS = -melf_i386

objects = loader.o gdt.o port.o interruptstubs.o interrupts.o keyboard.o kernel.o

%.o: %.cpp
	gcc $(GPPPARAMS) -c -o $@ $<

%.o: %.s
	as $(ASPARAMS) -o $@ $<

mykernel.bin: linker.ld $(objects)
	ld $(LDPARAMS) -T $< -o $@ $(objects)

mykernel.iso: mykernel.bin
	mkdir -p iso
	mkdir -p iso/boot
	mkdir -p iso/boot/grub
	cp $< iso/boot/
	echo 'set timeout=0' 					>> iso/boot/grub/grub.cfg
	echo 'set default=0' 					>> iso/boot/grub/grub.cfg
	echo '' 								>> iso/boot/grub/grub.cfg
	echo 'menuentry "My OS" {' 				>> iso/boot/grub/grub.cfg
	echo ' multiboot /boot/mykernel.bin' 	>> iso/boot/grub/grub.cfg
	echo ' boot' 							>> iso/boot/grub/grub.cfg
	echo '}' 								>> iso/boot/grub/grub.cfg
	grub2-mkrescue --output=$@ iso
	rm -rf iso
	
run: mykernel.iso
	(VBoxManage controlvm "My Os" poweroff && sleep 1) || true
	VBoxManage startvm "My Os"

.PHONY: clean run
clean:
	rm -f $(objects) mykernel.bin mykernel.iso
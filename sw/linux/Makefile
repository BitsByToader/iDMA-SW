# Makefile for On-Target Compilation

# Kernel module object file(s)
# List both the provider and client here
obj-m += idma_provider.o
obj-m += idma_client.o

# Path to the kernel source/headers for the currently running kernel
CC := gcc
#KERNEL_SRC := /lib/modules/$(shell uname -r)/build
KERNEL_SRC := /lib/modules/6.6.40-xilinx-g68aa9b010ac4/build

# Phony targets prevent conflicts with files of the same name
.PHONY: all kernel_module user_app clean load unload run

# Default target: build both the kernel module and the user application
all: overlay kernel_module user_app

# Rule to build the kernel module(s)
kernel_module:
	$(MAKE) -C $(KERNEL_SRC) M=$(PWD) modules

# Rule to build the user-space application
user_app: user_app.c idma_common.h
	$(CC) user_app.c -o user_app -Wall -Wextra

# Rule to build device tree overlay for hardware peripheral.
overlay:
	dtc -O dtb -o idma-overlay.dtbo -b 0 -@ idma-overlay.dts

# Rule to clean up build artifacts
clean:
	$(MAKE) -C $(KERNEL_SRC) M=$(PWD) clean
	rm -f idma-overlay.dtbo
	rm -f user_app

# --- Convenience Targets (for use on the target) ---

# Target for loading the bitstream onto the FPGA
# TEMP FIX: After custom kernel, also write to AFIFM registers in order to set AXI ports to 64bit.
program:
	sudo fpgautil -R -n full
	sudo fpgautil -b /lib/firmware/idma/idma_bitstream.bit.bin -o idma-overlay.dtbo -f Full -n full
	sudo fpgautil -b /lib/firmware/idma/idma_bitstream.bit.bin -f Full # FIXME: for some reason, above command sets overlay correctly, but doesn't program FPGA correctly. this one does.
	sudo devmem 0x00FD360000 32 0x000003B1
	sudo devmem 0x00FD360014 32 0x000003B1
	sudo devmem 0x00FD370000 32 0x000003B1
	sudo devmem 0x00FD370014 32 0x000003B1
	sudo devmem 0x00FD380000 32 0x000003B1
	sudo devmem 0x00FD380014 32 0x000003B1


# Target for loading the modules (Order matters: Provider first, then Client)
load: kernel_module
	sudo insmod idma_provider.ko
	sudo insmod idma_client.ko
	# Check dmesg to confirm they loaded
	dmesg | tail -n 5

# Target for unloading the modules (Order matters: Client first, then Provider)
unload:
	-sudo rmmod idma_client
	-sudo rmmod idma_provider

# Target to run user app
# Note: Ensure /dev/my_dma_interface exists or permissions are set
run: user_app
	sudo ./user_app

/* Copyright 2016 Pedro Falcato

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

http ://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/
/**************************************************************************
 * 
 * 
 * File: kernel.c
 * 
 * Description: Main kernel file, contains the entry point and initialization
 * 
 * Date: 30/1/2016
 * 
 * 
 **************************************************************************/

#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#ifdef AMD64
#include <multiboot2.h>
typedef multiboot_info_t multiboot_tag_structure
#else
#include <multiboot.h>
#endif
#include <kernel/tty.h>
#include <kernel/compiler.h>
#include <stdlib.h>
#include <kernel/timer.h>
#include <kernel/pmm.h>
#include <kernel/sleep.h>
#include <kernel/initrd.h>
#include <kernel/fs.h>
#include <drivers/ps2.h>
/* Function: init_arch()
 * Purpose: Initialize architecture specific features, should be hooked by the architecture the kernel will run on
 */
ARCH_SPECIFIC void init_arch();
ARCH_SPECIFIC void init_vmm();
ARCH_SPECIFIC void jump_userspace();
static multiboot_info_t* mbt;
static uint32_t initrd_addr;
extern uint32_t end;
void kernel_early(multiboot_info_t* info, size_t magic)
{
	if(info == NULL)
		panic("Invalid multiboot_info_t*.The bootloader currently being used is broken");
	mbt = info;
	terminal_initialize();
	puts("Booting ...");
	// Check if the magic number is the same as the multiboot 1 spec
	if(magic == 0x2BADB002)
		puts("Spartix kernel booted by a Multiboot 1 compliant bootloader");
	else
		panic("Bootloader not Multiboot 1 compliant"); // If not, panic, because our kernel relies on it 
	init_arch();
	
	mbt->mmap_addr+=0xC0000000;
	multiboot_memory_map_t* mmap = (multiboot_memory_map_t*) mbt->mmap_addr;
	memset(0xC0200000,0,4096);
	pmm_init(mbt->mem_lower + mbt->mem_upper,(uintptr_t) 0xC0200000);
	multiboot_memory_map_t*  mmap_arr[10];
	while((unsigned int)mmap < mbt->mmap_addr + mbt->mmap_length) {
		static int i = 0;

		mmap_arr [i] = mmap;

		if(mmap->type==MULTIBOOT_MEMORY_AVAILABLE)
		    pmm_push(mmap->addr,mmap->len);

		printf("Size:0x%X ",mmap->len);

		printf("Base Address:0x%X ",mmap->addr);

		printf("Limit:0x%X ",mmap->addr + mmap->len);

		switch(mmap->type){
		    case MULTIBOOT_MEMORY_AVAILABLE:
			printf("Type: Available\n");
			break;
		    case MULTIBOOT_MEMORY_RESERVED:
			printf("Type: Reserved\n");
			break;
		    case MULTIBOOT_MEMORY_ACPI_RECLAIMABLE:
			printf("Type: ACPI\n");
			break;
		    case MULTIBOOT_MEMORY_NVS:
			printf("Type: ACPI NVS\n");
			break;
		    case MULTIBOOT_MEMORY_BADRAM:
			printf("Type: Bad ram\n");
			break;
		}
		mmap = (multiboot_memory_map_t*) ( (unsigned int)mmap + mmap->size + sizeof(unsigned int) );
	}
	printf("Memory in KiB:%i\n",mbt->mem_lower+mbt->mem_upper);
	multiboot_module_t* mod_start_ptr = (multiboot_module_t*)mbt->mods_addr;
	initrd_addr = mod_start_ptr->mod_start;
// 	pmmngr_deinit_region((physical_addr)mod_start_ptr,sizeof(multiboot_module_t));
// 	pmmngr_deinit_region((physical_addr)mod_start_ptr->mod_start,mod_start_ptr->mod_end - mod_start_ptr->mod_start);
}
void kernel_main()
{
	puts("Spartix kernel 0.1");
	// Enable interrupts
	asm volatile("sti");
	// Initialize the timer
	timer_init(1000);
	//Initialize the VMM
	init_vmm();
	// Initialize the kernel heap
	init_heap();
	fs_node_t* initrd_root = init_initrd(initrd_addr);
	if(!initrd_root)
		abort();
	fs_node_t* node = finddir_fs(initrd_root,"boot/Kernel.map");
	if(!node)
		abort();
	init_keyboard();
	init_scheduler();
	jump_userspace();
	
	for(;;)
	{
		asm volatile("hlt");
	}
}

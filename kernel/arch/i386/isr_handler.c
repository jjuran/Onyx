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
#include <stdlib.h>
#include <stdio.h>
#include <kernel/isr.h>
#include <kernel/panic.h>
#include <kernel/sbrk.h>
static uint32_t faulting_address;
char *exception_msg[] = {
    "Division by zero exception",
    "Debug Trap",
    "Non-maskable interrupt",
    "Hit a breakpoint",
    "Overflow trap",
    "Overflow trap",
    "Fault: Bound range exceeded",
    "Invalid Instruction",
    "FPU not detected",
    "Critical error: DOUBLE FAULT",
    "Invalid TSS",
    "Segment not present",
    "Stack segment fault",
    "General Protection Fault",
    "Page fault at ",
    "",
    "x87 floating point exception",
    "Alignment check exception",
    "Machine check exception",
    "SIMD floating point exception",
    "Virtualization exception",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "Security exception"
};
_Bool faulting = false;
inline void exit_isr_handler()
{
	faulting = false;
}
inline void enter_isr_handler()
{
	faulting = true;
}
inline _Bool is_recursive_fault()
{
	return faulting;
}
void isr_handler(uint32_t ds, uint32_t int_no, uint32_t err_code)
{
	(void)ds;
	if(is_recursive_fault()) { /* If this fault happened inside another fault, */
		for(;;);           /* Something is wrong with the environment, so just for(;;); */
	}
	enter_isr_handler();
	switch (int_no) {
	case 0:{
			panic(exception_msg[int_no]);
			break;
		}
	case 1:{
			panic(exception_msg[int_no]);
			break;
		}
	case 2:{
			break;
		}
	case 3:{
			printf(exception_msg[int_no]);
			break;
		}
	case 4:{
			printf(exception_msg[int_no]);
			break;
		}
	case 5:{
			printf(exception_msg[int_no]);
			break;
		}
	case 6:{
			panic(exception_msg[int_no]);
			break;
		}
	case 7:{
			printf(exception_msg[int_no]);
			break;
		}
	case 8:{
			panic(exception_msg[int_no]);
			break;
		}
	case 9:{
			panic("i386 processors not supported by Spartix");
			break;
		}
	case 10:{
			panic(exception_msg[int_no]);
			break;
		}
	case 11:{
			panic(exception_msg[int_no]);
			break;
		}
	case 12:{
			panic(exception_msg[int_no]);
			break;
		}
	case 13:{
			printf(exception_msg[int_no]);
			if (err_code != 0)
				printf("\nSegment 0x%X\n", err_code);
			panic("");
			break;
		}
	case 14:{
			/* A page fault has occurred. */
			/* The faulting address is stored in the CR2 register. */
			__asm__ __volatile__ ("mov %%cr2, %0":"=r"
				      (faulting_address));
			if (err_code & 0x2) {
				printf(" Trying to use COW\n");
				if (vmm_alloc_cow(faulting_address & 0xFFFFF000) == 1) {
					printf("Copy-on-Write failed\n");
					exit_isr_handler();
					return;
				}
				else {
					exit_isr_handler();
					return;
				}
			}
			printf("%s0x%X\n",exception_msg[int_no],faulting_address);
			if(err_code & 0x2)
				printf(" caused by a write\n");
			if(err_code & 0x4)
			{
				printf("user-mode\n");
			}
                        while(1);
		}
	case 15:{
			break;	/*Reserved exception */
		}
	case 16:{
			printf(exception_msg[int_no]);
			break;
		}
	case 17:{
			break;
		}
	case 18:{
			break;
		}
	case 19:{
			printf(exception_msg[int_no]);
			break;
		}
	case 20:{
			printf(exception_msg[int_no]);
			break;
		}
	case 21:		/*Handle the intel reserved exceptions to do nothing */
	case 22:
	case 23:
	case 24:
	case 25:
	case 26:
	case 27:
	case 28:
	case 29:
	case 30:
	case 31:
		break;
	}
	exit_isr_handler();
}

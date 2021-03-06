/*
* Copyright (c) 2016, 2017 Pedro Falcato
* This file is part of Onyx, and is released under the terms of the MIT License
* check LICENSE at the root directory for more information
*/
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>

#include <kernel/vmm.h>
#include <kernel/ethernet.h>
#include <kernel/pic.h>
#include <kernel/irq.h>
#include <kernel/log.h>

#include <drivers/mmio.h>
#include <drivers/e1000.h>
#include <drivers/pci.h>

static struct pci_device *nicdev = NULL;
static struct e1000_rx_desc *rx_descs[E1000_NUM_RX_DESC];
static struct e1000_tx_desc *tx_descs[E1000_NUM_TX_DESC];
static int rx_cur = 0, tx_cur = 0;
_Bool eeprom_exists = false;
_Bool got_packet = false;
static char *mem_space = NULL;
static uint16_t io_space = 0;
void e1000_write_command(uint16_t addr, uint32_t val);
uint32_t e1000_read_command(uint16_t p_address);

static void initialize_e1000_busmastering()
{
	pci_enable_busmastering(nicdev);
}
void e1000_handle_recieve()
{
	uint16_t old_cur = 0;
	while((rx_descs[rx_cur]->status & 0x1))
	{
		got_packet = true;
		uint8_t *buf = (uint8_t *)rx_descs[rx_cur]->addr;
		uint16_t len = rx_descs[rx_cur]->length;
		ethernet_handle_packet(buf, len);
		eth_set_packet_buf(buf + PHYS_BASE);
		eth_set_packet_len(len);
		rx_descs[rx_cur]->status = 0;
		old_cur = rx_cur;
		rx_cur = (rx_cur + 1) % E1000_NUM_RX_DESC;
		e1000_write_command(REG_RXDESCTAIL, old_cur);
}
}
static uintptr_t e1000_irq(registers_t *regs)
{
	volatile uint32_t status = e1000_read_command(0xc0);
	if(status & 0x80)
	{
		e1000_handle_recieve();
	}
	return 0;
}
void e1000_write_command(uint16_t addr, uint32_t val)
{
	mmio_writel((uintptr_t) (mem_space + addr), val);
}
uint32_t e1000_read_command(uint16_t addr)
{
	return mmio_readl((uintptr_t) (mem_space + addr));
}
void e1000_detect_eeprom()
{
	e1000_write_command(REG_EEPROM, 0x1);
	for(int i = 0; i < 1000000; i++)
	{
		uint32_t test = e1000_read_command(REG_EEPROM);
		if(test & 0x10)
		{
			INFO("e1000", "confirmed eeprom exists at spin %d\n", i);
			eeprom_exists = true;
			break;
		}
	}
}
uint32_t e1000_eeprom_read(uint8_t addr)
{
	uint16_t data = 0;
	uint32_t tmp = 0;
        if (eeprom_exists)
        {
            	e1000_write_command(REG_EEPROM, (1) | ((uint32_t)(addr) << 8));
        	while(!((tmp = e1000_read_command(REG_EEPROM)) & (1 << 4)));
        }
        else
        {
            e1000_write_command(REG_EEPROM, (1) | ((uint32_t)(addr) << 2));
            while(!((tmp = e1000_read_command(REG_EEPROM)) & (1 << 1)));
        }
	data = (uint16_t)((tmp >> 16) & 0xFFFF);
	return data;
}
unsigned char mac_address[6];
int e1000_read_mac_address()
{
	if(eeprom_exists)
	{
		uint32_t temp;
		temp = e1000_eeprom_read(0);
		mac_address[0] = temp &0xff;
		mac_address[1] = temp >> 8;
		temp = e1000_eeprom_read(1);
		mac_address[2] = temp &0xff;
		mac_address[3] = temp >> 8;
		temp = e1000_eeprom_read(2);
		mac_address[4] = temp &0xff;
		mac_address[5] = temp >> 8;
		return 0;
	}
	else
	{
		uint8_t *mem_base_mac_8 = (uint8_t *) (mem_space+0x5400);
		uint32_t *mem_base_mac_32 = (uint32_t *) (mem_space+0x5400);
		if ( mem_base_mac_32[0] != 0 )
		{
			for(int i = 0; i < 6; i++)
			{
				mac_address[i] = mem_base_mac_8[i];
			}
			return 0;
		}
   	}
	return 1;
}
int e1000_init_descs()
{
	uint8_t *ptr = NULL;
	struct e1000_rx_desc *rxdescs = NULL;
	size_t needed_pages = (sizeof(struct e1000_rx_desc) * E1000_NUM_RX_DESC + 16) / 4096;
	if((sizeof(struct e1000_rx_desc) * E1000_NUM_RX_DESC + 16) % 4096)
		needed_pages++;
	ptr = vmm_allocate_virt_address(VM_KERNEL, needed_pages, VM_TYPE_HW, VM_WRITE | VM_GLOBAL | VM_NOEXEC, 0);
	if(!ptr)
		return 1;
	vmm_map_range(ptr, needed_pages, VMM_WRITE | VMM_GLOBAL | VMM_NOEXEC);
	rxdescs = (struct e1000_rx_desc *) ptr;
	for(int i = 0; i < E1000_NUM_RX_DESC; i++)
	{
		rx_descs[i] = (struct e1000_rx_desc *)((uint8_t *)rxdescs + i*16);
		rx_descs[i]->addr = (uint64_t) malloc(MAX_MTU);
		if(!rx_descs[i]->addr)
		{
			/* Free the past entries */
			for(int j = 0; j < i; j++)
			{
				free(rx_descs[j]);
			}
			vmm_unmap_range(ptr, needed_pages);
			return 1;
		}
		rx_descs[i]->addr = (uint64_t) virtual2phys((void*) rx_descs[i]->addr);
		rx_descs[i]->status = 0;
	}
	ptr = virtual2phys(ptr);
	e1000_write_command(REG_RXDESCLO, (uint32_t)((uint64_t)ptr & 0xFFFFFFFF));
	e1000_write_command(REG_RXDESCHI, (uint32_t)((uint64_t)ptr >> 32));

	e1000_write_command(REG_RXDESCLEN, E1000_NUM_RX_DESC * 16);

	e1000_write_command(REG_RXDESCHEAD, 0);
	e1000_write_command(REG_RXDESCTAIL, E1000_NUM_RX_DESC-1);
	rx_cur = 0;
	e1000_write_command(REG_RCTRL, RCTL_EN| RCTL_SBP| RCTL_UPE | RCTL_MPE | RCTL_LBM_NONE | RTCL_RDMTS_HALF | RCTL_BAM | RCTL_SECRC | RCTL_BSIZE_2048);
	
	struct e1000_tx_desc *txdescs = NULL;
	
	needed_pages = (sizeof(struct e1000_tx_desc) * E1000_NUM_TX_DESC + 16) / 4096;
	if((sizeof(struct e1000_tx_desc) * E1000_NUM_TX_DESC + 16) % 4096)
		needed_pages++;
	ptr = vmm_allocate_virt_address(VM_KERNEL, needed_pages, VMM_TYPE_HW, VMM_WRITE | VMM_GLOBAL | VMM_NOEXEC, 0);
	if(!ptr)
		return 1;
	vmm_map_range(ptr, needed_pages, VMM_WRITE | VMM_GLOBAL | VMM_NOEXEC);
	txdescs = (struct e1000_tx_desc *) ptr;

	for(int i = 0; i < E1000_NUM_TX_DESC; i++)
	{
		tx_descs[i] = (struct e1000_tx_desc *)((uint8_t *)txdescs + i*16);
		tx_descs[i]->addr = 0;
		tx_descs[i]->cmd = 0;
		tx_descs[i]->status = TSTA_DD;
	}
	ptr = virtual2phys(ptr);
	e1000_write_command(REG_TXDESCLO, (uint32_t)((uint64_t)ptr & 0xFFFFFFFF));
	e1000_write_command(REG_TXDESCHI, (uint32_t)((uint64_t)ptr >> 32));

	e1000_write_command(REG_TXDESCLEN, E1000_NUM_TX_DESC * 16);

	e1000_write_command( REG_TXDESCHEAD, 0);
	e1000_write_command( REG_TXDESCTAIL, 0);
	tx_cur = 0;
	/*e1000_write_command(REG_TCTRL,  TCTL_EN
        | TCTL_PSP
        | (15 << TCTL_CT_SHIFT)
        | (64 << TCTL_COLD_SHIFT)
        | TCTL_RTLC); */
 
	e1000_write_command(REG_TCTRL,  0b0110000000000111111000011111010);
	e1000_write_command(REG_TIPG,  0x0060200A);

	return 0;
}
void e1000_enable_interrupts()
{
	uint16_t int_no = pci_get_intn(nicdev->bus, nicdev->device, nicdev->function);
	
	// Get the IRQ number and install its handler
	INFO("e1000", "using IRQ number %u\n", int_no);

	irq_install_handler(int_no, e1000_irq);
	
	e1000_write_command(REG_IMASK, 0x1F6DC);
	e1000_write_command(REG_IMASK ,0xff & ~4);
	e1000_read_command(0xC0);
}
int e1000_send_packet(const void *data, uint16_t len)
{
	tx_descs[tx_cur]->addr = (uint64_t)virtual2phys((void*) data);
	tx_descs[tx_cur]->length = len;
	tx_descs[tx_cur]->cmd = CMD_EOP | CMD_IFCS | CMD_RS | CMD_RPS | CMD_IC;
	tx_descs[tx_cur]->status = 0;
	uint8_t old_cur = tx_cur;
	tx_cur = (tx_cur + 1) % E1000_NUM_TX_DESC;
	e1000_write_command(REG_TXDESCTAIL, tx_cur);
	while(!(tx_descs[old_cur]->status & 0xff));
	return 0;
}
void e1000_init(struct pci_device *dev)
{
	nicdev = dev;
	pcibar_t *bar = pci_get_bar(nicdev, 0);
	char *phys_mem_space = NULL;
	if(bar->isIO)
		io_space = (uint16_t)bar->address;
	else
		phys_mem_space = (char *)(uint64_t)bar->address;
	if(phys_mem_space)
		INFO("e1000", "mmio mode\n");
	else
	{
		free(dev);
		free(bar);
		ERROR("e1000", "Sorry! This driver only supports e1000 register access through MMIO, and sadly your card needs the legacy I/O port method of accessing registers\n");
		return;
	}
	INFO("e1000", "physical mem %p\n", phys_mem_space);
	size_t needed_pages = bar->size / PAGE_SIZE;
	if(bar->size % PAGE_SIZE)
		needed_pages++;
	mem_space = vmm_allocate_virt_address(VM_KERNEL, needed_pages, VMM_TYPE_HW, VMM_WRITE | VMM_GLOBAL | VMM_NOEXEC, 0);
	if(!mem_space)
		return;
	uintptr_t virt = (uintptr_t) mem_space;
	for(size_t i = 0; i < needed_pages; i++)
	{
		paging_map_phys_to_virt(virt, (uintptr_t) phys_mem_space, VMM_GLOBAL | VMM_WRITE | VMM_NOEXEC);
		phys_mem_space += 0x1000;
		virt += 0x1000;
	}
	// Initialize PCI Busmastering (needed for DMA)
	initialize_e1000_busmastering();
	e1000_detect_eeprom();
	if(e1000_read_mac_address())
		return;
	//INFO("eth0", "MAC address: %x:%x:%x:%x:%x:%x\n", mac_address[0], mac_address[1], mac_address[2], mac_address[3], mac_address[4], mac_address[5]);
	if(e1000_init_descs())
		ERROR("e1000", "failed to initialize!\n");
	e1000_enable_interrupts();
	//eth_set_dev_send_packet(e1000_send_packet);
	free(bar); // Don't forget to free bar, as we don't want a memory leak
}

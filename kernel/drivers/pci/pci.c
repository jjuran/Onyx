/*
* Copyright (c) 2016, 2017 Pedro Falcato
* This file is part of Onyx, and is released under the terms of the MIT License
* check LICENSE at the root directory for more information
*/
#include <stdio.h>
#include <string.h>
#include <assert.h>

#include <kernel/compiler.h>
#include <kernel/log.h>
#include <kernel/acpi.h>
#include <kernel/panic.h>
#include <kernel/dev.h>

#include <drivers/pci.h>

int pci_shutdown(struct device *__dev);
static struct bus pci_bus = 
{
	.name = "pci",
	.shutdown = pci_shutdown
};
const uint16_t CONFIG_ADDRESS = 0xCF8;
const uint16_t CONFIG_DATA = 0xCFC;
static spinlock_t pci_lock;
uint16_t __pci_config_read_word (uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset)
{
        union { uint8_t bytes[4]; uint32_t val;} data;
	data.val = __pci_config_read_dword(bus, slot, func, offset);
	return data.bytes[(offset & 0x3)] | (data.bytes[(offset & 3) + 1] << 8);
}
uint32_t __pci_config_read_dword(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset)
{
	uint32_t address;
	uint32_t lbus  = (uint32_t)bus;
	uint32_t lslot = (uint32_t)slot;
	uint32_t lfunc = (uint32_t)func;
	uint32_t tmp = 0;

	address = (uint32_t)((lbus << 16) | (lslot << 11) |
                     (lfunc << 8) | (offset & 0xfc) | ((uint32_t)0x80000000));
	acquire_spinlock(&pci_lock);
	/* write out the address */
	outl(CONFIG_ADDRESS, address);
	/* read in the data */
	tmp = inl(CONFIG_DATA);
	release_spinlock(&pci_lock);
	return tmp;
}
void __pci_write_dword(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset, uint32_t data)
{
	uint32_t address;
	uint32_t lbus  = (uint32_t)bus;
	uint32_t lslot = (uint32_t)slot;
	uint32_t lfunc = (uint32_t)func;

	/* create configuration address as per Figure 1 */
	address = (uint32_t)((lbus << 16) | (lslot << 11) |
		  (lfunc << 8) | (offset & 0xfc) | ((uint32_t)0x80000000));
	
	acquire_spinlock(&pci_lock);
	/* write out the address */
	outl(CONFIG_ADDRESS, address);
	/* read in the data */
	outl(CONFIG_DATA, data);
	release_spinlock(&pci_lock);
}
void __pci_write_word(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset, uint16_t data)
{
	uint32_t address;
	uint32_t lbus  = (uint32_t)bus;
	uint32_t lslot = (uint32_t)slot;
	uint32_t lfunc = (uint32_t)func;

	/* create configuration address as per Figure 1 */
	address = (uint32_t)((lbus << 16) | (lslot << 11) |
		  (lfunc << 8) | (offset & 0xfc) | ((uint32_t)0x80000000));
	
	acquire_spinlock(&pci_lock);
	/* write out the address */
	outl (CONFIG_ADDRESS, address);
	/* read in the data */
	outw(CONFIG_DATA, data);
	release_spinlock(&pci_lock);
}
int pci_set_power_state(struct pci_device *dev, int power_state)
{
	struct pci_device *element = NULL;
	void *saveptr = NULL; /* Used by list_get_element in a strtok_r kind of way */
	/* If we can't perform power management on this device, just return 
	 * success(it wasn't really an error was it?) 
	*/
	if(dev->has_power_management == false)
		return -1;
	/* I guess we're already there, so just return */
	if(dev->current_power_state == power_state)
		return 0;
	/* TODO: It's unsafe to cut power to the PCI bridge just like that, so we ignore setting it */
	if(dev->type == PCI_TYPE_BRIDGE)
		return 0;
	/* Check if the desired power state is supported */
	if(dev->supported_power_states & power_state)
		return -1;	/* If not, just return */
	/* Set its children's power state as well */
	while((element = list_get_element(&dev->dev.children, &saveptr)))
	{
		pci_set_power_state(element, power_state);
	}
	/* Ok, if we can perform power management, get the PMCSR offset */
	uint16_t pmcsr_off = dev->pm_cap_off + 4;

	uint16_t pmcsr = pci_read(dev, pmcsr_off, sizeof(uint16_t));

	/* Translate the argument into the actual bits */
	int p;
	switch(power_state)
	{
		case PCI_POWER_STATE_D0:
			p = 0;
			break;
		case PCI_POWER_STATE_D1:
			p = 1;
			break;
		case PCI_POWER_STATE_D2:
			p = 2;
			break;
		case PCI_POWER_STATE_D3:
			p = 3;
			break;
		default:
			panic("pci: Invalid target power state\n");
	}
	/* And set them in PMCSR, writing them back */
	pmcsr |= p;
	pci_write(dev, p, pmcsr_off, sizeof(uint16_t));
	return 0;
}
int pci_shutdown(struct device *__dev)
{
	/* Okay, we're shutting down and our purpose here is to cut power to the device.
	 * Hopefully the device driver has already been notified that we're shutting down, so
	 * we're safe to cut power to the device by setting the power state to D3
	*/
	assert(__dev);
	return pci_set_power_state((struct pci_device*) __dev, PCI_POWER_STATE_D3);
}
void pci_find_supported_capabilities(struct pci_device *dev)
{
	off_t pm_off = pci_find_capability(dev, PCI_CAP_ID_POWER_MANAGEMENT_INTERFACE);
	if(pm_off != -1)
	{
		/* We found the PM Register block! Great, now we'll cache the offset and the
		 * fact that the capability exists
		*/
		dev->has_power_management = true;
		dev->pm_cap_off = (uint8_t) pm_off;
		/* Now, grab the PMC and cache the available power states 
		 * The PMC is at pm_off + 2, and is 16 bits in size
		*/
		uint16_t pmc = pci_read(dev, pm_off + 2, sizeof(uint16_t));
		/* D0 and D3 are always supported */
		dev->supported_power_states = PCI_POWER_STATE_D0 | PCI_POWER_STATE_D3;
		if(pmc & PCI_PMC_D1_SUPPORT)
			dev->supported_power_states |= PCI_POWER_STATE_D1;
		if(pmc & PCI_PMC_D2_SUPPORT)
			dev->supported_power_states |= PCI_POWER_STATE_D2;
	}
}
struct pci_device *linked_list = NULL;
struct pci_device* last = NULL;
void pci_enumerate_device(uint16_t bus, uint8_t device, uint8_t function, struct pci_device *parent)
{
	// Get vendor
	uint16_t vendor = (uint16_t)(__pci_config_read_dword(bus, device, function ,0) & 0x0000ffff);

	if(vendor == 0xFFFF) /* Invalid, just skip this device */
		return;

	uint16_t header = (uint16_t)(__pci_config_read_word(bus, device, function, 0xE));

	uint8_t pciClass = (uint8_t)(__pci_config_read_word(bus, device, function, 0xA)>>8);
	uint8_t subClass = (uint8_t)__pci_config_read_word(bus, device, function, 0xB);
	uint8_t progIF = (uint8_t)(__pci_config_read_word(bus, device, function, 0xC)>>8);

	// Set up some meta-data
	struct pci_device* dev = malloc(sizeof(struct pci_device));
	if(!dev)
		panic("pci: early unrecoverable oom\n");
	memset(dev, 0 , sizeof(struct pci_device));

	dev->bus = bus;
	dev->function = function;
	dev->device = device;
	dev->vendorID = vendor;
	dev->deviceID = (__pci_config_read_dword(bus, device, function, 0) >> 16);
	dev->pciClass = pciClass;
	dev->subClass = subClass;
	dev->progIF = progIF;
	dev->current_power_state = PCI_POWER_STATE_D0;
	dev->type = header & PCI_TYPE_MASK;
	/* Find supported caps and add them to dev */
	pci_find_supported_capabilities(dev);
	/* Set up the pci device's name */
	char name_buf[200] = {0};
	snprintf(name_buf, 200, "pci-%x%x", vendor, dev->deviceID);
	dev->dev.name = strdup(name_buf);
	assert(dev->dev.name);
	
	bus_add_device(&pci_bus, (struct device*) dev);
	if(likely(last))
		last->next = dev;
	else
		linked_list = dev;
	if(parent)
	{
		dev->dev.parent = (struct device*) parent;
		/* Failing to enumerate PCI devices is pretty much a failure anyway */
		assert(list_add(&dev->dev.children, dev) == 0);		
	}

	last = dev;
	if(header & 0x80)
	{
		for(int i = 1; i < 8;i++)
		{
			pci_enumerate_device(bus, device, i, dev);
		}
	}
}
void pci_enumerate_devices(void)
{
	for(uint16_t slot = 0; slot < 256; slot++)
	{
		for(uint16_t device = 0; device < 32; device++)
		{
			pci_enumerate_device(slot, device, 0, NULL);	
		}
	}
}
pcibar_t* pci_get_bar(struct pci_device *dev, uint8_t barindex)
{
	uint8_t offset = 0x10 + 0x4 * barindex;
	uint32_t i = (uint32_t) pci_read(dev, offset, sizeof(uint32_t));
	pcibar_t* pcibar = malloc(sizeof(pcibar_t));
	if(!pcibar)
		return NULL;
	pcibar->address = i & 0xFFFFFFF0;
	pcibar->isIO = i & 1;
	if(i & 1)
		pcibar->address = i & 0xFFFFFFFC;
	pcibar->isPrefetchable = i & 4;
	__pci_write_dword(dev->bus, dev->device, dev->function, offset, 0xFFFFFFFF);
	size_t size = (~((__pci_config_read_dword(dev->bus, dev->device, dev->function, offset) & 0xFFFFFFF0))) + 1;
	pcibar->size = size;
	__pci_write_dword(dev->bus, dev->device, dev->function, offset, i);
	return pcibar;
}
uint16_t pci_get_intn(uint8_t slot, uint8_t device, uint8_t function)
{
	return acpi_get_irq_routing_for_dev(slot, device, function);
}
void pci_init()
{
	/* Register the PCI bus */
	bus_register(&pci_bus);
	/* Check every pci device and add it onto the bus */
	pci_enumerate_devices();
}
struct pci_device *get_pcidev_from_vendor_device(uint16_t deviceid, uint16_t vendorid)
{
	for(struct pci_device *i = linked_list; i;i = i->next)
	{
		if(i->deviceID == deviceid && i->vendorID == vendorid)
			return i;
	}
	return NULL;
}
struct pci_device *get_pcidev_from_classes(uint8_t class, uint8_t subclass, uint8_t progif)
{
	for(struct pci_device *i = linked_list; i;i = i->next)
	{
		if(i->pciClass == class && i->subClass == subclass && i->progIF == progif)
			return i;
	}
	return NULL;
}
void pci_set_barx(uint8_t slot, uint8_t device, uint8_t function, uint8_t index, uint32_t address, uint8_t is_io, uint8_t is_prefetch)
{
	uint32_t bar = address | is_io | (is_prefetch << 2);
	__pci_write_dword(slot, device, function, PCI_BARx(index), bar);
}
/* All the PCI drivers' headers */
#include <drivers/e1000.h>
#include <drivers/ata.h>
pci_driver_t pci_drivers[] =
{
	{E1000_DEV, INTEL_VEND, CLASS_NETWORK_CONTROLLER, 0, 0, PCI_DRIVER_SPECIFIC, e1000_init},
	{E1000_I217, INTEL_VEND, CLASS_NETWORK_CONTROLLER, 0, 0, PCI_DRIVER_SPECIFIC, e1000_init},
	{E1000_82577LM, INTEL_VEND, CLASS_NETWORK_CONTROLLER, 0, 0, PCI_DRIVER_SPECIFIC, e1000_init},
	{E1000E_DEV, INTEL_VEND, CLASS_NETWORK_CONTROLLER, 0, 0, PCI_DRIVER_SPECIFIC, e1000_init},
	{0, 0, CLASS_MASS_STORAGE_CONTROLLER, 1, 0, PCI_DRIVER_GENERIC, ata_init},
};

const size_t pci_driver_array_entries = sizeof(pci_drivers) / sizeof(pci_driver_t);
void pci_initialize_drivers()
{
	for(size_t i = 0; i < pci_driver_array_entries; i++)
	{
		if(pci_drivers[i].driver_type == PCI_DRIVER_GENERIC)
		{
			struct pci_device *dev = get_pcidev_from_classes(pci_drivers[i].pciClass, pci_drivers[i].subClass, pci_drivers[i].progIF);
			if(!dev)
				continue;
			pci_drivers[i].cb(dev);
		}	
		else
		{
			struct pci_device *dev = get_pcidev_from_vendor_device(pci_drivers[i].deviceID, pci_drivers[i].vendorID);
			if(!dev)
				continue;
			pci_drivers[i].cb(dev);
		}
			
	}
}
struct pci_device *get_pcidev(uint8_t bus, uint8_t device, uint8_t function)
{
	for(struct pci_device *i = linked_list; i;i = i->next)
	{
		if(i->bus == bus && i->device == device && i->function == function)
			return i;
	}
	return NULL;
}
void __pci_write_byte(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset, uint8_t data)
{
	uint32_t address;
	uint32_t lbus  = (uint32_t)bus;
	uint32_t lslot = (uint32_t)slot;
	uint32_t lfunc = (uint32_t)func;

	/* create configuration address as per Figure 1 */
	address = (uint32_t)((lbus << 16) | (lslot << 11) |
		  (lfunc << 8) | (offset & 0xfc) | ((uint32_t)0x80000000));
	
	acquire_spinlock(&pci_lock);
	/* write out the address */
	outl(CONFIG_ADDRESS, address);
	/* read in the data */
	outb(CONFIG_DATA, data);
	release_spinlock(&pci_lock);
}
uint8_t __pci_read_byte(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset)
{
	uint32_t address;
	uint32_t lbus  = (uint32_t)bus;
	uint32_t lslot = (uint32_t)slot;
	uint32_t lfunc = (uint32_t)func;

	/* create configuration address as per Figure 1 */
	address = (uint32_t)((lbus << 16) | (lslot << 11) |
		  (lfunc << 8) | (offset & 0xfc) | ((uint32_t)0x80000000));
	
	acquire_spinlock(&pci_lock);
	/* write out the address */
	outl(CONFIG_ADDRESS, address);
	/* read in the data */
	uint8_t ret = inb(CONFIG_DATA);
	release_spinlock(&pci_lock);

	return ret;
}
void __pci_write_qword(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset, uint64_t data)
{
	uint32_t address;
	uint32_t lbus  = (uint32_t)bus;
	uint32_t lslot = (uint32_t)slot;
	uint32_t lfunc = (uint32_t)func;

	/* create configuration address */
	address = (uint32_t)((lbus << 16) | (lslot << 11) |
		  (lfunc << 8) | (offset & 0xfc) | ((uint32_t)0x80000000));
	
	acquire_spinlock(&pci_lock);
	/* write out the address */
	outl(CONFIG_ADDRESS, address);
	/* Write out the lower half of the data */
	outl(CONFIG_DATA, data & 0xFFFFFFFF);
	address = (uint32_t)((lbus << 16) | (lslot << 11) |
		  (lfunc << 8) | ((offset+4) & 0xfc) | ((uint32_t) 0x80000000));

	/* write out the address */
	outl(CONFIG_ADDRESS, address);
	outl(CONFIG_DATA, data & 0xFFFFFFFF00000000);
	release_spinlock(&pci_lock);
}
void pci_write(struct pci_device *dev, uint64_t value, uint16_t off, size_t size)
{
	if(size == sizeof(uint8_t))
		__pci_write_byte(dev->bus, dev->device, dev->function, off, (uint8_t) value);
	if(size == sizeof(uint16_t))
		__pci_write_word(dev->bus, dev->device, dev->function, off, (uint16_t) value);
	if(size == sizeof(uint32_t))
		__pci_write_dword(dev->bus, dev->device, dev->function, off, (uint32_t) value);
	if(size == sizeof(uint64_t))
		__pci_write_qword(dev->bus, dev->device, dev->function, off, value);
}
uint64_t pci_read(struct pci_device *dev, uint16_t off, size_t size)
{
	uint64_t val = 0;
	switch(size)
	{
		case sizeof(uint16_t):
			val = __pci_config_read_word(dev->bus, dev->device, dev->function, off);
			break;
		case sizeof(uint32_t):
			val = __pci_config_read_dword(dev->bus, dev->device, dev->function, off);
			break;
		case sizeof(uint64_t):
			val = __pci_config_read_dword(dev->bus, dev->device, dev->function, off);
			break;
		default:
			val = __pci_read_byte(dev->bus, dev->device, dev->function, off);
			break;
	}
	return val;
}
void pci_enable_busmastering(struct pci_device *dev)
{
	uint32_t command_register = (uint32_t) pci_read(dev, PCI_COMMAND, sizeof(uint32_t));
	pci_write(dev, command_register | PCI_COMMAND_BUS_MASTER, PCI_COMMAND, sizeof(uint32_t));
}
uint16_t pci_get_status(struct pci_device *dev)
{
	return (uint16_t) pci_read(dev, PCI_REG_STATUS, sizeof(uint16_t));
}
off_t pci_find_capability(struct pci_device *dev, uint8_t cap)
{
	uint16_t status = pci_get_status(dev);
	if(!(status & PCI_STATUS_CAP_LIST_SUPPORTED))
		return -1;
	
	uint8_t offset = (uint8_t) pci_read(dev, PCI_REG_CAPABILTIES_POINTER, sizeof(uint8_t)) & ~3;

	while(offset)
	{
		uint16_t _cap = pci_read(dev, offset, sizeof(uint16_t));
		if((_cap & 0xFF) == cap)
			return offset;
		offset = ((uint8_t) (_cap & 0xFF00)) & ~3;
	}
	return -1;
}
int pci_enable_msi(struct pci_device *dev)
{
	/*off_t offset = pci_find_capability(dev, PCI_CAP_ID_MSI);

	printk("Offset: %x\n", offset);*/
	return -1;
}

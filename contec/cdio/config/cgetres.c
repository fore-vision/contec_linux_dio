////////////////////////////////////////////////////////////////////////////////
/// @file   cgetres.c
/// @brief  API-DIO(LNX) config(PCI) : Common driver resource acquisition source file
/// @author &copy;CONTEC CO.,LTD.
/// @since  2002
////////////////////////////////////////////////////////////////////////////////

#ifndef LINUX_VERSION_CODE
#include <linux/version.h>
#endif

#include <linux/module.h>
#include <linux/fs.h>
#include <linux/delay.h>
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4,15,0))
#include <linux/uaccess.h>
#else
#include <asm/io.h>
#include <asm/uaccess.h>
#endif
#include <linux/pci.h>		
#include <linux/ioport.h>
#include <linux/interrupt.h>
#include <linux/proc_fs.h>

#ifndef KERNEL_VERSION
#define KERNEL_VERSION(a,b,c)	((a)<<16+(b)<<8+c)
#endif

#include "cgetres.h"

#define		THIS_DEVICE_NAME		"cgetres"

// Memory Input/Output
#define	_inm(adr)						ioread8(adr)
#define	_inmw(adr)						ioread16(adr)
#define	_inmd(adr)						ioread32(adr)
#define	_outm(adr, data)				iowrite8(data, adr)
#define	_outmw(adr, data)				iowrite16(data, adr)
#define	_outmd(adr, data)				iowrite32(data, adr)
//================================================================
// External variable
//================================================================
static	int	major = 0;
//================================================================
// Function prototype
//================================================================
// For kernel version 2.2.x, 2.4.x and 2.6.x
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,2,0) && LINUX_VERSION_CODE < KERNEL_VERSION(2,7,0))
int	Ccom_ioctl(struct inode *inode, struct file *file, unsigned int ctl_code, unsigned long param);
// For kernel version 3.10.x -
#elif (LINUX_VERSION_CODE >= KERNEL_VERSION(3,10,0))
long	Ccom_ioctl(struct file *file, unsigned int ctl_code, unsigned long param);
#endif
long pci_read_revision(
	struct pci_dev *ppci_dev, 
	unsigned char* board_id, 
	unsigned char* board_revision);
long Ccom_get_pci_resource(
	unsigned short vendor_id,
 	unsigned short device_id,
	unsigned short board_id,
	unsigned long *io_addr,
	unsigned long *io_num,
	unsigned long *mem_addr,
	unsigned long *mem_num,
	unsigned long *irq);
long Ccom_get_pci_contec_revision(
	unsigned short vendor_id,
 	unsigned short device_id,
	unsigned short board_id,
	unsigned long *revision_id);
long Ccom_get_pci_subsys(
	unsigned short vendor_id,
 	unsigned short device_id,
	unsigned short board_id,
	unsigned long *subsys_id);
//================================================================
// Load module
//================================================================
int init_module(void)
{
	static struct file_operations fops;

	//--------------------------------------
	// Do not export the symbol
	//--------------------------------------
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,2,0) && LINUX_VERSION_CODE < KERNEL_VERSION(2,5,0))
	EXPORT_NO_SYMBOLS;
#endif
	//--------------------------------------
	// Copy into the entry of device list
	//--------------------------------------
	// File operation structure (function entry points)
	// For kernel version 3.10.x -
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3,10,0))
	fops.unlocked_ioctl	= Ccom_ioctl;
#else
	fops.ioctl	= Ccom_ioctl;
#endif
	//--------------------------------------
	// Register a character device
	//--------------------------------------
	major = 0;
	major = register_chrdev(major, THIS_DEVICE_NAME, &fops);
	if (major < 0) {
		return major;
	}
	return 0;
}
//================================================================
// Unload module
//================================================================
void cleanup_module(void)
{
	//--------------------------------------
	// Unregister the character device
	//--------------------------------------
	unregister_chrdev(major, THIS_DEVICE_NAME);
}

//================================================================
// Dispatch entry
//================================================================
// For kernel version 2.2.x, 2.4.x and 2.6.x
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,2,0) && LINUX_VERSION_CODE < KERNEL_VERSION(2,7,0))
int	Ccom_ioctl(struct inode *inode, struct file *file, unsigned int ctl_code, unsigned long param)
// For kernel version 3.10.x -
#elif (LINUX_VERSION_CODE >= KERNEL_VERSION(3,10,0))
long	Ccom_ioctl(struct file *file, unsigned int ctl_code, unsigned long param)
#endif
{
	int		iret = -EINVAL;
	int		copy_from_ret;
	int		copy_to_ret;

	switch(ctl_code) {
	case CGETRES_IOC_GET_PCI:
		iret = 0;
		copy_from_ret=0;
		copy_to_ret=0;
		{
		PGETPCIRES pres;
		pres = kmalloc(sizeof(GETPCIRES),GFP_KERNEL);
		copy_from_ret = copy_from_user(pres,(PGETPCIRES)param,sizeof(GETPCIRES));	
		pres->ret = Ccom_get_pci_resource(
											(unsigned short)pres->vendor_id,
				 							(unsigned short)pres->device_id,
											(unsigned short)pres->board_id,
											pres->io_addr,
											&pres->io_num,
											pres->mem_addr,
											&pres->mem_num,
											&pres->irq);
		copy_to_ret = copy_to_user((PGETPCIRES)param,pres,sizeof(GETPCIRES));		
		kfree(pres);
		}
		break;
	case CGETREV_IOC_GET_PCI:
		iret = 0;
		copy_from_ret=0;
		copy_to_ret=0;
		{
		PGETPCIREV	prev;
		prev = kmalloc(sizeof(GETPCIREV),GFP_KERNEL);	
		copy_from_ret = copy_from_user(prev,(PGETPCIREV)param,sizeof(GETPCIREV));
		prev->ret = Ccom_get_pci_contec_revision(
													(unsigned short)prev->vendor_id,
						 							(unsigned short)prev->device_id,
													(unsigned short)prev->board_id,
													&prev->revision_id);
		copy_to_ret = copy_to_user((PGETPCIREV)param,prev,sizeof(GETPCIREV));			
		kfree(prev);
		}
		break;
	case CGETSUBSYS_IOC_GET_PCI:
		iret = 0;
		copy_from_ret=0;
		copy_to_ret=0;
		{
		PGETPCISUBSYS	presubsys;
		presubsys = kmalloc(sizeof(GETPCISUBSYS),GFP_KERNEL);
		copy_from_ret = copy_from_user(presubsys,(PGETPCISUBSYS)param,sizeof(GETPCISUBSYS));
		presubsys->ret = Ccom_get_pci_subsys(
													(unsigned short)presubsys->vendor_id,
						 							(unsigned short)presubsys->device_id,
													(unsigned short)presubsys->board_id,
													&presubsys->subsys_id);
		copy_to_ret = copy_to_user((PGETPCISUBSYS)param,presubsys,sizeof(GETPCISUBSYS));
		kfree(presubsys);
		}
		break;
	default:
		break;
	}
	return iret;
}
//=====================================================================================
// Function for Get the Board ID and the Board Revision of the PCI device.
//=====================================================================================
long pci_read_revision(struct pci_dev *ppci_dev, unsigned char* board_id, unsigned char* board_revision)
{
	unsigned long	io_tmp[6];
	unsigned long	io_num_tmp = 0;
	unsigned long	mem_tmp[6];
	unsigned long	mem_num_tmp = 0;
	int				mem_flag = 0;
	unsigned char*	mem_base = NULL;								// Base address of memory
	unsigned char*	bm_mem_base = NULL;
	unsigned short	mem_data, mem_data2, mem_data3;
	unsigned char	BoardID_MEM, BoardRev_MEM;
	int		i = 0;
	//----------------------------------------
	// Loop for retrieving I/O and memory
	//----------------------------------------
	udelay(1000);
	for (i=0; i<6 ;i++) {
		//----------------------------------------
		// Kernel version 2.4.X -
		//----------------------------------------
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,4,0))
		if (pci_resource_flags(ppci_dev, i) & IORESOURCE_IO) {
			io_tmp[io_num_tmp] = pci_resource_start(ppci_dev, i);
			io_num_tmp++;
		} else if (pci_resource_flags(ppci_dev, i) & IORESOURCE_MEM) {
			mem_flag = 1;
			mem_tmp[mem_num_tmp] = pci_resource_start(ppci_dev, i) & PCI_BASE_ADDRESS_MEM_MASK;
			mem_num_tmp++;
		} else{
			//do nothing
		}
#endif
		//----------------------------------------
		// Kernel version 2.2.X
		//----------------------------------------
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,2,0) && LINUX_VERSION_CODE < KERNEL_VERSION(2,3,0))
		if ( ppci_dev->base_address[i] & PCI_BASE_ADDRESS_SPACE_IO) {
			io_tmp[io_num_tmp] = ppci_dev->base_address[i] & PCI_BASE_ADDRESS_IO_MASK;
			io_num_tmp++;
		} else if ( ppci_dev->base_address[i] & PCI_BASE_ADDRESS_SPACE_MEMORY) {
			mem_tmp[mem_num_tmp] =  ppci_dev->base_address[i] & PCI_BASE_ADDRESS_MEM_MASK;
			mem_num_tmp++;
		}
#endif
	}
	if (mem_flag == 1) {
		mem_base		= (unsigned char *)ioremap(mem_tmp[0], 0xfffffff);
		bm_mem_base		= (unsigned char *)ioremap(mem_tmp[1], 0xfffffff);
		if (mem_base == NULL || bm_mem_base == NULL) {
			return -1;
		}
		mem_data	= _inmw(mem_base + 0xB000054);
		mem_data2	= _inmw(mem_base + 0xB000056);
		//--------------------------------------
		///	Save board ID in extension
		//--------------------------------------
		if ((mem_data == 0x1221) && (mem_data2 == 0xA642)) {
			mem_data3	= _inmw(mem_base + 0xB000050);
			BoardID_MEM	= mem_data3 & 0xff;
			*board_id = BoardID_MEM;
		} else {
			mem_data3	= _inmw(bm_mem_base + 0xB000050);
			BoardID_MEM	= mem_data3 & 0xff;
			*board_id = BoardID_MEM;
		}
		//--------------------------------------
		//	Save Board Revision Configuration Register 0x40h-> Revision
		//--------------------------------------
		if ((mem_data == 0x1221) && (mem_data2 == 0xA642)) {
			mem_data3	= _inmw(mem_base + 0xB000052);
			BoardRev_MEM	= (mem_data3 & 0xff00) >> 8;
			*board_revision = BoardRev_MEM;
		} else {
			mem_data3	= _inmw(bm_mem_base + 0xB000052);
			BoardRev_MEM	= (mem_data3 & 0xff00) >> 8;
			*board_revision = BoardRev_MEM;
		}
		iounmap(bm_mem_base);
		iounmap(mem_base);
	} else {
		pci_read_config_byte(ppci_dev, PCI_REVISION_ID, (unsigned char *)board_id);
		pci_read_config_byte(ppci_dev,0x40,(unsigned char *)board_revision);
	}
	return 0;

}
//================================================================
// Function for retrieving PCI resource
//================================================================
long Ccom_get_pci_resource(
	unsigned short vendor_id,
 	unsigned short device_id,
	unsigned short board_id,
	unsigned long *io_addr,
	unsigned long *io_num,
	unsigned long *mem_addr,
	unsigned long *mem_num,
	unsigned long *irq)
{

	struct pci_dev	*ppci_dev = NULL;
	unsigned char	irq_pin;
	int				found_num;
	unsigned long	io_tmp[6];
	unsigned long	io_num_tmp;
	unsigned long	mem_tmp[6];
	unsigned long	mem_num_tmp;
	unsigned long	irq_tmp;
	unsigned long	i;
	unsigned char 	real_board_id;
	unsigned char 	board_revision;

	//----------------------------------------
	// If PCI bus is not present, it indicates No Board Error
	//----------------------------------------
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,2,0) && LINUX_VERSION_CODE < KERNEL_VERSION(2,5,0))
	if (!pci_present()) {
		return -1;
	}
#endif
	//----------------------------------------
	// If the area is not sufficient, it indicates Array Size Error
	//----------------------------------------
	if (io_addr == NULL || *io_num < 6) {
		return -1;
	}
	if (mem_addr == NULL || *mem_num < 6) {
		return -1;
	}
	if (irq == NULL) {
		return -1;
	}
	//----------------------------------------
	// Prepare and initialize the return data
	// for the case of no board found
	//----------------------------------------
	*io_num		= 0;
	*mem_num	= 0;
	*irq		= 0xffffffff;
	//----------------------------------------
	// Initialize the temporary data and the number of boards
	//----------------------------------------
	io_num_tmp	= 0;
	mem_num_tmp	= 0;
	irq_tmp		= 0xffffffff;
	found_num	= 0;
	//----------------------------------------
	// For the found PCI board
	//----------------------------------------
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,21))
	while ((ppci_dev = pci_get_device(vendor_id, device_id, ppci_dev))) {
#else
	while ((ppci_dev = pci_find_device(vendor_id, device_id, ppci_dev))) {
#endif
		//----------------------------------------
		// Pass the case that the vendor ID and the device ID do not match with each one you want
		//----------------------------------------
		if (!(ppci_dev->vendor == vendor_id && ppci_dev->device == device_id)) {
			continue;
		}
		//----------------------------------------
		// Pass the case that the revision ID (Board ID) does not match with that you want
		//----------------------------------------
		if (pci_read_revision(ppci_dev, &real_board_id, &board_revision) != 0) {
			return -1;
		}
		if (real_board_id != board_id) {
			continue;
		}
		//----------------------------------------
		// Board is found!
		//----------------------------------------
		found_num++;
		io_num_tmp	= 0;
		mem_num_tmp	= 0;
		irq_tmp		= 0xffffffff;
		//----------------------------------------
		// Loop for retrieving I/O and memory
		//----------------------------------------
		for (i=0; i<6 ;i++) {
			//----------------------------------------
			// Kernel version 2.4.X -
			//----------------------------------------
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,4,0))
			if (!pci_resource_start(ppci_dev, i)) {
   				break;
			}
			if (pci_resource_flags(ppci_dev, i) & IORESOURCE_IO) {
				io_tmp[io_num_tmp] = pci_resource_start(ppci_dev, i);
				io_num_tmp++;
			} else {
				mem_tmp[mem_num_tmp] = pci_resource_start(ppci_dev, i) & PCI_BASE_ADDRESS_MEM_MASK;
				mem_num_tmp++;
			}
#endif
			//----------------------------------------
			// Kernel version 2.2.X
			//----------------------------------------
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,2,0) && LINUX_VERSION_CODE < KERNEL_VERSION(2,3,0))
			if ( ppci_dev->base_address[i] & PCI_BASE_ADDRESS_SPACE_IO) {
				io_tmp[io_num_tmp] = ppci_dev->base_address[i] & PCI_BASE_ADDRESS_IO_MASK;
				io_num_tmp++;
			} else if ( ppci_dev->base_address[i] & PCI_BASE_ADDRESS_SPACE_MEMORY) {
				mem_tmp[mem_num_tmp] =  ppci_dev->base_address[i] & PCI_BASE_ADDRESS_MEM_MASK;
				mem_num_tmp++;
			}
#endif
		}
		//----------------------------------------
		// Retrieve IRQ
		//----------------------------------------
		pci_read_config_byte(ppci_dev, PCI_INTERRUPT_PIN, &irq_pin);
		if (irq_pin != 0 && ppci_dev->irq != 0) {
			irq_tmp = ppci_dev->irq;
		}
	}
	//----------------------------------------
	// Return an error if the found number is equal to 0 or more than 1
	//----------------------------------------
	if (found_num == 0) {
		return -1;
	}
	if (found_num > 1) {
		return -1;
	}
	//----------------------------------------
	// If the found number is equal to 1, copy the resource and return
	//----------------------------------------
	for (i=0; i<io_num_tmp; i++) {
		io_addr[i]	= io_tmp[i];
	}
	*io_num = io_num_tmp;
	for (i=0; i<mem_num_tmp; i++) {
		mem_addr[i]	= mem_tmp[i];
	}
	*mem_num	= mem_num_tmp;
	*irq		= irq_tmp;
	return 0;
}

//================================================================
// Function for retrieving PCI revision
//================================================================
long Ccom_get_pci_contec_revision(
	unsigned short vendor_id,
 	unsigned short device_id,
	unsigned short board_id,
	unsigned long *revision_id)
{
	struct pci_dev	*ppci_dev = NULL;
	int				found_num;
	unsigned char 	real_board_id;
	unsigned char 	board_revision;

	//----------------------------------------
	// Prepare and initialize the return data
	// for the case of no board found
	//----------------------------------------
	*revision_id	= 0;
	//----------------------------------------
	// Initialize the temporary data and the number of boards
	//----------------------------------------
	found_num		= 0;
	//----------------------------------------
	// For the found PCI board
	//----------------------------------------
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,21))
	while ((ppci_dev = pci_get_device(vendor_id, device_id, ppci_dev))) {
#else
	while ((ppci_dev = pci_find_device(vendor_id, device_id, ppci_dev))) {
#endif
		//----------------------------------------
		// Pass the case that the vendor ID and the device ID do not match with each one you want
		//----------------------------------------
		if (!(ppci_dev->vendor == vendor_id && ppci_dev->device == device_id)) {
			continue;
		}
		//----------------------------------------
		// Pass the case that the revision ID (Board ID) does not match with that you want
		//----------------------------------------
		if (pci_read_revision(ppci_dev, &real_board_id, &board_revision) != 0) {
				return -1;
		}
		if (real_board_id != board_id) {
			continue;
		}
/*
		//----------------------------------------
		// Copy the revision ID (Contec definition) to return value
		//----------------------------------------
		pci_read_config_byte(ppci_dev,0x40,&revision);
*/
		*revision_id	= (unsigned long)board_revision;
		//----------------------------------------
		// Board is found!
		//----------------------------------------
		found_num++;
	}
	//----------------------------------------
	// Return an error if the found number is equal to 0 or more than 1
	//----------------------------------------
	if (found_num == 0) {
		return -1;
	}
	if (found_num > 1) {
		return -1;
	}
	//----------------------------------------
	// If the found number is equal to 1, normal complete
	//----------------------------------------
	return 0;
}

//================================================================
// Function for retrieving PCI SUBSYSTEM ID
//================================================================
long Ccom_get_pci_subsys(
	unsigned short vendor_id,
 	unsigned short device_id,
	unsigned short board_id,
	unsigned long *subsys_id)
{
	struct pci_dev	*ppci_dev = NULL;
	unsigned char	revision;
	unsigned short	subsystem;
	int				found_num;

	//----------------------------------------
	// Prepare and initialize the return data
	// for the case of no board found
	//----------------------------------------
	*subsys_id	= 0;
	//----------------------------------------
	// Initialize the temporary data and the number of boards
	//----------------------------------------
	found_num		= 0;
	//----------------------------------------
	// For the found PCI board
	//----------------------------------------
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,21))
	while ((ppci_dev = pci_get_device(vendor_id, device_id, ppci_dev))) {
#else
	while ((ppci_dev = pci_find_device(vendor_id, device_id, ppci_dev))) {
#endif
		//----------------------------------------
		// Pass the case that the vendor ID and the device ID do not match with each one you want
		//----------------------------------------
		if (!(ppci_dev->vendor == vendor_id && ppci_dev->device == device_id)) {
			continue;
		}
		//----------------------------------------
		// Pass the case that the revision ID (Board ID) does not match with that you want
		//----------------------------------------
		pci_read_config_byte(ppci_dev,PCI_REVISION_ID,&revision);
		if (revision != board_id) {
			continue;
		}
		//----------------------------------------
		// Copy the SUBSYSTEM ID to return value
		//----------------------------------------
		pci_read_config_word(ppci_dev,PCI_SUBSYSTEM_ID,&subsystem);
		*subsys_id	= (unsigned long)subsystem;
		//----------------------------------------
		// Board is found!
		//----------------------------------------
		found_num++;
	}
	//----------------------------------------
	// Return an error if the found number is equal to 0 or more than 1
	//----------------------------------------
	if (found_num == 0) {
		return -1;
	}
	if (found_num > 1) {
		return -1;
	}
	//----------------------------------------
	// If the found number is equal to 1, normal complete
	//----------------------------------------
	return 0;
}

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,0))
MODULE_DESCRIPTION("API-DIO(LNX) Resource Acq Device Driver");
MODULE_AUTHOR("CONTEC CO., LTD.");
MODULE_VERSION("6.80");
MODULE_LICENSE("Proprietary");
#endif

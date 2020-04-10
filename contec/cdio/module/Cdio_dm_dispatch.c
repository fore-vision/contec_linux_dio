////////////////////////////////////////////////////////////////////////////////
/// @file   Cdio_dm_dispatch.c
/// @brief  API-DIO(LNX) PCI Module - Dispatch source file (for DM Board)
/// @author &copy;CONTEC CO.,LTD.
/// @since  2002
////////////////////////////////////////////////////////////////////////////////

#define __NO_VERSION__
#include <linux/module.h>
#include <linux/version.h>
#include <linux/fs.h>
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
#include <linux/delay.h>

#include "Ccom_module.h"
#include "Cdio_dispatch.h"
#include "Cdio_dm_dispatch.h"
#include "Cdio_bios.h"
#include "BusMaster.h"
#include "BusMasterMemory.h"

//--------------------------------------------------
// Macro definition
//--------------------------------------------------
#define	BM_MEM_INP_NXT_DESC			0x06000040					// S/G Address of Writing destination for 32DM3
#define	BM_MEM_OUT_NXT_DESC			0x08000040					// S/G Address of Writing destination for 32DM3
#define	BM_MEM_INP_ReadAdd			0x07000000					// S/G Address of Reading destination for 32DM3		(Input)

// I/O Address for 32DM3
#define		MEM_DIO_PORT_ADD	(dio_data->bm_board.dwAddr_mem+0xB000000)			// Base Port Address for memory map					(Input/Output)
#define		MEM_DIO_INP_ADD		(dio_data->bm_board.dwAddrBM_mem+0x6020000)			// Base Port Address for memory map					(Input)
#define		MEM_DIO_OUT_ADD		(dio_data->bm_board.dwAddrBM_mem+0x8020000)			// Base Port Address for memory map					(Output)
#define		MEM_DIO_INP_PRE_ADD	(dio_data->bm_board.dwAddrBM_mem+0x6010000)			// Prefetcher CSR Base Port Address	for memory map	(Input)
#define		MEM_DIO_OUT_PRE_ADD	(dio_data->bm_board.dwAddrBM_mem+0x8010000)			// Prefetcher CSR Base Port Address	for memory map	(Output)
#define		BM_MEM_Reset		(dio_data->bm_board.dwAddr_mem+0xB000044)			// Reset for 32DM3									(Output)
#define		MEM_OUT_FIFO_ADD	(dio_data->bm_board.dwAddr_mem+0x9000000)			// FIFO address for memory map						(Output)

// Macro for I/O
/*
#define BmInpB(p, d)		{d = (unsigned char)inb(p);		printk("IN %04X, %02X\n", p, d);}
#define BmInpW(p, d)		{d = (unsigned short)inw(p);	printk("IN %04X, %04X\n", p, d);}
#define BmInpD(p, d)		{d = (unsigned long)inl(p);		printk("IN %04X, %08X\n", p, d);}
#define BmOutB(p, d)		{outb(d, p);					printk("OUT %04X, %02X\n", p, d);}
#define BmOutW(p, d)		{outw(d, p);					printk("OUT %04X, %04X\n", p, d);}
#define BmOutD(p, d)		{outl(d, p);					printk("OUT %04X, %08X\n", p, d);}
*/

#define BmInpB(p, d)		d = (unsigned char)inb(p)
#define BmInpW(p, d)		d = (unsigned short)inw(p)
#define BmInpD(p, d)		d = (unsigned long)inl(p)
#define BmOutB(p, d)		outb(d, p)
#define BmOutW(p, d)		outw(d, p)
#define BmOutD(p, d)		outl(d, p)

// Memory Input/Output
#define	_inm(adr)			ioread8(adr)
#define	_inmw(adr)			ioread16(adr)
#define	_inmd(adr)			ioread32(adr)
#define	_outm(adr, data)	iowrite8(data, adr)
#define	_outmw(adr, data)	iowrite16(data, adr)
#define	_outmd(adr, data)	iowrite32(data, adr)

//================================================================
// Each function 
//================================================================
//================================================================
// I/O direction setting
//================================================================
long Cdio_dm_set_dir(void *device_ext, PCDIO_DM_SET_DIR param)
{
	PCDIO_DATA			dio_data;
	unsigned long		flags;

	//----------------------------------------
	// Cast to structure
	//----------------------------------------
	dio_data	= (PCDIO_DATA)device_ext;
	if (dio_data == NULL || param == NULL) {
		return -1;
	}
	//----------------------------------------
	// Check the supported boards
	//----------------------------------------
	if(dio_data->bios_d.b_info->board_type != CDIOBI_BT_PCI_DM){
		param->ret= DIO_ERR_SYS_NOT_SUPPORTED;
		return 0;
	}
	//----------------------------------------
	// Get the spinlock
	//----------------------------------------
	spin_lock_irqsave(&dio_data->spinlock_flag, flags);
	//---------------------------------------------
	// Set direction to the hardware directly.
	//---------------------------------------------
	if (strcmp(dio_data->board_name, "DIO-32DM3-PE") == 0){
		switch(param->dir) {
		case PI_32:
			_outmw(MEM_DIO_PORT_ADD + 0x04, 0x00);
			break;
		case PO_32:
			_outmw(MEM_DIO_PORT_ADD + 0x04, 0x03);
			break;
		case PIO_1616:
			_outmw(MEM_DIO_PORT_ADD + 0x04, 0x02);
			break;
		default:
			param->ret = DIO_ERR_SYS_DIRECTION;
			//------------------------------------
			// Release the spinlock
			//------------------------------------
			spin_unlock_irqrestore(&dio_data->spinlock_flag, flags);
			return 0;
			break;
		}
	} else {
		switch(param->dir) {
		case PI_32:		BmOutW(dio_data->bm_board.dwAddr + 0x04, 0x00);	break;
		case PO_32:		BmOutW(dio_data->bm_board.dwAddr + 0x04, 0x03);	break;
		case PIO_1616:	BmOutW(dio_data->bm_board.dwAddr + 0x04, 0x02);	break;
		default:		param->ret = DIO_ERR_SYS_DIRECTION;	return 0; break;
		}
	}
	//----------------------------------------
	// Save input/output direction
	//----------------------------------------
	dio_data->bm_board.direction		= param->dir;
	dio_data->bios_d.direction			= param->dir;
	//----------------------------------------
	// Initialize the output latch data
	//----------------------------------------
	Cdiobi_init_latch_data(&dio_data->bios_d);
	//----------------------------------------
	// Release the spinlock
	//----------------------------------------
	spin_unlock_irqrestore(&dio_data->spinlock_flag, flags);
	//----------------------------------------
	// Set the return value
	//----------------------------------------
	param->ret	= DIO_ERR_SUCCESS;
	return 0;
}

//================================================================
// Stand alone setting
//================================================================
long Cdio_dm_set_stand_alone(void *device_ext, PCDIO_DM_SET_STAND_ALONE param)
{
	PCDIO_DATA		dio_data;
	unsigned long	flags;

	//----------------------------------------
	// Cast to structure
	//----------------------------------------
	dio_data	= (PCDIO_DATA)device_ext;
	if (dio_data == NULL || param == NULL) {
		return -1;
	}
	//----------------------------------------
	// Check the supported boards
	//----------------------------------------
	if(dio_data->bios_d.b_info->board_type != CDIOBI_BT_PCI_DM){
		param->ret = DIO_ERR_SYS_NOT_SUPPORTED;
		return 0;
	}
	//----------------------------------------
	// Get the spinlock
	//----------------------------------------
	spin_lock_irqsave(&dio_data->spinlock_flag, flags);
	//---------------------------------------------
	// Set to the hardware directly.
	//---------------------------------------------
	if (strcmp(dio_data->board_name, "DIO-32DM3-PE") == 0){
		//ExtSig1
		_outmw(MEM_DIO_PORT_ADD + 0x08, 0x01);
		_outmd(MEM_DIO_PORT_ADD + 0x0c, 0x00000000);
		//ExtSig2
		_outmw(MEM_DIO_PORT_ADD + 0x08, 0x02);
		_outmd(MEM_DIO_PORT_ADD + 0x0c, 0x00000000);
		//ExtSig3
		_outmw(MEM_DIO_PORT_ADD + 0x08, 0x03);
		_outmd(MEM_DIO_PORT_ADD + 0x0c, 0x00000000);
		// Sync Control Connector
		_outmd(MEM_DIO_PORT_ADD + 0x18, 0);
		dio_data->bm_board.Master.MasterEnable	= 0;
		dio_data->bm_board.Master.SlaveEnable	= 0;
	} else {
		//ExtSig1
		BmOutW(dio_data->bm_board.dwAddr + 0x08, 0x01);
		BmOutD(dio_data->bm_board.dwAddr + 0x0c, 0x00000000);
		//ExtSig2
		BmOutW(dio_data->bm_board.dwAddr + 0x08, 0x02);
		BmOutD(dio_data->bm_board.dwAddr + 0x0c, 0x00000000);
		//ExtSig3
		BmOutW(dio_data->bm_board.dwAddr + 0x08, 0x03);
		BmOutD(dio_data->bm_board.dwAddr + 0x0c, 0x00000000);
		// Sync Control Connector
		BmOutD(dio_data->bm_board.dwAddr + 0x18, 0);
	}
	//----------------------------------------
	// Release the spinlock
	//----------------------------------------
	spin_unlock_irqrestore(&dio_data->spinlock_flag, flags);
	//----------------------------------------
	// Set the return value
	//----------------------------------------
	param->ret	= DIO_ERR_SUCCESS;
	return 0;
}

//================================================================
// Master setting
//================================================================
long Cdio_dm_set_master(void *device_ext, PCDIO_DM_SET_MASTER param)
{
	PCDIO_DATA		dio_data;
	unsigned long	ExtSig1;
	unsigned long	ExtSig2;
	unsigned long	ExtSig3;
	unsigned long	MasterHalt;
	unsigned long	SlaveHalt;
	unsigned long	Enable;
	unsigned long	flags;

	//----------------------------------------
	// Cast to structure
	//----------------------------------------
	dio_data	= (PCDIO_DATA)device_ext;
	if (dio_data == NULL || param == NULL) {
		return -1;
	}
	//----------------------------------------
	// Check the supported boards
	//----------------------------------------
	if(dio_data->bios_d.b_info->board_type != CDIOBI_BT_PCI_DM){
		param->ret	= DIO_ERR_SYS_NOT_SUPPORTED;
		return 0;
	}
	//----------------------------------------
	// Get the spinlock
	//----------------------------------------
	spin_lock_irqsave(&dio_data->spinlock_flag, flags);
	//---------------------------------------------
	// Set master configuration to the hardware directly.
	//---------------------------------------------
	//---------------------------------------------
	// Converse ExtSig1
	//---------------------------------------------
	switch(param->ext_sig1) {
	case 0:									ExtSig1 = 0x00000000;	break;
	case DIODM_EXT_START_SOFT_IN:			ExtSig1 = 0x00000007;	break;
	case DIODM_EXT_STOP_SOFT_IN:			ExtSig1 = 0x00000008;	break;
	case DIODM_EXT_CLOCK_IN:				ExtSig1 = 0x00000001;	break;
	case DIODM_EXT_EXT_TRG_IN:				ExtSig1 = 0x00000002;	break;
	case DIODM_EXT_START_EXT_RISE_IN:		ExtSig1 = 0x00000003;	break;
	case DIODM_EXT_START_EXT_FALL_IN:		ExtSig1 = 0x00000004;	break;
	case DIODM_EXT_START_PATTERN_IN:		ExtSig1 = 0x0000000b;	break;
	case DIODM_EXT_STOP_EXT_RISE_IN:		ExtSig1 = 0x00000005;	break;
	case DIODM_EXT_STOP_EXT_FALL_IN:		ExtSig1 = 0x00000006;	break;
	case DIODM_EXT_CLOCK_ERROR_IN:			ExtSig1 = 0x00000009;	break;
	case DIODM_EXT_HANDSHAKE_IN:			ExtSig1 = 0x0000000a;	break;
	case DIODM_EXT_TRNSNUM_IN:				ExtSig1 = 0x0000000c;	break;
	case DIODM_EXT_START_SOFT_OUT:			ExtSig1 = 0x00001007;	break;
	case DIODM_EXT_STOP_SOFT_OUT:			ExtSig1 = 0x00001008;	break;
	case DIODM_EXT_CLOCK_OUT:				ExtSig1 = 0x00001001;	break;
	case DIODM_EXT_EXT_TRG_OUT:				ExtSig1 = 0x00001002;	break;
	case DIODM_EXT_START_EXT_RISE_OUT:		ExtSig1 = 0x00001003;	break;
	case DIODM_EXT_START_EXT_FALL_OUT:		ExtSig1 = 0x00001004;	break;
	case DIODM_EXT_STOP_EXT_RISE_OUT:		ExtSig1 = 0x00001005;	break;
	case DIODM_EXT_STOP_EXT_FALL_OUT:		ExtSig1 = 0x00001006;	break;
	case DIODM_EXT_CLOCK_ERROR_OUT:			ExtSig1 = 0x00001009;	break;
	case DIODM_EXT_HANDSHAKE_OUT:			ExtSig1 = 0x0000100a;	break;
	case DIODM_EXT_TRNSNUM_OUT:				ExtSig1 = 0x0000100c;	break;
	default:		param->ret = DIO_ERR_SYS_SIGNAL;	return 0;	break;
	}
	//---------------------------------------------
	// Converse ExtSig2
	//---------------------------------------------
	switch(param->ext_sig2) {
	case 0:									ExtSig2 = 0x00000000;	break;
	case DIODM_EXT_START_SOFT_IN:			ExtSig2 = 0x00000007;	break;
	case DIODM_EXT_STOP_SOFT_IN:			ExtSig2 = 0x00000008;	break;
	case DIODM_EXT_CLOCK_IN:				ExtSig2 = 0x00000001;	break;
	case DIODM_EXT_EXT_TRG_IN:				ExtSig2 = 0x00000002;	break;
	case DIODM_EXT_START_EXT_RISE_IN:		ExtSig2 = 0x00000003;	break;
	case DIODM_EXT_START_EXT_FALL_IN:		ExtSig2 = 0x00000004;	break;
	case DIODM_EXT_START_PATTERN_IN:		ExtSig2 = 0x0000000b;	break;
	case DIODM_EXT_STOP_EXT_RISE_IN:		ExtSig2 = 0x00000005;	break;
	case DIODM_EXT_STOP_EXT_FALL_IN:		ExtSig2 = 0x00000006;	break;
	case DIODM_EXT_CLOCK_ERROR_IN:			ExtSig2 = 0x00000009;	break;
	case DIODM_EXT_HANDSHAKE_IN:			ExtSig2 = 0x0000000a;	break;
	case DIODM_EXT_TRNSNUM_IN:				ExtSig2 = 0x0000000c;	break;
	case DIODM_EXT_START_SOFT_OUT:			ExtSig2 = 0x00001007;	break;
	case DIODM_EXT_STOP_SOFT_OUT:			ExtSig2 = 0x00001008;	break;
	case DIODM_EXT_CLOCK_OUT:				ExtSig2 = 0x00001001;	break;
	case DIODM_EXT_EXT_TRG_OUT:				ExtSig2 = 0x00001002;	break;
	case DIODM_EXT_START_EXT_RISE_OUT:		ExtSig2 = 0x00001003;	break;
	case DIODM_EXT_START_EXT_FALL_OUT:		ExtSig2 = 0x00001004;	break;
	case DIODM_EXT_STOP_EXT_RISE_OUT:		ExtSig2 = 0x00001005;	break;
	case DIODM_EXT_STOP_EXT_FALL_OUT:		ExtSig2 = 0x00001006;	break;
	case DIODM_EXT_CLOCK_ERROR_OUT:			ExtSig2 = 0x00001009;	break;
	case DIODM_EXT_HANDSHAKE_OUT:			ExtSig2 = 0x0000100a;	break;
	case DIODM_EXT_TRNSNUM_OUT:				ExtSig2 = 0x0000100c;	break;
	default:		param->ret = DIO_ERR_SYS_SIGNAL;	return 0;	break;
	}
	//---------------------------------------------
	// Converse ExtSig3
	//---------------------------------------------
	switch(param->ext_sig3) {
	case 0:									ExtSig3 = 0x00000000;	break;
	case DIODM_EXT_START_SOFT_IN:			ExtSig3 = 0x00000007;	break;
	case DIODM_EXT_STOP_SOFT_IN:			ExtSig3 = 0x00000008;	break;
	case DIODM_EXT_CLOCK_IN:				ExtSig3 = 0x00000001;	break;
	case DIODM_EXT_EXT_TRG_IN:				ExtSig3 = 0x00000002;	break;
	case DIODM_EXT_START_EXT_RISE_IN:		ExtSig3 = 0x00000003;	break;
	case DIODM_EXT_START_EXT_FALL_IN:		ExtSig3 = 0x00000004;	break;
	case DIODM_EXT_START_PATTERN_IN:		ExtSig3 = 0x0000000b;	break;
	case DIODM_EXT_STOP_EXT_RISE_IN:		ExtSig3 = 0x00000005;	break;
	case DIODM_EXT_STOP_EXT_FALL_IN:		ExtSig3 = 0x00000006;	break;
	case DIODM_EXT_CLOCK_ERROR_IN:			ExtSig3 = 0x00000009;	break;
	case DIODM_EXT_HANDSHAKE_IN:			ExtSig3 = 0x0000000a;	break;
	case DIODM_EXT_TRNSNUM_IN:				ExtSig3 = 0x0000000c;	break;
	case DIODM_EXT_START_SOFT_OUT:			ExtSig3 = 0x00001007;	break;
	case DIODM_EXT_STOP_SOFT_OUT:			ExtSig3 = 0x00001008;	break;
	case DIODM_EXT_CLOCK_OUT:				ExtSig3 = 0x00001001;	break;
	case DIODM_EXT_EXT_TRG_OUT:				ExtSig3 = 0x00001002;	break;
	case DIODM_EXT_START_EXT_RISE_OUT:		ExtSig3 = 0x00001003;	break;
	case DIODM_EXT_START_EXT_FALL_OUT:		ExtSig3 = 0x00001004;	break;
	case DIODM_EXT_STOP_EXT_RISE_OUT:		ExtSig3 = 0x00001005;	break;
	case DIODM_EXT_STOP_EXT_FALL_OUT:		ExtSig3 = 0x00001006;	break;
	case DIODM_EXT_CLOCK_ERROR_OUT:			ExtSig3 = 0x00001009;	break;
	case DIODM_EXT_HANDSHAKE_OUT:			ExtSig3 = 0x0000100a;	break;
	case DIODM_EXT_TRNSNUM_OUT:				ExtSig3 = 0x0000100c;	break;
	default:		param->ret = DIO_ERR_SYS_SIGNAL;	return 0;	break;
	}
	//---------------------------------------------
	// Set to the hardware directly.
	//---------------------------------------------
	if (strcmp(dio_data->board_name, "DIO-32DM3-PE") == 0){
		//ExtSig1
		_outmw(MEM_DIO_PORT_ADD + 0x08, 0x01);
		_outmd(MEM_DIO_PORT_ADD + 0x0c, ExtSig1);
		//ExtSig2
		_outmw(MEM_DIO_PORT_ADD + 0x08, 0x02);
		_outmd(MEM_DIO_PORT_ADD + 0x0c, ExtSig2);
		//ExtSig3
		_outmw(MEM_DIO_PORT_ADD + 0x08, 0x03);
		_outmd(MEM_DIO_PORT_ADD + 0x0c, ExtSig3);
		// Enable
		MasterHalt	= param->master_halt & 0x01;
		SlaveHalt	= param->slave_halt & 0x01;
		Enable = 0;
		if (ExtSig1 != 0) 		Enable |= 0x10;
		if (ExtSig2 != 0) 		Enable |= 0x08;
		if (ExtSig3 != 0) 		Enable |= 0x04;
		if (MasterHalt != 0)	Enable |= 0x02;
		if (SlaveHalt != 0)		Enable |= 0x01;
		_outmd(MEM_DIO_PORT_ADD + 0x18, Enable);
		dio_data->bm_board.Master.MasterEnable	= Enable;
		dio_data->bm_board.Master.SlaveEnable	= 0;
	} else {
		//ExtSig1
		BmOutW(dio_data->bm_board.dwAddr + 0x08, 0x01);
		BmOutD(dio_data->bm_board.dwAddr + 0x0c, ExtSig1);
		//ExtSig2
		BmOutW(dio_data->bm_board.dwAddr + 0x08, 0x02);
		BmOutD(dio_data->bm_board.dwAddr + 0x0c, ExtSig2);
		//ExtSig3
		BmOutW(dio_data->bm_board.dwAddr + 0x08, 0x03);
		BmOutD(dio_data->bm_board.dwAddr + 0x0c, ExtSig3);
		// Enable
		MasterHalt	= param->master_halt & 0x01;
		SlaveHalt	= param->slave_halt & 0x01;
		Enable = 0;
		if (ExtSig1 != 0) 		Enable |= 0x10;
		if (ExtSig2 != 0) 		Enable |= 0x08;
		if (ExtSig3 != 0) 		Enable |= 0x04;
		if (MasterHalt != 0)	Enable |= 0x02;
		if (SlaveHalt != 0)		Enable |= 0x01;
		BmOutD(dio_data->bm_board.dwAddr + 0x18, Enable);
	}
	//----------------------------------------
	// Release the spinlock
	//----------------------------------------
	spin_unlock_irqrestore(&dio_data->spinlock_flag, flags);
	//----------------------------------------
	// Set the return value
	//----------------------------------------
	param->ret = DIO_ERR_SUCCESS;
	return 0;
}

//================================================================
// Slave setting
//================================================================
long Cdio_dm_set_slave(void *device_ext, PCDIO_DM_SET_SLAVE param)
{
	PCDIO_DATA		dio_data;
	unsigned long	ExtSig1;
	unsigned long	ExtSig2;
	unsigned long	ExtSig3;
	unsigned long	MasterHalt;
	unsigned long	SlaveHalt;
	unsigned long	Enable;
	unsigned long	flags;

	//----------------------------------------
	// Cast to structure
	//----------------------------------------
	dio_data	= (PCDIO_DATA)device_ext;
	if (dio_data == NULL || param == NULL) {
		return -1;
	}
	//----------------------------------------
	// Check the supported boards
	//----------------------------------------
	if(dio_data->bios_d.b_info->board_type != CDIOBI_BT_PCI_DM){
		param->ret	= DIO_ERR_SYS_NOT_SUPPORTED;
		return 0;
	}
	//----------------------------------------
	// Check the arguments
	//----------------------------------------
	if ((param->ext_sig1 != 0 && param->ext_sig1 != 1) ||
		(param->ext_sig2 != 0 && param->ext_sig2 != 1) ||
		(param->ext_sig3 != 0 && param->ext_sig3 != 1) ||
		(param->master_halt != 1 && param->master_halt != 0) ||
		(param->slave_halt != 1 && param->slave_halt != 0)){
		param->ret	= DIO_ERR_SYS_SIGNAL;
		return 0;
	}
	//----------------------------------------
	// Get the spinlock
	//----------------------------------------
	spin_lock_irqsave(&dio_data->spinlock_flag, flags);
	//---------------------------------------------
	// Set thread configuration to the hardware directly.
	//---------------------------------------------
	//---------------------------------------------
	// Store in local variables
	//---------------------------------------------
	ExtSig1		= param->ext_sig1 & 0x01;
	ExtSig2		= param->ext_sig2 & 0x01;
	ExtSig3		= param->ext_sig3 & 0x01;
	MasterHalt	= param->master_halt & 0x01;
	SlaveHalt	= param->slave_halt & 0x01;
	//---------------------------------------------
	// Set to the hardware directly.
	//---------------------------------------------
	Enable	= 0x80;		//slave
	if (ExtSig1 != 0) 		Enable |= 0x10;
	if (ExtSig2 != 0) 		Enable |= 0x08;
	if (ExtSig3 != 0) 		Enable |= 0x04;
	if (MasterHalt != 0)	Enable |= 0x02;
	if (SlaveHalt != 0)		Enable |= 0x01;
	if (strcmp(dio_data->board_name, "DIO-32DM3-PE") == 0){
		_outmd(MEM_DIO_PORT_ADD + 0x18, Enable);
		dio_data->bm_board.Master.SlaveEnable	= Enable;
		dio_data->bm_board.Master.MasterEnable	= 0;
	} else {
		BmOutD(dio_data->bm_board.dwAddr + 0x18, Enable);
	}
	//----------------------------------------
	// Release the spinlock
	//----------------------------------------
	spin_unlock_irqrestore(&dio_data->spinlock_flag, flags);
	//----------------------------------------
	// Set the return value
	//----------------------------------------
	param->ret	= DIO_ERR_SUCCESS;
	return 0;
}

//================================================================
// Start condition setting
//================================================================
long Cdio_dm_set_start_trg(void *device_ext, PCDIO_DM_SET_START_TRG param)
{
	PCDIO_DATA		dio_data;
	unsigned long	Start;
	unsigned long	flags;

	//----------------------------------------
	// Cast to structure
	//----------------------------------------
	dio_data	= (PCDIO_DATA)device_ext;
	if (dio_data == NULL || param == NULL) {
		return -1;
	}
	//----------------------------------------
	// Check the supported boards
	//----------------------------------------
	if(dio_data->bios_d.b_info->board_type != CDIOBI_BT_PCI_DM){
		param->ret	= DIO_ERR_SYS_NOT_SUPPORTED;
		return 0;
	}
	//---------------------------------------------
	// Check the parameters
	//---------------------------------------------
	if ((param->dir != DIODM_DIR_IN) && 
		(param->dir != DIODM_DIR_OUT)){
		param->ret = DIO_ERR_SYS_DIRECTION;
		return 0;
	}
	//----------------------------------------
	// Get the spinlock
	//----------------------------------------
	spin_lock_irqsave(&dio_data->spinlock_flag, flags);
	//---------------------------------------------
	// Set start trigger to the work.
	//---------------------------------------------
	//---------------------------------------------
	// Converse the start condition
	//---------------------------------------------
	switch(param->start) {
	case DIODM_START_SOFT:		Start = 0x00000004;	break;
	case DIODM_START_EXT_RISE:	Start = 0x00000005;	break;
	case DIODM_START_EXT_FALL:	Start = 0x00000006;	break;
	case DIODM_START_PATTERN:	Start = 0x00000007;	break;
	case DIODM_START_EXTSIG_1:	Start = 0x00000001;	break;
	case DIODM_START_EXTSIG_2:	Start = 0x00000002;	break;
	case DIODM_START_EXTSIG_3:	Start = 0x00000003;	break;
	default:	param->ret = DIO_ERR_SYS_START;	return 0;	break;
	}
	//---------------------------------------------
	// Store the start trigger
	//---------------------------------------------
	if (param->dir == DIODM_DIR_IN) {
		dio_data->bm_board.Sampling.Start	= Start;
	} else {
		dio_data->bm_board.Generating.Start	= Start;
	}
	//----------------------------------------
	// Release the spinlock
	//----------------------------------------
	spin_unlock_irqrestore(&dio_data->spinlock_flag, flags);
	//----------------------------------------
	// Set the return value
	//----------------------------------------
	param->ret = DIO_ERR_SUCCESS;
	return 0;
}

//================================================================
// Pattern matching condition of start condition setting
//================================================================
long Cdio_dm_set_start_ptn(void *device_ext, PCDIO_DM_SET_START_PTN param)
{
	PCDIO_DATA		dio_data;
	unsigned long	flags;

	//----------------------------------------
	// Cast to structure
	//----------------------------------------
	dio_data = (PCDIO_DATA)device_ext;
	if (dio_data == NULL || param == NULL) {
		return -1;
	}
	//----------------------------------------
	// Check the supported boards
	//----------------------------------------
	if(dio_data->bios_d.b_info->board_type != CDIOBI_BT_PCI_DM){
		param->ret = DIO_ERR_SYS_NOT_SUPPORTED;
		return 0;
	}
	//----------------------------------------
	// Get the spinlock
	//----------------------------------------
	spin_lock_irqsave(&dio_data->spinlock_flag, flags);
	//---------------------------------------------
	// Set patten to the work.
	//---------------------------------------------
	//---------------------------------------------
	// Store the patten, the mask
	//---------------------------------------------
	dio_data->bm_board.Sampling.Pattern	= param->ptn;
	dio_data->bm_board.Sampling.Mask	= param->mask;
	//----------------------------------------
	// Release the spinlock
	//----------------------------------------
	spin_unlock_irqrestore(&dio_data->spinlock_flag, flags);
	//----------------------------------------
	// Set the return value
	//----------------------------------------
	param->ret = DIO_ERR_SUCCESS;
	return 0;
}

//================================================================
// Clock condition setting
//================================================================
long Cdio_dm_set_clock_trg(void *device_ext, PCDIO_DM_SET_CLOCK_TRG param)
{
	PCDIO_DATA		dio_data;
	unsigned long	Clock;
	unsigned long	flags;

	//----------------------------------------
	// Cast to structure
	//----------------------------------------
	dio_data = (PCDIO_DATA)device_ext;
	if (dio_data == NULL || param == NULL) {
		return -1;
	}
	//----------------------------------------
	// Check the supported boards
	//----------------------------------------
	if(dio_data->bios_d.b_info->board_type != CDIOBI_BT_PCI_DM){
		param->ret = DIO_ERR_SYS_NOT_SUPPORTED;
		return 0;
	}
	//---------------------------------------------
	// Check the parameters
	//---------------------------------------------
	if ((param->dir != DIODM_DIR_IN) && 
		(param->dir != DIODM_DIR_OUT)){
		param->ret = DIO_ERR_SYS_DIRECTION;
		return 0;
	}
	//----------------------------------------
	// Get the spinlock
	//----------------------------------------
	spin_lock_irqsave(&dio_data->spinlock_flag, flags);
	//---------------------------------------------
	// Set clock trigger to the work.
	//---------------------------------------------
	//---------------------------------------------
	// Converse the clock condition
	//---------------------------------------------
	switch(param->clock) {
	case DIODM_CLK_CLOCK:		Clock = 0x00040000;	break;
	case DIODM_CLK_EXT_TRG:		Clock = 0x00050000;	break;
	case DIODM_CLK_HANDSHAKE:	Clock = 0x00060000;	break;
	case DIODM_CLK_EXTSIG_1:	Clock = 0x00010000;	break;
	case DIODM_CLK_EXTSIG_2:	Clock = 0x00020000;	break;
	case DIODM_CLK_EXTSIG_3:	Clock = 0x00030000;	break;
	default:	param->ret = DIO_ERR_SYS_CLOCK;	return 0;	break;
	}
	//---------------------------------------------
	// Store the clock condition
	//---------------------------------------------
	if (param->dir == DIODM_DIR_IN) {
		dio_data->bm_board.Sampling.Clock	= Clock;
	} else {
		dio_data->bm_board.Generating.Clock	= Clock;
	}
	//----------------------------------------
	// Release the spinlock
	//----------------------------------------
	spin_unlock_irqrestore(&dio_data->spinlock_flag, flags);
	//----------------------------------------
	// Set the return value
	//----------------------------------------
	param->ret = DIO_ERR_SUCCESS;
	return 0;
}

//================================================================
// Internal clock setting
//================================================================
long Cdio_dm_set_internal_clock(void *device_ext, PCDIO_DM_SET_INTERNAL_CLOCK param)
{
	PCDIO_DATA		dio_data;
	unsigned long	SetClk = 0;
	unsigned long	flags;

	//----------------------------------------
	// Cast to structure
	//----------------------------------------
	dio_data = (PCDIO_DATA)device_ext;
	if (dio_data == NULL || param == NULL) {
		return -1;
	}
	//----------------------------------------
	// Check the supported boards
	//----------------------------------------
	if(dio_data->bios_d.b_info->board_type != CDIOBI_BT_PCI_DM){
		param->ret = DIO_ERR_SYS_NOT_SUPPORTED;
		return 0;
	}
	//---------------------------------------------
	// Check the parameters
	//---------------------------------------------
	if ((param->dir != DIODM_DIR_IN) && 
		(param->dir != DIODM_DIR_OUT)){
		param->ret = DIO_ERR_SYS_DIRECTION;
		return 0;
	}
	switch(param->unit) {
	case DIODM_TIM_UNIT_S:
	if (strcmp(dio_data->board_name, "DIO-32DM3-PE") == 0){
			SetClk = param->clock * 40000000 * 5;
			if(param->clock > 21){
				param->ret = DIO_ERR_SYS_CLOCK_VAL;
				return 0;
			}
		} else {
			SetClk = param->clock * 40000000;
			if(param->clock > 107){
				param->ret = DIO_ERR_SYS_CLOCK_VAL;
				return 0;
			}
		}
		break;
	case DIODM_TIM_UNIT_MS:	
		if (strcmp(dio_data->board_name, "DIO-32DM3-PE") == 0){
			SetClk = param->clock * 40000 * 5;
			if(param->clock > 21 * 1000){
				param->ret = DIO_ERR_SYS_CLOCK_VAL;
				return 0;
			}
		} else {
			SetClk = param->clock * 40000;
			if(param->clock > 107 * 1000){
				param->ret = DIO_ERR_SYS_CLOCK_VAL;
				return 0;
			}
		}
		break;
	case DIODM_TIM_UNIT_US:
		if (strcmp(dio_data->board_name, "DIO-32DM3-PE") == 0){
			SetClk = param->clock * 40 * 5;
			if(param->clock > 21 * 1000 * 1000){
				param->ret = DIO_ERR_SYS_CLOCK_VAL;
				return 0;
			}
		} else {
			SetClk = param->clock * 40;
			if(param->clock > 107 * 1000 * 1000){
				param->ret = DIO_ERR_SYS_CLOCK_VAL;
				return 0;
			}
		}
		break;
	case DIODM_TIM_UNIT_NS:
		if (strcmp(dio_data->board_name, "DIO-32DM3-PE") == 0){
			SetClk = param->clock / 5;
			if(param->clock < 20){			// The upper limit check is not performed because it exceeds the maximum value of the type.
				param->ret = DIO_ERR_SYS_CLOCK_VAL;
				return 0;
			}
		} else {
			SetClk = param->clock / 25;
			if(param->clock < 50){			// Don't execute because the upper limit check exceeds the maximum value of the type
				param->ret = DIO_ERR_SYS_CLOCK_VAL;
				return 0;
			}
		}
		break;
	default:
		param->ret = DIO_ERR_SYS_CLOCK_UNIT;
		return 0;
	}
	//----------------------------------------
	// Get the spinlock
	//----------------------------------------
	spin_lock_irqsave(&dio_data->spinlock_flag, flags);
	//---------------------------------------------
	// Set internal clock to the work.
	//---------------------------------------------
	//---------------------------------------------
	// Store the clock
	//---------------------------------------------
	if (param->dir == DIODM_DIR_IN) {
		dio_data->bm_board.Sampling.InternalClock	= SetClk;
	} else {
		dio_data->bm_board.Generating.InternalClock	= SetClk;
	}
	//----------------------------------------
	// Release the spinlock
	//----------------------------------------
	spin_unlock_irqrestore(&dio_data->spinlock_flag, flags);
	//----------------------------------------
	// Set the return value
	//----------------------------------------
	param->ret = DIO_ERR_SUCCESS;
	return 0;
}

//================================================================
// Stop condition setting
//================================================================
long Cdio_dm_set_stop_trg(void *device_ext, PCDIO_DM_SET_STOP_TRG param)
{
	PCDIO_DATA		dio_data;
	unsigned long	Stop;
	unsigned long	flags;

	//----------------------------------------
	// Cast to structure
	//----------------------------------------
	dio_data = (PCDIO_DATA)device_ext;
	if (dio_data == NULL || param == NULL) {
		return -1;
	}
	//----------------------------------------
	// Check the supported boards
	//----------------------------------------
	if(dio_data->bios_d.b_info->board_type != CDIOBI_BT_PCI_DM){
		param->ret = DIO_ERR_SYS_NOT_SUPPORTED;
		return 0;
	}
	//---------------------------------------------
	// Check the parameters
	//---------------------------------------------
	if ((param->dir != DIODM_DIR_IN) && 
		(param->dir != DIODM_DIR_OUT)){
		param->ret = DIO_ERR_SYS_DIRECTION;
		return 0;
	}
	//---------------------------------------------
	// Set stop trigger to the work.
	//---------------------------------------------
	//---------------------------------------------
	// Converse the stop condition
	//---------------------------------------------
	switch(param->stop) {
	case DIODM_STOP_SOFT:		Stop = 0x00000400;	break;
	case DIODM_STOP_EXT_RISE:	Stop = 0x00000500;	break;
	case DIODM_STOP_EXT_FALL:	Stop = 0x00000600;	break;
	case DIODM_STOP_NUM:		Stop = 0x00000700;	break;
	case DIODM_STOP_EXTSIG_1:	Stop = 0x00000100;	break;
	case DIODM_STOP_EXTSIG_2:	Stop = 0x00000200;	break;
	case DIODM_STOP_EXTSIG_3:	Stop = 0x00000300;	break;
	default:					
		param->ret	= DIO_ERR_SYS_STOP;
		return 0;
		break;
	}
	//----------------------------------------
	// Get the spinlock
	//----------------------------------------
	spin_lock_irqsave(&dio_data->spinlock_flag, flags);
	//---------------------------------------------
	// Store the stop condition
	//---------------------------------------------
	if (param->dir == DIODM_DIR_IN) {
		dio_data->bm_board.Sampling.Stop	= Stop;
		dio_data->bm_board.Master.BmInp.ringstartflag	= 0;
		if (strcmp(dio_data->board_name, "DIO-32DM3-PE") == 0){
			//---------------------------------------------
			// In the case of infinite transfer and transfer completed with specified number,
			// set the flag because the description of SGList is different.
			//---------------------------------------------
			if (Stop == 0x700){
				dio_data->bm_board.Master.BmInp.ringstartflag	= 1;
			}
		}
	} else {
		dio_data->bm_board.Generating.Stop	= Stop;
	}
	//----------------------------------------
	// Release the spinlock
	//----------------------------------------
	spin_unlock_irqrestore(&dio_data->spinlock_flag, flags);
	//----------------------------------------
	// Set the return value
	//----------------------------------------
	param->ret = DIO_ERR_SUCCESS;
	return 0;
}

//================================================================
// Stop number setting
//================================================================
long Cdio_dm_set_stop_num(void *device_ext, PCDIO_DM_SET_STOP_NUM param)
{
	PCDIO_DATA		dio_data;
	unsigned long	flags;

	//----------------------------------------
	// Cast to structure
	//----------------------------------------
	dio_data = (PCDIO_DATA)device_ext;
	if (dio_data == NULL || param == NULL) {
		return -1;
	}
	//----------------------------------------
	// Check the supported boards
	//----------------------------------------
	if(dio_data->bios_d.b_info->board_type != CDIOBI_BT_PCI_DM){
		param->ret = DIO_ERR_SYS_NOT_SUPPORTED;
		return 0;
	}
	//---------------------------------------------
	// Check the parameters
	//---------------------------------------------
	if ((param->dir != DIODM_DIR_IN) && 
		(param->dir != DIODM_DIR_OUT)){
		param->ret = DIO_ERR_SYS_DIRECTION;
		return 0;
	}
	if (strcmp(dio_data->board_name, "DIO-32DM3-PE") == 0){
		if (param->stop_num > 0x20000000 || param->stop_num == 0 ){
			param->ret = DIO_ERR_SYS_STOP_NUM;
			return 0;
		}
	} else {
		if (param->stop_num > 0xffffff || param->stop_num == 0 ){
			param->ret = DIO_ERR_SYS_STOP_NUM;
			return 0;
		}
	}
	//----------------------------------------
	// Get the spinlock
	//----------------------------------------
	spin_lock_irqsave(&dio_data->spinlock_flag, flags);
	//---------------------------------------------
	// Set stop number to the work
	//---------------------------------------------
	//---------------------------------------------
	// Store the clock
	//---------------------------------------------
	if (param->dir == DIODM_DIR_IN) {
		dio_data->bm_board.Sampling.StopNum		= param->stop_num;
	}else{
		dio_data->bm_board.Generating.StopNum	= param->stop_num;
	}
	//----------------------------------------
	// Release the spinlock
	//----------------------------------------
	spin_unlock_irqrestore(&dio_data->spinlock_flag, flags);
	//----------------------------------------
	// Set the return value
	//----------------------------------------
	param->ret = DIO_ERR_SUCCESS;
	return 0;
}

//================================================================
//  FIFO reset
//================================================================
long Cdio_dm_reset(void *device_ext, PCDIO_DM_RESET param)
{
	PCDIO_DATA		dio_data;
	long			lret;
	unsigned long	flags;

	//----------------------------------------
	// Cast to structure
	//----------------------------------------
	dio_data = (PCDIO_DATA)device_ext;
	if (dio_data == NULL || param == NULL) {
		return -1;
	}
	//----------------------------------------
	// Check the supported boards
	//----------------------------------------
	if(dio_data->bios_d.b_info->board_type != CDIOBI_BT_PCI_DM){
		param->ret = DIO_ERR_SYS_NOT_SUPPORTED;
		return 0;
	}
	//---------------------------------------------
	// Check the parameters
	//---------------------------------------------
	if ((param->reset & ~(DIODM_RESET_FIFO_IN | DIODM_RESET_FIFO_OUT))){
		param->ret = DIO_ERR_SYS_RESET;
		return 0;
	}
	//----------------------------------------
	// Get the spinlock
	//----------------------------------------
	spin_lock_irqsave(&dio_data->spinlock_flag, flags);
	//---------------------------------------------
	// To reset.
	//---------------------------------------------
	if (strcmp(dio_data->board_name, "DIO-32DM3-PE") == 0){
		if ((param->reset & DIODM_RESET_FIFO_IN) == DIODM_RESET_FIFO_IN){
			dio_data->bm_board.KeepStart &= ~BM_DIR_IN;
		}
		lret = ApiMemBmReset(&dio_data->bm_board.Master, param->reset);
	} else {
		lret = ApiBmReset(&dio_data->bm_board.Master, param->reset);
	}
	//----------------------------------------
	// Release the spinlock
	//----------------------------------------
	spin_unlock_irqrestore(&dio_data->spinlock_flag, flags);
	if (lret == BM_ERROR_PARAM) {
		param->ret = DIO_ERR_DM_PARAM;
		return 0;
	}
	//----------------------------------------
	// Set the return value
	//----------------------------------------
	param->ret = DIO_ERR_SUCCESS;
	return 0;
}

//================================================================
// Buffer setting
//================================================================
long Cdio_dm_set_buf(void *device_ext, PCDIO_DM_SET_BUF param)
{
	PCDIO_DATA		dio_data;
	long			lret;
//	unsigned long	flags;

	//----------------------------------------
	// Cast to structure
	//----------------------------------------
	dio_data = (PCDIO_DATA)device_ext;
	if (dio_data == NULL || param == NULL) {
		return -1;
	}
	//----------------------------------------
	// Check the supported boards
	//----------------------------------------
	if(dio_data->bios_d.b_info->board_type != CDIOBI_BT_PCI_DM){
		param->ret = DIO_ERR_SYS_NOT_SUPPORTED;
		return 0;
	}
	//-------------------------------------------
	// Check the parameters
	//-------------------------------------------
	if ((param->dir != DIODM_DIR_IN) && 
		(param->dir != DIODM_DIR_OUT)){
		param->ret = DIO_ERR_SYS_DIRECTION;
		return 0;
	}
	if (strcmp(dio_data->board_name, "DIO-32DM3-PE") == 0){
		if (param->len > (0x80000000 / 4)) {
			param->ret = DIO_ERR_SYS_LEN;
			return 0;
		}
	} else {
		if (param->len > (0x4000000 / 4)) {
			param->ret = DIO_ERR_SYS_LEN;
			return 0;
		}
	}
	if (param->is_ring != DIODM_WRITE_ONCE && param->is_ring != DIODM_WRITE_RING) {
		param->ret = DIO_ERR_SYS_RING;
		return 0;
	}
	//----------------------------------------
	// Get the spinlock
	//----------------------------------------
//	spin_lock_irqsave(&dio_data->spinlock_flag, flags);
	//---------------------------------------------
	// Set buffer to the S/G list.
	//---------------------------------------------
	//---------------------------------------------
	// Set the buffer.
	//---------------------------------------------
	if (strcmp(dio_data->board_name, "DIO-32DM3-PE") == 0){
		lret = ApiMemBmSetBuffer(&dio_data->bm_board.Master,
								param->dir,
								param->buf,
								param->len,
								param->is_ring);
	} else {
		lret = ApiBmSetBuffer(&dio_data->bm_board.Master,
							param->dir,
							param->buf,
							param->len,
							param->is_ring);
	}
	//----------------------------------------
	// Release the spinlock
	//----------------------------------------
//	spin_unlock_irqrestore(&dio_data->spinlock_flag, flags);
	switch(lret) {
	case BM_ERROR_SUCCESS:
		param->ret = DIO_ERR_SUCCESS;
		break;
	case BM_ERROR_PARAM:
		param->ret = DIO_ERR_DM_PARAM;
		return 0;
		break;
	case BM_ERROR_MEM:
		param->ret = DIO_ERR_SYS_MEMORY;
		return 0;
		break;
	case BM_ERROR_BUFF:
		param->ret = DIO_ERR_DM_BUFFER;
		return 0;
		break;
	case BM_ERROR_LOCK_MEM:
		param->ret = DIO_ERR_DM_LOCK_MEMORY;
		return 0;
		break;
	case BM_ERROR_SEQUENCE:
		param->ret = DIO_ERR_DM_SEQUENCE;
		return 0;
		break;
	default:
		param->ret = lret;
		return 0;
		break;
	}
	return 0;
}
//================================================================
// Sampling/Generating start
//================================================================
long Cdio_dm_start(void *device_ext, PCDIO_DM_START param)
{
	PCDIO_DATA		dio_data;
	unsigned long	Condition;
	long			lret;
	unsigned long	flags;
	unsigned long	dwlentest;
	unsigned long	dwlentest2;
	unsigned long	dwlentest3;
	unsigned long	dwlentest4;
	unsigned long	loop = 0;
	unsigned long	SmallLen;
	int				sampling_stop_flag, generating_stop_flag;

	//----------------------------------------
	// Cast to structure
	//----------------------------------------
	dio_data = (PCDIO_DATA)device_ext;
	if (dio_data == NULL || param == NULL) {
		return -1;
	}
	//----------------------------------------
	// Check the supported boards
	//----------------------------------------
	if(dio_data->bios_d.b_info->board_type != CDIOBI_BT_PCI_DM){
		param->ret = DIO_ERR_SYS_NOT_SUPPORTED;
		return 0;
	}
	dio_data->bm_board.dwdir	|= param->dir;
	//----------------------------------------
	// Get the spinlock
	//----------------------------------------
	spin_lock_irqsave(&dio_data->spinlock_flag, flags);
	//---------------------------------------------
	// Set the sampling condition, 
	// start BusMaster and PIO in the order.
	//---------------------------------------------
	//---------------------------------------------
	// Set the sampling condition
	//---------------------------------------------
	if (strcmp(dio_data->board_name, "DIO-32DM3-PE") == 0){
		//---------------------------------------------
		// Sampling starts the PIO after setting the sampling conditions.
		// Generating starts the DMA transfer after setting the sampling conditions.
		//---------------------------------------------
		sampling_stop_flag		= dio_data->bm_board.Master.BmInp.dmastopflag;
		generating_stop_flag	= dio_data->bm_board.Master.BmOut.dmastopflag;
		//---------------------------------------------
		// Sampling Condition Setting
		//---------------------------------------------
		if ((param->dir & DIODM_DIR_IN) && (sampling_stop_flag == 1)) {
			_outmw(BM_MEM_Reset, 0x2);
			// Condition
			Condition = dio_data->bm_board.Sampling.Start | dio_data->bm_board.Sampling.Clock | dio_data->bm_board.Sampling.Stop;
			_outmw(MEM_DIO_PORT_ADD + 0x08, 0x04);
			_outmd(MEM_DIO_PORT_ADD + 0x0c, Condition);
			// Pattern
			_outmw(MEM_DIO_PORT_ADD + 0x08, 0x06);
			_outmd(MEM_DIO_PORT_ADD + 0x0c, dio_data->bm_board.Sampling.Mask);
			_outmw(MEM_DIO_PORT_ADD + 0x08, 0x07);
			_outmd(MEM_DIO_PORT_ADD + 0x0c, dio_data->bm_board.Sampling.Pattern);
			// Internal Clock
			_outmw(MEM_DIO_PORT_ADD + 0x08, 0x05);
			_outmd(MEM_DIO_PORT_ADD + 0x0c, dio_data->bm_board.Sampling.InternalClock);
			// Stop Number
			_outmw(MEM_DIO_PORT_ADD + 0x08, 0x08);
			_outmd(MEM_DIO_PORT_ADD + 0x0c, dio_data->bm_board.Sampling.StopNum);
			// Open the mask (input) of sampling stop
			dio_data->bm_board.PioMask &= 0xfffffffd;
			dio_data->bm_board.SavePioMask = dio_data->bm_board.PioMask;
		}
		//---------------------------------------------
		// Generating Condition Setting
		//---------------------------------------------
		if ((param->dir & DIODM_DIR_OUT) && (generating_stop_flag == 1)) {
			_outmw(BM_MEM_Reset, 0x4);
			// Condition
			Condition = dio_data->bm_board.Generating.Start | dio_data->bm_board.Generating.Clock | dio_data->bm_board.Generating.Stop;
			_outmw(MEM_DIO_PORT_ADD + 0x08, 0x09);
			_outmd(MEM_DIO_PORT_ADD + 0x0c, Condition);
			// Internal Clock
			_outmw(MEM_DIO_PORT_ADD + 0x08, 0x0a);
			_outmd(MEM_DIO_PORT_ADD + 0x0c, dio_data->bm_board.Generating.InternalClock);
			// Stop Number
			_outmw(MEM_DIO_PORT_ADD + 0x08, 0x0b);
			_outmd(MEM_DIO_PORT_ADD + 0x0c, dio_data->bm_board.Generating.StopNum);
			// Open the mask (output) of generating stop
			dio_data->bm_board.PioMask &= 0xfffdffff;
			dio_data->bm_board.SavePioMask = dio_data->bm_board.PioMask;
		}
		_outmd(MEM_DIO_PORT_ADD + 0x10, dio_data->bm_board.PioMask);
		dio_data->bm_board.PioSence = 0x00;
		dwlentest	= 0;
		dwlentest2	= 0;
		dwlentest3	= 0;
		dwlentest4	= 0;
		if ((param->dir & DIODM_DIR_IN) && (sampling_stop_flag == 1)) {
			for (loop=0; loop<dio_data->bm_board.Master.BmInp.SgList.nr_pages; loop++) {
				dwlentest	+=  dio_data->bm_board.Master.BmInp.SgList.List[loop].length;
			}
			dio_data->bm_board.Master.BmInp.sglen	= dwlentest / 4;
			dwlentest	= 0;
			dio_data->bm_board.Master.overlistsizeflag	= 0;
			//---------------------------------------------
			// If the stop number is less than the buffer, correct SGList
			//---------------------------------------------
			if ((dio_data->bm_board.Master.BmInp.SgList.dwIsRing == 1) && (dio_data->bm_board.Master.BmInp.sglen < dio_data->bm_board.Sampling.StopNum)){
				dio_data->bm_board.Master.BmInp.ringstartflag	= 0;
			} else if ((dio_data->bm_board.Master.BmInp.SgList.dwIsRing == 1) && dio_data->bm_board.Master.BmInp.ringstartflag == 0){
				dio_data->bm_board.Master.BmInp.ringstartflag	= 0;
			} else if ((dio_data->bm_board.Master.BmInp.SgList.dwIsRing == 0) && ((dio_data->bm_board.Sampling.Stop & 0x700) != 0x700)){
				dio_data->bm_board.Master.BmInp.ringstartflag	= 0;
			} else {
				if (dio_data->bm_board.Master.BmInp.SgList.nr_pages <= 150){
					for (loop = 0; loop < dio_data->bm_board.Master.BmInp.SgList.nr_pages; loop++) {
						dwlentest4	=  _inmd(dio_data->bm_board.dwAddrBM_mem + 0x06000000 + (0x40 * (loop + 1)) + 0x8);
						dwlentest	+=  dwlentest4;
						if (((dio_data->bm_board.Sampling.StopNum * 4) <= dwlentest) && (loop != (dio_data->bm_board.Master.BmInp.SgList.nr_pages - 1))){
							if (loop == 0) {
								dwlentest3	= (dio_data->bm_board.Sampling.StopNum * 4);
							} else if (loop > 0) {
								dwlentest3	= (dio_data->bm_board.Sampling.StopNum * 4) - dwlentest2;
							}
							MemBmSetSGAddressRePlace(&dio_data->bm_board.Master, DIODM_DIR_IN, loop, dwlentest3);
							break;
						} else if ((dio_data->bm_board.Sampling.StopNum * 4) < dwlentest){
							if (loop == 0) {
								dwlentest3	= (dio_data->bm_board.Sampling.StopNum * 4);
							} else if (loop > 0) {
								dwlentest3	= (dio_data->bm_board.Sampling.StopNum * 4) - dwlentest2;
							}
							MemBmSetSGAddressRePlace(&dio_data->bm_board.Master, DIODM_DIR_IN, loop, dwlentest3);
							break;
						}
						dwlentest2	+=  _inmd(dio_data->bm_board.dwAddrBM_mem + 0x06000000 + (0x40 * (loop + 1)) + 0x8);
					}
				} else {
					for (loop = 0; loop < 150; loop++) {
						dwlentest4	=  _inmd(dio_data->bm_board.dwAddrBM_mem + 0x06000000 + (0x40 * (loop + 1)) + 0x8);
						dwlentest	+=  dwlentest4;
						if (((dio_data->bm_board.Sampling.StopNum * 4) <= dwlentest) && (loop != 149)){
							if (loop == 0) {
								dwlentest3	= (dio_data->bm_board.Sampling.StopNum * 4);
							} else if (loop > 0) {
								dwlentest3	= (dio_data->bm_board.Sampling.StopNum * 4) - dwlentest2;
							}
							MemBmSetSGAddressRePlace(&dio_data->bm_board.Master, DIODM_DIR_IN, loop, dwlentest3);
							dio_data->bm_board.Master.overlistsizeflag	= 1;
							break;
						} else if ((dio_data->bm_board.Sampling.StopNum * 4) < dwlentest){
							if (loop == 0) {
								dwlentest3	= (dio_data->bm_board.Sampling.StopNum * 4);
							} else if (loop > 0) {
								dwlentest3	= (dio_data->bm_board.Sampling.StopNum * 4) - dwlentest2;
							}
							MemBmSetSGAddressRePlace(&dio_data->bm_board.Master, DIODM_DIR_IN, loop, dwlentest3);
							dio_data->bm_board.Master.overlistsizeflag	= 1;
							break;
						}
						dwlentest2	+=  _inmd(dio_data->bm_board.dwAddrBM_mem + 0x06000000 + (0x40 * (loop + 1)) + 0x8);
					}
				}
			} 
			//---------------------------------------------
			// Clear and set variables
			//---------------------------------------------
			dio_data->bm_board.Master.BmInp.endsmp			= 0;
			dio_data->bm_board.Master.BmInp.end_ev_flag		= 0;
			dio_data->bm_board.Master.BmInp.outflag			= 0;
			dio_data->bm_board.Master.BmInp.cntflag			= 0;
			dio_data->bm_board.Master.BmInp.end_ev_save		= 0;
			dio_data->bm_board.Master.BmInp.sgoverflag		= 0;
			dio_data->bm_board.Master.BmInp.ringresetflag	= 0;
			dio_data->bm_board.Master.BmInp.oddtransferflag	= 0;
			dio_data->bm_board.Master.BmInp.oddbufflag		= 0;
			dio_data->bm_board.Master.BmInp.listoverflag	= 0;
			dio_data->bm_board.Master.BmInp.TimeOutFlag		= 0;
			dio_data->bm_board.Master.BmInp.resetcountFlag	= 0;
			dio_data->bm_board.KeepStart					|= DIODM_DIR_IN;
			dio_data->bm_board.SavePioSence					&= 0x10000;
			dwlentest										= 0;
			//---------------------------------------------
			// FIFO REACH IN Size setting
			//---------------------------------------------
			if (dio_data->bm_board.Master.BmInp.SgList.dwIsRing == 1){
				if (dio_data->bm_board.Master.BmInp.sglen < 0x800){
					SmallLen										= dio_data->bm_board.Master.BmInp.sglen;
					dio_data->bm_board.Master.BmInp.fiforeachnum	= SmallLen;
					_outmw(MEM_DIO_PORT_ADD + 0x28, SmallLen);
				} else {
					dio_data->bm_board.Master.BmInp.fiforeachnum	= 0x800;
					_outmw(MEM_DIO_PORT_ADD + 0x28, 0x800);
				}
			} else {
				if ((dio_data->bm_board.Sampling.StopNum < 0x800) && ((dio_data->bm_board.Sampling.Stop & 0x700) == 0x700)){
					if (dio_data->bm_board.Master.BmInp.sglen < dio_data->bm_board.Sampling.StopNum){
						dio_data->bm_board.Master.BmInp.fiforeachnum	= dio_data->bm_board.Master.BmInp.sglen;
						_outmw(MEM_DIO_PORT_ADD + 0x28, dio_data->bm_board.Master.BmInp.sglen);
					} else {
						dio_data->bm_board.Master.BmInp.fiforeachnum	= dio_data->bm_board.Sampling.StopNum;
						_outmw(MEM_DIO_PORT_ADD + 0x28, dio_data->bm_board.Sampling.StopNum);
					}
				} else if (dio_data->bm_board.Master.BmInp.sglen < 0x800){
					dio_data->bm_board.Master.BmInp.fiforeachnum		= dio_data->bm_board.Master.BmInp.sglen;
					_outmw(MEM_DIO_PORT_ADD + 0x28, dio_data->bm_board.Master.BmInp.sglen);
				} else {
					dio_data->bm_board.Master.BmInp.fiforeachnum		= 0x800;
					_outmw(MEM_DIO_PORT_ADD + 0x28, 0x800);
				}
			}
		}
		if ((param->dir & DIODM_DIR_OUT) && (generating_stop_flag == 1)) {
			//---------------------------------------------
			// Clear and set variables
			//---------------------------------------------
			dio_data->bm_board.Master.BmOut.end_ev_save		= 0;
			dio_data->bm_board.Master.BmOut.endsmp			= 0;
			dio_data->bm_board.Master.BmOut.outflag			= 1;
			dio_data->bm_board.Master.BmOut.TimeOutFlag		= 0;
			dio_data->bm_board.Master.BmOut.resetcountFlag	= 1;
			dio_data->bm_board.SavePioSence					&= 0x1;
			dwlentest										= 0;
			//---------------------------------------------
			// Acquisition of the set buffer size
			//---------------------------------------------
			for (loop=0; loop<dio_data->bm_board.Master.BmOut.SgList.nr_pages; loop++) {
				dwlentest	+=  dio_data->bm_board.Master.BmOut.SgList.List[loop].length;
			}
			dio_data->bm_board.Master.BmOut.sglen	= dwlentest / 4;
			//---------------------------------------------
			// FIFO REACH OUT Size setting
			//---------------------------------------------
			if (dio_data->bm_board.Master.BmOut.SgList.dwIsRing == 1){
				if((0x1000 / dio_data->bm_board.Master.BmOut.sglen) > 150){
					SmallLen	= dio_data->bm_board.Master.BmOut.sglen * 75;
					_outmw(MEM_DIO_PORT_ADD + 0x38, SmallLen);
				} else {
					_outmw(MEM_DIO_PORT_ADD + 0x38, 0x800);
				}
			} else {
				_outmw(MEM_DIO_PORT_ADD + 0x38, 0x800);
			}
		}
		//---------------------------------------------
		// Start the BusMaster
		//---------------------------------------------
		lret = ApiMemBmStart(&dio_data->bm_board.Master, param->dir);
		switch(lret) {
		case BM_ERROR_SUCCESS:
			param->ret = DIO_ERR_SUCCESS;
			break;
		case BM_ERROR_PARAM:
			param->ret = DIO_ERR_DM_PARAM;
			//------------------------------------
			// Release the spinlock
			//------------------------------------
			spin_unlock_irqrestore(&dio_data->spinlock_flag, flags);
			return 0;
			break;
		case BM_ERROR_SEQUENCE:
			param->ret = DIO_ERR_DM_SEQUENCE;
			//------------------------------------
			// Release the spinlock
			//------------------------------------
			spin_unlock_irqrestore(&dio_data->spinlock_flag, flags);
			return 0;
			break;
		default:
			param->ret = lret;
			//------------------------------------
			// Release the spinlock
			//------------------------------------
			spin_unlock_irqrestore(&dio_data->spinlock_flag, flags);
			return 0;
			break;
		}
		if (param->dir & DIODM_DIR_OUT) {
			//---------------------------------------------	
			// If WaitTimeWhenOutput is not 0
			//---------------------------------------------
			if (dio_data->bm_board.WaitTimeWhenOutput != 0) {
				// Release the spinlock
				//----------------------------------------
				spin_unlock_irqrestore(&dio_data->spinlock_flag, flags);
				//---------------------------------------------
				// Wait for data to accumulate in the FIFO
				//---------------------------------------------
				Ccom_sleep_on_timeout(dio_data->bm_board.WaitTimeWhenOutput);
				//----------------------------------------
				// Get the spinlock
				//----------------------------------------
				spin_lock_irqsave(&dio_data->spinlock_flag, flags);
			}
		}
		if ((param->dir & DIODM_DIR_IN) && (sampling_stop_flag == 1)) {
			//---------------------------------------------
			// PIO start
			//---------------------------------------------
			_outmw(MEM_DIO_PORT_ADD + 0x06, dio_data->bm_board.KeepStart);
		}
	} else {
		if (param->dir & DIODM_DIR_IN) {
			// Condition
			Condition = dio_data->bm_board.Sampling.Start | dio_data->bm_board.Sampling.Clock | dio_data->bm_board.Sampling.Stop;
			BmOutW(dio_data->bm_board.dwAddr + 0x08, 0x04);
			BmOutD(dio_data->bm_board.dwAddr + 0x0c, Condition);
			// Pattern
			BmOutW(dio_data->bm_board.dwAddr + 0x08, 0x06);
			BmOutD(dio_data->bm_board.dwAddr + 0x0c, dio_data->bm_board.Sampling.Mask);
			BmOutW(dio_data->bm_board.dwAddr + 0x08, 0x07);
			BmOutD(dio_data->bm_board.dwAddr + 0x0c, dio_data->bm_board.Sampling.Pattern);
			// Internal clock
			BmOutW(dio_data->bm_board.dwAddr + 0x08, 0x05);
			BmOutD(dio_data->bm_board.dwAddr + 0x0c, dio_data->bm_board.Sampling.InternalClock);
			// Stop number
			BmOutW(dio_data->bm_board.dwAddr + 0x08, 0x08);
			BmOutD(dio_data->bm_board.dwAddr + 0x0c, dio_data->bm_board.Sampling.StopNum);
			// Open the mask (input) of sampling stop
			dio_data->bm_board.PioMask &= 0xfffffffd;
		}
		//---------------------------------------------
		// Set the generating condition
		//---------------------------------------------
		if (param->dir & DIODM_DIR_OUT) {
			// Condition
			Condition = dio_data->bm_board.Generating.Start | dio_data->bm_board.Generating.Clock | dio_data->bm_board.Generating.Stop;
			BmOutW(dio_data->bm_board.dwAddr + 0x08, 0x09);
			BmOutD(dio_data->bm_board.dwAddr + 0x0c, Condition);
			// Internal clock
			BmOutW(dio_data->bm_board.dwAddr + 0x08, 0x0a);
			BmOutD(dio_data->bm_board.dwAddr + 0x0c, dio_data->bm_board.Generating.InternalClock);
			// Stop number
			BmOutW(dio_data->bm_board.dwAddr + 0x08, 0x0b);
			BmOutD(dio_data->bm_board.dwAddr + 0x0c, dio_data->bm_board.Generating.StopNum);
			// Open the mask (input) of generating stop
			dio_data->bm_board.PioMask &= 0xfffdffff;
		}
		BmOutD(dio_data->bm_board.dwAddr + 0x10, dio_data->bm_board.PioMask);
		//---------------------------------------------
		// Start the BusMaster
		//---------------------------------------------
		dio_data->bm_board.PioSence = 0x00;
		lret = ApiBmStart(&dio_data->bm_board.Master, param->dir);
		switch(lret) {
		case BM_ERROR_SUCCESS:
			param->ret = DIO_ERR_SUCCESS;
			break;
		case BM_ERROR_PARAM:
			param->ret = DIO_ERR_DM_PARAM;
			//----------------------------------------
			// Release the spinlock
			//----------------------------------------
			spin_unlock_irqrestore(&dio_data->spinlock_flag, flags);
			return 0;
			break;
		case BM_ERROR_SEQUENCE:
			param->ret = DIO_ERR_DM_SEQUENCE;
			//----------------------------------------
			// Release the spinlock
			//----------------------------------------
			spin_unlock_irqrestore(&dio_data->spinlock_flag, flags);
			return 0;
			break;
		default:
			param->ret = lret;
			//----------------------------------------
			// Release the spinlock
			//----------------------------------------
			spin_unlock_irqrestore(&dio_data->spinlock_flag, flags);
			return 0;
			break;
		}
		//---------------------------------------------
		// Output
		//---------------------------------------------
		if (param->dir & DIODM_DIR_OUT) {
			//---------------------------------------------
			// If the WaitTimeWhenOutput is not 0 in the setting file
			//---------------------------------------------
			if (dio_data->bm_board.WaitTimeWhenOutput != 0) {
				//----------------------------------------
				// Release the spinlock
				//----------------------------------------
				spin_unlock_irqrestore(&dio_data->spinlock_flag, flags);
				//---------------------------------------------
				// Wait for data to accumulate in the FIFO
				//---------------------------------------------
				Ccom_sleep_on_timeout(dio_data->bm_board.WaitTimeWhenOutput);
				//----------------------------------------
				// Get the spinlock
				//----------------------------------------
				spin_lock_irqsave(&dio_data->spinlock_flag, flags);
			}
		}
		//---------------------------------------------
		// Start the PIO
		//---------------------------------------------
		dio_data->bm_board.KeepStart |= param->dir;
		BmOutW(dio_data->bm_board.dwAddr + 0x06, dio_data->bm_board.KeepStart);
	}
	//----------------------------------------
	// Release the spinlock
	//----------------------------------------
	spin_unlock_irqrestore(&dio_data->spinlock_flag, flags);
	param->ret = DIO_ERR_SUCCESS;
	return 0;
}

//================================================================
// Interrupt synchronization routine structure
//================================================================
typedef struct {
	PCDIO_DATA	dio_data;
} GET_DM_INT_STATUS_SUB, *PGET_DM_INT_STATUS_SUB;
// Interrupt synchronization subroutine

//////////////////////////////////////////////////////////////////////////////////////////
///	@brief	Get bus master interrupt status (subroutine)
///	@param	data		Interrupt synchronization routine structure
///	@return	Always TRUE
///	@note	Call with interrupt synchronization by Cdio_dm_get_int_count()
//////////////////////////////////////////////////////////////////////////////////////////
static int Cdio_dm_get_int_status_sub(void *data)
{
	PGET_DM_INT_STATUS_SUB	param;
	PCDIO_DATA				dio_data;
	unsigned long			IntSence;

	//----------------------------------------
	// Cast to structure
	//----------------------------------------
	param		= (PGET_DM_INT_STATUS_SUB)data;
	dio_data	= param->dio_data;
	//---------------------------------------------
	// Get interrupt status
	//---------------------------------------------
	if (strcmp(dio_data->board_name, "DIO-32DM3-PE") == 0){
		IntSence	= _inmd(MEM_DIO_PORT_ADD + 0x14);
	} else {
		BmInpD((dio_data->bm_board.dwAddr+0x14), IntSence);
	}
	dio_data->bm_board.PioSence	|= IntSence;
	return 1;
}

//================================================================
// Sampling/Generating stop
//================================================================
long Cdio_dm_stop(void *device_ext, PCDIO_DM_STOP param)
{
	PCDIO_DATA		dio_data;
	long			lret;
	unsigned long	flags;
	GET_DM_INT_STATUS_SUB	sub_data;

	//----------------------------------------
	// Cast to structure
	//----------------------------------------
	dio_data = (PCDIO_DATA)device_ext;
	if (dio_data == NULL || param == NULL) {
		return -1;
	}
	//----------------------------------------
	// Check the supported boards
	//----------------------------------------
	if(dio_data->bios_d.b_info->board_type != CDIOBI_BT_PCI_DM){
		param->ret = DIO_ERR_SYS_NOT_SUPPORTED;
		return 0;
	}
	//----------------------------------------
	// Get the spinlock
	//----------------------------------------
	spin_lock_irqsave(&dio_data->spinlock_flag, flags);
	// Set parameter
	sub_data.dio_data	= dio_data;
	// Execute interrupt synchronous subroutine
	Cdio_dm_get_int_status_sub((void *)&sub_data);
	//---------------------------------------------
	// Stop the PIO
	//---------------------------------------------
	if (strcmp(dio_data->board_name, "DIO-32DM3-PE") == 0){
		dio_data->bm_board.KeepStart &= ~param->dir;
		_outmw(MEM_DIO_PORT_ADD + 0x06, dio_data->bm_board.KeepStart);
		//---------------------------------------------
		// When stopping output and bus master transfer has not been started
		//---------------------------------------------
		if(((param->dir & DIODM_DIR_OUT) == DIODM_DIR_OUT) && !((dio_data->bm_board.PioSence >> 16) & 0x0001)){
			//-----------------------------------------
			// Close the bus master mask and open the general-purpose interrupt mask
			//-----------------------------------------
			dio_data->bm_board.PioMask |= 0xffff0000;
			dio_data->bm_board.PioMask &= 0xffff0fff | (((unsigned short)dio_data->bios_d.int_mask[0] << 12) & 0x0000f000);
			_outmd(MEM_DIO_PORT_ADD + 0x10, dio_data->bm_board.PioMask);
			dio_data->bm_board.SavePioMask				= dio_data->bm_board.PioMask;
			dio_data->bm_board.Master.BmOut.dmastopflag	= 1;
		}
		//---------------------------------------------
		// Stop the BusMaster
		//---------------------------------------------
		lret = ApiMemBmStop(&dio_data->bm_board.Master, param->dir);
	} else {
		dio_data->bm_board.KeepStart &= ~param->dir;
		BmOutW(dio_data->bm_board.dwAddr + 0x06, dio_data->bm_board.KeepStart);
		//---------------------------------------------
		// Stop the BusMaster
		//---------------------------------------------
		lret = ApiBmStop(&dio_data->bm_board.Master, param->dir);
	}
	//----------------------------------------
	// Release the spinlock
	//----------------------------------------
	spin_unlock_irqrestore(&dio_data->spinlock_flag, flags);
	if (strcmp(dio_data->board_name, "DIO-32DM3-PE") == 0){
		if(((param->dir & DIODM_DIR_OUT) == DIODM_DIR_OUT) && !((dio_data->bm_board.PioSence >> 16) & 0x0001)){
			ApiBmSetStopEvent(&dio_data->bm_board.Master, BM_DIR_OUT);
		}
	} else {
		if((param->dir == DIODM_DIR_IN) && !((dio_data->bm_board.PioSence) & 0x0001)){
			ApiBmSetStopEvent(&dio_data->bm_board.Master, BM_DIR_IN);
		}
		if((param->dir == DIODM_DIR_OUT) && !((dio_data->bm_board.PioSence >> 16) & 0x0001)){
			ApiBmSetStopEvent(&dio_data->bm_board.Master, BM_DIR_OUT);
		}
	}
	if (lret == BM_ERROR_PARAM) {
		param->ret = DIO_ERR_DM_PARAM;
		return 0;
	}
	param->ret = DIO_ERR_SUCCESS;
	return 0;
}

//================================================================
// Retrieve the transfer status
//================================================================
long Cdio_dm_get_status(void *device_ext, PCDIO_DM_GET_STATUS param)
{
	PCDIO_DATA		dio_data;
	long			lret;
	long			Start;
	unsigned long	PioStatus;
	unsigned long	BmStatus;
	unsigned long	dwHalt;
	unsigned long	flags;
	int				TimeOut = 0;

	//----------------------------------------
	// Cast to structure
	//----------------------------------------
	dio_data = (PCDIO_DATA)device_ext;
	if (dio_data == NULL || param == NULL) {
		return -1;
	}
	//----------------------------------------
	// Check the supported boards
	//----------------------------------------
	if(dio_data->bios_d.b_info->board_type != CDIOBI_BT_PCI_DM){
		param->ret = DIO_ERR_SYS_NOT_SUPPORTED;
		return 0;
	}
	//-------------------------------------------
	// Check the parameters
	//-------------------------------------------
	if ((param->dir != DIODM_DIR_IN) && 
		(param->dir != DIODM_DIR_OUT)){
		param->ret = DIO_ERR_SYS_DIRECTION;
		return 0;
	}
	//----------------------------------------
	// Get the spinlock
	//----------------------------------------
	spin_lock_irqsave(&dio_data->spinlock_flag, flags);
	//---------------------------------------------
	// Get the status, the error.
	//---------------------------------------------
	//---------------------------------------------
	// Get the status.
	//---------------------------------------------
	if (strcmp(dio_data->board_name, "DIO-32DM3-PE") == 0){
		//---------------------------------------------
		// Get the status
		//---------------------------------------------
		lret = ApiBmGetStatus(&dio_data->bm_board.Master, param->dir, &BmStatus);
		PioStatus	= _inmd(MEM_DIO_PORT_ADD + 0x14);
		Start		= 0;
		PioStatus	|= dio_data->bm_board.PioSence;
		//---------------------------------------------
		// If the PIO is not started, clear the STOP status
		//---------------------------------------------
		if((param->dir == DIODM_DIR_IN) && !(dio_data->bm_board.PioSence & 0x0001) && !(dio_data->bm_board.SavePioSence & 0x0001)){
			PioStatus	&= ~(0x2);
		}
		if((param->dir == DIODM_DIR_OUT) && !((dio_data->bm_board.PioSence >> 16) & 0x0001) && !((dio_data->bm_board.SavePioSence >> 16) & 0x0001)){
			PioStatus	&= ~(0x20000);
		}
		//----------------------------------------
		// Release the spinlock
		//----------------------------------------
		spin_unlock_irqrestore(&dio_data->spinlock_flag, flags);
		dwHalt = (PioStatus >> 30) & 0x03;
		if (param->dir == DIODM_DIR_IN) {
			PioStatus	= PioStatus & 0x0000ffff;
			TimeOut		= dio_data->bm_board.Master.BmInp.TimeOutFlag;
		}
		if (param->dir == DIODM_DIR_OUT) {
			PioStatus	= (PioStatus >> 16) & 0x0000ffff;
			TimeOut		= dio_data->bm_board.Master.BmOut.TimeOutFlag;
		}
	} else {
		lret = ApiBmGetStatus(&dio_data->bm_board.Master, param->dir, &BmStatus);
		BmInpD(dio_data->bm_board.dwAddr + 0x14, PioStatus);
		Start = ApiBmGetStartEnabled(&dio_data->bm_board.Master, param->dir);
		PioStatus |= dio_data->bm_board.PioSence;
		//----------------------------------------
		// Release the spinlock
		//----------------------------------------
		spin_unlock_irqrestore(&dio_data->spinlock_flag, flags);
		dwHalt = (PioStatus >> 30) & 0x03;
		if (param->dir == DIODM_DIR_IN) {
			PioStatus = PioStatus & 0x0000ffff;
		} else {
			PioStatus = (PioStatus >> 16) & 0x0000ffff;
		}
		TimeOut	= 0;
	}
	//---------------------------------------------
	// Split the status to error and information.
	//---------------------------------------------
	param->status	= 0x00;
	param->err		= 0x00;
	// Status
	if (BmStatus & BM_STATUS_ALL_END)		param->status |= DIODM_STATUS_BMSTOP;
	if (PioStatus & 0x0001)					param->status |= DIODM_STATUS_PIOSTART;
	if (PioStatus & 0x0002)					param->status |= DIODM_STATUS_PIOSTOP;
	if (PioStatus & 0x0004)					param->status |= DIODM_STATUS_TRGIN;
	if (PioStatus & 0x0008)					param->status |= DIODM_STATUS_OVERRUN;
	if ((PioStatus != 0 || BmStatus != 0) && Start == BM_TRANS_STOP) param->status |= DIODM_STATUS_BMSTOP;
	// Error
	if (BmStatus & BM_STATUS_FIFO_EMPTY)	param->err |= DIODM_STATUS_FIFOEMPTY;
	if (BmStatus & BM_STATUS_FIFO_FULL)		param->err |= DIODM_STATUS_FIFOFULL;
	if (BmStatus & BM_STATUS_SG_OVER_IN)	param->err |= DIODM_STATUS_SGOVERIN;
	if (PioStatus & 0x0010)					param->err |= DIODM_STATUS_TRGERR;
	if (PioStatus & 0x0020)					param->err |= DIODM_STATUS_CLKERR;
	if (dwHalt    & 0x0001)					param->err |= DIODM_STATUS_SLAVEHALT;
	if (dwHalt    & 0x0002)					param->err |= DIODM_STATUS_MASTERHALT;
	//---------------------------------------------
	// This status occurres when DMA transfer processing is not completed within the specified time.
	// This status does not normally occur.
	//---------------------------------------------
	if (TimeOut == 1)						param->err |= DIODM_STATUS_TIMEOUTERR;
	//----------------------------------------
	// Set the return value
	//----------------------------------------
	param->ret = DIO_ERR_SUCCESS;
	return 0;
}

//================================================================
// Retrieve the transfer count
//================================================================
long Cdio_dm_get_count(void *device_ext, PCDIO_DM_GET_COUNT param)
{
	PCDIO_DATA		dio_data;
	long			lret;
	unsigned long	flags;
	unsigned long	IntSence;

	//----------------------------------------
	// Cast to structure
	//----------------------------------------
	dio_data = (PCDIO_DATA)device_ext;
	if (dio_data == NULL || param == NULL) {
		return -1;
	}
	//----------------------------------------
	// Check the supported boards
	//----------------------------------------
	if(dio_data->bios_d.b_info->board_type != CDIOBI_BT_PCI_DM){
		param->ret = DIO_ERR_SYS_NOT_SUPPORTED;
		return 0;
	}
	//-------------------------------------------
	// Check the parameters
	//-------------------------------------------
	if ((param->dir != DIODM_DIR_IN) && 
		(param->dir != DIODM_DIR_OUT)){
		param->ret = DIO_ERR_SYS_DIRECTION;
		return 0;
	}
	//----------------------------------------
	// Get the spinlock
	//----------------------------------------
	spin_lock_irqsave(&dio_data->spinlock_flag, flags);
	//---------------------------------------------
	// Get the count number.
	//---------------------------------------------
	if (strcmp(dio_data->board_name, "DIO-32DM3-PE") == 0){
		//---------------------------------------------
		// In the case of output, if the PIO is not started, returns 0 as the count value
		//---------------------------------------------
		if(((param->dir & DIODM_DIR_OUT) == DIODM_DIR_OUT) && !((dio_data->bm_board.SavePioSence >> 16) & 0x0001)){
			IntSence					= _inmd(MEM_DIO_PORT_ADD + 0x14);
			if (!((IntSence >> 16) & 0x0001)){
				dio_data->bm_board.Master.BmOut.resetcountFlag	= 1;
			} else {
				dio_data->bm_board.Master.BmOut.resetcountFlag	= 0;
				dio_data->bm_board.SavePioSence	|= IntSence;
			}
		} else if(((param->dir & DIODM_DIR_OUT) == DIODM_DIR_OUT) && ((dio_data->bm_board.SavePioSence >> 16) & 0x0001)){
			dio_data->bm_board.Master.BmOut.resetcountFlag	= 0;
		}
		lret = ApiMemBmGetCount(&dio_data->bm_board.Master, param->dir, &param->count, &param->carry);
	} else {
		lret = ApiBmGetCount(&dio_data->bm_board.Master, param->dir, &param->count, &param->carry);
	}
	//----------------------------------------
	// Release the spinlock
	//----------------------------------------
	spin_unlock_irqrestore(&dio_data->spinlock_flag, flags);
	if (lret == BM_ERROR_PARAM) {
		param->ret = DIO_ERR_DM_PARAM;
		return 0;
	}
	//----------------------------------------
	// Set the return value
	//----------------------------------------
	param->ret = DIO_ERR_SUCCESS;
	return 0;
}

//================================================================
// Retrieve the transfer count on user buffer (write point)
//================================================================
long Cdio_dm_get_write_pointer_user_buf(void *device_ext, PCDIO_DM_GET_WRITEPOINTER_USER_BUF param)
{
	PCDIO_DATA		dio_data;
	long			lret;
	unsigned long	flags;

	//----------------------------------------
	// Cast to structure
	//----------------------------------------
	dio_data = (PCDIO_DATA)device_ext;
	if (dio_data == NULL || param == NULL) {
		return -1;
	}
	//----------------------------------------
	// Check the supported boards
	//----------------------------------------
	if(dio_data->bios_d.b_info->board_type != CDIOBI_BT_PCI_DM){
		param->ret = DIO_ERR_SYS_NOT_SUPPORTED;
		return 0;
	}
	//-------------------------------------------
	// Check the parameters
	//-------------------------------------------
	if ((param->dir != DIODM_DIR_IN) && 
		(param->dir != DIODM_DIR_OUT)){
		param->ret = DIO_ERR_SYS_DIRECTION;
		return 0;
	}
	//----------------------------------------
	// Get the spinlock
	//----------------------------------------
	spin_lock_irqsave(&dio_data->spinlock_flag, flags);
	//---------------------------------------------
	// Get the count number.
	//---------------------------------------------
	if (strcmp(dio_data->board_name, "DIO-32DM3-PE") == 0){
		lret = ApiMemBmGetCount(&dio_data->bm_board.Master, param->dir, &param->count, &param->carry);
		if ((param->dir == DIODM_DIR_IN) &&
			(dio_data->bm_board.Master.BmInp.SgList.dwIsRing == 1) &&
			(dio_data->bm_board.Master.BmInp.sglen < 0x800) &&
			((dio_data->bm_board.Master.BmInp.sglen % 2) == 1) &&
			((param->count / dio_data->bm_board.Master.BmInp.sglen) % 2) &&
			((param->count - 1) == ((param->count / dio_data->bm_board.Master.BmInp.sglen) * dio_data->bm_board.Master.BmInp.sglen))){
			param->count	= param->count - 1;
		}
	} else {
		lret = ApiBmGetCount(&dio_data->bm_board.Master, param->dir, &param->count, &param->carry);
	}
	if (lret == BM_ERROR_PARAM) {
		param->ret = DIO_ERR_DM_PARAM;
		//----------------------------------------
		// Release the spinlock
		//----------------------------------------
		spin_unlock_irqrestore(&dio_data->spinlock_flag, flags);
		return 0;
	}
	//---------------------------------------------
	// Get the user buffer size.
	//---------------------------------------------
	lret = ApiBmGetBufLen(&dio_data->bm_board.Master, param->dir, &param->buf_len);
	//----------------------------------------
	// Release the spinlock
	//----------------------------------------
	spin_unlock_irqrestore(&dio_data->spinlock_flag, flags);
	if (lret == BM_ERROR_PARAM) {
		param->ret = DIO_ERR_DM_PARAM;
		return 0;
	}
	//----------------------------------------
	// Set the return value
	//----------------------------------------
	param->ret = DIO_ERR_SUCCESS;
	return 0;
}

//================================================================
// Retrieve the FIFO count
//================================================================
long Cdio_dm_get_fifo_count(void *device_ext, PCDIO_DM_GET_FIFO_COUNT param)
{
	PCDIO_DATA		dio_data;
	long			lret;
	unsigned long	flags;

	//----------------------------------------
	// Cast to structure
	//----------------------------------------
	dio_data = (PCDIO_DATA)device_ext;
	if (dio_data == NULL || param == NULL) {
		return -1;
	}
	//----------------------------------------
	// Check the supported boards
	//----------------------------------------
	if(dio_data->bios_d.b_info->board_type != CDIOBI_BT_PCI_DM){
		param->ret = DIO_ERR_SYS_NOT_SUPPORTED;
		return 0;
	}
	//-------------------------------------------
	// Check the parameters
	//-------------------------------------------
	if ((param->dir != DIODM_DIR_IN) && 
		(param->dir != DIODM_DIR_OUT)){
		param->ret = DIO_ERR_SYS_DIRECTION;
		return 0;
	}
	//----------------------------------------
	// Get the spinlock
	//----------------------------------------
	spin_lock_irqsave(&dio_data->spinlock_flag, flags);
	//---------------------------------------------
	// Get the FIFO count number.
	//---------------------------------------------
	if (strcmp(dio_data->board_name, "DIO-32DM3-PE") == 0){
		lret = ApiMemBmCheckFifo(&dio_data->bm_board.Master, param->dir, &param->count);
	} else {
		lret = ApiBmCheckFifo(&dio_data->bm_board.Master, param->dir, &param->count);
	}
	//----------------------------------------
	// Release the spinlock
	//----------------------------------------
	spin_unlock_irqrestore(&dio_data->spinlock_flag, flags);
	if (lret == BM_ERROR_PARAM) {
		param->ret = DIO_ERR_DM_PARAM;
		return 0;
	}
	//----------------------------------------
	// Set the return value
	//----------------------------------------
	param->ret = DIO_ERR_SUCCESS;
	return 0;
}

//================================================================
// Retrieve the BusMaster interrupt count
//================================================================
long Cdio_dm_get_int_count(void *device_ext, PCDIO_DM_GET_INT_COUNT param)
{
	PCDIO_DATA		dio_data;
	BM_GET_INT_CNT	int_count;
	long			lret;
	unsigned long	flags;

	//----------------------------------------
	// Cast to structure
	//----------------------------------------
	dio_data = (PCDIO_DATA)device_ext;
	if (dio_data == NULL || param == NULL) {
		return -1;
	}
	//----------------------------------------
	// Check the supported boards
	//----------------------------------------
	if(dio_data->bios_d.b_info->board_type != CDIOBI_BT_PCI_DM){
		param->ret = DIO_ERR_SYS_NOT_SUPPORTED;
		return 0;
	}
	//----------------------------------------
	// Get the spinlock
	//----------------------------------------
	spin_lock_irqsave(&dio_data->spinlock_flag, flags);
	//---------------------------------------------
	// Get the number of interrupt counts.
	//---------------------------------------------
	lret = ApiBmGetIntCount(&dio_data->bm_board.Master, &int_count);
	//----------------------------------------
	// Release the spinlock
	//----------------------------------------
	spin_unlock_irqrestore(&dio_data->spinlock_flag, flags);
	if (lret == BM_ERROR_PARAM) {
		param->ret = DIO_ERR_DM_PARAM;
		return 0;
	}
	//---------------------------------------------
	// Set number of counts to the return value
	//---------------------------------------------
	param->in_count_sence_num	= int_count.dwInCountSenceNum;
	param->in_end_sence_num		= int_count.dwInEndSenceNum;
	param->out_count_sence_num	= int_count.dwOutCountSenceNum;
	param->out_end_sence_num	= int_count.dwOutEndSenceNum;
	//----------------------------------------
	// Set the return value
	//----------------------------------------
	param->ret = DIO_ERR_SUCCESS;
	return 0;
}

//================================================================
// DM thread sleep function
//================================================================
long Cdio_dm_wait_event(void *device_ext, PCDIO_DM_TH_WAIT param)
{
	PCDIO_DATA		dio_data;

	//----------------------------------------
	// Cast to structure
	//----------------------------------------
	dio_data = (PCDIO_DATA)device_ext;
	if (dio_data == NULL || param == NULL) {
		return -1;
	}
	//----------------------------------------
	// Check the supported boards
	//----------------------------------------
	if(dio_data->bios_d.b_info->board_type != CDIOBI_BT_PCI_DM){
		param->ret = DIO_ERR_SYS_NOT_SUPPORTED;
		return 0;
	}
	//---------------------------------------------
	// To sleep the thread for DM
	//---------------------------------------------
	ApiBmWaitEvent(&dio_data->bm_board.Master);
	//----------------------------------------
	// Set the return value
	//----------------------------------------
	param->ret = DIO_ERR_SUCCESS;
	return 0;
}
//================================================================
// DM thread wake-up function
//================================================================
long Cdio_dm_wake_up_event(void *device_ext, PCDIO_DM_TH_WAKE_UP param)
{
	PCDIO_DATA		dio_data;

	//----------------------------------------
	// Cast to structure
	//----------------------------------------
	dio_data = (PCDIO_DATA)device_ext;
	if (dio_data == NULL || param == NULL) {
		return -1;
	}
	//----------------------------------------
	// Check the supported boards
	//----------------------------------------
	if(dio_data->bios_d.b_info->board_type != CDIOBI_BT_PCI_DM){
		param->ret = DIO_ERR_SYS_NOT_SUPPORTED;
		return 0;
	}
	//---------------------------------------------
	// Wake up the thread for DM
	//---------------------------------------------
	ApiBmWakeUpEvent(&dio_data->bm_board.Master);
	//----------------------------------------
	// Set the return value
	//----------------------------------------
	param->ret = DIO_ERR_SUCCESS;
	return 0;
}
//================================================================
// Notification of the transfer completion with specified number setting
//================================================================
long Cdio_dm_set_count_event(void *device_ext, PCDIO_DM_SET_COUNT_EVENT param)
{
	PCDIO_DATA		dio_data;
	long			lret;
	unsigned long	flags;

	//----------------------------------------
	// Cast to structure
	//----------------------------------------
	dio_data = (PCDIO_DATA)device_ext;
	if (dio_data == NULL || param == NULL) {
		return -1;
	}
	//----------------------------------------
	// Check the supported boards
	//----------------------------------------
	if(dio_data->bios_d.b_info->board_type != CDIOBI_BT_PCI_DM){
		param->ret = DIO_ERR_SYS_NOT_SUPPORTED;
		return 0;
	}
	//-------------------------------------------
	// Check the parameters
	//-------------------------------------------
	if ((param->dir != DIODM_DIR_IN) && 
		(param->dir != DIODM_DIR_OUT)){
		param->ret = DIO_ERR_SYS_DIRECTION;
		return 0;
	}
	if (strcmp(dio_data->board_name, "DIO-32DM3-PE") == 0){
		//-------------------------------------------
		// In the memory access device, due to the hardware IP specification,
		// if 1 or 2 is set to the count value, no interrupt will occur (during sampling).
		// There is no problem on the generating side, 
		// but according to the sampling the settable range is 3 or more.
		//-------------------------------------------
		if ((param->count > 0x20000000) | (param->count < 3)) {
			param->ret = DIO_ERR_SYS_COUNT;
			return 0;
		}
	} else {
		if ((param->count > 0xffffff) | (param->count == 0)) {
			param->ret = DIO_ERR_SYS_COUNT;
			return 0;
		}
	}
	//----------------------------------------
	// Get the spinlock
	//----------------------------------------
	spin_lock_irqsave(&dio_data->spinlock_flag, flags);
	//---------------------------------------------
	// Set the transfer number interrupt.
	//---------------------------------------------
	if (strcmp(dio_data->board_name, "DIO-32DM3-PE") == 0){
		lret = ApiMemBmSetNotifyCount(&dio_data->bm_board.Master, param->dir, param->count);
	} else {
		lret = ApiBmSetNotifyCount(&dio_data->bm_board.Master, param->dir, param->count);
	}
	//----------------------------------------
	// Release the spinlock
	//----------------------------------------
	spin_unlock_irqrestore(&dio_data->spinlock_flag, flags);
	if (lret == BM_ERROR_PARAM) {
		param->ret = DIO_ERR_DM_PARAM;
		return 0;
	}
	//----------------------------------------
	// Set the return value
	//----------------------------------------
	param->ret = DIO_ERR_SUCCESS;
	return 0;
}
//================================================================
// Post-process at the transfer completion interrupt
//================================================================
void Cdio_dm_trans_end_process(unsigned long dio_data_)
{
	unsigned long	dwFifoCount, dwStatus, dwCnt;
	long			Enabled;
	unsigned long	Inp_stop;
	unsigned short 	PioInSence;
	unsigned short 	PioOutSence;

	PCDIO_DATA dio_data = (PCDIO_DATA) dio_data_;
	//-----------------------------------------------
	// Split interrupt factor
	//-----------------------------------------------
	PioInSence	= (unsigned short)(dio_data->bm_board.PioSence & 0xffff);
	PioOutSence	= (unsigned short)((dio_data->bm_board.PioSence >> 16) & 0xffff);

	if (strcmp(dio_data->board_name, "DIO-32DM3-PE") == 0){
		//-----------------------------------------------
		// Completion process of input
		//-----------------------------------------------
		if ((PioInSence & 0x02) ||(dio_data->bm_board.Master.BmInp.sgoverflag == 1)) {
			//-----------------------------------------
			// Wait until FIFO is empty or S/GOverIn comes up
			//-----------------------------------------
			for (dwCnt = 0; ; dwCnt++) {
				dwFifoCount	= 0;
				dwStatus	= 0;
				ApiMemBmCheckFifo(&dio_data->bm_board.Master, BM_DIR_IN, &dwFifoCount);
				ApiBmGetStatus(&dio_data->bm_board.Master, BM_DIR_IN, &dwStatus);
				//-----------------------------------------
				// Stop the master transfer
				//-----------------------------------------
				if ((dwFifoCount == 0) ||
					(dwStatus & BM_STATUS_SG_OVER_IN) ||
					(dwStatus & BM_STATUS_ALL_END) ||
					(dwCnt == 100)){
					ApiMemBmStop(&dio_data->bm_board.Master, BM_DIR_IN);
					dio_data->bm_board.Master.BmInp.sgoverflag	= 0;
					Inp_stop	= _inmd(MEM_DIO_PORT_ADD + 0x6);
					_outmd(MEM_DIO_PORT_ADD + 0x6, (Inp_stop & 0x2));
					break;
				}
				udelay(1);
			}
			//-----------------------------------------
			// Close the mask and open the Event interrupt mask
			//-----------------------------------------
			dio_data->bm_board.PioMask |= 0x0000ffff;
			dio_data->bm_board.PioMask &= 0xffff0fff | (((unsigned short)dio_data->bios_d.int_mask[0] << 12) & 0x0000f000);
			_outmd(MEM_DIO_PORT_ADD + 0x10, dio_data->bm_board.PioMask);
			dio_data->bm_board.Master.BmInp.dmastopflag	= 1;
		}
		//-----------------------------------------------
		// Completion process of output
		//-----------------------------------------------
		if (PioOutSence & 0x02) {
			//-----------------------------------------
			// Stop the master transfer
			//-----------------------------------------
			ApiMemBmStop(&dio_data->bm_board.Master, BM_DIR_OUT);
			//-----------------------------------------
			// Close the mask and open the Event interrupt mask
			//-----------------------------------------
			dio_data->bm_board.PioMask |= 0xffff0000;
			dio_data->bm_board.PioMask &= 0xffff0fff | (((unsigned short)dio_data->bios_d.int_mask[0] << 12) & 0x0000f000);
			_outmd(MEM_DIO_PORT_ADD + 0x10, dio_data->bm_board.PioMask);
		}
		dio_data->bm_board.SavePioMask	= dio_data->bm_board.PioMask;
	} else {
		//-----------------------------------------------
		// Completion process of input
		//-----------------------------------------------
		if (PioInSence & 0x02) {
			//-----------------------------------------
			// Stop the process if it was stopped by the master error
			//-----------------------------------------
			ApiBmGetStatus(&dio_data->bm_board.Master, BM_DIR_IN, &dwStatus);
			if (dwStatus & BM_STATUS_END) {
				return;
			}
			//-----------------------------------------
			// Wait until FIFO is empty or S/GOverIn comes up
			//-----------------------------------------
			dwCnt = 0;
			for ( ; ; ) {
				dwFifoCount = 0;
				dwStatus = 0;
				ApiBmCheckFifo(&dio_data->bm_board.Master, BM_DIR_IN, &dwFifoCount);
				ApiBmGetStatus(&dio_data->bm_board.Master, BM_DIR_IN, &dwStatus);
				Enabled = ApiBmGetStartEnabled(&dio_data->bm_board.Master, BM_DIR_IN);
				//-----------------------------------------
				// Stop the master transfer
				//-----------------------------------------
				if ((dwFifoCount == 0) || (dwStatus & BM_STATUS_SG_OVER_IN) ||
					(dwStatus & BM_STATUS_END) || (Enabled == BM_TRANS_STOP) || dwCnt == 100) {
					ApiBmStop(&dio_data->bm_board.Master, BM_DIR_IN);
					break;
				}
				dwCnt++;
			}
			// Close the mask and open the Event interrupt mask
			dio_data->bm_board.PioMask |= 0x0000ffff;
			dio_data->bm_board.PioMask &= 0xffff0fff | (((unsigned short)dio_data->bios_d.int_mask[0] << 12) & 0x0000f000);
			BmOutD(dio_data->bm_board.dwAddr + 0x10, dio_data->bm_board.PioMask);
		}
		//-----------------------------------------------
		// Completion process of output
		//-----------------------------------------------
		if (PioOutSence & 0x02) {
			//-----------------------------------------
			// Stop the master transfer
			//-----------------------------------------
			ApiBmStop(&dio_data->bm_board.Master, BM_DIR_OUT);
			// Close the mask and open the Event interrupt mask
			dio_data->bm_board.PioMask |= 0xffff0000;
			dio_data->bm_board.PioMask &= 0xffff0fff | (((unsigned short)dio_data->bios_d.int_mask[0] << 12) & 0x0000f000);
			BmOutD(dio_data->bm_board.dwAddr + 0x10, dio_data->bm_board.PioMask);
		}
	}
	//-----------------------------------------
	// Notification of transfer completion on input
	//-----------------------------------------
	if ((PioInSence & 0x02) && (strcmp(dio_data->board_name, "DIO-32DM3-PE") != 0)){
		ApiBmSetStopEvent(&dio_data->bm_board.Master, BM_DIR_IN);
	}
	//-----------------------------------------
	// Notification of transfer completion on output
	//-----------------------------------------
	if (PioOutSence & 0x02) {
		ApiBmSetStopEvent(&dio_data->bm_board.Master, BM_DIR_OUT);
	}
	//----------------------------------------
	// Set the return value
	//----------------------------------------
	return;
}

//================================================================
// Subroutine of the interrupt service routine 
//================================================================
long Cdio_dm_isr(void *device_ext, unsigned char *sence)
{
	PCDIO_DATA		dio_data;
	long			MyInt =  0;		// No own interrupt
	unsigned short	InStatus, OutStatus;
	unsigned short	PioInSence, PioOutSence;
	unsigned long	IntSence;
	long			lret;
	unsigned long	dmastatus_pre_o, dmastatus_pre_i;
	unsigned long	IntSenceBM;
	unsigned long	trnum, MemData, resetadd;
	unsigned long	loop, waitloop;
	unsigned long	outData, inData;
	int				restarttop;
	
	unsigned long	tatnum;
	unsigned long	tasklet_flag = 0;

	//----------------------------------------
	// Cast to structure
	//----------------------------------------
	dio_data = (PCDIO_DATA)device_ext;
	if (dio_data == NULL) {
		return 0;
	}

	if (strcmp(dio_data->board_name, "DIO-32DM3-PE") == 0){
		//-----------------------------------------------
		// Interrupt status input and drop
		//-----------------------------------------------
		dmastatus_pre_o					= _inmd(MEM_DIO_OUT_PRE_ADD + 0x10);
		dmastatus_pre_i					= _inmd(MEM_DIO_INP_PRE_ADD + 0x10);
		IntSenceBM						= _inmd(MEM_DIO_PORT_ADD + 0x4C);
		IntSence						= _inmd(MEM_DIO_PORT_ADD + 0x14);
		dio_data->bm_board.PioSence		|= IntSence;
		dio_data->bm_board.SavePioSence	|= dio_data->bm_board.PioSence;
		_outmd((MEM_DIO_PORT_ADD + 0x14), IntSence);
		//---------------------------------------------
		// Process only when the BusMaster is operating
		//---------------------------------------------
		if ((dio_data->bm_board.Master.BmInp.dmastopflag == 0) || (dio_data->bm_board.Master.BmOut.dmastopflag == 0)){
			//---------------------------------------------
			// FIFO REACH IN Interrupt
			//---------------------------------------------
			if ((IntSenceBM & 0x80) == 0x80){
				//---------------------------------------------
				// Interrupt Status Clear
				//---------------------------------------------
				_outmd((MEM_DIO_PORT_ADD + 0x4C), 0x80);
				//---------------------------------------------
				// The number of Descriptor List is in the range
				//---------------------------------------------
				if (dio_data->bm_board.Master.BmInp.resetchangedesc < 0x400){
					//---------------------------------------------
					// Infinite transfer
					//---------------------------------------------
					if (dio_data->bm_board.Master.BmInp.SgList.dwIsRing == 1){
						//---------------------------------------------
						// When DMA transfer is performed once or more
						//---------------------------------------------
						if (dio_data->bm_board.Master.BmInp.cntflag == 1){
							//---------------------------------------------
							// When the buffer is 4kByte or more
							//---------------------------------------------
							if ((dio_data->bm_board.Master.BmInp.sglen) >= 0x1000){
								//---------------------------------------------
								// If the buffer size is not divisible by 2kByte
								//---------------------------------------------
								if ((dio_data->bm_board.Master.BmInp.sglen % 0x800) != 0){
									//---------------------------------------------
									// When the configured Descriptor is final
									//---------------------------------------------
									if (dio_data->bm_board.Master.BmInp.outcnt >= (dio_data->bm_board.Master.BmInp.SgList.ReSetArray - 1)){
										//---------------------------------------------
										// Reconfigure Descriptor List
										//---------------------------------------------
										MemBmSetSGAddressRing(&dio_data->bm_board.Master, DIODM_DIR_IN);
										//---------------------------------------------
										// Set transfer start command to Prefetcher CSR
										//---------------------------------------------
										_outmd((MEM_DIO_INP_PRE_ADD + 0x4), dio_data->bm_board.Master.BmInp.SgList.MemParam.ATPreFetchAdd);
										_outmd((MEM_DIO_INP_PRE_ADD), 0x1);
										if (dio_data->bm_board.Master.BmInp.outcnt == (dio_data->bm_board.Master.BmInp.SgList.ReSetArray - 1)){
											dio_data->bm_board.Master.BmInp.outcnt++;
										}
									} else {
										//---------------------------------------------
										// Set transfer start command to Prefetcher CSR
										//---------------------------------------------
										_outmd((MEM_DIO_INP_PRE_ADD + 0x4), (dio_data->bm_board.Master.BmInp.SgList.ChangeDescAdd[dio_data->bm_board.Master.BmInp.outcnt] + 0x40));
										_outmd((MEM_DIO_INP_PRE_ADD), 0x1);
										dio_data->bm_board.Master.BmInp.outcnt++;
									}
								//---------------------------------------------
								// If the buffer size is a multiple of 2kByte
								//---------------------------------------------
								} else {
									//---------------------------------------------
									// When the configured Descriptor is final
									//---------------------------------------------
									if (dio_data->bm_board.Master.BmInp.outcnt == dio_data->bm_board.Master.BmInp.SgList.ReSetArray){
										//---------------------------------------------
										// Set transfer start command to Prefetcher CSR
										//---------------------------------------------
										if (dio_data->bm_board.Master.BmInp.SgList.nr_pages > 150){
											//---------------------------------------------
											// Reconfigure Descriptor List
											//---------------------------------------------
											MemBmSetSGAddressRing(&dio_data->bm_board.Master, DIODM_DIR_IN);
											_outmd((MEM_DIO_INP_PRE_ADD + 0x4), dio_data->bm_board.Master.BmInp.SgList.MemParam.ATPreFetchAdd);
											_outmd((MEM_DIO_INP_PRE_ADD), 0x1);
										} else {
											if ((dio_data->bm_board.Master.BmInp.sglen % 4096) != 0){
												if (dio_data->bm_board.Master.BmInp.ringstartflag == 0){
													//---------------------------------------------
													// Reconfigure Descriptor List
													//---------------------------------------------
													MemBmSetSGAddressRing(&dio_data->bm_board.Master, DIODM_DIR_IN);
													dio_data->bm_board.Master.BmInp.ringresetflag	= 1;
													_outmd((MEM_DIO_INP_PRE_ADD + 0x4), dio_data->bm_board.Master.BmInp.SgList.MemParam.ATPreFetchAdd);
													_outmd((MEM_DIO_INP_PRE_ADD), 0x1);
												}
											} else {
												if (dio_data->bm_board.Master.BmInp.ringstartflag == 0){
													dio_data->bm_board.Master.BmInp.outcnt	= 0;
													_outmd((MEM_DIO_INP_PRE_ADD + 0x4), BM_MEM_INP_NXT_DESC);
													_outmd((MEM_DIO_INP_PRE_ADD), 0x11);
												}
											}
										}
									} else {
										//---------------------------------------------
										// Set transfer start command to Prefetcher CSR
										//---------------------------------------------
										_outmd((MEM_DIO_INP_PRE_ADD + 0x4), (dio_data->bm_board.Master.BmInp.SgList.ChangeDescAdd[dio_data->bm_board.Master.BmInp.outcnt] + 0x40));
										if (dio_data->bm_board.Master.BmInp.SgList.nr_pages > 150){
											if (dio_data->bm_board.Master.BmInp.outcnt == (dio_data->bm_board.Master.BmInp.SgList.ReSetArray - 1)){
												//---------------------------------------------
												// Reconfigure Descriptor List
												//---------------------------------------------
												MemBmSetSGAddressRing(&dio_data->bm_board.Master, DIODM_DIR_IN);
												_outmd((MEM_DIO_INP_PRE_ADD), 0x1);
											} else {
												_outmd((MEM_DIO_INP_PRE_ADD), 0x1);
											}
										} else {
											if ((dio_data->bm_board.Master.BmInp.sglen % 4096) != 0){
												if (dio_data->bm_board.Master.BmInp.ringstartflag == 0){
													_outmd((MEM_DIO_INP_PRE_ADD), 0x1);
												} else {
													_outmd((MEM_DIO_INP_PRE_ADD), 0x9);
												}
											} else {
												if (dio_data->bm_board.Master.BmInp.ringstartflag == 0){
													_outmd((MEM_DIO_INP_PRE_ADD), 0x11);
												} else {
													_outmd((MEM_DIO_INP_PRE_ADD), 0x9);
												}
											}
										}
										dio_data->bm_board.Master.BmInp.outcnt++;
									}
								}
							//---------------------------------------------
							// If the buffer is less than 4kByte
							//---------------------------------------------
							} else {
								//---------------------------------------------
								// Set transfer start command to Prefetcher CSR
								//---------------------------------------------
								if (dio_data->bm_board.Master.BmInp.sglen > 0x800){
									if (dio_data->bm_board.Master.BmInp.firstDMA == 0){
										//---------------------------------------------
										// Reconfigure Descriptor List
										//---------------------------------------------
										MemBmSetSGAddressRing(&dio_data->bm_board.Master, DIODM_DIR_IN);
										_outmd((MEM_DIO_INP_PRE_ADD + 0x4), dio_data->bm_board.Master.BmInp.SgList.MemParam.ATPreFetchAdd);
										_outmd((MEM_DIO_INP_PRE_ADD), 0x11);
									} else {
										_outmd((MEM_DIO_INP_PRE_ADD + 0x4), (dio_data->bm_board.Master.BmInp.SgList.ChangeDescAdd[0] + 0x40));
										_outmd((MEM_DIO_INP_PRE_ADD), 0x11);
										dio_data->bm_board.Master.BmInp.firstDMA	= 0;
									}
								} else {
									//---------------------------------------------
									// Reconfigure Descriptor List
									//---------------------------------------------
									//---------------------------------------------
									// When the buffer size is 2KByte
									//---------------------------------------------
									if (dio_data->bm_board.Master.BmInp.sglen == 0x800){
										MemBmReSetSGAddressRing(&dio_data->bm_board.Master, DIODM_DIR_IN);
										_outmd((MEM_DIO_INP_PRE_ADD + 0x4), dio_data->bm_board.Master.BmInp.SgList.MemParam.ATPreFetchAdd);
										_outmd((MEM_DIO_INP_PRE_ADD), 0x11);
									} else {
										MemBmReSetSGAddressRing(&dio_data->bm_board.Master, DIODM_DIR_IN);
										_outmd((MEM_DIO_INP_PRE_ADD + 0x4), dio_data->bm_board.Master.BmInp.SgList.MemParam.ATPreFetchAdd);
										_outmd((MEM_DIO_INP_PRE_ADD), 0x1);
									}
								}
							}
						} else {
							//---------------------------------------------
							// Start the BusMaster
							//---------------------------------------------
							//---------------------------------------------
							// Infinite transfer
							//---------------------------------------------
							if (dio_data->bm_board.Master.BmInp.ringstartflag == 1){
								_outmd(MEM_DIO_INP_PRE_ADD, 0x9);
							//---------------------------------------------
							// Transfer once
							//---------------------------------------------
							} else {
								//---------------------------------------------
								// If the buffer size is a multiple of 4KByte,
								// because there is no need to modify the Descriptor List,
								// Add option to transfer command
								//---------------------------------------------
								if ((dio_data->bm_board.Master.BmInp.sglen % 4096) == 0){
									_outmd(MEM_DIO_INP_PRE_ADD, 0x11);
								} else {
									_outmd(MEM_DIO_INP_PRE_ADD, 0x1);
								}
							}
							dio_data->bm_board.Master.BmInp.cntflag		= 1;
							dio_data->bm_board.Master.BmInp.firstDMA	= 1;
						}
					//---------------------------------------------
					// Transfer once
					//---------------------------------------------
					} else {
						//---------------------------------------------
						// When DMA transfer is performed once or more
						//---------------------------------------------
						if (dio_data->bm_board.Master.BmInp.cntflag == 1){
							//---------------------------------------------
							// When the buffer size is 2 KBytes or more and the final data transfer is not completed
							// Execute DMA transfer start command
							//---------------------------------------------
							if ((dio_data->bm_board.Master.BmInp.sglen) > 0x800){
								if ((dio_data->bm_board.Master.BmInp.outcnt < dio_data->bm_board.Master.BmInp.SgList.ReSetArray) &&
									(dio_data->bm_board.Master.BmInp.listoverflag == 0)){
									//---------------------------------------------
									// Set transfer start command to Prefetcher CSR
									//---------------------------------------------
									_outmd((MEM_DIO_INP_PRE_ADD + 0x4), (dio_data->bm_board.Master.BmInp.SgList.ChangeDescAdd[dio_data->bm_board.Master.BmInp.outcnt] + 0x40));
									_outmd((MEM_DIO_INP_PRE_ADD), 0x9);
								}
								dio_data->bm_board.Master.BmInp.outcnt++;
								//---------------------------------------------
								// Initialize count when count reaches maximum
								//---------------------------------------------
								if (dio_data->bm_board.Master.BmInp.outcnt == dio_data->bm_board.Master.BmInp.SgList.ReSetArray){
									dio_data->bm_board.Master.BmInp.outcnt	= 0;
									if (dio_data->bm_board.Master.BmInp.SgList.nr_pages <= 150){
										dio_data->bm_board.Master.BmInp.listoverflag	= 1;
									}
								}
								//---------------------------------------------
								// If there is a configurable SGList and 
								// the configured Descriptor is final, reconfigure the Descriptor List
								//---------------------------------------------
								if (dio_data->bm_board.Master.BmInp.addtablenum > 0){
									if (dio_data->bm_board.Master.BmInp.atstartflag == 0){
										if (dio_data->bm_board.Master.BmInp.outcnt >= dio_data->bm_board.Master.BmInp.athalflen){
											dio_data->bm_board.Master.BmInp.atstartflag	= 1;
										}
									} else {
										//---------------------------------------------
										// Reconfigure Descriptor List
										//---------------------------------------------
										MemBmReSetSGAddress(&dio_data->bm_board.Master, DIODM_DIR_IN);
									}
								}
							}
						} else {
							//---------------------------------------------
							// Start the BusMaster
							//---------------------------------------------
							_outmd((MEM_DIO_INP_PRE_ADD), 0x9);
							dio_data->bm_board.Master.BmInp.cntflag	= 1;
						}
					}
				}
				MyInt = 2;	// BusMaster interrupt
			}
			//---------------------------------------------
			// FIFO REACH OUT Interrupt
			//---------------------------------------------
			if ((IntSenceBM & 0x800000) == 0x800000){
				//---------------------------------------------
				// Interrupt Status Clear
				//---------------------------------------------
				_outmd((MEM_DIO_PORT_ADD + 0x4C), 0x800000);
				if ((IntSence & 0x20000) != 0x20000){
					//---------------------------------------------
					// If the number of Descriptor Lists is in the range
					//---------------------------------------------
					if (dio_data->bm_board.Master.BmOut.resetchangedesc < 0x400){
						//---------------------------------------------
						// Infinite transfer
						//---------------------------------------------
						if (dio_data->bm_board.Master.BmOut.SgList.dwIsRing == 1){
							//---------------------------------------------
							// When the buffer is 4kByte or more
							//---------------------------------------------
							if ((dio_data->bm_board.Master.BmOut.sglen) >= 0x1000){
								//---------------------------------------------
								// If the buffer size is not divisible by 2kByte
								//---------------------------------------------
								if ((dio_data->bm_board.Master.BmOut.sglen % 0x800) != 0){
									//---------------------------------------------
									// Set transfer start command to Prefetcher CSR
									//---------------------------------------------
									if (dio_data->bm_board.Master.BmOut.outcnt == dio_data->bm_board.Master.BmOut.SgList.ReSetArray){
										//---------------------------------------------
										// Reconfigure Descriptor List
										//---------------------------------------------
										MemBmSetSGAddressRing(&dio_data->bm_board.Master, DIODM_DIR_OUT);
										_outmd((MEM_DIO_OUT_PRE_ADD + 0x4), dio_data->bm_board.Master.BmOut.SgList.MemParam.ATPreFetchAdd);
										_outmd((MEM_DIO_OUT_PRE_ADD), 0x1);
									} else {
										_outmd((MEM_DIO_OUT_PRE_ADD + 0x4), (dio_data->bm_board.Master.BmOut.SgList.ChangeDescAdd[dio_data->bm_board.Master.BmOut.outcnt] + 0x40));
										if (dio_data->bm_board.Master.BmOut.outcnt == (dio_data->bm_board.Master.BmOut.SgList.ReSetArray - 1)){
											//---------------------------------------------
											// Reconfigure Descriptor List and align to 2 KByte boundary
											//---------------------------------------------
											MemBmSetSGAddressRing(&dio_data->bm_board.Master, DIODM_DIR_OUT);
											_outmd((MEM_DIO_OUT_PRE_ADD), 0x1);
										} else {
											_outmd((MEM_DIO_OUT_PRE_ADD), 0x1);
										}
										dio_data->bm_board.Master.BmOut.outcnt++;
										dio_data->bm_board.Master.BmOut.cntflag	= 0;
									}
								//---------------------------------------------
								// If the buffer size is a multiple of 2kByte
								//---------------------------------------------
								} else {
									//---------------------------------------------
									// When the Descriptor List is final
									//---------------------------------------------
									if ((dio_data->bm_board.Master.BmOut.outcnt == dio_data->bm_board.Master.BmOut.SgList.ReSetArray)){
										if (dio_data->bm_board.Master.BmOut.SgList.nr_pages > 150){
											//---------------------------------------------
											// Reconfigure Descriptor List
											//---------------------------------------------
											MemBmSetSGAddressRing(&dio_data->bm_board.Master, DIODM_DIR_OUT);
											_outmd((MEM_DIO_OUT_PRE_ADD + 0x4), dio_data->bm_board.Master.BmOut.SgList.MemParam.ATPreFetchAdd);
											_outmd((MEM_DIO_OUT_PRE_ADD), 0x11);
										} else {
											if (dio_data->bm_board.Master.BmOut.cntflag == 0){
												//---------------------------------------------
												// Set next block as final block
												//---------------------------------------------
												_outmd((dio_data->bm_board.dwAddrBM_mem + dio_data->bm_board.Master.BmOut.SgList.ChangeDescAdd[0] + 0xC), (BM_MEM_OUT_NXT_DESC - 0x40));
												dio_data->bm_board.Master.BmOut.cntflag = 1;
											}
											dio_data->bm_board.Master.BmOut.outcnt	= 0;
											_outmd((MEM_DIO_OUT_PRE_ADD + 0x4), BM_MEM_OUT_NXT_DESC);
											_outmd((MEM_DIO_OUT_PRE_ADD), 0x11);
										}
									} else {
										_outmd((MEM_DIO_OUT_PRE_ADD + 0x4), (dio_data->bm_board.Master.BmOut.SgList.ChangeDescAdd[dio_data->bm_board.Master.BmOut.outcnt] + 0x40));
										//---------------------------------------------
										// When the number of SGList is 151 or more
										//---------------------------------------------
										if (dio_data->bm_board.Master.BmOut.SgList.nr_pages > 150){
											//---------------------------------------------
											// When Descriptor List is (total number - 1)
											//---------------------------------------------
											if ((dio_data->bm_board.Master.BmOut.outcnt == (dio_data->bm_board.Master.BmOut.SgList.ReSetArray - 1))){
												//---------------------------------------------
												// Reconfigure Descriptor List and align to 2 KByte boundary
												//---------------------------------------------
												MemBmSetSGAddressRing(&dio_data->bm_board.Master, DIODM_DIR_OUT);
												_outmd((MEM_DIO_OUT_PRE_ADD), 0x11);
											} else {
												_outmd((MEM_DIO_OUT_PRE_ADD), 0x11);
											}
										} else {
											_outmd((MEM_DIO_OUT_PRE_ADD), 0x11);
										}
										dio_data->bm_board.Master.BmOut.outcnt++;
									}
								}
							} else {
								//---------------------------------------------
								// If the buffer is less than 4kByte
								//---------------------------------------------
								if ((dio_data->bm_board.Master.BmOut.sglen % 32) != 0){
									//---------------------------------------------
									// When the buffer size is less than 28 data
									//---------------------------------------------
									if (dio_data->bm_board.Master.BmOut.sglen < 28){
										//---------------------------------------------
										// Wait until DMA transfer completed
										//---------------------------------------------
										loop	= 0;
										outData	= _inmd(MEM_DIO_OUT_ADD);
										if ((outData & 0x1) == 0x1){
											while (loop < 1000){
												udelay(1);
												outData	= _inmd(MEM_DIO_OUT_ADD);
												if ((outData & 0x1) != 0x1){
													break;
												}
												loop++;
											}
										}
										//------------------------------------
										// If the number of loops exceeds the maximum value,
										// Return TimeOut Error
										//------------------------------------
										if (loop >= 1000){
											dio_data->bm_board.Master.BmOut.TimeOutFlag	= 1;
											//-----------------------------------------------
											// Completion processing on output
											//-----------------------------------------------
											dio_data->bm_board.KeepStart &= ~BM_DIR_OUT;
											tasklet_flag = 1;
											if (dio_data->bm_board.Master.BmOut.end_ev_save != 1){
												dio_data->bm_board.Master.BmOut.endsmp = 1;
											}
											lret = ApiMemBmInterrupt(&dio_data->bm_board.Master, &InStatus, &OutStatus, IntSenceBM);
											MyInt = 2;	// BusMaster interrupt
											return MyInt;
										}
										//---------------------------------------------
										// Reconfigure Descriptor List
										//---------------------------------------------
										MemBmReSetSGAddressRing(&dio_data->bm_board.Master, DIODM_DIR_OUT);
										_outmd((MEM_DIO_OUT_PRE_ADD + 0x4), dio_data->bm_board.Master.BmOut.SgList.MemParam.ATPreFetchAdd);
										_outmd((MEM_DIO_OUT_PRE_ADD), 0x1);
									//---------------------------------------------
									// When the buffer size is 28 data or more
									//---------------------------------------------
									} else {
										//---------------------------------------------
										// Wait until DMA transfer completed
										//---------------------------------------------
										loop	= 0;
										outData	= _inmd(MEM_DIO_OUT_ADD);
										if ((outData & 0x1) == 0x1){
											while (loop < 1000){
												udelay(1);
												outData	= _inmd(MEM_DIO_OUT_ADD);
												if ((outData & 0x1) != 0x1){
													break;
												}
												loop++;
											}
										}
										//------------------------------------
										// If the number of loops exceeds the maximum value,
										// Return TimeOut Error
										//------------------------------------
										if (loop >= 1000){
											dio_data->bm_board.Master.BmOut.TimeOutFlag	= 1;
											//-----------------------------------------------
											// Completion processing on output
											//-----------------------------------------------
											dio_data->bm_board.KeepStart &= ~BM_DIR_OUT;
											tasklet_flag = 1;
											if (dio_data->bm_board.Master.BmOut.end_ev_save != 1){
												dio_data->bm_board.Master.BmOut.endsmp = 1;
											}
											lret = ApiMemBmInterrupt(&dio_data->bm_board.Master, &InStatus, &OutStatus, IntSenceBM);
											MyInt = 2;	// BusMaster interrupt
											return MyInt;
										}
										//---------------------------------------------
										// Reconfigure Descriptor List
										//---------------------------------------------
										MemBmSetSGAddressRing(&dio_data->bm_board.Master, DIODM_DIR_OUT);
										_outmd((MEM_DIO_OUT_PRE_ADD + 0x4), dio_data->bm_board.Master.BmOut.SgList.MemParam.ATPreFetchAdd);
										_outmd((MEM_DIO_OUT_PRE_ADD), 0x1);
										dio_data->bm_board.Master.BmOut.cntflag	= 0;
									}
								} else {
									//---------------------------------------------
									// Set transfer start command to Prefetcher CSR
									//---------------------------------------------
									//---------------------------------------------
									// When the Descriptor List is final
									//---------------------------------------------
									if (dio_data->bm_board.Master.BmOut.outcnt == dio_data->bm_board.Master.BmOut.SgList.ReSetArray){
										dio_data->bm_board.Master.BmOut.outcnt	= 0;
										_outmd((MEM_DIO_OUT_PRE_ADD + 0x4), BM_MEM_OUT_NXT_DESC);
										_outmd((MEM_DIO_OUT_PRE_ADD), 0x11);
									} else {
										_outmd((MEM_DIO_OUT_PRE_ADD + 0x4), (dio_data->bm_board.Master.BmOut.SgList.ChangeDescAdd[dio_data->bm_board.Master.BmOut.outcnt] + 0x40));
										_outmd((MEM_DIO_OUT_PRE_ADD), 0x11);
										dio_data->bm_board.Master.BmOut.outcnt++;
									}
								}
							}
						//---------------------------------------------
						// Transfer once
						//---------------------------------------------
						} else {
							//---------------------------------------------
							// DMA transfer command is necessary if the buffer size is 4 KBytes or more
							//---------------------------------------------
							if ((dio_data->bm_board.Master.BmOut.sglen) > 0x1000){
								//---------------------------------------------
								// Perform DMA transfer if the configured Descriptor is not final
								//---------------------------------------------
								if (dio_data->bm_board.Master.BmOut.outcnt < dio_data->bm_board.Master.BmOut.SgList.ReSetArray){
									//---------------------------------------------
									// Set transfer start command to Prefetcher CSR
									//---------------------------------------------
									_outmd((MEM_DIO_OUT_PRE_ADD + 0x4), (dio_data->bm_board.Master.BmOut.SgList.ChangeDescAdd[dio_data->bm_board.Master.BmOut.outcnt] + 0x40));
									_outmd((MEM_DIO_OUT_PRE_ADD), 0x9);
								}
								dio_data->bm_board.Master.BmOut.outcnt++;
								dio_data->bm_board.Master.BmOut.cntflag	= 0;
								//---------------------------------------------
								// Initialize count of transferred Descriptors for reconfiguring Descriptor List
								//---------------------------------------------
								if (dio_data->bm_board.Master.BmOut.outcnt == dio_data->bm_board.Master.BmOut.SgList.ReSetArray){
									dio_data->bm_board.Master.BmOut.outcnt		= 0;
								}
								//---------------------------------------------
								// If there is a configurable SGList and the configured Descriptor is final, 
								// reconfigure the Descriptor List
								//---------------------------------------------
								if (dio_data->bm_board.Master.BmOut.addtablenum > 0){
									if (dio_data->bm_board.Master.BmOut.atstartflag == 0){
										if (dio_data->bm_board.Master.BmOut.outcnt >= dio_data->bm_board.Master.BmOut.athalflen){
											dio_data->bm_board.Master.BmOut.atstartflag	= 1;
										}
									} else {
										//---------------------------------------------
										// Reconfigure Descriptor List
										//---------------------------------------------
										MemBmReSetSGAddress(&dio_data->bm_board.Master, DIODM_DIR_OUT);
									}
								}
							}
						}
					}
					MyInt = 2;	// BusMaster interrupt
				}
			}
			//---------------------------------------------
			// DMA transfer completion interrupt (output)
			// Start port only for the first time after clearing interrupt status
			//---------------------------------------------
			if ((dmastatus_pre_o & 0x1) == 0x1) {
				//---------------------------------------------
				// Interrupt Status Clear
				//---------------------------------------------
				_outmd((MEM_DIO_OUT_PRE_ADD + 0x10), 0x1);
				if (dio_data->bm_board.Master.BmOut.outflag == 1){
					//---------------------------------------------
					// If it is infinite transfer and the buffer size is a multiple of 32, 
					// calculation of the Descriptor list may differ, so correction here
					//---------------------------------------------
					if (((dio_data->bm_board.Master.BmOut.sglen % 32) == 0) &&
						((dio_data->bm_board.Master.BmOut.sglen) < 0x1000) &&
						(dio_data->bm_board.Master.BmOut.SgList.dwIsRing == 1)){
						_outmd((dio_data->bm_board.dwAddrBM_mem + dio_data->bm_board.Master.BmOut.SgList.ChangeDescAdd[0] + 0xC), (BM_MEM_OUT_NXT_DESC - 0x40));
					}
					//---------------------------------------------
					// Port Start
					//---------------------------------------------
					dio_data->bm_board.KeepStart	|= DIODM_DIR_OUT;
					_outmw(MEM_DIO_PORT_ADD + 0x06, dio_data->bm_board.KeepStart);
					dio_data->bm_board.Master.BmOut.outflag = 0;
				}
				MyInt = 2;	// BusMaster interrupt
			}
			//---------------------------------------------
			// DMA transfer completion interrupt (input)
			// Start port and BusMaster after clearing interrupt status
			//---------------------------------------------
			if ((dmastatus_pre_i & 0x1) == 0x1) {
				//---------------------------------------------
				// Interrupt Status Clear
				//---------------------------------------------
				_outmd((MEM_DIO_INP_PRE_ADD + 0x10), 0x1);
				tasklet_flag = 1;
				//-----------------------------------------------
				// Completion process of input
				//-----------------------------------------------
				if (dio_data->bm_board.Master.BmInp.end_ev_save != 1){
					dio_data->bm_board.Master.BmInp.endsmp	= 1;
				}
				dio_data->bm_board.Master.BmInp.dmastopflag	= 1;
				//-----------------------------------------
				// Determination of S/GOverIn
				//-----------------------------------------
				//-----------------------------------------
				// Get number of remaining data
				//-----------------------------------------
				dio_data->bm_board.Master.BmInp.remaingdatanum	= _inmw(MEM_DIO_PORT_ADD + 0x60);
				//-----------------------------------------
				// Set SGOverIn status if there is remaining data in the once transfer
				//-----------------------------------------
				if (dio_data->bm_board.Master.BmInp.SgList.dwIsRing == 0){
					if ((dio_data->bm_board.Master.BmInp.remaingdatanum > 0) && (dio_data->bm_board.Master.BmInp.remaingdatanum <= 0x1000)){
						dio_data->bm_board.Master.BmInp.dwIntSence |= BM_STATUS_SG_OVER_IN;
						dio_data->bm_board.KeepStart			&= ~BM_DIR_IN;
						_outmd(MEM_DIO_PORT_ADD + 0x6, dio_data->bm_board.KeepStart);
						//-----------------------------------------
						// In case of synchronous operation and Slave side, SlaveHalt signal is output.
						//-----------------------------------------
						if (dio_data->bm_board.Master.SlaveEnable != 0){
							_outmd(MEM_DIO_PORT_ADD + 0x18, 0x8100);
							_outmd(MEM_DIO_PORT_ADD + 0x18, dio_data->bm_board.Master.SlaveEnable);
						}
						//-----------------------------------------
						// In case of synchronous operation and Master side, MasterHalt signal is output.
						//-----------------------------------------
						if (dio_data->bm_board.Master.MasterEnable != 0){
							_outmd(MEM_DIO_PORT_ADD + 0x18, 0x821F);
							_outmd(MEM_DIO_PORT_ADD + 0x18, dio_data->bm_board.Master.MasterEnable);
						}
					}
				}
				//-----------------------------------------
				// Get the number of transfer completion data
				//-----------------------------------------
				trnum	= _inmd(MEM_DIO_PORT_ADD + 0x20);
				//-----------------------------------------
				// When the number of transfer completion data is an odd number,
				// Set a flag to correct the count value
				//-----------------------------------------
				if ((((trnum + dio_data->bm_board.Master.BmInp.remaingdatanum) % 2) == 1) || (dio_data->bm_board.Master.BmInp.remaingdatanum > 0x1000)){
					if ((dio_data->bm_board.Master.BmInp.SgList.dwIsRing == 0) &&
						((trnum - 1) == dio_data->bm_board.Master.BmInp.sglen) &&
						((dio_data->bm_board.Master.BmInp.sglen % 2) == 1)){
							dio_data->bm_board.Master.BmInp.oddtransferflag	= 1;
					}
				} else if ((dio_data->bm_board.Master.BmInp.SgList.dwIsRing == 0) &&
					((trnum - 1) == dio_data->bm_board.Master.BmInp.sglen)){
					dio_data->bm_board.Master.BmInp.oddtransferflag	= 1;
				} else if (((trnum - 1) < dio_data->bm_board.Master.BmInp.sglen) && (trnum >= dio_data->bm_board.Master.BmInp.sglen)){
					dio_data->bm_board.Master.BmInp.oddtransferflag	= 0;
				}
				ApiMemBmStop(&dio_data->bm_board.Master, BM_DIR_IN);
				MyInt = 2;	// BusMaster interrupt
			}
			//---------------------------------------------
			// Sampling complete interrupt
			//---------------------------------------------
			if ((IntSence & 0x2) == 0x2){
				//---------------------------------------------
				// Infinite transfer
				//---------------------------------------------
				if (dio_data->bm_board.Master.BmInp.SgList.dwIsRing == 1){
					//---------------------------------------------
					// If the transfer completion post process has not been done
					//---------------------------------------------
					if (dio_data->bm_board.Master.BmInp.endsmp == 0){
						//---------------------------------------------
						// Wait until DMA transfer completed
						//---------------------------------------------
						waitloop	= 0;
						udelay(10);
						inData	= _inmd(MEM_DIO_INP_ADD);
						if (((inData & 0x1) == 0x1) || ((inData & 0x4) == 0x4) || ((inData & 0x10) == 0x10)){
							while (waitloop < 10000){
								udelay(10);
								_outmd(MEM_DIO_INP_ADD, inData);
								inData	= _inmd(MEM_DIO_INP_ADD);
								if (((inData & 0x1) != 0x1) && ((inData & 0x4) != 0x4) && ((inData & 0x10) != 0x10)){
									break;
								}
								waitloop++;
							}
						}
						//------------------------------------
						// If the number of loops exceeds the maximum value,
						// Return TimeOut Error
						//------------------------------------
						if (waitloop >= 10000){
							dio_data->bm_board.Master.BmInp.TimeOutFlag	= 1;
							//-----------------------------------------------
							// Completion process of input
							//-----------------------------------------------
							dio_data->bm_board.KeepStart			&= ~BM_DIR_IN;
							tasklet_flag = 1;
							if (dio_data->bm_board.Master.BmInp.end_ev_save != 1){
								dio_data->bm_board.Master.BmInp.endsmp	= 1;
							}
							lret = ApiMemBmInterrupt(&dio_data->bm_board.Master, &InStatus, &OutStatus, IntSenceBM);
							MyInt = 2;	// BusMaster interrupt
							return MyInt;
						}
						//-----------------------------------------
						// Get current transfer data count
						//-----------------------------------------
						trnum	= _inmd(MEM_DIO_PORT_ADD + 0x20);
						//-----------------------------------------
						// When the stop condition is Specified Number and 
						// the number of transfer completion data are more than the Specified Number
						//-----------------------------------------
						if ((dio_data->bm_board.Master.BmInp.ringstartflag == 1) && (trnum >= dio_data->bm_board.Sampling.StopNum)){
							//-----------------------------------------------
							// Completion process of input
							//-----------------------------------------------
							dio_data->bm_board.KeepStart			&= ~BM_DIR_IN;
							tasklet_flag = 1;
							if (dio_data->bm_board.Master.BmInp.end_ev_save != 1){
								dio_data->bm_board.Master.BmInp.endsmp	= 1;
							}
							//-----------------------------------------
							// When the number of transfer completion data is an odd number,
							// Set a flag to correct the count value
							//-----------------------------------------
							if (((trnum - 1) == dio_data->bm_board.Sampling.StopNum) && ((dio_data->bm_board.Sampling.StopNum % 2)) == 1){
								dio_data->bm_board.Master.BmInp.oddtransferflag	= 1;
							}
						} else {
							//-----------------------------------------
							// Get the number of remaining data in FIFO
							//-----------------------------------------
							dio_data->bm_board.Master.BmInp.remaingdatanum	= (_inmw(MEM_DIO_PORT_ADD + 0x60)) * 0x4;
							//-----------------------------------------
							// When the number of remaining data in FIFO is valid data and larger than FIFOReachIn
							//-----------------------------------------
							if ((dio_data->bm_board.Master.BmInp.remaingdatanum >= (dio_data->bm_board.Master.BmInp.fiforeachnum  * 0x4)) &&
								(dio_data->bm_board.Master.BmInp.remaingdatanum > 0) &&
								(dio_data->bm_board.Master.BmInp.remaingdatanum < 0x4000)){
								//-----------------------------------------
								// When the remaining data is 1 data
								//-----------------------------------------
								if (dio_data->bm_board.Master.BmInp.fiforeachnum == 0x1){
									dio_data->bm_board.Master.BmInp.remaingdatanum	=  0x4;
								} else {
									dio_data->bm_board.Master.BmInp.remaingdatanum	= (dio_data->bm_board.Master.BmInp.fiforeachnum - 1) * 0x4;
								}
							}
							//-----------------------------------------
							// When the sum of number of transfer completion data and number of remaining data in the FIFO is an odd number,
							// Set a flag to correct the count value
							//-----------------------------------------
							if ((((trnum + (dio_data->bm_board.Master.BmInp.remaingdatanum / 0x4 )) % 2) == 1) || (dio_data->bm_board.Master.BmInp.remaingdatanum > 0x4000)){
								dio_data->bm_board.Master.BmInp.oddtransferflag	= 1;
								//-----------------------------------------
								// When the buffer size is 2047 or less and an odd number, 
								// Set a flag to correct the count value
								//-----------------------------------------
								if (((trnum / dio_data->bm_board.Master.BmInp.sglen) % 2) &&
									((dio_data->bm_board.Master.BmInp.sglen % 2) == 1) &&
									(dio_data->bm_board.Master.BmInp.sglen < 0x800)){
									dio_data->bm_board.Master.BmInp.oddbufflag		= 1;
								}
							}
							//-----------------------------------------
							// If the number of remaining data is 0 or a negative number, perform the complete process
							//-----------------------------------------
							if ((dio_data->bm_board.Master.BmInp.remaingdatanum == 0) ||
								(dio_data->bm_board.Master.BmInp.remaingdatanum >= 0x4000)){
								//-----------------------------------------------
								// Completion process of input
								//-----------------------------------------------
								dio_data->bm_board.KeepStart			&= ~BM_DIR_IN;
								tasklet_flag = 1;
								if (dio_data->bm_board.Master.BmInp.end_ev_save != 1){
									dio_data->bm_board.Master.BmInp.endsmp	= 1;
								}
							//-----------------------------------------
							// When the number of remaining data is valid data
							//-----------------------------------------
							} else {
								MemData	= 0;
								tatnum	= 0;
								//---------------------------------------------
								// When transferred once or more
								//---------------------------------------------
								if (dio_data->bm_board.Master.BmInp.cntflag == 1){
									//---------------------------------------------
									// When the buffer is 4kByte or more
									//---------------------------------------------
									if ((dio_data->bm_board.Master.BmInp.sglen) >= 0x1000){
										//---------------------------------------------
										// If the buffer size is not divisible by 2kByte
										//---------------------------------------------
										if ((dio_data->bm_board.Master.BmInp.sglen % 0x800) != 0){
											if (dio_data->bm_board.Master.BmInp.outcnt == dio_data->bm_board.Master.BmInp.SgList.ReSetArray){
												//---------------------------------------------
												// Reconfiguring SGList
												//---------------------------------------------
												MemBmSetSGAddressRing(&dio_data->bm_board.Master, DIODM_DIR_IN);
												//---------------------------------------------
												// Transfer data size correction process
												//---------------------------------------------
												loop	= 0;
												while(loop < 10){
													MemData	+= _inmd(dio_data->bm_board.dwAddrBM_mem + dio_data->bm_board.Master.BmInp.SgList.MemParam.ATPreFetchAdd + (0x40 * loop) + 0x8);
													if (MemData >= dio_data->bm_board.Master.BmInp.remaingdatanum){
														_outmd((dio_data->bm_board.dwAddrBM_mem + (dio_data->bm_board.Master.BmInp.SgList.MemParam.ATPreFetchAdd + (0x40 * loop) + 0x8)), (dio_data->bm_board.Master.BmInp.remaingdatanum - tatnum));
														_outmd((dio_data->bm_board.dwAddrBM_mem + (dio_data->bm_board.Master.BmInp.SgList.MemParam.ATPreFetchAdd + (0x40 * loop) + 0xC)), (BM_MEM_INP_NXT_DESC - 0x40));
														_outmd((dio_data->bm_board.dwAddrBM_mem + (dio_data->bm_board.Master.BmInp.SgList.MemParam.ATPreFetchAdd + (0x40 * loop) + 0x3C)), 0x40004000);
														break;
													}
													tatnum	= MemData;
													loop++;
												}
												//---------------------------------------------
												// Set transfer start command to Prefetcher CSR
												//---------------------------------------------
												_outmd((MEM_DIO_INP_PRE_ADD + 0x4), dio_data->bm_board.Master.BmInp.SgList.MemParam.ATPreFetchAdd);
												_outmd((MEM_DIO_INP_PRE_ADD), 0x9);
											} else {
												restarttop	= 0;
												//---------------------------------------------
												// If SGList is final, reconfigure SGList
												//---------------------------------------------
												if (dio_data->bm_board.Master.BmInp.outcnt == (dio_data->bm_board.Master.BmInp.SgList.ReSetArray - 1)){
													MemBmSetSGAddressRing(&dio_data->bm_board.Master, DIODM_DIR_IN);
												}
												//---------------------------------------------
												// Transfer data size correction process
												//---------------------------------------------
												loop	= 1;
												while(loop < 10){
													MemData	+= _inmd((dio_data->bm_board.dwAddrBM_mem + (dio_data->bm_board.Master.BmInp.SgList.ChangeDescAdd[dio_data->bm_board.Master.BmInp.outcnt] + (0x40 * loop) + 0x8)));
													if (MemData >= dio_data->bm_board.Master.BmInp.remaingdatanum){
														_outmd(dio_data->bm_board.dwAddrBM_mem + (dio_data->bm_board.Master.BmInp.SgList.ChangeDescAdd[dio_data->bm_board.Master.BmInp.outcnt] + (0x40 * loop) + 0x8), (dio_data->bm_board.Master.BmInp.remaingdatanum - tatnum));
														_outmd(dio_data->bm_board.dwAddrBM_mem + (dio_data->bm_board.Master.BmInp.SgList.ChangeDescAdd[dio_data->bm_board.Master.BmInp.outcnt] + (0x40 * loop) + 0xC), (BM_MEM_INP_NXT_DESC - 0x40));
														_outmd(dio_data->bm_board.dwAddrBM_mem + (dio_data->bm_board.Master.BmInp.SgList.ChangeDescAdd[dio_data->bm_board.Master.BmInp.outcnt] + (0x40 * loop) + 0x3C), 0x40004000);
														break;
													}
													tatnum	= MemData;
													resetadd	= _inmd((dio_data->bm_board.dwAddrBM_mem + (dio_data->bm_board.Master.BmInp.SgList.ChangeDescAdd[dio_data->bm_board.Master.BmInp.outcnt] + (0x40 * loop) + 0xC)));
													if (resetadd == BM_MEM_INP_NXT_DESC){
														_outmd(dio_data->bm_board.dwAddrBM_mem + (dio_data->bm_board.Master.BmInp.SgList.ChangeDescAdd[dio_data->bm_board.Master.BmInp.outcnt] + (0x40 * loop) + 0x3C), 0x40000000);
														restarttop	= 1;
														break;
													}
													loop++;
												}
												//-----------------------------------------
												// If it overlaps with S/GList reset timing,
												// Because the next Descriptor may need to be fixed,
												// Check the next Descriptor 
												//-----------------------------------------
												if (restarttop == 1){
													loop	= 1;
													while(loop < 10){
														MemData	+= _inmd((dio_data->bm_board.dwAddrBM_mem + 0x06000000 + (0x40 * loop) + 0x8));
														if (MemData >= dio_data->bm_board.Master.BmInp.remaingdatanum){
															_outmd((dio_data->bm_board.dwAddrBM_mem + 0x06000000 + (0x40 * loop) + 0x8), (dio_data->bm_board.Master.BmInp.remaingdatanum - tatnum));
															_outmd((dio_data->bm_board.dwAddrBM_mem + 0x06000000 + (0x40 * loop) + 0xC), (BM_MEM_INP_NXT_DESC - 0x40));
															_outmd((dio_data->bm_board.dwAddrBM_mem + 0x06000000 + (0x40 * loop) + 0x3C), 0x40004000);
															break;
														}
														tatnum	= MemData;
														loop++;
													}
												}
												//---------------------------------------------
												// Set transfer start command to Prefetcher CSR
												//---------------------------------------------
												_outmd((MEM_DIO_INP_PRE_ADD + 0x4), (dio_data->bm_board.Master.BmInp.SgList.ChangeDescAdd[dio_data->bm_board.Master.BmInp.outcnt] + 0x40));
												_outmd((MEM_DIO_INP_PRE_ADD), 0x9);
											}
										//---------------------------------------------
										// If the buffer size is a multiple of 2kByte
										//---------------------------------------------
										} else {
											//---------------------------------------------
											// If the Descriptor is final
											//---------------------------------------------
											if (dio_data->bm_board.Master.BmInp.outcnt == dio_data->bm_board.Master.BmInp.SgList.ReSetArray){
												//---------------------------------------------
												// When the number of SGList is 151 or more
												//---------------------------------------------
												if (dio_data->bm_board.Master.BmInp.SgList.nr_pages > 150){
													//---------------------------------------------
													// Reconfigure SGList
													//---------------------------------------------
													MemBmSetSGAddressRing(&dio_data->bm_board.Master, DIODM_DIR_IN);
													//---------------------------------------------
													// Transfer data size correction process
													//---------------------------------------------
													loop	= 0;
													while(loop < 10){
														MemData	+= _inmd(dio_data->bm_board.dwAddrBM_mem + dio_data->bm_board.Master.BmInp.SgList.MemParam.ATPreFetchAdd + (0x40 * loop) + 0x8);
														if (MemData >= dio_data->bm_board.Master.BmInp.remaingdatanum){
															_outmd((dio_data->bm_board.dwAddrBM_mem + (dio_data->bm_board.Master.BmInp.SgList.MemParam.ATPreFetchAdd + (0x40 * loop) + 0x8)), (dio_data->bm_board.Master.BmInp.remaingdatanum - tatnum));
															_outmd((dio_data->bm_board.dwAddrBM_mem + (dio_data->bm_board.Master.BmInp.SgList.MemParam.ATPreFetchAdd + (0x40 * loop) + 0xC)), (BM_MEM_INP_NXT_DESC - 0x40));
															_outmd((dio_data->bm_board.dwAddrBM_mem + (dio_data->bm_board.Master.BmInp.SgList.MemParam.ATPreFetchAdd + (0x40 * loop) + 0x3C)), 0x40004000);
															break;
														}
														tatnum	= MemData;
														loop++;
													}
													_outmd((MEM_DIO_INP_PRE_ADD + 0x4), dio_data->bm_board.Master.BmInp.SgList.MemParam.ATPreFetchAdd);
													_outmd((MEM_DIO_INP_PRE_ADD), 0x9);
												//---------------------------------------------
												// When the number of SGList is 150 or less
												//---------------------------------------------
												} else {
													loop	= 0;
													//---------------------------------------------
													// If the buffer size is not a multiple of 4096
													//---------------------------------------------
													if ((dio_data->bm_board.Master.BmInp.sglen % 4096) != 0){
														//---------------------------------------------
														// Reconfigure SGList
														//---------------------------------------------
														MemBmSetSGAddressRing(&dio_data->bm_board.Master, DIODM_DIR_IN);
														//---------------------------------------------
														// Transfer data size correction process
														//---------------------------------------------
														loop	= 0;
														while(loop < 10){
															MemData	+= _inmd(dio_data->bm_board.dwAddrBM_mem + dio_data->bm_board.Master.BmInp.SgList.MemParam.ATPreFetchAdd + (0x40 * loop) + 0x8);
															if (MemData >= dio_data->bm_board.Master.BmInp.remaingdatanum){
																_outmd((dio_data->bm_board.dwAddrBM_mem + (dio_data->bm_board.Master.BmInp.SgList.MemParam.ATPreFetchAdd + (0x40 * loop) + 0x8)), (dio_data->bm_board.Master.BmInp.remaingdatanum - tatnum));
																_outmd((dio_data->bm_board.dwAddrBM_mem + (dio_data->bm_board.Master.BmInp.SgList.MemParam.ATPreFetchAdd + (0x40 * loop) + 0xC)), (BM_MEM_INP_NXT_DESC - 0x40));
																_outmd((dio_data->bm_board.dwAddrBM_mem + (dio_data->bm_board.Master.BmInp.SgList.MemParam.ATPreFetchAdd + (0x40 * loop) + 0x3C)), 0x40004000);
																break;
															}
															tatnum	= MemData;
															loop++;
														}
														_outmd((MEM_DIO_INP_PRE_ADD + 0x4), dio_data->bm_board.Master.BmInp.SgList.MemParam.ATPreFetchAdd);
														_outmd((MEM_DIO_INP_PRE_ADD), 0x9);
													//---------------------------------------------
													// If the buffer size is a multiple of 4096
													//---------------------------------------------
													} else {
														//---------------------------------------------
														// Transfer data size correction process
														//---------------------------------------------
														loop	= 1;
														while(loop < 10){
															MemData	+= _inmd((dio_data->bm_board.dwAddrBM_mem + 0x06000000 + (0x40 * loop) + 0x8));
															if (MemData >= dio_data->bm_board.Master.BmInp.remaingdatanum){
																_outmd((dio_data->bm_board.dwAddrBM_mem + 0x06000000 + (0x40 * loop) + 0x8), (dio_data->bm_board.Master.BmInp.remaingdatanum - tatnum));
																_outmd((dio_data->bm_board.dwAddrBM_mem + 0x06000000 + (0x40 * loop) + 0xC), (BM_MEM_INP_NXT_DESC - 0x40));
																_outmd((dio_data->bm_board.dwAddrBM_mem + 0x06000000 + (0x40 * loop) + 0x3C), 0x40004000);
																break;
															}
															tatnum	= MemData;
															loop++;
														}
														_outmd((MEM_DIO_INP_PRE_ADD + 0x4), BM_MEM_INP_NXT_DESC);
														_outmd((MEM_DIO_INP_PRE_ADD), 0x9);
													}
												}
											//---------------------------------------------
											// If the Descriptor is not final
											//---------------------------------------------
											} else {
												//---------------------------------------------
												// Transfer data size correction process
												//---------------------------------------------
												loop	= 1;
												while(loop < 10){
													MemData	+= _inmd((dio_data->bm_board.dwAddrBM_mem + (dio_data->bm_board.Master.BmInp.SgList.ChangeDescAdd[dio_data->bm_board.Master.BmInp.outcnt] + (0x40 * loop) + 0x8)));
													if (MemData >= dio_data->bm_board.Master.BmInp.remaingdatanum){
														_outmd(dio_data->bm_board.dwAddrBM_mem + (dio_data->bm_board.Master.BmInp.SgList.ChangeDescAdd[dio_data->bm_board.Master.BmInp.outcnt] + (0x40 * loop) + 0x8), (dio_data->bm_board.Master.BmInp.remaingdatanum - tatnum));
														_outmd(dio_data->bm_board.dwAddrBM_mem + (dio_data->bm_board.Master.BmInp.SgList.ChangeDescAdd[dio_data->bm_board.Master.BmInp.outcnt] + (0x40 * loop) + 0xC), (BM_MEM_INP_NXT_DESC - 0x40));
														_outmd(dio_data->bm_board.dwAddrBM_mem + (dio_data->bm_board.Master.BmInp.SgList.ChangeDescAdd[dio_data->bm_board.Master.BmInp.outcnt] + (0x40 * loop) + 0x3C), 0x40004000);
														break;
													}
													tatnum	= MemData;
													loop++;
												}
												//---------------------------------------------
												// Set transfer start command to Prefetcher CSR
												//---------------------------------------------
												_outmd((MEM_DIO_INP_PRE_ADD + 0x4), (dio_data->bm_board.Master.BmInp.SgList.ChangeDescAdd[dio_data->bm_board.Master.BmInp.outcnt] + 0x40));
												_outmd((MEM_DIO_INP_PRE_ADD), 0x9);
											}
										}
									//---------------------------------------------
									// If the buffer is less than 4kByte
									//---------------------------------------------
									} else {
										//---------------------------------------------
										// When the buffer size is 2049 or more
										//---------------------------------------------
										if (dio_data->bm_board.Master.BmInp.sglen > 0x800){
											//---------------------------------------------
											// When DMA transfer is performed once or more
											//---------------------------------------------
											if (dio_data->bm_board.Master.BmInp.firstDMA == 0){
												//---------------------------------------------
												// Reconfigure SGList
												//---------------------------------------------
												MemBmSetSGAddressRing(&dio_data->bm_board.Master, DIODM_DIR_IN);
												//---------------------------------------------
												// Transfer data size correction process
												//---------------------------------------------
												loop	= 0;
												while(loop < 10){
													MemData	+= _inmd(dio_data->bm_board.dwAddrBM_mem + dio_data->bm_board.Master.BmInp.SgList.MemParam.ATPreFetchAdd  + (0x40 * loop) + 0x8);
													if (MemData >= dio_data->bm_board.Master.BmInp.remaingdatanum){
														_outmd((dio_data->bm_board.dwAddrBM_mem + (dio_data->bm_board.Master.BmInp.SgList.MemParam.ATPreFetchAdd + (0x40 * loop) + 0x8)), (dio_data->bm_board.Master.BmInp.remaingdatanum - tatnum));
														_outmd((dio_data->bm_board.dwAddrBM_mem + (dio_data->bm_board.Master.BmInp.SgList.MemParam.ATPreFetchAdd + (0x40 * loop) + 0xC)), (BM_MEM_INP_NXT_DESC - 0x40));
														_outmd((dio_data->bm_board.dwAddrBM_mem + (dio_data->bm_board.Master.BmInp.SgList.MemParam.ATPreFetchAdd + (0x40 * loop) + 0x3C)), 0x40004000);
														break;
													}
													tatnum	= MemData;
													loop++;
												}
												_outmd((MEM_DIO_INP_PRE_ADD + 0x4), dio_data->bm_board.Master.BmInp.SgList.MemParam.ATPreFetchAdd);
												_outmd((MEM_DIO_INP_PRE_ADD), 0x9);
											//---------------------------------------------
											// If DMA transfer has not been performed even once
											//---------------------------------------------
											} else {
												//---------------------------------------------
												// Transfer data size correction process
												//---------------------------------------------
												loop	= 1;
												while(loop < 10){
													MemData	+= _inmd((dio_data->bm_board.dwAddrBM_mem + dio_data->bm_board.Master.BmInp.SgList.ChangeDescAdd[0] + (0x40 * loop) + 0x8));
													if (MemData >= dio_data->bm_board.Master.BmInp.remaingdatanum){
														_outmd((dio_data->bm_board.dwAddrBM_mem + dio_data->bm_board.Master.BmInp.SgList.ChangeDescAdd[0] + (0x40 * loop) + 0x8), (dio_data->bm_board.Master.BmInp.remaingdatanum - tatnum));
														_outmd((dio_data->bm_board.dwAddrBM_mem + dio_data->bm_board.Master.BmInp.SgList.ChangeDescAdd[0] + (0x40 * loop) + 0xC), (BM_MEM_INP_NXT_DESC - 0x40));
														_outmd((dio_data->bm_board.dwAddrBM_mem + dio_data->bm_board.Master.BmInp.SgList.ChangeDescAdd[0] + (0x40 * loop) + 0x3C), 0x40004000);
														break;
													}
													tatnum	= MemData;
													loop++;
												}
												_outmd((MEM_DIO_INP_PRE_ADD + 0x4), (dio_data->bm_board.Master.BmInp.SgList.ChangeDescAdd[0] + 0x40));
												_outmd((MEM_DIO_INP_PRE_ADD), 0x9);
												dio_data->bm_board.Master.BmInp.firstDMA	= 0;
											}
										//---------------------------------------------
										// When the buffer size is 2049 or less
										//---------------------------------------------
										} else {
											//---------------------------------------------
											// When the buffer size is 2 KByte
											//---------------------------------------------
											if (dio_data->bm_board.Master.BmInp.sglen == 0x800){
												//---------------------------------------------
												// Reconfigure SGList
												//---------------------------------------------
												MemBmReSetSGAddressRing(&dio_data->bm_board.Master, DIODM_DIR_IN);
												//---------------------------------------------
												// Transfer data size correction process
												//---------------------------------------------
												loop	= 0;
												while(loop < 10){
													MemData	+= _inmd(dio_data->bm_board.dwAddrBM_mem + dio_data->bm_board.Master.BmInp.SgList.MemParam.ATPreFetchAdd  + (0x40 * loop) + 0x8);
													if (MemData >= dio_data->bm_board.Master.BmInp.remaingdatanum){
														_outmd((dio_data->bm_board.dwAddrBM_mem + (dio_data->bm_board.Master.BmInp.SgList.MemParam.ATPreFetchAdd + (0x40 * loop) + 0x8)), (dio_data->bm_board.Master.BmInp.remaingdatanum - tatnum));
														_outmd((dio_data->bm_board.dwAddrBM_mem + (dio_data->bm_board.Master.BmInp.SgList.MemParam.ATPreFetchAdd + (0x40 * loop) + 0xC)), (BM_MEM_INP_NXT_DESC - 0x40));
														_outmd((dio_data->bm_board.dwAddrBM_mem + (dio_data->bm_board.Master.BmInp.SgList.MemParam.ATPreFetchAdd + (0x40 * loop) + 0x3C)), 0x40004000);
														break;
													}
													tatnum	= MemData;
													loop++;
												}
												_outmd((MEM_DIO_INP_PRE_ADD + 0x4), dio_data->bm_board.Master.BmInp.SgList.MemParam.ATPreFetchAdd);
												_outmd((MEM_DIO_INP_PRE_ADD), 0x9);
											} else {
												//---------------------------------------------
												// Correct the transfer data in case of odd data size
												//---------------------------------------------
												if ((dio_data->bm_board.Master.BmInp.oddbufflag == 1) &&
													(dio_data->bm_board.Master.BmInp.remaingdatanum < ((dio_data->bm_board.Master.BmInp.fiforeachnum - 1)  * 0x4))){
													dio_data->bm_board.Master.BmInp.remaingdatanum	+=  0x4;
												}
												//---------------------------------------------
												// Reconfigure SGList
												//---------------------------------------------
												MemBmReSetSGAddressRing(&dio_data->bm_board.Master, DIODM_DIR_IN);
												//---------------------------------------------
												// Transfer data size correction process
												//---------------------------------------------
												loop	= 0;
												while(loop < 10){
													MemData	+= _inmd(dio_data->bm_board.dwAddrBM_mem + dio_data->bm_board.Master.BmInp.SgList.MemParam.ATPreFetchAdd + (0x40 * loop) + 0x8);
													if (MemData >= dio_data->bm_board.Master.BmInp.remaingdatanum){
														_outmd((dio_data->bm_board.dwAddrBM_mem + (dio_data->bm_board.Master.BmInp.SgList.MemParam.ATPreFetchAdd + (0x40 * loop) + 0x8)), (dio_data->bm_board.Master.BmInp.remaingdatanum - tatnum));
														_outmd((dio_data->bm_board.dwAddrBM_mem + (dio_data->bm_board.Master.BmInp.SgList.MemParam.ATPreFetchAdd + (0x40 * loop) + 0xC)), (BM_MEM_INP_NXT_DESC - 0x40));
														_outmd((dio_data->bm_board.dwAddrBM_mem + (dio_data->bm_board.Master.BmInp.SgList.MemParam.ATPreFetchAdd + (0x40 * loop) + 0x3C)), 0x40004000);
														break;
													}
													tatnum	= MemData;
													loop++;
												}
												_outmd((MEM_DIO_INP_PRE_ADD + 0x4), dio_data->bm_board.Master.BmInp.SgList.MemParam.ATPreFetchAdd);
												_outmd((MEM_DIO_INP_PRE_ADD), 0x9);
											}
										}
									}
								//-----------------------------------------
								// If DMA transfer has not been performed even once
								//-----------------------------------------
								} else {
									//---------------------------------------------
									// Transfer data size correction process
									//---------------------------------------------
									loop	= 1;
									while(loop < 10){
										MemData	+= _inmd((dio_data->bm_board.dwAddrBM_mem + 0x06000000 + (0x40 * loop) + 0x8));
										if (MemData >= dio_data->bm_board.Master.BmInp.remaingdatanum){
											_outmd((dio_data->bm_board.dwAddrBM_mem + 0x06000000 + (0x40 * loop) + 0x8), (dio_data->bm_board.Master.BmInp.remaingdatanum - tatnum));
											_outmd((dio_data->bm_board.dwAddrBM_mem + 0x06000000 + (0x40 * loop) + 0xC), (BM_MEM_INP_NXT_DESC - 0x40));
											_outmd((dio_data->bm_board.dwAddrBM_mem + 0x06000000 + (0x40 * loop) + 0x3C), 0x40004000);
											break;
										}
										tatnum	= MemData;
										loop++;
									}
									_outmd(MEM_DIO_INP_PRE_ADD, 0x9);
									dio_data->bm_board.Master.BmInp.cntflag		= 1;
									dio_data->bm_board.Master.BmInp.firstDMA	= 1;
								}
							}
						}
					}
				//-----------------------------------------
				// Transfer once
				//-----------------------------------------
				} else {
					//-----------------------------------------
					// If the transfer completion post process has not been done
					//-----------------------------------------
					if (dio_data->bm_board.Master.BmInp.endsmp == 0){
						//---------------------------------------------
						// Wait until DMA transfer completed
						//---------------------------------------------
						waitloop	= 0;
						udelay(10);
						inData	= _inmd(MEM_DIO_INP_ADD);
						if (((inData & 0x1) == 0x1) || ((inData & 0x4) == 0x4) || ((inData & 0x10) == 0x10)){
							while (waitloop < 10000){
								udelay(10);
								_outmd(MEM_DIO_INP_ADD, inData);
								inData	= _inmd(MEM_DIO_INP_ADD);
								if (((inData & 0x1) != 0x1) && ((inData & 0x4) != 0x4) && ((inData & 0x10) != 0x10)){
									break;
								}
								waitloop++;
							}
						}
						//------------------------------------
						// If the number of loops exceeds the maximum value,
						// Return TimeOut Error
						//------------------------------------
						if (waitloop >= 10000){
							dio_data->bm_board.Master.BmInp.TimeOutFlag	= 1;
							MyInt = 2;	// BusMaster interrupt
							//-----------------------------------------------
							// Completion process of input
							//-----------------------------------------------
							dio_data->bm_board.KeepStart			&= ~BM_DIR_IN;
							tasklet_flag = 1;
							if (dio_data->bm_board.Master.BmInp.end_ev_save != 1){
								dio_data->bm_board.Master.BmInp.endsmp	= 1;
							}
							lret = ApiMemBmInterrupt(&dio_data->bm_board.Master, &InStatus, &OutStatus, IntSenceBM);
							return MyInt;
						}
						//-----------------------------------------
						// Check current transfer data count
						//-----------------------------------------
						trnum		= _inmd(MEM_DIO_PORT_ADD + 0x20);
						//-----------------------------------------
						// Get number of remaining data
						//-----------------------------------------
						dio_data->bm_board.Master.BmInp.remaingdatanum	= (_inmw(MEM_DIO_PORT_ADD + 0x60)) * 0x4;
						//-----------------------------------------
						// When the number of remaining data in FIFO is valid data and larger than FIFOReachIn
						//-----------------------------------------
						if ((dio_data->bm_board.Master.BmInp.remaingdatanum > (dio_data->bm_board.Master.BmInp.fiforeachnum  * 0x4)) &&
							(dio_data->bm_board.Master.BmInp.remaingdatanum > 0) &&
							(dio_data->bm_board.Master.BmInp.remaingdatanum < 0x4000)){
							dio_data->bm_board.Master.BmInp.remaingdatanum	= (dio_data->bm_board.Master.BmInp.fiforeachnum - 1) * 0x4;
						}
						//-----------------------------------------
						// When the sum of number of transfer completion data and number of remaining data in the FIFO is an odd number,
						// Set a flag to correct the count value
						//-----------------------------------------
						if ((((trnum + (dio_data->bm_board.Master.BmInp.remaingdatanum / 0x4 )) % 2) == 1) || (dio_data->bm_board.Master.BmInp.remaingdatanum > 0x4000)){
							dio_data->bm_board.Master.BmInp.oddtransferflag	= 1;
						}
						//-----------------------------------------------
						// When the number of transfer completion data is more than the buffer size
						//-----------------------------------------------
						if (trnum >= dio_data->bm_board.Master.BmInp.sglen){
							//-----------------------------------------------
							// Completion process of input
							//-----------------------------------------------
							dio_data->bm_board.KeepStart			&= ~BM_DIR_IN;
							tasklet_flag = 1;
							if (dio_data->bm_board.Master.BmInp.end_ev_save != 1){
								dio_data->bm_board.Master.BmInp.endsmp	= 1;
							}
							//-----------------------------------------
							// Set SGOverIn status if there is remaining data in FIFO
							//-----------------------------------------
							if ((dio_data->bm_board.Master.BmInp.remaingdatanum > 0) && (dio_data->bm_board.Master.BmInp.remaingdatanum < 0x4000)){
								dio_data->bm_board.Master.BmInp.dwIntSence |= BM_STATUS_SG_OVER_IN;
								//-----------------------------------------
								// When the number of transfer completion data is an odd number,
								// Set a flag to correct the count value
								//-----------------------------------------
								if ((trnum - 1) < dio_data->bm_board.Master.BmInp.sglen){
									dio_data->bm_board.Master.BmInp.oddtransferflag	= 0;
								}
								//-----------------------------------------
								// In case of synchronous operation and Slave side, SlaveHalt signal is output.
								//-----------------------------------------
								if (dio_data->bm_board.Master.SlaveEnable != 0){
									_outmd(MEM_DIO_PORT_ADD + 0x18, 0x8100);
									_outmd(MEM_DIO_PORT_ADD + 0x18, dio_data->bm_board.Master.SlaveEnable);
								}
								//-----------------------------------------
								// In case of synchronous operation and Master side, MasterHalt signal is output.
								//-----------------------------------------
								if (dio_data->bm_board.Master.MasterEnable != 0){
									_outmd(MEM_DIO_PORT_ADD + 0x18, 0x821F);
									_outmd(MEM_DIO_PORT_ADD + 0x18, dio_data->bm_board.Master.MasterEnable);
								}
							}
						//-----------------------------------------------
						// When the stop condition is Specified Number and 
						// the number of transfer completion data are more than the Specified Number
						//-----------------------------------------------
						} else if (((dio_data->bm_board.Sampling.Stop & 0x700) == 0x700) && (trnum >= dio_data->bm_board.Sampling.StopNum)){
							if (dio_data->bm_board.Master.BmInp.end_ev_save != 1){
								//-----------------------------------------------
								// Completion process of input
								//-----------------------------------------------
								dio_data->bm_board.KeepStart			&= ~BM_DIR_IN;
								tasklet_flag = 1;
								dio_data->bm_board.Master.BmInp.endsmp	= 1;
							}
							//-----------------------------------------
							// Set SGOverIn status if there is remaining data in FIFO
							//-----------------------------------------
							if ((dio_data->bm_board.Master.BmInp.remaingdatanum > 0) && (dio_data->bm_board.Master.BmInp.remaingdatanum < 0x4000)){
								dio_data->bm_board.Master.BmInp.dwIntSence |= BM_STATUS_SG_OVER_IN;
								//-----------------------------------------
								// In case of synchronous operation and Slave side, SlaveHalt signal is output.
								//-----------------------------------------
								if (dio_data->bm_board.Master.SlaveEnable != 0){
									_outmd(MEM_DIO_PORT_ADD + 0x18, 0x8100);
									_outmd(MEM_DIO_PORT_ADD + 0x18, dio_data->bm_board.Master.SlaveEnable);
								}
								//-----------------------------------------
								// In case of synchronous operation and Master side, MasterHalt signal is output.
								//-----------------------------------------
								if (dio_data->bm_board.Master.MasterEnable != 0){
									_outmd(MEM_DIO_PORT_ADD + 0x18, 0x821F);
									_outmd(MEM_DIO_PORT_ADD + 0x18, dio_data->bm_board.Master.MasterEnable);
								}
							}
						} else {
							//-----------------------------------------
							// When the number of remaining data in FIFO is valid data
							//-----------------------------------------
							if ((dio_data->bm_board.Master.BmInp.remaingdatanum > 0) && (dio_data->bm_board.Master.BmInp.remaingdatanum < 0x4000)){
								//-----------------------------------------
								// If the sum of the transfer completion data count and the remaining data count exceeds the buffer size
								// Correct transfer size to fit in buffer size
								//-----------------------------------------
								if ((trnum + (dio_data->bm_board.Master.BmInp.remaingdatanum / 0x4 )) > dio_data->bm_board.Master.BmInp.sglen){
									dio_data->bm_board.Master.BmInp.remaingdatanum	= (dio_data->bm_board.Master.BmInp.sglen - trnum) * 0x4;
									dio_data->bm_board.Master.BmInp.dwIntSence |= BM_STATUS_SG_OVER_IN;
									//-----------------------------------------
									// When the buffer size is an odd number,
									// Set a flag to correct the count value
									//-----------------------------------------
									if (dio_data->bm_board.Master.BmInp.sglen % 2){
										dio_data->bm_board.Master.BmInp.oddtransferflag	= 1;
									}
									//-----------------------------------------
									// In case of synchronous operation and Slave side, SlaveHalt signal is output.
									//-----------------------------------------
									if (dio_data->bm_board.Master.SlaveEnable != 0){
										_outmd(MEM_DIO_PORT_ADD + 0x18, 0x8100);
										_outmd(MEM_DIO_PORT_ADD + 0x18, dio_data->bm_board.Master.SlaveEnable);
									}
									//-----------------------------------------
									// In case of synchronous operation and Master side, MasterHalt signal is output.
									//-----------------------------------------
									if (dio_data->bm_board.Master.MasterEnable != 0){
										_outmd(MEM_DIO_PORT_ADD + 0x18, 0x821F);
										_outmd(MEM_DIO_PORT_ADD + 0x18, dio_data->bm_board.Master.MasterEnable);
									}
								}
								MemData	= 0;
								tatnum	= 0;
								loop	= 1;
								//---------------------------------------------
								// When transferred once or more
								//---------------------------------------------
								if (dio_data->bm_board.Master.BmInp.cntflag == 1){
									//---------------------------------------------
									// If the final SGList hasn't been transferred
									//---------------------------------------------
									if (dio_data->bm_board.Master.BmInp.outcnt < dio_data->bm_board.Master.BmInp.SgList.ReSetArray){
										while(loop < 10){
											MemData	+= _inmd((dio_data->bm_board.dwAddrBM_mem + (dio_data->bm_board.Master.BmInp.SgList.ChangeDescAdd[dio_data->bm_board.Master.BmInp.outcnt] + (0x40 * loop) + 0x8)));
											if (MemData >= dio_data->bm_board.Master.BmInp.remaingdatanum){
												_outmd(dio_data->bm_board.dwAddrBM_mem + (dio_data->bm_board.Master.BmInp.SgList.ChangeDescAdd[dio_data->bm_board.Master.BmInp.outcnt] + (0x40 * loop) + 0x8), (dio_data->bm_board.Master.BmInp.remaingdatanum - tatnum));
												_outmd(dio_data->bm_board.dwAddrBM_mem + (dio_data->bm_board.Master.BmInp.SgList.ChangeDescAdd[dio_data->bm_board.Master.BmInp.outcnt] + (0x40 * loop) + 0xC), (BM_MEM_INP_NXT_DESC - 0x40));
												_outmd(dio_data->bm_board.dwAddrBM_mem + (dio_data->bm_board.Master.BmInp.SgList.ChangeDescAdd[dio_data->bm_board.Master.BmInp.outcnt] + (0x40 * loop) + 0x3C), 0x40004000);
												break;
											}
											tatnum	= MemData;
											loop++;
										}
										//---------------------------------------------
										// Set transfer start command to Prefetcher CSR
										//---------------------------------------------
										_outmd((MEM_DIO_INP_PRE_ADD + 0x4), (dio_data->bm_board.Master.BmInp.SgList.ChangeDescAdd[dio_data->bm_board.Master.BmInp.outcnt] + 0x40));
										_outmd((MEM_DIO_INP_PRE_ADD), 0x9);
									}
								//-----------------------------------------
								// If DMA transfer has not been performed even once
								//-----------------------------------------
								} else {
									while(loop < 10){
										MemData	+= _inmd((dio_data->bm_board.dwAddrBM_mem + 0x06000000 + (0x40 * loop) + 0x8));
										if (MemData >= dio_data->bm_board.Master.BmInp.remaingdatanum){
											_outmd((dio_data->bm_board.dwAddrBM_mem + 0x06000000 + (0x40 * loop) + 0x8), (dio_data->bm_board.Master.BmInp.remaingdatanum - tatnum));
											_outmd((dio_data->bm_board.dwAddrBM_mem + 0x06000000 + (0x40 * loop) + 0xC), (BM_MEM_INP_NXT_DESC - 0x40));
											_outmd((dio_data->bm_board.dwAddrBM_mem + 0x06000000 + (0x40 * loop) + 0x3C), 0x40004000);
											break;
										}
										tatnum	= MemData;
										loop++;
									}
									//-----------------------------------------
									// The remaining data transfer command
									//-----------------------------------------
									_outmd((MEM_DIO_INP_PRE_ADD), 0x9);
								}
							//-----------------------------------------
							// When the number of remaining data is 0 or a negative number
							//-----------------------------------------
							} else {
								//-----------------------------------------------
								// Completion process of input
								//-----------------------------------------------
								dio_data->bm_board.KeepStart			&= ~BM_DIR_IN;
								tasklet_flag = 1;
								if (dio_data->bm_board.Master.BmInp.end_ev_save != 1){
									dio_data->bm_board.Master.BmInp.endsmp	= 1;
								}
								//-----------------------------------------
								// Set SGOverIn status if there is remaining data in FIFO
								//-----------------------------------------
								if ((dio_data->bm_board.Master.BmInp.remaingdatanum > 0) && (dio_data->bm_board.Master.BmInp.remaingdatanum < 0x4000)){
									dio_data->bm_board.Master.BmInp.dwIntSence |= BM_STATUS_SG_OVER_IN;
									//-----------------------------------------
									// In case of synchronous operation and Slave side, SlaveHalt signal is output.
									//-----------------------------------------
									if (dio_data->bm_board.Master.SlaveEnable != 0){
										_outmd(MEM_DIO_PORT_ADD + 0x18, 0x8100);
										_outmd(MEM_DIO_PORT_ADD + 0x18, dio_data->bm_board.Master.SlaveEnable);
									}
									//-----------------------------------------
									// In case of synchronous operation and Master side, MasterHalt signal is output.
									//-----------------------------------------
									if (dio_data->bm_board.Master.MasterEnable != 0){
										_outmd(MEM_DIO_PORT_ADD + 0x18, 0x821F);
										_outmd(MEM_DIO_PORT_ADD + 0x18, dio_data->bm_board.Master.MasterEnable);
									}
								}
							}
						}
					}
				}
				MyInt = 2;	// BusMaster interrupt
			}
			if (((~dio_data->bm_board.SavePioMask) & IntSence) != 0x0000) {
				//-----------------------------------------------
				// Split interrupt factor
				//-----------------------------------------------
				PioInSence	= (unsigned short)(IntSence & 0xffff);
				PioOutSence	= (unsigned short)((IntSence >> 16) & 0xffff);
				//-----------------------------------------------
				// Completion process of input
				//-----------------------------------------------
				if ((PioInSence & 0x02) && (dio_data->bm_board.dwdir != DIODM_DIR_OUT)){
					MyInt = 2;	// BusMaster interrupt
				}
				//-----------------------------------------------
				// Completion process of output
				//-----------------------------------------------
				if (PioOutSence & 0x02) {
					dio_data->bm_board.KeepStart &= ~BM_DIR_OUT;
					tasklet_flag = 1;
					if (dio_data->bm_board.Master.BmOut.end_ev_save != 1){
						dio_data->bm_board.Master.BmOut.endsmp = 1;
					}
					MyInt = 2;	// BusMaster interrupt
				}
			}
		}
		if ((dio_data->bm_board.Master.BmInp.dmastopflag == 1) && ((IntSenceBM & 0x80) == 0x80)){
			//---------------------------------------------
			// Interrupt Status Clear
			//---------------------------------------------
			_outmd((MEM_DIO_PORT_ADD + 0x4C), 0x80);
			tasklet_flag = 1;
			//-----------------------------------------------
			// Completion process of input
			//-----------------------------------------------
			dio_data->bm_board.KeepStart					&= ~BM_DIR_IN;
			dio_data->bm_board.Master.BmInp.sgoverflag		= 1;
			dio_data->bm_board.Master.BmInp.dwIntSence		|= BM_STATUS_SG_OVER_IN;
			//-----------------------------------------
			// In case of synchronous operation and Slave side, SlaveHalt signal is output.
			//-----------------------------------------
			if (dio_data->bm_board.Master.SlaveEnable != 0){
				_outmd(MEM_DIO_PORT_ADD + 0x18, 0x8100);
				_outmd(MEM_DIO_PORT_ADD + 0x18, dio_data->bm_board.Master.SlaveEnable);
			}
			//-----------------------------------------
			// In case of synchronous operation and Master side, MasterHalt signal is output.
			//-----------------------------------------
			if (dio_data->bm_board.Master.MasterEnable != 0){
				_outmd(MEM_DIO_PORT_ADD + 0x18, 0x821F);
				_outmd(MEM_DIO_PORT_ADD + 0x18, dio_data->bm_board.Master.MasterEnable);
			}
		}
		//-----------------------------------------------
		// Return the interrupt sence of general-purpose input
		//-----------------------------------------------
		sence[0] = (unsigned char)((IntSence >> 12) & 0x0000000f);
		// OFF the masked bits
		sence[0] = sence[0] & ~(dio_data->bios_d.int_mask[0]);
		if (sence[0] != 0) {
			MyInt = 1;	// General-purpose input interrupt
			return MyInt;
		}
		//-----------------------------------------------
		// Master side
		//-----------------------------------------------
		lret = ApiMemBmInterrupt(&dio_data->bm_board.Master, &InStatus, &OutStatus, IntSenceBM);
		if (lret == BM_INT_EXIST) {
			MyInt = 2;	// BusMaster interrupt
		}
		if (tasklet_flag == 1) {
			tasklet_schedule(&dio_data->tasklet);
		}
	} else {
		//-----------------------------------------------
		// Local side
		// In the interrupt handling routine,
		// first process the local side and then
		// process the master side.
		//-----------------------------------------------
		//-----------------------------------------------
		// Interrupt status input and clear
		//-----------------------------------------------
		BmInpD((dio_data->bm_board.dwAddr+0x14), IntSence);
		dio_data->bm_board.PioSence |= IntSence;
		BmOutD((dio_data->bm_board.dwAddr+0x14), IntSence);
		if (((~dio_data->bm_board.PioMask) & IntSence) != 0x0000) {
			MyInt = 2;	// BusMaster interrupt
			//-----------------------------------------------
			// Split interrupt factor
			//-----------------------------------------------
			PioInSence	= (unsigned short)(IntSence & 0xffff);
			PioOutSence	= (unsigned short)((IntSence >> 16) & 0xffff);
			//-----------------------------------------------
			// Completion process of input
			//-----------------------------------------------
			if (PioInSence & 0x02) {
				dio_data->bm_board.KeepStart &= ~BM_DIR_IN;
			}
			//-----------------------------------------------
			// Completion process of output
			//-----------------------------------------------
			if (PioOutSence & 0x02) {
				dio_data->bm_board.KeepStart &= ~BM_DIR_OUT;
			}
			//-----------------------------------------------
			// Post-process of the transfer completion interrupt (local)
			//-----------------------------------------------
			Cdio_dm_trans_end_process((unsigned long)dio_data);
		}
		//-----------------------------------------------
		// Return the interrupt sence of general-purpose input
		//-----------------------------------------------
		sence[0] = (unsigned char)((IntSence >> 12) & 0x0000000f);
		// OFF the masked bits
		sence[0] = sence[0] & ~(dio_data->bios_d.int_mask[0]);
		if (sence[0] != 0) {
			MyInt = 1;	// General-purpose input interrupt
			return MyInt;
		}
		//-----------------------------------------------
		// Master side
		//-----------------------------------------------
		lret = ApiBmInterrupt(&dio_data->bm_board.Master, &InStatus, &OutStatus);
		if (lret == BM_INT_EXIST) {
			MyInt = 2;	// BusMaster interrupt
		}
	}
	return MyInt;
}


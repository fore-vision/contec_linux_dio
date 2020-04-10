////////////////////////////////////////////////////////////////////////////////
/// @file   Cdio_bios.c
/// @brief  API-DIO(LNX) PCI Module - BIOS source file
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
#include "Ccom_module.h"
#include "Cdio_dispatch.h"
#include "Cdio_bios.h"
//================================================================
// Macro definition
//================================================================
// Input/Output
#define		_inp(port)			(unsigned char)inb(port)
#define		_inpw(port)			(unsigned short)inw(port)
#define		_inpd(port)			(unsigned long)inl(port)
#define		_outp(port, data)	outb(data, port)
#define		_outpw(port, data)	outw(data, port)
#define		_outpd(port, data)	outl(data, port)

// Memory Input/Output
#define		_inm(adr)			ioread8(adr)
#define		_inmw(adr)			ioread16(adr)
#define		_inmd(adr)			ioread32(adr)
#define		_outm(adr, data)	iowrite8(data, adr)
#define		_outmw(adr, data)	iowrite16(data, adr)
#define		_outmd(adr, data)	iowrite32(data, adr)
//================================================================
// For CPS-BXC200
//================================================================
#define	BYTE	unsigned char
#define	FALSE	0
#define	SMB_ADR	0xf040

#define	SIO_ADR	0x295
#define	SIO_DAT	SIO_ADR+1

#define	SIO_CR_A	0x2e
#define	SIO_CR_D	SIO_CR_A+1

extern unsigned long GET_MSR( unsigned long );
extern void OUTPD( int, unsigned long);
extern unsigned long INPD( int );

void ShowUsage(void);
BYTE Ct_I2CReadByte(BYTE,BYTE);
void Ct_I2CWriteByte(BYTE, BYTE, BYTE);
void Ct_Chk_SMBus_Ready(void);
BYTE smb_access( int, int, BYTE, BYTE, BYTE );
void Sleep(int);

int SMBUS_PORT = 0;
BYTE DeviceID = 0;
int dwRet;

int wait_rdy( void )
{
	unsigned long	wait_count;
	
	wait_count = 0;
	while( smb_access(0, SMB_ADR, 0x44, 0, 0 ) & 3 ){
		wait_count++;
		if(wait_count > 1000000){
			return 1;
		}
	}
	return 0;
}

int wait_dir( void )
{
	unsigned long	wait_count;
	
	wait_count = 0;
	while( smb_access(0, SMB_ADR, 0x44, 0, 0 ) != 6 ){
		wait_count++;
		if(wait_count > 1000000){
			return 1;
		}
	}
	
	return 0;
}

void set_busy( void )
{
	smb_access(1,SMB_ADR, 0x44, 0, (BYTE)((smb_access(0, SMB_ADR, 0x44, 0, 0 ) & 0xfb) | 1) );
}
void clr_busy( void )
{
	smb_access(1,SMB_ADR, 0x44, 0,(BYTE)(smb_access(0, SMB_ADR, 0x44, 0, 0 ) & 0xfe) );
}

void set_drdy( void )
{
	smb_access(1,SMB_ADR, 0x44, 0,(BYTE)((smb_access(0, SMB_ADR, 0x44, 0, 0 ) & 0xfa) | 2) );
}

void clr_drdy( void )
{
	smb_access(1,SMB_ADR, 0x44, 0,(BYTE)(smb_access(0, SMB_ADR, 0x44, 0, 0 ) & 0xfd) );
}

void set_data( BYTE cnt, BYTE dat )
{
	smb_access(1,SMB_ADR, 0x44, cnt, dat );
}
BYTE get_data( BYTE cnt )
{
	return smb_access(0,SMB_ADR, 0x44, cnt, 0 );
}

void clr_stat( void )
{
	set_data(9,0);
	set_data(8,0);
	set_data(7,0);
	set_data(6,0);
	set_data(5,0);
	set_data(4,0);
	set_data(3,0);
	set_data(2,0);
	set_data(1,0);
	set_data(0,0);
}

BYTE smb_access( int rw, int port_adr, BYTE id, BYTE Index, BYTE write_value )
{
	BYTE read_value = 0;
	SMBUS_PORT 		= port_adr;
	DeviceID 		= id;

	if(rw){
		Ct_I2CWriteByte(write_value, Index, DeviceID);
	}else{
		read_value = Ct_I2CReadByte(Index, DeviceID);
	}

	return read_value;
}

void sleeps( void )
{
	int	cnt=200;
	
	while( cnt-- )
		_outp( 0xed, 0x00 );
}

void Sleep(int cnt)
{
	while( cnt-- )
		sleeps();
}

BYTE Ct_I2CReadByte(BYTE RegIndex, BYTE DeviceID)
{
	BYTE value;
	BYTE cmd_read;

	cmd_read = DeviceID + 1;

	// Write ID Command Read
	_outp(SMBUS_PORT + 0x04, cmd_read);
	Sleep(1);
	Ct_Chk_SMBus_Ready();

	// Write Index
	_outp(SMBUS_PORT + 0x03, RegIndex);
	Sleep(1);

	// Read Data
	_outp(SMBUS_PORT + 0x02, 0x48);
	Sleep(1);
	Ct_Chk_SMBus_Ready();
	value=_inp(SMBUS_PORT + 0x05);
	Sleep(1);
	return value;
}

void Ct_I2CWriteByte(BYTE OutData, BYTE RegIndex, BYTE DeviceID)
{
	// Write ID Command Write
	_outp(SMBUS_PORT + 0x04, DeviceID);
	Sleep(1);
	Ct_Chk_SMBus_Ready();

	// Write Index
	_outp(SMBUS_PORT + 0x03, RegIndex);
	Sleep(1);
	
	// Write Data
	_outp(SMBUS_PORT + 0x05, OutData);
	Sleep(1);
	_outp(SMBUS_PORT + 0x02, 0x48);
	Sleep(1);
	Ct_Chk_SMBus_Ready();
	return;
}

void Ct_Chk_SMBus_Ready()
{
	int		i = 0;
	BYTE	value = 0;
	BYTE	mask_inuse = 0xBF;
	
	for(i = 0; i < 0x400; i++){
		value=_inp(SMBUS_PORT);
		Sleep(1);
		_outp(SMBUS_PORT, value);
		if((value & mask_inuse) == 0){	// Status OK?
			return;
		}
		else if(((value & mask_inuse) & 0x04) == 0x04){	//SMBus Error
		}
	}
}
//================================================================
// External variables
//================================================================
static CDIOBI_BOARD_INFO	board_info[] = {
// H
	{// PIO-32/32L(PCI)H
	board_name			:		"PIO-32/32L(PCI)H",		// Board name
	board_type			:		CDIOBI_BT_PCI_H_SERIES,	// Board type
	device_type			:		DEVICE_TYPE_PCI,		// PCI bus
	vendor_id			:		0x1221,					// Vendor ID
	device_id			:		0x9152,					// Device ID
	resource_type		:		SRC_TYPE_PORTIO,		// Revision type (1: Not use, 2: Present in PCI configuration register, 3: Present on port map)
	port_num			:		32,						// Number of occupied ports
	inp_port_off		:		0,						// Offset of input port address
	out_port_off		:		4,						// Offset of output port address
	filter_off			:		CDIOBI_PORT_H_FILTER,	// Offset of filter address
	int_mask_off		:		CDIOBI_PORT_H_INTMASK,	// Offset of interrupt mask address
	int_sence_off		:		CDIOBI_PORT_H_INTSTS,	// Offset of interrupt sence address
	int_ctrl_off		:		CDIOBI_PORT_H_INTCTRL,	// Offset of interrupt control address
	inp_port_num		:		4,						// Number of input ports
	out_port_num		:		4,						// Number of output ports
	min_filter_value	:		0,						// Minimum value of filter
	max_filter_value	:		20,						// Maximum value of filter
	int_bit				:		{ 0xff, 0xff, 0xff, 0xff},// Interrupt bit (valid bit is 1)
	max_int_bit			:		32,						// Maximum number of interrupt bits
	num_of_8255			:		0,						// Number of 8255 chips
	can_echo_back		:		1,						// Echo back available? (0: Unavailable, 1: Available)
	is_positive			:		0						// Positive logic? (0: Negative logic, 1: Positive logic)
	},
	{// PI-64L(PCI)H
	board_name			:		"PI-64L(PCI)H",			// Board name
	board_type			:		CDIOBI_BT_PCI_H_SERIES,	// Board type
	device_type			:		DEVICE_TYPE_PCI,		// PCI bus
	vendor_id			:		0x1221,					// Vendor ID
	device_id			:		0x9162,					// Device ID
	resource_type		:		SRC_TYPE_PORTIO,		// Revision type (1: Not use, 2: Present in PCI configuration register, 3: Present on port map)
	port_num			:		32,						// Number of occupied ports
	inp_port_off		:		0,						// Offset of input port address
	out_port_off		:		0,						// Offset of output port address
	filter_off			:		CDIOBI_PORT_H_FILTER,	// Offset of filter address
	int_mask_off		:		CDIOBI_PORT_H_INTMASK,	// Offset of interrupt mask address
	int_sence_off		:		CDIOBI_PORT_H_INTSTS,	// Offset of interrupt sence address
	int_ctrl_off		:		CDIOBI_PORT_H_INTCTRL,	// Offset of interrupt control address
	inp_port_num		:		8,						// Number of input ports
	out_port_num		:		0,						// Number of output ports
	min_filter_value	:		0,						// Minimum value of filter
	max_filter_value	:		20,						// Maximum value of filter
	int_bit				:		{ 0xff, 0xff, 0xff, 0xff},// Interrupt bit (valid bit is 1)
	max_int_bit			:		32,						// Maximum number of interrupt bits
	num_of_8255			:		0,						// Number of 8255 chips
	can_echo_back		:		0,						// Echo back available? (0: Unavailable, 1: Available)
	is_positive			:		0						// Positive logic? (0: Negative logic, 1: Positive logic)
	},
	{// PO-64L(PCI)H
	board_name			:		"PO-64L(PCI)H",			// Board name
	board_type			:		CDIOBI_BT_PCI_H_SERIES,	// Board type
	device_type			:		DEVICE_TYPE_PCI,		// PCI bus
	vendor_id			:		0x1221,					// Vendor ID
	device_id			:		0x9172,					// Device ID
	resource_type		:		SRC_TYPE_PORTIO,		// Revision type (1: Not use, 2: Present in PCI configuration register, 3: Present on port map)
	port_num			:		32,						// Number of occupied ports
	inp_port_off		:		0,						// Offset of input port address
	out_port_off		:		0,						// Offset of output port address
	filter_off			:		0,						// Offset of filter address
	int_mask_off		:		0,						// Offset of interrupt mask address
	int_sence_off		:		0,						// Offset of interrupt sence address
	int_ctrl_off		:		0,						// Offset of interrupt control address
	inp_port_num		:		0,						// Number of input ports
	out_port_num		:		8,						// Number of output ports
	min_filter_value	:		0,						// Minimum value of filter
	max_filter_value	:		0,						// Maximum value of filter
	int_bit				:		{ 0x00},				// Interrupt bit (valid bit is 1)
	max_int_bit			:		0,						// Maximum number of interrupt bits
	num_of_8255			:		0,						// Number of 8255 chips
	can_echo_back		:		1,						// Echo back available? (0: Unavailable, 1: Available)
	is_positive			:		0						// Positive logic? (0: Negative logic, 1: Positive logic)
	},
	{// PIO-16/16L(PCI)H
	board_name			:		"PIO-16/16L(PCI)H",		// Board name
	board_type			:		CDIOBI_BT_PCI_H_SERIES,	// Board type
	device_type			:		DEVICE_TYPE_PCI,		// PCI bus
	vendor_id			:		0x1221,					// Vendor ID
	device_id			:		0x9182,					// Device ID
	resource_type		:		SRC_TYPE_PORTIO,		// Revision type (1: Not use, 2: Present in PCI configuration register, 3: Present on port map)
	port_num			:		32,						// Number of occupied ports
	inp_port_off		:		0,						// Offset of input port address
	out_port_off		:		2,						// Offset of output port address
	filter_off			:		CDIOBI_PORT_H_FILTER,	// Offset of filter address
	int_mask_off		:		CDIOBI_PORT_H_INTMASK,	// Offset of interrupt mask address
	int_sence_off		:		CDIOBI_PORT_H_INTSTS,	// Offset of interrupt sence address
	int_ctrl_off		:		CDIOBI_PORT_H_INTCTRL,	// Offset of interrupt control address
	inp_port_num		:		2,						// Number of input ports
	out_port_num		:		2,						// Number of output ports
	min_filter_value	:		0,						// Minimum value of filter
	max_filter_value	:		20,						// Maximum value of filter
	int_bit				:		{ 0xff, 0xff },			// Interrupt bit (valid bit is 1)
	max_int_bit			:		16,						// Maximum number of interrupt bits
	num_of_8255			:		0,						// Number of 8255 chips
	can_echo_back		:		1,						// Echo back available? (0: Unavailable, 1: Available)
	is_positive			:		0						// Positive logic? (0: Negative logic, 1: Positive logic)
	},
	{// PI-32L(PCI)H
	board_name			:		"PI-32L(PCI)H",			// Board name
	board_type			:		CDIOBI_BT_PCI_H_SERIES,	// Board type
	device_type			:		DEVICE_TYPE_PCI,		// PCI bus
	vendor_id			:		0x1221,					// Vendor ID
	device_id			:		0x9192,					// Device ID
	resource_type		:		SRC_TYPE_PORTIO,		// Revision type (1: Not use, 2: Present in PCI configuration register, 3: Present on port map)
	port_num			:		32,						// Number of occupied ports
	inp_port_off		:		0,						// Offset of input port address
	out_port_off		:		0,						// Offset of output port address
	filter_off			:		CDIOBI_PORT_H_FILTER,	// Offset of filter address
	int_mask_off		:		CDIOBI_PORT_H_INTMASK,	// Offset of interrupt mask address
	int_sence_off		:		CDIOBI_PORT_H_INTSTS,	// Offset of interrupt sence address
	int_ctrl_off		:		CDIOBI_PORT_H_INTCTRL,	// Offset of interrupt control address
	inp_port_num		:		4,						// Number of input ports
	out_port_num		:		0,						// Number of output ports
	min_filter_value	:		0,						// Minimum value of filter
	max_filter_value	:		20,						// Maximum value of filter
	int_bit				:		{ 0xff, 0xff, 0xff, 0xff},// Interrupt bit (valid bit is 1)
	max_int_bit			:		32,						// Maximum number of interrupt bits
	num_of_8255			:		0,						// Number of 8255 chips
	can_echo_back		:		0,						// Echo back available? (0: Unavailable, 1: Available)
	is_positive			:		0						// Positive logic? (0: Negative logic, 1: Positive logic)
	},
	{// PO-32L(PCI)H
	board_name			:		"PO-32L(PCI)H",			// Board name
	board_type			:		CDIOBI_BT_PCI_H_SERIES,	// Board type
	device_type			:		DEVICE_TYPE_PCI,		// PCI bus
	vendor_id			:		0x1221,					// Vendor ID
	device_id			:		0x91A2,					// Device ID
	resource_type		:		SRC_TYPE_PORTIO,		// Revision type (1: Not use, 2: Present in PCI configuration register, 3: Present on port map)
	port_num			:		32,						// Number of occupied ports
	inp_port_off		:		0,						// Offset of input port address
	out_port_off		:		0,						// Offset of output port address
	filter_off			:		0,						// Offset of filter address
	int_mask_off		:		0,						// Offset of interrupt mask address
	int_sence_off		:		0,						// Offset of interrupt sence address
	int_ctrl_off		:		0,						// Offset of interrupt control address
	inp_port_num		:		0,						// Number of input ports
	out_port_num		:		4,						// Number of output ports
	min_filter_value	:		0,						// Minimum value of filter
	max_filter_value	:		0,						// Maximum value of filter
	int_bit				:		{0x00},					// Interrupt bit (valid bit is 1)
	max_int_bit			:		0,						// Maximum number of interrupt bits
	num_of_8255			:		0,						// Number of 8255 chips
	can_echo_back		:		1,						// Echo back available? (0: Unavailable, 1: Available)
	is_positive			:		0						// Positive logic? (0: Negative logic, 1: Positive logic)
	},
	{// PIO-16/16RY(PCI)
	board_name			:		"PIO-16/16RY(PCI)",		// Board name
	board_type			:		CDIOBI_BT_PCI_H_SERIES,	// Board type
	device_type			:		DEVICE_TYPE_PCI,		// PCI bus
	vendor_id			:		0x1221,					// Vendor ID
	device_id			:		0x91b2,					// Device ID
	resource_type		:		SRC_TYPE_PORTIO,		// Revision type (1: Not use, 2: Present in PCI configuration register, 3: Present on port map)
	port_num			:		32,						// Number of occupied ports
	inp_port_off		:		0,						// Offset of input port address
	out_port_off		:		2,						// Offset of output port address
	filter_off			:		CDIOBI_PORT_H_FILTER,	// Offset of filter address
	int_mask_off		:		CDIOBI_PORT_H_INTMASK,	// Offset of interrupt mask address
	int_sence_off		:		CDIOBI_PORT_H_INTSTS,	// Offset of interrupt sence address
	int_ctrl_off		:		CDIOBI_PORT_H_INTCTRL,	// Offset of interrupt control address
	inp_port_num		:		2,						// Number of input ports
	out_port_num		:		2,						// Number of output ports
	min_filter_value	:		0,						// Minimum value of filter
	max_filter_value	:		20,						// Maximum value of filter
	int_bit				:		{ 0xff, 0xff },			// Interrupt bit (valid bit is 1)
	max_int_bit			:		16,						// Maximum number of interrupt bits
	num_of_8255			:		0,						// Number of 8255 chips
	can_echo_back		:		1,						// Echo back available? (0: Unavailable, 1: Available)
	is_positive			:		0						// Positive logic? (0: Negative logic, 1: Positive logic)
	},
	{// PIO-16/16B(PCI)H
	board_name			:		"PIO-16/16B(PCI)H",		// Board name
	board_type			:		CDIOBI_BT_PCI_H_SERIES,	// Board type
	device_type			:		DEVICE_TYPE_PCI,		// PCI bus
	vendor_id			:		0x1221,					// Vendor ID
	device_id			:		0x91C2,					// Device ID
	resource_type		:		SRC_TYPE_PORTIO,		// Revision type (1: Not use, 2: Present in PCI configuration register, 3: Present on port map)
	port_num			:		32,						// Number of occupied ports
	inp_port_off		:		0,						// Offset of input port address
	out_port_off		:		2,						// Offset of output port address
	filter_off			:		CDIOBI_PORT_H_FILTER,	// Offset of filter address
	int_mask_off		:		CDIOBI_PORT_H_INTMASK,	// Offset of interrupt mask address
	int_sence_off		:		CDIOBI_PORT_H_INTSTS,	// Offset of interrupt sence address
	int_ctrl_off		:		CDIOBI_PORT_H_INTCTRL,	// Offset of interrupt control address
	inp_port_num		:		2,						// Number of input ports
	out_port_num		:		2,						// Number of output ports
	min_filter_value	:		0,						// Minimum value of filter
	max_filter_value	:		20,						// Maximum value of filter
	int_bit				:		{ 0xff, 0xff },			// Interrupt bit (valid bit is 1)
	max_int_bit			:		16,						// Maximum number of interrupt bits
	num_of_8255			:		0,						// Number of 8255 chips
	can_echo_back		:		1,						// Echo back available? (0: Unavailable, 1: Available)
	is_positive			:		0						// Positive logic? (0: Negative logic, 1: Positive logic)
	},
	{// PI-32B(PCI)H
	board_name			:		"PI-32B(PCI)H",			// Board name
	board_type			:		CDIOBI_BT_PCI_H_SERIES,	// Board type
	device_type			:		DEVICE_TYPE_PCI,		// PCI bus
	vendor_id			:		0x1221,					// Vendor ID
	device_id			:		0x91D2,					// Device ID
	resource_type		:		SRC_TYPE_PORTIO,		// Revision type (1: Not use, 2: Present in PCI configuration register, 3: Present on port map)
	port_num			:		32,						// Number of occupied ports
	inp_port_off		:		0,						// Offset of input port address
	out_port_off		:		0,						// Offset of output port address
	filter_off			:		CDIOBI_PORT_H_FILTER,	// Offset of filter address
	int_mask_off		:		CDIOBI_PORT_H_INTMASK,	// Offset of interrupt mask address
	int_sence_off		:		CDIOBI_PORT_H_INTSTS,	// Offset of interrupt sence address
	int_ctrl_off		:		CDIOBI_PORT_H_INTCTRL,	// Offset of interrupt control address
	inp_port_num		:		4,						// Number of input ports
	out_port_num		:		0,						// Number of output ports
	min_filter_value	:		0,						// Minimum value of filter
	max_filter_value	:		20,						// Maximum value of filter
	int_bit				:		{ 0xff, 0xff,0xff, 0xff },	// Interrupt bit (valid bit is 1)
	max_int_bit			:		32,						// Maximum number of interrupt bits
	num_of_8255			:		0,						// Number of 8255 chips
	can_echo_back		:		0,						// Echo back available? (0: Unavailable, 1: Available)
	is_positive			:		0						// Positive logic? (0: Negative logic, 1: Positive logic)
	},
	{// PO-32B(PCI)H
	board_name			:		"PO-32B(PCI)H",			// Board name
	board_type			:		CDIOBI_BT_PCI_H_SERIES,	// Board type
	device_type			:		DEVICE_TYPE_PCI,		// PCI bus
	vendor_id			:		0x1221,					// Vendor ID
	device_id			:		0x91E2,					// Device ID
	resource_type		:		SRC_TYPE_PORTIO,		// Revision type (1: Not use, 2: Present in PCI configuration register, 3: Present on port map)
	port_num			:		32,						// Number of occupied ports
	inp_port_off		:		0,						// Offset of input port address
	out_port_off		:		0,						// Offset of output port address
	filter_off			:		CDIOBI_PORT_H_FILTER,	// Offset of filter address
	int_mask_off		:		CDIOBI_PORT_H_INTMASK,	// Offset of interrupt mask address
	int_sence_off		:		CDIOBI_PORT_H_INTSTS,	// Offset of interrupt sence address
	int_ctrl_off		:		CDIOBI_PORT_H_INTCTRL,	// Offset of interrupt control address
	inp_port_num		:		0,						// Number of input ports
	out_port_num		:		4,						// Number of output ports
	min_filter_value	:		0,						// Minimum value of filter
	max_filter_value	:		0,						// Maximum value of filter
	int_bit				:		{ 0x00 },				// Interrupt bit (valid bit is 1)
	max_int_bit			:		0,						// Maximum number of interrupt bits
	num_of_8255			:		0,						// Number of 8255 chips
	can_echo_back		:		1,						// Echo back available? (0: Unavailable, 1: Available)
	is_positive			:		0						// Positive logic? (0: Negative logic, 1: Positive logic)
	},
	{// PIO-16/16L(LPCI)H
	board_name			:		"PIO-16/16L(LPCI)H",	// Board name
	board_type			:		CDIOBI_BT_PCI_H_SERIES,	// Board type
	device_type			:		DEVICE_TYPE_PCI,		// PCI bus
	vendor_id			:		0x1221,					// Vendor ID
	device_id			:		0xA102,					// Device ID
	resource_type		:		SRC_TYPE_PORTIO,		// Revision type (1: Not use, 2: Present in PCI configuration register, 3: Present on port map)
	port_num			:		32,						// Number of occupied ports
	inp_port_off		:		0,						// Offset of input port address
	out_port_off		:		2,						// Offset of output port address
	filter_off			:		CDIOBI_PORT_H_FILTER,	// Offset of filter address
	int_mask_off		:		CDIOBI_PORT_H_INTMASK,	// Offset of interrupt mask address
	int_sence_off		:		CDIOBI_PORT_H_INTSTS,	// Offset of interrupt sence address
	int_ctrl_off		:		CDIOBI_PORT_H_INTCTRL,	// Offset of interrupt control address
	inp_port_num		:		2,						// Number of input ports
	out_port_num		:		2,						// Number of output ports
	min_filter_value	:		0,						// Minimum value of filter
	max_filter_value	:		20,						// Maximum value of filter
	int_bit				:		{ 0xff, 0xff },			// Interrupt bit (valid bit is 1)
	max_int_bit			:		16,						// Maximum number of interrupt bits
	num_of_8255			:		0,						// Number of 8255 chips
	can_echo_back		:		1,						// Echo back available? (0: Unavailable, 1: Available)
	is_positive			:		0						// Positive logic? (0: Negative logic, 1: Positive logic)
	},
	{// PIO-16/16B(LPCI)H
	board_name			:		"PIO-16/16B(LPCI)H",	// Board name
	board_type			:		CDIOBI_BT_PCI_H_SERIES,	// Board type
	device_type			:		DEVICE_TYPE_PCI,		// PCI bus
	vendor_id			:		0x1221,					// Vendor ID
	device_id			:		0xA112,					// Device ID
	resource_type		:		SRC_TYPE_PORTIO,		// Revision type (1: Not use, 2: Present in PCI configuration register, 3: Present on port map)
	port_num			:		32,						// Number of occupied ports
	inp_port_off		:		0,						// Offset of input port address
	out_port_off		:		2,						// Offset of output port address
	filter_off			:		CDIOBI_PORT_H_FILTER,	// Offset of filter address
	int_mask_off		:		CDIOBI_PORT_H_INTMASK,	// Offset of interrupt mask address
	int_sence_off		:		CDIOBI_PORT_H_INTSTS,	// Offset of interrupt sence address
	int_ctrl_off		:		CDIOBI_PORT_H_INTCTRL,	// Offset of interrupt control address
	inp_port_num		:		2,						// Number of input ports
	out_port_num		:		2,						// Number of output ports
	min_filter_value	:		0,						// Minimum value of filter
	max_filter_value	:		20,						// Maximum value of filter
	int_bit				:		{ 0xff, 0xff },			// Interrupt bit (valid bit is 1)
	max_int_bit			:		16,						// Maximum number of interrupt bits
	num_of_8255			:		0,						// Number of 8255 chips
	can_echo_back		:		1,						// Echo back available? (0: Unavailable, 1: Available)
	is_positive			:		0						// Positive logic? (0: Negative logic, 1: Positive logic)
	},
	{// PIO-16/16T(LPCI)H
	board_name			:		"PIO-16/16T(LPCI)H",	// Board name
	board_type			:		CDIOBI_BT_PCI_H_SERIES,	// Board type
	device_type			:		DEVICE_TYPE_PCI,		// PCI bus
	vendor_id			:		0x1221,					// Vendor ID
	device_id			:		0xA122,					// Device ID
	resource_type		:		SRC_TYPE_PORTIO,		// Revision type (1: Not use, 2: Present in PCI configuration register, 3: Present on port map)
	port_num			:		32,						// Number of occupied ports
	inp_port_off		:		0,						// Offset of input port address
	out_port_off		:		2,						// Offset of output port address
	filter_off			:		CDIOBI_PORT_H_FILTER,	// Offset of filter address
	int_mask_off		:		CDIOBI_PORT_H_INTMASK,	// Offset of interrupt mask address
	int_sence_off		:		CDIOBI_PORT_H_INTSTS,	// Offset of interrupt sence address
	int_ctrl_off		:		CDIOBI_PORT_H_INTCTRL,	// Offset of interrupt control address
	inp_port_num		:		2,						// Number of input ports
	out_port_num		:		2,						// Number of output ports
	min_filter_value	:		0,						// Minimum value of filter
	max_filter_value	:		20,						// Maximum value of filter
	int_bit				:		{ 0xff, 0xff },			// Interrupt bit (valid bit is 1)
	max_int_bit			:		16,						// Maximum number of interrupt bits
	num_of_8255			:		0,						// Number of 8255 chips
	can_echo_back		:		1,						// Echo back available? (0: Unavailable, 1: Available)
	is_positive			:		0						// Positive logic? (0: Negative logic, 1: Positive logic)
	},
	{// PIO-32/32H(PCI)H
	board_name			:		"PIO-32/32H(PCI)H",	// Board name
	board_type			:		CDIOBI_BT_PCI_H_SERIES,	// Board type
	device_type			:		DEVICE_TYPE_PCI,		// PCI bus
	vendor_id			:		0x1221,					// Vendor ID
	device_id			:		0xA142,					// Device ID
	resource_type		:		SRC_TYPE_PORTIO,		// Revision type (1: Not use, 2: Present in PCI configuration register, 3: Present on port map)
	port_num			:		32,						// Number of occupied ports
	inp_port_off		:		0,						// Offset of input port address
	out_port_off		:		4,						// Offset of output port address
	filter_off			:		CDIOBI_PORT_H_FILTER,	// Offset of filter address
	int_mask_off		:		CDIOBI_PORT_H_INTMASK,	// Offset of interrupt mask address
	int_sence_off		:		CDIOBI_PORT_H_INTSTS,	// Offset of interrupt sence address
	int_ctrl_off		:		CDIOBI_PORT_H_INTCTRL,	// Offset of interrupt control address
	inp_port_num		:		4,						// Number of input ports
	out_port_num		:		4,						// Number of output ports
	min_filter_value	:		0,						// Minimum value of filter
	max_filter_value	:		20,						// Maximum value of filter
	int_bit				:		{ 0xff, 0xff, 0xff, 0xff},// Interrupt bit (valid bit is 1)
	max_int_bit			:		32,						// Maximum number of interrupt bits
	num_of_8255			:		0,						// Number of 8255 chips
	can_echo_back		:		1,						// Echo back available? (0: Unavailable, 1: Available)
	is_positive			:		0						// Positive logic? (0: Negative logic, 1: Positive logic)
	},
	{// PIO-16/16H(PCI)H
	board_name			:		"PIO-16/16H(PCI)H",		// Board name
	board_type			:		CDIOBI_BT_PCI_H_SERIES,	// Board type
	device_type			:		DEVICE_TYPE_PCI,		// PCI bus
	vendor_id			:		0x1221,					// Vendor ID
	device_id			:		0xA132,					// Device ID
	resource_type		:		SRC_TYPE_PORTIO,		// Revision type (1: Not use, 2: Present in PCI configuration register, 3: Present on port map)
	port_num			:		32,						// Number of occupied ports
	inp_port_off		:		0,						// Offset of input port address
	out_port_off		:		2,						// Offset of output port address
	filter_off			:		CDIOBI_PORT_H_FILTER,	// Offset of filter address
	int_mask_off		:		CDIOBI_PORT_H_INTMASK,	// Offset of interrupt mask address
	int_sence_off		:		CDIOBI_PORT_H_INTSTS,	// Offset of interrupt sence address
	int_ctrl_off		:		CDIOBI_PORT_H_INTCTRL,	// Offset of interrupt control address
	inp_port_num		:		2,						// Number of input ports
	out_port_num		:		2,						// Number of output ports
	min_filter_value	:		0,						// Minimum value of filter
	max_filter_value	:		20,						// Maximum value of filter
	int_bit				:		{ 0xff, 0xff },			// Interrupt bit (valid bit is 1)
	max_int_bit			:		16,						// Maximum number of interrupt bits
	num_of_8255			:		0,						// Number of 8255 chips
	can_echo_back		:		1,						// Echo back available? (0: Unavailable, 1: Available)
	is_positive			:		0						// Positive logic? (0: Negative logic, 1: Positive logic)
	},
	{// PIO-32/32RL(PCI)H
	board_name			:		"PIO-32/32RL(PCI)H",	// Board name
	board_type			:		CDIOBI_BT_PCI_H_SERIES,	// Board type
	device_type			:		DEVICE_TYPE_PCI,		// PCI bus
	vendor_id			:		0x1221,					// Vendor ID
	device_id			:		0xA162,					// Device ID
	resource_type		:		SRC_TYPE_PORTIO,		// Revision type (1: Not use, 2: Present in PCI configuration register, 3: Present on port map)
	port_num			:		32,						// Number of occupied ports
	inp_port_off		:		0,						// Offset of input port address
	out_port_off		:		4,						// Offset of output port address
	filter_off			:		CDIOBI_PORT_H_FILTER,	// Offset of filter address
	int_mask_off		:		CDIOBI_PORT_H_INTMASK,	// Offset of interrupt mask address
	int_sence_off		:		CDIOBI_PORT_H_INTSTS,	// Offset of interrupt sence address
	int_ctrl_off		:		CDIOBI_PORT_H_INTCTRL,	// Offset of interrupt control address
	inp_port_num		:		4,						// Number of input ports
	out_port_num		:		4,						// Number of output ports
	min_filter_value	:		0,						// Minimum value of filter
	max_filter_value	:		20,						// Maximum value of filter
	int_bit				:		{ 0xff, 0xff, 0xff, 0xff},// Interrupt bit (valid bit is 1)
	max_int_bit			:		32,						// Maximum number of interrupt bits
	num_of_8255			:		0,						// Number of 8255 chips
	can_echo_back		:		1,						// Echo back available? (0: Unavailable, 1: Available)
	is_positive			:		0						// Positive logic? (0: Negative logic, 1: Positive logic)
	},
	{// PIO-16/16RL(PCI)H
	board_name			:		"PIO-16/16RL(PCI)H",	// Board name
	board_type			:		CDIOBI_BT_PCI_H_SERIES,	// Board type
	device_type			:		DEVICE_TYPE_PCI,		// PCI bus
	vendor_id			:		0x1221,					// Vendor ID
	device_id			:		0xA152,					// Device ID
	resource_type		:		SRC_TYPE_PORTIO,		// Revision type (1: Not use, 2: Present in PCI configuration register, 3: Present on port map)
	port_num			:		32,						// Number of occupied ports
	inp_port_off		:		0,						// Offset of input port address
	out_port_off		:		2,						// Offset of output port address
	filter_off			:		CDIOBI_PORT_H_FILTER,	// Offset of filter address
	int_mask_off		:		CDIOBI_PORT_H_INTMASK,	// Offset of interrupt mask address
	int_sence_off		:		CDIOBI_PORT_H_INTSTS,	// Offset of interrupt sence address
	int_ctrl_off		:		CDIOBI_PORT_H_INTCTRL,	// Offset of interrupt control address
	inp_port_num		:		2,						// Number of input ports
	out_port_num		:		2,						// Number of output ports
	min_filter_value	:		0,						// Minimum value of filter
	max_filter_value	:		20,						// Maximum value of filter
	int_bit				:		{ 0xff, 0xff },			// Interrupt bit (valid bit is 1)
	max_int_bit			:		16,						// Maximum number of interrupt bits
	num_of_8255			:		0,						// Number of 8255 chips
	can_echo_back		:		1,						// Echo back available? (0: Unavailable, 1: Available)
	is_positive			:		0						// Positive logic? (0: Negative logic, 1: Positive logic)
	},
	{// PIO-32/32T(PCI)H
	board_name			:		"PIO-32/32T(PCI)H",		// Board name
	board_type			:		CDIOBI_BT_PCI_H_SERIES,	// Board type
	device_type			:		DEVICE_TYPE_PCI,		// PCI bus
	vendor_id			:		0x1221,					// Vendor ID
	device_id			:		0xA172,					// Device ID
	resource_type		:		SRC_TYPE_PORTIO,		// Revision type (1: Not use, 2: Present in PCI configuration register, 3: Present on port map)
	port_num			:		32,						// Number of occupied ports
	inp_port_off		:		0,						// Offset of input port address
	out_port_off		:		4,						// Offset of output port address
	filter_off			:		CDIOBI_PORT_H_FILTER,	// Offset of filter address
	int_mask_off		:		CDIOBI_PORT_H_INTMASK,	// Offset of interrupt mask address
	int_sence_off		:		CDIOBI_PORT_H_INTSTS,	// Offset of interrupt sence address
	int_ctrl_off		:		CDIOBI_PORT_H_INTCTRL,	// Offset of interrupt control address
	inp_port_num		:		4,						// Number of input ports
	out_port_num		:		4,						// Number of output ports
	min_filter_value	:		0,						// Minimum value of filter
	max_filter_value	:		20,						// Maximum value of filter
	int_bit				:		{ 0xff, 0xff, 0xff, 0xff},// Interrupt bit (valid bit is 1)
	max_int_bit			:		32,						// Maximum number of interrupt bits
	num_of_8255			:		0,						// Number of 8255 chips
	can_echo_back		:		1,						// Echo back available? (0: Unavailable, 1: Available)
	is_positive			:		0						// Positive logic? (0: Negative logic, 1: Positive logic)
	},
	{// PIO-32/32B(PCI)V
	board_name			:		"PIO-32/32B(PCI)V",		// Board name
	board_type			:		CDIOBI_BT_PCI_H_SERIES,	// Board type
	device_type			:		DEVICE_TYPE_PCI,		// PCI bus
	vendor_id			:		0x1221,					// Vendor ID
	device_id			:		0xA182,					// Device ID
	resource_type		:		SRC_TYPE_PORTIO,		// Revision type (1: Not use, 2: Present in PCI configuration register, 3: Present on port map)
	port_num			:		32,						// Number of occupied ports
	inp_port_off		:		0,						// Offset of input port address
	out_port_off		:		4,						// Offset of output port address
	filter_off			:		CDIOBI_PORT_H_FILTER,	// Offset of filter address
	int_mask_off		:		CDIOBI_PORT_H_INTMASK,	// Offset of interrupt mask address
	int_sence_off		:		CDIOBI_PORT_H_INTSTS,	// Offset of interrupt sence address
	int_ctrl_off		:		CDIOBI_PORT_H_INTCTRL,	// Offset of interrupt control address
	inp_port_num		:		4,						// Number of input ports
	out_port_num		:		4,						// Number of output ports
	min_filter_value	:		0,						// Minimum value of filter
	max_filter_value	:		20,						// Maximum value of filter
	int_bit				:		{ 0xff, 0xff, 0xff, 0xff},// Interrupt bit (valid bit is 1)
	max_int_bit			:		32,						// Maximum number of interrupt bits
	num_of_8255			:		0,						// Number of 8255 chips
	can_echo_back		:		1,						// Echo back available? (0: Unavailable, 1: Available)
	is_positive			:		0						// Positive logic? (0: Negative logic, 1: Positive logic)
	},
	{// RRY-16C(PCI)H
	board_name			:		"RRY-16C(PCI)H",		// Board name
	board_type			:		CDIOBI_BT_PCI_H_SERIES,	// Board type
	device_type			:		DEVICE_TYPE_PCI,		// PCI bus
	vendor_id			:		0x1221,					// Vendor ID
	device_id			:		0xA192,					// Device ID
	resource_type		:		SRC_TYPE_PORTIO,		// Revision type (1: Not use, 2: Present in PCI configuration register, 3: Present on port map)
	port_num			:		32,						// Number of occupied ports
	inp_port_off		:		0,						// Offset of input port address
	out_port_off		:		0,						// Offset of output port address
	filter_off			:		CDIOBI_PORT_H_FILTER,	// Offset of filter address
	int_mask_off		:		CDIOBI_PORT_H_INTMASK,	// Offset of interrupt mask address
	int_sence_off		:		CDIOBI_PORT_H_INTSTS,	// Offset of interrupt sence address
	int_ctrl_off		:		CDIOBI_PORT_H_INTCTRL,	// Offset of interrupt control address
	inp_port_num		:		0,						// Number of input ports
	out_port_num		:		2,						// Number of output ports
	min_filter_value	:		0,						// Minimum value of filter
	max_filter_value	:		20,						// Maximum value of filter
	int_bit				:		{ 0x00},				// Interrupt bit (valid bit is 1)
	max_int_bit			:		16,						// Maximum number of interrupt bits
	num_of_8255			:		0,						// Number of 8255 chips
	can_echo_back		:		1,						// Echo back available? (0: Unavailable, 1: Available)
	is_positive			:		0						// Positive logic? (0: Negative logic, 1: Positive logic)
	},
	{// RRY-32(PCI)H
	board_name			:		"RRY-32(PCI)H",			// Board name
	board_type			:		CDIOBI_BT_PCI_H_SERIES,	// Board type
	device_type			:		DEVICE_TYPE_PCI,		// PCI bus
	vendor_id			:		0x1221,					// Vendor ID
	device_id			:		0xA1A2,					// Device ID
	resource_type		:		SRC_TYPE_PORTIO,		// Revision type (1: Not use, 2: Present in PCI configuration register, 3: Present on port map)
	port_num			:		32,						// Number of occupied ports
	inp_port_off		:		0,						// Offset of input port address
	out_port_off		:		0,						// Offset of output port address
	filter_off			:		CDIOBI_PORT_H_FILTER,	// Offset of filter address
	int_mask_off		:		CDIOBI_PORT_H_INTMASK,	// Offset of interrupt mask address
	int_sence_off		:		CDIOBI_PORT_H_INTSTS,	// Offset of interrupt sence address
	int_ctrl_off		:		CDIOBI_PORT_H_INTCTRL,	// Offset of interrupt control address
	inp_port_num		:		0,						// Number of input ports
	out_port_num		:		4,						// Number of output ports
	min_filter_value	:		0,						// Minimum value of filter
	max_filter_value	:		20,						// Maximum value of filter
	int_bit				:		{ 0x00},				// Interrupt bit (valid bit is 1)
	max_int_bit			:		32,						// Maximum number of interrupt bits
	num_of_8255			:		0,						// Number of 8255 chips
	can_echo_back		:		1,						// Echo back available? (0: Unavailable, 1: Available)
	is_positive			:		0						// Positive logic? (0: Negative logic, 1: Positive logic)
	},
	{// PIO-32/32F(PCI)H
	board_name			:		"PIO-32/32F(PCI)H",		// Board name
	board_type			:		CDIOBI_BT_PCI_H_SERIES,	// Board type
	device_type			:		DEVICE_TYPE_PCI,		// PCI bus
	vendor_id			:		0x1221,					// Vendor ID
	device_id			:		0xA1F2,					// Device ID
	resource_type		:		SRC_TYPE_PORTIO,		// Revision type (1: Not use, 2: Present in PCI configuration register, 3: Present on port map)
	port_num			:		32,						// Number of occupied ports
	inp_port_off		:		0,						// Offset of input port address
	out_port_off		:		4,						// Offset of output port address
	filter_off			:		CDIOBI_PORT_H_FILTER,	// Offset of filter address
	int_mask_off		:		CDIOBI_PORT_H_INTMASK,	// Offset of interrupt mask address
	int_sence_off		:		CDIOBI_PORT_H_INTSTS,	// Offset of interrupt sence address
	int_ctrl_off		:		CDIOBI_PORT_H_INTCTRL,	// Offset of interrupt control address
	inp_port_num		:		4,						// Number of input ports
	out_port_num		:		4,						// Number of output ports
	min_filter_value	:		0,						// Minimum value of filter
	max_filter_value	:		20,						// Maximum value of filter
	int_bit				:		{ 0xff, 0xff, 0xff, 0xff},// Interrupt bit (valid bit is 1)
	max_int_bit			:		32,						// Maximum number of interrupt bits
	num_of_8255			:		0,						// Number of 8255 chips
	can_echo_back		:		1,						// Echo back available? (0: Unavailable, 1: Available)
	is_positive			:		0						// Positive logic? (0: Negative logic, 1: Positive logic)
	},
	{// PIO-16/16T(PCI)H
	board_name			:		"PIO-16/16T(PCI)H",		// Board name
	board_type			:		CDIOBI_BT_PCI_H_SERIES,	// Board type
	device_type			:		DEVICE_TYPE_PCI,		// PCI bus
	vendor_id			:		0x1221,					// Vendor ID
	device_id			:		0xB102,					// Device ID
	resource_type		:		SRC_TYPE_PORTIO,		// Revision type (1: Not use, 2: Present in PCI configuration register, 3: Present on port map)
	port_num			:		32,						// Number of occupied ports
	inp_port_off		:		0,						// Offset of input port address
	out_port_off		:		2,						// Offset of output port address
	filter_off			:		CDIOBI_PORT_H_FILTER,	// Offset of filter address
	int_mask_off		:		CDIOBI_PORT_H_INTMASK,	// Offset of interrupt mask address
	int_sence_off		:		CDIOBI_PORT_H_INTSTS,	// Offset of interrupt sence address
	int_ctrl_off		:		CDIOBI_PORT_H_INTCTRL,	// Offset of interrupt control address
	inp_port_num		:		2,						// Number of input ports
	out_port_num		:		2,						// Number of output ports
	min_filter_value	:		0,						// Minimum value of filter
	max_filter_value	:		20,						// Maximum value of filter
	int_bit				:		{ 0xff, 0xff},			// Interrupt bit (valid bit is 1)
	max_int_bit			:		16,						// Maximum number of interrupt bits
	num_of_8255			:		0,						// Number of 8255 chips
	can_echo_back		:		1,						// Echo back available? (0: Unavailable, 1: Available)
	is_positive			:		0						// Positive logic? (0: Negative logic, 1: Positive logic)
	},
	{// PIO-16/16TB(PCI)H
	board_name			:		"PIO-16/16TB(PCI)H",	// Board name
	board_type			:		CDIOBI_BT_PCI_H_SERIES,	// Board type
	device_type			:		DEVICE_TYPE_PCI,		// PCI bus
	vendor_id			:		0x1221,					// Vendor ID
	device_id			:		0xB112,					// Device ID
	resource_type		:		SRC_TYPE_PORTIO,		// Revision type (1: Not use, 2: Present in PCI configuration register, 3: Present on port map)
	port_num			:		32,						// Number of occupied ports
	inp_port_off		:		0,						// Offset of input port address
	out_port_off		:		2,						// Offset of output port address
	filter_off			:		CDIOBI_PORT_H_FILTER,	// Offset of filter address
	int_mask_off		:		CDIOBI_PORT_H_INTMASK,	// Offset of interrupt mask address
	int_sence_off		:		CDIOBI_PORT_H_INTSTS,	// Offset of interrupt sence address
	int_ctrl_off		:		CDIOBI_PORT_H_INTCTRL,	// Offset of interrupt control address
	inp_port_num		:		2,						// Number of input ports
	out_port_num		:		2,						// Number of output ports
	min_filter_value	:		0,						// Minimum value of filter
	max_filter_value	:		20,						// Maximum value of filter
	int_bit				:		{ 0xff, 0xff},			// Interrupt bit (valid bit is 1)
	max_int_bit			:		16,						// Maximum number of interrupt bits
	num_of_8255			:		0,						// Number of 8255 chips
	can_echo_back		:		1,						// Echo back available? (0: Unavailable, 1: Available)
	is_positive			:		0						// Positive logic? (0: Negative logic, 1: Positive logic)
	},
	{// DIO-1616T-LPE
	board_name			:		"DIO-1616T-LPE",		// Board name
	board_type			:		CDIOBI_BT_PCI_H_SERIES,	// Board type
	device_type			:		DEVICE_TYPE_PCI,		// PCI bus
	vendor_id			:		0x1221,					// Vendor ID
	device_id			:		0x8612,					// Device ID
	resource_type		:		SRC_TYPE_PORTIO,		// Revision type (1: Not use, 2: Present in PCI configuration register, 3: Present on port map)
	port_num			:		32,						// Number of occupied ports
	inp_port_off		:		0,						// Offset of input port address
	out_port_off		:		2,						// Offset of output port address
	filter_off			:		CDIOBI_PORT_H_FILTER,	// Offset of filter address
	int_mask_off		:		CDIOBI_PORT_H_INTMASK,	// Offset of interrupt mask address
	int_sence_off		:		CDIOBI_PORT_H_INTSTS,	// Offset of interrupt sence address
	int_ctrl_off		:		CDIOBI_PORT_H_INTCTRL,	// Offset of interrupt control address
	inp_port_num		:		2,						// Number of input ports
	out_port_num		:		2,						// Number of output ports
	min_filter_value	:		0,						// Minimum value of filter
	max_filter_value	:		20,						// Maximum value of filter
	int_bit				:		{ 0xff, 0xff},			// Interrupt bit (valid bit is 1)
	max_int_bit			:		16,						// Maximum number of interrupt bits
	num_of_8255			:		0,						// Number of 8255 chips
	can_echo_back		:		1,						// Echo back available? (0: Unavailable, 1: Available)
	is_positive			:		0						// Positive logic? (0: Negative logic, 1: Positive logic)
	},
	{// DIO-3232L-PE
	board_name			:		"DIO-3232L-PE",			// Board name
	board_type			:		CDIOBI_BT_PCI_H_SERIES,	// Board type
	device_type			:		DEVICE_TYPE_PCI,		// PCI bus
	vendor_id			:		0x1221,					// Vendor ID
	device_id			:		0x8622,					// Device ID
	resource_type		:		SRC_TYPE_PORTIO,		// Revision type (1: Not use, 2: Present in PCI configuration register, 3: Present on port map)
	port_num			:		32,						// Number of occupied ports
	inp_port_off		:		0,						// Offset of input port address
	out_port_off		:		4,						// Offset of output port address
	filter_off			:		CDIOBI_PORT_H_FILTER,	// Offset of filter address
	int_mask_off		:		CDIOBI_PORT_H_INTMASK,	// Offset of interrupt mask address
	int_sence_off		:		CDIOBI_PORT_H_INTSTS,	// Offset of interrupt sence address
	int_ctrl_off		:		CDIOBI_PORT_H_INTCTRL,	// Offset of interrupt control address
	inp_port_num		:		4,						// Number of input ports
	out_port_num		:		4,						// Number of output ports
	min_filter_value	:		0,						// Minimum value of filter
	max_filter_value	:		20,						// Maximum value of filter
	int_bit				:		{ 0xff, 0xff, 0xff, 0xff},// Interrupt bit (valid bit is 1)
	max_int_bit			:		32,						// Maximum number of interrupt bits
	num_of_8255			:		0,						// Number of 8255 chips
	can_echo_back		:		1,						// Echo back available? (0: Unavailable, 1: Available)
	is_positive			:		0						// Positive logic? (0: Negative logic, 1: Positive logic)
	},
	{// DIO-1616L-PE
	board_name			:		"DIO-1616L-PE",			// Board name
	board_type			:		CDIOBI_BT_PCI_H_SERIES,	// Board type
	device_type			:		DEVICE_TYPE_PCI,		// PCI bus
	vendor_id			:		0x1221,					// Vendor ID
	device_id			:		0x8632,					// Device ID
	resource_type		:		SRC_TYPE_PORTIO,		// Revision type (1: Not use, 2: Present in PCI configuration register, 3: Present on port map)
	port_num			:		32,						// Number of occupied ports
	inp_port_off		:		0,						// Offset of input port address
	out_port_off		:		2,						// Offset of output port address
	filter_off			:		CDIOBI_PORT_H_FILTER,	// Offset of filter address
	int_mask_off		:		CDIOBI_PORT_H_INTMASK,	// Offset of interrupt mask address
	int_sence_off		:		CDIOBI_PORT_H_INTSTS,	// Offset of interrupt sence address
	int_ctrl_off		:		CDIOBI_PORT_H_INTCTRL,	// Offset of interrupt control address
	inp_port_num		:		2,						// Number of input ports
	out_port_num		:		2,						// Number of output ports
	min_filter_value	:		0,						// Minimum value of filter
	max_filter_value	:		20,						// Maximum value of filter
	int_bit				:		{ 0xff, 0xff},			// Interrupt bit (valid bit is 1)
	max_int_bit			:		16,						// Maximum number of interrupt bits
	num_of_8255			:		0,						// Number of 8255 chips
	can_echo_back		:		1,						// Echo back available? (0: Unavailable, 1: Available)
	is_positive			:		0						// Positive logic? (0: Negative logic, 1: Positive logic)
	},
	{// DIO-1616L-LPE
	board_name			:		"DIO-1616L-LPE",			// Board name
	board_type			:		CDIOBI_BT_PCI_H_SERIES,	// Board type
	device_type			:		DEVICE_TYPE_PCI,		// PCI bus
	vendor_id			:		0x1221,					// Vendor ID
	device_id			:		0x8632,					// Device ID
	resource_type		:		SRC_TYPE_PORTIO,		// Revision type (1: Not use, 2: Present in PCI configuration register, 3: Present on port map)
	port_num			:		32,						// Number of occupied ports
	inp_port_off		:		0,						// Offset of input port address
	out_port_off		:		2,						// Offset of output port address
	filter_off			:		CDIOBI_PORT_H_FILTER,	// Offset of filter address
	int_mask_off		:		CDIOBI_PORT_H_INTMASK,	// Offset of interrupt mask address
	int_sence_off		:		CDIOBI_PORT_H_INTSTS,	// Offset of interrupt sence address
	int_ctrl_off		:		CDIOBI_PORT_H_INTCTRL,	// Offset of interrupt control address
	inp_port_num		:		2,						// Number of input ports
	out_port_num		:		2,						// Number of output ports
	min_filter_value	:		0,						// Minimum value of filter
	max_filter_value	:		20,						// Maximum value of filter
	int_bit				:		{ 0xff, 0xff},			// Interrupt bit (valid bit is 1)
	max_int_bit			:		16,						// Maximum number of interrupt bits
	num_of_8255			:		0,						// Number of 8255 chips
	can_echo_back		:		1,						// Echo back available? (0: Unavailable, 1: Available)
	is_positive			:		0						// Positive logic? (0: Negative logic, 1: Positive logic)
	},
	{// PIO-16/16L(CB)H
	board_name			:		"PIO-16/16L(CB)H",		// Board name
	board_type			:		CDIOBI_BT_PCI_H_SERIES,	// Board type
	device_type			:		DEVICE_TYPE_PCI,		// PCI bus
	vendor_id			:		0x1221,					// Vendor ID
	device_id			:		0x8502,					// Device ID
	resource_type		:		SRC_TYPE_PORTIO,		// Revision type (1: Not use, 2: Present in PCI configuration register, 3: Present on port map)
	port_num			:		32,						// Number of occupied ports
	inp_port_off		:		0,						// Offset of input port address
	out_port_off		:		2,						// Offset of output port address
	filter_off			:		CDIOBI_PORT_H_FILTER,	// Offset of filter address
	int_mask_off		:		CDIOBI_PORT_H_INTMASK,	// Offset of interrupt mask address
	int_sence_off		:		CDIOBI_PORT_H_INTSTS,	// Offset of interrupt sence address
	int_ctrl_off		:		CDIOBI_PORT_H_INTCTRL,	// Offset of interrupt control address
	inp_port_num		:		2,						// Number of input ports
	out_port_num		:		2,						// Number of output ports
	min_filter_value	:		0,						// Minimum value of filter
	max_filter_value	:		20,						// Maximum value of filter
	int_bit				:		{ 0xff, 0xff },			// Interrupt bit (valid bit is 1)
	max_int_bit			:		16,						// Maximum number of interrupt bits
	num_of_8255			:		0,						// Number of 8255 chips
	can_echo_back		:		1,						// Echo back available? (0: Unavailable, 1: Available)
	is_positive			:		0						// Positive logic? (0: Negative logic, 1: Positive logic)
	},
	{// DIO-3232T-PE
	board_name			:		"DIO-3232T-PE",			// Board name
	board_type			:		CDIOBI_BT_PCI_H_SERIES,	// Board type
	device_type			:		DEVICE_TYPE_PCI,		// PCI bus
	vendor_id			:		0x1221,					// Vendor ID
	device_id			:		0x8642,					// Device ID
	resource_type		:		SRC_TYPE_PORTIO,		// Revision type (1: Not use, 2: Present in PCI configuration register, 3: Present on port map)
	port_num			:		32,						// Number of occupied ports
	inp_port_off		:		0,						// Offset of input port address
	out_port_off		:		4,						// Offset of output port address
	filter_off			:		CDIOBI_PORT_H_FILTER,	// Offset of filter address
	int_mask_off		:		CDIOBI_PORT_H_INTMASK,	// Offset of interrupt mask address
	int_sence_off		:		CDIOBI_PORT_H_INTSTS,	// Offset of interrupt sence address
	int_ctrl_off		:		CDIOBI_PORT_H_INTCTRL,	// Offset of interrupt control address
	inp_port_num		:		4,						// Number of input ports
	out_port_num		:		4,						// Number of output ports
	min_filter_value	:		0,						// Minimum value of filter
	max_filter_value	:		20,						// Maximum value of filter
	int_bit				:		{ 0xff, 0xff, 0xff, 0xff},// Interrupt bit (valid bit is 1)
	max_int_bit			:		32,						// Maximum number of interrupt bits
	num_of_8255			:		0,						// Number of 8255 chips
	can_echo_back		:		1,						// Echo back available? (0: Unavailable, 1: Available)
	is_positive			:		0						// Positive logic? (0: Negative logic, 1: Positive logic)
	},
	{// DIO-1616T-PE
	board_name			:		"DIO-1616T-PE",			// Board name
	board_type			:		CDIOBI_BT_PCI_H_SERIES,	// Board type
	device_type			:		DEVICE_TYPE_PCI,		// PCI bus
	vendor_id			:		0x1221,					// Vendor ID
	device_id			:		0x8652,					// Device ID
	resource_type		:		SRC_TYPE_PORTIO,		// Revision type (1: Not use, 2: Present in PCI configuration register, 3: Present on port map)
	port_num			:		32,						// Number of occupied ports
	inp_port_off		:		0,						// Offset of input port address
	out_port_off		:		2,						// Offset of output port address
	filter_off			:		CDIOBI_PORT_H_FILTER,	// Offset of filter address
	int_mask_off		:		CDIOBI_PORT_H_INTMASK,	// Offset of interrupt mask address
	int_sence_off		:		CDIOBI_PORT_H_INTSTS,	// Offset of interrupt sence address
	int_ctrl_off		:		CDIOBI_PORT_H_INTCTRL,	// Offset of interrupt control address
	inp_port_num		:		2,						// Number of input ports
	out_port_num		:		2,						// Number of output ports
	min_filter_value	:		0,						// Minimum value of filter
	max_filter_value	:		20,						// Maximum value of filter
	int_bit				:		{ 0xff, 0xff},			// Interrupt bit (valid bit is 1)
	max_int_bit			:		16,						// Maximum number of interrupt bits
	num_of_8255			:		0,						// Number of 8255 chips
	can_echo_back		:		1,						// Echo back available? (0: Unavailable, 1: Available)
	is_positive			:		0						// Positive logic? (0: Negative logic, 1: Positive logic)
	},
	{// DIO-3232B-PE
	board_name			:		"DIO-3232B-PE",			// Board name
	board_type			:		CDIOBI_BT_PCI_H_SERIES,	// Board type
	device_type			:		DEVICE_TYPE_PCI,		// PCI bus
	vendor_id			:		0x1221,					// Vendor ID
	device_id			:		0x8662,					// Device ID
	resource_type		:		SRC_TYPE_PORTIO,		// Revision type (1: Not use, 2: Present in PCI configuration register, 3: Present on port map)
	port_num			:		32,						// Number of occupied ports
	inp_port_off		:		0,						// Offset of input port address
	out_port_off		:		4,						// Offset of output port address
	filter_off			:		CDIOBI_PORT_H_FILTER,	// Offset of filter address
	int_mask_off		:		CDIOBI_PORT_H_INTMASK,	// Offset of interrupt mask address
	int_sence_off		:		CDIOBI_PORT_H_INTSTS,	// Offset of interrupt sence address
	int_ctrl_off		:		CDIOBI_PORT_H_INTCTRL,	// Offset of interrupt control address
	inp_port_num		:		4,						// Number of input ports
	out_port_num		:		4,						// Number of output ports
	min_filter_value	:		0,						// Minimum value of filter
	max_filter_value	:		20,						// Maximum value of filter
	int_bit				:		{ 0xff, 0xff, 0xff, 0xff},// Interrupt bit (valid bit is 1)
	max_int_bit			:		32,						// Maximum number of interrupt bits
	num_of_8255			:		0,						// Number of 8255 chips
	can_echo_back		:		1,						// Echo back available? (0: Unavailable, 1: Available)
	is_positive			:		0						// Positive logic? (0: Negative logic, 1: Positive logic)
	},
	{// DIO-3232F-PE
	board_name			:		"DIO-3232F-PE",			// Board name
	board_type			:		CDIOBI_BT_PCI_H_SERIES,	// Board type
	device_type			:		DEVICE_TYPE_PCI,		// PCI bus
	vendor_id			:		0x1221,					// Vendor ID
	device_id			:		0x86F2,					// Device ID
	resource_type		:		SRC_TYPE_PORTIO,		// Revision type (1: Not use, 2: Present in PCI configuration register, 3: Present on port map)
	port_num			:		32,						// Number of occupied ports
	inp_port_off		:		0,						// Offset of input port address
	out_port_off		:		4,						// Offset of output port address
	filter_off			:		CDIOBI_PORT_H_FILTER,	// Offset of filter address
	int_mask_off		:		CDIOBI_PORT_H_INTMASK,	// Offset of interrupt mask address
	int_sence_off		:		CDIOBI_PORT_H_INTSTS,	// Offset of interrupt sence address
	int_ctrl_off		:		CDIOBI_PORT_H_INTCTRL,	// Offset of interrupt control address
	inp_port_num		:		4,						// Number of input ports
	out_port_num		:		4,						// Number of output ports
	min_filter_value	:		0,						// Minimum value of filter
	max_filter_value	:		20,						// Maximum value of filter
	int_bit				:		{ 0xff, 0xff, 0xff, 0xff},// Interrupt bit (valid bit is 1)
	max_int_bit			:		32,						// Maximum number of interrupt bits
	num_of_8255			:		0,						// Number of 8255 chips
	can_echo_back		:		1,						// Echo back available? (0: Unavailable, 1: Available)
	is_positive			:		0						// Positive logic? (0: Negative logic, 1: Positive logic)
	},
	{// DIO-1616B-PE
	board_name			:		"DIO-1616B-PE",			// Board name
	board_type			:		CDIOBI_BT_PCI_H_SERIES,	// Board type
	device_type			:		DEVICE_TYPE_PCI,		// PCI bus
	vendor_id			:		0x1221,					// Vendor ID
	device_id			:		0x8672,					// Device ID
	resource_type		:		SRC_TYPE_PORTIO,		// Revision type (1: Not use, 2: Present in PCI configuration register, 3: Present on port map)
	port_num			:		32,						// Number of occupied ports
	inp_port_off		:		0,						// Offset of input port address
	out_port_off		:		2,						// Offset of output port address
	filter_off			:		CDIOBI_PORT_H_FILTER,	// Offset of filter address
	int_mask_off		:		CDIOBI_PORT_H_INTMASK,	// Offset of interrupt mask address
	int_sence_off		:		CDIOBI_PORT_H_INTSTS,	// Offset of interrupt sence address
	int_ctrl_off		:		CDIOBI_PORT_H_INTCTRL,	// Offset of interrupt control address
	inp_port_num		:		2,						// Number of input ports
	out_port_num		:		2,						// Number of output ports
	min_filter_value	:		0,						// Minimum value of filter
	max_filter_value	:		20,						// Maximum value of filter
	int_bit				:		{ 0xff, 0xff },			// Interrupt bit (valid bit is 1)
	max_int_bit			:		16,						// Maximum number of interrupt bits
	num_of_8255			:		0,						// Number of 8255 chips
	can_echo_back		:		1,						// Echo back available? (0: Unavailable, 1: Available)
	is_positive			:		0						// Positive logic? (0: Negative logic, 1: Positive logic)
	},
	{// DIO-1616B-LPE
	board_name			:		"DIO-1616B-LPE",			// Board name
	board_type			:		CDIOBI_BT_PCI_H_SERIES,	// Board type
	device_type			:		DEVICE_TYPE_PCI,		// PCI bus
	vendor_id			:		0x1221,					// Vendor ID
	device_id			:		0x8672,					// Device ID
	resource_type		:		SRC_TYPE_PORTIO,		// Revision type (1: Not use, 2: Present in PCI configuration register, 3: Present on port map)
	port_num			:		32,						// Number of occupied ports
	inp_port_off		:		0,						// Offset of input port address
	out_port_off		:		2,						// Offset of output port address
	filter_off			:		CDIOBI_PORT_H_FILTER,	// Offset of filter address
	int_mask_off		:		CDIOBI_PORT_H_INTMASK,	// Offset of interrupt mask address
	int_sence_off		:		CDIOBI_PORT_H_INTSTS,	// Offset of interrupt sence address
	int_ctrl_off		:		CDIOBI_PORT_H_INTCTRL,	// Offset of interrupt control address
	inp_port_num		:		2,						// Number of input ports
	out_port_num		:		2,						// Number of output ports
	min_filter_value	:		0,						// Minimum value of filter
	max_filter_value	:		20,						// Maximum value of filter
	int_bit				:		{ 0xff, 0xff },			// Interrupt bit (valid bit is 1)
	max_int_bit			:		16,						// Maximum number of interrupt bits
	num_of_8255			:		0,						// Number of 8255 chips
	can_echo_back		:		1,						// Echo back available? (0: Unavailable, 1: Available)
	is_positive			:		0						// Positive logic? (0: Negative logic, 1: Positive logic)
	},
	{// DIO-1616TB-PE
	board_name			:		"DIO-1616TB-PE",		// Board name
	board_type			:		CDIOBI_BT_PCI_H_SERIES,	// Board type
	device_type			:		DEVICE_TYPE_PCI,		// PCI bus
	vendor_id			:		0x1221,					// Vendor ID
	device_id			:		0x9602,					// Device ID
	resource_type		:		SRC_TYPE_PORTIO,		// Revision type (1: Not use, 2: Present in PCI configuration register, 3: Present on port map)
	port_num			:		32,						// Number of occupied ports
	inp_port_off		:		0,						// Offset of input port address
	out_port_off		:		2,						// Offset of output port address
	filter_off			:		CDIOBI_PORT_H_FILTER,	// Offset of filter address
	int_mask_off		:		CDIOBI_PORT_H_INTMASK,	// Offset of interrupt mask address
	int_sence_off		:		CDIOBI_PORT_H_INTSTS,	// Offset of interrupt sence address
	int_ctrl_off		:		CDIOBI_PORT_H_INTCTRL,	// Offset of interrupt control address
	inp_port_num		:		2,						// Number of input ports
	out_port_num		:		2,						// Number of output ports
	min_filter_value	:		0,						// Minimum value of filter
	max_filter_value	:		20,						// Maximum value of filter
	int_bit				:		{ 0xff, 0xff },			// Interrupt bit (valid bit is 1)
	max_int_bit			:		16,						// Maximum number of interrupt bits
	num_of_8255			:		0,						// Number of 8255 chips
	can_echo_back		:		1,						// Echo back available? (0: Unavailable, 1: Available)
	is_positive			:		0						// Positive logic? (0: Negative logic, 1: Positive logic)
	},
	{// DI-64L-PE
	board_name			:		"DI-64L-PE",			// Board name
	board_type			:		CDIOBI_BT_PCI_H_SERIES,	// Board type
	device_type			:		DEVICE_TYPE_PCI,		// PCI bus
	vendor_id			:		0x1221,					// Vendor ID
	device_id			:		0x86B2,					// Device ID
	resource_type		:		SRC_TYPE_PORTIO,		// Revision type (1: Not use, 2: Present in PCI configuration register, 3: Present on port map)
	port_num			:		32,						// Number of occupied ports
	inp_port_off		:		0,						// Offset of input port address
	out_port_off		:		0,						// Offset of output port address
	filter_off			:		CDIOBI_PORT_H_FILTER,	// Offset of filter address
	int_mask_off		:		CDIOBI_PORT_H_INTMASK,	// Offset of interrupt mask address
	int_sence_off		:		CDIOBI_PORT_H_INTSTS,	// Offset of interrupt sence address
	int_ctrl_off		:		CDIOBI_PORT_H_INTCTRL,	// Offset of interrupt control address
	inp_port_num		:		8,						// Number of input ports
	out_port_num		:		0,						// Number of output ports
	min_filter_value	:		0,						// Minimum value of filter
	max_filter_value	:		20,						// Maximum value of filter
	int_bit				:		{ 0xff, 0xff, 0xff, 0xff},// Interrupt bit (valid bit is 1)
	max_int_bit			:		32,						// Maximum number of interrupt bits
	num_of_8255			:		0,						// Number of 8255 chips
	can_echo_back		:		0,						// Echo back available? (0: Unavailable, 1: Available)
	is_positive			:		0						// Positive logic? (0: Negative logic, 1: Positive logic)
	},
	{// DI-32L-PE
	board_name			:		"DI-32L-PE",			// Board name
	board_type			:		CDIOBI_BT_PCI_H_SERIES,	// Board type
	device_type			:		DEVICE_TYPE_PCI,		// PCI bus
	vendor_id			:		0x1221,					// Vendor ID
	device_id			:		0x86D2,					// Device ID
	resource_type		:		SRC_TYPE_PORTIO,		// Revision type (1: Not use, 2: Present in PCI configuration register, 3: Present on port map)
	port_num			:		32,						// Number of occupied ports
	inp_port_off		:		0,						// Offset of input port address
	out_port_off		:		0,						// Offset of output port address
	filter_off			:		CDIOBI_PORT_H_FILTER,	// Offset of filter address
	int_mask_off		:		CDIOBI_PORT_H_INTMASK,	// Offset of interrupt mask address
	int_sence_off		:		CDIOBI_PORT_H_INTSTS,	// Offset of interrupt sence address
	int_ctrl_off		:		CDIOBI_PORT_H_INTCTRL,	// Offset of interrupt control address
	inp_port_num		:		4,						// Number of input ports
	out_port_num		:		0,						// Number of output ports
	min_filter_value	:		0,						// Minimum value of filter
	max_filter_value	:		20,						// Maximum value of filter
	int_bit				:		{ 0xff, 0xff, 0xff, 0xff},// Interrupt bit (valid bit is 1)
	max_int_bit			:		32,						// Maximum number of interrupt bits
	num_of_8255			:		0,						// Number of 8255 chips
	can_echo_back		:		0,						// Echo back available? (0: Unavailable, 1: Available)
	is_positive			:		0						// Positive logic? (0: Negative logic, 1: Positive logic)
	},
	{// DO-64L-PE
	board_name			:		"DO-64L-PE",			// Board name
	board_type			:		CDIOBI_BT_PCI_H_SERIES,	// Board type
	device_type			:		DEVICE_TYPE_PCI,		// PCI bus
	vendor_id			:		0x1221,					// Vendor ID
	device_id			:		0x86C2,					// Device ID
	resource_type		:		SRC_TYPE_PORTIO,		// Revision type (1: Not use, 2: Present in PCI configuration register, 3: Present on port map)
	port_num			:		32,						// Number of occupied ports
	inp_port_off		:		0,						// Offset of input port address
	out_port_off		:		0,						// Offset of output port address
	filter_off			:		0,						// Offset of filter address
	int_mask_off		:		0,						// Offset of interrupt mask address
	int_sence_off		:		0,						// Offset of interrupt sence address
	int_ctrl_off		:		0,						// Offset of interrupt control address
	inp_port_num		:		0,						// Number of input ports
	out_port_num		:		8,						// Number of output ports
	min_filter_value	:		0,						// Minimum value of filter
	max_filter_value	:		0,						// Maximum value of filter
	int_bit				:		{ 0x00},				// Interrupt bit (valid bit is 1)
	max_int_bit			:		0,						// Maximum number of interrupt bits
	num_of_8255			:		0,						// Number of 8255 chips
	can_echo_back		:		1,						// Echo back available? (0: Unavailable, 1: Available)
	is_positive			:		0						// Positive logic? (0: Negative logic, 1: Positive logic)
	},
	{// DO-32L-PE
	board_name			:		"DO-32L-PE",			// Board name
	board_type			:		CDIOBI_BT_PCI_H_SERIES,	// Board type
	device_type			:		DEVICE_TYPE_PCI,		// PCI bus
	vendor_id			:		0x1221,					// Vendor ID
	device_id			:		0x86E2,					// Device ID
	resource_type		:		SRC_TYPE_PORTIO,		// Revision type (1: Not use, 2: Present in PCI configuration register, 3: Present on port map)
	port_num			:		32,						// Number of occupied ports
	inp_port_off		:		0,						// Offset of input port address
	out_port_off		:		0,						// Offset of output port address
	filter_off			:		0,						// Offset of filter address
	int_mask_off		:		0,						// Offset of interrupt mask address
	int_sence_off		:		0,						// Offset of interrupt sence address
	int_ctrl_off		:		0,						// Offset of interrupt control address
	inp_port_num		:		0,						// Number of input ports
	out_port_num		:		4,						// Number of output ports
	min_filter_value	:		0,						// Minimum value of filter
	max_filter_value	:		0,						// Maximum value of filter
	int_bit				:		{0x00},					// Interrupt bit (valid bit is 1)
	max_int_bit			:		0,						// Maximum number of interrupt bits
	num_of_8255			:		0,						// Number of 8255 chips
	can_echo_back		:		1,						// Echo back available? (0: Unavailable, 1: Available)
	is_positive			:		0						// Positive logic? (0: Negative logic, 1: Positive logic)
	},
	{// DI-32T2-PCI
	board_name			:		"DI-32T2-PCI",			// Board name
	board_type			:		CDIOBI_BT_PCI_H_SERIES,	// Board type
	device_type			:		DEVICE_TYPE_PCI,		// PCI bus
	vendor_id			:		0x1221,					// Vendor ID
	device_id			:		0xB162,					// Device ID
	resource_type		:		SRC_TYPE_PORTIO,		// Revision type (1: Not use, 2: Present in PCI configuration register, 3: Present on port map)
	port_num			:		32,						// Number of occupied ports
	inp_port_off		:		0,						// Offset of input port address
	out_port_off		:		0,						// Offset of output port address
	filter_off			:		CDIOBI_PORT_H_FILTER,	// Offset of filter address
	int_mask_off		:		CDIOBI_PORT_H_INTMASK,	// Offset of interrupt mask address
	int_sence_off		:		CDIOBI_PORT_H_INTSTS,	// Offset of interrupt sence address
	int_ctrl_off		:		CDIOBI_PORT_H_INTCTRL,	// Offset of interrupt control address
	inp_port_num		:		4,						// Number of input ports
	out_port_num		:		0,						// Number of output ports
	min_filter_value	:		0,						// Minimum value of filter
	max_filter_value	:		20,						// Maximum value of filter
	int_bit				:		{ 0xff, 0xff, 0xff, 0xff},// Interrupt bit (valid bit is 1)
	max_int_bit			:		32,						// Maximum number of interrupt bits
	num_of_8255			:		0,						// Number of 8255 chips
	can_echo_back		:		0,						// Echo back available? (0: Unavailable, 1: Available)
	is_positive			:		0						// Positive logic? (0: Negative logic, 1: Positive logic)
	},
	{// DO-32T2-PCI
	board_name			:		"DO-32T2-PCI",			// Board name
	board_type			:		CDIOBI_BT_PCI_H_SERIES,	// Board type
	device_type			:		DEVICE_TYPE_PCI,		// PCI bus
	vendor_id			:		0x1221,					// Vendor ID
	device_id			:		0xB172,					// Device ID
	resource_type		:		SRC_TYPE_PORTIO,		// Revision type (1: Not use, 2: Present in PCI configuration register, 3: Present on port map)
	port_num			:		32,						// Number of occupied ports
	inp_port_off		:		0,						// Offset of input port address
	out_port_off		:		0,						// Offset of output port address
	filter_off			:		0,						// Offset of filter address
	int_mask_off		:		0,						// Offset of interrupt mask address
	int_sence_off		:		0,						// Offset of interrupt sence address
	int_ctrl_off		:		0,						// Offset of interrupt control address
	inp_port_num		:		0,						// Number of input ports
	out_port_num		:		4,						// Number of output ports
	min_filter_value	:		0,						// Minimum value of filter
	max_filter_value	:		0,						// Maximum value of filter
	int_bit				:		{0x00},					// Interrupt bit (valid bit is 1)
	max_int_bit			:		0,						// Maximum number of interrupt bits
	num_of_8255			:		0,						// Number of 8255 chips
	can_echo_back		:		1,						// Echo back available? (0: Unavailable, 1: Available)
	is_positive			:		0						// Positive logic? (0: Negative logic, 1: Positive logic)
	},
	{// DI-64T2-PCI
	board_name			:		"DI-64T2-PCI",			// Board name
	board_type			:		CDIOBI_BT_PCI_H_SERIES,	// Board type
	device_type			:		DEVICE_TYPE_PCI,		// PCI bus
	vendor_id			:		0x1221,					// Vendor ID
	device_id			:		0xB182,					// Device ID
	resource_type		:		SRC_TYPE_PORTIO,		// Revision type (1: Not use, 2: Present in PCI configuration register, 3: Present on port map)
	port_num			:		32,						// Number of occupied ports
	inp_port_off		:		0,						// Offset of input port address
	out_port_off		:		0,						// Offset of output port address
	filter_off			:		CDIOBI_PORT_H_FILTER,	// Offset of filter address
	int_mask_off		:		CDIOBI_PORT_H_INTMASK,	// Offset of interrupt mask address
	int_sence_off		:		CDIOBI_PORT_H_INTSTS,	// Offset of interrupt sence address
	int_ctrl_off		:		CDIOBI_PORT_H_INTCTRL,	// Offset of interrupt control address
	inp_port_num		:		8,						// Number of input ports
	out_port_num		:		0,						// Number of output ports
	min_filter_value	:		0,						// Minimum value of filter
	max_filter_value	:		20,						// Maximum value of filter
	int_bit				:		{ 0xff, 0xff, 0xff, 0xff},// Interrupt bit (valid bit is 1)
	max_int_bit			:		32,						// Maximum number of interrupt bits
	num_of_8255			:		0,						// Number of 8255 chips
	can_echo_back		:		0,						// Echo back available? (0: Unavailable, 1: Available)
	is_positive			:		0						// Positive logic? (0: Negative logic, 1: Positive logic)
	},
	{// DO-64T2-PCI
	board_name			:		"DO-64T2-PCI",			// Board name
	board_type			:		CDIOBI_BT_PCI_H_SERIES,	// Board type
	device_type			:		DEVICE_TYPE_PCI,		// PCI bus
	vendor_id			:		0x1221,					// Vendor ID
	device_id			:		0xB192,					// Device ID
	resource_type		:		SRC_TYPE_PORTIO,		// Revision type (1: Not use, 2: Present in PCI configuration register, 3: Present on port map)
	port_num			:		32,						// Number of occupied ports
	inp_port_off		:		0,						// Offset of input port address
	out_port_off		:		0,						// Offset of output port address
	filter_off			:		0,						// Offset of filter address
	int_mask_off		:		0,						// Offset of interrupt mask address
	int_sence_off		:		0,						// Offset of interrupt sence address
	int_ctrl_off		:		0,						// Offset of interrupt control address
	inp_port_num		:		0,						// Number of input ports
	out_port_num		:		8,						// Number of output ports
	min_filter_value	:		0,						// Minimum value of filter
	max_filter_value	:		0,						// Maximum value of filter
	int_bit				:		{ 0x00},				// Interrupt bit (valid bit is 1)
	max_int_bit			:		0,						// Maximum number of interrupt bits
	num_of_8255			:		0,						// Number of 8255 chips
	can_echo_back		:		1,						// Echo back available? (0: Unavailable, 1: Available)
	is_positive			:		0						// Positive logic? (0: Negative logic, 1: Positive logic)
	},
	{// DI-32T-PE
	board_name			:		"DI-32T-PE",			// Board name
	board_type			:		CDIOBI_BT_PCI_H_SERIES,	// Board type
	device_type			:		DEVICE_TYPE_PCI,		// PCI bus
	vendor_id			:		0x1221,					// Vendor ID
	device_id			:		0x9642,					// Device ID
	resource_type		:		SRC_TYPE_PORTIO,		// Revision type (1: Not use, 2: Present in PCI configuration register, 3: Present on port map)
	port_num			:		32,						// Number of occupied ports
	inp_port_off		:		0,						// Offset of input port address
	out_port_off		:		0,						// Offset of output port address
	filter_off			:		CDIOBI_PORT_H_FILTER,	// Offset of filter address
	int_mask_off		:		CDIOBI_PORT_H_INTMASK,	// Offset of interrupt mask address
	int_sence_off		:		CDIOBI_PORT_H_INTSTS,	// Offset of interrupt sence address
	int_ctrl_off		:		CDIOBI_PORT_H_INTCTRL,	// Offset of interrupt control address
	inp_port_num		:		4,						// Number of input ports
	out_port_num		:		0,						// Number of output ports
	min_filter_value	:		0,						// Minimum value of filter
	max_filter_value	:		20,						// Maximum value of filter
	int_bit				:		{ 0xff, 0xff, 0xff, 0xff},// Interrupt bit (valid bit is 1)
	max_int_bit			:		32,						// Maximum number of interrupt bits
	num_of_8255			:		0,						// Number of 8255 chips
	can_echo_back		:		0,						// Echo back available? (0: Unavailable, 1: Available)
	is_positive			:		0						// Positive logic? (0: Negative logic, 1: Positive logic)
	},
	{// DO-32T-PE
	board_name			:		"DO-32T-PE",			// Board name
	board_type			:		CDIOBI_BT_PCI_H_SERIES,	// Board type
	device_type			:		DEVICE_TYPE_PCI,		// PCI bus
	vendor_id			:		0x1221,					// Vendor ID
	device_id			:		0x9652,					// Device ID
	resource_type		:		SRC_TYPE_PORTIO,		// Revision type (1: Not use, 2: Present in PCI configuration register, 3: Present on port map)
	port_num			:		32,						// Number of occupied ports
	inp_port_off		:		0,						// Offset of input port address
	out_port_off		:		0,						// Offset of output port address
	filter_off			:		0,						// Offset of filter address
	int_mask_off		:		0,						// Offset of interrupt mask address
	int_sence_off		:		0,						// Offset of interrupt sence address
	int_ctrl_off		:		0,						// Offset of interrupt control address
	inp_port_num		:		0,						// Number of input ports
	out_port_num		:		4,						// Number of output ports
	min_filter_value	:		0,						// Minimum value of filter
	max_filter_value	:		0,						// Maximum value of filter
	int_bit				:		{0x00},					// Interrupt bit (valid bit is 1)
	max_int_bit			:		0,						// Maximum number of interrupt bits
	num_of_8255			:		0,						// Number of 8255 chips
	can_echo_back		:		1,						// Echo back available? (0: Unavailable, 1: Available)
	is_positive			:		0						// Positive logic? (0: Negative logic, 1: Positive logic)
	},
	{// DI-64T-PE
	board_name			:		"DI-64T-PE",			// Board name
	board_type			:		CDIOBI_BT_PCI_H_SERIES,	// Board type
	device_type			:		DEVICE_TYPE_PCI,		// PCI bus
	vendor_id			:		0x1221,					// Vendor ID
	device_id			:		0x9622,					// Device ID
	resource_type		:		SRC_TYPE_PORTIO,		// Revision type (1: Not use, 2: Present in PCI configuration register, 3: Present on port map)
	port_num			:		32,						// Number of occupied ports
	inp_port_off		:		0,						// Offset of input port address
	out_port_off		:		0,						// Offset of output port address
	filter_off			:		CDIOBI_PORT_H_FILTER,	// Offset of filter address
	int_mask_off		:		CDIOBI_PORT_H_INTMASK,	// Offset of interrupt mask address
	int_sence_off		:		CDIOBI_PORT_H_INTSTS,	// Offset of interrupt sence address
	int_ctrl_off		:		CDIOBI_PORT_H_INTCTRL,	// Offset of interrupt control address
	inp_port_num		:		8,						// Number of input ports
	out_port_num		:		0,						// Number of output ports
	min_filter_value	:		0,						// Minimum value of filter
	max_filter_value	:		20,						// Maximum value of filter
	int_bit				:		{ 0xff, 0xff, 0xff, 0xff},// Interrupt bit (valid bit is 1)
	max_int_bit			:		32,						// Maximum number of interrupt bits
	num_of_8255			:		0,						// Number of 8255 chips
	can_echo_back		:		0,						// Echo back available? (0: Unavailable, 1: Available)
	is_positive			:		0						// Positive logic? (0: Negative logic, 1: Positive logic)
	},
	{// DO-64T-PE
	board_name			:		"DO-64T-PE",			// Board name
	board_type			:		CDIOBI_BT_PCI_H_SERIES,	// Board type
	device_type			:		DEVICE_TYPE_PCI,		// PCI bus
	vendor_id			:		0x1221,					// Vendor ID
	device_id			:		0x9632,					// Device ID
	resource_type		:		SRC_TYPE_PORTIO,		// Revision type (1: Not use, 2: Present in PCI configuration register, 3: Present on port map)
	port_num			:		32,						// Number of occupied ports
	inp_port_off		:		0,						// Offset of input port address
	out_port_off		:		0,						// Offset of output port address
	filter_off			:		0,						// Offset of filter address
	int_mask_off		:		0,						// Offset of interrupt mask address
	int_sence_off		:		0,						// Offset of interrupt sence address
	int_ctrl_off		:		0,						// Offset of interrupt control address
	inp_port_num		:		0,						// Number of input ports
	out_port_num		:		8,						// Number of output ports
	min_filter_value	:		0,						// Minimum value of filter
	max_filter_value	:		0,						// Maximum value of filter
	int_bit				:		{ 0x00},				// Interrupt bit (valid bit is 1)
	max_int_bit			:		0,						// Maximum number of interrupt bits
	num_of_8255			:		0,						// Number of 8255 chips
	can_echo_back		:		1,						// Echo back available? (0: Unavailable, 1: Available)
	is_positive			:		0						// Positive logic? (0: Negative logic, 1: Positive logic)
	},
	{// DI-32B-PE
	board_name			:		"DI-32B-PE",			// Board name
	board_type			:		CDIOBI_BT_PCI_H_SERIES,	// Board type
	device_type			:		DEVICE_TYPE_PCI,		// PCI bus
	vendor_id			:		0x1221,					// Vendor ID
	device_id			:		0x9662,					// Device ID
	resource_type		:		SRC_TYPE_PORTIO,		// Revision type (1: Not use, 2: Present in PCI configuration register, 3: Present on port map)
	port_num			:		32,						// Number of occupied ports
	inp_port_off		:		0,						// Offset of input port address
	out_port_off		:		0,						// Offset of output port address
	filter_off			:		CDIOBI_PORT_H_FILTER,	// Offset of filter address
	int_mask_off		:		CDIOBI_PORT_H_INTMASK,	// Offset of interrupt mask address
	int_sence_off		:		CDIOBI_PORT_H_INTSTS,	// Offset of interrupt sence address
	int_ctrl_off		:		CDIOBI_PORT_H_INTCTRL,	// Offset of interrupt control address
	inp_port_num		:		4,						// Number of input ports
	out_port_num		:		0,						// Number of output ports
	min_filter_value	:		0,						// Minimum value of filter
	max_filter_value	:		20,						// Maximum value of filter
	int_bit				:		{ 0xff, 0xff, 0xff, 0xff},// Interrupt bit (valid bit is 1)
	max_int_bit			:		32,						// Maximum number of interrupt bits
	num_of_8255			:		0,						// Number of 8255 chips
	can_echo_back		:		0,						// Echo back available? (0: Unavailable, 1: Available)
	is_positive			:		0						// Positive logic? (0: Negative logic, 1: Positive logic)
	},
	{// DO-32B-PE
	board_name			:		"DO-32B-PE",			// Board name
	board_type			:		CDIOBI_BT_PCI_H_SERIES,	// Board type
	device_type			:		DEVICE_TYPE_PCI,		// PCI bus
	vendor_id			:		0x1221,					// Vendor ID
	device_id			:		0x9672,					// Device ID
	resource_type		:		SRC_TYPE_PORTIO,		// Revision type (1: Not use, 2: Present in PCI configuration register, 3: Present on port map)
	port_num			:		32,						// Number of occupied ports
	inp_port_off		:		0,						// Offset of input port address
	out_port_off		:		0,						// Offset of output port address
	filter_off			:		0,						// Offset of filter address
	int_mask_off		:		0,						// Offset of interrupt mask address
	int_sence_off		:		0,						// Offset of interrupt sence address
	int_ctrl_off		:		0,						// Offset of interrupt control address
	inp_port_num		:		0,						// Number of input ports
	out_port_num		:		4,						// Number of output ports
	min_filter_value	:		0,						// Minimum value of filter
	max_filter_value	:		0,						// Maximum value of filter
	int_bit				:		{0x00},					// Interrupt bit (valid bit is 1)
	max_int_bit			:		0,						// Maximum number of interrupt bits
	num_of_8255			:		0,						// Number of 8255 chips
	can_echo_back		:		1,						// Echo back available? (0: Unavailable, 1: Available)
	is_positive			:		0						// Positive logic? (0: Negative logic, 1: Positive logic)
	},
	{// DIO-1616H-PE
	board_name			:		"DIO-1616H-PE",			// Board name
	board_type			:		CDIOBI_BT_PCI_H_SERIES,	// Board type
	device_type			:		DEVICE_TYPE_PCI,		// PCI bus
	vendor_id			:		0x1221,					// Vendor ID
	device_id			:		0x9682,					// Device ID
	resource_type		:		SRC_TYPE_PORTIO,		// Revision type (1: Not use, 2: Present in PCI configuration register, 3: Present on port map)
	port_num			:		32,						// Number of occupied ports
	inp_port_off		:		0,						// Offset of input port address
	out_port_off		:		2,						// Offset of output port address
	filter_off			:		CDIOBI_PORT_H_FILTER,	// Offset of filter address
	int_mask_off		:		CDIOBI_PORT_H_INTMASK,	// Offset of interrupt mask address
	int_sence_off		:		CDIOBI_PORT_H_INTSTS,	// Offset of interrupt sence address
	int_ctrl_off		:		CDIOBI_PORT_H_INTCTRL,	// Offset of interrupt control address
	inp_port_num		:		2,						// Number of input ports
	out_port_num		:		2,						// Number of output ports
	min_filter_value	:		0,						// Minimum value of filter
	max_filter_value	:		20,						// Maximum value of filter
	int_bit				:		{ 0xff, 0xff },			// Interrupt bit (valid bit is 1)
	max_int_bit			:		16,						// Maximum number of interrupt bits
	num_of_8255			:		0,						// Number of 8255 chips
	can_echo_back		:		1,						// Echo back available? (0: Unavailable, 1: Available)
	is_positive			:		0						// Positive logic? (0: Negative logic, 1: Positive logic)
	},
	{// DIO-3232H-PE
	board_name			:		"DIO-3232H-PE",			// Board name
	board_type			:		CDIOBI_BT_PCI_H_SERIES,	// Board type
	device_type			:		DEVICE_TYPE_PCI,		// PCI bus
	vendor_id			:		0x1221,					// Vendor ID
	device_id			:		0x9692,					// Device ID
	resource_type		:		SRC_TYPE_PORTIO,		// Revision type (1: Not use, 2: Present in PCI configuration register, 3: Present on port map)
	port_num			:		32,						// Number of occupied ports
	inp_port_off		:		0,						// Offset of input port address
	out_port_off		:		4,						// Offset of output port address
	filter_off			:		CDIOBI_PORT_H_FILTER,	// Offset of filter address
	int_mask_off		:		CDIOBI_PORT_H_INTMASK,	// Offset of interrupt mask address
	int_sence_off		:		CDIOBI_PORT_H_INTSTS,	// Offset of interrupt sence address
	int_ctrl_off		:		CDIOBI_PORT_H_INTCTRL,	// Offset of interrupt control address
	inp_port_num		:		4,						// Number of input ports
	out_port_num		:		4,						// Number of output ports
	min_filter_value	:		0,						// Minimum value of filter
	max_filter_value	:		20,						// Maximum value of filter
	int_bit				:		{ 0xff, 0xff, 0xff, 0xff},// Interrupt bit (valid bit is 1)
	max_int_bit			:		32,						// Maximum number of interrupt bits
	num_of_8255			:		0,						// Number of 8255 chips
	can_echo_back		:		1,						// Echo back available? (0: Unavailable, 1: Available)
	is_positive			:		0						// Positive logic? (0: Negative logic, 1: Positive logic)
	},
	{// DIO-1616RL-PE
	board_name			:		"DIO-1616RL-PE",		// Board name
	board_type			:		CDIOBI_BT_PCI_H_SERIES,	// Board type
	device_type			:		DEVICE_TYPE_PCI,		// PCI bus
	vendor_id			:		0x1221,					// Vendor ID
	device_id			:		0x96A2,					// Device ID
	resource_type		:		SRC_TYPE_PORTIO,		// Revision type (1: Not use, 2: Present in PCI configuration register, 3: Present on port map)
	port_num			:		32,						// Number of occupied ports
	inp_port_off		:		0,						// Offset of input port address
	out_port_off		:		2,						// Offset of output port address
	filter_off			:		CDIOBI_PORT_H_FILTER,	// Offset of filter address
	int_mask_off		:		CDIOBI_PORT_H_INTMASK,	// Offset of interrupt mask address
	int_sence_off		:		CDIOBI_PORT_H_INTSTS,	// Offset of interrupt sence address
	int_ctrl_off		:		CDIOBI_PORT_H_INTCTRL,	// Offset of interrupt control address
	inp_port_num		:		2,						// Number of input ports
	out_port_num		:		2,						// Number of output ports
	min_filter_value	:		0,						// Minimum value of filter
	max_filter_value	:		20,						// Maximum value of filter
	int_bit				:		{ 0xff, 0xff },			// Interrupt bit (valid bit is 1)
	max_int_bit			:		16,						// Maximum number of interrupt bits
	num_of_8255			:		0,						// Number of 8255 chips
	can_echo_back		:		1,						// Echo back available? (0: Unavailable, 1: Available)
	is_positive			:		0						// Positive logic? (0: Negative logic, 1: Positive logic)
	},
	{// DIO-3232RL-PE
	board_name			:		"DIO-3232RL-PE",		// Board name
	board_type			:		CDIOBI_BT_PCI_H_SERIES,	// Board type
	device_type			:		DEVICE_TYPE_PCI,		// PCI bus
	vendor_id			:		0x1221,					// Vendor ID
	device_id			:		0x96B2,					// Device ID
	resource_type		:		SRC_TYPE_PORTIO,		// Revision type (1: Not use, 2: Present in PCI configuration register, 3: Present on port map)
	port_num			:		32,						// Number of occupied ports
	inp_port_off		:		0,						// Offset of input port address
	out_port_off		:		4,						// Offset of output port address
	filter_off			:		CDIOBI_PORT_H_FILTER,	// Offset of filter address
	int_mask_off		:		CDIOBI_PORT_H_INTMASK,	// Offset of interrupt mask address
	int_sence_off		:		CDIOBI_PORT_H_INTSTS,	// Offset of interrupt sence address
	int_ctrl_off		:		CDIOBI_PORT_H_INTCTRL,	// Offset of interrupt control address
	inp_port_num		:		4,						// Number of input ports
	out_port_num		:		4,						// Number of output ports
	min_filter_value	:		0,						// Minimum value of filter
	max_filter_value	:		20,						// Maximum value of filter
	int_bit				:		{ 0xff, 0xff, 0xff, 0xff},// Interrupt bit (valid bit is 1)
	max_int_bit			:		32,						// Maximum number of interrupt bits
	num_of_8255			:		0,						// Number of 8255 chips
	can_echo_back		:		1,						// Echo back available? (0: Unavailable, 1: Available)
	is_positive			:		0						// Positive logic? (0: Negative logic, 1: Positive logic)
	},
// ASIC
	// PIO
	{// PIO-32/32L(PCI)
	board_name			:		"PIO-32/32L(PCI)",		// Board name
	board_type			:		CDIOBI_BT_PCI_ASIC,		// Board type
	device_type			:		DEVICE_TYPE_PCI,		// PCI bus
	vendor_id			:		0x1221,					// Vendor ID
	device_id			:		0x8122,					// Device ID
	resource_type		:		SRC_TYPE_PORTIO,		// Revision type (1: Not use, 2: Present in PCI configuration register, 3: Present on port map)
	port_num			:		32,						// Number of occupied ports
	inp_port_off		:		0,						// Offset of input port address
	out_port_off		:		4,						// Offset of output port address
	filter_off			:		CDIOBI_PORT_ASIC_FILTER,// Offset of filter address
	int_mask_off		:		CDIOBI_PORT_ASIC_INTMASK,// Offset of interrupt mask address
	int_sence_off		:		CDIOBI_PORT_ASIC_INTSTS,// Offset of interrupt sence address
	int_ctrl_off		:		CDIOBI_PORT_ASIC_INTCTRL,// Offset of interrupt control address
	inp_port_num		:		4,						// Number of input ports
	out_port_num		:		4,						// Number of output ports
	min_filter_value	:		0,						// Minimum value of filter
	max_filter_value	:		20,						// Maximum value of filter
	int_bit				:		{ 0x0f, 0},				// Interrupt bit (valid bit is 1)
	max_int_bit			:		4,						// Maximum number of interrupt bits
	num_of_8255			:		0,						// Number of 8255 chips
	can_echo_back		:		1,						// Echo back available? (0: Unavailable, 1: Available)
	is_positive			:		0						// Positive logic? (0: Negative logic, 1: Positive logic)
	},
	{// PIO-32/32T(PCI)
	board_name			:		"PIO-32/32T(PCI)",		// Board name
	board_type			:		CDIOBI_BT_PCI_ASIC,		// Board type
	device_type			:		DEVICE_TYPE_PCI,		// PCI bus
	vendor_id			:		0x1221,					// Vendor ID
	device_id			:		0x8152,					// Device ID
	resource_type		:		SRC_TYPE_PORTIO,		// Revision type (1: Not use, 2: Present in PCI configuration register, 3: Present on port map)
	port_num			:		32,						// Number of occupied ports
	inp_port_off		:		0,						// Offset of input port address
	out_port_off		:		4,						// Offset of output port address
	filter_off			:		CDIOBI_PORT_ASIC_FILTER,// Offset of filter address
	int_mask_off		:		CDIOBI_PORT_ASIC_INTMASK,// Offset of interrupt mask address
	int_sence_off		:		CDIOBI_PORT_ASIC_INTSTS,// Offset of interrupt sence address
	int_ctrl_off		:		CDIOBI_PORT_ASIC_INTCTRL,// Offset of interrupt control address
	inp_port_num		:		4,						// Number of input ports
	out_port_num		:		4,						// Number of output ports
	min_filter_value	:		0,						// Minimum value of filter
	max_filter_value	:		20,						// Maximum value of filter
	int_bit				:		{ 0x0f, 0},				// Interrupt bit (valid bit is 1)
	max_int_bit			:		4,						// Maximum number of interrupt bits
	num_of_8255			:		0,						// Number of 8255 chips
	can_echo_back		:		1,						// Echo back available? (0: Unavailable, 1: Available)
	is_positive			:		0						// Positive logic? (0: Negative logic, 1: Positive logic)
	},
	{// PIO-32/32F(PCI)
	board_name			:		"PIO-32/32F(PCI)",		// Board name
	board_type			:		CDIOBI_BT_PCI_ASIC,		// Board type
	device_type			:		DEVICE_TYPE_PCI,		// PCI bus
	vendor_id			:		0x1221,					// Vendor ID
	device_id			:		0x81A2,					// Device ID
	resource_type		:		SRC_TYPE_PORTIO,		// Revision type (1: Not use, 2: Present in PCI configuration register, 3: Present on port map)
	port_num			:		32,						// Number of occupied ports
	inp_port_off		:		0,						// Offset of input port address
	out_port_off		:		4,						// Offset of output port address
	filter_off			:		CDIOBI_PORT_ASIC_FILTER,// Offset of filter address
	int_mask_off		:		CDIOBI_PORT_ASIC_INTMASK,// Offset of interrupt mask address
	int_sence_off		:		CDIOBI_PORT_ASIC_INTSTS,// Offset of interrupt sence address
	int_ctrl_off		:		CDIOBI_PORT_ASIC_INTCTRL,// Offset of interrupt control address
	inp_port_num		:		4,						// Number of input ports
	out_port_num		:		4,						// Number of output ports
	min_filter_value	:		0,						// Minimum value of filter
	max_filter_value	:		20,						// Maximum value of filter
	int_bit				:		{ 0x0f, 0},				// Interrupt bit (valid bit is 1)
	max_int_bit			:		4,						// Maximum number of interrupt bits
	num_of_8255			:		0,						// Number of 8255 chips
	can_echo_back		:		1,						// Echo back available? (0: Unavailable, 1: Available)
	is_positive			:		0						// Positive logic? (0: Negative logic, 1: Positive logic)
	},
	{// PIO-32/32B(PCI)H
	board_name			:		"PIO-32/32B(PCI)H",		// Board name
	board_type			:		CDIOBI_BT_PCI_ASIC,		// Board type
	device_type			:		DEVICE_TYPE_PCI,		// PCI bus
	vendor_id			:		0x1221,					// Vendor ID
	device_id			:		0x8112,					// Device ID
	resource_type		:		SRC_TYPE_PORTIO,		// Revision type (1: Not use, 2: Present in PCI configuration register, 3: Present on port map)
	port_num			:		32,						// Number of occupied ports
	inp_port_off		:		0,						// Offset of input port address
	out_port_off		:		4,						// Offset of output port address
	filter_off			:		CDIOBI_PORT_ASIC_FILTER,// Offset of filter address
	int_mask_off		:		CDIOBI_PORT_ASIC_INTMASK,// Offset of interrupt mask address
	int_sence_off		:		CDIOBI_PORT_ASIC_INTSTS,// Offset of interrupt sence address
	int_ctrl_off		:		CDIOBI_PORT_ASIC_INTCTRL,// Offset of interrupt control address
	inp_port_num		:		4,						// Number of input ports
	out_port_num		:		4,						// Number of output ports
	min_filter_value	:		0,						// Minimum value of filter
	max_filter_value	:		20,						// Maximum value of filter
	int_bit				:		{ 0x0f, 0},				// Interrupt bit (valid bit is 1)
	max_int_bit			:		4,						// Maximum number of interrupt bits
	num_of_8255			:		0,						// Number of 8255 chips
	can_echo_back		:		1,						// Echo back available? (0: Unavailable, 1: Available)
	is_positive			:		0						// Positive logic? (0: Negative logic, 1: Positive logic)
	},
	{// PIO-16/16L(PCI)
	board_name			:		"PIO-16/16L(PCI)",		// Board name
	board_type			:		CDIOBI_BT_PCI_ASIC,		// Board type
	device_type			:		DEVICE_TYPE_PCI,		// PCI bus
	vendor_id			:		0x1221,					// Vendor ID
	device_id			:		0x8172,					// Device ID
	resource_type		:		SRC_TYPE_PORTIO,		// Revision type (1: Not use, 2: Present in PCI configuration register, 3: Present on port map)
	port_num			:		32,						// Number of occupied ports
	inp_port_off		:		0,						// Offset of input port address
	out_port_off		:		2,						// Offset of output port address
	filter_off			:		CDIOBI_PORT_ASIC_FILTER,// Offset of filter address
	int_mask_off		:		CDIOBI_PORT_ASIC_INTMASK,// Offset of interrupt mask address
	int_sence_off		:		CDIOBI_PORT_ASIC_INTSTS,// Offset of interrupt sence address
	int_ctrl_off		:		CDIOBI_PORT_ASIC_INTCTRL,// Offset of interrupt control address
	inp_port_num		:		2,						// Number of input ports
	out_port_num		:		2,						// Number of output ports
	min_filter_value	:		0,						// Minimum value of filter
	max_filter_value	:		20,						// Maximum value of filter
	int_bit				:		{ 0x0f, 0},				// Interrupt bit (valid bit is 1)
	max_int_bit			:		4,						// Maximum number of interrupt bits
	num_of_8255			:		0,						// Number of 8255 chips
	can_echo_back		:		1,						// Echo back available? (0: Unavailable, 1: Available)
	is_positive			:		0						// Positive logic? (0: Negative logic, 1: Positive logic)
	},
	{// PIO-16/16T(PCI)
	board_name			:		"PIO-16/16T(PCI)",		// Board name
	board_type			:		CDIOBI_BT_PCI_ASIC,		// Board type
	device_type			:		DEVICE_TYPE_PCI,		// PCI bus
	vendor_id			:		0x1221,					// Vendor ID
	device_id			:		0x8162,					// Device ID
	resource_type		:		SRC_TYPE_PORTIO,		// Revision type (1: Not use, 2: Present in PCI configuration register, 3: Present on port map)
	port_num			:		32,						// Number of occupied ports
	inp_port_off		:		0,						// Offset of input port address
	out_port_off		:		2,						// Offset of output port address
	filter_off			:		CDIOBI_PORT_ASIC_FILTER,// Offset of filter address
	int_mask_off		:		CDIOBI_PORT_ASIC_INTMASK,// Offset of interrupt mask address
	int_sence_off		:		CDIOBI_PORT_ASIC_INTSTS,// Offset of interrupt sence address
	int_ctrl_off		:		CDIOBI_PORT_ASIC_INTCTRL,// Offset of interrupt control address
	inp_port_num		:		2,						// Number of input ports
	out_port_num		:		2,						// Number of output ports
	min_filter_value	:		0,						// Minimum value of filter
	max_filter_value	:		20,						// Maximum value of filter
	int_bit				:		{ 0x0f, 0},				// Interrupt bit (valid bit is 1)
	max_int_bit			:		4,						// Maximum number of interrupt bits
	num_of_8255			:		0,						// Number of 8255 chips
	can_echo_back		:		1,						// Echo back available? (0: Unavailable, 1: Available)
	is_positive			:		0						// Positive logic? (0: Negative logic, 1: Positive logic)
	},
	{// PIO-16/16TB(PCI)
	board_name			:		"PIO-16/16TB(PCI)",		// Board name
	board_type			:		CDIOBI_BT_PCI_ASIC,		// Board type
	device_type			:		DEVICE_TYPE_PCI,		// PCI bus
	vendor_id			:		0x1221,					// Vendor ID
	device_id			:		0x8192,					// Device ID
	resource_type		:		SRC_TYPE_PORTIO,		// Revision type (1: Not use, 2: Present in PCI configuration register, 3: Present on port map)
	port_num			:		32,						// Number of occupied ports
	inp_port_off		:		0,						// Offset of input port address
	out_port_off		:		2,						// Offset of output port address
	filter_off			:		CDIOBI_PORT_ASIC_FILTER,// Offset of filter address
	int_mask_off		:		CDIOBI_PORT_ASIC_INTMASK,// Offset of interrupt mask address
	int_sence_off		:		CDIOBI_PORT_ASIC_INTSTS,// Offset of interrupt sence address
	int_ctrl_off		:		CDIOBI_PORT_ASIC_INTCTRL,// Offset of interrupt control address
	inp_port_num		:		2,						// Number of input ports
	out_port_num		:		2,						// Number of output ports
	min_filter_value	:		0,						// Minimum value of filter
	max_filter_value	:		20,						// Maximum value of filter
	int_bit				:		{ 0x0f, 0},				// Interrupt bit (valid bit is 1)
	max_int_bit			:		4,						// Maximum number of interrupt bits
	num_of_8255			:		0,						// Number of 8255 chips
	can_echo_back		:		1,						// Echo back available? (0: Unavailable, 1: Available)
	is_positive			:		0						// Positive logic? (0: Negative logic, 1: Positive logic)
	},
	{// PIO-16/16B(PCI)
	board_name			:		"PIO-16/16B(PCI)",		// Board name
	board_type			:		CDIOBI_BT_PCI_ASIC,		// Board type
	device_type			:		DEVICE_TYPE_PCI,		// PCI bus
	vendor_id			:		0x1221,					// Vendor ID
	device_id			:		0x81D2,					// Device ID
	resource_type		:		SRC_TYPE_PORTIO,		// Revision type (1: Not use, 2: Present in PCI configuration register, 3: Present on port map)
	port_num			:		32,						// Number of occupied ports
	inp_port_off		:		0,						// Offset of input port address
	out_port_off		:		2,						// Offset of output port address
	filter_off			:		CDIOBI_PORT_ASIC_FILTER,// Offset of filter address
	int_mask_off		:		CDIOBI_PORT_ASIC_INTMASK,// Offset of interrupt mask address
	int_sence_off		:		CDIOBI_PORT_ASIC_INTSTS,// Offset of interrupt sence address
	int_ctrl_off		:		CDIOBI_PORT_ASIC_INTCTRL,// Offset of interrupt control address
	inp_port_num		:		2,						// Number of input ports
	out_port_num		:		2,						// Number of output ports
	min_filter_value	:		0,						// Minimum value of filter
	max_filter_value	:		20,						// Maximum value of filter
	int_bit				:		{ 0x0f, 0},				// Interrupt bit (valid bit is 1)
	max_int_bit			:		4,						// Maximum number of interrupt bits
	num_of_8255			:		0,						// Number of 8255 chips
	can_echo_back		:		1,						// Echo back available? (0: Unavailable, 1: Available)
	is_positive			:		0						// Positive logic? (0: Negative logic, 1: Positive logic)
	},
	// PI
	{// PI-64L(PCI)
	board_name			:		"PI-64L(PCI)",			// Board name
	board_type			:		CDIOBI_BT_PCI_ASIC,		// Board type
	device_type			:		DEVICE_TYPE_PCI,		// PCI bus
	vendor_id			:		0x1221,					// Vendor ID
	device_id			:		0x8132,					// Device ID
	resource_type		:		SRC_TYPE_PORTIO,		// Revision type (1: Not use, 2: Present in PCI configuration register, 3: Present on port map)
	port_num			:		32,						// Number of occupied ports
	inp_port_off		:		0,						// Offset of input port address
	out_port_off		:		0,						// Offset of output port address
	filter_off			:		CDIOBI_PORT_ASIC_FILTER,// Offset of filter address
	int_mask_off		:		CDIOBI_PORT_ASIC_INTMASK,// Offset of interrupt mask address
	int_sence_off		:		CDIOBI_PORT_ASIC_INTSTS,// Offset of interrupt sence address
	int_ctrl_off		:		CDIOBI_PORT_ASIC_INTCTRL,// Offset of interrupt control address
	inp_port_num		:		8,						// Number of input ports
	out_port_num		:		0,						// Number of output ports
	min_filter_value	:		0,						// Minimum value of filter
	max_filter_value	:		20,						// Maximum value of filter
	int_bit				:		{ 0x0f, 0},				// Interrupt bit (valid bit is 1)
	max_int_bit			:		4,						// Maximum number of interrupt bits
	num_of_8255			:		0,						// Number of 8255 chips
	can_echo_back		:		0,						// Echo back available? (0: Unavailable, 1: Available)
	is_positive			:		0						// Positive logic? (0: Negative logic, 1: Positive logic)
	},
	{// PI-32L(PCI)
	board_name			:		"PI-32L(PCI)",			// Board name
	board_type			:		CDIOBI_BT_PCI_ASIC,		// Board type
	device_type			:		DEVICE_TYPE_PCI,		// PCI bus
	vendor_id			:		0x1221,					// Vendor ID
	device_id			:		0x81E2,					// Device ID
	resource_type		:		SRC_TYPE_PORTIO,		// Revision type (1: Not use, 2: Present in PCI configuration register, 3: Present on port map)
	port_num			:		32,						// Number of occupied ports
	inp_port_off		:		0,						// Offset of input port address
	out_port_off		:		0,						// Offset of output port address
	filter_off			:		CDIOBI_PORT_ASIC_FILTER,// Offset of filter address
	int_mask_off		:		CDIOBI_PORT_ASIC_INTMASK,// Offset of interrupt mask address
	int_sence_off		:		CDIOBI_PORT_ASIC_INTSTS,// Offset of interrupt sence address
	int_ctrl_off		:		CDIOBI_PORT_ASIC_INTCTRL,// Offset of interrupt control address
	inp_port_num		:		4,						// Number of input ports
	out_port_num		:		0,						// Number of output ports
	min_filter_value	:		0,						// Minimum value of filter
	max_filter_value	:		20,						// Maximum value of filter
	int_bit				:		{ 0x0f, 0},				// Interrupt bit (valid bit is 1)
	max_int_bit			:		4,						// Maximum number of interrupt bits
	num_of_8255			:		0,						// Number of 8255 chips
	can_echo_back		:		0,						// Echo back available? (0: Unavailable, 1: Available)
	is_positive			:		0						// Positive logic? (0: Negative logic, 1: Positive logic)
	},
	{// PI-32B(PCI)
	board_name			:		"PI-32B(PCI)",			// Board name
	board_type			:		CDIOBI_BT_PCI_ASIC,		// Board type
	device_type			:		DEVICE_TYPE_PCI,		// PCI bus
	vendor_id			:		0x1221,					// Vendor ID
	device_id			:		0x81F2,					// Device ID
	resource_type		:		SRC_TYPE_PORTIO,		// Revision type (1: Not use, 2: Present in PCI configuration register, 3: Present on port map)
	port_num			:		32,						// Number of occupied ports
	inp_port_off		:		0,						// Offset of input port address
	out_port_off		:		0,						// Offset of output port address
	filter_off			:		CDIOBI_PORT_ASIC_FILTER,// Offset of filter address
	int_mask_off		:		CDIOBI_PORT_ASIC_INTMASK,// Offset of interrupt mask address
	int_sence_off		:		CDIOBI_PORT_ASIC_INTSTS,// Offset of interrupt sence address
	int_ctrl_off		:		CDIOBI_PORT_ASIC_INTCTRL,// Offset of interrupt control address
	inp_port_num		:		4,						// Number of input ports
	out_port_num		:		0,						// Number of output ports
	min_filter_value	:		0,						// Minimum value of filter
	max_filter_value	:		20,						// Maximum value of filter
	int_bit				:		{ 0x0f, 0},				// Interrupt bit (valid bit is 1)
	max_int_bit			:		4,						// Maximum number of interrupt bits
	num_of_8255			:		0,						// Number of 8255 chips
	can_echo_back		:		0,						// Echo back available? (0: Unavailable, 1: Available)
	is_positive			:		0						// Positive logic? (0: Negative logic, 1: Positive logic)
	},
	// PO
	{// PO-64L(PCI)
	board_name			:		"PO-64L(PCI)",			// Board name
	board_type			:		CDIOBI_BT_PCI_ASIC,		// Board type
	device_type			:		DEVICE_TYPE_PCI,		// PCI bus
	vendor_id			:		0x1221,					// Vendor ID
	device_id			:		0x8142,					// Device ID
	resource_type		:		SRC_TYPE_PORTIO,		// Revision type (1: Not use, 2: Present in PCI configuration register, 3: Present on port map)
	port_num			:		32,						// Number of occupied ports
	inp_port_off		:		0,						// Offset of input port address
	out_port_off		:		0,						// Offset of output port address
	filter_off			:		0,						// Offset of filter address
	int_mask_off		:		0,						// Offset of interrupt mask address
	int_sence_off		:		0,						// Offset of interrupt sence address
	int_ctrl_off		:		0,						// Offset of interrupt control address
	inp_port_num		:		0,						// Number of input ports
	out_port_num		:		8,						// Number of output ports
	min_filter_value	:		0,						// Minimum value of filter
	max_filter_value	:		0,						// Maximum value of filter
	int_bit				:		{0},					// Interrupt bit (valid bit is 1)
	max_int_bit			:		0,						// Maximum number of interrupt bits
	num_of_8255			:		0,						// Number of 8255 chips
	can_echo_back		:		1,						// Echo back available? (0: Unavailable, 1: Available)
	is_positive			:		0						// Positive logic? (0: Negative logic, 1: Positive logic)
	},
	{// PO-32L(PCI)
	board_name			:		"PO-32L(PCI)",			// Board name
	board_type			:		CDIOBI_BT_PCI_ASIC,		// Board type
	device_type			:		DEVICE_TYPE_PCI,		// PCI bus
	vendor_id			:		0x1221,					// Vendor ID
	device_id			:		0x9102,					// Device ID
	resource_type		:		SRC_TYPE_PORTIO,		// Revision type (1: Not use, 2: Present in PCI configuration register, 3: Present on port map)
	port_num			:		32,						// Number of occupied ports
	inp_port_off		:		0,						// Offset of input port address
	out_port_off		:		0,						// Offset of output port address
	filter_off			:		0,						// Offset of filter address
	int_mask_off		:		0,						// Offset of interrupt mask address
	int_sence_off		:		0,						// Offset of interrupt sence address
	int_ctrl_off		:		0,						// Offset of interrupt control address
	inp_port_num		:		0,						// Number of input ports
	out_port_num		:		4,						// Number of output ports
	min_filter_value	:		0,						// Minimum value of filter
	max_filter_value	:		0,						// Maximum value of filter
	int_bit				:		{0},					// Interrupt bit (valid bit is 1)
	max_int_bit			:		0,						// Maximum number of interrupt bits
	num_of_8255			:		0,						// Number of 8255 chips
	can_echo_back		:		1,						// Echo back available? (0: Unavailable, 1: Available)
	is_positive			:		0						// Positive logic? (0: Negative logic, 1: Positive logic)
	},
	{// PO-32B(PCI)
	board_name			:		"PO-32B(PCI)",			// Board name
	board_type			:		CDIOBI_BT_PCI_ASIC,		// Board type
	device_type			:		DEVICE_TYPE_PCI,		// PCI bus
	vendor_id			:		0x1221,					// Vendor ID
	device_id			:		0x9112,					// Device ID
	resource_type		:		SRC_TYPE_PORTIO,		// Revision type (1: Not use, 2: Present in PCI configuration register, 3: Present on port map)
	port_num			:		32,						// Number of occupied ports
	inp_port_off		:		0,						// Offset of input port address
	out_port_off		:		0,						// Offset of output port address
	filter_off			:		0,						// Offset of filter address
	int_mask_off		:		0,						// Offset of interrupt mask address
	int_sence_off		:		0,						// Offset of interrupt sence address
	int_ctrl_off		:		0,						// Offset of interrupt control address
	inp_port_num		:		0,						// Number of input ports
	out_port_num		:		4,						// Number of output ports
	min_filter_value	:		0,						// Minimum value of filter
	max_filter_value	:		0,						// Maximum value of filter
	int_bit				:		{0},					// Interrupt bit (valid bit is 1)
	max_int_bit			:		0,						// Maximum number of interrupt bits
	num_of_8255			:		0,						// Number of 8255 chips
	can_echo_back		:		1,						// Echo back available? (0: Unavailable, 1: Available)
	is_positive			:		0						// Positive logic? (0: Negative logic, 1: Positive logic)
	},
// Compact PCI
	{// PIO-32/32L(CPCI)
	board_name			:		"PIO-32/32L(CPCI)",		// Board name
	board_type			:		CDIOBI_BT_PCI_ASIC,		// Board type
	device_type			:		DEVICE_TYPE_PCI,		// PCI bus
	vendor_id			:		0x1221,					// Vendor ID
	device_id			:		0x8202,					// Device ID
	resource_type		:		SRC_TYPE_PORTIO,		// Revision type (1: Not use, 2: Present in PCI configuration register, 3: Present on port map)
	port_num			:		32,						// Number of occupied ports
	inp_port_off		:		0,						// Offset of input port address
	out_port_off		:		4,						// Offset of output port address
	filter_off			:		CDIOBI_PORT_ASIC_FILTER,// Offset of filter address
	int_mask_off		:		CDIOBI_PORT_ASIC_INTMASK,// Offset of interrupt mask address
	int_sence_off		:		CDIOBI_PORT_ASIC_INTSTS,// Offset of interrupt sence address
	int_ctrl_off		:		CDIOBI_PORT_ASIC_INTCTRL,// Offset of interrupt control address
	inp_port_num		:		4,						// Number of input ports
	out_port_num		:		4,						// Number of output ports
	min_filter_value	:		0,						// Minimum value of filter
	max_filter_value	:		20,						// Maximum value of filter
	int_bit				:		{ 0x0f, 0},				// Interrupt bit (valid bit is 1)
	max_int_bit			:		4,						// Maximum number of interrupt bits
	num_of_8255			:		0,						// Number of 8255 chips
	can_echo_back		:		1,						// Echo back available? (0: Unavailable, 1: Available)
	is_positive			:		0						// Positive logic? (0: Negative logic, 1: Positive logic)
	},
	{// PI-64L(CPCI)
	board_name			:		"PI-64L(CPCI)",			// Board name
	board_type			:		CDIOBI_BT_PCI_ASIC,		// Board type
	device_type			:		DEVICE_TYPE_PCI,		// PCI bus
	vendor_id			:		0x1221,					// Vendor ID
	device_id			:		0x8212,					// Device ID
	resource_type		:		SRC_TYPE_PORTIO,		// Revision type (1: Not use, 2: Present in PCI configuration register, 3: Present on port map)
	port_num			:		32,						// Number of occupied ports
	inp_port_off		:		0,						// Offset of input port address
	out_port_off		:		0,						// Offset of output port address
	filter_off			:		CDIOBI_PORT_ASIC_FILTER,// Offset of filter address
	int_mask_off		:		CDIOBI_PORT_ASIC_INTMASK,// Offset of interrupt mask address
	int_sence_off		:		CDIOBI_PORT_ASIC_INTSTS,// Offset of interrupt sence address
	int_ctrl_off		:		CDIOBI_PORT_ASIC_INTCTRL,// Offset of interrupt control address
	inp_port_num		:		8,						// Number of input ports
	out_port_num		:		0,						// Number of output ports
	min_filter_value	:		0,						// Minimum value of filter
	max_filter_value	:		20,						// Maximum value of filter
	int_bit				:		{ 0x0f, 0},				// Interrupt bit (valid bit is 1)
	max_int_bit			:		4,						// Maximum number of interrupt bits
	num_of_8255			:		0,						// Number of 8255 chips
	can_echo_back		:		0,						// Echo back available? (0: Unavailable, 1: Available)
	is_positive			:		0						// Positive logic? (0: Negative logic, 1: Positive logic)
	},
	{// PO-64L(CPCI)
	board_name			:		"PO-64L(CPCI)",			// Board name
	board_type			:		CDIOBI_BT_PCI_ASIC,		// Board type
	device_type			:		DEVICE_TYPE_PCI,		// PCI bus
	vendor_id			:		0x1221,					// Vendor ID
	device_id			:		0x8222,					// Device ID
	resource_type		:		SRC_TYPE_PORTIO,		// Revision type (1: Not use, 2: Present in PCI configuration register, 3: Present on port map)
	port_num			:		32,						// Number of occupied ports
	inp_port_off		:		0,						// Offset of input port address
	out_port_off		:		0,						// Offset of output port address
	filter_off			:		0,						// Offset of filter address
	int_mask_off		:		0,						// Offset of interrupt mask address
	int_sence_off		:		0,						// Offset of interrupt sence address
	int_ctrl_off		:		0,						// Offset of interrupt control address
	inp_port_num		:		0,						// Number of input ports
	out_port_num		:		8,						// Number of output ports
	min_filter_value	:		0,						// Minimum value of filter
	max_filter_value	:		0,						// Maximum value of filter
	int_bit				:		{0},					// Interrupt bit (valid bit is 1)
	max_int_bit			:		0,						// Maximum number of interrupt bits
	num_of_8255			:		0,						// Number of 8255 chips
	can_echo_back		:		1,						// Echo back available? (0: Unavailable, 1: Available)
	is_positive			:		0						// Positive logic? (0: Negative logic, 1: Positive logic)
	},
// Relay
	{// DIO-1616RY-PE
	board_name			:		"DIO-1616RY-PE",		// Board name
	board_type			:		CDIOBI_BT_PCI_H_SERIES,	// Board type
	device_type			:		DEVICE_TYPE_PCI,		// PCI bus
	vendor_id			:		0x1221,					// Vendor ID
	device_id			:		0x96E2,					// Device ID
	resource_type		:		SRC_TYPE_PORTIO,		// Revision type (1: Not use, 2: Present in PCI configuration register, 3: Present on port map)
	port_num			:		32,						// Number of occupied ports
	inp_port_off		:		0,						// Offset of input port address
	out_port_off		:		2,						// Offset of output port address
	filter_off			:		CDIOBI_PORT_H_FILTER,	// Offset of filter address
	int_mask_off		:		CDIOBI_PORT_H_INTMASK,	// Offset of interrupt mask address
	int_sence_off		:		CDIOBI_PORT_H_INTSTS,	// Offset of interrupt sence address
	int_ctrl_off		:		CDIOBI_PORT_H_INTCTRL,	// Offset of interrupt control address
	inp_port_num		:		2,						// Number of input ports
	out_port_num		:		2,						// Number of output ports
	min_filter_value	:		0,						// Minimum value of filter
	max_filter_value	:		20,						// Maximum value of filter
	int_bit				:		{ 0xff, 0xff },			// Interrupt bit (valid bit is 1)
	max_int_bit			:		16,						// Maximum number of interrupt bits
	num_of_8255			:		0,						// Number of 8255 chips
	can_echo_back		:		1,						// Echo back available? (0: Unavailable, 1: Available)
	is_positive			:		0						// Positive logic? (0: Negative logic, 1: Positive logic)
	},
	{// RRY-16C-PE
	board_name			:		"RRY-16C-PE",			// Board name
	board_type			:		CDIOBI_BT_PCI_H_SERIES,	// Board type
	device_type			:		DEVICE_TYPE_PCI,		// PCI bus
	vendor_id			:		0x1221,					// Vendor ID
	device_id			:		0x96D2,					// Device ID
	resource_type		:		SRC_TYPE_PORTIO,		// Revision type (1: Not use, 2: Present in PCI configuration register, 3: Present on port map)
	port_num			:		4,						// Number of occupied ports
	inp_port_off		:		0,						// Offset of input port address
	out_port_off		:		0,						// Offset of output port address
	filter_off			:		0,						// Offset of filter address
	int_mask_off		:		0,						// Offset of interrupt mask address
	int_sence_off		:		0,						// Offset of interrupt sence address
	int_ctrl_off		:		0,						// Offset of interrupt control address
	inp_port_num		:		0,						// Number of input ports
	out_port_num		:		2,						// Number of output ports
	min_filter_value	:		0,						// Minimum value of filter
	max_filter_value	:		0,						// Maximum value of filter
	int_bit				:		{0},					// Interrupt bit (valid bit is 1)
	max_int_bit			:		0,						// Maximum number of interrupt bits
	num_of_8255			:		0,						// Number of 8255 chips
	can_echo_back		:		1,						// Echo back available? (0: Unavailable, 1: Available)
	is_positive			:		0						// Positive logic? (0: Negative logic, 1: Positive logic)
	},
	{// RRY-32-PE
	board_name			:		"RRY-32-PE",			// Board name
	board_type			:		CDIOBI_BT_PCI_H_SERIES,	// Board type
	device_type			:		DEVICE_TYPE_PCI,		// PCI bus
	vendor_id			:		0x1221,					// Vendor ID
	device_id			:		0x96C2,					// Device ID
	resource_type		:		SRC_TYPE_PORTIO,		// Revision type (1: Not use, 2: Present in PCI configuration register, 3: Present on port map)
	port_num			:		4,						// Number of occupied ports
	inp_port_off		:		0,						// Offset of input port address
	out_port_off		:		0,						// Offset of output port address
	filter_off			:		0,						// Offset of filter address
	int_mask_off		:		0,						// Offset of interrupt mask address
	int_sence_off		:		0,						// Offset of interrupt sence address
	int_ctrl_off		:		0,						// Offset of interrupt control address
	inp_port_num		:		0,						// Number of input ports
	out_port_num		:		4,						// Number of output ports
	min_filter_value	:		0,						// Minimum value of filter
	max_filter_value	:		0,						// Maximum value of filter
	int_bit				:		{0},					// Interrupt bit (valid bit is 1)
	max_int_bit			:		0,						// Maximum number of interrupt bits
	num_of_8255			:		0,						// Number of 8255 chips
	can_echo_back		:		1,						// Echo back available? (0: Unavailable, 1: Available)
	is_positive			:		0						// Positive logic? (0: Negative logic, 1: Positive logic)
	},
	{// RRY-16C(PCI)
	board_name			:		"RRY-16C(PCI)",			// Board name
	board_type			:		CDIOBI_BT_PCI_RELAY,	// Board type
	device_type			:		DEVICE_TYPE_PCI,		// PCI bus
	vendor_id			:		0x1221,					// Vendor ID
	device_id			:		0x81B2,					// Device ID
	resource_type		:		SRC_TYPE_PORTIO,		// Revision type (1: Not use, 2: Present in PCI configuration register, 3: Present on port map)
	port_num			:		4,						// Number of occupied ports
	inp_port_off		:		0,						// Offset of input port address
	out_port_off		:		0,						// Offset of output port address
	filter_off			:		0,						// Offset of filter address
	int_mask_off		:		0,						// Offset of interrupt mask address
	int_sence_off		:		0,						// Offset of interrupt sence address
	int_ctrl_off		:		0,						// Offset of interrupt control address
	inp_port_num		:		0,						// Number of input ports
	out_port_num		:		2,						// Number of output ports
	min_filter_value	:		0,						// Minimum value of filter
	max_filter_value	:		0,						// Maximum value of filter
	int_bit				:		{0},					// Interrupt bit (valid bit is 1)
	max_int_bit			:		0,						// Maximum number of interrupt bits
	num_of_8255			:		0,						// Number of 8255 chips
	can_echo_back		:		1,						// Echo back available? (0: Unavailable, 1: Available)
	is_positive			:		0						// Positive logic? (0: Negative logic, 1: Positive logic)
	},
	{// RRY-32(PCI)
	board_name			:		"RRY-32(PCI)",			// Board name
	board_type			:		CDIOBI_BT_PCI_RELAY,	// Board type
	device_type			:		DEVICE_TYPE_PCI,		// PCI bus
	vendor_id			:		0x1221,					// Vendor ID
	device_id			:		0x8182,					// Device ID
	resource_type		:		SRC_TYPE_PORTIO,		// Revision type (1: Not use, 2: Present in PCI configuration register, 3: Present on port map)
	port_num			:		4,						// Number of occupied ports
	inp_port_off		:		0,						// Offset of input port address
	out_port_off		:		0,						// Offset of output port address
	filter_off			:		0,						// Offset of filter address
	int_mask_off		:		0,						// Offset of interrupt mask address
	int_sence_off		:		0,						// Offset of interrupt sence address
	int_ctrl_off		:		0,						// Offset of interrupt control address
	inp_port_num		:		0,						// Number of input ports
	out_port_num		:		4,						// Number of output ports
	min_filter_value	:		0,						// Minimum value of filter
	max_filter_value	:		0,						// Maximum value of filter
	int_bit				:		{0},					// Interrupt bit (valid bit is 1)
	max_int_bit			:		0,						// Maximum number of interrupt bits
	num_of_8255			:		0,						// Number of 8255 chips
	can_echo_back		:		1,						// Echo back available? (0: Unavailable, 1: Available)
	is_positive			:		0						// Positive logic? (0: Negative logic, 1: Positive logic)
	},
// Multipoint
	{// PIO-64/64L(PCI)
	board_name			:		"PIO-64/64L(PCI)",		// Board name
	board_type			:		CDIOBI_BT_PCI_TATEN,	// Board type
	device_type			:		DEVICE_TYPE_PCI,		// PCI bus
	vendor_id			:		0x1221,					// Vendor ID
	device_id			:		0x9122,					// Device ID
	resource_type		:		SRC_TYPE_PORTIO,		// Revision type (1: Not use, 2: Present in PCI configuration register, 3: Present on port map)
	port_num			:		32,						// Number of occupied ports
	inp_port_off		:		0,						// Offset of input port address
	out_port_off		:		8,						// Offset of output port address
	filter_off			:		CDIOBI_PORT_TATEN_FILTER,// Offset of filter address
	int_mask_off		:		CDIOBI_PORT_TATEN_INTMASK,// Offset of interrupt mask address
	int_sence_off		:		CDIOBI_PORT_TATEN_INTSTS,// Offset of interrupt sence address
	int_ctrl_off		:		CDIOBI_PORT_TATEN_INTCTRL,// Offset of interrupt control address
	inp_port_num		:		8,						// Number of input ports
	out_port_num		:		8,						// Number of output ports
	min_filter_value	:		0,						// Minimum value of filter
	max_filter_value	:		20,						// Maximum value of filter
	int_bit				:		{0xff,0xff,0},			// Interrupt bit (valid bit is 1)
	max_int_bit			:		16,						// Maximum number of interrupt bits
	num_of_8255			:		0,						// Number of 8255 chips
	can_echo_back		:		1,						// Echo back available? (0: Unavailable, 1: Available)
	is_positive			:		0						// Positive logic? (0: Negative logic, 1: Positive logic)
	},
	{// PI-128L(PCI)
	board_name			:		"PI-128L(PCI)",			// Board name
	board_type			:		CDIOBI_BT_PCI_TATEN,	// Board type
	device_type			:		DEVICE_TYPE_PCI,		// PCI bus
	vendor_id			:		0x1221,					// Vendor ID
	device_id			:		0x9132,					// Device ID
	resource_type		:		SRC_TYPE_PORTIO,		// Revision type (1: Not use, 2: Present in PCI configuration register, 3: Present on port map)
	port_num			:		32,						// Number of occupied ports
	inp_port_off		:		0,						// Offset of input port address
	out_port_off		:		0,						// Offset of output port address
	filter_off			:		CDIOBI_PORT_TATEN_FILTER,// Offset of filter address
	int_mask_off		:		CDIOBI_PORT_TATEN_INTMASK,// Offset of interrupt mask address
	int_sence_off		:		CDIOBI_PORT_TATEN_INTSTS,// Offset of interrupt sence address
	int_ctrl_off		:		CDIOBI_PORT_TATEN_INTCTRL,// Offset of interrupt control address
	inp_port_num		:		16,						// Number of input ports
	out_port_num		:		0,						// Number of output ports
	min_filter_value	:		0,						// Minimum value of filter
	max_filter_value	:		20,						// Maximum value of filter
	int_bit				:		{0xff,0xff,0},			// Interrupt bit (valid bit is 1)
	max_int_bit			:		16,						// Maximum number of interrupt bits
	num_of_8255			:		0,						// Number of 8255 chips
	can_echo_back		:		0,						// Echo back available? (0: Unavailable, 1: Available)
	is_positive			:		0						// Positive logic? (0: Negative logic, 1: Positive logic)
	},
	{// PO-128L(PCI)
	board_name			:		"PO-128L(PCI)",			// Board name
	board_type			:		CDIOBI_BT_PCI_TATEN,	// Board type
	device_type			:		DEVICE_TYPE_PCI,		// PCI bus
	vendor_id			:		0x1221,					// Vendor ID
	device_id			:		0x9142,					// Device ID
	resource_type		:		SRC_TYPE_PORTIO,		// Revision type (1: Not use, 2: Present in PCI configuration register, 3: Present on port map)
	port_num			:		32,						// Number of occupied ports
	inp_port_off		:		0,						// Offset of input port address
	out_port_off		:		0,						// Offset of output port address
	filter_off			:		0,						// Offset of filter address
	int_mask_off		:		0,						// Offset of interrupt mask address
	int_sence_off		:		0,						// Offset of interrupt sence address
	int_ctrl_off		:		0,						// Offset of interrupt control address
	inp_port_num		:		0,						// Number of input ports
	out_port_num		:		16,						// Number of output ports
	min_filter_value	:		0,						// Minimum value of filter
	max_filter_value	:		0,						// Maximum value of filter
	int_bit				:		{0},					// Interrupt bit (valid bit is 1)
	max_int_bit			:		0,						// Maximum number of interrupt bits
	num_of_8255			:		0,						// Number of 8255 chips
	can_echo_back		:		1,						// Echo back available? (0: Unavailable, 1: Available)
	is_positive			:		0						// Positive logic? (0: Negative logic, 1: Positive logic)
	},
	{// PIO-64/64L(PCI)H
	board_name			:		"PIO-64/64L(PCI)H",		// Board name
	board_type			:		CDIOBI_BT_PCI_TATEN,	// Board type
	device_type			:		DEVICE_TYPE_PCI,		// PCI bus
	vendor_id			:		0x1221,					// Vendor ID
	device_id			:		0xA1C2,					// Device ID
	resource_type		:		SRC_TYPE_PORTIO,		// Revision type (1: Not use, 2: Present in PCI configuration register, 3: Present on port map)
	port_num			:		32,						// Number of occupied ports
	inp_port_off		:		0,						// Offset of input port address
	out_port_off		:		8,						// Offset of output port address
	filter_off			:		CDIOBI_PORT_TATEN_FILTER,// Offset of filter address
	int_mask_off		:		CDIOBI_PORT_TATEN_INTMASK,// Offset of interrupt mask address
	int_sence_off		:		CDIOBI_PORT_TATEN_INTSTS,// Offset of interrupt sence address
	int_ctrl_off		:		CDIOBI_PORT_TATEN_INTCTRL,// Offset of interrupt control address
	inp_port_num		:		8,						// Number of input ports
	out_port_num		:		8,						// Number of output ports
	min_filter_value	:		0,						// Minimum value of filter
	max_filter_value	:		20,						// Maximum value of filter
	int_bit				:		{0xff,0xff,0},			// Interrupt bit (valid bit is 1)
	max_int_bit			:		16,						// Maximum number of interrupt bits
	num_of_8255			:		0,						// Number of 8255 chips
	can_echo_back		:		1,						// Echo back available? (0: Unavailable, 1: Available)
	is_positive			:		0						// Positive logic? (0: Negative logic, 1: Positive logic)
	},
	{// PI-128L(PCI)H
	board_name			:		"PI-128L(PCI)H",		// Board name
	board_type			:		CDIOBI_BT_PCI_TATEN,	// Board type
	device_type			:		DEVICE_TYPE_PCI,		// PCI bus
	vendor_id			:		0x1221,					// Vendor ID
	device_id			:		0xA1D2,					// Device ID
	resource_type		:		SRC_TYPE_PORTIO,		// Revision type (1: Not use, 2: Present in PCI configuration register, 3: Present on port map)
	port_num			:		32,						// Number of occupied ports
	inp_port_off		:		0,						// Offset of input port address
	out_port_off		:		0,						// Offset of output port address
	filter_off			:		CDIOBI_PORT_TATEN_FILTER,// Offset of filter address
	int_mask_off		:		CDIOBI_PORT_TATEN_INTMASK,// Offset of interrupt mask address
	int_sence_off		:		CDIOBI_PORT_TATEN_INTSTS,// Offset of interrupt sence address
	int_ctrl_off		:		CDIOBI_PORT_TATEN_INTCTRL,// Offset of interrupt control address
	inp_port_num		:		16,						// Number of input ports
	out_port_num		:		0,						// Number of output ports
	min_filter_value	:		0,						// Minimum value of filter
	max_filter_value	:		20,						// Maximum value of filter
	int_bit				:		{0xff,0xff,0},			// Interrupt bit (valid bit is 1)
	max_int_bit			:		16,						// Maximum number of interrupt bits
	num_of_8255			:		0,						// Number of 8255 chips
	can_echo_back		:		0,						// Echo back available? (0: Unavailable, 1: Available)
	is_positive			:		0						// Positive logic? (0: Negative logic, 1: Positive logic)
	},
	{// PO-128L(PCI)H
	board_name			:		"PO-128L(PCI)H",		// Board name
	board_type			:		CDIOBI_BT_PCI_TATEN,	// Board type
	device_type			:		DEVICE_TYPE_PCI,		// PCI bus
	vendor_id			:		0x1221,					// Vendor ID
	device_id			:		0xA1E2,					// Device ID
	resource_type		:		SRC_TYPE_PORTIO,		// Revision type (1: Not use, 2: Present in PCI configuration register, 3: Present on port map)
	port_num			:		32,						// Number of occupied ports
	inp_port_off		:		0,						// Offset of input port address
	out_port_off		:		0,						// Offset of output port address
	filter_off			:		0,						// Offset of filter address
	int_mask_off		:		0,						// Offset of interrupt mask address
	int_sence_off		:		0,						// Offset of interrupt sence address
	int_ctrl_off		:		0,						// Offset of interrupt control address
	inp_port_num		:		0,						// Number of input ports
	out_port_num		:		16,						// Number of output ports
	min_filter_value	:		0,						// Minimum value of filter
	max_filter_value	:		0,						// Maximum value of filter
	int_bit				:		{0},					// Interrupt bit (valid bit is 1)
	max_int_bit			:		0,						// Maximum number of interrupt bits
	num_of_8255			:		0,						// Number of 8255 chips
	can_echo_back		:		1,						// Echo back available? (0: Unavailable, 1: Available)
	is_positive			:		0						// Positive logic? (0: Negative logic, 1: Positive logic)
	},
	{// DIO-6464T-PE
	board_name			:		"DIO-6464T-PE",			// Board name
	board_type			:		CDIOBI_BT_PCI_TATEN,	// Board type
	device_type			:		DEVICE_TYPE_PCI,		// PCI bus
	vendor_id			:		0x1221,					// Vendor ID
	device_id			:		0x9612,					// Device ID
	resource_type		:		SRC_TYPE_PORTIO,		// Revision type (1: Not use, 2: Present in PCI configuration register, 3: Present on port map)
	port_num			:		32,						// Number of occupied ports
	inp_port_off		:		0,						// Offset of input port address
	out_port_off		:		8,						// Offset of output port address
	filter_off			:		CDIOBI_PORT_TATEN_FILTER,// Offset of filter address
	int_mask_off		:		CDIOBI_PORT_TATEN_INTMASK,// Offset of interrupt mask address
	int_sence_off		:		CDIOBI_PORT_TATEN_INTSTS,// Offset of interrupt sence address
	int_ctrl_off		:		CDIOBI_PORT_TATEN_INTCTRL,// Offset of interrupt control address
	inp_port_num		:		8,						// Number of input ports
	out_port_num		:		8,						// Number of output ports
	min_filter_value	:		0,						// Minimum value of filter
	max_filter_value	:		20,						// Maximum value of filter
	int_bit				:		{0xff,0xff,0},			// Interrupt bit (valid bit is 1)
	max_int_bit			:		16,						// Maximum number of interrupt bits
	num_of_8255			:		0,						// Number of 8255 chips
	can_echo_back		:		1,						// Echo back available? (0: Unavailable, 1: Available)
	is_positive			:		0						// Positive logic? (0: Negative logic, 1: Positive logic)
	},
	{// DIO-6464L-PE
	board_name			:		"DIO-6464L-PE",			// Board name
	board_type			:		CDIOBI_BT_PCI_TATEN,	// Board type
	device_type			:		DEVICE_TYPE_PCI,		// PCI bus
	vendor_id			:		0x1221,					// Vendor ID
	device_id			:		0x8682,					// Device ID
	resource_type		:		SRC_TYPE_PORTIO,		// Revision type (1: Not use, 2: Present in PCI configuration register, 3: Present on port map)
	port_num			:		32,						// Number of occupied ports
	inp_port_off		:		0,						// Offset of input port address
	out_port_off		:		8,						// Offset of output port address
	filter_off			:		CDIOBI_PORT_TATEN_FILTER,// Offset of filter address
	int_mask_off		:		CDIOBI_PORT_TATEN_INTMASK,// Offset of interrupt mask address
	int_sence_off		:		CDIOBI_PORT_TATEN_INTSTS,// Offset of interrupt sence address
	int_ctrl_off		:		CDIOBI_PORT_TATEN_INTCTRL,// Offset of interrupt control address
	inp_port_num		:		8,						// Number of input ports
	out_port_num		:		8,						// Number of output ports
	min_filter_value	:		0,						// Minimum value of filter
	max_filter_value	:		20,						// Maximum value of filter
	int_bit				:		{0xff,0xff,0},			// Interrupt bit (valid bit is 1)
	max_int_bit			:		16,						// Maximum number of interrupt bits
	num_of_8255			:		0,						// Number of 8255 chips
	can_echo_back		:		1,						// Echo back available? (0: Unavailable, 1: Available)
	is_positive			:		0						// Positive logic? (0: Negative logic, 1: Positive logic)
	},
	{// DI-128L-PE
	board_name			:		"DI-128L-PE",			// Board name
	board_type			:		CDIOBI_BT_PCI_TATEN,	// Board type
	device_type			:		DEVICE_TYPE_PCI,		// PCI bus
	vendor_id			:		0x1221,					// Vendor ID
	device_id			:		0x8692,					// Device ID
	resource_type		:		SRC_TYPE_PORTIO,		// Revision type (1: Not use, 2: Present in PCI configuration register, 3: Present on port map)
	port_num			:		32,						// Number of occupied ports
	inp_port_off		:		0,						// Offset of input port address
	out_port_off		:		0,						// Offset of output port address
	filter_off			:		CDIOBI_PORT_TATEN_FILTER,// Offset of filter address
	int_mask_off		:		CDIOBI_PORT_TATEN_INTMASK,// Offset of interrupt mask address
	int_sence_off		:		CDIOBI_PORT_TATEN_INTSTS,// Offset of interrupt sence address
	int_ctrl_off		:		CDIOBI_PORT_TATEN_INTCTRL,// Offset of interrupt control address
	inp_port_num		:		16,						// Number of input ports
	out_port_num		:		0,						// Number of output ports
	min_filter_value	:		0,						// Minimum value of filter
	max_filter_value	:		20,						// Maximum value of filter
	int_bit				:		{0xff,0xff,0},			// Interrupt bit (valid bit is 1)
	max_int_bit			:		16,						// Maximum number of interrupt bits
	num_of_8255			:		0,						// Number of 8255 chips
	can_echo_back		:		0,						// Echo back available? (0: Unavailable, 1: Available)
	is_positive			:		0						// Positive logic? (0: Negative logic, 1: Positive logic)
	},
	{// DO-128L-PE
	board_name			:		"DO-128L-PE",			// Board name
	board_type			:		CDIOBI_BT_PCI_TATEN,	// Board type
	device_type			:		DEVICE_TYPE_PCI,		// PCI bus
	vendor_id			:		0x1221,					// Vendor ID
	device_id			:		0x86A2,					// Device ID
	resource_type		:		SRC_TYPE_PORTIO,		// Revision type (1: Not use, 2: Present in PCI configuration register, 3: Present on port map)
	port_num			:		32,						// Number of occupied ports
	inp_port_off		:		0,						// Offset of input port address
	out_port_off		:		0,						// Offset of output port address
	filter_off			:		0,						// Offset of filter address
	int_mask_off		:		0,						// Offset of interrupt mask address
	int_sence_off		:		0,						// Offset of interrupt sence address
	int_ctrl_off		:		0,						// Offset of interrupt control address
	inp_port_num		:		0,						// Number of input ports
	out_port_num		:		16,						// Number of output ports
	min_filter_value	:		0,						// Minimum value of filter
	max_filter_value	:		0,						// Maximum value of filter
	int_bit				:		{0},					// Interrupt bit (valid bit is 1)
	max_int_bit			:		0,						// Maximum number of interrupt bits
	num_of_8255			:		0,						// Number of 8255 chips
	can_echo_back		:		1,						// Echo back available? (0: Unavailable, 1: Available)
	is_positive			:		0						// Positive logic? (0: Negative logic, 1: Positive logic)
	},
	{// DIO-6464T2-PCI
	board_name			:		"DIO-6464T2-PCI",		// Board name
	board_type			:		CDIOBI_BT_PCI_TATEN,	// Board type
	device_type			:		DEVICE_TYPE_PCI,		// PCI bus
	vendor_id			:		0x1221,					// Vendor ID
	device_id			:		0xB122,					// Device ID
	resource_type		:		SRC_TYPE_PORTIO,		// Revision type (1: Not use, 2: Present in PCI configuration register, 3: Present on port map)
	port_num			:		32,						// Number of occupied ports
	inp_port_off		:		0,						// Offset of input port address
	out_port_off		:		8,						// Offset of output port address
	filter_off			:		CDIOBI_PORT_TATEN_FILTER,// Offset of filter address
	int_mask_off		:		CDIOBI_PORT_TATEN_INTMASK,// Offset of interrupt mask address
	int_sence_off		:		CDIOBI_PORT_TATEN_INTSTS,// Offset of interrupt sence address
	int_ctrl_off		:		CDIOBI_PORT_TATEN_INTCTRL,// Offset of interrupt control address
	inp_port_num		:		8,						// Number of input ports
	out_port_num		:		8,						// Number of output ports
	min_filter_value	:		0,						// Minimum value of filter
	max_filter_value	:		20,						// Maximum value of filter
	int_bit				:		{0xff,0xff,0},			// Interrupt bit (valid bit is 1)
	max_int_bit			:		16,						// Maximum number of interrupt bits
	num_of_8255			:		0,						// Number of 8255 chips
	can_echo_back		:		1,						// Echo back available? (0: Unavailable, 1: Available)
	is_positive			:		0						// Positive logic? (0: Negative logic, 1: Positive logic)
	},
	{// DI-128T2-PCI
	board_name			:		"DI-128T2-PCI",			// Board name
	board_type			:		CDIOBI_BT_PCI_TATEN,	// Board type
	device_type			:		DEVICE_TYPE_PCI,		// PCI bus
	vendor_id			:		0x1221,					// Vendor ID
	device_id			:		0xB132,					// Device ID
	resource_type		:		SRC_TYPE_PORTIO,		// Revision type (1: Not use, 2: Present in PCI configuration register, 3: Present on port map)
	port_num			:		32,						// Number of occupied ports
	inp_port_off		:		0,						// Offset of input port address
	out_port_off		:		0,						// Offset of output port address
	filter_off			:		CDIOBI_PORT_TATEN_FILTER,// Offset of filter address
	int_mask_off		:		CDIOBI_PORT_TATEN_INTMASK,// Offset of interrupt mask address
	int_sence_off		:		CDIOBI_PORT_TATEN_INTSTS,// Offset of interrupt sence address
	int_ctrl_off		:		CDIOBI_PORT_TATEN_INTCTRL,// Offset of interrupt control address
	inp_port_num		:		16,						// Number of input ports
	out_port_num		:		0,						// Number of output ports
	min_filter_value	:		0,						// Minimum value of filter
	max_filter_value	:		20,						// Maximum value of filter
	int_bit				:		{0xff,0xff,0},			// Interrupt bit (valid bit is 1)
	max_int_bit			:		16,						// Maximum number of interrupt bits
	num_of_8255			:		0,						// Number of 8255 chips
	can_echo_back		:		0,						// Echo back available? (0: Unavailable, 1: Available)
	is_positive			:		0						// Positive logic? (0: Negative logic, 1: Positive logic)
	},
	{// DO-128T2-PCI
	board_name			:		"DO-128T2-PCI",			// Board name
	board_type			:		CDIOBI_BT_PCI_TATEN,	// Board type
	device_type			:		DEVICE_TYPE_PCI,		// PCI bus
	vendor_id			:		0x1221,					// Vendor ID
	device_id			:		0xB142,					// Device ID
	resource_type		:		SRC_TYPE_PORTIO,		// Revision type (1: Not use, 2: Present in PCI configuration register, 3: Present on port map)
	port_num			:		32,						// Number of occupied ports
	inp_port_off		:		0,						// Offset of input port address
	out_port_off		:		0,						// Offset of output port address
	filter_off			:		0,						// Offset of filter address
	int_mask_off		:		0,						// Offset of interrupt mask address
	int_sence_off		:		0,						// Offset of interrupt sence address
	int_ctrl_off		:		0,						// Offset of interrupt control address
	inp_port_num		:		0,						// Number of input ports
	out_port_num		:		16,						// Number of output ports
	min_filter_value	:		0,						// Minimum value of filter
	max_filter_value	:		0,						// Maximum value of filter
	int_bit				:		{0},					// Interrupt bit (valid bit is 1)
	max_int_bit			:		0,						// Maximum number of interrupt bits
	num_of_8255			:		0,						// Number of 8255 chips
	can_echo_back		:		1,						// Echo back available? (0: Unavailable, 1: Available)
	is_positive			:		0						// Positive logic? (0: Negative logic, 1: Positive logic)
	},
	{// DI-128T-PE
	board_name			:		"DI-128T-PE",			// Board name
	board_type			:		CDIOBI_BT_PCI_TATEN,	// Board type
	device_type			:		DEVICE_TYPE_PCI,		// PCI bus
	vendor_id			:		0x1221,					// Vendor ID
	device_id			:		0xA612,					// Device ID
	resource_type		:		SRC_TYPE_PORTIO,		// Revision type (1: Not use, 2: Present in PCI configuration register, 3: Present on port map)
	port_num			:		32,						// Number of occupied ports
	inp_port_off		:		0,						// Offset of input port address
	out_port_off		:		0,						// Offset of output port address
	filter_off			:		CDIOBI_PORT_TATEN_FILTER,// Offset of filter address
	int_mask_off		:		CDIOBI_PORT_TATEN_INTMASK,// Offset of interrupt mask address
	int_sence_off		:		CDIOBI_PORT_TATEN_INTSTS,// Offset of interrupt sence address
	int_ctrl_off		:		CDIOBI_PORT_TATEN_INTCTRL,// Offset of interrupt control address
	inp_port_num		:		16,						// Number of input ports
	out_port_num		:		0,						// Number of output ports
	min_filter_value	:		0,						// Minimum value of filter
	max_filter_value	:		20,						// Maximum value of filter
	int_bit				:		{0xff,0xff,0},			// Interrupt bit (valid bit is 1)
	max_int_bit			:		16,						// Maximum number of interrupt bits
	num_of_8255			:		0,						// Number of 8255 chips
	can_echo_back		:		0,						// Echo back available? (0: Unavailable, 1: Available)
	is_positive			:		0						// Positive logic? (0: Negative logic, 1: Positive logic)
	},
	{// DO-128T-PE
	board_name			:		"DO-128T-PE",			// Board name
	board_type			:		CDIOBI_BT_PCI_TATEN,	// Board type
	device_type			:		DEVICE_TYPE_PCI,		// PCI bus
	vendor_id			:		0x1221,					// Vendor ID
	device_id			:		0xA622,					// Device ID
	resource_type		:		SRC_TYPE_PORTIO,		// Revision type (1: Not use, 2: Present in PCI configuration register, 3: Present on port map)
	port_num			:		32,						// Number of occupied ports
	inp_port_off		:		0,						// Offset of input port address
	out_port_off		:		0,						// Offset of output port address
	filter_off			:		0,						// Offset of filter address
	int_mask_off		:		0,						// Offset of interrupt mask address
	int_sence_off		:		0,						// Offset of interrupt sence address
	int_ctrl_off		:		0,						// Offset of interrupt control address
	inp_port_num		:		0,						// Number of input ports
	out_port_num		:		16,						// Number of output ports
	min_filter_value	:		0,						// Minimum value of filter
	max_filter_value	:		0,						// Maximum value of filter
	int_bit				:		{0},					// Interrupt bit (valid bit is 1)
	max_int_bit			:		0,						// Maximum number of interrupt bits
	num_of_8255			:		0,						// Number of 8255 chips
	can_echo_back		:		1,						// Echo back available? (0: Unavailable, 1: Available)
	is_positive			:		0						// Positive logic? (0: Negative logic, 1: Positive logic)
	},
// 8255
	{// PIO-48D(PCI)
	board_name			:		"PIO-48D(PCI)",						// Board name
	board_type			:		CDIOBI_BT_PCI_8255,					// Board type
	device_type			:		DEVICE_TYPE_PCI,					// PCI bus
	vendor_id			:		0x1221,								// Vendor ID
	device_id			:		0x81C2,								// Device ID
	resource_type		:		SRC_TYPE_PORTIO,					// Revision type (1: Not use, 2: Present in PCI configuration register, 3: Present on port map)
	port_num			:		32,									// Number of occupied ports
	inp_port_off		:		0,									// Offset of input port address
	out_port_off		:		0,									// Offset of output port address
	filter_off			:		0,									// Offset of filter address
	int_mask_off		:		CDIOBI_PORT_8255_INTMASK,			// Offset of interrupt mask address
	int_sence_off		:		CDIOBI_PORT_8255_INTSTS,			// Offset of interrupt sence address
	int_ctrl_off		:		0,									// Offset of interrupt control address
	inp_port_num		:		6,									// Number of input ports
	out_port_num		:		6,									// Number of output ports
	min_filter_value	:		0,									// Minimum value of filter
	max_filter_value	:		0,									// Maximum value of filter
	int_bit				:		{0xff,0xff,0xff,0xff,0xff,0xff},	// Interrupt bit (valid bit is 1)
	max_int_bit			:		48,									// Maximum number of interrupt bits
	num_of_8255			:		2,									// Number of 8255 chips
	can_echo_back		:		1,									// Echo back available? (0: Unavailable, 1: Available)
	is_positive			:		1									// Positive logic? (0: Negative logic, 1: Positive logic)
	},
	{// PIO-48D(CB)H
	board_name			:		"PIO-48D(CB)H",						// Board name
	board_type			:		CDIOBI_BT_PCI_8255,					// Board type
	device_type			:		DEVICE_TYPE_PCI,					// PCI bus
	vendor_id			:		0x1221,								// Vendor ID
	device_id			:		0x8512,								// Device ID
	resource_type		:		SRC_TYPE_PORTIO,					// Revision type (1: Not use, 2: Present in PCI configuration register, 3: Present on port map)
	port_num			:		32,									// Number of occupied ports
	inp_port_off		:		0,									// Offset of input port address
	out_port_off		:		0,									// Offset of output port address
	filter_off			:		CDIOBI_PORT_8255_H_FILTER,			// Offset of filter address
	int_mask_off		:		CDIOBI_PORT_8255_INTMASK,			// Offset of interrupt mask address
	int_sence_off		:		CDIOBI_PORT_8255_INTSTS,			// Offset of interrupt sence address
	int_ctrl_off		:		CDIOBI_PORT_8255_H_INTCTRL,			// Offset of interrupt control address
	inp_port_num		:		6,									// Number of input ports
	out_port_num		:		6,									// Number of output ports
	min_filter_value	:		0,									// Minimum value of filter
	max_filter_value	:		20,									// Maximum value of filter
	int_bit				:		{0xff,0xff,0xff,0xff,0xff,0xff},	// Interrupt bit (valid bit is 1)
	max_int_bit			:		48,									// Maximum number of interrupt bits
	num_of_8255			:		2,									// Number of 8255 chips
	can_echo_back		:		1,									// Echo back available? (0: Unavailable, 1: Available)
	is_positive			:		1									// Positive logic? (0: Negative logic, 1: Positive logic)
	},
	{// PIO-48D(LPCI)H
	board_name			:		"PIO-48D(LPCI)H",					// Board name
	board_type			:		CDIOBI_BT_PCI_8255,					// Board type
	device_type			:		DEVICE_TYPE_PCI,					// PCI bus
	vendor_id			:		0x1221,								// Vendor ID
	device_id			:		0xA1B2,								// Device ID
	resource_type		:		SRC_TYPE_PORTIO,					// Revision type (1: Not use, 2: Present in PCI configuration register, 3: Present on port map)
	port_num			:		32,									// Number of occupied ports
	inp_port_off		:		0,									// Offset of input port address
	out_port_off		:		0,									// Offset of output port address
	filter_off			:		CDIOBI_PORT_8255_H_FILTER,			// Offset of filter address
	int_mask_off		:		CDIOBI_PORT_8255_INTMASK,			// Offset of interrupt mask address
	int_sence_off		:		CDIOBI_PORT_8255_INTSTS,			// Offset of interrupt sence address
	int_ctrl_off		:		CDIOBI_PORT_8255_H_INTCTRL,			// Offset of interrupt control address
	inp_port_num		:		6,									// Number of input ports
	out_port_num		:		6,									// Number of output ports
	min_filter_value	:		0,									// Minimum value of filter
	max_filter_value	:		20,									// Maximum value of filter
	int_bit				:		{0xff,0xff,0xff,0xff,0xff,0xff},	// Interrupt bit (valid bit is 1)
	max_int_bit			:		48,									// Maximum number of interrupt bits
	num_of_8255			:		2,									// Number of 8255 chips
	can_echo_back		:		1,									// Echo back available? (0: Unavailable, 1: Available)
	is_positive			:		1									// Positive logic? (0: Negative logic, 1: Positive logic)
	},
	{// DIO-48D-LPE
	board_name			:		"DIO-48D-LPE",						// Board name
	board_type			:		CDIOBI_BT_PCI_8255,					// Board type
	device_type			:		DEVICE_TYPE_PCI,					// PCI bus
	vendor_id			:		0x1221,								// Vendor ID
	device_id			:		0x8602,								// Device ID
	resource_type		:		SRC_TYPE_PORTIO,					// Revision type (1: Not use, 2: Present in PCI configuration register, 3: Present on port map)
	port_num			:		32,									// Number of occupied ports
	inp_port_off		:		0,									// Offset of input port address
	out_port_off		:		0,									// Offset of output port address
	filter_off			:		CDIOBI_PORT_8255_H_FILTER,			// Offset of filter address
	int_mask_off		:		CDIOBI_PORT_8255_INTMASK,			// Offset of interrupt mask address
	int_sence_off		:		CDIOBI_PORT_8255_INTSTS,			// Offset of interrupt sence address
	int_ctrl_off		:		CDIOBI_PORT_8255_H_INTCTRL,			// Offset of interrupt control address
	inp_port_num		:		6,									// Number of input ports
	out_port_num		:		6,									// Number of output ports
	min_filter_value	:		0,									// Minimum value of filter
	max_filter_value	:		20,									// Maximum value of filter
	int_bit				:		{0xff,0xff,0xff,0xff,0xff,0xff},	// Interrupt bit (valid bit is 1)
	max_int_bit			:		48,									// Maximum number of interrupt bits
	num_of_8255			:		2,									// Number of 8255 chips
	can_echo_back		:		1,									// Echo back available? (0: Unavailable, 1: Available)
	is_positive			:		1									// Positive logic? (0: Negative logic, 1: Positive logic)
	},
	{// DIO-48D-PE
	board_name			:		"DIO-48D-PE",						// Board name
	board_type			:		CDIOBI_BT_PCI_8255,					// Board type
	device_type			:		DEVICE_TYPE_PCI,					// PCI bus
	vendor_id			:		0x1221,								// Vendor ID
	device_id			:		0x96F2,								// Device ID
	resource_type		:		SRC_TYPE_PORTIO,					// Revision type (1: Not use, 2: Present in PCI configuration register, 3: Present on port map)
	port_num			:		32,									// Number of occupied ports
	inp_port_off		:		0,									// Offset of input port address
	out_port_off		:		0,									// Offset of output port address
	filter_off			:		CDIOBI_PORT_8255_H_FILTER,			// Offset of filter address
	int_mask_off		:		CDIOBI_PORT_8255_INTMASK,			// Offset of interrupt mask address
	int_sence_off		:		CDIOBI_PORT_8255_INTSTS,			// Offset of interrupt sence address
	int_ctrl_off		:		CDIOBI_PORT_8255_H_INTCTRL,			// Offset of interrupt control address
	inp_port_num		:		6,									// Number of input ports
	out_port_num		:		6,									// Number of output ports
	min_filter_value	:		0,									// Minimum value of filter
	max_filter_value	:		20,									// Maximum value of filter
	int_bit				:		{0xff,0xff,0xff,0xff,0xff,0xff},	// Interrupt bit (valid bit is 1)
	max_int_bit			:		48,									// Maximum number of interrupt bits
	num_of_8255			:		2,									// Number of 8255 chips
	can_echo_back		:		1,									// Echo back available? (0: Unavailable, 1: Available)
	is_positive			:		1									// Positive logic? (0: Negative logic, 1: Positive logic)
	},
	{// DIO-48D2-PCI
	board_name			:		"DIO-48D2-PCI",						// Board name
	board_type			:		CDIOBI_BT_PCI_8255,					// Board type
	device_type			:		DEVICE_TYPE_PCI,					// PCI bus
	vendor_id			:		0x1221,								// Vendor ID
	device_id			:		0x81C2,								// Device ID
	resource_type		:		SRC_TYPE_PORTIO,					// Revision type (1: Not use, 2: Present in PCI configuration register, 3: Present on port map)
	port_num			:		32,									// Number of occupied ports
	inp_port_off		:		0,									// Offset of input port address
	out_port_off		:		0,									// Offset of output port address
	filter_off			:		CDIOBI_PORT_8255_H_FILTER,			// Offset of filter address
	int_mask_off		:		CDIOBI_PORT_8255_INTMASK,			// Offset of interrupt mask address
	int_sence_off		:		CDIOBI_PORT_8255_INTSTS,			// Offset of interrupt sence address
	int_ctrl_off		:		CDIOBI_PORT_8255_H_INTCTRL,			// Offset of interrupt control address
	inp_port_num		:		6,									// Number of input ports
	out_port_num		:		6,									// Number of output ports
	min_filter_value	:		0,									// Minimum value of filter
	max_filter_value	:		20,									// Maximum value of filter
	int_bit				:		{0xff,0xff,0xff,0xff,0xff,0xff},	// Interrupt bit (valid bit is 1)
	max_int_bit			:		48,									// Maximum number of interrupt bits
	num_of_8255			:		2,									// Number of 8255 chips
	can_echo_back		:		1,									// Echo back available? (0: Unavailable, 1: Available)
	is_positive			:		1									// Positive logic? (0: Negative logic, 1: Positive logic)
	},
	{// DIO-96D2-LPCI
	board_name			:		"DIO-96D2-LPCI",					// Board name
	board_type			:		CDIOBI_BT_PCI_8255,					// Board type
	device_type			:		DEVICE_TYPE_PCI,					// PCI bus
	vendor_id			:		0x1221,								// Vendor ID
	device_id			:		0xB152,								// Device ID
	resource_type		:		SRC_TYPE_PORTIO,					// Revision type (1: Not use, 2: Present in PCI configuration register, 3: Present on port map)
	port_num			:		128,								// Number of occupied ports
	inp_port_off		:		0,									// Offset of input port address
	out_port_off		:		0,									// Offset of output port address
	filter_off			:		CDIOBI_PORT_8255_T_FILTER,			// Offset of filter address
	int_mask_off		:		CDIOBI_PORT_8255_T_INTMASK,			// Offset of interrupt mask address
	int_sence_off		:		CDIOBI_PORT_8255_T_INTSTS,			// Offset of interrupt sence address
	int_ctrl_off		:		CDIOBI_PORT_8255_T_INTCTRL,			// Offset of interrupt control address
	inp_port_num		:		12,									// Number of input ports
	out_port_num		:		12,									// Number of output ports
	min_filter_value	:		0,									// Minimum value of filter
	max_filter_value	:		20,									// Maximum value of filter
	int_bit				:		{0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff},	// Interrupt bit (valid bit is 1)
	max_int_bit			:		96,									// Maximum number of interrupt bits
	num_of_8255			:		4,									// Number of 8255 chips
	can_echo_back		:		1,									// Echo back available? (0: Unavailable, 1: Available)
	is_positive			:		1									// Positive logic? (0: Negative logic, 1: Positive logic)
	},
	{// DIO-96D-LPE
	board_name			:		"DIO-96D-LPE",					// Board name
	board_type			:		CDIOBI_BT_PCI_8255,					// Board type
	device_type			:		DEVICE_TYPE_PCI,					// PCI bus
	vendor_id			:		0x1221,								// Vendor ID
	device_id			:		0xA602,								// Device ID
	resource_type		:		SRC_TYPE_PORTIO,					// Revision type (1: Not use, 2: Present in PCI configuration register, 3: Present on port map)
	port_num			:		128,								// Number of occupied ports
	inp_port_off		:		0,									// Offset of input port address
	out_port_off		:		0,									// Offset of output port address
	filter_off			:		CDIOBI_PORT_8255_T_FILTER,			// Offset of filter address
	int_mask_off		:		CDIOBI_PORT_8255_T_INTMASK,			// Offset of interrupt mask address
	int_sence_off		:		CDIOBI_PORT_8255_T_INTSTS,			// Offset of interrupt sence address
	int_ctrl_off		:		CDIOBI_PORT_8255_T_INTCTRL,			// Offset of interrupt control address
	inp_port_num		:		12,									// Number of input ports
	out_port_num		:		12,									// Number of output ports
	min_filter_value	:		0,									// Minimum value of filter
	max_filter_value	:		20,									// Maximum value of filter
	int_bit				:		{0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff},	// Interrupt bit (valid bit is 1)
	max_int_bit			:		96,									// Maximum number of interrupt bits
	num_of_8255			:		4,									// Number of 8255 chips
	can_echo_back		:		1,									// Echo back available? (0: Unavailable, 1: Available)
	is_positive			:		1									// Positive logic? (0: Negative logic, 1: Positive logic)
	},
	{// PIO-32DM(PCI)
	board_name			:		"PIO-32DM(PCI)",					// Board name
	board_type			:		CDIOBI_BT_PCI_DM,					// Board type
	device_type			:		DEVICE_TYPE_PCI,					// PCI bus
	vendor_id			:		0x1221,								// Vendor ID
	device_id			:		0xC102,								// Device ID
	resource_type		:		SRC_TYPE_PORTIO,					// Revision type (1: Not use, 2: Present in PCI configuration register, 3: Present on port map)
	port_num			:		32,									// Number of occupied ports
	inp_port_off		:		0,									// Offset of input port address
	out_port_off		:		0,									// Offset of output port address
	filter_off			:		0,									// Offset of filter address
	int_mask_off		:		CDIOBI_PORT_DM_INTMASK,				// Offset of interrupt mask address
	int_sence_off		:		CDIOBI_PORT_DM_INTSTS,				// Offset of interrupt sence address
	int_ctrl_off		:		0,									// Offset of interrupt control address
	inp_port_num		:		4,									// Number of input ports
	out_port_num		:		4,									// Number of output ports
	min_filter_value	:		0,									// Minimum value of filter
	max_filter_value	:		0,									// Maximum value of filter
	int_bit				:		{0x0f,0},							// Interrupt bit (valid bit is 1)
	max_int_bit			:		4,									// Maximum number of interrupt bits
	num_of_8255			:		0,									// Number of 8255 chips
	can_echo_back		:		1,									// Echo back available? (0: Unavailable, 1: Available)
	is_positive			:		1									// Positive logic? (0: Negative logic, 1: Positive logic)
	},
	{// DIO-32DM-PE
	board_name			:		"DIO-32DM-PE",						// Board name
	board_type			:		CDIOBI_BT_PCI_DM,					// Board type
	device_type			:		DEVICE_TYPE_PCI,					// PCI bus
	vendor_id			:		0x1221,								// Vendor ID
	device_id			:		0xA632,								// Device ID
	resource_type		:		SRC_TYPE_PORTIO,					// Revision type (1: Not use, 2: Present in PCI configuration register, 3: Present on port map)
	port_num			:		32,									// Number of occupied ports
	inp_port_off		:		0,									// Offset of input port address
	out_port_off		:		0,									// Offset of output port address
	filter_off			:		0,									// Offset of filter address
	int_mask_off		:		CDIOBI_PORT_DM_INTMASK,				// Offset of interrupt mask address
	int_sence_off		:		CDIOBI_PORT_DM_INTSTS,				// Offset of interrupt sence address
	int_ctrl_off		:		0,									// Offset of interrupt control address
	inp_port_num		:		4,									// Number of input ports
	out_port_num		:		4,									// Number of output ports
	min_filter_value	:		0,									// Minimum value of filter
	max_filter_value	:		0,									// Maximum value of filter
	int_bit				:		{0x0f,0},							// Interrupt bit (valid bit is 1)
	max_int_bit			:		4,									// Maximum number of interrupt bits
	num_of_8255			:		0,									// Number of 8255 chips
	can_echo_back		:		1,									// Echo back available? (0: Unavailable, 1: Available)
	is_positive			:		1									// Positive logic? (0: Negative logic, 1: Positive logic)
	},
	{// DIO-32DM3-PE
	board_name			:		"DIO-32DM3-PE",						// Board name
	board_type			:		CDIOBI_BT_PCI_DM,					// Board type
	device_type			:		DEVICE_TYPE_PCI,					// PCI bus
	vendor_id			:		0x1221,								// Vendor ID
	device_id			:		0xA642,								// Device ID
	resource_type		:		SRC_TYPE_MEMIO,						// Revision type (1: Not use, 2: Present in PCI configuration register, 3: Present on port map)
	port_num			:		0,									// Number of occupied ports
	inp_port_off		:		0xB000000,							// Offset of input port address
	out_port_off		:		0xB000000,							// Offset of output port address
	filter_off			:		0,									// Offset of filter address
	int_mask_off		:		CDIOBI_PORT_DM3_INTMASK,			// Offset of interrupt mask address
	int_sence_off		:		CDIOBI_PORT_DM3_INTSTS,				// Offset of interrupt sence address
	int_ctrl_off		:		0,								// Offset of interrupt control address
	inp_port_num		:		4,									// Number of input ports
	out_port_num		:		4,									// Number of output ports
	min_filter_value	:		0,									// Minimum value of filter
	max_filter_value	:		0,									// Maximum value of filter
	int_bit				:		{0x0f,0},							// Interrupt bit (valid bit is 1)
	max_int_bit			:		4,									// Maximum number of interrupt bits
	num_of_8255			:		0,									// Number of 8255 chips
	can_echo_back		:		1,									// Echo back available? (0: Unavailable, 1: Available)
	is_positive			:		1									// Positive logic? (0: Negative logic, 1: Positive logic)						
	},
	{// DI-128RL-PCI
	board_name			:		"DI-128RL-PCI",						// Board name
	board_type			:		CDIOBI_BT_PCI_TATEN,				// Board type
	device_type			:		DEVICE_TYPE_PCI,					// PCI bus
	vendor_id			:		0x1221,								// Vendor ID
	device_id			:		0xB1A2,								// Device ID
	resource_type		:		SRC_TYPE_PORTIO,					// Revision type (1: Not use, 2: Present in PCI configuration register, 3: Present on port map)
	port_num			:		32,									// Number of occupied ports
	inp_port_off		:		0,									// Offset of input port address
	out_port_off		:		0,									// Offset of output port address
	filter_off			:		CDIOBI_PORT_TATEN_FILTER,			// Offset of filter address
	int_mask_off		:		CDIOBI_PORT_TATEN_INTMASK,			// Offset of interrupt mask address
	int_sence_off		:		CDIOBI_PORT_TATEN_INTSTS,			// Offset of interrupt sence address
	int_ctrl_off		:		CDIOBI_PORT_TATEN_INTCTRL,			// Offset of interrupt control address
	inp_port_num		:		16,									// Number of input ports
	out_port_num		:		0,									// Number of output ports
	min_filter_value	:		0,									// Minimum value of filter
	max_filter_value	:		20,									// Maximum value of filter
	int_bit				:		{0xff,0xff,0},						// Interrupt bit (valid bit is 1)
	max_int_bit			:		16,									// Maximum number of interrupt bits
	num_of_8255			:		0,									// Number of 8255 chips
	can_echo_back		:		0,									// Echo back available? (0: Unavailable, 1: Available)
	is_positive			:		1									// Positive logic? (0: Negative logic, 1: Positive logic)
	},
	{// DO-128RL-PCI
	board_name			:		"DO-128RL-PCI",						// Board name
	board_type			:		CDIOBI_BT_PCI_TATEN,				// Board type
	device_type			:		DEVICE_TYPE_PCI,					// PCI bus
	vendor_id			:		0x1221,								// Vendor ID
	device_id			:		0xB1B2,								// Device ID
	resource_type		:		SRC_TYPE_PORTIO,					// Revision type (1: Not use, 2: Present in PCI configuration register, 3: Present on port map)
	port_num			:		32,									// Number of occupied ports
	inp_port_off		:		0,									// Offset of input port address
	out_port_off		:		0,									// Offset of output port address
	filter_off			:		0,									// Offset of filter address
	int_mask_off		:		0,									// Offset of interrupt mask address
	int_sence_off		:		0,									// Offset of interrupt sence address
	int_ctrl_off		:		0,									// Offset of interrupt control address
	inp_port_num		:		0,									// Number of input ports
	out_port_num		:		16,									// Number of output ports
	min_filter_value	:		0,									// Minimum value of filter
	max_filter_value	:		0,									// Maximum value of filter
	int_bit				:		{0},								// Interrupt bit (valid bit is 1)
	max_int_bit			:		0,									// Maximum number of interrupt bits
	num_of_8255			:		0,									// Number of 8255 chips
	can_echo_back		:		1,									// Echo back available? (0: Unavailable, 1: Available)
	is_positive			:		1									// Positive logic? (0: Negative logic, 1: Positive logic)
	},
// CONPROSYS
	{// CPS-DIO-0808L
	board_name			:		"CPS-DIO-0808L",					// Board name
	board_type			:		CDIOBI_BT_PCI_CONPROSYS,			// Board type
	device_type			:		DEVICE_TYPE_PCI,					// PCI bus
	vendor_id			:		0x1221,								// Vendor ID
	device_id			:		0xE101,								// Device ID
	resource_type		:		SRC_TYPE_PORTIO,					// Revision type (1: Not use, 2: Present in PCI configuration register, 3: Present on port map)
	port_num			:		48,									// Number of occupied ports
	inp_port_off		:		0x10,								// Offset of input port address
	out_port_off		:		0x12,								// Offset of output port address
	filter_off			:		CDIOBI_PORT_CONPROSYS_FILTER,		// Offset of filter address
	int_mask_off		:		CDIOBI_PORT_CONPROSYS_INTMASK,		// Offset of interrupt mask address
	int_sence_off		:		CDIOBI_PORT_CONPROSYS_INTSTS,		// Offset of interrupt sence address
	int_ctrl_off		:		CDIOBI_PORT_CONPROSYS_INTCTRL,		// Offset of interrupt control address
	inp_port_num		:		1,									// Number of input ports
	out_port_num		:		1,									// Number of output ports
	min_filter_value	:		0,									// Minimum value of filter
	max_filter_value	:		20,									// Maximum value of filter
	int_bit				:		{0xff},								// Interrupt bit (valid bit is 1)
	max_int_bit			:		8,									// Maximum number of interrupt bits
	num_of_8255			:		0,									// Number of 8255 chips
	can_echo_back		:		1,									// Echo back available? (0: Unavailable, 1: Available)
	is_positive			:		0									// Positive logic? (0: Negative logic, 1: Positive logic)
	},
	{// CPS-DIO-0808BL
	board_name			:		"CPS-DIO-0808BL",					// Board name
	board_type			:		CDIOBI_BT_PCI_CONPROSYS,			// Board type
	device_type			:		DEVICE_TYPE_PCI,					// PCI bus
	vendor_id			:		0x1221,								// Vendor ID
	device_id			:		0xE102,								// Device ID
	resource_type		:		SRC_TYPE_PORTIO,					// Revision type (1: Not use, 2: Present in PCI configuration register, 3: Present on port map)
	port_num			:		48,									// Number of occupied ports
	inp_port_off		:		0x10,								// Offset of input port address
	out_port_off		:		0x12,								// Offset of output port address
	filter_off			:		CDIOBI_PORT_CONPROSYS_FILTER,		// Offset of filter address
	int_mask_off		:		CDIOBI_PORT_CONPROSYS_INTMASK,		// Offset of interrupt mask address
	int_sence_off		:		CDIOBI_PORT_CONPROSYS_INTSTS,		// Offset of interrupt sence address
	int_ctrl_off		:		CDIOBI_PORT_CONPROSYS_INTCTRL,		// Offset of interrupt control address
	inp_port_num		:		1,									// Number of input ports
	out_port_num		:		1,									// Number of output ports
	min_filter_value	:		0,									// Minimum value of filter
	max_filter_value	:		20,									// Maximum value of filter
	int_bit				:		{0xff},								// Interrupt bit (valid bit is 1)
	max_int_bit			:		8,									// Maximum number of interrupt bits
	num_of_8255			:		0,									// Number of 8255 chips
	can_echo_back		:		1,									// Echo back available? (0: Unavailable, 1: Available)
	is_positive			:		0									// Positive logic? (0: Negative logic, 1: Positive logic)
	},
	{// CPS-DIO-0808RL
	board_name			:		"CPS-DIO-0808RL",					// Board name
	board_type			:		CDIOBI_BT_PCI_CONPROSYS,			// Board type
	device_type			:		DEVICE_TYPE_PCI,					// PCI bus
	vendor_id			:		0x1221,								// Vendor ID
	device_id			:		0xE10A,								// Device ID
	resource_type		:		SRC_TYPE_PORTIO,					// Revision type (1: Not use, 2: Present in PCI configuration register, 3: Present on port map)
	port_num			:		48,									// Number of occupied ports
	inp_port_off		:		0x10,								// Offset of input port address
	out_port_off		:		0x12,								// Offset of output port address
	filter_off			:		CDIOBI_PORT_CONPROSYS_FILTER,		// Offset of filter address
	int_mask_off		:		CDIOBI_PORT_CONPROSYS_INTMASK,		// Offset of interrupt mask address
	int_sence_off		:		CDIOBI_PORT_CONPROSYS_INTSTS,		// Offset of interrupt sence address
	int_ctrl_off		:		CDIOBI_PORT_CONPROSYS_INTCTRL,		// Offset of interrupt control address
	inp_port_num		:		1,									// Number of input ports
	out_port_num		:		1,									// Number of output ports
	min_filter_value	:		0,									// Minimum value of filter
	max_filter_value	:		20,									// Maximum value of filter
	int_bit				:		{0xff},								// Interrupt bit (valid bit is 1)
	max_int_bit			:		8,									// Maximum number of interrupt bits
	num_of_8255			:		0,									// Number of 8255 chips
	can_echo_back		:		1,									// Echo back available? (0: Unavailable, 1: Available)
	is_positive			:		0									// Positive logic? (0: Negative logic, 1: Positive logic)
	},
	{// CPS-RRY-4PCC
	board_name			:		"CPS-RRY-4PCC",						// Board name
	board_type			:		CDIOBI_BT_PCI_CONPROSYS,			// Board type
	device_type			:		DEVICE_TYPE_PCI,					// PCI bus
	vendor_id			:		0x1221,								// Vendor ID
	device_id			:		0xE10B,								// Device ID
	resource_type		:		SRC_TYPE_PORTIO,					// Revision type (1: Not use, 2: Present in PCI configuration register, 3: Present on port map)
	port_num			:		48,									// Number of occupied ports
	inp_port_off		:		0x0,								// Offset of input port address
	out_port_off		:		0x12,								// Offset of output port address
	filter_off			:		0,									// Offset of filter address
	int_mask_off		:		0,									// Offset of interrupt mask address
	int_sence_off		:		0,									// Offset of interrupt sence address
	int_ctrl_off		:		0,									// Offset of interrupt control address
	inp_port_num		:		0,									// Number of input ports
	out_port_num		:		1,									// Number of output ports
	min_filter_value	:		0,									// Minimum value of filter
	max_filter_value	:		0,									// Maximum value of filter
	int_bit				:		{0x00},								// Interrupt bit (valid bit is 1)
	max_int_bit			:		0,									// Maximum number of interrupt bits
	num_of_8255			:		0,									// Number of 8255 chips
	can_echo_back		:		1,									// Echo back available? (0: Unavailable, 1: Available)
	is_positive			:		0									// Positive logic? (0: Negative logic, 1: Positive logic)
	},
	{// CPS-DI-16L
	board_name			:		"CPS-DI-16L",						// Board name
	board_type			:		CDIOBI_BT_PCI_CONPROSYS,			// Board type
	device_type			:		DEVICE_TYPE_PCI,					// PCI bus
	vendor_id			:		0x1221,								// Vendor ID
	device_id			:		0xE110,								// Device ID
	resource_type		:		SRC_TYPE_PORTIO,					// Revision type (1: Not use, 2: Present in PCI configuration register, 3: Present on port map)
	port_num			:		48,									// Number of occupied ports
	inp_port_off		:		0x10,								// Offset of input port address
	out_port_off		:		0x0,								// Offset of output port address
	filter_off			:		CDIOBI_PORT_CONPROSYS_FILTER,		// Offset of filter address
	int_mask_off		:		CDIOBI_PORT_CONPROSYS_INTMASK,		// Offset of interrupt mask address
	int_sence_off		:		CDIOBI_PORT_CONPROSYS_INTSTS,		// Offset of interrupt sence address
	int_ctrl_off		:		CDIOBI_PORT_CONPROSYS_INTCTRL,		// Offset of interrupt control address
	inp_port_num		:		2,									// Number of input ports
	out_port_num		:		0,									// Number of output ports
	min_filter_value	:		0,									// Minimum value of filter
	max_filter_value	:		20,									// Maximum value of filter
	int_bit				:		{0xff,0xff},						// Interrupt bit (valid bit is 1)
	max_int_bit			:		16,									// Maximum number of interrupt bits
	num_of_8255			:		0,									// Number of 8255 chips
	can_echo_back		:		0,									// Echo back available? (0: Unavailable, 1: Available)
	is_positive			:		0									// Positive logic? (0: Negative logic, 1: Positive logic)
	},
	{// CPS-DI-16RL
	board_name			:		"CPS-DI-16RL",						// Board name
	board_type			:		CDIOBI_BT_PCI_CONPROSYS,			// Board type
	device_type			:		DEVICE_TYPE_PCI,					// PCI bus
	vendor_id			:		0x1221,								// Vendor ID
	device_id			:		0xE114,								// Device ID
	resource_type		:		SRC_TYPE_PORTIO,					// Revision type (1: Not use, 2: Present in PCI configuration register, 3: Present on port map)
	port_num			:		48,									// Number of occupied ports
	inp_port_off		:		0x10,								// Offset of input port address
	out_port_off		:		0x0,								// Offset of output port address
	filter_off			:		CDIOBI_PORT_CONPROSYS_FILTER,		// Offset of filter address
	int_mask_off		:		CDIOBI_PORT_CONPROSYS_INTMASK,		// Offset of interrupt mask address
	int_sence_off		:		CDIOBI_PORT_CONPROSYS_INTSTS,		// Offset of interrupt sence address
	int_ctrl_off		:		CDIOBI_PORT_CONPROSYS_INTCTRL,		// Offset of interrupt control address
	inp_port_num		:		2,									// Number of input ports
	out_port_num		:		0,									// Number of output ports
	min_filter_value	:		0,									// Minimum value of filter
	max_filter_value	:		20,									// Maximum value of filter
	int_bit				:		{0xff,0xff},						// Interrupt bit (valid bit is 1)
	max_int_bit			:		16,									// Maximum number of interrupt bits
	num_of_8255			:		0,									// Number of 8255 chips
	can_echo_back		:		0,									// Echo back available? (0: Unavailable, 1: Available)
	is_positive			:		0									// Positive logic? (0: Negative logic, 1: Positive logic)
	},
	{// CPS-DO-16L
	board_name			:		"CPS-DO-16L",						// Board name
	board_type			:		CDIOBI_BT_PCI_CONPROSYS,			// Board type
	device_type			:		DEVICE_TYPE_PCI,					// PCI bus
	vendor_id			:		0x1221,								// Vendor ID
	device_id			:		0xE112,								// Device ID
	resource_type		:		SRC_TYPE_PORTIO,					// Revision type (1: Not use, 2: Present in PCI configuration register, 3: Present on port map)
	port_num			:		48,									// Number of occupied ports
	inp_port_off		:		0x0,								// Offset of input port address
	out_port_off		:		0x12,								// Offset of output port address
	filter_off			:		0,									// Offset of filter address
	int_mask_off		:		0,									// Offset of interrupt mask address
	int_sence_off		:		0,									// Offset of interrupt sence address
	int_ctrl_off		:		0,									// Offset of interrupt control address
	inp_port_num		:		0,									// Number of input ports
	out_port_num		:		2,									// Number of output ports
	min_filter_value	:		0,									// Minimum value of filter
	max_filter_value	:		0,									// Maximum value of filter
	int_bit				:		{0x00},								// Interrupt bit (valid bit is 1)
	max_int_bit			:		0,									// Maximum number of interrupt bits
	num_of_8255			:		0,									// Number of 8255 chips
	can_echo_back		:		1,									// Echo back available? (0: Unavailable, 1: Available)
	is_positive			:		0									// Positive logic? (0: Negative logic, 1: Positive logic)
	},
	{// CPS-DO-16RL
	board_name			:		"CPS-DO-16RL",						// Board name
	board_type			:		CDIOBI_BT_PCI_CONPROSYS,			// Board type
	device_type			:		DEVICE_TYPE_PCI,					// PCI bus
	vendor_id			:		0x1221,								// Vendor ID
	device_id			:		0xE116,								// Device ID
	resource_type		:		SRC_TYPE_PORTIO,					// Revision type (1: Not use, 2: Present in PCI configuration register, 3: Present on port map)
	port_num			:		48,									// Number of occupied ports
	inp_port_off		:		0x0,								// Offset of input port address
	out_port_off		:		0x12,								// Offset of output port address
	filter_off			:		0,									// Offset of filter address
	int_mask_off		:		0,									// Offset of interrupt mask address
	int_sence_off		:		0,									// Offset of interrupt sence address
	int_ctrl_off		:		0,									// Offset of interrupt control address
	inp_port_num		:		0,									// Number of input ports
	out_port_num		:		2,									// Number of output ports
	min_filter_value	:		0,									// Minimum value of filter
	max_filter_value	:		0,									// Maximum value of filter
	int_bit				:		{0x00},								// Interrupt bit (valid bit is 1)
	max_int_bit			:		0,									// Maximum number of interrupt bits
	num_of_8255			:		0,									// Number of 8255 chips
	can_echo_back		:		1,									// Echo back available? (0: Unavailable, 1: Available)
	is_positive			:		0									// Positive logic? (0: Negative logic, 1: Positive logic)
	},
	{// CPS-BXC200
	board_name			:		"DIO-CPS-BXC200",					// Board name
	board_type			:		CDIOBI_BT_DEMO,						// Board type
	device_type			:		DEVICE_TYPE_DEMO,					// PCI bus
	vendor_id			:		0x1221,								// Vendor ID
	device_id			:		0x0000,								// Device ID
	resource_type		:		SRC_TYPE_UNUSED,					// Revision type (1: Not use, 2: Present in PCI configuration register, 3: Present on port map)
	port_num			:		0,									// Number of occupied ports
	inp_port_off		:		0,									// Offset of input port address
	out_port_off		:		0,									// Offset of output port address
	filter_off			:		0,									// Offset of filter address
	int_mask_off		:		0,									// Offset of interrupt mask address
	int_sence_off		:		0,									// Offset of interrupt sence address
	int_ctrl_off		:		0,									// Offset of interrupt control address
	inp_port_num		:		1,									// Number of input ports
	out_port_num		:		1,									// Number of output ports
	min_filter_value	:		0,									// Minimum value of filter
	max_filter_value	:		0,									// Maximum value of filter
	int_bit				:		{0},								// Interrupt bit (valid bit is 1)
	max_int_bit			:		0,									// Maximum number of interrupt bits
	num_of_8255			:		0,									// Number of 8255 chips
	can_echo_back		:		0,									// Echo back available? (0: Unavailable, 1: Available)
	is_positive			:		1									// Positive logic? (0: Negative logic, 1: Positive logic)
	},
// End
	{// End
	board_name			:		"End",								// Board name
	board_type			:		0,									// Board type
	device_type			:		DEVICE_TYPE_PCI,					// PCI bus
	vendor_id			:		0,									// Vendor ID
	device_id			:		0,									// Device ID
	resource_type		:		0,									// Revision type (1: Not use, 2: Present in PCI configuration register, 3: Present on port map)
	port_num			:		0,									// Number of occupied ports
	inp_port_off		:		0,									// Offset of input port address
	out_port_off		:		0,									// Offset of output port address
	filter_off			:		0,									// Offset of filter address
	int_mask_off		:		0,									// Offset of interrupt mask address
	int_sence_off		:		0,									// Offset of interrupt sence address
	int_ctrl_off		:		0,									// Offset of interrupt control address
	inp_port_num		:		0,									// Number of input ports
	out_port_num		:		0,									// Number of output ports
	min_filter_value	:		0,									// Minimum value of filter
	max_filter_value	:		0,									// Maximum value of filter
	int_bit				:		{0},								// Interrupt bit (valid bit is 1)
	max_int_bit			:		0,									// Maximum number of interrupt bits
	num_of_8255			:		0,									// Number of 8255 chips
	can_echo_back		:		0,									// Echo back available? (0: Unavailable, 1: Available)
	is_positive			:		0									// Positive logic? (0: Negative logic, 1: Positive logic)
	}
};

//================================================================
// Function
//================================================================
//================================================================
// BIOS initialization
//================================================================
long Cdiobi_init_bios(PCDIOBI bios_d, char *board_name, unsigned long *io_addr, unsigned long irq_no)
{
	int				i;
	unsigned short	port_base;

	//----------------------------------------
	// Check the parameters
	//----------------------------------------
	if (bios_d == NULL ||
		board_name == NULL) {
		return -1;
	}
	//----------------------------------------
	// Initialize the BIOS data
	//----------------------------------------
	memset(bios_d, 0, sizeof(CDIOBI));
	//----------------------------------------
	// Set the board information
	//----------------------------------------
	for (i=0; strcmp(board_info[i].board_name, "End") != 0; i++) {
		if (strcmp(board_info[i].board_name, board_name) == 0) {
			bios_d->b_info = &board_info[i];
			break;
		}
	}
	if (bios_d->b_info == NULL) {
		return -1;
	}
	if (((io_addr == NULL) || (io_addr[0] == 0x0000)) && !(board_info[i].board_type & CDIOBI_BT_DEMO)) {
		return -1;
	}
	//----------------------------------------
	// Store the interrupt number
	//----------------------------------------
	bios_d->irq_no = (unsigned short)(irq_no & 0xffff);
	//----------------------------------------
	// Embed the port
	//----------------------------------------
	port_base = (unsigned short)(io_addr[0] & 0xffff);
	bios_d->port_base = port_base;							// Base address of port
	// Input address
	if (bios_d->b_info->inp_port_num != 0) {
		bios_d->inp_port = port_base + bios_d->b_info->inp_port_off;
	} else {
		bios_d->inp_port = 0;
	}
	// Output address
	if (bios_d->b_info->out_port_num != 0) {
		bios_d->out_port = port_base + bios_d->b_info->out_port_off;
	} else {
		bios_d->out_port = 0;
	}
	// Filter address
	if (bios_d->b_info->filter_off != 0) {
		bios_d->filter_port = port_base + bios_d->b_info->filter_off;
	} else {
		bios_d->filter_port = 0;
	}
	// Interrupt mask address
	if (bios_d->b_info->int_mask_off != 0) {
		bios_d->int_mask_port = port_base + bios_d->b_info->int_mask_off;
	} else {
		bios_d->int_mask_port = 0;
	}
	// Interrupt sence address
	if (bios_d->b_info->int_sence_off != 0) {
		bios_d->int_sence_port = port_base + bios_d->b_info->int_sence_off;
	} else {
		bios_d->int_sence_port = 0;
	}
	// Interrupt control address
	if (bios_d->b_info->int_ctrl_off != 0) {
		bios_d->int_ctrl_port = port_base + bios_d->b_info->int_ctrl_off;
	} else {
		bios_d->int_ctrl_port = 0;
	}
	//----------------------------------------
	// Set the initialized flag
	//----------------------------------------
	bios_d->flag = 1;										// 0: Uninitialized, 1: Initialized
	//----------------------------------------
	// Mask the interrupt
	//----------------------------------------
	Cdiobi_set_all_int_mask(bios_d, CDIOBI_MASK_ALL, NULL);	// Interrupt mask data (0: Open, 1: Close)
	//----------------------------------------
	// Initialize the others work
	//----------------------------------------
	memset(bios_d->out_data, 0, sizeof(bios_d->out_data));	// Output latch data
	for (i=0; i<bios_d->b_info->out_port_num; i++) {		// Initialize by echo back
		Cdiobi_echo_port(bios_d, i, &bios_d->out_data[i]);
	}
	memset(bios_d->int_ctrl, 0, sizeof(bios_d->int_ctrl));	// Interrupt control data (0: Rising, 1: Falling)
	bios_d->filter_value = 0;								// Digital filter value
	Cdiobi_set_digital_filter(bios_d, 0);
	bios_d->direction	= 0;								// Input/output direction

	for(i = 0; i < bios_d->b_info->num_of_8255; i++){
		Cdiobi_set_8255_mode(bios_d, i, 0x9B);
	}

	return 0;
}

//================================================================
// Function
//================================================================
//================================================================
// BIOS initialization for 32DM3
//================================================================
long Cdiobi_init_bios_mem(PCDIOBI bios_d, char *board_name, unsigned char *mem_addr, unsigned long irq_no)
{
	int				i;
	unsigned char *	mem_base;

	//----------------------------------------
	// Check the parameters
	//----------------------------------------
	if (bios_d == NULL ||
		board_name == NULL) {
		return -1;
	}
	//----------------------------------------
	// Initialize the BIOS data
	//----------------------------------------
	memset(bios_d, 0, sizeof(CDIOBI));
	//----------------------------------------
	// Set the board information
	//----------------------------------------
	for (i=0; strcmp(board_info[i].board_name, "End") != 0; i++) {
		if (strcmp(board_info[i].board_name, board_name) == 0) {
			bios_d->b_info = &board_info[i];
			break;
		}
	}
	if (bios_d->b_info == NULL) {
		return -1;
	}
	if ((mem_addr == NULL) && !(board_info[i].board_type & CDIOBI_BT_DEMO)) {
		return -1;
	}
	//----------------------------------------
	// Store the interrupt number
	//----------------------------------------
	bios_d->irq_no = (unsigned short)(irq_no & 0xffff);
	//----------------------------------------
	// Embed the port
	//----------------------------------------
	mem_base = (unsigned char *)mem_addr;
	bios_d->port_base_mem = mem_base;							// Base address of port
	// Input address
	if (bios_d->b_info->inp_port_num != 0) {
		bios_d->inp_port_mem = mem_base + bios_d->b_info->inp_port_off;
	} else {
		bios_d->inp_port_mem = 0;
	}
	// Output address
	if (bios_d->b_info->out_port_num != 0) {
		bios_d->out_port_mem = mem_base + bios_d->b_info->out_port_off;
	} else {
		bios_d->out_port_mem = 0;
	}
	// Filter address
	if (bios_d->b_info->filter_off != 0) {
		bios_d->filter_port_mem = mem_base + bios_d->b_info->filter_off;
	} else {
		bios_d->filter_port_mem = 0;
	}
	// Interrupt mask address
	if (bios_d->b_info->int_mask_off != 0) {
		bios_d->int_mask_port_mem = mem_base + bios_d->b_info->int_mask_off;
	} else {
		bios_d->int_mask_port_mem = 0;
	}
	// Interrupt sence address
	if (bios_d->b_info->int_sence_off != 0) {
		bios_d->int_sence_port_mem = mem_base + bios_d->b_info->int_sence_off;
	} else {
		bios_d->int_sence_port_mem = 0;
	}
	// Interrupt control address
	if (bios_d->b_info->int_ctrl_off != 0) {
		bios_d->int_ctrl_port_mem = mem_base + bios_d->b_info->int_ctrl_off;
	} else {
		bios_d->int_ctrl_port_mem = 0;
	}
	//----------------------------------------
	// Set the initialized flag
	//----------------------------------------
	bios_d->flag = 1;										// 0: Uninitialized, 1: Initialized
	//----------------------------------------
	// Mask the interrupt
	//----------------------------------------
	Cdiobi_set_all_int_mask(bios_d, CDIOBI_MASK_ALL, NULL);	// Interrupt mask data (0: Open, 1: Close)
	//----------------------------------------
	// Initialize the others work
	//----------------------------------------
	memset(bios_d->out_data, 0, sizeof(bios_d->out_data));	// Output latch data
	for (i=0; i<bios_d->b_info->out_port_num; i++) {		// Initialize by echo back
		Cdiobi_echo_port(bios_d, i, &bios_d->out_data[i]);
	}
	memset(bios_d->int_ctrl, 0, sizeof(bios_d->int_ctrl));	// Interrupt control data (0: Rising, 1: Falling)
	bios_d->filter_value = 0;								// Digital filter value
	Cdiobi_set_digital_filter(bios_d, 0);
	bios_d->direction	= 0;								// Input/output direction
	return 0;
}

//================================================================
// Get resource type
//================================================================
long Cdiobi_init_resource_type(char *board_name, unsigned char *resource_type)
{
	short				i;
	PCDIOBI_BOARD_INFO	b_info_max_rev_id;	// Address of board information structure (Points to data that has the same device name and the latest revision)
	PCDIOBI_BOARD_INFO	b_info;
	//----------------------------------------
	// Check parameter
	//----------------------------------------
	if (board_name == NULL ||
		resource_type == NULL) {
		return -1;
	}
	//----------------------------------------
	// Set board information
	//----------------------------------------
	b_info				= NULL;
	b_info_max_rev_id	= NULL;
	//----------------------------------------
	// Search board information
	//----------------------------------------
	for (i=0; strcmp(board_info[i].board_name, "End") != 0; i++) {
		//----------------------------------------
		// Matche board name 
		//----------------------------------------
		if (strcmp(board_info[i].board_name, board_name) == 0){
			//----------------------------------------
			// If the revision matches or is a demo device
			//----------------------------------------
			if(board_info[i].board_type & CDIOBI_BT_DEMO){
				b_info = &board_info[i];
				strcpy(board_name, board_info[i].board_name);
				break;
			//----------------------------------------
			// If the revision does not match and the board information pointer for temporary storage is NULL
			//----------------------------------------
			}else if(b_info_max_rev_id == NULL){
				b_info_max_rev_id = &board_info[i];
			//----------------------------------------
			// If the revisions does not match and it is newer than the previously stored revision of board information
			//----------------------------------------
			}
		}
	}
	//----------------------------------------
	// If the board name and revision do not match, but the device information of the old revision is found, use that information
	//----------------------------------------
	if ((b_info == NULL) && (b_info_max_rev_id != NULL)){
		b_info	= b_info_max_rev_id;
	}
	if (b_info == NULL) {
		return -1;
	}
	*resource_type	= b_info->resource_type;
	return 0;
}

//================================================================
// BIOS exit
//================================================================
void Cdiobi_clear_bios(PCDIOBI bios_d)
{
	//----------------------------------------
	// Initialize the BIOS data
	//----------------------------------------
	memset(bios_d, 0, sizeof(CDIOBI));
}

//================================================================
// Output latch data initialization
//================================================================
void Cdiobi_init_latch_data(PCDIOBI bios_d)
{
	int i;

	memset(bios_d->out_data, 0, sizeof(bios_d->out_data));	// Output latch data
	for (i=0; i<bios_d->b_info->out_port_num; i++) {		// Initialize according to the echo back
		Cdiobi_echo_port(bios_d, i, &bios_d->out_data[i]);
	}
}

//================================================================
// Port input
//================================================================
void Cdiobi_input_port(PCDIOBI bios_d, short port_no, unsigned char *data)
{
	unsigned long	dwdata;
	
	//----------------------------------------
	// Check the parameters
	//----------------------------------------
	if (bios_d == NULL ||
		bios_d->flag == 0 ||
		port_no < 0 ||
		data == NULL) {
		return;
	}
	if (strcmp(bios_d->b_info->board_name, "DIO-32DM3-PE") == 0){
		if ((bios_d->inp_port_mem == 0) && !(bios_d->b_info->board_type & CDIOBI_BT_DEMO)){
			return;
		}
	}else{
		if ((bios_d->inp_port == 0) && !(bios_d->b_info->board_type & CDIOBI_BT_DEMO)){
			return;
		}
	}
	//----------------------------------------
	// Check the port number
	//----------------------------------------
	if (Cdiobi_check_inp_port(bios_d, port_no) == 0) {
		return;
	}
	//----------------------------------------
	// Modify the 8255 port number
	//----------------------------------------
	if (bios_d->b_info->board_type & CDIOBI_BT_PCI_8255) {			// Board type: 8255
		Cdiobi_8255_port_set(bios_d, &port_no);
	}
	//----------------------------------------
	// Input
	//----------------------------------------
	if (strcmp(bios_d->b_info->board_name, "DIO-CPS-BXC200") == 0){
		if(wait_rdy()){
			return;
		}
		set_busy();
		set_data(1,0x90);
		set_data(9,1);
		set_drdy();
		if(wait_dir()){
			return;
		}
		*data = get_data(2);
		clr_stat();
	}else if (bios_d->b_info->board_type & CDIOBI_BT_PCI_DM) {		// Board type: DM
		if (strcmp(bios_d->b_info->board_name, "DIO-32DM3-PE") == 0){
			//----------------------------------------
			// For 16-point input and 16-point output, the output port returns latch data
			//----------------------------------------
			if ((bios_d->direction == PIO_1616) &&
				((port_no == 2) || (port_no == 3))){
				*data	= bios_d->out_data[port_no];
			} else {
				dwdata	= _inmd(bios_d->inp_port_mem + ((port_no / 4) * 4));	// DWORD access
				*data	= (unsigned char)(dwdata >> ((port_no % 4) * 8));
			}
		}else{
			dwdata	= _inpd(bios_d->inp_port + ((port_no / 4) * 4));	// DWORD access
			*data	= dwdata >> ((port_no % 4) * 8);
		}
	}else{
		*data	= _inp(bios_d->inp_port + port_no);
	}
}

//================================================================
// Bit input
//================================================================
void Cdiobi_input_bit(PCDIOBI bios_d, short bit_no, unsigned char *data)
{
	unsigned char	tmp;
	unsigned long	dwdata;
	short			port_no;
	
	//----------------------------------------
	// Check the parameters
	//----------------------------------------
	if (bios_d == NULL ||
		bios_d->flag == 0 ||
		bit_no < 0 ||
		data == NULL) {
		return;
	}
	if (strcmp(bios_d->b_info->board_name, "DIO-32DM3-PE") == 0){
		if ((bios_d->inp_port_mem == 0) && !(bios_d->b_info->board_type & CDIOBI_BT_DEMO)){
			return;
		}
	}else{
		if ((bios_d->inp_port == 0) && !(bios_d->b_info->board_type & CDIOBI_BT_DEMO)){
			return;
		}
	}
	//----------------------------------------
	// Check the bit number
	//----------------------------------------
	if (Cdiobi_check_inp_bit(bios_d, bit_no) == 0) {
		return;
	}
	port_no = bit_no / 8;
	//----------------------------------------
	// Modify the 8255 port number
	//----------------------------------------
	if (bios_d->b_info->board_type & CDIOBI_BT_PCI_8255) {			// Board type: 8255
		Cdiobi_8255_port_set(bios_d, &port_no);
	}
	//----------------------------------------
	// Input
	//----------------------------------------
	if (strcmp(bios_d->b_info->board_name, "DIO-CPS-BXC200") == 0){
		if(wait_rdy()){
			return;
		}
		set_busy();
		set_data(1,0x90);
		set_data(9,1);
		set_drdy();
		if(wait_dir()){
			return;
		}
		tmp = get_data(2);
		clr_stat();
	}else if (bios_d->b_info->board_type & CDIOBI_BT_PCI_DM) {		// Board type: DM
		if (strcmp(bios_d->b_info->board_name, "DIO-32DM3-PE") == 0){
			//----------------------------------------
			// For 16-point input and 16-point output, the output port returns latch data
			//----------------------------------------
			if ((bios_d->direction == PIO_1616) &&
				((port_no == 2) || (port_no == 3))){
				tmp		= bios_d->out_data[port_no];
			} else {
				dwdata	= _inmd(bios_d->inp_port_mem + ((port_no / 4) * 4));	// DWORD access
				tmp		= (unsigned char)(dwdata >> ((port_no % 4) * 8));
			}
		}else{
			dwdata	= _inpd(bios_d->inp_port + ((port_no / 4) * 4));	// DWORD access
			tmp		= dwdata >> ((port_no % 4) * 8);
		}
	}else{
		tmp		= _inp(bios_d->inp_port + port_no);
	}
	*data	= (tmp >> (bit_no % 8)) & 0x01;
}

//================================================================
// Port output
//================================================================
void Cdiobi_output_port(PCDIOBI bios_d, short port_no, unsigned char data)
{
	unsigned long	dwdata;
	short			port_no_dw, i;

	//----------------------------------------
	// Check the parameters
	//----------------------------------------
	if (bios_d == NULL ||
		bios_d->flag == 0 ||
		port_no < 0) {
		return;
	}
	if (strcmp(bios_d->b_info->board_name, "DIO-32DM3-PE") == 0){
		if ((bios_d->out_port_mem == 0) && !(bios_d->b_info->board_type & CDIOBI_BT_DEMO)){
			return;
		}
	}else{
		if ((bios_d->out_port == 0) && !(bios_d->b_info->board_type & CDIOBI_BT_DEMO)){
			return;
		}
	}
	//----------------------------------------
	// Check the port number
	//----------------------------------------
	if (Cdiobi_check_out_port(bios_d, port_no) == 0) {
		return;
	}
	//----------------------------------------
	// Save the output data
	//----------------------------------------
	bios_d->out_data[port_no] = data;
	//----------------------------------------
	// Modify the 8255 port number
	//----------------------------------------
	if (bios_d->b_info->board_type & CDIOBI_BT_PCI_8255) {	// Board type: 8255
		Cdiobi_8255_port_set(bios_d, &port_no);
	}
	//----------------------------------------
	// Output
	//----------------------------------------
	if (strcmp(bios_d->b_info->board_name, "DIO-CPS-BXC200") == 0){
		if(wait_rdy()){
			return;
		}
		set_busy();
		set_data(1,0x91);
		set_data(2,(data + 1));
		set_data(9,2);
		set_drdy();
		clr_stat();
	}else if (bios_d->b_info->board_type & CDIOBI_BT_PCI_DM) {	// Board type: DM
	if (strcmp(bios_d->b_info->board_name, "DIO-32DM3-PE") == 0){
			// Create the output data
			dwdata	= 0;
			port_no_dw	= port_no / 4;	// What number of port in the DWORD unit
			for(i = 3; i >= 0; i--){
				dwdata	= (dwdata << 8) | (unsigned long)bios_d->out_data[port_no_dw * 4 + i];
			}
			_outmd(bios_d->out_port_mem + (port_no_dw * 4), dwdata);
	}else{
			// Create the output data
			dwdata		= 0;
			port_no_dw	= port_no / 4;	// What number of port in the DWORD unit
			for(i = 3; i >= 0; i--){
				dwdata	= (dwdata << 8) | (unsigned long)bios_d->out_data[port_no_dw * 4 + i];
			}
			_outpd(bios_d->out_port + (port_no_dw * 4), (unsigned int)dwdata);
		}
	}else{
		_outp(bios_d->out_port + port_no, data);
	}
}

//================================================================
// Bit output
//================================================================
void Cdiobi_output_bit(PCDIOBI bios_d, short bit_no, unsigned char data)
{
	unsigned char	tmp;
	unsigned long	dwdata;
	short			port_no, port_no_dw, i;
	
	//----------------------------------------
	// Check the parameters
	//----------------------------------------
	if (bios_d == NULL ||
		bios_d->flag == 0 ||
		bit_no < 0) {
		return;
	}
	if (strcmp(bios_d->b_info->board_name, "DIO-32DM3-PE") == 0){
		if ((bios_d->out_port_mem == 0) && !(bios_d->b_info->board_type & CDIOBI_BT_DEMO)){
			return;
		}
	}else{
		if ((bios_d->out_port == 0) && !(bios_d->b_info->board_type & CDIOBI_BT_DEMO)){
			return;
		}
	}
	//----------------------------------------
	// Check the bit number
	//----------------------------------------
	if (Cdiobi_check_out_bit(bios_d, bit_no) == 0) {
		return;
	}
	//----------------------------------------
	// Create the output data
	//----------------------------------------
	port_no = bit_no / 8;
	if (data == 0) {
		tmp = bios_d->out_data[port_no] & ~(0x01 << (bit_no % 8));
	} else {
		tmp = bios_d->out_data[port_no] | (0x01 << (bit_no % 8));
	}
	//----------------------------------------
	// Save the output data
	//----------------------------------------
	bios_d->out_data[port_no] = tmp;
	//----------------------------------------
	// Modify the 8255 port number
	//----------------------------------------
	if (bios_d->b_info->board_type & CDIOBI_BT_PCI_8255) {	// Board type: 8255
		Cdiobi_8255_port_set(bios_d, &port_no);
	}
	//----------------------------------------
	// Output
	//----------------------------------------
	if (strcmp(bios_d->b_info->board_name, "DIO-CPS-BXC200") == 0){
		if(wait_rdy()){
			return;
		}
		set_busy();
		set_data(1,0x91);
		set_data(2,(tmp + 1));
		set_data(9,2);
		set_drdy();
		clr_stat();
	}else if (bios_d->b_info->board_type & CDIOBI_BT_PCI_DM) {	// Board type: DM
		if (strcmp(bios_d->b_info->board_name, "DIO-32DM3-PE") == 0){
			// Create the output data
			dwdata	= 0;
			port_no_dw	= port_no / 4;	// What number of port in the DWORD unit
			for(i = 3; i >= 0; i--){
				dwdata	= (dwdata << 8) | (unsigned long)bios_d->out_data[port_no_dw * 4 + i];
			}
			_outmd(bios_d->out_port_mem + (port_no_dw * 4), dwdata);
		}else{
			// Create the output data
			dwdata		= 0;
			port_no_dw	= port_no / 4;	// What number of port in the DWORD unit
			for(i = 3; i >= 0; i--){
				dwdata	= (dwdata << 8) | (unsigned long)bios_d->out_data[port_no_dw * 4 + i];
			}
			_outpd(bios_d->out_port + (port_no_dw * 4), dwdata);
		}
	}else{
		_outp(bios_d->out_port + port_no, tmp);
	}
}

//================================================================
// Port echo back
//================================================================
void Cdiobi_echo_port(PCDIOBI bios_d, short port_no, unsigned char *data)
{
	unsigned long	dwdata;

	//----------------------------------------
	// Check the parameters
	//----------------------------------------
	if (bios_d == NULL ||
		bios_d->flag == 0 ||
		port_no < 0 ||
		data == NULL) {
		return;
	}
	if (strcmp(bios_d->b_info->board_name, "DIO-32DM3-PE") == 0){
		if ((bios_d->out_port_mem == 0) && !(bios_d->b_info->board_type & CDIOBI_BT_DEMO)){
			return;
		}
	}else{
		if ((bios_d->out_port == 0) && !(bios_d->b_info->board_type & CDIOBI_BT_DEMO)){
			return;
		}
	}
	//----------------------------------------
	// Check the port number
	//----------------------------------------
	if (Cdiobi_check_out_port(bios_d, port_no) == 0) {
		return;
	}
	//----------------------------------------
	// Echo back
	//----------------------------------------
	if (bios_d->b_info->can_echo_back == 0) {	// Echo back available? (0: Unavailable, 1: Available)
		*data = bios_d->out_data[port_no];
	} else {
		//----------------------------------------
		// Modify the 8255 port number
		//----------------------------------------
		if (bios_d->b_info->board_type & CDIOBI_BT_PCI_8255) {	// Board type: 8255
			Cdiobi_8255_port_set(bios_d, &port_no);
		}
		if (bios_d->b_info->board_type & CDIOBI_BT_PCI_DM) {			// Board type: DM
			if (strcmp(bios_d->b_info->board_name, "DIO-32DM3-PE") == 0){
				//----------------------------------------
				// For 16-point input and 16-point output, the output port returns latch data
				//----------------------------------------
				if ((bios_d->direction == PIO_1616) &&
					((port_no == 2) || (port_no == 3))){
					*data	= bios_d->out_data[port_no];
				} else {
					dwdata	= _inmd(bios_d->out_port_mem + ((port_no / 4) * 4));	// DWORD access
					*data	= (unsigned char)(dwdata >> ((port_no % 4) * 8));
				}
			}else{
				dwdata	= _inpd(bios_d->out_port + ((port_no / 4) * 4));	// DWORD access
				*data	= dwdata >> ((port_no % 4) * 8);
			}
		}else{
			*data	= _inp(bios_d->out_port + port_no);
		}
	}
}

//================================================================
// Bit echo back
//================================================================
void Cdiobi_echo_bit(PCDIOBI bios_d, short bit_no, unsigned char *data)
{
	unsigned char	tmp;
	short			port_no;
	unsigned long	dwdata;
	
	//----------------------------------------
	// Check the parameters
	//----------------------------------------
	if (bios_d == NULL ||
		bios_d->flag == 0 ||
		bit_no < 0 ||
		data == NULL) {
		return;
	}
	if (strcmp(bios_d->b_info->board_name, "DIO-32DM3-PE") == 0){
		if ((bios_d->out_port_mem == 0) && !(bios_d->b_info->board_type & CDIOBI_BT_DEMO)){
			return;
		}
	}else{
		if ((bios_d->out_port == 0) && !(bios_d->b_info->board_type & CDIOBI_BT_DEMO)){
			return;
		}
	}
	//----------------------------------------
	// Check the bit number
	//----------------------------------------
	if (Cdiobi_check_out_bit(bios_d, bit_no) == 0) {
		return;
	}
	//----------------------------------------
	// Echo back
	//----------------------------------------
	port_no	= bit_no / 8;
	//----------------------------------------
	// Modify the 8255 port number
	//----------------------------------------
	if (bios_d->b_info->board_type & CDIOBI_BT_PCI_8255) {	// Board type: 8255
		Cdiobi_8255_port_set(bios_d, &port_no);
	}
	if (bios_d->b_info->can_echo_back == 0) {				// Echo back available? (0: Unavailable, 1: Available)
		tmp = bios_d->out_data[port_no];
	} else {
		if (bios_d->b_info->board_type & CDIOBI_BT_PCI_DM) {			// Board type: DM
		if (strcmp(bios_d->b_info->board_name, "DIO-32DM3-PE") == 0){
				//----------------------------------------
				// For 16-point input and 16-point output, the output port returns latch data
				//----------------------------------------
				if ((bios_d->direction == PIO_1616) &&
					((port_no == 2) || (port_no == 3))){
					tmp		= bios_d->out_data[port_no];
				} else {
					dwdata	= _inmd(bios_d->out_port_mem + ((port_no / 4) * 4));	// DWORD access
					tmp		= (unsigned char)(dwdata >> ((port_no % 4) * 8));
				}
			}else{
				dwdata	= _inpd(bios_d->out_port + ((port_no / 4) * 4));	// DWORD access
				tmp		= dwdata >> ((port_no % 4) * 8);
			}
		}else{
			tmp		= _inp(bios_d->out_port + port_no);
		}
	}
	*data = (tmp >> (bit_no % 8)) & 0x01;
}

//================================================================
// Digital filter setting
//================================================================
long Cdiobi_set_digital_filter(PCDIOBI bios_d, unsigned long filter_value)
{
	//----------------------------------------
	// Check the parameters
	//----------------------------------------
	if (strcmp(bios_d->b_info->board_name, "DIO-32DM3-PE") == 0){
		if (bios_d == NULL ||
			bios_d->flag == 0 ||
			bios_d->filter_port_mem == 0) {
			return -1;		// DIO_ERR_SYS_NOT_SUPPORTED
		}
	}else{
		if (bios_d == NULL ||
			bios_d->flag == 0 ||
			bios_d->filter_port == 0) {
			return -1;		// DIO_ERR_SYS_NOT_SUPPORTED
		}
	}
	if (filter_value < bios_d->b_info->min_filter_value ||
		filter_value > bios_d->b_info->max_filter_value) {
		return -2;		// DIO_ERR_SYS_FILTER
	}
	//----------------------------------------
	// Set the digiter filter
	//----------------------------------------
	bios_d->filter_value = filter_value;
	_outp(bios_d->filter_port, (unsigned char)(filter_value & 0xff));
	return 0;
}

//================================================================
// Retrieve the digital filter
//================================================================
long Cdiobi_get_digital_filter(PCDIOBI bios_d, unsigned long *filter_value)
{
	//----------------------------------------
	// Check the parameters
	//----------------------------------------
	if (strcmp(bios_d->b_info->board_name, "DIO-32DM3-PE") == 0){
		if (bios_d == NULL ||
			bios_d->flag == 0 ||
			bios_d->filter_port_mem == 0 ||
			filter_value == NULL) {
			return -1;		// DIO_ERR_SYS_NOT_SUPPORTED
		}
	}else{
		if (bios_d == NULL ||
			bios_d->flag == 0 ||
			bios_d->filter_port == 0 ||
			filter_value == NULL) {
			return -1;		// DIO_ERR_SYS_NOT_SUPPORTED
		}
	}
	//----------------------------------------
	// Return the digital filter
	//----------------------------------------
	*filter_value = bios_d->filter_value;
	return 0;
}

//================================================================
// 8255 mode setting
//================================================================
long Cdiobi_set_8255_mode(PCDIOBI bios_d, unsigned short chip_no, unsigned short ctrl_word)
{
	//----------------------------------------
	// Check the parameters
	//----------------------------------------
	if (bios_d == NULL ||
		bios_d->flag == 0 ||
		bios_d->b_info->num_of_8255 == 0) {
		return -1;		// DIO_ERR_SYS_NOT_SUPPORTED
	}
	if (chip_no > (bios_d->b_info->num_of_8255 - 1)){
		return -2;		// DIO_ERR_SYS_8255
	}
	//----------------------------------------
	// Set the control word
	//----------------------------------------
	bios_d->ctrl_word_8255[chip_no] = ctrl_word;
	_outp(bios_d->port_base + (chip_no * 4 + 3), ctrl_word);
	return 0;
}

//================================================================
// Retrieve the 8255 mode settting
//================================================================
long Cdiobi_get_8255_mode(PCDIOBI bios_d, unsigned short chip_no, unsigned short *ctrl_word)
{
	unsigned short		c_ctrl_word;
	int					copy_ret;
	//----------------------------------------
	// Check the parameters
	//----------------------------------------
	if (bios_d == NULL ||
		bios_d->flag == 0 ||
		bios_d->b_info->num_of_8255 == 0) {
		return -1;		// DIO_ERR_SYS_NOT_SUPPORTED
	}
	if (chip_no > (bios_d->b_info->num_of_8255 -1)) {
		return -2;		// DIO_ERR_SYS_8255
	}
	//----------------------------------------
	// Retrieve the control word setting
	//----------------------------------------
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5,0,0))
	if (access_ok(ctrl_word, sizeof(unsigned short)))
#else
	if (access_ok(VERIFY_READ, ctrl_word, sizeof(unsigned short)))
#endif
	{
		copy_ret = copy_from_user(&c_ctrl_word, ctrl_word, sizeof(unsigned short));
	} else {
		memcpy(&c_ctrl_word, ctrl_word, sizeof(unsigned short));
	}

	c_ctrl_word = bios_d->ctrl_word_8255[chip_no];

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5,0,0))
	if (access_ok(ctrl_word, sizeof(unsigned short)))
#else
	if (access_ok(VERIFY_READ, ctrl_word, sizeof(unsigned short)))
#endif
	{
		copy_ret = copy_to_user(ctrl_word, &c_ctrl_word, sizeof(unsigned short));
	} else {
		memcpy(ctrl_word, &c_ctrl_word, sizeof(unsigned short));
	}

	return 0;
}

//================================================================
//	Calculate I/O ports of the board using 8255 chip
//	Because I/O ports on the port map are not arranged consecutively,
//	allow using the existing I/O functions by shifting the port number.
//================================================================
void Cdiobi_8255_port_set(PCDIOBI bios_d, unsigned short *pport_no)
{
	//------------------------------
	// Set the port number PIO-48D(PCI)
	//------------------------------
	*pport_no = *pport_no*4/3;
	return;
}

//================================================================
// Mask all interrupts
//================================================================
void Cdiobi_set_all_int_mask(PCDIOBI bios_d, int flag, unsigned char *mask)
{
	unsigned char	b_data;
	unsigned short	w_data;
	unsigned long	d_data;
	short			port_no, real_port_no;

	//----------------------------------------
	// Check the parameters
	//----------------------------------------
	if (bios_d == NULL ||
		bios_d->flag == 0) {
		return;
	}
	if (strcmp(bios_d->b_info->board_name, "DIO-32DM3-PE") == 0){
		if (!(bios_d->b_info->board_type & CDIOBI_BT_DEMO) && (bios_d->int_mask_port_mem == 0)){
			return;
		}
	} else {
		if (!(bios_d->b_info->board_type & CDIOBI_BT_DEMO) && (bios_d->int_mask_port == 0)){
			return;
		}
	}
	if (flag != CDIOBI_MASK_ALL &&
		flag != CDIOBI_MASK_DATA) {
		return;
	}
	if (flag == CDIOBI_MASK_DATA && mask == NULL) {
		return;
	}
	//----------------------------------------
	// In the case of masking all the interrupts
	//----------------------------------------
	if (flag == CDIOBI_MASK_ALL) {
		// 4 channels of the input signals as interrupt for ASIC
		if (bios_d->b_info->board_type & CDIOBI_BT_PCI_ASIC) {		// Board type: ASIC
			b_data = 0x0f;						// Mask 4 bits
			bios_d->int_mask[0] = b_data;
			_outp(bios_d->int_mask_port, b_data);
		}
		// 16 channels of the input signals as interrupt for Multipoint
		if (bios_d->b_info->board_type & CDIOBI_BT_PCI_TATEN) {		// Board type: Multipoint
			w_data = 0xffff;
			bios_d->int_mask[0] = 0xff;			// Mask 16 bits
			bios_d->int_mask[1] = 0xff;
			_outpw(bios_d->int_mask_port, w_data);
		}
		// 32 and 16 channels of the input signals as interrupt for H series
		if (bios_d->b_info->board_type & CDIOBI_BT_PCI_H_SERIES) {	// Board type: H series
			w_data = 0xffff;
			d_data = 0xffffffff;
			if (bios_d->b_info->inp_port_num == 2) {
				bios_d->int_mask[0] = 0xff;		// Mask 16 bits
				bios_d->int_mask[1] = 0xff;
				_outpw(bios_d->int_mask_port, w_data);
			} else {
				bios_d->int_mask[0] = 0xff;		// Mask 32 bits
				bios_d->int_mask[1] = 0xff;
				bios_d->int_mask[2] = 0xff;
				bios_d->int_mask[3] = 0xff;
				_outpd(bios_d->int_mask_port, d_data);
			}
		}
		// 8 and 16 channels of the input signals as interrupt for CONPROSYS series
		if (bios_d->b_info->board_type & CDIOBI_BT_PCI_CONPROSYS) {	// Board type: CONPROSYS series
			b_data = 0xff;
			if (bios_d->b_info->inp_port_num == 1) {
				bios_d->int_mask[0] = 0xff;		// Mask 8 bits
				_outp(bios_d->int_mask_port, b_data);
			}else if(bios_d->b_info->inp_port_num == 2) {
				bios_d->int_mask[0] = 0xff;		// Mask 16 bits
				bios_d->int_mask[1] = 0xff;
				_outp(bios_d->int_mask_port, b_data);
				_outp(bios_d->int_mask_port + 1, b_data);
			}
		}
		// All channels of the input signals as interrupt for 8255
		if (bios_d->b_info->board_type & CDIOBI_BT_PCI_8255) {	// Board type: 8255
			b_data = 0xff;						// Mask 8 bits
			for(port_no = 0; port_no < bios_d->b_info->inp_port_num; port_no++){
				// Mask data
				bios_d->int_mask[port_no]	= b_data;
				// Mask
				real_port_no = port_no;
				Cdiobi_8255_port_set(bios_d, &real_port_no);
				_outp(bios_d->int_mask_port + real_port_no, b_data);
			}
		}
		// Mask all the interrupts of the local side for the DM board
		if (bios_d->b_info->board_type & CDIOBI_BT_PCI_DM) {	// Board type: DM
			d_data = 0xffffffff;
			bios_d->int_mask[0] = 0x0f;			// Mask 4 bits
			if (strcmp(bios_d->b_info->board_name, "DIO-32DM3-PE") == 0){
				_outmd(bios_d->int_mask_port_mem, d_data);
			} else {
				_outpd(bios_d->int_mask_port, d_data);
			}
		}
	}
	//----------------------------------------
	// In the case of specified mask
	//----------------------------------------
	if (flag == CDIOBI_MASK_DATA) {
		// 4 channels of the input signals as interrupt for ASIC
		if (bios_d->b_info->board_type & CDIOBI_BT_PCI_ASIC) {	// Board type: ASIC
			b_data = mask[0];
			bios_d->int_mask[0] = b_data;
			_outp(bios_d->int_mask_port, b_data);
		}
		// 16 channels of the input signals as interrupt for Multipoint
		if (bios_d->b_info->board_type & CDIOBI_BT_PCI_TATEN) {	// Board type: Multipoint
			w_data = (mask[1] << 8) | (mask[0]);
			bios_d->int_mask[0] = mask[0];
			bios_d->int_mask[1] = mask[1];
			_outpw(bios_d->int_mask_port, w_data);
		}
		// 32 and 16 channels of the input signals as interrupt for H series
		if (bios_d->b_info->board_type & CDIOBI_BT_PCI_H_SERIES) {	// Board type: H series
			w_data = (mask[1] << 8) | (mask[0]);
			d_data = (mask[3] << 24) | (mask[2] << 16) | (mask[1] << 8) | (mask[0]);
			if (bios_d->b_info->inp_port_num == 2) {
				bios_d->int_mask[0] = mask[0];		// Mask 16 bits
				bios_d->int_mask[1] = mask[1];
				_outpw(bios_d->int_mask_port, w_data);
			} else {
				bios_d->int_mask[0] = mask[0];		// Mask 32 bits
				bios_d->int_mask[1] = mask[1];
				bios_d->int_mask[2] = mask[2];
				bios_d->int_mask[3] = mask[3];
				_outpd(bios_d->int_mask_port, d_data);
			}
		}
		// 8 and 16 channels of the input signals as interrupt for CONPROSYS series
		if (bios_d->b_info->board_type & CDIOBI_BT_PCI_CONPROSYS) {	// Board type: CONPROSYS series
			b_data = mask[0];
			if (bios_d->b_info->inp_port_num == 1) {
				bios_d->int_mask[0] = mask[0];		// Mask 8 bits
				_outp(bios_d->int_mask_port, b_data);
			}else if(bios_d->b_info->inp_port_num == 2) {
				bios_d->int_mask[0] = mask[0];		// Mask 16 bits
				bios_d->int_mask[1] = mask[1];
				_outp(bios_d->int_mask_port, b_data);
				b_data = mask[1];
				_outp(bios_d->int_mask_port + 1, b_data);
			}
		}
		// All channels of the input signals as interrupt for 8255
		if (bios_d->b_info->board_type & CDIOBI_BT_PCI_8255) {	// Board type: 8255
			for(port_no = 0; port_no < bios_d->b_info->inp_port_num; port_no++){
				// Mask data
				bios_d->int_mask[port_no]	= mask[port_no];
				// Mask
				real_port_no = port_no;
				Cdiobi_8255_port_set(bios_d, &real_port_no);
				_outp(bios_d->int_mask_port + real_port_no, mask[port_no]);
			}
		}
		// 4 channels of the input signals as interrupt for DM board.  Mask the interrupts expcept general-purpose input
		if (bios_d->b_info->board_type & CDIOBI_BT_PCI_DM) {	// Board type: DM
			d_data = 0xffff0fff | (mask[0] << 12);
			bios_d->int_mask[0] = mask[0];			// Mask 4 bits
			if (strcmp(bios_d->b_info->board_name, "DIO-32DM3-PE") == 0){
				_outmd(bios_d->int_mask_port_mem, d_data);
			} else {
				_outpd(bios_d->int_mask_port, d_data);
			}
		}
	}
	return;
}

//================================================================
// Retrieve the interrupt mask
//================================================================
void Cdiobi_get_all_int_mask(PCDIOBI bios_d, unsigned char *mask)
{
	short	port_no;

	//----------------------------------------
	// Check the parameters
	//----------------------------------------
	if (bios_d == NULL ||
		bios_d->flag == 0) {
		return;
	}
	if (strcmp(bios_d->b_info->board_name, "DIO-32DM3-PE") == 0){
		if (!(bios_d->b_info->board_type & CDIOBI_BT_DEMO) && (bios_d->int_mask_port_mem == 0)){
			return;
		}
	} else {
		if (!(bios_d->b_info->board_type & CDIOBI_BT_DEMO) && (bios_d->int_mask_port == 0)){
			return;
		}
	}
	if (mask == NULL) {
		return;
	}
	//----------------------------------------
	// Copy the mask
	//----------------------------------------
	memset(mask, 0, sizeof(bios_d->int_mask));
	// 4 channels of the input signals as interrupt for ASIC
	if (bios_d->b_info->board_type & CDIOBI_BT_PCI_ASIC) {		// Board type: ASIC
		mask[0] = bios_d->int_mask[0];
	}
	// 16 channels of the input signals as interrupt for Multipoint
	if (bios_d->b_info->board_type & CDIOBI_BT_PCI_TATEN) {		// Board type: Multipoint
		mask[0] = bios_d->int_mask[0];
		mask[1] = bios_d->int_mask[1];
	}
	// 32 and 16 channels of the input signals as interrupt for H series
	if (bios_d->b_info->board_type & CDIOBI_BT_PCI_H_SERIES) {	// Board type: H series
		if (bios_d->b_info->inp_port_num == 2) {
			mask[0] = bios_d->int_mask[0];
			mask[1] = bios_d->int_mask[1];
		} else {
			mask[0] = bios_d->int_mask[0];
			mask[1] = bios_d->int_mask[1];
			mask[2] = bios_d->int_mask[2];
			mask[3] = bios_d->int_mask[3];
		}
	}
	// 8 and 16 channels of the input signals as interrupt for CONPROSYS series
	if (bios_d->b_info->board_type & CDIOBI_BT_PCI_CONPROSYS) {	// Board type: CONPROSYS series
		if (bios_d->b_info->inp_port_num == 1) {
			mask[0] = bios_d->int_mask[0];
		}else if(bios_d->b_info->inp_port_num == 2) {
			mask[0] = bios_d->int_mask[0];
			mask[1] = bios_d->int_mask[1];
		}
	}
	// All channels of the input signals as interrupt for 8255
	if (bios_d->b_info->board_type & CDIOBI_BT_PCI_8255) {		// Board type: 8255
		for(port_no = 0; port_no < bios_d->b_info->inp_port_num; port_no++){
			mask[port_no]	= bios_d->int_mask[port_no];
		}
	}
	// 4 channels of the input signals as interrupt for DM board
	if (bios_d->b_info->board_type & CDIOBI_BT_PCI_DM) {		// Board type: DM
		mask[0] = bios_d->int_mask[0];
	}
	return;
}

//================================================================
// Mask one bit of interrupt
//================================================================
long Cdiobi_set_bit_int_mask(PCDIOBI bios_d, short bit_no, short open)
{
	unsigned char	b_data;
	unsigned short	w_data;
	unsigned long	d_data;
	short			port_no;

	//----------------------------------------
	// Check the parameters
	//----------------------------------------
	if (bios_d == NULL ||
		bios_d->flag == 0) {
		return 0;
	}
	if (strcmp(bios_d->b_info->board_name, "DIO-32DM3-PE") == 0){
		if (!(bios_d->b_info->board_type & CDIOBI_BT_DEMO) && (bios_d->int_mask_port_mem == 0)){
			return 0;
		}
	}else {
		if (!(bios_d->b_info->board_type & CDIOBI_BT_DEMO) && (bios_d->int_mask_port == 0)){
			return 0;
		}
	}
	if (open != CDIOBI_INT_MASK_CLOSE &&
		open != CDIOBI_INT_MASK_OPEN) {
		return -1;
	}
	//----------------------------------------
	// Check the interrupt bits
	//----------------------------------------
	if (Cdiobi_check_int_bit(bios_d, bit_no) == 0) {
		return -1;
	}
	//----------------------------------------
	// In the case of open mask
	//----------------------------------------
	if (open == CDIOBI_INT_MASK_OPEN) {
		// 4 channels of the input signals as interrupt for ASIC
		if (bios_d->b_info->board_type & CDIOBI_BT_PCI_ASIC) {		// Board type: ASIC
			b_data = bios_d->int_mask[0] & ~(0x01 << bit_no);
			bios_d->int_mask[0] = b_data;
			_outp(bios_d->int_mask_port, b_data);
		}
		// 16 channels of the input signals as interrupt for Multipoint
		if (bios_d->b_info->board_type & CDIOBI_BT_PCI_TATEN) {		// Board type: Multipoint
			w_data = (bios_d->int_mask[1] << 8) | (bios_d->int_mask[0]);
			w_data &= ~(0x01 << bit_no);
			bios_d->int_mask[0] = (unsigned char)(w_data & 0xff);
			bios_d->int_mask[1] = (unsigned char)((w_data >> 8) & 0xff);
			_outpw(bios_d->int_mask_port, w_data);
		}
		// 32 and 16 channels of the input signals as interrupt for H series
		if (bios_d->b_info->board_type & CDIOBI_BT_PCI_H_SERIES) {	// Board type: H series
			w_data = (bios_d->int_mask[1] << 8) | (bios_d->int_mask[0]);
			d_data = (bios_d->int_mask[3] << 24) | (bios_d->int_mask[2] << 16) |
					 (bios_d->int_mask[1] << 8) | (bios_d->int_mask[0]);
			if (bios_d->b_info->inp_port_num == 2) {
				w_data &= ~(0x01 << bit_no);
				bios_d->int_mask[0] = (unsigned char)(w_data & 0xff);
				bios_d->int_mask[1] = (unsigned char)((w_data >> 8) & 0xff);
				_outpw(bios_d->int_mask_port, w_data);
			} else {
				d_data &= ~(0x01 << bit_no);
				bios_d->int_mask[0] = (unsigned char)(d_data & 0xff);
				bios_d->int_mask[1] = (unsigned char)((d_data >> 8)  & 0xff);
				bios_d->int_mask[2] = (unsigned char)((d_data >> 16) & 0xff);
				bios_d->int_mask[3] = (unsigned char)((d_data >> 24) & 0xff);
				_outpd(bios_d->int_mask_port, d_data);
			}
		}
		// 8 and 16 channels of the input signals as interrupt for CONPROSYS series
		if (bios_d->b_info->board_type & CDIOBI_BT_PCI_CONPROSYS) {	// Board type: CONPROSYS series
			b_data = bios_d->int_mask[0];
			w_data = (bios_d->int_mask[1] << 8) | (bios_d->int_mask[0]);
			if (bios_d->b_info->inp_port_num == 1) {
				b_data &= ~(0x01 << bit_no);
				bios_d->int_mask[0] = (unsigned char)(b_data & 0xff);
				_outp(bios_d->int_mask_port, b_data);
			}else if(bios_d->b_info->inp_port_num == 2) {
				w_data &= ~(0x01 << bit_no);
				bios_d->int_mask[0] = (unsigned char)(w_data & 0xff);
				bios_d->int_mask[1] = (unsigned char)((w_data >> 8) & 0xff);
				_outpw(bios_d->int_mask_port, w_data);
			}
		}
		// All channels of the input signals as interrupt for 8255
		if (bios_d->b_info->board_type & CDIOBI_BT_PCI_8255) {		// Board type: 8255
			port_no = bit_no / 8;
			b_data = bios_d->int_mask[port_no] & ~(0x01 << (bit_no % 8));
			bios_d->int_mask[port_no] = b_data;
			Cdiobi_8255_port_set(bios_d, &port_no);
			_outp(bios_d->int_mask_port + port_no, b_data);
		}
		// 4 channels of the input signals as interrupt for DM board
		if (bios_d->b_info->board_type & CDIOBI_BT_PCI_DM) {		// Board type: DM
			b_data = bios_d->int_mask[0] & ~(0x01 << bit_no);
			bios_d->int_mask[0] = b_data;
			d_data = 0xffff0fff | (b_data << 12);
			if (strcmp(bios_d->b_info->board_name, "DIO-32DM3-PE") == 0){
				_outmd(bios_d->int_mask_port_mem, d_data);
			} else {
				_outpd(bios_d->int_mask_port, d_data);
			}
		}
	}
	//----------------------------------------
	// In the case of closing the mask
	//----------------------------------------
	if (open == CDIOBI_INT_MASK_CLOSE) {
		// 4 channels of the input signals as interrupt for ASIC
		if (bios_d->b_info->board_type & CDIOBI_BT_PCI_ASIC) {		// Board type: ASIC
			b_data = bios_d->int_mask[0] | (0x01 << bit_no);
			bios_d->int_mask[0] = b_data;
			_outp(bios_d->int_mask_port, b_data);
		}
		// 16 channels of the input signals as interrupt for Multipoint
		if (bios_d->b_info->board_type & CDIOBI_BT_PCI_TATEN) {		// Board type: Multipoint
			w_data = (bios_d->int_mask[1] << 8) | (bios_d->int_mask[0]);
			w_data |= (0x01 << bit_no);
			bios_d->int_mask[0] = (unsigned char)(w_data & 0xff);
			bios_d->int_mask[1] = (unsigned char)((w_data >> 8) & 0xff);
			_outpw(bios_d->int_mask_port, w_data);
		}
		// 32 and 16 channels of the input signals as interrupt for H series
		if (bios_d->b_info->board_type & CDIOBI_BT_PCI_H_SERIES) {	// Board type: H series
			w_data = (bios_d->int_mask[1] << 8) | (bios_d->int_mask[0]);
			d_data = (bios_d->int_mask[3] << 24) | (bios_d->int_mask[2] << 16) |
					 (bios_d->int_mask[1] << 8) | (bios_d->int_mask[0]);
			if (bios_d->b_info->inp_port_num == 2) {
				w_data |= (0x01 << bit_no);
				bios_d->int_mask[0] = (unsigned char)(w_data & 0xff);
				bios_d->int_mask[1] = (unsigned char)((w_data >> 8) & 0xff);
				_outpw(bios_d->int_mask_port, w_data);
			} else {
				d_data |= (0x01 << bit_no);
				bios_d->int_mask[0] = (unsigned char)(d_data & 0xff);
				bios_d->int_mask[1] = (unsigned char)((d_data >> 8)  & 0xff);
				bios_d->int_mask[2] = (unsigned char)((d_data >> 16) & 0xff);
				bios_d->int_mask[3] = (unsigned char)((d_data >> 24) & 0xff);
				_outpd(bios_d->int_mask_port, d_data);
			}
		}
		// 8 and 16 channels of the input signals as interrupt for CONPROSYS series
		if (bios_d->b_info->board_type & CDIOBI_BT_PCI_CONPROSYS) {	// Board type: CONPROSYS series
			b_data = bios_d->int_mask[0];
			w_data = (bios_d->int_mask[1] << 8) | (bios_d->int_mask[0]);
			if (bios_d->b_info->inp_port_num == 1) {
				b_data |= (0x01 << bit_no);
				bios_d->int_mask[0] = (unsigned char)(b_data & 0xff);
				_outp(bios_d->int_mask_port, b_data);
			}else if(bios_d->b_info->inp_port_num == 2) {
				w_data |= (0x01 << bit_no);
				bios_d->int_mask[0] = (unsigned char)(w_data & 0xff);
				bios_d->int_mask[1] = (unsigned char)((w_data >> 8) & 0xff);
				_outp(bios_d->int_mask_port, (unsigned char)(w_data & 0xff));
				_outp(bios_d->int_mask_port + 1, (unsigned char)((w_data >> 8) & 0xff));
			}
		}
		// All channels of the input signals as interrupt for 8255
		if (bios_d->b_info->board_type & CDIOBI_BT_PCI_8255) {		// Board type: 8255
			port_no	= bit_no / 8;
			b_data	= bios_d->int_mask[port_no] | (0x01 << (bit_no % 8));
			bios_d->int_mask[port_no] = b_data;
			Cdiobi_8255_port_set(bios_d, &port_no);
			_outp(bios_d->int_mask_port + port_no, b_data);
		}
		// 4 channels of the input signals as interrupt for DM board
		if (bios_d->b_info->board_type & CDIOBI_BT_PCI_DM) {		// Board type: DM
			b_data = bios_d->int_mask[0] | (0x01 << bit_no);
			bios_d->int_mask[0] = b_data;
			d_data = 0xffff0fff | (b_data << 12);
			if (strcmp(bios_d->b_info->board_name, "DIO-32DM3-PE") == 0){
				_outmd(bios_d->int_mask_port_mem, d_data);
			} else {
				_outpd(bios_d->int_mask_port, d_data);
			}
		}
	}
	return 0;
}

//================================================================
// Interrupt logic setting
//================================================================
long Cdiobi_set_int_logic(PCDIOBI bios_d, short bit_no, short logic)
{
	unsigned char	int_mask[CDIOBI_INT_SENCE_SIZE];
	short			port;
	short			bit;
	unsigned char	b_data;
	unsigned short	w_data;
	unsigned long	d_data;
	unsigned short	no_of_8255;
	
	//----------------------------------------
	// Check the parameters
	//----------------------------------------
	if (strcmp(bios_d->b_info->board_name, "DIO-32DM3-PE") == 0){
		// Return an error if no control port and no DIO_INT_RISE
		if ((bios_d->int_ctrl_port_mem == 0) && (logic != DIO_INT_RISE) &&
			!(bios_d->b_info->board_type & CDIOBI_BT_DEMO)){ // Not a demo device
			return -2;
		}
		if (bios_d == NULL ||
			bios_d->flag == 0) {
			return -1;
		}
		if ((bios_d->int_ctrl_port_mem == 0) &&
			!(bios_d->b_info->board_type & CDIOBI_BT_DEMO)) {
			return -1;
		}
		if (logic != DIO_INT_RISE &&
			logic != DIO_INT_FALL) {
			return 0;
		}
	} else {
		// Return an error if no control port and no DIO_INT_RISE
		if ((bios_d->int_ctrl_port == 0) && (logic != DIO_INT_RISE)){
			return -2;
		}
		if (bios_d == NULL ||
			bios_d->flag == 0 ||
			bios_d->int_ctrl_port == 0) {
			return -1;
		}
		if (logic != DIO_INT_RISE &&
			logic != DIO_INT_FALL) {
			return 0;
		}
	}
	//----------------------------------------
	// Check the interrupt bits
	//----------------------------------------
	if (Cdiobi_check_int_bit(bios_d, bit_no) == 0) {
		return -1;
	}
	//----------------------------------------
	// Make the interrupt logic
	//----------------------------------------
	port	= bit_no / 8;
	bit		= bit_no % 8;
	if (logic == DIO_INT_RISE) {
		bios_d->int_ctrl[port] &= ~(0x01 << bit);
	}
	if (logic == DIO_INT_FALL) {
		bios_d->int_ctrl[port] |= (0x01 << bit);
	}
	//----------------------------------------
	// Save the interrupt mask and full mask
	//----------------------------------------
	Cdiobi_get_all_int_mask(bios_d, int_mask);
	Cdiobi_set_all_int_mask(bios_d, CDIOBI_MASK_ALL, NULL);
	//----------------------------------------
	// Set the logic according to the board
	//----------------------------------------
	// In the case of ASIC
	if (bios_d->b_info->board_type & CDIOBI_BT_PCI_ASIC) {	// Board type: ASIC
		b_data = 0;
		if (logic == DIO_INT_RISE) {
			b_data	= (bit_no << 4) | 0x00;
		}
		if (logic == DIO_INT_FALL) {
			b_data	= (bit_no << 4) | 0x04;
		}
		_outp(bios_d->int_ctrl_port, b_data);
		Ccom_udelay(10);		// ASIC: Countermeasure against interrupt logic switching bug
	}
	// In the case of Mlutipoint
	if (bios_d->b_info->board_type & CDIOBI_BT_PCI_TATEN) {		// Board type: Multipoint
		w_data = (bios_d->int_ctrl[1] << 8) | bios_d->int_ctrl[0];
		_outpw(bios_d->int_ctrl_port, w_data);
	}
	// 32 and 16 channels of the input signals as interrupt for H series
	if (bios_d->b_info->board_type & CDIOBI_BT_PCI_H_SERIES) {	// Board type: H series
		w_data = (bios_d->int_ctrl[1] << 8) | bios_d->int_ctrl[0];
		d_data = (bios_d->int_ctrl[3] << 24) | (bios_d->int_ctrl[2] << 16) | 
				 (bios_d->int_ctrl[1] << 8) | bios_d->int_ctrl[0];
		if (bios_d->b_info->inp_port_num == 2) {
			_outpw(bios_d->int_ctrl_port, w_data);
		} else {
			_outpd(bios_d->int_ctrl_port, d_data);
		}
	}
	// 8 and 16 channels of the input signals as interrupt for CONPROSYS series
	if (bios_d->b_info->board_type & CDIOBI_BT_PCI_CONPROSYS) {	// Board type: CONPROSYS series
		b_data = bios_d->int_ctrl[0];
		w_data = (bios_d->int_ctrl[1] << 8) | bios_d->int_ctrl[0];
		if (bios_d->b_info->inp_port_num == 1) {
			_outp(bios_d->int_ctrl_port, b_data);
		}else if (bios_d->b_info->inp_port_num == 2) {
			_outpw(bios_d->int_ctrl_port, w_data);
		}
	}
	// For the devices that doesn't support the 8255 interrupt control, flick with the parameter check of this function
	if (bios_d->b_info->board_type & CDIOBI_BT_PCI_8255) {	// Board type: 8255
		for(no_of_8255 = 0; no_of_8255 < bios_d->b_info->num_of_8255; no_of_8255++){
			_outp(bios_d->int_ctrl_port + (no_of_8255 * 4) + 0, bios_d->int_ctrl[(no_of_8255 * 3) + 0]);
			_outp(bios_d->int_ctrl_port + (no_of_8255 * 4) + 1, bios_d->int_ctrl[(no_of_8255 * 3) + 1]);
			_outp(bios_d->int_ctrl_port + (no_of_8255 * 4) + 2, bios_d->int_ctrl[(no_of_8255 * 3) + 2]);
		}
	}
	//----------------------------------------
	// Restore the mask
	//----------------------------------------
	Cdiobi_set_all_int_mask(bios_d, CDIOBI_MASK_DATA, int_mask);
	return 0;
}

//================================================================
// Retrieve the interrupt status
//================================================================
long Cdiobi_sence_int_status(PCDIOBI bios_d, unsigned char *sence)
{
	long			lret = 0;		// No own interrupt
	unsigned char	b_data;
	unsigned short	w_data;
	unsigned long	d_data;
	short			port_no, real_port_no;
	
	//----------------------------------------
	// Check the parameters
	//----------------------------------------
	if (bios_d == NULL ||
		bios_d->flag == 0 ||
		sence == NULL) {
		return 0;
	}
	//----------------------------------------
	// In the case of no sence port
	// Keep that it is not own interrupt tentatively.
	// This way of thinking will change if the ISA is supported.
	//----------------------------------------
	if (strcmp(bios_d->b_info->board_name, "DIO-32DM3-PE") == 0){
		if (bios_d->int_sence_port_mem == 0) {
			return 0;
		}
	} else {
		if (bios_d->int_sence_port == 0) {
			return 0;
		}
	}
	//----------------------------------------
	// Read and reset the interrupt sense port
	//----------------------------------------
	memset(sence, 0, CDIOBI_INT_SENCE_SIZE);
	// In the case of ASIC
	if (bios_d->b_info->board_type & CDIOBI_BT_PCI_ASIC) {	// Board type: ASIC
		// Sense and reset
		b_data = _inp(bios_d->int_sence_port);
		_outp(bios_d->int_sence_port, b_data);
		// OFF the masked bits
		b_data &= ~(bios_d->int_mask[0]);
		sence[0] = b_data;
		// Own interrupt?
		if (b_data) {
			lret = 1;
		}
	}
	// In the case of Mlutipoint
	if (bios_d->b_info->board_type & CDIOBI_BT_PCI_TATEN) {	// Board type: Multipoint
		// Sense and reset
		w_data = _inpw(bios_d->int_sence_port);
		// OFF the masked bits
		w_data &= ~(((bios_d->int_mask[1]<<8) & 0xff00) | bios_d->int_mask[0]);
		sence[0] = (w_data & 0xff);
		sence[1] = ((w_data >> 8) & 0xff);
		// Own interrupt?
		if (w_data) {
			lret = 1;
		}
	}
	// 32 and 16 channels of the input signals as interrupt for H series
	if (bios_d->b_info->board_type & CDIOBI_BT_PCI_H_SERIES) {	// Board type: H series
		if (bios_d->b_info->inp_port_num == 2) {
			// Sence and reset
			w_data = _inpw(bios_d->int_sence_port);
			sence[0] = (w_data & 0xff);
			sence[1] = ((w_data >> 8) & 0xff);
			if (w_data) {
				lret = 1;
			}
		} else {
			// Sence and reset
			d_data = _inpd(bios_d->int_sence_port);
			sence[0] = (d_data & 0xff);
			sence[1] = ((d_data >> 8) & 0xff);
			sence[2] = ((d_data >> 16) & 0xff);
			sence[3] = ((d_data >> 24) & 0xff);
			if (d_data) {
				lret = 1;
			}
		}
	}
	// 8 and 16 channels of the input signals as interrupt for CONPROSYS series
	if (bios_d->b_info->board_type & CDIOBI_BT_PCI_CONPROSYS) {	// Board type: CONPROSYS series
		if (bios_d->b_info->inp_port_num == 1) {
			// Sence and reset
			b_data = _inp(bios_d->int_sence_port);
			sence[0] = (b_data & 0xff);
			if (b_data) {
				lret = 1;
			}
		}else if (bios_d->b_info->inp_port_num == 2) {
			// Sence and reset
			w_data = _inpw(bios_d->int_sence_port);
			sence[0] = (unsigned char)(w_data & 0xff);
			sence[1] = (unsigned char)((w_data >> 8) & 0xff);
			if (w_data) {
				lret = 1;
			}
		}
	}
	// All channels of the input signals as interrupt for 8255
	if (bios_d->b_info->board_type & CDIOBI_BT_PCI_8255) {	// Board type: 8255
		for(port_no = 0; port_no < bios_d->b_info->inp_port_num; port_no++){
			// Sence and reset
			real_port_no = port_no;
			Cdiobi_8255_port_set(bios_d, &real_port_no);
			sence[port_no] = _inp(bios_d->int_sence_port + real_port_no);
			// OFF the masked bits
			sence[port_no] = sence[port_no] & ~(bios_d->int_mask[port_no]);
			// Own interrupt?
			if (sence[port_no]){
				lret = 1;
			}
		}
	}
	// 4 channels of the input signals as interrupt for DM board
	if (bios_d->b_info->board_type & CDIOBI_BT_PCI_DM) {	// Board type: DM
		if (strcmp(bios_d->b_info->board_name, "DIO-32DM3-PE") == 0){
			// Sense and reset
			d_data = _inmd(bios_d->int_sence_port_mem);
			_outmd(bios_d->int_sence_port_mem, d_data);
			sence[0] = (unsigned char)((d_data >> 12) & 0x0000000f);
			// OFF the masked bits
			sence[0] = sence[0] & ~(bios_d->int_mask[0]);
			// Own interrupt?
			if (sence[0]){
				lret = 1;
			}
		} else {
			// Sense and reset	
			d_data = _inpd(bios_d->int_sence_port);
			_outpd(bios_d->int_sence_port, d_data);
			sence[0] = (unsigned char)((d_data >> 12) & 0x0000000f);
			// OFF the masked bits
			sence[0] = sence[0] & ~(bios_d->int_mask[0]);
			// Own interrupt?
			if (sence[0]){
				lret = 1;
			}
		}
	}
	return lret;
}

//================================================================
// Retrieve the interrupt logic
//================================================================
long Cdiobi_get_int_logic(PCDIOBI bios_d, unsigned short *logic)
{
	short		bit_no;
	short		port;
	short		bit;

	//----------------------------------------
	// Check the parameters
	//----------------------------------------
	if (bios_d == NULL ||
		bios_d->flag == 0 ||
		logic == NULL) {
		return 0;
	}
	//----------------------------------------
	// In the case of no control port,
	// set all valid bits to rise
	//----------------------------------------
	memset(logic, 0, sizeof(short) * CDIOBI_INT_SENCE_SIZE * 8);
	if (strcmp(bios_d->b_info->board_name, "DIO-32DM3-PE") == 0){
		if ((bios_d->int_ctrl_port_mem == 0) &&
			!(bios_d->b_info->board_type & CDIOBI_BT_DEMO)){
			for (bit_no=0; bit_no<bios_d->b_info->max_int_bit; bit_no++) {
				port = bit_no / 8;
				bit  = bit_no % 8;
				if (bios_d->b_info->int_bit[port] & (0x01<<bit)) {
					logic[bit_no] = DIO_INT_RISE;
				}
			}
			return bios_d->b_info->max_int_bit;
		}
	} else {
		if (bios_d->int_ctrl_port == 0) {
			for (bit_no=0; bit_no<bios_d->b_info->max_int_bit; bit_no++) {
				port	= bit_no / 8;
				bit		= bit_no % 8;
				if (bios_d->b_info->int_bit[port] & (0x01<<bit)) {
					logic[bit_no] = DIO_INT_RISE;
				}
			}
			return bios_d->b_info->max_int_bit;
		}
	}
	//----------------------------------------
	// In the case that there are control ports
	// Control 0 -> return the rising,
	// Control 1 -> return the falling 
	//----------------------------------------
	for (bit_no=0; bit_no<bios_d->b_info->max_int_bit; bit_no++) {
		port	= bit_no / 8;
		bit		= bit_no % 8;
		if (bios_d->int_ctrl[port] & (0x01<<bit)) {
			logic[bit_no] = DIO_INT_FALL;
		} else {
			logic[bit_no] = DIO_INT_RISE;
		}
	}
	return bios_d->b_info->max_int_bit;
}

//================================================================
// Function for retrieving device information
//================================================================
long Cdiobi_get_info(char *device, short info_type, void *param1, void *param2, void *param3)
{
	PCDIOBI_BOARD_INFO	b_info;
	short				i;
	int					copy_ret;
	char				d_name[CDIOBI_BOARD_NAME_MAX];
	void				*c_param1;

	//----------------------------------------
	// Check the parameters
	//----------------------------------------
	if (param1 == NULL ||
		device == NULL) {
		return -2;		// Buffer address error
	}

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5,0,0))
	if (access_ok(device, sizeof(unsigned short)))
#else
	if (access_ok(VERIFY_READ, device, sizeof(unsigned short)))
#endif
	{
		copy_ret = copy_from_user(d_name, device, sizeof(d_name));
	} else {
		strcpy(d_name, device);
	}

	c_param1 = kmalloc(sizeof(void *), GFP_KERNEL);
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5,0,0))
	if (access_ok(param1, sizeof(unsigned short)))
#else
	if (access_ok(VERIFY_READ, param1, sizeof(unsigned short)))
#endif
	{
		copy_ret = copy_from_user(c_param1, param1, sizeof(void *));
	} else {
		memcpy(c_param1, param1, sizeof(void *));
	}
	
	//----------------------------------------
	// Search the board information
	//----------------------------------------
	b_info = NULL;
	for (i=0; strcmp(board_info[i].board_name, "End") != 0; i++) {
		if (strcmp(board_info[i].board_name, d_name) == 0) {
			b_info = &board_info[i];
			break;
		}
	}
	if (b_info == NULL) {
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5,0,0))
		if (access_ok(param1, sizeof(unsigned short)))
#else
		if (access_ok(VERIFY_WRITE, param1, sizeof(unsigned short)))
#endif
		{
			copy_ret = copy_to_user(param1, c_param1, sizeof(void *));
		} else {
			memcpy(param1, c_param1, sizeof(void *));
		}
		kfree(c_param1);
		return -1;
	}
	//----------------------------------------
	// Check the parameters
	//----------------------------------------
	switch (info_type) {
	case IDIO_DEVICE_TYPE:				// Device type (short)
		*(short *)c_param1 = (short)b_info->device_type;
		break;
	case IDIO_NUMBER_OF_8255:			// Number of 8255 (int)
		*(int *)c_param1 = (int)b_info->num_of_8255;
		break;
	case IDIO_IS_8255_BOARD:			// 8255 Type (int)
		if (b_info->num_of_8255 == 0) {
			*(int *)c_param1 = 0;
		} else {
			*(int *)c_param1 = 1;
		}
		break;
	case IDIO_NUMBER_OF_DI_BIT:			// DI BIT(short)
		*(short *)c_param1 = (short)b_info->inp_port_num * 8;
		break;
	case IDIO_NUMBER_OF_DO_BIT:			// DO BIT(short)
		*(short *)c_param1 = (short)b_info->out_port_num * 8;
		break;
	case IDIO_NUMBER_OF_DI_PORT:		// DI PORT(short)
		*(short *)c_param1 = (short)b_info->inp_port_num;
		break;
	case IDIO_NUMBER_OF_DO_PORT:		// DO PORT(short)
		*(short *)c_param1 = (short)b_info->out_port_num;
		break;
	case IDIO_IS_POSITIVE_LOGIC:		// Positive logic? (int)
		*(int *)c_param1 = (int)b_info->is_positive;
		break;
	case IDIO_IS_ECHO_BACK:				// Echo back available? (int)
		*(int *)c_param1 = (int)b_info->can_echo_back;
		break;
	case IDIO_NUM_OF_PORT:				// Number of occupied ports (short)
		*(short *)c_param1 = (short)b_info->port_num;
		break;
	case IDIO_BOARD_TYPE:				// Board type(unsigned long)
		*(unsigned long *)c_param1 = (unsigned long)b_info->board_type;
		break;
	case IDIO_IS_FILTER:				// Digital filter available(int)
		if (b_info->filter_off == 0) {
			*(int *)c_param1 = 0;
		} else {
			*(int *)c_param1 = 1;
		}
		break;
	case IDIO_NUMBER_OF_INT_BIT:		// Interruptable number of bits(short)
		*(short *)c_param1 = (short)b_info->max_int_bit;
		break;
	case IDIO_IS_DIRECTION:				// DioSetIoDirection function available(int)
		*(int *)c_param1 = 0;
		break;
	default:
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5,0,0))
		if (access_ok(param1, sizeof(unsigned short)))
#else
		if (access_ok(VERIFY_WRITE, param1, sizeof(unsigned short)))
#endif
		{
			copy_ret = copy_to_user(param1, c_param1, sizeof(void *));
		} else {
			memcpy(param1, c_param1, sizeof(void *));
		}
		kfree(c_param1);
		return -1;		// Information type error
	}

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5,0,0))
	if (access_ok(param1, sizeof(unsigned short)))
#else
	if (access_ok(VERIFY_WRITE, param1, sizeof(unsigned short)))
#endif
	{
		copy_ret = copy_to_user(param1, c_param1, sizeof(void *));
	} else {
		memcpy(param1, c_param1, sizeof(void *));
	}
	kfree(c_param1);
	return 0;
}

//================================================================
// Information acquisition function for initialized devices
//================================================================
long Cdiobi_get_cur_info(PCDIOBI bios_d, short info_type, unsigned long *param1, unsigned long *param2, unsigned long *param3)
{
	PCDIOBI_BOARD_INFO	b_info;
	//----------------------------------------
	// Parameter check
	//----------------------------------------
	if (param1 == NULL) {
		return -2;		// Buffer address error
	}
	//----------------------------------------
	// Search board infomation
	//----------------------------------------
	b_info = bios_d->b_info;
	if (b_info == NULL) {
		return -1;
	}
	//----------------------------------------
	// Parameter check
	//----------------------------------------
	switch (info_type) {
	case IDIO_CUR_ROM_REVISION:				// ROM revision (unsigned char)
		switch (b_info->resource_type) {
		case SRC_TYPE_PORTIO:
		case SRC_TYPE_MEMIO:
			*param1 = (unsigned long)bios_d->bi_rom_revision;
			break;
		default:
			return -1;						// Information type error
			break;
		}
		break;
	case IDIO_CUR_BOARD_REVISION:			// Board revision (unsigned char)
		switch (b_info->resource_type) {
		case SRC_TYPE_PORTIO:
		case SRC_TYPE_MEMIO:
			*param1 = (unsigned long)bios_d->bi_board_version;
			break;
		default:
			return -1;						// Information type error
			break;
		}
		break;
	default:
		return -1;							// Information type error
	}
	return 0;
}

//================================================================
// Interrupt available bits check
//================================================================
long Cdiobi_check_int_bit(PCDIOBI bios_d, short bit_no)
{
	short	port;
	short	bit;

	//----------------------------------------
	// Check the parameters
	//----------------------------------------
	if (bios_d == NULL ||
		bios_d->flag == 0) {
		return 0;		// Buffer address error
	}
	port	= bit_no / 8;
	bit		= bit_no % 8;
	if (bios_d->b_info->int_bit[port] & (0x01<<bit)) {
		return 1;
	}
	return 0;
}

//================================================================
// Input ports check
//================================================================
long Cdiobi_check_inp_port(PCDIOBI bios_d, short port_no)
{
	//----------------------------------------
	// Check the parameters
	//----------------------------------------
	if (bios_d == NULL ||
		bios_d->flag == 0 ||
		port_no < 0) {
		return 0;
	}
	//----------------------------------------
	// Check
	//----------------------------------------
	if (bios_d->b_info->inp_port_num == 0) {
		return 0;
	}
	if (port_no < 0) {
		return 0;
	}
	if (port_no < bios_d->b_info->inp_port_num) {
		return 1;
	}
	return 0;
}

//================================================================
// Input bits check
//================================================================
long Cdiobi_check_inp_bit(PCDIOBI bios_d, short bit_no)
{
	short	comp_bit;
	
	//----------------------------------------
	// Check the parameters
	//----------------------------------------
	if (bios_d == NULL ||
		bios_d->flag == 0 ||
		bit_no < 0) {
		return 0;
	}
	//----------------------------------------
	// Check
	//----------------------------------------
	if (bios_d->b_info->inp_port_num == 0) {
		return 0;
	}
	comp_bit = bios_d->b_info->inp_port_num * 8;
	if (bit_no < 0) {
		return 0;
	}
	if (bit_no < comp_bit) {
		return 1;
	}
	return 0;
}
//================================================================
// Output ports check
//================================================================
long Cdiobi_check_out_port(PCDIOBI bios_d, short port_no)
{
	//----------------------------------------
	// Check the parameters
	//----------------------------------------
	if (bios_d == NULL ||
		bios_d->flag == 0 ||
		port_no < 0) {
		return 0;
	}
	//----------------------------------------
	// Check
	//----------------------------------------
	if (bios_d->b_info->out_port_num == 0) {
		return 0;
	}
	if (port_no < 0) {
		return 0;
	}
	if (port_no < bios_d->b_info->out_port_num) {
		return 1;
	}
	return 0;
}

//================================================================
// Output bits check
//================================================================
long Cdiobi_check_out_bit(PCDIOBI bios_d, short bit_no)
{
	short	comp_bit;
	
	//----------------------------------------
	// Check the parameters
	//----------------------------------------
	if (bios_d == NULL ||
		bios_d->flag == 0 ||
		bit_no < 0) {
		return 0;
	}
	//----------------------------------------
	// Check
	//----------------------------------------
	if (bios_d->b_info->out_port_num == 0) {
		return 0;
	}
	comp_bit = bios_d->b_info->out_port_num * 8;
	if (bit_no < 0) {
		return 0;
	}
	if (bit_no < comp_bit) {
		return 1;
	}
	return 0;
}

//================================================================
// Check whether the board exists
//================================================================
long Cdiobi_check_board_exist(PCDIOBI bios_d)
{
	unsigned long	filter_value;
	unsigned char	test_value;
	unsigned char	read_value;
	unsigned short	get_value;
	
	//----------------------------------------
	// Check the parameters
	//----------------------------------------
	if (bios_d == NULL ||
		bios_d->flag == 0) {
		return -1;		// Does not exist
	}
	//----------------------------------------
	// Check (use the filter board)
	//----------------------------------------
	// Backup
	filter_value = bios_d->filter_value;
	if (strcmp(bios_d->b_info->board_name, "DIO-32DM3-PE") == 0){
		// Can not check for the case of no filter
		if (bios_d->filter_port_mem == 0) {
			return 0;
		}
		// Test 3
		test_value = 3;
		_outmw(bios_d->filter_port_mem, test_value);
		get_value = _inmw(bios_d->filter_port_mem);
		read_value = get_value & 0xff;
		if (read_value != test_value) {
			return -1;
		}
		// Test 5
		test_value = 5;
		//_outm(bios_d->filter_port_mem, test_value);
		//read_value = _inm(bios_d->filter_port_mem);
		_outmw(bios_d->filter_port_mem, test_value);
		get_value = _inmw(bios_d->filter_port_mem);
		read_value = get_value & 0xff;
		if (read_value != test_value) {
			return -1;
		}
	}else{
		// Can not check for the case of no filter
		if (bios_d->filter_port == 0) {
			return 0;
		}
		// Test 3
		test_value = 3;
		_outp(bios_d->filter_port, test_value);
		read_value = _inp(bios_d->filter_port);
		if (read_value != test_value) {
			return -1;
		}
		// Test 5
		test_value = 5;
		_outp(bios_d->filter_port, test_value);
		read_value = _inp(bios_d->filter_port);
		if (read_value != test_value) {
			return -1;
		}
	}
	// Restore
	Cdiobi_set_digital_filter(bios_d, filter_value);
	return 0;		// Exist
}

//================================================================
// The internal power supply type of CONRPOSYS needs to be set with software
// This function is set only for CONPROSYS devices
// For devices other than CONRPOSYS series, it is an unnecessary processing
//================================================================
void Cdiobi_power_enable(PCDIOBI bios_d)
{
	if (strcmp(bios_d->b_info->board_name, "CPS-DIO-0808BL") == 0){
		_outp(bios_d->filter_port-1, 0x01);
	}
}

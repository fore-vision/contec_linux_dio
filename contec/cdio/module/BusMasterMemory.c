////////////////////////////////////////////////////////////////////////////////
/// @file   BusMasterMemory.c
/// @brief  API-DIO(LNX) PCI Module - BusMaster library source file
/// @author &copy;CONTEC CO.,LTD.
/// @since  2003
////////////////////////////////////////////////////////////////////////////////

#ifndef __BUSMASTERMEMORY_C__
#define __BUSMASTERMEMORY_C__

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
#include <linux/sched.h>
#include <linux/delay.h>

#include "Ccom_module.h"
#include "BusMasterMemory.h"
#include "../library/Cdio_ctrl.h"
#include "../library/Cdio_dm_ctrl.h"

//--------------------------------------------------
// Constant
//--------------------------------------------------
// Constant
#define	BM_MAX_TRANS_SIZE				0x80000000							///< 2GByte
#define	BM_MEM_MAX_SG_SIZE				0x100000							///< S/G Max. size
#define	BM_MEM_INP_ReadAdd				0x07000000							///< S/G first read address for 32DM3		(Input)
#define	BM_MEM_OUT_WriteAdd				0x09000000							///< S/G first write address for 32DM3		(Output)
#define	BM_MEM_INP_TOP_DESC				0x06000000							///< S/G first write address for 32DM3		(Input)
#define	BM_MEM_OUT_TOP_DESC				0x08000000							///< S/G first read address for 32DM3		(Output)
#define	BM_MEM_INP_NXT_DESC				0x06000040							///< S/G first write address for 32DM3		(Input)
#define	BM_MEM_OUT_NXT_DESC				0x08000040							///< S/G first read address for 32DM3		(Output)
#define	BM_MEM_EMPTY					0x00000002							///< Empty flag for 32DM3
#define	BM_MEM_BUSY						0x00000003							///< Busy flag for 32DM3
#define	BM_MEM_STOP						0x00000020							///< Stop flag for 32DM3
#define	BM_MEM_RESET_FLAG				0x00000040							///< Reset flag for 32DM3
#define	BM_MEM_RESET_Dispatcher			0x00000002							///< Reset Dispatcher for 32DM3
#define	BM_MEM_STOP_Dispatcher			0x00000001							///< Stop Descriptor for 32DM3
#define	BM_MEM_COUNTER_BIT				32									///< 32bit counter
#define	BM_MEM_COUNTER_MASK				0x00FFFFFFFF						///< 32bit counter mask
#define	BM_MEM_CRA_SELECT				0x1000								///< Transfer table switching flag
#define	BM_MEM_AT_OFFSET				256									///< Transfer table offset					(Output)

// Output port for 32DM3
#define	BM_MEM_OUT_Read_SGAddr			(pMasAddr->dwPort_mem+0x8000000)	///< Setting S/G address to read from		(Output)
#define	BM_MEM_OUT_Write_SGAddr			(pMasAddr->dwPort_mem+0x8000004)	///< Setting S/G address to write to		(Output)
#define	BM_MEM_OUT_Length				(pMasAddr->dwPort_mem+0x8000008)	///< Setting offset from S/G address		(Output)
#define	BM_MEM_OUT_NextDescPtr			(pMasAddr->dwPort_mem+0x800000C)	///< Setting the Next descriptor address	(Output)
#define	BM_MEM_OUT_ActualTransfer		(pMasAddr->dwPort_mem+0x8000010)	///< mSGDMA data transfer count				(Output)
#define	BM_MEM_OUT_PrefetcherStatus		(pMasAddr->dwPort_mem+0x8000014)	///< Setting Prefetcher status				(Output)
#define	BM_MEM_OUT_SequenceNum			(pMasAddr->dwPort_mem+0x800001C)	///< Setting sequence No.					(Output)
#define	BM_MEM_OUT_Burst				(pMasAddr->dwPort_mem+0x800001E)	///< Setting Burst count					(Output)
#define	BM_MEM_OUT_Stride				(pMasAddr->dwPort_mem+0x8000020)	///< Setting Stride value					(Output)
#define	BM_MEM_OUT_Read_SGAddr_high		(pMasAddr->dwPort_mem+0x8000024)	///< Setting S/G address to read from (High)(Output)
#define	BM_MEM_OUT_Write_SGAddr_high	(pMasAddr->dwPort_mem+0x8000028)	///< Setting S/G address to write to (High)	(Output)
#define	BM_MEM_OUT_NextDescPtr_high		(pMasAddr->dwPort_mem+0x800002C)	///< Setting the Next descriptor address (High)	(Output)
#define	BM_MEM_OUT_Descriptor			(pMasAddr->dwPort_mem+0x800003C)	///< Descriptor control register			(Output)
#define	BM_MEM_OUT_CSR					(pMasAddr->dwPort_mem+0x8020000)	///< Dispatcher CSR first address			(Output)
#define	BM_MEM_OUT_Prefetchar_CSR		(pMasAddr->dwPort_mem+0x8010000)	///< Prefetcher CSR first address			(Output)
#define	BM_MEM_OUT_TotalDataLen			(pMasAddr->dwPort2_mem+0xB000030)	///< Total number of transferred data		(Output)
#define	BM_MEM_OUT_IntDataLen			(pMasAddr->dwPort2_mem+0xB00003C)	///< Setting the number of data to occur interrupt				(Output)
#define	BM_MEM_OUT_FifoConter			(pMasAddr->dwPort2_mem+0xB000034)	///< FIFO counter							(Output)
#define	BM_MEM_OUT_FifoReach			(pMasAddr->dwPort2_mem+0xB000038)	///< Setting the number of FIFO data to occur interrupt			(Output)
#define	BM_MEM_OUT_IntMask				(pMasAddr->dwPort2_mem+0xB00004A)	///< Interrupt mask							(Output)
#define	BM_MEM_OUT_IntClear				(pMasAddr->dwPort2_mem+0xB00004E)	///< Clear interrupt status					(Output)

// Input port for 32DM3
#define	BM_MEM_INP_Read_SGAddr			(pMasAddr->dwPort_mem+0x6000000)	///< Setting S/G address to read from		(Input)
#define	BM_MEM_INP_Write_SGAddr			(pMasAddr->dwPort_mem+0x6000004)	///< Setting S/G address to write to		(Input)
#define	BM_MEM_INP_Length				(pMasAddr->dwPort_mem+0x6000008)	///< Setting offset from S/G address		(Input)
#define	BM_MEM_INP_NextDescPtr			(pMasAddr->dwPort_mem+0x600000C)	///< Setting the Next descriptor address	(Input)
#define	BM_MEM_INP_ActualTransfer		(pMasAddr->dwPort_mem+0x6000010)	///< mSGDMA data transfer count				(Input)
#define	BM_MEM_INP_PrefetcherStatus		(pMasAddr->dwPort_mem+0x6000014)	///< Setting Prefetcher status				(Input)
#define	BM_MEM_INP_SequenceNum			(pMasAddr->dwPort_mem+0x600001C)	///< Setting sequence No.					(Input)
#define	BM_MEM_INP_Burst				(pMasAddr->dwPort_mem+0x600001E)	///< Setting Burst count					(Input)
#define	BM_MEM_INP_Stride				(pMasAddr->dwPort_mem+0x6000020)	///< Setting Stride value					(Input)
#define	BM_MEM_INP_Read_SGAddr_high		(pMasAddr->dwPort_mem+0x6000024)	///< Setting S/G address to read from (High)(Input)
#define	BM_MEM_INP_Write_SGAddr_high	(pMasAddr->dwPort_mem+0x6000028)	///< Setting S/G address to write to (High)	(Input)
#define	BM_MEM_INP_NextDescPtr_high		(pMasAddr->dwPort_mem+0x600002C)	///< Setting the Next descriptor address (High)	(Input)
#define	BM_MEM_INP_Descriptor			(pMasAddr->dwPort_mem+0x600003C)	///< Descriptor control register			(Input)
#define	BM_MEM_INP_Prefetchar_CSR		(pMasAddr->dwPort_mem+0x6010000)	///< Prefetcher CSR first address			(Input)
#define	BM_MEM_INP_CSR					(pMasAddr->dwPort_mem+0x6020000)	///< Dispatcher CSR first address			(Input)
#define	BM_MEM_INP_TotalDataLen			(pMasAddr->dwPort2_mem+0xB000020)	///< Total number of transferred data		(Input)
#define	BM_MEM_INP_FifoConter			(pMasAddr->dwPort2_mem+0xB000024)	///< FIFO counter							(Input)
#define	BM_MEM_INP_FifoReach			(pMasAddr->dwPort2_mem+0xB000028)	///< Setting the number of FIFO data to occur interrupt				(Input)
#define	BM_MEM_INP_IntDataLen			(pMasAddr->dwPort2_mem+0xB00002C)	///< Setting the number of data to occur interrupt				(Input)
#define	BM_MEM_INP_IntMask				(pMasAddr->dwPort2_mem+0xB000048)	///< Interrupt mask							(Input)
#define	BM_MEM_INP_IntClear				(pMasAddr->dwPort2_mem+0xB00004C)	///< Clear interrupt status					(Input)

// Input/Output common port for 32DM3
#define	BM_MEM_BM_DIR					(pMasAddr->dwPort2_mem+0xB000004)	///< Setting transfer direction				(Input/Output)
#define	BM_MEM_DIO_STOP					(pMasAddr->dwPort2_mem+0xB000006)	///< Clear interrupt status					(Input/Output)
#define	BM_MEM_Reset					(pMasAddr->dwPort2_mem+0xB000044)	///< Reset									(Input/Output)
#define	BM_MEM_IntStatus				(pMasAddr->dwPort2_mem+0xB00004c)	///< Interrupt status						(Input/Output)
#define	BM_MEM_CRA						(pMasAddr->dwPort_mem + 0x1000)		///< Address Translation Table				(Input/Output)
#define	BM_MEM_CRA_HIGH					(pMasAddr->dwPort_mem + 0x1004)		///< Address Translation Table (High)		(Input/Output)

//--------------------------------------------------
// Macro definition
//--------------------------------------------------
// Memory Input/Output
#define	_inm(adr)						readb(adr)
#define	_inmw(adr)						readw(adr)
#define	_inmd(adr)						readl(adr)
#define	_outm(adr, data)				writeb(data, adr)
#define	_outmw(adr, data)				writew(data, adr)
#define	_outmd(adr, data)				writel(data, adr)


//========================================================================
// Function name  : ApiMemBmCreate
// Function	  	  : Initialize the hardware of the BusMaster and the MASTERADDR structure.
// I/F	 		  : External
// In	 		  : dwPortBmAddr	 	: Specify the base address (bus master) of the board.
//		   			dwPortPciAddr 	 	: Specify the address for PCI.
// Out	 		  : pMasAddr		 	: Specify the address of the MASTERADDR structure.
//		   			ppci_dev		 	: Address of the pci_dev structure.
// Return value   : Normal completed 	: BM_ERROR_SUCCESS
//		   			Abnormal completed	: BM_ERROR_PARAM
// Addition  	  : This function is called only once in the ccom_entry->add_device 
//					or in the corresponding portion
//========================================================================
long ApiMemBmCreate(PMASTERADDR pMasAddr, struct pci_dev *ppci_dev, unsigned char * dwPortBmAddr, unsigned char * dwPortPciAddr)
{
	long	lret;

	//--------------------------------------------
	// Initialize the variables and structures
	//--------------------------------------------
	lret	= BM_ERROR_SUCCESS;
	memset(pMasAddr, 0, sizeof(MASTERADDR));
	pMasAddr->BmOut.dwIntMask	= 0x9f;
	pMasAddr->BmInp.dwIntMask	= 0x9f;
	//--------------------------------------------
	// Check the parameters
	//--------------------------------------------
	if (dwPortBmAddr == (unsigned char *)0x0 || dwPortBmAddr == (unsigned char *)0xffffffff ||
		dwPortPciAddr == (unsigned char *)0x0 || dwPortPciAddr == (unsigned char *)0xffffffff) {
		return BM_ERROR_PARAM;
	}
	//--------------------------------------------
	// Set the memory base address
	//--------------------------------------------
	pMasAddr->dwPort_mem	= dwPortBmAddr;
	pMasAddr->dwPort2_mem	= dwPortPciAddr;
	//--------------------------------------------
	// Set the pci_dev structure
	//--------------------------------------------
	memcpy(&pMasAddr->PciDev, ppci_dev, sizeof(struct pci_dev));
	//--------------------------------------------
	// Mask the interrupt
	//--------------------------------------------
	_outmd(BM_MEM_INP_IntMask, 0xffffffff);
	//--------------------------------------------
	// Reset the board
	//--------------------------------------------
	lret	= ApiMemBmReset(pMasAddr, BM_RESET_ALL);
	if (lret != BM_ERROR_SUCCESS) {
		return	lret;
	}

	return	BM_ERROR_SUCCESS;
}

//========================================================================
// Function name  : ApiMemBmDestroy
// Function	  	  : Exit processing of the hardware and MASTERADDR structure of the BusMaster,
//					post-processing of the buffer
// I/F	 	  	  : External
// In	 	  	  : pMasAddr			: Point to the MASTERADDR structure
// Out	 	  	  : 
// Return value   : Normal completed	: BM_ERROR_SUCCESS
// Addition  	  : This function is called only once in driver exit processing.
//========================================================================
long ApiMemBmDestroy(PMASTERADDR pMasAddr)
{
	long	lret = BM_ERROR_SUCCESS;
	
	//--------------------------------------------
	// Stop transfer (Both directions)
	//--------------------------------------------
	lret = ApiMemBmStop(pMasAddr, BM_DIR_IN | BM_DIR_OUT);
	if (lret != BM_ERROR_SUCCESS) {
		return	lret;
	}
	//---------------------------------------------
	// Unlock the memory
	//---------------------------------------------
	lret = ApiMemBmUnlockMem(pMasAddr, BM_DIR_IN);
	lret = ApiMemBmUnlockMem(pMasAddr, BM_DIR_OUT);
	//--------------------------------------------
	// Mask the interrupt
	//--------------------------------------------
	_outmd(BM_MEM_INP_IntMask, 0xffffffff);
	_outmd(BM_MEM_INP_IntClear, 0xffffffff);
	//--------------------------------------------
	// Initialize the variables and structures
	//--------------------------------------------
	memset(pMasAddr, 0, sizeof(MASTERADDR));
	pMasAddr->BmOut.dwIntMask = 0x9f;
	pMasAddr->BmInp.dwIntMask = 0x9f;

	return BM_ERROR_SUCCESS;
}
//========================================================================
// Function name  : ApiMemBmSetBuffer
// Function	      : Set information such as the user memory.
// I/F 		  	  : External
// In 		  	  : pMasAddr			: Point to the MASTERADDR structure
//		   			dwDir				: Transfer direction (BM_DIR_IN / BM_DIR_OUT)
//		   			Buff				: Start point to the user memory
//		   			dwLen				: Size (number of data) of the user memory
//		   			dwIsRing			: Whether it is ring buffer or not (BM_WRITE_ONCE/BM_WRITE_RING)
// Out	 	  	  : pMasAddr 			: Store the various information
// Return value   : Normal completed	: BM_ERROR_SUCCESS
//		   		   	Abnormal completed	: BM_ERROR_PARAM
// Addition	  	  : 
//		   
//========================================================================
long ApiMemBmSetBuffer(PMASTERADDR pMasAddr, unsigned long dwDir, void *Buff, unsigned long dwLen, unsigned long dwIsRing)
{
	long			lret = BM_ERROR_SUCCESS;
	BMEMBER			*pBmTmp;
	int				direction;
	unsigned long	dwdata_inp, dwdata_out;
	
	//---------------------------------------------
	// Check the parameters (transfer direction)
	//---------------------------------------------
	if (dwDir != BM_DIR_IN && dwDir != BM_DIR_OUT) {
		return BM_ERROR_PARAM;
	}
	//---------------------------------------------
	// Error during transfer
	//---------------------------------------------
	if (dwDir == BM_DIR_IN) {
		dwdata_inp	= _inmd(BM_MEM_INP_CSR);
		if (((dwdata_inp & BM_MEM_BUSY) == BM_MEM_BUSY) || (pMasAddr->BmInp.dmastopflag == 0)){
			return BM_ERROR_SEQUENCE;
		}
	} else {
		dwdata_out	= _inmd(BM_MEM_OUT_CSR);
		if (((dwdata_out & BM_MEM_BUSY) == BM_MEM_BUSY) || (pMasAddr->BmOut.dmastopflag == 0)){
			return BM_ERROR_SEQUENCE;
		}
	}
	//---------------------------------------------
	// Select the I/O data structure
	//---------------------------------------------
	if (dwDir == BM_DIR_IN) {
		pBmTmp	= &pMasAddr->BmInp;
	} else {
		pBmTmp	= &pMasAddr->BmOut;
	}
	//---------------------------------------------
	// If the previous buffer is not unlocked, unlock it
	//---------------------------------------------
	if (pBmTmp->SgList.dwBuffLen != 0) {
		lret = ApiMemBmUnlockMem(pMasAddr, dwDir);
		if (lret != BM_ERROR_SUCCESS) {
			return	lret;
		}
	}
	//---------------------------------------------
	// Check the parameters (Check if the buffer is NULL)
	//---------------------------------------------
	if (Buff == NULL) {
		ApiMemBmUnlockMem(pMasAddr, dwDir);
		return BM_ERROR_PARAM;
	}
	//---------------------------------------------
	// Check the parameters (the number of data in the buffer)
	//---------------------------------------------
	if (dwLen == 0) {
		ApiMemBmUnlockMem(pMasAddr, dwDir);
		return BM_ERROR_PARAM;
	}
	//---------------------------------------------
	// Save point to the user memory
	//---------------------------------------------
	pBmTmp->SgList.Buff			= (unsigned char*)Buff;
	pBmTmp->SgList.dwBuffLen	= dwLen;
	pBmTmp->SgList.dwIsRing		= dwIsRing;
	//---------------------------------------------
	// Map the buffer of user space to kernel space
	//---------------------------------------------
	lret = MemBmMakeSGList(&pBmTmp->SgList, dwDir);
	if (lret != BM_ERROR_SUCCESS) {
		ApiMemBmUnlockMem(pMasAddr, dwDir);
		return	lret;
	}
	//--------------------------------------------
	// Specify the transfer direction
	//--------------------------------------------
	if(dwDir == BM_DIR_OUT){
		direction = PCI_DMA_TODEVICE;
	}else{
		direction = PCI_DMA_FROMDEVICE;
	}
	//--------------------------------------------
	// Map the scatter list
	//--------------------------------------------
	pci_map_sg(&pMasAddr->PciDev, 
				pBmTmp->SgList.List,
				pBmTmp->SgList.nr_pages,
				direction);
	//---------------------------------------------
	// Set S/G list to the board
	//---------------------------------------------
	if ((dwIsRing == 1) && (dwLen < 0x32)){
		if (dwDir == BM_DIR_IN) {
			lret	= MemBmSetSGAddress(pMasAddr, dwDir);
		} else {
			if (dwLen < 0x1C){
				lret	= MemBmSetSGAddressRingPreLittle(pMasAddr, dwDir);
			} else {
				lret	= MemBmSetSGAddressRingPre(pMasAddr, dwDir);
			}
		}
	} else if ((dwIsRing == 1) && (dwLen >= 0x32) && (dwLen < 0x1000)){
		if (dwDir == BM_DIR_IN) {
			if (dwLen <= 0x800){
				lret	= MemBmSetSGAddress(pMasAddr, dwDir);
			} else {
				lret	= MemBmSetSGAddressRingPre(pMasAddr, dwDir);
			}
		} else {
			lret	= MemBmSetSGAddressRingPre(pMasAddr, dwDir);
		}
	} else {
		lret	= MemBmSetSGAddress(pMasAddr, dwDir);
	}
	return	lret;
}

//========================================================================
// Function name  : MemBmMakeSGList
// Function 	  : Map the buffer of user space to kernel space and generate a list 
//					that can be registered in the hard S/G list.
// I/F	 		  : Internal
// In	 		  : SgList			: Point to the scatter list structure
//		   			dwDir			: BM_DIR_IN / BM_DIR_OUT
// Out	 		  : SgList
// Return value   : 
//========================================================================
long MemBmMakeSGList(PSGLIST pSgList, unsigned long dwDir)
{
	int				rw;				// Transfer direction
	int				page_no;
	unsigned long	buf_length;
	unsigned long	buf_offset;
	int				pgcount;
	unsigned long	v_address;		// Start virtual address of the user buffer
	struct page		**maplist;		// Map list

	//--------------------------------------------
	// Check the parameters
	//--------------------------------------------
	if (dwDir != BM_DIR_IN && dwDir != BM_DIR_OUT) {
		return	BM_ERROR_PARAM;
	}
	//--------------------------------------------
	// Transfer direction is specified by READ, WRITE which defined in /linux/fs.h
	//--------------------------------------------
	if(dwDir == BM_DIR_OUT){
		rw = WRITE;
	}else{
		rw = READ;
	}
	//--------------------------------------------
	// Get the size of the user buffer
	//--------------------------------------------
	buf_length	= sizeof(unsigned int) * pSgList->dwBuffLen;
	//--------------------------------------------
	// Get the start virtual address of the user buffer
	//--------------------------------------------
	v_address	= (unsigned long) pSgList->Buff;
	//--------------------------------------------
	// Get the required number of pages from the user buffer size
	// and allocate memory to the page structure for storing the address information
	//--------------------------------------------
	pgcount	= ((v_address + buf_length + PAGE_SIZE - 1) / PAGE_SIZE) - (v_address / PAGE_SIZE);
	maplist	= kmalloc(pgcount * sizeof(struct page **), GFP_KERNEL);
	if (maplist == NULL){
		return BM_ERROR_MEM;
	}
	//--------------------------------------------
	// Get the offset of the buffer
	//--------------------------------------------
	buf_offset	= v_address & (PAGE_SIZE-1);
	//--------------------------------------------
	// Retrieve the page structure of the user buffer.
	//--------------------------------------------
	down_read(&current->mm->mmap_sem);
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4,15,0))
	pSgList->nr_pages	= get_user_pages(v_address, pgcount, rw == READ, maplist, NULL);
#else
	pSgList->nr_pages	= get_user_pages(current, current->mm, v_address, pgcount, rw == READ, 0, maplist, NULL);
#endif
	up_read(&current->mm->mmap_sem);
	if (pSgList->nr_pages < 0) {
		pSgList->nr_pages = 0;
		return BM_ERROR_LOCK_MEM;
	}
	//---------------------------
	// Allocate the sg list memory
	//---------------------------
	pSgList->List	= Ccom_alloc_pages(GFP_KERNEL, (pSgList->nr_pages * sizeof(struct scatterlist)));
	if(pSgList->List == NULL){
		return BM_ERROR_MEM;
	}
	//---------------------------
	// Initialize sg List memory with 0
	//---------------------------
	memset(pSgList->List, 0, sizeof(struct scatterlist) * pSgList->nr_pages);

	//------------------------------------------
	// When use only one page
	//------------------------------------------
	if(pSgList->nr_pages == 1){
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,0) && LINUX_VERSION_CODE < KERNEL_VERSION(2,6,24))
		pSgList->List[0].page		= maplist[0];
#elif ((LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,24) && LINUX_VERSION_CODE < KERNEL_VERSION(2,7,0)) || \
	    (LINUX_VERSION_CODE >= KERNEL_VERSION(3,10,0)))
		pSgList->List[0].page_link	= (unsigned long)maplist[0];
#endif
		pSgList->List[0].offset		= buf_offset;
		pSgList->List[0].length		= buf_length;
	//------------------------------------------
	// When use the multiple pages
	//------------------------------------------
	}else{
		//-----------------------------
		// Set the page 0
		//-----------------------------
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,0) && LINUX_VERSION_CODE < KERNEL_VERSION(2,6,24))
		pSgList->List[0].page		= maplist[0];
#elif ((LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,24) && LINUX_VERSION_CODE < KERNEL_VERSION(2,7,0)) || \
	    (LINUX_VERSION_CODE >= KERNEL_VERSION(3,10,0)))
		pSgList->List[0].page_link	= (unsigned long)maplist[0];
#endif
		pSgList->List[0].offset		= buf_offset;
		pSgList->List[0].length		= PAGE_SIZE - buf_offset;
		//-----------------------------
		// Set the intermediate page
		//-----------------------------
		for (page_no = 1; page_no < (pSgList->nr_pages - 1); page_no++){
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,0) && LINUX_VERSION_CODE < KERNEL_VERSION(2,6,24))
			pSgList->List[page_no].page			= maplist[page_no];
#elif ((LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,24) && LINUX_VERSION_CODE < KERNEL_VERSION(2,7,0)) || \
	    (LINUX_VERSION_CODE >= KERNEL_VERSION(3,10,0)))
			pSgList->List[page_no].page_link	= (unsigned long)maplist[page_no];
#endif
			pSgList->List[page_no].offset		= 0;
			pSgList->List[page_no].length		= PAGE_SIZE;
		}
		//-----------------------------
		// Set the last page
		//-----------------------------
		page_no = pSgList->nr_pages - 1;
		if(buf_offset){
			pSgList->List[page_no].length	= (buf_offset + buf_length) % PAGE_SIZE ;
		}else{
			pSgList->List[page_no].length	= PAGE_SIZE;
		}
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,0) && LINUX_VERSION_CODE < KERNEL_VERSION(2,6,24))
		pSgList->List[page_no].page			= maplist[page_no];
#elif ((LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,24) && LINUX_VERSION_CODE < KERNEL_VERSION(2,7,0)) || \
	    (LINUX_VERSION_CODE >= KERNEL_VERSION(3,10,0)))
		pSgList->List[page_no].page_link	= (unsigned long)maplist[page_no];
#endif
		pSgList->List[page_no].offset		= 0;
	}
	//--------------------------------------------
	// Free the maplist (page structure) memory
	//--------------------------------------------
	kfree(maplist);

	return	BM_ERROR_SUCCESS;
}

//========================================================================
// Function name  : MemBmSetSGAddress
// Function		  : Set the S/G list to hardware
// I/F	 	  	  : Internal
// In	 		  : pMasAddr			: Point to the MASTERADDR structure
//		   		 	dwDir				: BM_DIR_IN / BM_DIR_OUT
// Out			  : None
// Return value   : Normal completed	: BM_ERROR_SUCCESS
//		   			Abnormal completed	: 
// Addition  	  : 
//========================================================================
long MemBmSetSGAddress(PMASTERADDR pMasAddr, unsigned long dwDir)
{
	unsigned long		i;
	PSGLIST 			pSgList;
	unsigned long long	desc_cal;
	unsigned long long	add_cal, add_cal_n;
	unsigned long long	add_cal_res;
	unsigned long long	add_cal_dev;
	unsigned long 		dwdata;
	unsigned long		desc_low_o_n, desc_high_o_n;
	unsigned long		desc_low_i_n, desc_high_i_n;
	unsigned long		cradata, cradata2;
	unsigned long		add_low_i, add_high_i;
	unsigned long		add_low_o, add_high_o;
	unsigned long		add_low_i_dev, add_high_i_dev;
	unsigned long		add_low_o_dev, add_high_o_dev;
	unsigned long		ListSize, ListNum;
	unsigned long		LengthStack, Lengthdev, Lengthrem;
	unsigned long		ListMove, n;
	unsigned long		OverHalf, HalfLen, loop;
	unsigned long		Outlen;
	unsigned long		readlist;
	int					UnderLen;
	int					SetDesc, DummySet;
	
	//--------------------------------------------
	// Check the parameters
	//--------------------------------------------
	if (dwDir != BM_DIR_IN && dwDir != BM_DIR_OUT) {
		return	BM_ERROR_PARAM;
	}
	if (dwDir == BM_DIR_IN) {
		pSgList	= &pMasAddr->BmInp.SgList;
	} else {
		pSgList	= &pMasAddr->BmOut.SgList;
	}
	if (pSgList->nr_pages > BM_MEM_MAX_SG_SIZE) {
		return BM_ERROR_BUFF;
	}

	//---------------------------------------------
	// Setting FIFO address
	//---------------------------------------------
	if(dwDir & BM_DIR_IN){
		add_cal						= (unsigned long long)BM_MEM_INP_ReadAdd;
		ApiMemBmDivideAddress(add_cal, &add_low_i, &add_high_i);
		add_cal_dev					= add_cal;
		add_low_i_dev				= add_low_i;
		add_high_i_dev				= add_high_i;
		pMasAddr->BmInp.addtablenum	= 0;
		HalfLen						= 0;
		for (loop=0; loop < pSgList->nr_pages; loop++) {
			HalfLen	+=  pMasAddr->BmInp.SgList.List[loop].length;
		}
		pMasAddr->BmInp.sglen		= HalfLen / 4;
	} else {
		add_cal						= (unsigned long long)BM_MEM_OUT_WriteAdd;
		ApiMemBmDivideAddress(add_cal, &add_low_o, &add_high_o);
		add_cal_dev					= add_cal;
		add_low_o_dev				= add_low_o;
		add_high_o_dev				= add_high_o;
		pMasAddr->BmOut.addtablenum	= 0;
	}
	add_cal_n	= add_cal;
	ListSize	= pSgList->nr_pages;
	//--------------------------------------------
	// If the number of SGList exceeds a certain number, limit the number of lists for reconfiguration
	//--------------------------------------------
	if (ListSize > 150){
		if(dwDir & BM_DIR_IN){
			pMasAddr->BmInp.addtablenum	= ListSize - 150;
			pMasAddr->BmInp.athalflen	= 0;
			HalfLen	= 0;
			for (loop=0; loop < 75; loop++) {
				HalfLen	+=  pMasAddr->BmInp.SgList.List[loop].length;
			}
			pMasAddr->BmInp.athalflen	= (((HalfLen / 4) / 2048) / 2) + 1;
		} else {
			pMasAddr->BmOut.addtablenum	= ListSize - 150;
			pMasAddr->BmOut.athalflen	= 0;
			HalfLen	= 0;
			for (loop=0; loop < 75; loop++) {
				HalfLen	+=  pMasAddr->BmOut.SgList.List[loop].length;
			}
			pMasAddr->BmOut.athalflen	= (((HalfLen / 4) / 2048) / 2) + 1;
		}
		ListSize	= 150;
	}
	LengthStack	= 0;
	n			= 0;
	ListMove	= 0;
	Lengthdev	= 0;
	Lengthrem	= 0;
	ListNum		= 0;
	SetDesc		= 0;
	DummySet	= 1;
	OverHalf	= 0;
	UnderLen	= 0;
	Outlen		= 0;
	//--------------------------------------------
	// Loop the number of times
	//--------------------------------------------
	for (i=0; i < ListSize; i++) {
		ListNum++;
		//--------------------------------------------
		// Set in master register
		//--------------------------------------------
		if(dwDir == BM_DIR_IN){	
			//--------------------------------------------
			// Set the Address Translation Table
			//--------------------------------------------
			if (pMasAddr->add64flag == 1){
				_outmd((BM_MEM_CRA + (0x8 * i)), 0xFFFFFFFD);
				dwdata		= _inmd(BM_MEM_CRA + (0x8 * i));
				_outmd((BM_MEM_CRA + (0x8 * i)), (((dwdata) & (sg_dma_address(&pSgList->List[i]))) | 0x1));
				_outmd((BM_MEM_CRA_HIGH + (0x8 * i)), ((sg_dma_address(&pSgList->List[i])) >> 32 & (0xffffffff)));
				cradata		= _inmd(BM_MEM_CRA + (0x8 * i));
				cradata2	= _inmd(BM_MEM_CRA_HIGH + (0x8 * i));
			} else {
				_outmd((BM_MEM_CRA + (0x8 * i)), 0xFFFFFFFC);
				dwdata		= _inmd(BM_MEM_CRA + (0x8 * i));
				_outmd((BM_MEM_CRA + (0x8 * i)), (dwdata & (sg_dma_address(&pSgList->List[i]))));
				_outmd((BM_MEM_CRA_HIGH + (0x8 * i)), ((sg_dma_address(&pSgList->List[i])) >> 32 & (0xffffffff)));
				cradata		= _inmd(BM_MEM_CRA + (0x8 * i));
				cradata2	= _inmd(BM_MEM_CRA_HIGH + (0x8 * i));
			}
			LengthStack	+= pSgList->List[i].length;
			//--------------------------------------------
			// Move FIFO address for second and subsequent lists
			//--------------------------------------------
			if (i > 0){
				if (SetDesc == 1){
					SetDesc	= 0;
				} else {
					add_cal	= (unsigned long long)(add_cal_n + pSgList->List[i - 1].length);
				}
				ApiMemBmDivideAddress(add_cal, &add_low_i, &add_high_i);
			}
			add_cal_n	= add_cal;
			//--------------------------------------------
			// Set the beginning of FIFO when block exceeds 4kByte boundary
			//--------------------------------------------
			if ((LengthStack >= 0x2000) && (OverHalf == 1)){
				Lengthdev	= LengthStack - 0x2000;
				add_cal_n	= add_cal_dev;
				Lengthrem	= 0x2000 - (LengthStack - pSgList->List[i].length);
			}
			//--------------------------------------------
			// Specify the beginning of the next Descriptor Format in NextDescPtr
			//--------------------------------------------
			desc_cal	= (unsigned long long)(BM_MEM_INP_NXT_DESC + (0x40 * (i + ListMove + 1)));
			ApiMemBmDivideAddress(desc_cal, &desc_low_i_n, &desc_high_i_n);
			//--------------------------------------------
			// Set Descriptor Format to Prefetcher FIFO
			//--------------------------------------------
			_outmd((BM_MEM_INP_Read_SGAddr + (0x40 * (i + ListMove + 1))), add_low_i);
			_outmd((BM_MEM_INP_Read_SGAddr_high + (0x40 * (i + ListMove + 1))), add_high_i);
			_outmd((BM_MEM_INP_Write_SGAddr + (0x40 * (i + ListMove + 1))), (((sg_dma_address(&pSgList->List[i])) & ~dwdata) + (BM_MEM_CRA_SELECT * i)));
			_outmd((BM_MEM_INP_Write_SGAddr_high + (0x40 * (i + ListMove + 1))), ((sg_dma_address(&pSgList->List[i])) >> 32 & (0xffffffff)));
			_outmd((BM_MEM_INP_Length + (0x40 * (i + ListMove + 1))), pSgList->List[i].length);
			_outmd((BM_MEM_INP_NextDescPtr + (0x40 * (i + ListMove + 1))), desc_low_i_n);
			_outmd((BM_MEM_INP_NextDescPtr_high + (0x40 * (i + ListMove + 1))), desc_high_i_n);
			_outmw((BM_MEM_INP_SequenceNum + (0x40 * (i + ListMove + 1))), (i + ListMove + 1));
			_outmd((BM_MEM_INP_Stride + (0x40 * (i + ListMove + 1))), 0x0);
			_outmw((BM_MEM_INP_Burst + (0x40 * (i + ListMove + 1))), 0x101);
			_outmd((BM_MEM_INP_Descriptor + (0x40 * (i + ListMove + 1))), 0x40000000);
			//--------------------------------------------
			// Transfer setting of final transfer block
			//--------------------------------------------
			if ((LengthStack <= 0x2000) && (i == (ListSize - 1))){
				if ((LengthStack == 0x2000) && (OverHalf == 1)){
					add_cal_res	= (unsigned long long)BM_MEM_INP_ReadAdd;
				} else {
					add_cal_res	= (unsigned long long)(add_cal_n + pSgList->List[i].length);
				}
				pMasAddr->BmInp.resetadd	= add_cal_res;
				UnderLen					= 1;
				//--------------------------------------------
				// Address setting at SgList overwrite
				//--------------------------------------------
				if (pSgList->nr_pages > 150){
					if (LengthStack != 0x2000){
						desc_cal	= (unsigned long long)BM_MEM_INP_NXT_DESC;
					} else {
						desc_cal	= (unsigned long long)BM_MEM_INP_TOP_DESC;
					}
					ApiMemBmDivideAddress(desc_cal, &desc_low_i_n, &desc_high_i_n);
					_outmd((BM_MEM_INP_NextDescPtr + (0x40 * (i + ListMove + 1))), desc_low_i_n);
					_outmd((BM_MEM_INP_NextDescPtr_high + (0x40 * (i + ListMove + 1))), desc_high_i_n);
				} else {
					desc_cal		= (unsigned long long)BM_MEM_INP_TOP_DESC;
					ApiMemBmDivideAddress(desc_cal, &desc_low_i_n, &desc_high_i_n);
					_outmd((BM_MEM_INP_NextDescPtr + (0x40 * (i + ListMove + 1))), desc_low_i_n);
					_outmd((BM_MEM_INP_NextDescPtr_high + (0x40 * (i + ListMove + 1))), desc_high_i_n);
					_outmd((BM_MEM_INP_Descriptor + (0x40 * (i + ListMove + 1))), 0x40004000);
				}
				//--------------------------------------------
				// Save various parameters for infinite transfer
				//--------------------------------------------
				if ((i == (ListSize - 1)) && (pMasAddr->BmInp.SgList.dwIsRing == 1)){
					//--------------------------------------------
					// If the SGList number is 150 or less, the transfer command will not be reset
					// Set transfer completion interrupt flag
					//--------------------------------------------
					if (pSgList->nr_pages <= 150){
						_outmd((BM_MEM_INP_Descriptor + (0x40 * (i + ListMove + 1))), 0x40004000);
					}
					//--------------------------------------------
					// If the final DMA transfer is 2KByte,
					// Hold the parameters of the first List
					//--------------------------------------------
					if (LengthStack == 0x2000){
						if (OverHalf == 0){
							OverHalf	= 1;
						} else {
							OverHalf	= 0;
						}
						add_cal								= (unsigned long long)(add_cal_n + pSgList->List[i].length);
						ApiMemBmDivideAddress(add_cal, &add_low_i, &add_high_i);
						pSgList->MemParam.FIFOOverAdd		= add_low_i;
						pSgList->MemParam.FIFOOverAdd_high	= add_high_i;
						pSgList->MemParam.SGOverAdd			= sg_dma_address(&pSgList->List[0]);
						pSgList->MemParam.SGOverAdd_high	=((sg_dma_address(&pSgList->List[0])) >> 32 & (0xffffffff));
						pSgList->MemParam.OverLen			= pSgList->List[0].length;
						pSgList->MemParam.ATNumber			= 0;
					} else if (pMasAddr->BmInp.sglen <= 0x800){
						add_cal								= (unsigned long long)(add_cal_n + pSgList->List[i].length);
						ApiMemBmDivideAddress(add_cal, &add_low_i, &add_high_i);
						pSgList->MemParam.FIFOOverAdd		= add_low_i;
						pSgList->MemParam.FIFOOverAdd_high	= add_high_i;
						pSgList->MemParam.SGOverAdd			= sg_dma_address(&pSgList->List[0]);
						pSgList->MemParam.SGOverAdd_high	=((sg_dma_address(&pSgList->List[0])) >> 32 & (0xffffffff));
						pSgList->MemParam.OverLen			= pSgList->List[0].length;
						pSgList->MemParam.ATNumber			= 0;
					} else {
						//--------------------------------------------
						// To reconfigure the list itself, the DescriptorList to chain
						// Hold the parameters of the first List
						//--------------------------------------------
						if (OverHalf == 0){
							add_cal								= (unsigned long long)(add_cal_dev);
							ApiMemBmDivideAddress(add_cal, &add_low_i, &add_high_i);
							pSgList->MemParam.FIFOOverAdd		= add_low_i;
							pSgList->MemParam.FIFOOverAdd_high	= add_high_i;
						} else {
							add_cal								= (unsigned long long)(add_cal_n);
							ApiMemBmDivideAddress(add_cal, &add_low_i, &add_high_i);
							pSgList->MemParam.FIFOOverAdd		= add_low_i;
							pSgList->MemParam.FIFOOverAdd_high	= add_high_i;
							pMasAddr->BmInp.resetListMove		= 0;
						}
						//--------------------------------------------
						// If this is the first List in the DescriptorList chained by this loop,
						// keep the current parameters
						//--------------------------------------------
						readlist	= _inmd((BM_MEM_INP_NextDescPtr + (0x40 * (i + ListMove))));
						if (readlist == BM_MEM_INP_TOP_DESC){
							pSgList->MemParam.SGOverAdd			= sg_dma_address(&pSgList->List[i]);
							pSgList->MemParam.SGOverAdd_high	= ((sg_dma_address(&pSgList->List[i])) >> 32 & (0xffffffff));
							pSgList->MemParam.OverLen			= pSgList->List[i].length;
							pSgList->MemParam.ATNumber			= i;
						} else {
							//--------------------------------------------
							// If the current loop is not the first List in the DescriptorList to chain,
							// search the first List and keep its parameters
							//--------------------------------------------
							loop	= 0;
							while (loop	< 100){
								readlist	= _inmd((BM_MEM_INP_NextDescPtr + (0x40 * (i + ListMove - loop))));
								if (readlist == BM_MEM_INP_TOP_DESC){
									readlist					= _inmd((BM_MEM_INP_Length + (0x40 * (i + ListMove + 1 - loop))));
									if (readlist == Lengthdev){
										pSgList->MemParam.SGOverAdd			= sg_dma_address(&pSgList->List[i - loop]) + (pSgList->List[i - loop].length - Lengthdev);
										pSgList->MemParam.SGOverAdd_high	= ((sg_dma_address(&pSgList->List[i - loop])) >> 32 & (0xffffffff));
										pSgList->MemParam.OverLen			= Lengthdev;
										pSgList->MemParam.ATNumber			= i - loop;
									} else {
										pSgList->MemParam.SGOverAdd			= sg_dma_address(&pSgList->List[i - loop -1]);
										pSgList->MemParam.SGOverAdd_high	= ((sg_dma_address(&pSgList->List[i - loop -1])) >> 32 & (0xffffffff));
										pSgList->MemParam.OverLen			= pSgList->List[i - loop - 1].length;
										pSgList->MemParam.ATNumber			= i - loop - 1;
									}
									if (OverHalf != 0){
										readlist							= _inmd((BM_MEM_INP_Read_SGAddr + (0x40 * (i + ListMove + 1 - loop))));
										pSgList->MemParam.FIFOOverAdd		= readlist;
										readlist							= _inmd((BM_MEM_INP_Read_SGAddr_high + (0x40 * (i + ListMove + 1 - loop))));
										pSgList->MemParam.FIFOOverAdd_high	= readlist;
									}
									
									break;
								}
								loop++;
							}
						}
					}
				}
			}
			//--------------------------------------------
			// Divide one block into 2k bytes
			//--------------------------------------------
			if ((LengthStack == 0x2000) && (i != (ListSize - 1))){
				//--------------------------------------------
				// Store the next Descriptor start address
				//--------------------------------------------
				desc_cal					= (unsigned long long)(BM_MEM_INP_NXT_DESC + (0x40 * (i + ListMove)));
				ApiMemBmDivideAddress(desc_cal, &desc_low_i_n, &desc_high_i_n);
				pSgList->ChangeDescAdd[n]	= desc_low_i_n;
				if (DummySet == 1){
					DummySet	= 0;
				} else {
				//--------------------------------------------
				// Set FIFO start address to NextDescPtr
				//--------------------------------------------
					desc_cal	= (unsigned long long)BM_MEM_INP_TOP_DESC;
					ApiMemBmDivideAddress(desc_cal, &desc_low_i_n, &desc_high_i_n);
					_outmd((BM_MEM_INP_NextDescPtr + (0x40 * (i + ListMove + 1))), desc_low_i_n);
					_outmd((BM_MEM_INP_NextDescPtr_high + (0x40 * (i + ListMove + 1))), desc_high_i_n);
				}
				LengthStack					= Lengthdev;
				ListNum						= 0;
				n++;
				if (OverHalf == 0){
					OverHalf	= 1;
				} else {
					OverHalf	= 0;
				}
			} else if (LengthStack > 0x2000){
				//--------------------------------------------
				// Split transfer number of 1 block into 2k bytes
				//--------------------------------------------
				Lengthdev					= LengthStack - 0x2000;
				Lengthrem					= 0x2000 - (LengthStack - pSgList->List[i].length);
				_outmd((BM_MEM_INP_Length + (0x40 * (i + ListMove + 1))), Lengthrem);
				//--------------------------------------------
				// Store the next Descriptor start address
				//--------------------------------------------
				desc_cal					= (unsigned long long)(BM_MEM_INP_NXT_DESC + (0x40 * (i + ListMove)));
				ApiMemBmDivideAddress(desc_cal, &desc_low_i_n, &desc_high_i_n);
				pSgList->ChangeDescAdd[n]	= desc_low_i_n;
				//--------------------------------------------
				// Set FIFO start address to NextDescPtr
				//--------------------------------------------
				if ((pMasAddr->BmInp.SgList.dwIsRing == 1) && (pMasAddr->BmInp.sglen >= 0x1000)){
					desc_cal	= (unsigned long long)BM_MEM_INP_TOP_DESC;
					ApiMemBmDivideAddress(desc_cal, &desc_low_i_n, &desc_high_i_n);
					_outmd((BM_MEM_INP_NextDescPtr + (0x40 * (i + ListMove + 1))), desc_low_i_n);
					_outmd((BM_MEM_INP_NextDescPtr_high + (0x40 * (i + ListMove + 1))), desc_high_i_n);
				} else if (pMasAddr->BmInp.SgList.dwIsRing == 0){
					desc_cal	= (unsigned long long)BM_MEM_INP_TOP_DESC;
					ApiMemBmDivideAddress(desc_cal, &desc_low_i_n, &desc_high_i_n);
					_outmd((BM_MEM_INP_NextDescPtr + (0x40 * (i + ListMove + 1))), desc_low_i_n);
					_outmd((BM_MEM_INP_NextDescPtr_high + (0x40 * (i + ListMove + 1))), desc_high_i_n);
				}
				DummySet					= 0;
				//--------------------------------------------
				// Specify the beginning of the next Descriptor Format in NextDescPtr
				//--------------------------------------------
				ListMove++;
				desc_cal					= (unsigned long long)(BM_MEM_INP_NXT_DESC + (0x40 * (i + ListMove + 1)));
				ApiMemBmDivideAddress(desc_cal, &desc_low_i_n, &desc_high_i_n);
				//--------------------------------------------
				// Set Descriptor Format to Prefetcher FIFO
				//--------------------------------------------
				_outmd((BM_MEM_INP_Read_SGAddr + (0x40 * (i + ListMove + 1))), add_low_i_dev);
				_outmd((BM_MEM_INP_Read_SGAddr_high + (0x40 * (i + ListMove + 1))), add_high_i_dev);
				_outmd((BM_MEM_INP_Write_SGAddr + (0x40 * (i + ListMove + 1))), (((sg_dma_address(&pSgList->List[i]) + (pSgList->List[i].length - Lengthdev)) & ~dwdata) + (BM_MEM_CRA_SELECT * i)));
				_outmd((BM_MEM_INP_Write_SGAddr_high + (0x40 * (i + ListMove + 1))), ((sg_dma_address(&pSgList->List[i])) >> 32 & (0xffffffff)));
				_outmd((BM_MEM_INP_Length + (0x40 * (i + ListMove + 1))), Lengthdev);
				_outmd((BM_MEM_INP_NextDescPtr + (0x40 * (i + ListMove + 1))), desc_low_i_n);
				_outmd((BM_MEM_INP_NextDescPtr_high + (0x40 * (i + ListMove + 1))), desc_high_i_n);
				_outmw((BM_MEM_INP_SequenceNum + (0x40 * (i + ListMove + 1))), (i + ListMove +1));
				_outmd((BM_MEM_INP_Stride + (0x40 * (i + ListMove + 1))), 0x0);
				_outmw((BM_MEM_INP_Burst + (0x40 * (i + ListMove + 1))), 0x101);
				_outmd((BM_MEM_INP_Descriptor + (0x40 * (i + ListMove + 1))), 0x40000000);
				//--------------------------------------------
				// If the FIFO address of the next block exceeds 4 kBytes, set the FIFO start address
				//--------------------------------------------
				if (OverHalf == 0){
					//--------------------------------------------
					// Set FIFO address for next list
					//--------------------------------------------
					add_cal		= (unsigned long long)(add_cal_n + Lengthrem);
					ApiMemBmDivideAddress(add_cal, &add_low_i, &add_high_i);
					_outmd((BM_MEM_INP_Read_SGAddr + (0x40 * (i + ListMove + 1))), add_low_i);
					_outmd((BM_MEM_INP_Read_SGAddr_high + (0x40 * (i + ListMove + 1))), add_high_i);
					SetDesc		= 0;
					OverHalf	= 1;
				} else if (OverHalf == 1){
					//--------------------------------------------
					// Move FIFO address for next list
					//--------------------------------------------
					add_cal		= (unsigned long long)(add_cal_dev + Lengthdev);
					SetDesc		= 1;
					OverHalf	= 0;
				}
				//--------------------------------------------
				// Transfer setting of final transfer block
				//--------------------------------------------
				if (i == (ListSize - 1)){
					//--------------------------------------------
					// Set NextDescPtr to the beginning of Prefetcher
					//--------------------------------------------
					desc_cal					= (unsigned long long)BM_MEM_INP_TOP_DESC;
					ApiMemBmDivideAddress(desc_cal, &desc_low_i_n, &desc_high_i_n);
					add_cal_res					= (unsigned long long)(add_cal_n + pSgList->List[i].length);
					pMasAddr->BmInp.resetadd	= add_cal_res;
					//--------------------------------------------
					// Address setting at SgList overwrite
					//--------------------------------------------
					if (pSgList->nr_pages > 150){
						//--------------------------------------------
						// Set current list as final list
						//--------------------------------------------
						_outmd((BM_MEM_INP_NextDescPtr + (0x40 * (i + ListMove))), desc_low_i_n);
						_outmd((BM_MEM_INP_NextDescPtr_high + (0x40 * (i + ListMove))), desc_high_i_n);
						//--------------------------------------------
						//Set NextDescPtr to the beginning of Prefetcher
						//--------------------------------------------
						desc_cal	= (unsigned long long)BM_MEM_INP_NXT_DESC;
						ApiMemBmDivideAddress(desc_cal, &desc_low_i_n, &desc_high_i_n);
						_outmd((BM_MEM_INP_NextDescPtr + (0x40 * (i + ListMove + 1))), desc_low_i_n);
						_outmd((BM_MEM_INP_NextDescPtr_high + (0x40 * (i + ListMove + 1))), desc_high_i_n);
					} else {
						_outmd((BM_MEM_INP_Descriptor + (0x40 * (i + ListMove + 1))), 0x40004000);
						_outmd((BM_MEM_INP_NextDescPtr + (0x40 * (i + ListMove + 1))), desc_low_i_n);
						_outmd((BM_MEM_INP_NextDescPtr_high + (0x40 * (i + ListMove + 1))), desc_high_i_n);
					}
					//--------------------------------------------
					// Save various parameters for infinite transfer
					//--------------------------------------------
					if (pMasAddr->BmInp.SgList.dwIsRing == 1){
						if (OverHalf == 0){
							add_cal								= (unsigned long long)(add_cal_dev);
							ApiMemBmDivideAddress(add_cal, &add_low_i, &add_high_i);
							pSgList->MemParam.FIFOOverAdd		= add_low_i;
							pSgList->MemParam.FIFOOverAdd_high	= add_high_i;
						} else {
							add_cal								= (unsigned long long)(add_cal_n);
							ApiMemBmDivideAddress(add_cal, &add_low_i, &add_high_i);
							pSgList->MemParam.FIFOOverAdd		= add_low_i;
							pSgList->MemParam.FIFOOverAdd_high	= add_high_i;
							pMasAddr->BmInp.resetListMove		= 0;
						}
						pSgList->MemParam.SGOverAdd			= sg_dma_address(&pSgList->List[i]) + (pSgList->List[i].length - Lengthdev);
						pSgList->MemParam.SGOverAdd_high	= ((sg_dma_address(&pSgList->List[i])) >> 32 & (0xffffffff));
						pSgList->MemParam.OverLen			= Lengthdev;
						pSgList->MemParam.ATNumber			= i;
						if ((pMasAddr->BmInp.sglen % 0x1000) == 0){
							desc_cal	= (unsigned long long)BM_MEM_INP_NXT_DESC;
							ApiMemBmDivideAddress(desc_cal, &desc_low_i_n, &desc_high_i_n);
							_outmd((BM_MEM_INP_NextDescPtr + (0x40 * (i + ListMove + 1))), desc_low_i_n);
							_outmd((BM_MEM_INP_NextDescPtr_high + (0x40 * (i + ListMove + 1))), desc_high_i_n);
						}
					}
				}
				LengthStack	= Lengthdev;
				ListNum		= 1;
				n++;
			}	
		}else{
			//--------------------------------------------
			// Set the Address Translation Table
			//--------------------------------------------
			if (pMasAddr->add64flag == 1){
				_outmd((BM_MEM_CRA + (0x8 * (i + BM_MEM_AT_OFFSET))), 0xFFFFFFFD);
				dwdata		= _inmd(BM_MEM_CRA + (0x8 * (i + BM_MEM_AT_OFFSET)));
				_outmd((BM_MEM_CRA + (0x8 * (i + BM_MEM_AT_OFFSET))), ((dwdata & (sg_dma_address(&pSgList->List[i]))) | 0x1));
				_outmd((BM_MEM_CRA_HIGH + (0x8 * (i + BM_MEM_AT_OFFSET))), ((sg_dma_address(&pSgList->List[i])) >> 32 & (0xffffffff)));
				cradata		= _inmd(BM_MEM_CRA + (0x8 * (i + BM_MEM_AT_OFFSET)));
				cradata2	= _inmd(BM_MEM_CRA_HIGH + (0x8 * (i + BM_MEM_AT_OFFSET)));
			} else {
				_outmd((BM_MEM_CRA + (0x8 * (i + BM_MEM_AT_OFFSET))), 0xFFFFFFFC);
				dwdata		= _inmd(BM_MEM_CRA + (0x8 * (i + BM_MEM_AT_OFFSET)));
				_outmd((BM_MEM_CRA + (0x8 * (i + BM_MEM_AT_OFFSET))), (dwdata & (sg_dma_address(&pSgList->List[i]))));
				_outmd((BM_MEM_CRA_HIGH + (0x8 * (i + BM_MEM_AT_OFFSET))), ((sg_dma_address(&pSgList->List[i])) >> 32 & (0xffffffff)));
				cradata		= _inmd(BM_MEM_CRA + (0x8 * (i + BM_MEM_AT_OFFSET)));
				cradata2	= _inmd(BM_MEM_CRA_HIGH + (0x8 * (i + BM_MEM_AT_OFFSET)));
			}
			LengthStack	+= pSgList->List[i].length;
			//--------------------------------------------
			// Move FIFO address for second and subsequent lists
			//--------------------------------------------
			if (i > 0){
				if (SetDesc == 1){
					SetDesc	= 0;
				} else {
					add_cal	= (unsigned long long)(add_cal_n + pSgList->List[i - 1].length);
				}
				ApiMemBmDivideAddress(add_cal, &add_low_o, &add_high_o);
			}
			add_cal_n	= add_cal;
			//--------------------------------------------
			// Set the beginning of FIFO when block exceeds 4kByte boundary
			//--------------------------------------------
			if ((LengthStack >= 0x2000) && (OverHalf == 1)){
				Lengthdev	= LengthStack - 0x2000;
				add_cal_n	= add_cal_dev;
				Lengthrem	= 0x2000 - (LengthStack - pSgList->List[i].length);
			}
			//--------------------------------------------
			// Specify the beginning of the next Descriptor Format in NextDescPtr
			//--------------------------------------------
			desc_cal	= (unsigned long long)(BM_MEM_OUT_NXT_DESC + (0x40 * (i + ListMove + 1)));
			ApiMemBmDivideAddress(desc_cal, &desc_low_o_n, &desc_high_o_n);
			//--------------------------------------------
			// Set Descriptor Format to Prefetcher FIFO
			//--------------------------------------------
			_outmd((BM_MEM_OUT_Write_SGAddr + (0x40 * (i + ListMove + 1))), add_low_o);
			_outmd((BM_MEM_OUT_Write_SGAddr_high + (0x40 * (i + ListMove + 1))), add_high_o);
			_outmd((BM_MEM_OUT_Read_SGAddr + (0x40 * (i + ListMove + 1))), (((sg_dma_address(&pSgList->List[i])) & ~dwdata) + (BM_MEM_CRA_SELECT * (i + BM_MEM_AT_OFFSET))));
			_outmd((BM_MEM_OUT_Read_SGAddr_high + (0x40 * (i + ListMove + 1))), ((sg_dma_address(&pSgList->List[i])) >> 32 & (0xffffffff)));
			_outmd((BM_MEM_OUT_Length + (0x40 * (i + ListMove + 1))), pSgList->List[i].length);
			_outmd((BM_MEM_OUT_NextDescPtr + (0x40 * (i + ListMove + 1))), desc_low_o_n);
			_outmd((BM_MEM_OUT_NextDescPtr_high + (0x40 * (i + ListMove + 1))), desc_high_o_n);
			_outmw((BM_MEM_OUT_SequenceNum + (0x40 * (i + ListMove + 1))), (i + ListMove + 1));
			_outmd((BM_MEM_OUT_Stride + (0x40 * (i + ListMove + 1))), 0x0);
			_outmw((BM_MEM_OUT_Burst + (0x40 * (i + ListMove + 1))), 0x101);
			_outmd((BM_MEM_OUT_Descriptor + (0x40 * (i + ListMove + 1))), 0x40000000);
			//--------------------------------------------
			// Transfer setting of final transfer block
			//--------------------------------------------
			if ((LengthStack <= 0x2000) && (i == (ListSize - 1))){
				_outmd((BM_MEM_OUT_Descriptor + (0x40 * (i + ListMove + 1))), 0x40004000);
				if ((LengthStack == 0x2000) && (OverHalf == 1)){
					add_cal_res	= (unsigned long long)BM_MEM_OUT_WriteAdd;
				} else {
					add_cal_res	= (unsigned long long)(add_cal_n + pSgList->List[i].length);
				}
				desc_cal					= (unsigned long long)BM_MEM_OUT_TOP_DESC;
				ApiMemBmDivideAddress(desc_cal, &desc_low_o_n, &desc_high_o_n);
				_outmd((BM_MEM_OUT_NextDescPtr + (0x40 * (i + ListMove + 1))), desc_low_o_n);
				_outmd((BM_MEM_OUT_NextDescPtr_high + (0x40 * (i + ListMove + 1))), desc_high_o_n);
				//--------------------------------------------
				// Save various parameters for infinite transfer
				//--------------------------------------------
				if (pMasAddr->BmOut.SgList.dwIsRing == 1){
					if (pSgList->nr_pages > 150){
						add_cal								= (unsigned long long)(add_cal_n + pSgList->List[i].length);
						ApiMemBmDivideAddress(add_cal, &add_low_o, &add_high_o);
						pSgList->MemParam.FIFOOverAdd		= add_low_o;
						pSgList->MemParam.FIFOOverAdd_high	= add_high_o;
						pSgList->MemParam.SGOverAdd			= sg_dma_address(&pSgList->List[i + 1]);
						pSgList->MemParam.SGOverAdd_high	= ((sg_dma_address(&pSgList->List[i + 1])) >> 32 & (0xffffffff));
						pSgList->MemParam.OverLen			= pSgList->List[i + 1].length;
						pSgList->MemParam.ATNumber			= i + 1;
					} else {
						add_cal								= (unsigned long long)(add_cal_n + pSgList->List[i].length);
						ApiMemBmDivideAddress(add_cal, &add_low_o, &add_high_o);
						pSgList->MemParam.FIFOOverAdd		= add_low_o;
						pSgList->MemParam.FIFOOverAdd_high	= add_high_o;
						pSgList->MemParam.SGOverAdd			= sg_dma_address(&pSgList->List[0]);
						pSgList->MemParam.SGOverAdd_high	= ((sg_dma_address(&pSgList->List[0])) >> 32 & (0xffffffff));
						pSgList->MemParam.OverLen			= pSgList->List[0].length;
						pSgList->MemParam.ATNumber			= 0;
					}
					if (LengthStack != 0x2000){
						desc_cal	= (unsigned long long)BM_MEM_OUT_NXT_DESC;
						ApiMemBmDivideAddress(desc_cal, &desc_low_o_n, &desc_high_o_n);
						_outmd((BM_MEM_OUT_NextDescPtr + (0x40 * (i + ListMove + 1))), desc_low_o_n);
						_outmd((BM_MEM_OUT_NextDescPtr_high + (0x40 * (i + ListMove + 1))), desc_high_o_n);
					}
				}
				pMasAddr->BmOut.resetadd	= add_cal_res;
				//--------------------------------------------
				// When ListSize is 150 or more, specify the beginning of the next Descriptor Format in NextDescPtr
				//--------------------------------------------
				if ((LengthStack != 0x2000) &&(pSgList->nr_pages > 150)){
					_outmd((BM_MEM_OUT_Descriptor + (0x40 * (i + ListMove))), 0x40000000);
					//--------------------------------------------
					//Set NextDescPtr to the beginning of Prefetcher
					//--------------------------------------------
					desc_cal	= (unsigned long long)BM_MEM_OUT_NXT_DESC;
					ApiMemBmDivideAddress(desc_cal, &desc_low_o_n, &desc_high_o_n);
					_outmd((BM_MEM_OUT_NextDescPtr + (0x40 * (i + ListMove + 1))), desc_low_o_n);
					_outmd((BM_MEM_OUT_NextDescPtr_high + (0x40 * (i + ListMove + 1))), desc_high_o_n);
				}
			}
			//--------------------------------------------
			// Divide one block into 2k bytes
			//--------------------------------------------
			if ((LengthStack == 0x2000) && (i != (ListSize - 1))){
				//--------------------------------------------
				// Store the next Descriptor start address
				//--------------------------------------------
				desc_cal					= (unsigned long long)(BM_MEM_OUT_NXT_DESC + (0x40 * (i + ListMove)));
				ApiMemBmDivideAddress(desc_cal, &desc_low_o_n, &desc_high_o_n);
				pSgList->ChangeDescAdd[n]	= desc_low_o_n;
				if (DummySet == 1){
					DummySet	= 0;
				} else {
					//--------------------------------------------
					// Set FIFO start address to NextDescPtr
					//--------------------------------------------
					desc_cal	= (unsigned long long)BM_MEM_OUT_TOP_DESC;
					ApiMemBmDivideAddress(desc_cal, &desc_low_o_n, &desc_high_o_n);
					_outmd((BM_MEM_OUT_NextDescPtr + (0x40 * (i + ListMove + 1))), desc_low_o_n);
					_outmd((BM_MEM_OUT_NextDescPtr_high + (0x40 * (i + ListMove + 1))), desc_high_o_n);
				}
				LengthStack					= Lengthdev;
				ListNum						= 0;
				n++;
				if (OverHalf == 0){
					OverHalf	= 1;
				} else {
					OverHalf	= 0;
				}
			} else if (LengthStack > 0x2000){
				//--------------------------------------------
				// Split transfer number of 1 block into 2k bytes
				//--------------------------------------------
				Lengthdev					= LengthStack - 0x2000;
				Lengthrem					= 0x2000 - (LengthStack - pSgList->List[i].length);
				_outmd((BM_MEM_OUT_Length + (0x40 * (i + ListMove + 1))), Lengthrem);
				//--------------------------------------------
				// Store the next Descriptor start address
				//--------------------------------------------
				desc_cal					= (unsigned long long)(BM_MEM_OUT_NXT_DESC + (0x40 * (i + ListMove)));
				ApiMemBmDivideAddress(desc_cal, &desc_low_o_n, &desc_high_o_n);
				pSgList->ChangeDescAdd[n]	= desc_low_o_n;
				if (DummySet == 1){
					DummySet	= 0;
				} else {
					desc_cal	= (unsigned long long)BM_MEM_OUT_TOP_DESC;
					ApiMemBmDivideAddress(desc_cal, &desc_low_o_n, &desc_high_o_n);
					_outmd((BM_MEM_OUT_NextDescPtr + (0x40 * (i + ListMove + 1))), desc_low_o_n);
					_outmd((BM_MEM_OUT_NextDescPtr_high + (0x40 * (i + ListMove + 1))), desc_high_o_n);
				}
				//--------------------------------------------
				// Specify the beginning of the next Descriptor Format in NextDescPtr
				//--------------------------------------------
				ListMove++;
				desc_cal					= (unsigned long long)(BM_MEM_OUT_NXT_DESC + (0x40 * (i + ListMove + 1)));
				ApiMemBmDivideAddress(desc_cal, &desc_low_o_n, &desc_high_o_n);
				//--------------------------------------------
				// Set Descriptor Format to Prefetcher FIFO
				//--------------------------------------------
				_outmd((BM_MEM_OUT_Write_SGAddr + (0x40 * (i + ListMove + 1))), add_low_o_dev);
				_outmd((BM_MEM_OUT_Write_SGAddr_high + (0x40 * (i + ListMove + 1))), add_high_o_dev);
				_outmd((BM_MEM_OUT_Read_SGAddr + (0x40 * (i + ListMove + 1))), (((sg_dma_address(&pSgList->List[i]) + (pSgList->List[i].length - Lengthdev)) & ~dwdata) + (BM_MEM_CRA_SELECT * (i + BM_MEM_AT_OFFSET))));
				_outmd((BM_MEM_OUT_Read_SGAddr_high + (0x40 * (i + ListMove + 1))), ((sg_dma_address(&pSgList->List[i])) >> 32 & (0xffffffff)));
				_outmd((BM_MEM_OUT_Length + (0x40 * (i + ListMove + 1))), Lengthdev);
				_outmd((BM_MEM_OUT_NextDescPtr + (0x40 * (i + ListMove + 1))), desc_low_o_n);
				_outmd((BM_MEM_OUT_NextDescPtr_high + (0x40 * (i + ListMove + 1))), desc_high_o_n);
				_outmw((BM_MEM_OUT_SequenceNum + (0x40 * (i + ListMove + 1))), (i + ListMove +1));
				_outmd((BM_MEM_OUT_Stride + (0x40 * (i + ListMove + 1))), 0x0);
				_outmw((BM_MEM_OUT_Burst + (0x40 * (i + ListMove + 1))), 0x101);
				_outmd((BM_MEM_OUT_Descriptor + (0x40 * (i + ListMove + 1))), 0x40000000);
				//--------------------------------------------
				// If the FIFO address of the next block exceeds 4 kBytes, set the FIFO start address
				//--------------------------------------------
				if (OverHalf == 0){
					//--------------------------------------------
					// Set FIFO address for next list
					//--------------------------------------------
					add_cal		= (unsigned long long)(add_cal_n + Lengthrem);
					ApiMemBmDivideAddress(add_cal, &add_low_o, &add_high_o);
					_outmd((BM_MEM_OUT_Write_SGAddr + (0x40 * (i + ListMove + 1))), add_low_o);
					_outmd((BM_MEM_OUT_Write_SGAddr_high + (0x40 * (i + ListMove + 1))), add_high_o);
					SetDesc		= 0;
					OverHalf	= 1;
				} else if (OverHalf == 1){
					//--------------------------------------------
					// Move FIFO address for next list
					//--------------------------------------------
					add_cal		= (unsigned long long)(add_cal_dev + Lengthdev);
					SetDesc		= 1;
					OverHalf	= 0;
				}
				//--------------------------------------------
				// Transfer setting of final transfer block
				//--------------------------------------------
				if (i == (ListSize - 1)){
					//--------------------------------------------
					// Set NextDescPtr to the beginning of Prefetcher FIFO
					//--------------------------------------------
					_outmd((BM_MEM_OUT_Descriptor + (0x40 * (i + ListMove + 1))), 0x40004000);
					desc_cal					= (unsigned long long)BM_MEM_OUT_TOP_DESC;
					ApiMemBmDivideAddress(desc_cal, &desc_low_o_n, &desc_high_o_n);
					_outmd((BM_MEM_OUT_NextDescPtr + (0x40 * (i + ListMove + 1))), desc_low_o_n);
					_outmd((BM_MEM_OUT_NextDescPtr_high + (0x40 * (i + ListMove + 1))), desc_high_o_n);
					add_cal_res					= (unsigned long long)(add_cal_n + pSgList->List[i].length);
					pMasAddr->BmOut.resetadd	= add_cal_res;
					//--------------------------------------------
					// Address setting at SgList overwrite
					//--------------------------------------------
					if (pSgList->nr_pages > 150){
						//--------------------------------------------
						// Set previous list to final list
						//--------------------------------------------
						_outmd((BM_MEM_OUT_NextDescPtr + (0x40 * (i + ListMove))), desc_low_o_n);
						_outmd((BM_MEM_OUT_NextDescPtr_high + (0x40 * (i + ListMove))), desc_high_o_n);
						//--------------------------------------------
						//Set NextDescPtr to the beginning of Prefetcher
						//--------------------------------------------
						desc_cal	= (unsigned long long)BM_MEM_OUT_NXT_DESC;
						ApiMemBmDivideAddress(desc_cal, &desc_low_o_n, &desc_high_o_n);
						_outmd((BM_MEM_OUT_NextDescPtr + (0x40 * (i + ListMove + 1))), desc_low_o_n);
						_outmd((BM_MEM_OUT_NextDescPtr_high + (0x40 * (i + ListMove + 1))), desc_high_o_n);
					}
					//--------------------------------------------
					// Save various parameters for infinite transfer
					//--------------------------------------------
					if (pMasAddr->BmOut.SgList.dwIsRing == 1){
						if (pSgList->nr_pages > 150){
							if (OverHalf == 0){
								ApiMemBmDivideAddress(add_cal, &add_low_o, &add_high_o);
								pSgList->MemParam.FIFOOverAdd		= add_low_o;
								pSgList->MemParam.FIFOOverAdd_high	= add_high_o;
							} else {
								pSgList->MemParam.FIFOOverAdd		= add_low_o;
								pSgList->MemParam.FIFOOverAdd_high	= add_high_o;
							}
							pSgList->MemParam.SGOverAdd			= sg_dma_address(&pSgList->List[i + 1]);
							pSgList->MemParam.SGOverAdd_high	= ((sg_dma_address(&pSgList->List[i + 1])) >> 32 & (0xffffffff));
							pSgList->MemParam.OverLen			= pSgList->List[i + 1].length;
							pSgList->MemParam.ATNumber			= i + 1;
						} else {
							if (OverHalf == 0){
								ApiMemBmDivideAddress(add_cal, &add_low_o, &add_high_o);
								pSgList->MemParam.FIFOOverAdd		= add_low_o;
								pSgList->MemParam.FIFOOverAdd_high	= add_high_o;
							} else {
								pSgList->MemParam.FIFOOverAdd		= add_low_o;
								pSgList->MemParam.FIFOOverAdd_high	= add_high_o;
							}
							pSgList->MemParam.SGOverAdd			= sg_dma_address(&pSgList->List[0]);
							pSgList->MemParam.SGOverAdd_high	= ((sg_dma_address(&pSgList->List[0])) >> 32 & (0xffffffff));
							pSgList->MemParam.OverLen			= pSgList->List[0].length;
							pSgList->MemParam.ATNumber			= 0;
						}
						desc_cal	= (unsigned long long)BM_MEM_OUT_NXT_DESC;
						ApiMemBmDivideAddress(desc_cal, &desc_low_o_n, &desc_high_o_n);
						_outmd((BM_MEM_OUT_NextDescPtr + (0x40 * (i + ListMove + 1))), desc_low_o_n);
						_outmd((BM_MEM_OUT_NextDescPtr_high + (0x40 * (i + ListMove + 1))), desc_high_o_n);
					}
				}
				LengthStack					= Lengthdev;
				ListNum		= 1;
				n++;
			}	
		}
		Outlen	+= pSgList->List[i].length;
	}
	//============================================
	// Set the descriptor for end discrimination at the beginning of the Descriptor List
	//============================================
	if(dwDir == BM_DIR_IN){
		//--------------------------------------------
		// Specify the beginning of prefetcher FIFO to NextDescPtr
		//--------------------------------------------
		desc_cal						= (unsigned long long)BM_MEM_INP_NXT_DESC;
		ApiMemBmDivideAddress(desc_cal, &desc_low_i_n, &desc_high_i_n);
		//--------------------------------------------
		// Set Descriptor Format to Prefetcher FIFO
		//--------------------------------------------
		_outmd((BM_MEM_INP_Read_SGAddr), 0x0);
		_outmd((BM_MEM_INP_Read_SGAddr_high), 0x0);
		_outmd((BM_MEM_INP_Write_SGAddr), 0x0);
		_outmd((BM_MEM_INP_Write_SGAddr_high), 0x0);
		_outmd((BM_MEM_INP_Length), 0x0);
		_outmd((BM_MEM_INP_NextDescPtr), desc_low_i_n);
		_outmd((BM_MEM_INP_NextDescPtr_high), desc_high_i_n);
		_outmw((BM_MEM_INP_SequenceNum), 0x0);
		_outmd((BM_MEM_INP_Stride), 0x0);
		_outmw((BM_MEM_INP_Burst), 0x101);
		_outmd((BM_MEM_INP_Descriptor), 0x0);
		pMasAddr->BmInp.resetchangedesc	= 0;
		pMasAddr->BmInp.atstartlen		= LengthStack;
		pMasAddr->BmInp.resetOverHalf	= OverHalf;
		if (pSgList->nr_pages > 150){
			pMasAddr->BmInp.resetListSize	= pSgList->nr_pages;
		} else {
			//--------------------------------------------
			// Stores the start address of the final Descriptor
			//--------------------------------------------
			if (UnderLen == 1){
				desc_cal					= (unsigned long long)(BM_MEM_INP_NXT_DESC + (0x40 * (i + ListMove - 2)));
				ApiMemBmDivideAddress(desc_cal, &desc_low_i_n, &desc_high_i_n);
				pSgList->ChangeDescAdd[n]	= desc_low_i_n;
				add_cal						= (unsigned long long)(add_cal_n + pSgList->List[ListSize - 1].length);
				ApiMemBmDivideAddress(add_cal, &add_low_i, &add_high_i);
			}
		}
		//--------------------------------------------
		// Stores the list No. when resetting SG list in the case of transferring infinite times
		//--------------------------------------------
		if ((pMasAddr->BmInp.SgList.dwIsRing == 1) && (pMasAddr->BmInp.sglen < 0x800)){
			pSgList->ListNumCnt	= 0;
		} else {
			pSgList->ListNumCnt	= i;
		}
	}else {
		//--------------------------------------------
		// Specify the beginning of prefetcher FIFO to NextDescPtr
		//--------------------------------------------
		desc_cal						= (unsigned long long)BM_MEM_OUT_NXT_DESC;
		ApiMemBmDivideAddress(desc_cal, &desc_low_o_n, &desc_high_o_n);
		//--------------------------------------------
		// Set Descriptor Format to Prefetcher FIFO
		//--------------------------------------------
		_outmd((BM_MEM_OUT_Write_SGAddr), 0x0);
		_outmd((BM_MEM_OUT_Write_SGAddr_high), 0x0);
		_outmd((BM_MEM_OUT_Read_SGAddr), 0x0);
		_outmd((BM_MEM_OUT_Read_SGAddr_high), 0x0);
		_outmd((BM_MEM_OUT_Length), 0x0);
		_outmd((BM_MEM_OUT_NextDescPtr), desc_low_o_n);
		_outmd((BM_MEM_OUT_NextDescPtr_high), desc_high_o_n);
		_outmw((BM_MEM_OUT_SequenceNum), 0x0);
		_outmd((BM_MEM_OUT_Stride), 0x0);
		_outmw((BM_MEM_OUT_Burst), 0x101);
		_outmd((BM_MEM_OUT_Descriptor), 0x0);
		pMasAddr->BmOut.resetchangedesc	= 0;
		//--------------------------------------------
		// Stores the start address of the final Descriptor
		//--------------------------------------------
		if (LengthStack < 0x2000){
			desc_cal						= (unsigned long long)(BM_MEM_OUT_NXT_DESC + (0x40 * (i + ListMove - 2)));
			ApiMemBmDivideAddress(desc_cal, &desc_low_o_n, &desc_high_o_n);
			pSgList->ChangeDescAdd[n]		= desc_low_o_n;
			add_cal							= (unsigned long long)(add_cal_n + pSgList->List[ListSize - 1].length);
			ApiMemBmDivideAddress(add_cal, &add_low_o, &add_high_o);
		}
		pMasAddr->BmOut.atstartlen		= LengthStack;
		pMasAddr->BmOut.resetOverHalf	= OverHalf;
		if (pSgList->nr_pages > 150){
			pMasAddr->BmOut.resetListSize	= pSgList->nr_pages;
		}
		pSgList->ListNumCnt				= i;
	}
	pSgList->ATOverCnt	= 0;
	pSgList->ReSetArray	= n;
	
	return	BM_ERROR_SUCCESS;
}

//========================================================================
// Function name  : MemBmSetSGAddressRePlace
// Function		  : If the number of stops is less than the buffer size, reconfigure the S/G list in hardware.
// I/F	 	  	  : Internal
// In	 		  : pMasAddr			: Point to the MASTERADDR structure
//		   		 	dwDir				: BM_DIR_IN / BM_DIR_OUT
//		   		 	loop				: S/G offset
//		   		 	stopnum				: The number of stops
// Out			  : None
// Return value   : Normal completed	: BM_ERROR_SUCCESS
//		   			Abnormal completed	: BM_ERROR_PARAM / BM_ERROR_BUFF
// Addition  	  : 
//========================================================================
long MemBmSetSGAddressRePlace(PMASTERADDR pMasAddr, unsigned long dwDir, unsigned long loop, unsigned long stopnum)
{
	PSGLIST 			pSgList;
	unsigned long long	desc_cal;
	unsigned long		desc_low_i_n, desc_high_i_n;

	//--------------------------------------------
	// Checking parameter
	//--------------------------------------------
	if(dwDir != BM_DIR_IN){
		return	BM_ERROR_PARAM;
	}
	if(dwDir == BM_DIR_IN){
		pSgList	= &pMasAddr->BmInp.SgList;
	}
	if(pSgList->nr_pages > BM_MEM_MAX_SG_SIZE){
		return	BM_ERROR_BUFF;
	}

	
	//--------------------------------------------
	// Set in master register
	//--------------------------------------------
	if(dwDir == BM_DIR_IN){	
		//--------------------------------------------
		// Specify the beginning of prefetcher FIFO to NextDescPtr
		//--------------------------------------------
		desc_cal	= (unsigned long long)BM_MEM_INP_TOP_DESC;
		ApiMemBmDivideAddress(desc_cal, &desc_low_i_n, &desc_high_i_n);
		_outmd((BM_MEM_INP_Length + (0x40 * (loop + 1))), stopnum);
		_outmd((BM_MEM_INP_NextDescPtr + (0x40 * (loop + 1))), desc_low_i_n);
		_outmd((BM_MEM_INP_NextDescPtr_high + (0x40 * (loop + 1))), desc_high_i_n);
		_outmd((BM_MEM_INP_Descriptor + (0x40 * (loop + 1))), 0x40004000);
	}
	return	BM_ERROR_SUCCESS;
}

//========================================================================
// Function name  : MemBmReSetSGAddress
// Function		  : If the number of S/G lists is equal to or more than the specific number, reconfigure the S/G list in hardware.
// I/F	 	  	  : Internal
// In	 		  : pMasAddr			: Point to the MASTERADDR structure
//		   		 	dwDir				: BM_DIR_IN / BM_DIR_OUT
// Out			  : None
// Return value   : Normal completed	: BM_ERROR_SUCCESS
//		   			Abnormal completed	: BM_ERROR_PARAM / BM_ERROR_BUFF
// Addition  	  : 
//========================================================================
long MemBmReSetSGAddress(PMASTERADDR pMasAddr, unsigned long dwDir)
{
	unsigned long		i;
	PSGLIST 			pSgList;
	unsigned long long	add_cal, add_cal_n;
	unsigned long long	add_cal_res;
	unsigned long long	desc_cal;
	unsigned long long	add_cal_dev;
	unsigned long		dwdata;
	unsigned long		desc_low_o_n, desc_high_o_n;
	unsigned long		desc_low_i_n, desc_high_i_n;
	unsigned long		cradata, cradata2;
	unsigned long		add_low_i = 0, add_high_i = 0;
	unsigned long		add_low_o = 0, add_high_o = 0;
	unsigned long		add_low_i_dev, add_high_i_dev;
	unsigned long		add_low_o_dev, add_high_o_dev;
	unsigned long		ListSize, ListNum;
	unsigned long		LengthStack, Lengthdev, Lengthrem;
	unsigned long		j, l, n;
	unsigned long		ListMove, OverHalf;
	unsigned long		ListStart, ListEnd;
	unsigned long		addtable, selectlen;
	int					selecttable, RePreflag, ReSetFlag, ListPreFlag;
	int					SetDesc, DummySet;

	//--------------------------------------------
	// Checking parameter
	//--------------------------------------------
	if(dwDir != BM_DIR_IN && dwDir != BM_DIR_OUT){
		return	BM_ERROR_PARAM;
	}
	if(dwDir == BM_DIR_IN){
		pSgList	= &pMasAddr->BmInp.SgList;
	}else{
		pSgList	= &pMasAddr->BmOut.SgList;
	}
	if(pSgList->nr_pages > BM_MEM_MAX_SG_SIZE){
		return	BM_ERROR_BUFF;
	}

	//---------------------------------------------
	// Setting FIFO address
	//---------------------------------------------
	if(dwDir & BM_DIR_IN){
		add_cal		= (unsigned long long)(pMasAddr->BmInp.resetadd);
		ApiMemBmDivideAddress(add_cal, &add_low_i, &add_high_i);
		ListSize	= pMasAddr->BmInp.addtablenum;
		addtable	= pMasAddr->BmInp.addtablenum;
	} else {
		add_cal		= (unsigned long long)(pMasAddr->BmOut.resetadd);
		ApiMemBmDivideAddress(add_cal, &add_low_o, &add_high_o);
		ListSize	= pMasAddr->BmOut.addtablenum;
		addtable	= pMasAddr->BmOut.addtablenum;
	}
	add_cal_n	= add_cal;
	ListSize	= pSgList->nr_pages;
	RePreflag	= 0;
	//--------------------------------------------
	// If the number of SGList exceeds a certain number, limit the number of lists for reconfiguration
	//--------------------------------------------
	if(dwDir & BM_DIR_IN){
		add_cal_dev	= (unsigned long long)BM_MEM_INP_ReadAdd;
		ApiMemBmDivideAddress(add_cal_dev, &add_low_i_dev, &add_high_i_dev);
		if (pSgList->ATOverCnt != (pSgList->ListNumCnt / 150)){
			pMasAddr->BmInp.atstartnum		= 0;
			pMasAddr->BmInp.resetListMove	= 0;
			if (pSgList->ATOverCnt != 0){
				pSgList->ReSetArray	= pMasAddr->BmInp.resetchangedesc;
			}
			pMasAddr->BmInp.resetchangedesc	= 0;
			pSgList->ATOverCnt				= (pSgList->ListNumCnt / 150);
			ReSetFlag						= 1;
		} else {
			ReSetFlag						= 0;
		}
		selectlen	= 0;
		l			= pSgList->ListNumCnt;
		if (l != (pMasAddr->BmInp.resetListSize - 1)){
			while (selectlen <= 0x2000){
				selectlen	+= pSgList->List[l].length;
				l++;
				if (l == pMasAddr->BmInp.resetListSize){
					break;
				}
			}
			if (l == pMasAddr->BmInp.resetListSize){
				if ((l - pSgList->ListNumCnt) <= 2){
					ListNum	= l - pSgList->ListNumCnt;
				} else {
					ListNum	= l - pSgList->ListNumCnt - 1;
				}
			} else {
				ListNum	= l - pSgList->ListNumCnt - 1;
			}
			ListPreFlag	= 0;
		} else {
			ListNum		= 1;
			ListPreFlag	= 1;
		}
		if ((ReSetFlag == 0) && (ListPreFlag == 0)){
			pMasAddr->BmInp.atstartnum	+= ListNum;
		} else if ((ReSetFlag == 0) && (ListPreFlag == 1)){
			pMasAddr->BmInp.atstartnum	+= (ListNum + 1);
		}
		if ((pMasAddr->BmInp.atstartnum + ListNum) >= 150){
			RePreflag					= 1;
		}
		if (pMasAddr->BmInp.addtablenum < ListNum){
			pMasAddr->BmInp.addtablenum	= pMasAddr->BmInp.resetListSize - pSgList->ListNumCnt - addtable;
			ListEnd						= pSgList->ListNumCnt + addtable;
		} else {
			pMasAddr->BmInp.addtablenum	= pMasAddr->BmInp.resetListSize - pSgList->ListNumCnt - ListNum;
			ListEnd						= pSgList->ListNumCnt + ListNum;
		}
		j			= pMasAddr->BmInp.atstartnum;
		ListStart	= pSgList->ListNumCnt;
		LengthStack	= pMasAddr->BmInp.atstartlen;
		OverHalf	= pMasAddr->BmInp.resetOverHalf;
		ListMove	= pMasAddr->BmInp.resetListMove;
		n			= pMasAddr->BmInp.resetchangedesc;
	} else {
		add_cal_dev	= (unsigned long long)BM_MEM_OUT_WriteAdd;
		ApiMemBmDivideAddress(add_cal_dev, &add_low_o_dev, &add_high_o_dev);
		if (pSgList->ATOverCnt != (pSgList->ListNumCnt / 150)){
			pMasAddr->BmOut.atstartnum		= 0;
			pMasAddr->BmOut.resetListMove	= 0;
			if (pSgList->ATOverCnt != 0){
				pSgList->ReSetArray	= pMasAddr->BmOut.resetchangedesc;
			}
			pMasAddr->BmOut.resetchangedesc	= 0;
			pSgList->ATOverCnt				= (pSgList->ListNumCnt / 150);
			ReSetFlag						= 1;
		} else {
			ReSetFlag						= 0;
		}
		selectlen	= 0;
		l			= pSgList->ListNumCnt;
		if (l != (pMasAddr->BmOut.resetListSize - 1)){
			while (selectlen <= 0x2000){
				selectlen	+= pSgList->List[l].length;
				l++;
				if (l == pMasAddr->BmOut.resetListSize){
					break;
				}
			}
			if (l == pMasAddr->BmOut.resetListSize){
				if ((l - pSgList->ListNumCnt) <= 2){
					ListNum	= l - pSgList->ListNumCnt;
				} else {
					ListNum	= l - pSgList->ListNumCnt - 1;
				}
			} else {
				ListNum		= l - pSgList->ListNumCnt - 1;
			}
			ListPreFlag	= 0;
		} else {
			ListNum		= 1;
			ListPreFlag	= 1;
		}
		if ((ReSetFlag == 0) && (ListPreFlag == 0)){
			pMasAddr->BmOut.atstartnum	+= ListNum;
		} else if ((ReSetFlag == 0) && (ListPreFlag == 1)){
			pMasAddr->BmOut.atstartnum	+= (ListNum + 1);
		}
		if ((pMasAddr->BmOut.atstartnum + ListNum) >= 150){
			RePreflag	= 1;
		}
		if (pMasAddr->BmOut.addtablenum < ListNum){
			pMasAddr->BmOut.addtablenum	= pMasAddr->BmOut.resetListSize - pSgList->ListNumCnt - addtable;
			ListEnd						= pSgList->ListNumCnt + addtable;
		} else {
			pMasAddr->BmOut.addtablenum	= pMasAddr->BmOut.resetListSize - pSgList->ListNumCnt - ListNum;
			ListEnd						= pSgList->ListNumCnt + ListNum;
		}
		j			= pMasAddr->BmOut.atstartnum;
		ListStart	= pSgList->ListNumCnt;
		LengthStack	= pMasAddr->BmOut.atstartlen;
		OverHalf	= pMasAddr->BmOut.resetOverHalf;
		ListMove	= pMasAddr->BmOut.resetListMove;
		n			= pMasAddr->BmOut.resetchangedesc;
	}
	Lengthdev	= 0;
	Lengthrem	= 0;
	DummySet	= 1;
	selecttable	= 0;
	SetDesc		= 0;
	//--------------------------------------------
	// Loop the number of times
	//--------------------------------------------
	for (i = ListStart; i < ListEnd; i++) {
		//--------------------------------------------
		// Set in master register
		//--------------------------------------------
		if(dwDir == BM_DIR_IN){
			//--------------------------------------------
			// Set the Address Translation Table
			//--------------------------------------------
			if (pMasAddr->add64flag == 1){
				_outmd((BM_MEM_CRA + (0x8 * j)), 0xFFFFFFFD);
				dwdata		= _inmd(BM_MEM_CRA + (0x8 * j));
				_outmd((BM_MEM_CRA + (0x8 * j)), ((dwdata & sg_dma_address(&pSgList->List[i])) | 0x1));
				_outmd((BM_MEM_CRA_HIGH + (0x8 * j)), (sg_dma_address(&pSgList->List[i]) >> 32 & 0xffffffff));
				cradata		= _inmd(BM_MEM_CRA + (0x8 * j));
				cradata2	= _inmd(BM_MEM_CRA_HIGH + (0x8 * j));
			} else {
				_outmd((BM_MEM_CRA + (0x8 * j)), 0xFFFFFFFC);
				dwdata		= _inmd(BM_MEM_CRA + (0x8 * j));
				_outmd((BM_MEM_CRA + (0x8 * j)), (dwdata & sg_dma_address(&pSgList->List[i])));
				_outmd((BM_MEM_CRA_HIGH + (0x8 * j)), (sg_dma_address(&pSgList->List[i]) >> 32 & 0xffffffff));
				cradata		= _inmd(BM_MEM_CRA + (0x8 * j));
				cradata2	= _inmd(BM_MEM_CRA_HIGH + (0x8 * j));
			}
			LengthStack	+= pSgList->List[i].length;
			//--------------------------------------------
			// Move FIFO address for second and subsequent lists
			//--------------------------------------------
			if (selecttable == 1){
				if (SetDesc == 1){
					SetDesc	= 0;
				} else {
					add_cal	= (unsigned long long)(add_cal_n + pSgList->List[i - 1].length);
				}
				ApiMemBmDivideAddress(add_cal, &add_low_i, &add_high_i);
			}
			add_cal_n	= add_cal;
			//--------------------------------------------
			// Set the beginning of FIFO when block exceeds 4kByte boundary
			//--------------------------------------------
			if ((LengthStack >= 0x2000) && (OverHalf == 1)){
				Lengthdev	= LengthStack - 0x2000;
				add_cal_n	= add_cal_dev;
				Lengthrem	= 0x2000 - (LengthStack - pSgList->List[i].length);
			}
			//--------------------------------------------
			// Specify the beginning of the next Descriptor Format in NextDescPtr
			//--------------------------------------------
			desc_cal	= (unsigned long long)(BM_MEM_INP_NXT_DESC + (0x40 * (j + ListMove + 1)));
			ApiMemBmDivideAddress(desc_cal, &desc_low_i_n, &desc_high_i_n);
			//--------------------------------------------
			// Set Descriptor Format to Prefetcher FIFO
			//--------------------------------------------
			_outmd((BM_MEM_INP_Read_SGAddr + (0x40 * (j + ListMove + 1))), add_low_i);
			_outmd((BM_MEM_INP_Read_SGAddr_high + (0x40 * (j + ListMove + 1))), add_high_i);
			_outmd((BM_MEM_INP_Write_SGAddr + (0x40 * (j + ListMove + 1))), ((sg_dma_address(&pSgList->List[i]) & ~dwdata) + (BM_MEM_CRA_SELECT * j)));
			_outmd((BM_MEM_INP_Write_SGAddr_high + (0x40 * (j + ListMove + 1))), (sg_dma_address(&pSgList->List[i]) >> 32 & 0xffffffff));
			_outmd((BM_MEM_INP_Length + (0x40 * (j + ListMove + 1))), pSgList->List[i].length);
			_outmd((BM_MEM_INP_NextDescPtr + (0x40 * (j + ListMove + 1))), desc_low_i_n);
			_outmd((BM_MEM_INP_NextDescPtr_high + (0x40 * (j + ListMove + 1))), desc_high_i_n);
			_outmw((BM_MEM_INP_SequenceNum + (0x40 * (j + ListMove + 1))), (i + ListMove + 1));
			_outmd((BM_MEM_INP_Stride + (0x40 * (j + ListMove + 1))), 0x0);
			_outmw((BM_MEM_INP_Burst + (0x40 * (j + ListMove + 1))), 0x101);
			_outmd((BM_MEM_INP_Descriptor + (0x40 * (j + ListMove + 1))), 0x40000000);
			//--------------------------------------------
			// Transfer setting of final transfer block
			//--------------------------------------------
			if ((LengthStack <= 0x2000) && (i == (ListEnd - 1))){
				if ((LengthStack == 0x2000) && (OverHalf == 1)){
					add_cal_res		= (unsigned long long)BM_MEM_INP_ReadAdd;
				} else {
					add_cal_res		= (unsigned long long)(add_cal_n + pSgList->List[i].length);
				}
				pMasAddr->BmInp.resetadd	= add_cal_res;
				//--------------------------------------------
				//Set NextDescPtr to the beginning of Prefetcher
				//--------------------------------------------
				if (i == (pSgList->nr_pages - 1)) {
					desc_cal		= (unsigned long long)BM_MEM_INP_TOP_DESC;
					ApiMemBmDivideAddress(desc_cal, &desc_low_i_n, &desc_high_i_n);
					_outmd((BM_MEM_INP_NextDescPtr + (0x40 * (j + ListMove + 1))), desc_low_i_n);
					_outmd((BM_MEM_INP_NextDescPtr_high + (0x40 * (j + ListMove + 1))), desc_high_i_n);
					_outmd((BM_MEM_INP_Descriptor + (0x40 * (j + ListMove + 1))), 0x40004000);
				} else {
					if (RePreflag == 1){
						desc_cal	= (unsigned long long)BM_MEM_INP_NXT_DESC;
						ApiMemBmDivideAddress(desc_cal, &desc_low_i_n, &desc_high_i_n);
						_outmd((BM_MEM_INP_NextDescPtr + (0x40 * (j + ListMove + 1))), desc_low_i_n);
						_outmd((BM_MEM_INP_NextDescPtr_high + (0x40 * (j + ListMove + 1))), desc_high_i_n);
					}
				}
			}
			//--------------------------------------------
			// Divide one block into 2k bytes
			//--------------------------------------------
			if ((LengthStack == 0x2000) && (i != (ListEnd - 1))){
				//--------------------------------------------
				// Store the next Descriptor start address
				//--------------------------------------------
				desc_cal					= (unsigned long long)(BM_MEM_INP_NXT_DESC + (0x40 * (j + ListMove)));
				ApiMemBmDivideAddress(desc_cal, &desc_low_i_n, &desc_high_i_n);
				pSgList->ChangeDescAdd[n]	= desc_low_i_n;
				n++;
				if (DummySet == 1){
					DummySet						= 0;
				} else {
					//--------------------------------------------
					// Set FIFO start address to NextDescPtr
					//--------------------------------------------
					desc_cal						= (unsigned long long)BM_MEM_INP_TOP_DESC;
					ApiMemBmDivideAddress(desc_cal, &desc_low_i_n, &desc_high_i_n);
					_outmd((BM_MEM_INP_NextDescPtr + (0x40 * (j + ListMove + 1))), desc_low_i_n);
					_outmd((BM_MEM_INP_NextDescPtr_high + (0x40 * (j + ListMove + 1))), desc_high_i_n);
				}
				LengthStack		= Lengthdev;
				if (OverHalf == 0){
					OverHalf	= 1;
				} else {
					OverHalf	= 0;
				}
			} else if (LengthStack > 0x2000){
				//--------------------------------------------
				// Split transfer number of 1 block into 2k bytes
				//--------------------------------------------
				Lengthdev					= LengthStack - 0x2000;
				Lengthrem					= 0x2000 - (LengthStack - pSgList->List[i].length);
				_outmd((BM_MEM_INP_Length + (0x40 * (j + ListMove + 1))), Lengthrem);
				//--------------------------------------------
				// Store the next Descriptor start address
				//--------------------------------------------
				desc_cal					= (unsigned long long)(BM_MEM_INP_NXT_DESC + (0x40 * (j + ListMove)));
				ApiMemBmDivideAddress(desc_cal, &desc_low_i_n, &desc_high_i_n);
				pSgList->ChangeDescAdd[n]	= desc_low_i_n;
				n++;
				//--------------------------------------------
				// Specify the beginning of the next Descriptor Format in NextDescPtr
				//--------------------------------------------
				desc_cal					= (unsigned long long)BM_MEM_INP_TOP_DESC;
				ApiMemBmDivideAddress(desc_cal, &desc_low_i_n, &desc_high_i_n);
				_outmd((BM_MEM_INP_NextDescPtr + (0x40 * (j + ListMove + 1))), desc_low_i_n);
				_outmd((BM_MEM_INP_NextDescPtr_high + (0x40 * (j + ListMove + 1))), desc_high_i_n);
				if (DummySet == 1){	
					DummySet						= 0;
				}
				//--------------------------------------------
				// Specify the beginning of the next Descriptor Format in NextDescPtr
				//--------------------------------------------
				ListMove++;
				desc_cal					= (unsigned long long)(BM_MEM_INP_NXT_DESC + (0x40 * (j + ListMove + 1)));
				ApiMemBmDivideAddress(desc_cal, &desc_low_i_n, &desc_high_i_n);
				//--------------------------------------------
				// Set Descriptor Format to Prefetcher FIFO
				//--------------------------------------------
				_outmd((BM_MEM_INP_Read_SGAddr + (0x40 * (j + ListMove + 1))), add_low_i_dev);
				_outmd((BM_MEM_INP_Read_SGAddr_high + (0x40 * (j + ListMove + 1))), add_high_i_dev);
				_outmd((BM_MEM_INP_Write_SGAddr + (0x40 * (j + ListMove + 1))), (((sg_dma_address(&pSgList->List[i]) + (pSgList->List[i].length - Lengthdev)) & ~dwdata) + (BM_MEM_CRA_SELECT * j)));
				_outmd((BM_MEM_INP_Write_SGAddr_high + (0x40 * (j + ListMove + 1))), (sg_dma_address(&pSgList->List[i]) >> 32 & 0xffffffff));
				_outmd((BM_MEM_INP_Length + (0x40 * (j + ListMove + 1))), Lengthdev);
				_outmd((BM_MEM_INP_NextDescPtr + (0x40 * (j + ListMove + 1))), desc_low_i_n);
				_outmd((BM_MEM_INP_NextDescPtr_high + (0x40 * (j + ListMove + 1))), desc_high_i_n);
				_outmw((BM_MEM_INP_SequenceNum + (0x40 * (j + ListMove + 1))), (i + ListMove +1));
				_outmd((BM_MEM_INP_Stride + (0x40 * (j + ListMove + 1))), 0x0);
				_outmw((BM_MEM_INP_Burst + (0x40 * (j + ListMove + 1))), 0x101);
				_outmd((BM_MEM_INP_Descriptor + (0x40 * (j + ListMove + 1))), 0x40000000);
				//--------------------------------------------
				// If the FIFO address of the next block exceeds 4 kBytes, set the FIFO start address
				//--------------------------------------------
				if (OverHalf == 0){
					//--------------------------------------------
					// Set FIFO address for next list
					//--------------------------------------------
					add_cal		= (unsigned long long)(add_cal_n + Lengthrem);
					ApiMemBmDivideAddress(add_cal, &add_low_i, &add_high_i);
					_outmd((BM_MEM_INP_Read_SGAddr + (0x40 * (j + ListMove + 1))), add_low_i);
					_outmd((BM_MEM_INP_Read_SGAddr_high + (0x40 * (j + ListMove + 1))), add_high_i);
					SetDesc		= 0;
					OverHalf	= 1;
				} else if (OverHalf == 1){
					//--------------------------------------------
					// Move FIFO address for next list
					//--------------------------------------------
					add_cal		= (unsigned long long)(add_cal_dev + Lengthdev);
					SetDesc		= 1;
					OverHalf	= 0;
				}
				//--------------------------------------------
				// Transfer setting of final transfer block
				//--------------------------------------------
				if (i == (ListEnd - 1)){
					if (OverHalf == 1){
						add_cal_res					= (unsigned long long)(add_cal_n + pSgList->List[i].length);
						pMasAddr->BmInp.resetadd	= add_cal_res;
					} else if (OverHalf == 0){
						add_cal_res					= (unsigned long long)add_cal;
						pMasAddr->BmInp.resetadd	= add_cal_res;
					}
					if (pSgList->nr_pages > 150){
						if (i == (pSgList->nr_pages - 1)) {
							//--------------------------------------------
							// Set NextDescPtr to the beginning of Prefetcher
							//--------------------------------------------
							desc_cal		= (unsigned long long)BM_MEM_INP_TOP_DESC;
							ApiMemBmDivideAddress(desc_cal, &desc_low_i_n, &desc_high_i_n);
							_outmd((BM_MEM_INP_NextDescPtr + (0x40 * (j + ListMove + 1))), desc_low_i_n);
							_outmd((BM_MEM_INP_NextDescPtr_high + (0x40 * (j + ListMove + 1))), desc_high_i_n);
							_outmd((BM_MEM_INP_Descriptor + (0x40 * (j + ListMove + 1))), 0x40004000);
						} else {
							if (RePreflag == 1){
								//--------------------------------------------
								// Set NextDescPtr to the beginning of Prefetcher
								//--------------------------------------------
								desc_cal	= (unsigned long long)BM_MEM_INP_NXT_DESC;
								ApiMemBmDivideAddress(desc_cal, &desc_low_i_n, &desc_high_i_n);
								_outmd((BM_MEM_INP_NextDescPtr + (0x40 * (j + ListMove + 1))), desc_low_i_n);
								_outmd((BM_MEM_INP_NextDescPtr_high + (0x40 * (j + ListMove + 1))), desc_high_i_n);
							}
						}
					}
				}
				LengthStack					= Lengthdev;
			}
		}else{
			//--------------------------------------------
			// Set the Address Translation Table
			//--------------------------------------------
			if (pMasAddr->add64flag == 1){
				_outmd((BM_MEM_CRA + (0x8 * (j + BM_MEM_AT_OFFSET))), 0xFFFFFFFD);
				dwdata		= _inmd(BM_MEM_CRA + (0x8 * (j + BM_MEM_AT_OFFSET)));
				_outmd((BM_MEM_CRA + (0x8 * (j + BM_MEM_AT_OFFSET))), ((dwdata & sg_dma_address(&pSgList->List[i])) | 0x1));
				_outmd((BM_MEM_CRA_HIGH + (0x8 * (j + BM_MEM_AT_OFFSET))), (sg_dma_address(&pSgList->List[i]) >> 32 & 0xffffffff));
				cradata		= _inmd(BM_MEM_CRA + (0x8 * (j + BM_MEM_AT_OFFSET)));
				cradata2	= _inmd(BM_MEM_CRA_HIGH + (0x8 * (j + BM_MEM_AT_OFFSET)));
			} else {
				_outmd((BM_MEM_CRA + (0x8 * (j + BM_MEM_AT_OFFSET))), 0xFFFFFFFC);
				dwdata		= _inmd(BM_MEM_CRA + (0x8 * (j + BM_MEM_AT_OFFSET)));
				_outmd((BM_MEM_CRA + (0x8 * (j + BM_MEM_AT_OFFSET))), (dwdata & sg_dma_address(&pSgList->List[i])));
				_outmd((BM_MEM_CRA_HIGH + (0x8 * (j + BM_MEM_AT_OFFSET))), (sg_dma_address(&pSgList->List[i]) >> 32 & 0xffffffff));
				cradata		= _inmd(BM_MEM_CRA + (0x8 * (j + BM_MEM_AT_OFFSET)));
				cradata2	= _inmd(BM_MEM_CRA_HIGH + (0x8 * (j + BM_MEM_AT_OFFSET)));
			}
			LengthStack	+= pSgList->List[i].length;
			//--------------------------------------------
			// Move FIFO address for second and subsequent lists
			//--------------------------------------------
			if (selecttable	== 1){
				if (SetDesc == 1){
					SetDesc	= 0;
				} else {
					add_cal	= (unsigned long long)(add_cal_n + pSgList->List[i - 1].length);
				}
				ApiMemBmDivideAddress(add_cal, &add_low_o, &add_high_o);
			}
			add_cal_n	= add_cal;
			//--------------------------------------------
			// Set the beginning of FIFO when block exceeds 4kByte boundary
			//--------------------------------------------
			if ((LengthStack >= 0x2000) && (OverHalf == 1)){
				Lengthdev	= LengthStack - 0x2000;
				add_cal_n	= add_cal_dev;
				Lengthrem	= 0x2000 - (LengthStack - pSgList->List[i].length);
			}
			//--------------------------------------------
			// Specify the beginning of the next Descriptor Format in NextDescPtr
			//--------------------------------------------
			desc_cal	= (unsigned long long)(BM_MEM_OUT_NXT_DESC + (0x40 * (j + ListMove + 1)));
			ApiMemBmDivideAddress(desc_cal, &desc_low_o_n, &desc_high_o_n);
			//--------------------------------------------
			// Set Descriptor Format to Prefetcher FIFO
			//--------------------------------------------
			_outmd((BM_MEM_OUT_Write_SGAddr + (0x40 * (j + ListMove + 1))), add_low_o);
			_outmd((BM_MEM_OUT_Write_SGAddr_high + (0x40 * (j + ListMove + 1))), add_high_o);
			_outmd((BM_MEM_OUT_Read_SGAddr + (0x40 * (j + ListMove + 1))), ((sg_dma_address(&pSgList->List[i]) & ~dwdata) + (BM_MEM_CRA_SELECT * (j + BM_MEM_AT_OFFSET))));
			_outmd((BM_MEM_OUT_Read_SGAddr_high + (0x40 * (j + ListMove + 1))), (sg_dma_address(&pSgList->List[i]) >> 32 & 0xffffffff));
			_outmd((BM_MEM_OUT_Length + (0x40 * (j + ListMove + 1))), pSgList->List[i].length);
			_outmd((BM_MEM_OUT_NextDescPtr + (0x40 * (j + ListMove + 1))), desc_low_o_n);
			_outmd((BM_MEM_OUT_NextDescPtr_high + (0x40 * (j + ListMove + 1))), desc_high_o_n);
			_outmw((BM_MEM_OUT_SequenceNum + (0x40 * (j + ListMove + 1))), (i + ListMove +1));
			_outmd((BM_MEM_OUT_Stride + (0x40 * (j + ListMove + 1))), 0x0);
			_outmw((BM_MEM_OUT_Burst + (0x40 * (j + ListMove + 1))), 0x101);
			_outmd((BM_MEM_OUT_Descriptor + (0x40 * (j + ListMove + 1))), 0x40000000);
			//--------------------------------------------
			// Transfer setting of final transfer block
			//--------------------------------------------
			if ((LengthStack <= 0x2000) && (i == (ListEnd - 1))){
				if (i == (ListSize - 1)) {
					_outmd((BM_MEM_OUT_Descriptor + (0x40 * (j + ListMove + 1))), 0x40004000);
				}
				if ((LengthStack == 0x2000) && (OverHalf == 1)){
					add_cal_res	= (unsigned long long)(BM_MEM_OUT_WriteAdd);
				} else {
					add_cal_res	= (unsigned long long)(add_cal_n + pSgList->List[i].length);
				}
				pMasAddr->BmOut.resetadd	= add_cal_res;
				if ((LengthStack != 0x2000) && (pSgList->nr_pages > 150)){
					//--------------------------------------------
					// Set NextDescPtr of final transfer block
					//--------------------------------------------
					if (i == (ListSize - 1)) {
						desc_cal		= (unsigned long long)BM_MEM_OUT_TOP_DESC;
						ApiMemBmDivideAddress(desc_cal, &desc_low_o_n, &desc_high_o_n);
						_outmd((BM_MEM_OUT_NextDescPtr + (0x40 * (j + ListMove + 1))), desc_low_o_n);
						_outmd((BM_MEM_OUT_NextDescPtr_high + (0x40 * (j + ListMove + 1))), desc_high_o_n);
					} else {
						if (RePreflag == 1){
							desc_cal	= (unsigned long long)BM_MEM_OUT_NXT_DESC;
							ApiMemBmDivideAddress(desc_cal, &desc_low_o_n, &desc_high_o_n);
							_outmd((BM_MEM_OUT_NextDescPtr + (0x40 * (j + ListMove + 1))), desc_low_o_n);
							_outmd((BM_MEM_OUT_NextDescPtr_high + (0x40 * (j + ListMove + 1))), desc_high_o_n);
						}
					}
				}
			}
			//--------------------------------------------
			// Divide one block into 2k bytes
			//--------------------------------------------
			if ((LengthStack == 0x2000) && (i != (ListEnd - 1))){
				//--------------------------------------------
				// Store the next Descriptor start address
				//--------------------------------------------
				desc_cal					= (unsigned long long)(BM_MEM_OUT_NXT_DESC + (0x40 * (j + ListMove)));
				ApiMemBmDivideAddress(desc_cal, &desc_low_o_n, &desc_high_o_n);
				pSgList->ChangeDescAdd[n]	= desc_low_o_n;
				n++;
				if (DummySet == 1){
					DummySet						= 0;
				} else {
					//--------------------------------------------
					// Set FIFO start address to NextDescPtr
					//--------------------------------------------
					desc_cal						= (unsigned long long)BM_MEM_OUT_TOP_DESC;
					ApiMemBmDivideAddress(desc_cal, &desc_low_o_n, &desc_high_o_n);

					_outmd((BM_MEM_OUT_NextDescPtr + (0x40 * (j + ListMove + 1))), desc_low_o_n);
					_outmd((BM_MEM_OUT_NextDescPtr_high + (0x40 * (j + ListMove + 1))), desc_high_o_n);
				}
				LengthStack					= Lengthdev;
				if (OverHalf == 0){
					OverHalf	= 1;
				} else {
					OverHalf	= 0;
				}
			}else if (LengthStack > 0x2000){
				//--------------------------------------------
				// Split transfer number of 1 block into 2k bytes
				//--------------------------------------------
				Lengthdev					= LengthStack - 0x2000;
				Lengthrem					= 0x2000 - (LengthStack - pSgList->List[i].length);
				_outmd((BM_MEM_OUT_Length + (0x40 * (j + ListMove + 1))), Lengthrem);
				//--------------------------------------------
				// Store the next Descriptor start address
				//--------------------------------------------
				desc_cal					= (unsigned long long)(BM_MEM_OUT_NXT_DESC + (0x40 * (j + ListMove)));
				ApiMemBmDivideAddress(desc_cal, &desc_low_o_n, &desc_high_o_n);
				pSgList->ChangeDescAdd[n]	= desc_low_o_n;
				n++;
				//--------------------------------------------
				// Set current list as final list
				//--------------------------------------------
				desc_cal					= (unsigned long long)BM_MEM_OUT_TOP_DESC;
				ApiMemBmDivideAddress(desc_cal, &desc_low_o_n, &desc_high_o_n);
				_outmd((BM_MEM_OUT_NextDescPtr + (0x40 * (j + ListMove + 1))), desc_low_o_n);
				_outmd((BM_MEM_OUT_NextDescPtr_high + (0x40 * (j + ListMove + 1))), desc_high_o_n);
				if (DummySet == 1){
					DummySet						= 0;
				}
				//--------------------------------------------
				// Specify the beginning of the next Descriptor Format in NextDescPtr
				//--------------------------------------------
				ListMove++;
				desc_cal					= (unsigned long long)(BM_MEM_OUT_NXT_DESC + (0x40 * (j + ListMove + 1)));
				ApiMemBmDivideAddress(desc_cal, &desc_low_o_n, &desc_high_o_n);
				//--------------------------------------------
				// Set Descriptor Format to Prefetcher FIFO
				//--------------------------------------------
				_outmd((BM_MEM_OUT_Write_SGAddr + (0x40 * (j + ListMove + 1))), add_low_o_dev);
				_outmd((BM_MEM_OUT_Write_SGAddr_high + (0x40 * (j + ListMove + 1))), add_high_o_dev);
				_outmd((BM_MEM_OUT_Read_SGAddr + (0x40 * (j + ListMove + 1))), (((sg_dma_address(&pSgList->List[i]) + (pSgList->List[i].length - Lengthdev)) & ~dwdata) + (BM_MEM_CRA_SELECT * (j + BM_MEM_AT_OFFSET))));
				_outmd((BM_MEM_OUT_Read_SGAddr_high + (0x40 * (j + ListMove + 1))), (sg_dma_address(&pSgList->List[i]) >> 32 & 0xffffffff));
				_outmd((BM_MEM_OUT_Length + (0x40 * (j + ListMove + 1))), Lengthdev);
				_outmd((BM_MEM_OUT_NextDescPtr + (0x40 * (j + ListMove + 1))), desc_low_o_n);
				_outmd((BM_MEM_OUT_NextDescPtr_high + (0x40 * (j + ListMove + 1))), desc_high_o_n);
				_outmw((BM_MEM_OUT_SequenceNum + (0x40 * (j + ListMove + 1))), (i + ListMove +1));
				_outmd((BM_MEM_OUT_Stride + (0x40 * (j + ListMove + 1))), 0x0);
				_outmw((BM_MEM_OUT_Burst + (0x40 * (j + ListMove + 1))), 0x101);
				_outmd((BM_MEM_OUT_Descriptor + (0x40 * (j + ListMove + 1))), 0x40000000);
				//--------------------------------------------
				// If the FIFO address of the next block exceeds 4 kBytes, set the FIFO start address
				//--------------------------------------------
				if (OverHalf == 0){
					//--------------------------------------------
					// Set FIFO address for next list
					//--------------------------------------------
					add_cal		= (unsigned long long)(add_cal_n + Lengthrem);
					ApiMemBmDivideAddress(add_cal, &add_low_o, &add_high_o);
					_outmd((BM_MEM_OUT_Write_SGAddr + (0x40 * (j + ListMove + 1))), add_low_o);
					_outmd((BM_MEM_OUT_Write_SGAddr_high + (0x40 * (j + ListMove + 1))), add_high_o);
					SetDesc		= 0;
					OverHalf	= 1;
				} else if (OverHalf == 1){
					//--------------------------------------------
					// Move FIFO address for next list
					//--------------------------------------------
					add_cal		= (unsigned long long)(add_cal_dev + Lengthdev);
					SetDesc		= 1;
					OverHalf	= 0;
				}
				//--------------------------------------------
				// Transfer setting of final transfer block
				//--------------------------------------------
				if (i == (ListEnd - 1)){
					if (OverHalf == 1){
						add_cal_res					= (unsigned long long)(add_cal_n + pSgList->List[i].length);
						pMasAddr->BmOut.resetadd	= add_cal_res;
					} else if (OverHalf == 0){
						add_cal_res					= (unsigned long long)(add_cal);
						pMasAddr->BmOut.resetadd	= add_cal_res;
					}
					//--------------------------------------------
					// Address setting at SgList overwrite
					//--------------------------------------------
					if (pSgList->nr_pages > 150){
						if (i == (ListSize - 1)) {
							desc_cal		= (unsigned long long)BM_MEM_OUT_TOP_DESC;
							ApiMemBmDivideAddress(desc_cal, &desc_low_o_n, &desc_high_o_n);
							_outmd((BM_MEM_OUT_NextDescPtr + (0x40 * (j + ListMove + 1))), desc_low_o_n);
							_outmd((BM_MEM_OUT_NextDescPtr_high + (0x40 * (j + ListMove + 1))), desc_high_o_n);
							_outmd((BM_MEM_OUT_Descriptor + (0x40 * (j + ListMove + 1))), 0x40004000);
						} else {
							if (RePreflag == 1){
								desc_cal	= (unsigned long long)BM_MEM_OUT_NXT_DESC;
								ApiMemBmDivideAddress(desc_cal, &desc_low_o_n, &desc_high_o_n);
								_outmd((BM_MEM_OUT_NextDescPtr + (0x40 * (j + ListMove + 1))), desc_low_o_n);
								_outmd((BM_MEM_OUT_NextDescPtr_high + (0x40 * (j + ListMove + 1))), desc_high_o_n);
							}
						}
					}
				} else {
					//--------------------------------------------
					// Set current list as final list
					//--------------------------------------------
					if (i == (ListSize - 1)) {
						_outmd((BM_MEM_OUT_Descriptor + (0x40 * (j + ListMove + 1))), 0x40004000);
					}
				}
				LengthStack					= Lengthdev;
			}
		}
		j++;
		selecttable	= 1;
	}
	//============================================
	// Descriptor List Store parameters for reconfiguring
	//============================================
	if(dwDir == BM_DIR_IN){
		pMasAddr->BmInp.resetchangedesc	= n;
		pMasAddr->BmInp.atstartlen		= LengthStack;
		pMasAddr->BmInp.resetOverHalf	= OverHalf;
		pMasAddr->BmInp.resetListMove	= ListMove;
	}else {
		pMasAddr->BmOut.resetchangedesc	= n;
		pMasAddr->BmOut.atstartlen		= LengthStack;
		pMasAddr->BmOut.resetOverHalf	= OverHalf;
		pMasAddr->BmOut.resetListMove	= ListMove;
	}
	pSgList->ListNumCnt	= i;

	return	BM_ERROR_SUCCESS;
}

//========================================================================
// Function name  : MemBmSetSGAddressRing
// Function		  : Reconfigure S/G list in hardware. (Infinite transfer)
// I/F	 	  	  : Internal
// In	 		  : pMasAddr			: Point to the MASTERADDR structure
//		   		 	dwDir				: BM_DIR_IN / BM_DIR_OUT
// Out			  : None
// Return value   : Normal completed	: BM_ERROR_SUCCESS
//		   			Abnormal completed	: BM_ERROR_PARAM / BM_ERROR_BUFF
// Addition  	  : 
//========================================================================
long MemBmSetSGAddressRing(PMASTERADDR pMasAddr, unsigned long dwDir)
{
	unsigned long		i = 0;
	PSGLIST 			pSgList;
	unsigned long long	desc_cal;
	unsigned long long	add_cal, add_cal_n;
	unsigned long long	add_cal_dev;
	unsigned long		dwdata;
	unsigned long		desc_low_o_n, desc_high_o_n;
	unsigned long		desc_low_i_n, desc_high_i_n;
	unsigned long		cradata, cradata2;
	unsigned long 		add_low_i, add_high_i;
	unsigned long 		add_low_o, add_high_o;
	unsigned long		add_low_i_dev, add_high_i_dev;
	unsigned long		add_low_o_dev, add_high_o_dev;
	unsigned long		ListSize, ListNum;
	unsigned long		LengthStack, Lengthdev, Lengthrem;
	unsigned long		n, k, l;
	unsigned long		OverHalf, loop;
	unsigned long		LenSize;
	unsigned long		j, LenNum;
	unsigned long		ListStart, ListEnd;
	int					SetDesc, LastList;
	int					NextStart;

	//--------------------------------------------
	// Checking parameter
	//--------------------------------------------
	if(dwDir != BM_DIR_IN && dwDir != BM_DIR_OUT){
		return	BM_ERROR_PARAM;
	}
	if(dwDir == BM_DIR_IN){
		pSgList	= &pMasAddr->BmInp.SgList;
	}else{
		pSgList	= &pMasAddr->BmOut.SgList;
		n		= pMasAddr->BmOut.resetchangedesc;
	}
	if(pSgList->nr_pages > BM_MEM_MAX_SG_SIZE){
		return	BM_ERROR_BUFF;
	}
	//---------------------------------------------
	// Setting FIFO address
	//---------------------------------------------
	if(dwDir & BM_DIR_IN){
		add_low_i					= pSgList->MemParam.FIFOOverAdd;
		add_high_i					= pSgList->MemParam.FIFOOverAdd_high;
		add_cal						= (unsigned long long)(add_low_i + ((unsigned long long)add_high_i << 32));
		add_cal_dev					= (unsigned long long)BM_MEM_INP_ReadAdd;
		ApiMemBmDivideAddress(add_cal_dev, &add_low_i_dev, &add_high_i_dev);
		pMasAddr->BmInp.addtablenum	= 0;
	} else {
		add_low_o					= pSgList->MemParam.FIFOOverAdd;
		add_high_o					= pSgList->MemParam.FIFOOverAdd_high;
		add_cal						= (unsigned long long)(add_low_o + ((unsigned long long)add_high_o << 32));
		add_cal_dev					= (unsigned long long)BM_MEM_OUT_WriteAdd;
		ApiMemBmDivideAddress(add_cal_dev, &add_low_o_dev, &add_high_o_dev);
		pMasAddr->BmOut.addtablenum	= 0;
	}
	add_cal_n	= add_cal;
	ListSize	= pSgList->nr_pages;
	//--------------------------------------------
	// If the number of SGList exceeds a certain number, limit the number of lists for reconfiguration
	//--------------------------------------------
	if(dwDir & BM_DIR_IN){
		OverHalf	= pMasAddr->BmInp.resetOverHalf;
		if (pMasAddr->BmInp.sglen < 0x1000){
			ListEnd	= ListSize;
			LenSize	= 0;
			for (loop = 0; loop < ListSize; loop++) {
				LenSize	+=  pMasAddr->BmInp.SgList.List[loop].length;
			}
			LenNum	= 0x2000 / LenSize;
			if (((LenSize * LenNum) + pSgList->MemParam.OverLen) < 0x2000){
				LenNum++;
			}
			if ((0x2000 % LenSize) != 0){
				LenNum++;
			}
			if (OverHalf == 1){
				l	= pMasAddr->BmInp.resetListMove;
				k	= 5;
				n	= pMasAddr->BmInp.resetchangedesc;
			} else {
				k	= 0;
				n	= 0;
				l	= 0;
			}
		} else {
			loop	= pSgList->MemParam.ATNumber + 1;
			LenNum	= 1;
			LenSize	= pSgList->MemParam.OverLen;
			while (LenSize <= 0x2000){
				if (loop == ListSize){
					loop	= 0;
					LenNum++;
				}
				LenSize	+= pSgList->List[loop].length;
				loop++;
			}
			ListEnd	= ListSize;
			ListNum	= loop - pSgList->ListNumCnt - 1;
			if (OverHalf == 1){
				l	= pMasAddr->BmInp.resetListMove;
				k	= 5;
			} else {
				k	= 0;
				l	= 0;
			}
			n		= pMasAddr->BmInp.resetchangedesc;
		}
		if ((add_low_i == BM_MEM_INP_ReadAdd) || ((add_low_i - 0x2000) == BM_MEM_INP_ReadAdd)){
			LengthStack	= pSgList->MemParam.OverLen;
		} else {
			LengthStack	= pSgList->MemParam.OverLen + (pSgList->MemParam.FIFOOverAdd - BM_MEM_INP_ReadAdd);
			if (LengthStack >= 0x2000){
				LengthStack	= LengthStack - 0x2000;
			}
		}
	} else {
		OverHalf	= pMasAddr->BmOut.resetOverHalf;
		if (pMasAddr->BmOut.sglen < 0x1000){
			ListEnd	= ListSize;
			LenSize	= 0;
			for (loop = 0; loop < ListSize; loop++) {
				LenSize	+=  pMasAddr->BmOut.SgList.List[loop].length;
			}
			LenNum	= 0x2000 / LenSize;
			if (((LenSize * LenNum) + pSgList->MemParam.OverLen) < 0x2000){
				LenNum++;
			}
			if ((0x2000 % LenSize) != 0){
				LenNum++;
			}
			if (OverHalf == 1){
				l	= pMasAddr->BmOut.resetListMove;
				k	= 5;
				n	= pMasAddr->BmOut.resetchangedesc;
			} else {
				k	= 0;
				n	= 0;
				l	= 0;
			}
		} else {
			loop	= pSgList->MemParam.ATNumber + 1;
			LenNum	= 1;
			LenSize	= pSgList->MemParam.OverLen;
			while (LenSize <= 0x2000){
				if (loop == ListSize){
					loop	= 0;
					LenNum++;
				}
				LenSize	+= pSgList->List[loop].length;
				loop++;
			}
			ListEnd	= ListSize;
			ListNum	= loop - pSgList->ListNumCnt - 1;
			if (OverHalf == 1){
				l	= pMasAddr->BmOut.resetListMove;
				k	= 5;
			} else {
				k	= 0;
				l	= 0;
			}
			n	= pMasAddr->BmOut.resetchangedesc;
		}
		if (LenSize < 0x7D0){
				if ((add_low_o == BM_MEM_OUT_WriteAdd) || ((add_low_o - 0x2000) == BM_MEM_OUT_WriteAdd)){
					LengthStack	= pSgList->MemParam.OverLen;
			} else {
				LengthStack	= pSgList->MemParam.OverLen + (pSgList->MemParam.FIFOOverAdd - BM_MEM_OUT_WriteAdd);
				if (LengthStack >= 0x4000){
					LengthStack	= LengthStack - 0x2000;
				}
			}
		} else {
			if ((add_low_o == BM_MEM_OUT_WriteAdd) || ((add_low_o - 0x2000) == BM_MEM_OUT_WriteAdd)){
				LengthStack	= pSgList->MemParam.OverLen;
			} else {
				LengthStack	= pSgList->MemParam.OverLen + (pSgList->MemParam.FIFOOverAdd - BM_MEM_OUT_WriteAdd);
				if (LengthStack >= 0x4000){
					LengthStack	= LengthStack - 0x2000;
				}
			}
		}
	}
	ListStart	= pSgList->MemParam.ATNumber;
	Lengthdev	= 0;
	Lengthrem	= 0;
	SetDesc		= 0;
	LastList	= 0;
	NextStart	= 0;
	//--------------------------------------------
	// Loop the number of times
	//--------------------------------------------
	for (j=0; j < LenNum; j++) {
		//--------------------------------------------
		// Loop the number of times
		//--------------------------------------------
		if (j == 0){
			i	= ListStart;
		} else {
			i	= 0;
		}
		for (; i < ListEnd; i++) {
			ListNum++;
			//--------------------------------------------
			// Set in master register
			//--------------------------------------------
			if(dwDir == BM_DIR_IN){	
				//--------------------------------------------
				// Set the Address Translation Table
				//--------------------------------------------
				if (pMasAddr->add64flag == 1){
					_outmd((BM_MEM_CRA + (0x8 * l)), 0xFFFFFFFD);
					dwdata = _inmd(BM_MEM_CRA + (0x8 * l));
					_outmd((BM_MEM_CRA + (0x8 * l)), ((dwdata & sg_dma_address(&pSgList->List[i])) | 0x1));
					_outmd((BM_MEM_CRA_HIGH + (0x8 * l)), (sg_dma_address(&pSgList->List[i]) >> 32 & 0xffffffff));
					cradata = _inmd(BM_MEM_CRA + (0x8 * l));
					cradata2 = _inmd(BM_MEM_CRA_HIGH + (0x8 * l));
				} else {
					_outmd((BM_MEM_CRA + (0x8 * l)), 0xFFFFFFFC);
					dwdata = _inmd(BM_MEM_CRA + (0x8 * l));
					_outmd((BM_MEM_CRA + (0x8 * l)), (dwdata & sg_dma_address(&pSgList->List[i])));
					_outmd((BM_MEM_CRA_HIGH + (0x8 * l)), (sg_dma_address(&pSgList->List[i]) >> 32 & 0xffffffff));
					cradata = _inmd(BM_MEM_CRA + (0x8 * l));
					cradata2 = _inmd(BM_MEM_CRA_HIGH + (0x8 * l));
				}
				if ((i > ListStart) || (j != 0)){
					LengthStack	+= pSgList->List[i].length;
				}
				//--------------------------------------------
				// Move FIFO address for second and subsequent lists
				//--------------------------------------------
				if ((i > (ListStart + 1)) && (j == 0)){
					if (SetDesc == 1){
						SetDesc	= 0;
					} else {
						add_cal	= (unsigned long long)(add_cal_n + pSgList->List[i - 1].length);
					}
					
				} else if ((i == (ListStart + 1)) && (j == 0)){
					add_cal		= (unsigned long long)(add_cal_n + pSgList->MemParam.OverLen);
					NextStart	= 1;
				} else if ((i == 0) && (NextStart == 0) && (j == 1)){
					add_cal		= (unsigned long long)(add_cal_n + pSgList->MemParam.OverLen);
				} else if (j > 0){
					if (SetDesc == 1){
						SetDesc	= 0;
					} else {
						if (i > 0){
							add_cal	= (unsigned long long)(add_cal_n + pSgList->List[i - 1].length);
						} else {
							add_cal	= (unsigned long long)(add_cal_n + pSgList->List[ListSize - 1].length);
						}
					}
				}
				ApiMemBmDivideAddress(add_cal, &add_low_i, &add_high_i);
				add_cal_n	= add_cal;
				//--------------------------------------------
				// Set the beginning of FIFO when block exceeds 4kByte boundary
				//--------------------------------------------
				if ((LengthStack >= 0x2000) && (OverHalf == 1)){
					Lengthdev	= LengthStack - 0x2000;
					add_cal_n	= add_cal_dev;
					Lengthrem	= 0x2000 - (LengthStack - pSgList->List[i].length);
				}
				//--------------------------------------------
				// Specify the beginning of the next Descriptor Format in NextDescPtr
				//--------------------------------------------
				desc_cal	= (unsigned long long)(BM_MEM_INP_NXT_DESC + (0x40 * (l + k + 1)));
				ApiMemBmDivideAddress(desc_cal, &desc_low_i_n, &desc_high_i_n);
				//--------------------------------------------
				// Set Descriptor Format to Prefetcher FIFO
				//--------------------------------------------
				if ((j == 0) && (i == ListStart)){
					_outmd((BM_MEM_INP_Write_SGAddr + (0x40 * (l + k + 1))), ((pSgList->MemParam.SGOverAdd & ~dwdata) + (BM_MEM_CRA_SELECT * l)));
					_outmd((BM_MEM_INP_Write_SGAddr_high + (0x40 * (l + k + 1))), pSgList->MemParam.SGOverAdd_high);
					_outmd((BM_MEM_INP_Length + (0x40 * (l + k + 1))), pSgList->MemParam.OverLen);
					pSgList->MemParam.ATPreFetchAdd	= (BM_MEM_INP_NXT_DESC + (0x40 * (l + k)));
				} else {
					_outmd((BM_MEM_INP_Write_SGAddr + (0x40 * (l + k + 1))), ((sg_dma_address(&pSgList->List[i]) & ~dwdata) + (BM_MEM_CRA_SELECT * l)));
					_outmd((BM_MEM_INP_Write_SGAddr_high + (0x40 * (l + k + 1))), (sg_dma_address(&pSgList->List[i]) >> 32 & 0xffffffff));
					_outmd((BM_MEM_INP_Length + (0x40 * (l + k + 1))), pSgList->List[i].length);
				}
				_outmd((BM_MEM_INP_Read_SGAddr + (0x40 * (l + k + 1))), add_low_i);
				_outmd((BM_MEM_INP_Read_SGAddr_high + (0x40 * (l + k + 1))), add_high_i);
				_outmd((BM_MEM_INP_NextDescPtr + (0x40 * (l + k + 1))), desc_low_i_n);
				_outmd((BM_MEM_INP_NextDescPtr_high + (0x40 * (l + k + 1))), desc_high_i_n);
				_outmw((BM_MEM_INP_SequenceNum + (0x40 * (l + k + 1))), (l + k + 1));
				_outmd((BM_MEM_INP_Stride + (0x40 * (l + k + 1))), 0x0);
				_outmw((BM_MEM_INP_Burst + (0x40 * (l + k + 1))), 0x101);
				_outmd((BM_MEM_INP_Descriptor + (0x40 * (l + k + 1))), 0x40000000);
				//--------------------------------------------
				// Transfer setting of final transfer block
				//--------------------------------------------
				if ((pMasAddr->BmInp.sglen < 0x1000) && (j == (LenNum - 1)) && (i == (ListEnd - 1))){
					_outmd((BM_MEM_INP_Descriptor + (0x40 * (l + k + 1))), 0x40004000);
				}
				//--------------------------------------------
				// Divide one block into 2k bytes
				//--------------------------------------------
				if (LengthStack == 0x2000){
					desc_cal	= (unsigned long long)BM_MEM_INP_TOP_DESC;
					ApiMemBmDivideAddress(desc_cal, &desc_low_i_n, &desc_high_i_n);
					_outmd((BM_MEM_INP_NextDescPtr + (0x40 * (l + k + 1))), desc_low_i_n);
					_outmd((BM_MEM_INP_NextDescPtr_high + (0x40 * (l + k + 1))), desc_high_i_n);
					//--------------------------------------------
					// Descriptor Store parameters for reconfiguring
					//--------------------------------------------
					if (OverHalf == 0){
						add_cal								= (unsigned long long)(add_cal_n + pSgList->List[i].length);
						ApiMemBmDivideAddress(add_cal, &add_low_i, &add_high_i);
						pSgList->MemParam.FIFOOverAdd		= add_low_i;
						pSgList->MemParam.FIFOOverAdd_high	= add_high_i;
						OverHalf							= 1;
					} else {
						pSgList->MemParam.FIFOOverAdd		= add_low_i_dev;
						pSgList->MemParam.FIFOOverAdd_high	= add_high_i_dev;
						OverHalf							= 0;
					}
					if (i == (ListSize - 1)){
						pSgList->MemParam.SGOverAdd			= sg_dma_address(&pSgList->List[0]);
						pSgList->MemParam.SGOverAdd_high	= (sg_dma_address(&pSgList->List[0]) >> 32 & 0xffffffff);
						pSgList->MemParam.OverLen			= pSgList->List[0].length;
						pSgList->MemParam.ATNumber			= 0;
					} else {
						pSgList->MemParam.SGOverAdd			= sg_dma_address(&pSgList->List[i + 1]);
						pSgList->MemParam.SGOverAdd_high	= (sg_dma_address(&pSgList->List[i + 1]) >> 32 & 0xffffffff);
						pSgList->MemParam.OverLen			= pSgList->List[i + 1].length;
						pSgList->MemParam.ATNumber			= i + 1;
					}
					_outmd((BM_MEM_INP_Descriptor + (0x40 * (l + k + 1))), 0x40004000);
					LengthStack	= Lengthdev;
					LastList	= 1;
				} else if (LengthStack > 0x2000){
					Lengthdev							= LengthStack - 0x2000;
					Lengthrem							= 0x2000 - (LengthStack - pSgList->List[i].length);
					_outmd((BM_MEM_INP_Length + (0x40 * (l + k + 1))), Lengthrem);
					//--------------------------------------------
					// Specify the beginning of the next Descriptor Format in NextDescPtr
					//--------------------------------------------
					desc_cal							= (unsigned long long)BM_MEM_INP_TOP_DESC;
					ApiMemBmDivideAddress(desc_cal, &desc_low_i_n, &desc_high_i_n);
					_outmd((BM_MEM_INP_NextDescPtr + (0x40 * (l + k + 1))), desc_low_i_n);
					_outmd((BM_MEM_INP_NextDescPtr_high + (0x40 * (l + k + 1))), desc_high_i_n);
					//--------------------------------------------
					// Descriptor Store parameters for reconfiguring
					//--------------------------------------------
					if (OverHalf == 0){
						add_cal								= (unsigned long long)(add_cal_n + Lengthrem);
						ApiMemBmDivideAddress(add_cal, &add_low_i, &add_high_i);
						pSgList->MemParam.FIFOOverAdd		= add_low_i;
						pSgList->MemParam.FIFOOverAdd_high	= add_high_i;
						OverHalf		= 1;
					} else {
						pSgList->MemParam.FIFOOverAdd		= add_low_i_dev;
						pSgList->MemParam.FIFOOverAdd_high	= add_high_i_dev;
						OverHalf		= 0;
					}
					pSgList->MemParam.SGOverAdd			= sg_dma_address(&pSgList->List[i]) + Lengthrem;
					pSgList->MemParam.SGOverAdd_high	= (sg_dma_address(&pSgList->List[i]) >> 32 & 0xffffffff);
					pSgList->MemParam.OverLen			= Lengthdev;
					pSgList->MemParam.ATNumber			= i;
					LengthStack							= Lengthdev;
					LastList							= 1;
				}
			}else{
				//--------------------------------------------
				// Set the Address Translation Table
				//--------------------------------------------
				if (pMasAddr->add64flag == 1){
					_outmd((BM_MEM_CRA + (0x8 * (l + BM_MEM_AT_OFFSET))), 0xFFFFFFFD);
					dwdata = _inmd(BM_MEM_CRA + (0x8 * (l + BM_MEM_AT_OFFSET)));
					_outmd((BM_MEM_CRA + (0x8 * (l + BM_MEM_AT_OFFSET))), ((dwdata & sg_dma_address(&pSgList->List[i])) | 0x1));
					_outmd((BM_MEM_CRA_HIGH + (0x8 * (l + BM_MEM_AT_OFFSET))), (sg_dma_address(&pSgList->List[i]) >> 32 & 0xffffffff));
					cradata = _inmd(BM_MEM_CRA + (0x8 * (l + BM_MEM_AT_OFFSET)));
					cradata2 = _inmd(BM_MEM_CRA_HIGH + (0x8 * (l + BM_MEM_AT_OFFSET)));
				} else {
					_outmd((BM_MEM_CRA + (0x8 * (l + BM_MEM_AT_OFFSET))), 0xFFFFFFFC);
					dwdata = _inmd(BM_MEM_CRA + (0x8 * (l + BM_MEM_AT_OFFSET)));
					_outmd((BM_MEM_CRA + (0x8 * (l + BM_MEM_AT_OFFSET))), (dwdata & sg_dma_address(&pSgList->List[i])));
					_outmd((BM_MEM_CRA_HIGH + (0x8 * (l + BM_MEM_AT_OFFSET))), (sg_dma_address(&pSgList->List[i]) >> 32 & 0xffffffff));
					cradata = _inmd(BM_MEM_CRA + (0x8 * (l + BM_MEM_AT_OFFSET)));
					cradata2 = _inmd(BM_MEM_CRA_HIGH + (0x8 * (l + BM_MEM_AT_OFFSET)));
				}
				if ((i > ListStart) || (j != 0)){
					LengthStack	+= pSgList->List[i].length;
				}
				
				//--------------------------------------------
				// Move FIFO address for second and subsequent lists
				//--------------------------------------------
				if ((i > (ListStart + 1)) && (j == 0)){
					if (SetDesc == 1){
						SetDesc	= 0;
					} else {
						add_cal	= (unsigned long long)(add_cal_n + pSgList->List[i - 1].length);
					}
				} else if ((i == (ListStart + 1)) && (j == 0)){
					add_cal		= (unsigned long long)(add_cal_n + pSgList->MemParam.OverLen);
					NextStart	= 1;
				} else if ((i == 0) && (NextStart == 0) && (j == 1)){
					add_cal		= (unsigned long long)(add_cal_n + pSgList->MemParam.OverLen);
				} else if (j > 0){
					if (SetDesc == 1){
						SetDesc	= 0;
					} else {
						if (i > 0){
							add_cal	= (unsigned long long)(add_cal_n + pSgList->List[i - 1].length);
						} else {
							add_cal	= (unsigned long long)(add_cal_n + pSgList->List[ListSize - 1].length);
						}
					}
				}
				ApiMemBmDivideAddress(add_cal, &add_low_o, &add_high_o);
				add_cal_n	= add_cal;
				//--------------------------------------------
				// Set the beginning of FIFO when block exceeds 4kByte boundary
				//--------------------------------------------
				if ((LengthStack >= 0x2000) && (OverHalf == 1)){
					Lengthdev	= LengthStack - 0x2000;
					add_cal_n	= add_cal_dev;
					Lengthrem	= 0x2000 - (LengthStack - pSgList->List[i].length);
				}
				//--------------------------------------------
				// Specify the beginning of the next Descriptor Format in NextDescPtr
				//--------------------------------------------
				desc_cal	= (unsigned long long)(BM_MEM_OUT_NXT_DESC + (0x40 * (l + k + 1)));
				ApiMemBmDivideAddress(desc_cal, &desc_low_o_n, &desc_high_o_n);
				//--------------------------------------------
				// Set Descriptor Format to Prefetcher FIFO
				//--------------------------------------------
				if ((j == 0) && (i == ListStart)){
					_outmd((BM_MEM_OUT_Read_SGAddr + (0x40 * (l + k + 1))), ((pSgList->MemParam.SGOverAdd & ~dwdata) + (BM_MEM_CRA_SELECT * (l + BM_MEM_AT_OFFSET))));
					//_outmd((BM_MEM_OUT_Read_SGAddr_high + (0x40 * (l + k + 1))), pSgList->MemParam.SGOverAdd_high);
					_outmd((BM_MEM_OUT_Length + (0x40 * (l + k + 1))), pSgList->MemParam.OverLen);
					pSgList->MemParam.ATPreFetchAdd	= (BM_MEM_OUT_NXT_DESC + (0x40 * (l + k)));
				} else {
					_outmd((BM_MEM_OUT_Read_SGAddr + (0x40 * (l + k + 1))), ((sg_dma_address(&pSgList->List[i]) & ~dwdata) + (BM_MEM_CRA_SELECT * (l + BM_MEM_AT_OFFSET))));
					_outmd((BM_MEM_OUT_Read_SGAddr_high + (0x40 * (l + k + 1))), (sg_dma_address(&pSgList->List[i]) >> 32 & 0xffffffff));
					_outmd((BM_MEM_OUT_Length + (0x40 * (l + k + 1))), pSgList->List[i].length);
				}
				_outmd((BM_MEM_OUT_Write_SGAddr + (0x40 * (l + k + 1))), add_low_o);
				_outmd((BM_MEM_OUT_Write_SGAddr_high + (0x40 * (l + k + 1))), add_high_o);
				_outmd((BM_MEM_OUT_NextDescPtr + (0x40 * (l + k + 1))), desc_low_o_n);
				_outmd((BM_MEM_OUT_NextDescPtr_high + (0x40 * (l + k + 1))), desc_high_o_n);
				_outmw((BM_MEM_OUT_SequenceNum + (0x40 * (l + k + 1))), (l + k + 1));
				_outmd((BM_MEM_OUT_Stride + (0x40 * (l + k + 1))), 0x0);
				_outmw((BM_MEM_OUT_Burst + (0x40 * (l + k + 1))), 0x101);
				_outmd((BM_MEM_OUT_Descriptor + (0x40 * (l + k + 1))), 0x40000000);
				//--------------------------------------------
				// Transfer setting of final transfer block
				//--------------------------------------------
				if (LengthStack == 0x2000){
					desc_cal	= (unsigned long long)BM_MEM_OUT_TOP_DESC;
					ApiMemBmDivideAddress(desc_cal, &desc_low_o_n, &desc_high_o_n);
					_outmd((BM_MEM_OUT_NextDescPtr + (0x40 * (l + k + 1))), desc_low_o_n);
					_outmd((BM_MEM_OUT_NextDescPtr_high + (0x40 * (l + k + 1))), desc_high_o_n);
					//--------------------------------------------
					// Descriptor Store parameters for reconfiguring
					//--------------------------------------------
					if (OverHalf == 0){
						add_cal								= (unsigned long long)(add_cal_n + pSgList->List[i].length);
						ApiMemBmDivideAddress(add_cal, &add_low_o, &add_high_o);
						pSgList->MemParam.FIFOOverAdd		= add_low_o;
						pSgList->MemParam.FIFOOverAdd_high	= add_high_o;
						OverHalf							= 1;
					} else {
						pSgList->MemParam.FIFOOverAdd		= add_low_o_dev;
						pSgList->MemParam.FIFOOverAdd_high	= add_high_o_dev;
						OverHalf							= 0;
					}
					if (i == (ListSize - 1)){
						pSgList->MemParam.SGOverAdd			= sg_dma_address(&pSgList->List[0]);
						pSgList->MemParam.SGOverAdd_high	= (sg_dma_address(&pSgList->List[0]) >> 32 & 0xffffffff);
						pSgList->MemParam.OverLen			= pSgList->List[0].length;
						pSgList->MemParam.ATNumber			= 0;
					} else {
						pSgList->MemParam.SGOverAdd			= sg_dma_address(&pSgList->List[i + 1]);
						pSgList->MemParam.SGOverAdd_high	= (sg_dma_address(&pSgList->List[i + 1]) >> 32 & 0xffffffff);
						pSgList->MemParam.OverLen			= pSgList->List[i + 1].length;
						pSgList->MemParam.ATNumber			= i + 1;
					}
					LengthStack	= Lengthdev;
					if (LenSize >= 0x7D0){
						LastList	= 1;
					} else if ((LenSize < 0x7D0) && (j == (LenNum - 1))){
						LastList	= 1;
					}
				} else if (LengthStack > 0x2000){
					Lengthdev							= LengthStack - 0x2000;
					Lengthrem							= 0x2000 - (LengthStack - pSgList->List[i].length);
					_outmd((BM_MEM_OUT_Length + (0x40 * (l + k + 1))), Lengthrem);
					//--------------------------------------------
					// Set NextDescPtr to the beginning of Prefetcher
					//--------------------------------------------
					desc_cal							= (unsigned long long)BM_MEM_OUT_TOP_DESC;
					ApiMemBmDivideAddress(desc_cal, &desc_low_o_n, &desc_high_o_n);
					_outmd((BM_MEM_OUT_NextDescPtr + (0x40 * (l + k + 1))), desc_low_o_n);
					_outmd((BM_MEM_OUT_NextDescPtr_high + (0x40 * (l + k + 1))), desc_high_o_n);
					//--------------------------------------------
					// Descriptor Store parameters for reconfiguring
					//--------------------------------------------
					if (OverHalf == 0){
						add_cal		= (unsigned long long)(add_cal_n + Lengthrem);
						ApiMemBmDivideAddress(add_cal, &add_low_o, &add_high_o);
						pSgList->MemParam.FIFOOverAdd		= add_low_o;
						pSgList->MemParam.FIFOOverAdd_high	= add_high_o;
						OverHalf		= 1;
					} else {
						pSgList->MemParam.FIFOOverAdd		= add_low_o_dev;
						pSgList->MemParam.FIFOOverAdd_high	= add_high_o_dev;
						OverHalf		= 0;
					}
					pSgList->MemParam.SGOverAdd			= sg_dma_address(&pSgList->List[i]) + Lengthrem;
					pSgList->MemParam.SGOverAdd_high	= (sg_dma_address(&pSgList->List[i]) >> 32 & 0xffffffff);
					pSgList->MemParam.OverLen			= Lengthdev;
					pSgList->MemParam.ATNumber			= i;	
					LengthStack	= Lengthdev;
					if (LenSize >= 0x7D0){
						LastList	= 1;
					} else if ((LenSize < 0x7D0) && (j == (LenNum - 1))){
						LastList	= 1;
					}
				}	
			}
			l++;
			if (LastList == 1){
				break;
			}
		}
		if (LastList == 1){
			break;
		}
	}
	//============================================
	// Descriptor List Store parameters for reconfiguring
	//============================================
	if(dwDir == BM_DIR_IN){
		pMasAddr->BmInp.resetchangedesc = n;
		pMasAddr->BmInp.resetListMove	= l;
		pMasAddr->BmInp.resetOverHalf	= OverHalf;
	}else {
		pMasAddr->BmOut.resetchangedesc = n;
		pMasAddr->BmOut.resetListMove	= l;
		pMasAddr->BmOut.resetOverHalf	= OverHalf;
	}
	pSgList->ListNumCnt				= i;
	pSgList->ATOverCnt				= 0;
	return	BM_ERROR_SUCCESS;
}

//========================================================================
// Function name  : MemBmSetSGAddressRingPre
// Function		  : Reconfigure S/G list in hardware. (Infinite transfer)
// I/F	 	  	  : Internal
// In	 		  : pMasAddr			: Point to the MASTERADDR structure
//		   		 	dwDir				: BM_DIR_IN / BM_DIR_OUT
// Out			  : None
// Return value   : Normal completed	: BM_ERROR_SUCCESS
//		   			Abnormal completed	: BM_ERROR_PARAM / BM_ERROR_BUFF
// Addition  	  : 
//========================================================================
long MemBmSetSGAddressRingPre(PMASTERADDR pMasAddr, unsigned long dwDir)
{
	unsigned long		i = 0;
	PSGLIST 			pSgList;
	unsigned long long	desc_cal;
	unsigned long long	add_cal, add_cal_n;
	unsigned long long	add_cal_res;
	unsigned long long	add_cal_dev;
	unsigned long		dwdata = 0;
	unsigned long		desc_low_o_n, desc_high_o_n;
	unsigned long		desc_low_i_n, desc_high_i_n;
	unsigned long		cradata, cradata2;
	unsigned long		add_low_i, add_high_i;
	unsigned long		add_low_o, add_high_o;
	unsigned long		add_low_i_dev, add_high_i_dev;
	unsigned long		add_low_o_dev, add_high_o_dev;
	unsigned long		ListSize, ListNum;
	unsigned long		LengthStack, Lengthdev, Lengthrem;
	unsigned long		j, k, n;
	unsigned long		OverHalf, loop;
	unsigned long		Outlen;
	unsigned long		LenSize;
	unsigned long		ListMove, LenNum;
	int					SetDesc, DummySet, LastList;
	int					UnderLen;
	int					OverFIFO = 0;

	//--------------------------------------------
	// Checking parameter
	//--------------------------------------------
	if(dwDir != BM_DIR_IN && dwDir != BM_DIR_OUT){
		return	BM_ERROR_PARAM;
	}
	if(dwDir == BM_DIR_IN){
		pSgList	= &pMasAddr->BmInp.SgList;
		n		= 0;
	}else{
		pSgList	= &pMasAddr->BmOut.SgList;
		n		= pMasAddr->BmOut.resetchangedesc;
	}
	if(pSgList->nr_pages > BM_MEM_MAX_SG_SIZE){
		return	BM_ERROR_BUFF;
	}
	//---------------------------------------------
	// Setting FIFO address
	//---------------------------------------------
	if(dwDir & BM_DIR_IN){
		add_cal						= (unsigned long long)BM_MEM_INP_ReadAdd;
		add_cal_dev					= add_cal;
		ApiMemBmDivideAddress(add_cal_dev, &add_low_i_dev, &add_high_i_dev);
		pMasAddr->BmInp.addtablenum	= 0;
	} else {
		add_cal						= (unsigned long long)BM_MEM_OUT_WriteAdd;
		add_cal_dev					= add_cal;
		ApiMemBmDivideAddress(add_cal_dev, &add_low_o_dev, &add_high_o_dev);
		pMasAddr->BmOut.addtablenum	= 0;
	}
	add_cal_n	= add_cal;
	ListSize	= pSgList->nr_pages;
	//--------------------------------------------
	// If the number of SGList exceeds a certain number, limit the number of lists for reconfiguration
	//--------------------------------------------
	if(dwDir & BM_DIR_IN){
		LenSize	= 0;
		for (loop = 0; loop < ListSize; loop++) {
			LenSize	+=  pMasAddr->BmInp.SgList.List[loop].length;
		}
		LenNum	= 0x4000 / LenSize;
		if (((0x4000 % LenSize) != 0) && (pMasAddr->BmInp.ringstartflag == 0)){
			LenNum++;
		}
		if (LenNum > 150){
			LenNum		= 150;
			OverFIFO	= 1;
		}
	} else {
		OverFIFO	= 0;
		LenSize		= 0;
		for (loop = 0; loop < ListSize; loop++) {
			LenSize	+=  pMasAddr->BmOut.SgList.List[loop].length;
		}
		LenNum	= 0x4000 / LenSize;
		if ((0x4000 % LenSize) != 0){
			LenNum++;
		}
		if (LenNum > 150){
			LenNum		= 150;
			OverFIFO	= 1;
		}
	}
	LengthStack	= 0;
	k			= 0;
	ListMove	= 0;
	Lengthdev	= 0;
	Lengthrem	= 0;
	ListNum		= 0;
	SetDesc		= 0;
	DummySet	= 1;
	LastList	= 0;
	OverHalf	= 0;
	UnderLen	= 0;
	Outlen		= 0;
	//--------------------------------------------
	// Loop the number of times
	//--------------------------------------------
	for (j=0; j < LenNum; j++) {
		//--------------------------------------------
		// Loop the number of times
		//--------------------------------------------
		for (i=0; i < ListSize; i++) {
			ListNum++;
			//--------------------------------------------
			// Set in master register
			//--------------------------------------------
			if(dwDir == BM_DIR_IN){	
				if (j == 0){
					//--------------------------------------------
					// Set the Address Translation Table
					//--------------------------------------------
					if (pMasAddr->add64flag == 1){
						_outmd((BM_MEM_CRA + (0x8 * i)), 0xFFFFFFFD);
						dwdata		= _inmd(BM_MEM_CRA + (0x8 * i));
						_outmd((BM_MEM_CRA + (0x8 * i)), ((dwdata & sg_dma_address(&pSgList->List[i])) | 0x1));
						_outmd((BM_MEM_CRA_HIGH + (0x8 * i)), (sg_dma_address(&pSgList->List[i]) >> 32 & 0xffffffff));
						cradata		= _inmd(BM_MEM_CRA + (0x8 * i));
						cradata2	= _inmd(BM_MEM_CRA_HIGH + (0x8 * i));
					} else {
						_outmd((BM_MEM_CRA + (0x8 * i)), 0xFFFFFFFC);
						dwdata		= _inmd(BM_MEM_CRA + (0x8 * i));
						_outmd((BM_MEM_CRA + (0x8 * i)), (dwdata & sg_dma_address(&pSgList->List[i])));
						_outmd((BM_MEM_CRA_HIGH + (0x8 * i)), (sg_dma_address(&pSgList->List[i]) >> 32 & 0xffffffff));
						cradata		= _inmd(BM_MEM_CRA + (0x8 * i));
						cradata2	= _inmd(BM_MEM_CRA_HIGH + (0x8 * i));
					}
				}
				LengthStack	+= pSgList->List[i].length;
				//--------------------------------------------
				// Move FIFO address for second and subsequent lists
				//--------------------------------------------
				if ((i > 0) && (j == 0)){
					if (SetDesc == 1){
						SetDesc	= 0;
					} else {
						add_cal	= (unsigned long long)(add_cal_n + pSgList->List[i - 1].length);
					}
				} else if (j > 0){
					if (SetDesc == 1){
						SetDesc	= 0;
					} else {
						if (i > 0){
							add_cal	= (unsigned long long)(add_cal_n + pSgList->List[i - 1].length);
						} else {
							add_cal	= (unsigned long long)(add_cal_n + pSgList->List[ListSize - 1].length);
						}
					}
				}
				ApiMemBmDivideAddress(add_cal, &add_low_i, &add_high_i);
				add_cal_n	= add_cal;
				//--------------------------------------------
				// Set the beginning of FIFO when block exceeds 4kByte boundary
				//--------------------------------------------
				if ((LengthStack >= 0x2000) && (OverHalf == 1)){
					Lengthdev	= LengthStack - 0x4000;
					add_cal_n	= add_cal_dev;
					Lengthrem	= 0x4000 - (LengthStack - pSgList->List[i].length);
					LastList	= 1;
				}
				//--------------------------------------------
				// Specify the beginning of the next Descriptor Format in NextDescPtr
				//--------------------------------------------
				desc_cal	= (unsigned long long)(BM_MEM_INP_NXT_DESC + (0x40 * (i + ListMove + k + 1)));
				ApiMemBmDivideAddress(desc_cal, &desc_low_i_n, &desc_high_i_n);
				//--------------------------------------------
				// Set Descriptor Format to Prefetcher FIFO
				//--------------------------------------------
				_outmd((BM_MEM_INP_Read_SGAddr + (0x40 * (i + ListMove + k + 1))), add_low_i);
				_outmd((BM_MEM_INP_Read_SGAddr_high + (0x40 * (i + ListMove + k + 1))), add_high_i);
				_outmd((BM_MEM_INP_Write_SGAddr + (0x40 * (i + ListMove + k + 1))), ((sg_dma_address(&pSgList->List[i]) & ~dwdata) + (BM_MEM_CRA_SELECT * i)));
				_outmd((BM_MEM_INP_Write_SGAddr_high + (0x40 * (i + ListMove + k + 1))), (sg_dma_address(&pSgList->List[i]) >> 32 & 0xffffffff));
				_outmd((BM_MEM_INP_Length + (0x40 * (i + ListMove + k + 1))), pSgList->List[i].length);
				_outmd((BM_MEM_INP_NextDescPtr + (0x40 * (i + ListMove + k + 1))), desc_low_i_n);
				_outmd((BM_MEM_INP_NextDescPtr_high + (0x40 * (i + ListMove + k + 1))), desc_high_i_n);
				_outmw((BM_MEM_INP_SequenceNum + (0x40 * (i + ListMove + k + 1))), (i + ListMove + k + 1));
				_outmd((BM_MEM_INP_Stride + (0x40 * (i + ListMove + k + 1))), 0x0);
				_outmw((BM_MEM_INP_Burst + (0x40 * (i + ListMove + k + 1))), 0x101);
				_outmd((BM_MEM_INP_Descriptor + (0x40 * (i + ListMove + k + 1))), 0x40000000);
				//--------------------------------------------
				// Transfer setting of final transfer block
				//--------------------------------------------
				if ((LengthStack <= 0x2000) && (i == (ListSize - 1)) && (j == (LenNum - 1))){
					if ((LengthStack == 0x4000) && (OverHalf == 1)){
						add_cal_res	= (unsigned long long)BM_MEM_INP_ReadAdd;
					} else {
						add_cal_res	= (unsigned long long)(add_cal_n + pSgList->List[i].length);
					}
					desc_cal					= (unsigned long long)BM_MEM_INP_TOP_DESC;
					ApiMemBmDivideAddress(desc_cal, &desc_low_i_n, &desc_high_i_n);
					_outmd((BM_MEM_INP_NextDescPtr + (0x40 * (i + ListMove + k + 1))), desc_low_i_n);
					_outmd((BM_MEM_INP_NextDescPtr_high + (0x40 * (i + ListMove + k + 1))), desc_high_i_n);
					if (LengthStack == 0x4000){
						n++;
					}
					pMasAddr->BmInp.resetadd	= add_cal_res;
					UnderLen					= 1;
				}
				//--------------------------------------------
				// Divide one block into 2k bytes
				//--------------------------------------------
				if ((LengthStack == 0x2000) && (j != (LenNum - 1))){
					//--------------------------------------------
					// Store the next Descriptor start address
					//--------------------------------------------
					desc_cal					= (unsigned long long)(BM_MEM_INP_NXT_DESC + (0x40 * (i + ListMove + k)));
					ApiMemBmDivideAddress(desc_cal, &desc_low_i_n, &desc_high_i_n);
					pSgList->ChangeDescAdd[n]	= desc_low_i_n;
					//--------------------------------------------
					// Set FIFO start address to NextDescPtr
					//--------------------------------------------
					desc_cal					= (unsigned long long)BM_MEM_INP_TOP_DESC;
					ApiMemBmDivideAddress(desc_cal, &desc_low_i_n, &desc_high_i_n);
					_outmd((BM_MEM_INP_NextDescPtr + (0x40 * (i + ListMove + k + 1))), desc_low_i_n);
					_outmd((BM_MEM_INP_NextDescPtr_high + (0x40 * (i + ListMove + k + 1))), desc_high_i_n);
					LengthStack					= Lengthdev;
					ListNum						= 0;
					n++;
					if (OverHalf == 0){
						OverHalf	= 1;
					} else {
						OverHalf	= 0;
					}
				} else if (LengthStack > 0x2000){
					Lengthdev					= LengthStack - 0x2000;
					Lengthrem					= 0x2000 - (LengthStack - pSgList->List[i].length);
					_outmd((BM_MEM_INP_Length + (0x40 * (i + ListMove + k + 1))), Lengthrem);
					//--------------------------------------------
					// Store the next Descriptor start address
					//--------------------------------------------
					desc_cal					= (unsigned long long)(BM_MEM_INP_NXT_DESC + (0x40 * (i + ListMove + k)));
					ApiMemBmDivideAddress(desc_cal, &desc_low_i_n, &desc_high_i_n);
					pSgList->ChangeDescAdd[n]	= desc_low_i_n;
					//--------------------------------------------
					// Set current list as final list
					//--------------------------------------------
					desc_cal					= (unsigned long long)BM_MEM_INP_TOP_DESC;
					ApiMemBmDivideAddress(desc_cal, &desc_low_i_n, &desc_high_i_n);
					_outmd((BM_MEM_INP_NextDescPtr + (0x40 * (i + ListMove + k + 1))), desc_low_i_n);
					_outmd((BM_MEM_INP_NextDescPtr_high + (0x40 * (i + ListMove + k + 1))), desc_high_i_n);
					//--------------------------------------------
					// Descriptor Store parameters for reconfiguring
					//--------------------------------------------
					if ((j == (LenNum - 1)) && (pMasAddr->BmInp.ringstartflag == 0)){
						if (OverHalf == 0){
							add_cal								= (unsigned long long)(add_cal_n + Lengthrem);
							ApiMemBmDivideAddress(add_cal, &add_low_i, &add_high_i);
							pSgList->MemParam.FIFOOverAdd		= add_low_i;
							pSgList->MemParam.FIFOOverAdd_high	= add_high_i;
						} else {
							pSgList->MemParam.FIFOOverAdd		= add_low_i_dev;
							pSgList->MemParam.FIFOOverAdd_high	= add_high_i_dev;
						}
						pSgList->MemParam.SGOverAdd			= (((sg_dma_address(&pSgList->List[i]) + (pSgList->List[i].length - Lengthdev)) & ~dwdata) + (BM_MEM_CRA_SELECT * i));
						pSgList->MemParam.SGOverAdd_high	= (sg_dma_address(&pSgList->List[i]) >> 32 & 0xffffffff);
						pSgList->MemParam.OverLen			= Lengthdev;
						pSgList->MemParam.ATNumber			= i;
						if (OverHalf == 0){
							OverHalf	= 1;
						} else {
							OverHalf	= 0;
						}
					} else {
						//--------------------------------------------
						// Specify the beginning of the next Descriptor Format in NextDescPtr
						//--------------------------------------------
						ListMove++;
						desc_cal	= (unsigned long long)(BM_MEM_INP_NXT_DESC + (0x40 * (i + ListMove + k + 1)));
						ApiMemBmDivideAddress(desc_cal, &desc_low_i_n, &add_high_i);
						//--------------------------------------------
						// Set Descriptor Format to Prefetcher FIFO
						//--------------------------------------------
						_outmd((BM_MEM_INP_Read_SGAddr + (0x40 * (i + ListMove + k + 1))), add_low_i_dev);
						_outmd((BM_MEM_INP_Read_SGAddr_high + (0x40 * (i + ListMove + k + 1))), add_high_i_dev);
						_outmd((BM_MEM_INP_Write_SGAddr + (0x40 * (i + ListMove + k + 1))), (((sg_dma_address(&pSgList->List[i]) + (pSgList->List[i].length - Lengthdev)) & ~dwdata) + (BM_MEM_CRA_SELECT * i)));
						_outmd((BM_MEM_INP_Write_SGAddr_high + (0x40 * (i + ListMove + k + 1))), (sg_dma_address(&pSgList->List[i]) >> 32 & 0xffffffff));
						_outmd((BM_MEM_INP_Length + (0x40 * (i + ListMove + k + 1))), Lengthdev);
						_outmd((BM_MEM_INP_NextDescPtr + (0x40 * (i + ListMove + k + 1))), desc_low_i_n);
						_outmd((BM_MEM_INP_NextDescPtr_high + (0x40 * (i + ListMove + k + 1))), desc_high_i_n);
						_outmw((BM_MEM_INP_SequenceNum + (0x40 * (i + ListMove + k + 1))), (i + ListMove + k +1));
						_outmd((BM_MEM_INP_Stride + (0x40 * (i + ListMove + k + 1))), 0x0);
						_outmw((BM_MEM_INP_Burst + (0x40 * (i + ListMove + k + 1))), 0x101);
						_outmd((BM_MEM_INP_Descriptor + (0x40 * (i + ListMove + k + 1))), 0x40000000);
						if (OverHalf == 0){
							//--------------------------------------------
							// Set FIFO address for next list
							//--------------------------------------------
							add_cal		= (unsigned long long)(add_cal_n + Lengthrem);
							ApiMemBmDivideAddress(desc_cal, &desc_low_i_n, &add_high_i);
							_outmd((BM_MEM_INP_Read_SGAddr + (0x40 * (i + ListMove + k + 1))), add_low_i);
							_outmd((BM_MEM_INP_Read_SGAddr_high + (0x40 * (i + ListMove + k + 1))), add_high_i);
							SetDesc		= 0;
							OverHalf	= 1;
						} else if (OverHalf == 1){
							//--------------------------------------------
							// Move FIFO address for next list
							//--------------------------------------------
							add_cal		= (unsigned long long)(add_cal_dev + Lengthdev);
							SetDesc		= 1;
							OverHalf	= 0;
						}
						//--------------------------------------------
						// Transfer setting of final transfer block
						//--------------------------------------------
						if (i == (ListSize - 1)){
							//--------------------------------------------
							// Set NextDescPtr to the beginning of Prefetcher
							//--------------------------------------------
							desc_cal					= (unsigned long long)BM_MEM_INP_TOP_DESC;
							ApiMemBmDivideAddress(desc_cal, &desc_low_i_n, &add_high_i);
							add_cal_res					= (unsigned long long)(add_cal_n + pSgList->List[i].length);
							pMasAddr->BmInp.resetadd	= add_cal_res;
							_outmd((BM_MEM_INP_NextDescPtr + (0x40 * (i + ListMove + k + 1))), desc_low_i_n);
							_outmd((BM_MEM_INP_NextDescPtr_high + (0x40 * (i + ListMove + k + 1))), desc_high_i_n);
						}
					}
					LengthStack	= Lengthdev;
					ListNum		= 1;
					n++;
				}
			}else{
				if (j == 0){
					//--------------------------------------------
					// Set the Address Translation Table
					//--------------------------------------------
					if (pMasAddr->add64flag == 1){
						_outmd((BM_MEM_CRA + (0x8 * (i + BM_MEM_AT_OFFSET))), 0xFFFFFFFD);
						dwdata		= _inmd(BM_MEM_CRA + (0x8 * (i + BM_MEM_AT_OFFSET)));
						_outmd((BM_MEM_CRA + (0x8 * (i + BM_MEM_AT_OFFSET))), ((dwdata & sg_dma_address(&pSgList->List[i])) | 0x1));
						_outmd((BM_MEM_CRA_HIGH + (0x8 * (i + BM_MEM_AT_OFFSET))), (sg_dma_address(&pSgList->List[i]) >> 32 & 0xffffffff));
						cradata		= _inmd(BM_MEM_CRA + (0x8 * (i + BM_MEM_AT_OFFSET)));
						cradata2	= _inmd(BM_MEM_CRA_HIGH + (0x8 * (i + BM_MEM_AT_OFFSET)));
					} else {
						_outmd((BM_MEM_CRA + (0x8 * (i + BM_MEM_AT_OFFSET))), 0xFFFFFFFC);
						dwdata		= _inmd(BM_MEM_CRA + (0x8 * (i + BM_MEM_AT_OFFSET)));
						_outmd((BM_MEM_CRA + (0x8 * (i + BM_MEM_AT_OFFSET))), (dwdata & sg_dma_address(&pSgList->List[i])));
						_outmd((BM_MEM_CRA_HIGH + (0x8 * (i + BM_MEM_AT_OFFSET))), (sg_dma_address(&pSgList->List[i]) >> 32 & 0xffffffff));
						cradata		= _inmd(BM_MEM_CRA + (0x8 * (i + BM_MEM_AT_OFFSET)));
						cradata2	= _inmd(BM_MEM_CRA_HIGH + (0x8 * (i + BM_MEM_AT_OFFSET)));
					}
				}
				LengthStack	+= pSgList->List[i].length;
				//--------------------------------------------
				// Move FIFO address for second and subsequent lists
				//--------------------------------------------
				if ((i > 0) && (j == 0)){
					if (SetDesc == 1){
						SetDesc	= 0;
					} else {
						add_cal	= (unsigned long long)(add_cal_n + pSgList->List[i - 1].length);
					}
					
				} else if (j > 0){
					if (SetDesc == 1){
						SetDesc	= 0;
					} else {
						if (i > 0){
							add_cal	= (unsigned long long)(add_cal_n + pSgList->List[i - 1].length);
						} else {
							add_cal	= (unsigned long long)(add_cal_n + pSgList->List[ListSize - 1].length);
						}
					}
				}
				ApiMemBmDivideAddress(add_cal, &add_low_o, &add_high_o);
				add_cal_n	= add_cal;
				//--------------------------------------------
				// Set the beginning of FIFO when block exceeds 4kByte boundary
				//--------------------------------------------
				if ((LengthStack >= 0x2000) && (OverHalf == 1)){
					Lengthdev	= LengthStack - 0x2000;
					add_cal_n	= add_cal_dev;
					Lengthrem	= 0x2000 - (LengthStack - pSgList->List[i].length);
				}
				//--------------------------------------------
				// Specify the beginning of the next Descriptor Format in NextDescPtr
				//--------------------------------------------
				desc_cal	= (unsigned long long)(BM_MEM_OUT_NXT_DESC + (0x40 * (i + ListMove + k + 1)));
				ApiMemBmDivideAddress(desc_cal, &desc_low_o_n, &desc_high_o_n);
				//--------------------------------------------
				// Set Descriptor Format to Prefetcher FIFO
				//--------------------------------------------
				_outmd((BM_MEM_OUT_Write_SGAddr + (0x40 * (i + ListMove + k + 1))), add_low_o);
				_outmd((BM_MEM_OUT_Write_SGAddr_high + (0x40 * (i + ListMove + k + 1))), add_high_o);
				_outmd((BM_MEM_OUT_Read_SGAddr + (0x40 * (i + ListMove + k + 1))), ((sg_dma_address(&pSgList->List[i]) & ~dwdata) + (BM_MEM_CRA_SELECT * (i + BM_MEM_AT_OFFSET))));
				_outmd((BM_MEM_OUT_Read_SGAddr_high + (0x40 * (i + ListMove + k + 1))), (sg_dma_address(&pSgList->List[i]) >> 32 & 0xffffffff));
				_outmd((BM_MEM_OUT_Length + (0x40 * (i + ListMove + k + 1))), pSgList->List[i].length);
				_outmd((BM_MEM_OUT_NextDescPtr + (0x40 * (i + ListMove + k + 1))), desc_low_o_n);
				_outmd((BM_MEM_OUT_NextDescPtr_high + (0x40 * (i + ListMove + k + 1))), desc_high_o_n);
				_outmw((BM_MEM_OUT_SequenceNum + (0x40 * (i + ListMove + k + 1))), (i + ListMove + k + 1));
				_outmd((BM_MEM_OUT_Stride + (0x40 * (i + ListMove + k + 1))), 0x0);
				_outmw((BM_MEM_OUT_Burst + (0x40 * (i + ListMove + k + 1))), 0x101);
				_outmd((BM_MEM_OUT_Descriptor + (0x40 * (i + ListMove + k + 1))), 0x40000000);
				//--------------------------------------------
				// Transfer setting of final transfer block
				//--------------------------------------------
				if ((i == (ListSize - 1)) && (j == (LenNum - 1))){
					_outmd((BM_MEM_OUT_Descriptor + (0x40 * (i + ListMove + k + 1))), 0x40004000);
					if ((LengthStack == 0x2000) && (OverHalf == 1)){
						add_cal_res	= (unsigned long long)(BM_MEM_OUT_WriteAdd);
					} else {
						add_cal_res	= (unsigned long long)(add_cal_n + pSgList->List[i].length);
					}
					//--------------------------------------------
					// Set NextDescPtr to the beginning of Prefetcher
					//--------------------------------------------
					desc_cal					= (unsigned long long)BM_MEM_OUT_TOP_DESC;
					ApiMemBmDivideAddress(desc_cal, &desc_low_o_n, &desc_high_o_n);
					_outmd((BM_MEM_OUT_NextDescPtr + (0x40 * (i + ListMove + k + 1))), desc_low_o_n);
					_outmd((BM_MEM_OUT_NextDescPtr_high + (0x40 * (i + ListMove + k + 1))), desc_high_o_n);
					if (LengthStack == 0x2000){
						n++;
					}
					pMasAddr->BmOut.resetadd	= add_cal_res;
					//--------------------------------------------
					// Descriptor Store parameters for reconfiguring
					//--------------------------------------------
					if (OverFIFO == 1){
						add_cal		= (unsigned long long)(add_cal_n + pSgList->List[i].length);
						ApiMemBmDivideAddress(add_cal, &add_low_o, &add_high_o);
						pSgList->MemParam.FIFOOverAdd		= add_low_o;
						pSgList->MemParam.FIFOOverAdd_high	= add_high_o;
						pSgList->MemParam.SGOverAdd			= sg_dma_address(&pSgList->List[0]);
						pSgList->MemParam.SGOverAdd_high	= (sg_dma_address(&pSgList->List[0]) >> 32 & 0xffffffff);
						pSgList->MemParam.OverLen			= LengthStack;
						pSgList->MemParam.ATNumber			= 0;
					}
				}
				//--------------------------------------------
				// Divide one block into 2k bytes
				//--------------------------------------------
				if (LengthStack == 0x2000){
					if ((LengthStack == 0x2000) && (j != (LenNum - 1))){
						//--------------------------------------------
						// Store the next Descriptor start address
						//--------------------------------------------
						desc_cal					= (unsigned long long)(BM_MEM_OUT_NXT_DESC + (0x40 * (i + ListMove + k)));
						desc_low_o_n				= (unsigned long)(desc_cal & 0xffffffff);
						ApiMemBmDivideAddress(desc_cal, &desc_low_o_n, &desc_high_o_n);
						pSgList->ChangeDescAdd[n]	= desc_low_o_n;
						if (DummySet == 1){
							DummySet	= 0;
						} else {
							//--------------------------------------------
							// Set current list as final list
							//--------------------------------------------
							desc_cal	= (unsigned long long)BM_MEM_OUT_TOP_DESC;
							ApiMemBmDivideAddress(desc_cal, &desc_low_o_n, &desc_high_o_n);
							_outmd((BM_MEM_OUT_NextDescPtr + (0x40 * (i + ListMove + k + 1))), desc_low_o_n);
							_outmd((BM_MEM_OUT_NextDescPtr_high + (0x40 * (i + ListMove + k + 1))), desc_high_o_n);
						}
						LengthStack					= Lengthdev;
						ListNum						= 0;
						n++;
						if (OverHalf == 0){
							OverHalf	= 1;
						} else {
							OverHalf	= 0;
						}
					}
				} else if (LengthStack > 0x2000){
					Lengthdev					= LengthStack - 0x2000;
					Lengthrem					= 0x2000 - (LengthStack - pSgList->List[i].length);
					_outmd((BM_MEM_OUT_Length + (0x40 * (i + ListMove + k + 1))), Lengthrem);
					//--------------------------------------------
					// Store the next Descriptor start address
					//--------------------------------------------
					desc_cal					= (unsigned long long)(BM_MEM_OUT_NXT_DESC + (0x40 * (i + ListMove + k)));
					ApiMemBmDivideAddress(desc_cal, &desc_low_o_n, &desc_high_o_n);
					pSgList->ChangeDescAdd[n]	= desc_low_o_n;
					if (DummySet == 1){
						DummySet	= 0;
					} else {
						//--------------------------------------------
						// Set current list as final list
						//--------------------------------------------
						desc_cal	= (unsigned long long)BM_MEM_OUT_TOP_DESC;
						ApiMemBmDivideAddress(desc_cal, &desc_low_o_n, &desc_high_o_n);
						_outmd((BM_MEM_OUT_NextDescPtr + (0x40 * (i + ListMove + k + 1))), desc_low_o_n);
						_outmd((BM_MEM_OUT_NextDescPtr_high + (0x40 * (i + ListMove + k + 1))), desc_high_o_n);
						_outmd((BM_MEM_OUT_Descriptor + (0x40 * (i + ListMove + k + 1))), 0x40004000);
					}
					//--------------------------------------------
					// Transfer setting of final transfer block
					//--------------------------------------------
					if ((OverHalf == 1) && (j == (LenNum - 1))){
						//--------------------------------------------
						// Descriptor Store parameters for reconfiguring
						//--------------------------------------------
						if (OverHalf == 0){
							add_cal		= (unsigned long long)(add_cal_n + Lengthrem);
							ApiMemBmDivideAddress(add_cal, &add_low_o, &add_high_o);
							pSgList->MemParam.FIFOOverAdd		= add_low_o;
							pSgList->MemParam.FIFOOverAdd_high	= add_high_o;
						} else {
							pSgList->MemParam.FIFOOverAdd		= add_low_o_dev;
							pSgList->MemParam.FIFOOverAdd_high	= add_high_o_dev;
						}
						pSgList->MemParam.SGOverAdd			= (((sg_dma_address(&pSgList->List[i]) + (pSgList->List[i].length - Lengthdev)) & ~dwdata) + (BM_MEM_CRA_SELECT * (i + BM_MEM_AT_OFFSET)));
						pSgList->MemParam.SGOverAdd_high	= (sg_dma_address(&pSgList->List[i]) >> 32 & 0xffffffff);
						pSgList->MemParam.OverLen			= Lengthdev;
						pSgList->MemParam.ATNumber			= i;
						LastList							= 1;
						if (OverHalf == 0){
							OverHalf	= 1;
						} else {
							OverHalf	= 0;
						}
					} else {
						//--------------------------------------------
						// Specify the beginning of the next Descriptor Format in NextDescPtr
						//--------------------------------------------
						ListMove++;
						desc_cal	= (unsigned long long)(BM_MEM_OUT_NXT_DESC + (0x40 * (i + ListMove + k + 1)));
						ApiMemBmDivideAddress(desc_cal, &desc_low_o_n, &desc_high_o_n);
						//--------------------------------------------
						// Set Descriptor Format to Prefetcher FIFO
						//--------------------------------------------
						_outmd((BM_MEM_OUT_Write_SGAddr + (0x40 * (i + ListMove + k + 1))), add_low_o_dev);
						_outmd((BM_MEM_OUT_Write_SGAddr_high + (0x40 * (i + ListMove + k + 1))), add_high_o_dev);
						_outmd((BM_MEM_OUT_Read_SGAddr + (0x40 * (i + ListMove + k + 1))), (((sg_dma_address(&pSgList->List[i]) + (pSgList->List[i].length - Lengthdev)) & ~dwdata) + (BM_MEM_CRA_SELECT * (i + BM_MEM_AT_OFFSET))));
						_outmd((BM_MEM_OUT_Read_SGAddr_high + (0x40 * (i + ListMove + k + 1))), (sg_dma_address(&pSgList->List[i]) >> 32 & 0xffffffff));
						_outmd((BM_MEM_OUT_Length + (0x40 * (i + ListMove + k + 1))), Lengthdev);
						_outmd((BM_MEM_OUT_NextDescPtr + (0x40 * (i + ListMove + k + 1))), desc_low_o_n);
						_outmd((BM_MEM_OUT_NextDescPtr_high + (0x40 * (i + ListMove + k + 1))), desc_high_o_n);
						_outmw((BM_MEM_OUT_SequenceNum + (0x40 * (i + ListMove + k + 1))), (i + ListMove + k +1));
						_outmd((BM_MEM_OUT_Stride + (0x40 * (i + ListMove + k + 1))), 0x0);
						_outmw((BM_MEM_OUT_Burst + (0x40 * (i + ListMove + k + 1))), 0x101);
						_outmd((BM_MEM_OUT_Descriptor + (0x40 * (i + ListMove + k + 1))), 0x40000000);
						if (OverHalf == 0){
							//--------------------------------------------
							// Set FIFO address for next list
							//--------------------------------------------
							add_cal		= (unsigned long long)(add_cal_n + Lengthrem);
							ApiMemBmDivideAddress(add_cal, &add_low_o, &add_high_o);
							_outmd((BM_MEM_OUT_Write_SGAddr + (0x40 * (i + ListMove + k + 1))), add_low_o);
							_outmd((BM_MEM_OUT_Write_SGAddr_high + (0x40 * (i + ListMove + k + 1))), add_high_o);
							SetDesc		= 0;
							OverHalf	= 1;
						} else if (OverHalf == 1){
							//--------------------------------------------
							// Move FIFO address for next list
							//--------------------------------------------
							add_cal		= (unsigned long long)(add_cal_dev + Lengthdev);
							SetDesc		= 1;
							OverHalf	= 0;
						}
					}
					LengthStack	= Lengthdev;
					ListNum		= 1;
					n++;
				}
			}
			Outlen	+= pSgList->List[i].length;
			if (LastList == 1){
				break;
			}
		}
		k	+= i;
		if (LastList == 1){
				break;
		}
	}
	//============================================
	// Set the descriptor for end discrimination at the beginning of the Descriptor List
	//============================================
	if(dwDir == BM_DIR_IN){
		//--------------------------------------------
		// Specify the beginning of prefetcher FIFO to NextDescPtr
		//--------------------------------------------
		desc_cal						= (unsigned long long)BM_MEM_INP_NXT_DESC;
		ApiMemBmDivideAddress(desc_cal, &desc_low_i_n, &desc_high_i_n);
		//--------------------------------------------
		// Set Descriptor Format to Prefetcher FIFO
		//--------------------------------------------
		_outmd((BM_MEM_INP_Read_SGAddr), 0x0);
		_outmd((BM_MEM_INP_Read_SGAddr_high), 0x0);
		_outmd((BM_MEM_INP_Write_SGAddr), 0x0);
		_outmd((BM_MEM_INP_Write_SGAddr_high), 0x0);
		_outmd((BM_MEM_INP_Length), 0x0);
		_outmd((BM_MEM_INP_NextDescPtr), desc_low_i_n);
		_outmd((BM_MEM_INP_NextDescPtr_high), desc_high_i_n);
		_outmw((BM_MEM_INP_SequenceNum), 0x0);
		_outmd((BM_MEM_INP_Stride), 0x0);
		_outmw((BM_MEM_INP_Burst), 0x101);
		_outmd((BM_MEM_INP_Descriptor), 0x0);
		//--------------------------------------------
		// Descriptor Store parameters for reconfiguring
		//--------------------------------------------
		pMasAddr->BmInp.resetchangedesc	= 0;
		pMasAddr->BmInp.atstartlen		= 0;
		pMasAddr->BmInp.resetOverHalf	= OverHalf;
		pMasAddr->BmInp.resetListMove	= ListMove;
		if (pSgList->nr_pages > 150){
			pMasAddr->BmInp.resetListSize	= pSgList->nr_pages;
		} else {
			//--------------------------------------------
			// Store the next Descriptor start address
			//--------------------------------------------
			if (UnderLen == 1){
				desc_cal					= (unsigned long long)(BM_MEM_INP_NXT_DESC + (0x40 * (i + ListMove - 2)));
				ApiMemBmDivideAddress(desc_cal, &desc_low_i_n, &desc_high_i_n);
				pSgList->ChangeDescAdd[n]	= desc_low_i_n;
				add_cal						= (unsigned long long)(add_cal_n + pSgList->List[ListSize - 1].length);
				ApiMemBmDivideAddress(add_cal, &add_low_i, &add_high_i);
			}
		}
		if (pMasAddr->BmInp.sglen < 50){
			pMasAddr->BmInp.resetListMove	= 0;
			pSgList->ListNumCnt				= 0;
		} else {
			pMasAddr->BmInp.resetListMove	= ListMove;
			pSgList->ListNumCnt				= i * j;
		}
	}else {
		//--------------------------------------------
		// Specify the beginning of prefetcher FIFO to NextDescPtr
		//--------------------------------------------
		desc_cal		= (unsigned long long)BM_MEM_OUT_NXT_DESC;
		ApiMemBmDivideAddress(desc_cal, &desc_low_o_n, &desc_high_o_n);
		//--------------------------------------------
		// Set Descriptor Format to Prefetcher FIFO
		//--------------------------------------------
		_outmd((BM_MEM_OUT_Write_SGAddr), 0x0);
		_outmd((BM_MEM_OUT_Write_SGAddr_high), 0x0);
		_outmd((BM_MEM_OUT_Read_SGAddr), 0x0);
		_outmd((BM_MEM_OUT_Read_SGAddr_high), 0x0);
		_outmd((BM_MEM_OUT_Length), 0x0);
		_outmd((BM_MEM_OUT_NextDescPtr), desc_low_o_n);
		_outmd((BM_MEM_OUT_NextDescPtr_high), desc_high_o_n);
		_outmw((BM_MEM_OUT_SequenceNum), 0x0);
		_outmd((BM_MEM_OUT_Stride), 0x0);
		_outmw((BM_MEM_OUT_Burst), 0x101);
		_outmd((BM_MEM_OUT_Descriptor), 0x0);
		//--------------------------------------------
		// Descriptor Store parameters for reconfiguring
		//--------------------------------------------
		pMasAddr->BmOut.resetchangedesc	= 0;
		//--------------------------------------------
		// Store the next Descriptor start address
		//--------------------------------------------
		if (LengthStack < 0x2000){
			desc_cal					= (unsigned long long)(BM_MEM_OUT_NXT_DESC + (0x40 * (i + ListMove - 2)));
			ApiMemBmDivideAddress(desc_cal, &desc_low_o_n, &desc_high_o_n);
			pSgList->ChangeDescAdd[n]	= desc_low_o_n;
			add_cal						= (unsigned long long)(add_cal_n + pSgList->List[ListSize - 1].length);
			ApiMemBmDivideAddress(add_cal, &add_low_o, &add_high_o);
		}
		pMasAddr->BmOut.atstartlen		= 0;
		pMasAddr->BmOut.resetOverHalf	= OverHalf;
		if (pMasAddr->BmOut.sglen < 50){
			pMasAddr->BmOut.resetListMove	= 0;
		} else {
			pMasAddr->BmOut.resetListMove	= ListMove;
		}
		if (pMasAddr->BmOut.sglen < 50){
			pSgList->ListNumCnt	= 0;
		} else {
			pSgList->ListNumCnt	= i * j;
		}
	}
	pSgList->ReSetArray	= n - 1;
	pSgList->ATOverCnt	= 0;

	return	BM_ERROR_SUCCESS;
}

//========================================================================
// Function name  : MemBmSetSGAddressRingPreLittle
// Function		  : Setting S/G list when buffer size is less than specific size. (Infinite transfer)
// I/F	 	  	  : Internal
// In	 		  : pMasAddr			: Point to the MASTERADDR structure
//		   		 	dwDir				: BM_DIR_IN / BM_DIR_OUT
// Out			  : None
// Return value   : Normal completed	: BM_ERROR_SUCCESS
//		   			Abnormal completed	: BM_ERROR_PARAM / BM_ERROR_BUFF
// Addition  	  : 
//========================================================================
long MemBmSetSGAddressRingPreLittle(PMASTERADDR pMasAddr, unsigned long dwDir)
{
	unsigned long		i = 0;
	PSGLIST 			pSgList;
	unsigned long long	desc_cal;
	unsigned long long	add_cal, add_cal_n;
	unsigned long long	add_cal_res;
	unsigned long long	add_cal_dev;
	unsigned long		dwdata = 0;
	unsigned long		desc_low_o_n, desc_high_o_n;
	unsigned long		desc_low_i_n, desc_high_i_n;
	unsigned long		cradata, cradata2;
	unsigned long		add_low_i, add_high_i;
	unsigned long		add_low_o, add_high_o;
	unsigned long		add_low_i_dev, add_high_i_dev;
	unsigned long		add_low_o_dev, add_high_o_dev;
	unsigned long		ListSize, ListNum;
	unsigned long		LengthStack, Lengthdev, Lengthrem;
	unsigned long		j, k, n;
	unsigned long		OverHalf, loop;
	unsigned long		Outlen;
	unsigned long		LenSize;
	unsigned long		ListMove, LenNum, HalfSize;
	int					SetDesc, DummySet;
	int					OverFIFO = 0;
	
	//--------------------------------------------
	// Checking parameter
	//--------------------------------------------
	if(dwDir != BM_DIR_IN && dwDir != BM_DIR_OUT){
		return	BM_ERROR_PARAM;
	}
	if(dwDir == BM_DIR_IN){
		pSgList	= &pMasAddr->BmInp.SgList;
		n		= 0;
	}else{
		pSgList	= &pMasAddr->BmOut.SgList;
		n		= pMasAddr->BmOut.resetchangedesc;
	}
	if(pSgList->nr_pages > BM_MEM_MAX_SG_SIZE){
		return	BM_ERROR_BUFF;
	}
	//---------------------------------------------
	// Setting FIFO address
	//---------------------------------------------
	if(dwDir & BM_DIR_IN){
		add_cal						= (unsigned long long)BM_MEM_INP_ReadAdd;
		add_cal_dev					= add_cal;
		ApiMemBmDivideAddress(add_cal_dev, &add_low_i_dev, &add_high_i_dev);
		pMasAddr->BmInp.addtablenum	= 0;
	} else {
		add_cal						= (unsigned long long)BM_MEM_OUT_WriteAdd;
		add_cal_dev					= add_cal;
		ApiMemBmDivideAddress(add_cal_dev, &add_low_o_dev, &add_high_o_dev);
		pMasAddr->BmOut.addtablenum	= 0;
	}
	add_cal_n	= add_cal;
	ListSize	= pSgList->nr_pages;
	//--------------------------------------------
	// If the number of SGList exceeds a certain number, limit the number of lists for reconfiguration
	//--------------------------------------------
	if(dwDir & BM_DIR_IN){
		OverFIFO	= 0;
		LenSize		= 0;
		for (loop = 0; loop < ListSize; loop++) {
			LenSize	+=  pMasAddr->BmInp.SgList.List[loop].length;
		}
		LenNum	= 0x4000 / LenSize;
		if ((0x4000 % LenSize) != 0){
			LenNum++;
		}
		if (LenNum > 150){
			LenNum		= 150;
			OverFIFO	= 1;
		}
	} else {
		OverFIFO	= 0;
		LenSize		= 0;
		for (loop = 0; loop < ListSize; loop++) {
			LenSize	+=  pMasAddr->BmOut.SgList.List[loop].length;
		}
		LenNum	= 0x4000 / LenSize;
		if ((0x4000 % LenSize) != 0){
			LenNum++;
		}
		if (LenNum > 150){
			LenNum		= 150;
			OverFIFO	= 1;
		}
	}
	HalfSize	= (LenNum / 2) * LenSize;
	LengthStack	= 0;
	k			= 0;
	ListMove	= 0;
	Lengthdev	= 0;
	Lengthrem	= 0;
	ListNum		= 0;
	SetDesc		= 0;
	DummySet	= 1;
	OverHalf	= 0;
	Outlen		= 0;
	//--------------------------------------------
	// Loop the number of times
	//--------------------------------------------
	for (j = 0; j < LenNum; j++) {
		//--------------------------------------------
		// Loop the number of times
		//--------------------------------------------
		for (i = 0; i < ListSize; i++) {
			ListNum++;
			//--------------------------------------------
			// Set in master register
			//--------------------------------------------
			if(dwDir == BM_DIR_IN){	
				if (j == 0){
					//--------------------------------------------
					// Set the Address Translation Table
					//--------------------------------------------
					if (pMasAddr->add64flag == 1){
						_outmd((BM_MEM_CRA + (0x8 * i)), 0xFFFFFFFD);
						dwdata		= _inmd(BM_MEM_CRA + (0x8 * i));
						_outmd((BM_MEM_CRA + (0x8 * i)), ((dwdata & sg_dma_address(&pSgList->List[i])) | 0x1));
						_outmd((BM_MEM_CRA_HIGH + (0x8 * i)), (sg_dma_address(&pSgList->List[i]) >> 32 & 0xffffffff));
						cradata		= _inmd(BM_MEM_CRA + (0x8 * i));
						cradata2	= _inmd(BM_MEM_CRA_HIGH + (0x8 * i));
					} else {
						_outmd((BM_MEM_CRA + (0x8 * i)), 0xFFFFFFFC);
						dwdata		= _inmd(BM_MEM_CRA + (0x8 * i));
						_outmd((BM_MEM_CRA + (0x8 * i)), (dwdata & sg_dma_address(&pSgList->List[i])));
						_outmd((BM_MEM_CRA_HIGH + (0x8 * i)), (sg_dma_address(&pSgList->List[i]) >> 32 & 0xffffffff));
						cradata		= _inmd(BM_MEM_CRA + (0x8 * i));
						cradata2	= _inmd(BM_MEM_CRA_HIGH + (0x8 * i));
					}
				}
				LengthStack	+= pSgList->List[i].length;
				//--------------------------------------------
				// Move FIFO address for second and subsequent lists
				//--------------------------------------------
				if ((i > 0) && (j == 0)){
					if (SetDesc == 1){
						SetDesc	= 0;
					} else {
						add_cal	= (unsigned long long)(add_cal_n + pSgList->List[i - 1].length);
					}
					
				} else if (j > 0){
					if (SetDesc == 1){
						SetDesc		= 0;
					} else {
						if (i > 0){
							add_cal	= (unsigned long long)(add_cal_n + pSgList->List[i - 1].length);
						} else {
							add_cal	= (unsigned long long)(add_cal_n + pSgList->List[ListSize - 1].length);
						}
					}
				}
				ApiMemBmDivideAddress(add_cal, &add_low_i, &add_high_i);
				add_cal_n	= add_cal;
				//--------------------------------------------
				// Set the beginning of FIFO when block exceeds 4kByte boundary
				//--------------------------------------------
				if (LengthStack >= 0x4000){
					Lengthdev	= LengthStack - HalfSize;
					add_cal_n	= add_cal_dev;
					Lengthrem	= HalfSize - (LengthStack - pSgList->List[i].length);
				}
				//--------------------------------------------
				// Specify the beginning of the next Descriptor Format in NextDescPtr
				//--------------------------------------------
				desc_cal	= (unsigned long long)(BM_MEM_INP_NXT_DESC + (0x40 * (i + ListMove + k + 1)));
				ApiMemBmDivideAddress(desc_cal, &desc_low_i_n, &desc_high_i_n);
				//--------------------------------------------
				// Set Descriptor Format to Prefetcher FIFO
				//--------------------------------------------
				_outmd((BM_MEM_INP_Read_SGAddr + (0x40 * (i + ListMove + k + 1))), add_low_i);
				_outmd((BM_MEM_INP_Read_SGAddr_high + (0x40 * (i + ListMove + k + 1))), add_high_i);
				_outmd((BM_MEM_INP_Write_SGAddr + (0x40 * (i + ListMove + k + 1))), ((sg_dma_address(&pSgList->List[i]) & ~dwdata) + (BM_MEM_CRA_SELECT * i)));
				_outmd((BM_MEM_INP_Write_SGAddr_high + (0x40 * (i + ListMove + k + 1))), (sg_dma_address(&pSgList->List[i]) >> 32 & 0xffffffff));
				_outmd((BM_MEM_INP_Length + (0x40 * (i + ListMove + k + 1))), pSgList->List[i].length);
				_outmd((BM_MEM_INP_NextDescPtr + (0x40 * (i + ListMove + k + 1))), desc_low_i_n);
				_outmd((BM_MEM_INP_NextDescPtr_high + (0x40 * (i + ListMove + k + 1))), desc_high_i_n);
				_outmw((BM_MEM_INP_SequenceNum + (0x40 * (i + ListMove + k + 1))), (i + ListMove + k + 1));
				_outmd((BM_MEM_INP_Stride + (0x40 * (i + ListMove + k + 1))), 0x0);
				_outmw((BM_MEM_INP_Burst + (0x40 * (i + ListMove + k + 1))), 0x101);
				_outmd((BM_MEM_INP_Descriptor + (0x40 * (i + ListMove + k + 1))), 0x40000000);
				//--------------------------------------------
				// Transfer setting of final transfer block
				//--------------------------------------------
				if ((LengthStack <= HalfSize) && (i == (ListSize - 1)) && (j == (LenNum - 1))){
					if ((LengthStack == HalfSize) && (OverHalf == 1)){
						add_cal_res	= (unsigned long long)BM_MEM_INP_ReadAdd;
					} else {
						add_cal_res	= (unsigned long long)(add_cal_n + pSgList->List[i].length);
					}
					//--------------------------------------------
					// Set current list as final list
					//--------------------------------------------
					desc_cal							= (unsigned long long)BM_MEM_INP_TOP_DESC;
					ApiMemBmDivideAddress(desc_cal, &desc_low_i_n, &desc_high_i_n);
					_outmd((BM_MEM_INP_NextDescPtr + (0x40 * (i + ListMove + k + 1))), desc_low_i_n);
					_outmd((BM_MEM_INP_NextDescPtr_high + (0x40 * (i + ListMove + k + 1))), desc_high_i_n);
					if (LengthStack == 0x2000){
						n++;
					}
					//--------------------------------------------
					// Descriptor Store parameters for reconfiguring
					//--------------------------------------------
					pMasAddr->BmInp.resetadd			= add_cal_res;
					add_cal								= (unsigned long long)(add_cal_n + pSgList->List[i].length);
					ApiMemBmDivideAddress(add_cal, &add_low_i, &add_high_i);
					pSgList->MemParam.FIFOOverAdd		= add_low_i;
					pSgList->MemParam.FIFOOverAdd_high	= add_high_i;
					pSgList->MemParam.SGOverAdd			= sg_dma_address(&pSgList->List[0]);
					pSgList->MemParam.SGOverAdd_high	= (sg_dma_address(&pSgList->List[0]) >> 32 & 0xffffffff);
					pSgList->MemParam.OverLen			= pSgList->List[0].length;
					pSgList->MemParam.ATNumber			= 0;
					if ((LengthStack == HalfSize) && (j == (LenNum - 1))){
						if (OverHalf == 0){
							OverHalf	= 1;
						} else {
							OverHalf	= 0;
						}
					}
				}
				//--------------------------------------------
				// Split transfer size of 1 block
				//--------------------------------------------
				if ((LengthStack == HalfSize) && (j != (LenNum - 1))){
					//--------------------------------------------
					// Store the next Descriptor start address
					//--------------------------------------------
					desc_cal					= (unsigned long long)(BM_MEM_INP_NXT_DESC + (0x40 * (i + ListMove + k)));
					ApiMemBmDivideAddress(desc_cal, &desc_low_i_n, &desc_high_i_n);
					pSgList->ChangeDescAdd[n]	= desc_low_i_n;
					//--------------------------------------------
					// Set NextDescPtr to the beginning address of Prefetcher FIFO
					//--------------------------------------------
					desc_cal					= (unsigned long long)BM_MEM_INP_TOP_DESC;
					ApiMemBmDivideAddress(desc_cal, &desc_low_i_n, &desc_high_i_n);
					_outmd((BM_MEM_INP_NextDescPtr + (0x40 * (i + ListMove + k + 1))), desc_low_i_n);
					_outmd((BM_MEM_INP_NextDescPtr_high + (0x40 * (i + ListMove + k + 1))), desc_high_i_n);
					LengthStack					= 0;
					ListNum						= 0;
					n++;
					if (OverHalf == 0){
						OverHalf	= 1;
					} else {
						OverHalf	= 0;
					}
				} else if (LengthStack > HalfSize){
					//--------------------------------------------
					// Split transfer size of 1 block
					//--------------------------------------------
					Lengthdev					= LengthStack - HalfSize;
					Lengthrem					= HalfSize - (LengthStack - pSgList->List[i].length);
					_outmd((BM_MEM_INP_Length + (0x40 * (i + ListMove + k + 1))), Lengthrem);
					//--------------------------------------------
					// Store the next Descriptor start address
					//--------------------------------------------
					desc_cal					= (unsigned long long)(BM_MEM_INP_NXT_DESC + (0x40 * (i + ListMove + k)));
					ApiMemBmDivideAddress(desc_cal, &desc_low_i_n, &desc_high_i_n);
					pSgList->ChangeDescAdd[n]	= desc_low_i_n;
					//--------------------------------------------
					// Set current list as final list
					//--------------------------------------------
					desc_cal					= (unsigned long long)BM_MEM_INP_TOP_DESC;
					ApiMemBmDivideAddress(desc_cal, &desc_low_i_n, &desc_high_i_n);
					_outmd((BM_MEM_INP_NextDescPtr + (0x40 * (i + ListMove + k + 1))), desc_low_i_n);
					_outmd((BM_MEM_INP_NextDescPtr_high + (0x40 * (i + ListMove + k + 1))), desc_high_i_n);
					//--------------------------------------------
					// Store parameters for final block
					//--------------------------------------------
					if ((i == (ListSize - 1)) && (j == (LenNum - 1))){
						//--------------------------------------------
						// Descriptor Store parameters for reconfiguring
						//--------------------------------------------
						add_cal								= (unsigned long long)(add_cal_n + Lengthrem);
						ApiMemBmDivideAddress(add_cal, &add_low_i, &add_high_i);
						pSgList->MemParam.FIFOOverAdd		= add_low_i;
						pSgList->MemParam.FIFOOverAdd_high	= add_high_i;
						pSgList->MemParam.SGOverAdd			= sg_dma_address(&pSgList->List[0]);
						pSgList->MemParam.SGOverAdd_high	= (sg_dma_address(&pSgList->List[0]) >> 32 & 0xffffffff);
						pSgList->MemParam.OverLen			= Lengthdev;
						pSgList->MemParam.ATNumber			= i;
						if (OverHalf == 0){
							OverHalf	= 1;
						} else {
							OverHalf	= 0;
						}
					} else {
						//--------------------------------------------
						// Specify the beginning of the next Descriptor Format in NextDescPtr
						//--------------------------------------------
						ListMove++;
						desc_cal	= (unsigned long long)(BM_MEM_INP_NXT_DESC + (0x40 * (i + ListMove + k + 1)));
						ApiMemBmDivideAddress(desc_cal, &desc_low_i_n, &desc_high_i_n);
						//--------------------------------------------
						// Set Descriptor Format to Prefetcher FIFO
						//--------------------------------------------
						_outmd((BM_MEM_INP_Read_SGAddr + (0x40 * (i + ListMove + k + 1))), add_low_i_dev);
						_outmd((BM_MEM_INP_Read_SGAddr_high + (0x40 * (i + ListMove + k + 1))), add_high_i_dev);
						_outmd((BM_MEM_INP_Write_SGAddr + (0x40 * (i + ListMove + k + 1))), (((sg_dma_address(&pSgList->List[i]) + (pSgList->List[i].length - Lengthdev)) & ~dwdata) + (BM_MEM_CRA_SELECT * i)));
						_outmd((BM_MEM_INP_Write_SGAddr_high + (0x40 * (i + ListMove + k + 1))), (sg_dma_address(&pSgList->List[i]) >> 32 & 0xffffffff));
						_outmd((BM_MEM_INP_Length + (0x40 * (i + ListMove + k + 1))), Lengthdev);
						_outmd((BM_MEM_INP_NextDescPtr + (0x40 * (i + ListMove + k + 1))), desc_low_i_n);
						_outmd((BM_MEM_INP_NextDescPtr_high + (0x40 * (i + ListMove + k + 1))), desc_high_i_n);
						_outmw((BM_MEM_INP_SequenceNum + (0x40 * (i + ListMove + k + 1))), (i + ListMove + k +1));
						_outmd((BM_MEM_INP_Stride + (0x40 * (i + ListMove + k + 1))), 0x0);
						_outmw((BM_MEM_INP_Burst + (0x40 * (i + ListMove + k + 1))), 0x101);
						_outmd((BM_MEM_INP_Descriptor + (0x40 * (i + ListMove + k + 1))), 0x40000000);
						if (OverHalf == 0){
							//--------------------------------------------
							// Set FIFO address for next list
							//--------------------------------------------
							add_cal		= (unsigned long long)(add_cal_n + Lengthrem);
							ApiMemBmDivideAddress(add_cal, &add_low_i, &add_high_i);
							_outmd((BM_MEM_INP_Read_SGAddr + (0x40 * (i + ListMove + k + 1))), add_low_i);
							_outmd((BM_MEM_INP_Read_SGAddr_high + (0x40 * (i + ListMove + k + 1))), add_high_i);
							SetDesc		= 0;
							OverHalf	= 1;
						} else if (OverHalf == 1){
							//--------------------------------------------
							// Move FIFO address for next list
							//--------------------------------------------
							add_cal		= (unsigned long long)(add_cal_dev + Lengthdev);
							SetDesc		= 1;
							OverHalf	= 0;
						}
						n++;
					}
					LengthStack	= Lengthdev;
					ListNum		= 1;
				}
			}else{
				if (j == 0){
					//--------------------------------------------
					// Set the Address Translation Table
					//--------------------------------------------
					if (pMasAddr->add64flag == 1){
						_outmd((BM_MEM_CRA + (0x8 * (i + BM_MEM_AT_OFFSET))), 0xFFFFFFFD);
						dwdata = _inmd(BM_MEM_CRA + (0x8 * (i + BM_MEM_AT_OFFSET)));
						_outmd((BM_MEM_CRA + (0x8 * (i + BM_MEM_AT_OFFSET))), ((dwdata & sg_dma_address(&pSgList->List[i])) | 0x1));
						_outmd((BM_MEM_CRA_HIGH + (0x8 * (i + BM_MEM_AT_OFFSET))), (sg_dma_address(&pSgList->List[i]) >> 32 & 0xffffffff));
						cradata = _inmd(BM_MEM_CRA + (0x8 * (i + BM_MEM_AT_OFFSET)));
						cradata2 = _inmd(BM_MEM_CRA_HIGH + (0x8 * i));
					} else {
						_outmd((BM_MEM_CRA + (0x8 * (i + BM_MEM_AT_OFFSET))), 0xFFFFFFFC);
						dwdata = _inmd(BM_MEM_CRA + (0x8 * (i + BM_MEM_AT_OFFSET)));
						_outmd((BM_MEM_CRA + (0x8 * (i + BM_MEM_AT_OFFSET))), (dwdata & sg_dma_address(&pSgList->List[i])));
						_outmd((BM_MEM_CRA_HIGH + (0x8 * (i + BM_MEM_AT_OFFSET))), (sg_dma_address(&pSgList->List[i]) >> 32 & 0xffffffff));
						cradata = _inmd(BM_MEM_CRA + (0x8 * (i + BM_MEM_AT_OFFSET)));
						cradata2 = _inmd(BM_MEM_CRA_HIGH + (0x8 * (i + BM_MEM_AT_OFFSET)));
					}
				}
				LengthStack	+= pSgList->List[i].length;
				//--------------------------------------------
				// Move FIFO address for second and subsequent lists
				//--------------------------------------------
				if ((i > 0) && (j == 0)){
					if (SetDesc == 1){
						SetDesc	= 0;
					} else {
						add_cal	= (unsigned long long)(add_cal_n + pSgList->List[i - 1].length);
					}
				} else if (j > 0){
					if (SetDesc == 1){
						SetDesc	= 0;
					} else {
						if (i > 0){
							add_cal	= (unsigned long long)(add_cal_n + pSgList->List[i - 1].length);
						} else {
							add_cal	= (unsigned long long)(add_cal_n + pSgList->List[ListSize - 1].length);
						}
					}
				}
				ApiMemBmDivideAddress(add_cal, &add_low_o, &add_high_o);
				add_cal_n	= add_cal;
				//--------------------------------------------
				// Set the beginning of FIFO when block exceeds 4kByte boundary
				//--------------------------------------------
				if (LengthStack >= 0x4000){
					Lengthdev	= LengthStack - 0x4000;
					add_cal_n	= add_cal_dev;
					Lengthrem	= 0x4000 - (LengthStack - pSgList->List[i].length);
				}
				//--------------------------------------------
				// Specify the beginning of the next Descriptor Format in NextDescPtr
				//--------------------------------------------
				desc_cal	= (unsigned long long)(BM_MEM_OUT_NXT_DESC + (0x40 * (i + ListMove + k + 1)));
				ApiMemBmDivideAddress(desc_cal, &desc_low_o_n, &desc_high_o_n);
				//--------------------------------------------
				// Transfer setting of final transfer block
				//--------------------------------------------
				_outmd((BM_MEM_OUT_Write_SGAddr + (0x40 * (i + ListMove + k + 1))), add_low_o);
				_outmd((BM_MEM_OUT_Write_SGAddr_high + (0x40 * (i + ListMove + k + 1))), add_high_o);
				_outmd((BM_MEM_OUT_Read_SGAddr + (0x40 * (i + ListMove + k + 1))), ((sg_dma_address(&pSgList->List[i]) & ~dwdata) + (BM_MEM_CRA_SELECT * (i + BM_MEM_AT_OFFSET))));
				_outmd((BM_MEM_OUT_Read_SGAddr_high + (0x40 * (i + ListMove + k + 1))), (sg_dma_address(&pSgList->List[i]) >> 32 & 0xffffffff));
				_outmd((BM_MEM_OUT_Length + (0x40 * (i + ListMove + k + 1))), pSgList->List[i].length);
				_outmd((BM_MEM_OUT_NextDescPtr + (0x40 * (i + ListMove + k + 1))), desc_low_o_n);
				_outmd((BM_MEM_OUT_NextDescPtr_high + (0x40 * (i + ListMove + k + 1))), desc_high_o_n);
				_outmw((BM_MEM_OUT_SequenceNum + (0x40 * (i + ListMove + k + 1))), (i + ListMove + k + 1));
				_outmd((BM_MEM_OUT_Stride + (0x40 * (i + ListMove + k + 1))), 0x0);
				_outmw((BM_MEM_OUT_Burst + (0x40 * (i + ListMove + k + 1))), 0x101);
				_outmd((BM_MEM_OUT_Descriptor + (0x40 * (i + ListMove + k + 1))), 0x40000000);
				//--------------------------------------------
				// If the block exceeds the 4kByte boundary, reconfigure the Descriptor
				//---------------------------------------------
				if ((LengthStack <= 0x4000) && (i == (ListSize - 1)) && (j == (LenNum - 1))){
					_outmd((BM_MEM_OUT_Descriptor + (0x40 * (i + ListMove + k + 1))), 0x40004000);
					if ((LengthStack == 0x4000) && (OverHalf == 1)){
						add_cal_res	= (unsigned long long)BM_MEM_OUT_WriteAdd;
					} else {
						add_cal_res	= (unsigned long long)(add_cal_n + pSgList->List[i].length);
					}
					//--------------------------------------------
					// Set NextDescPtr to the beginning address of Prefetcher FIFO
					//--------------------------------------------
					desc_cal	= (unsigned long long)BM_MEM_OUT_TOP_DESC;
					ApiMemBmDivideAddress(desc_cal, &desc_low_o_n, &desc_high_o_n);
					_outmd((BM_MEM_OUT_NextDescPtr + (0x40 * (i + ListMove + k + 1))), desc_low_o_n);
					_outmd((BM_MEM_OUT_NextDescPtr_high + (0x40 * (i + ListMove + k + 1))), desc_high_o_n);
					if (LengthStack == 0x4000){
						n++;
					}
					//--------------------------------------------
					// Descriptor Store parameters for reconfiguring
					//--------------------------------------------
					pMasAddr->BmOut.resetadd			= add_cal_res;
					add_cal								= (unsigned long long)(add_cal_n + pSgList->List[i].length);
					add_low_o							= (unsigned long)(add_cal & 0xffffffff);
					add_high_o							= (unsigned long)((add_cal & 0xffffffff00000000LL) >> 32);
					pSgList->MemParam.FIFOOverAdd		= add_low_o;
					pSgList->MemParam.FIFOOverAdd_high	= add_high_o;
					pSgList->MemParam.SGOverAdd			= sg_dma_address(&pSgList->List[0]);
					pSgList->MemParam.SGOverAdd_high	= (sg_dma_address(&pSgList->List[0]) >> 32 & 0xffffffff);
					pSgList->MemParam.OverLen			= pSgList->List[0].length;
					pSgList->MemParam.ATNumber			= 0;
				}
				//--------------------------------------------
				// Split transfer size of 1 block
				//--------------------------------------------
				if ((LengthStack == 0x4000) && (j != (LenNum - 1))){
					if (DummySet == 1){
						DummySet	= 0;
					} else {
						desc_cal	= (unsigned long long)BM_MEM_OUT_TOP_DESC;
						ApiMemBmDivideAddress(desc_cal, &desc_low_o_n, &desc_high_o_n);
						_outmd((BM_MEM_OUT_NextDescPtr + (0x40 * (i + ListMove + k + 1))), desc_low_o_n);
						_outmd((BM_MEM_OUT_NextDescPtr_high + (0x40 * (i + ListMove + k + 1))), desc_high_o_n);
					}
					LengthStack	= Lengthdev;
					ListNum		= 0;
					n++;
					if (OverHalf == 0){
						OverHalf	= 1;
					} else {
						OverHalf	= 0;
					}
				} else if (LengthStack > 0x4000){
					Lengthdev					= LengthStack - 0x4000;
					Lengthrem					= 0x4000 - (LengthStack - pSgList->List[i].length);
					_outmd((BM_MEM_OUT_Length + (0x40 * (i + ListMove + k + 1))), Lengthrem);
					//--------------------------------------------
					// Store the next Descriptor start address
					//--------------------------------------------
					desc_cal					= (unsigned long long)(BM_MEM_OUT_NXT_DESC + (0x40 * (i + ListMove + k)));
					ApiMemBmDivideAddress(desc_cal, &desc_low_o_n, &desc_high_o_n);
					pSgList->ChangeDescAdd[n]	= desc_low_o_n;
					if (DummySet == 1){
						DummySet	= 0;
					} else {
						//--------------------------------------------
						// Set current list as final list
						//--------------------------------------------
						desc_cal	= (unsigned long long)BM_MEM_OUT_TOP_DESC;
						ApiMemBmDivideAddress(desc_cal, &desc_low_o_n, &desc_high_o_n);
						_outmd((BM_MEM_OUT_NextDescPtr + (0x40 * (i + ListMove + k + 1))), desc_low_o_n);
						_outmd((BM_MEM_OUT_NextDescPtr_high + (0x40 * (i + ListMove + k + 1))), desc_high_o_n);
						_outmd((BM_MEM_OUT_Descriptor + (0x40 * (i + ListMove + k + 1))), 0x40004000);
					}
					//--------------------------------------------
					// Transfer setting of final transfer block
					//--------------------------------------------
					if (j == (LenNum - 1)){
						//--------------------------------------------
						// Descriptor Store parameters for reconfiguring
						//--------------------------------------------
						if (OverHalf == 0){
							add_cal								= (unsigned long long)(add_cal_n + Lengthrem);
							ApiMemBmDivideAddress(add_cal, &add_low_o, &add_high_o);
							pSgList->MemParam.FIFOOverAdd		= add_low_o;
							pSgList->MemParam.FIFOOverAdd_high	= add_high_o;
						} else {
							pSgList->MemParam.FIFOOverAdd		= add_low_o_dev;
							pSgList->MemParam.FIFOOverAdd_high	= add_high_o_dev;
						}
						pSgList->MemParam.SGOverAdd			= sg_dma_address(&pSgList->List[0]) + Lengthrem;
						pSgList->MemParam.SGOverAdd_high	= (sg_dma_address(&pSgList->List[0]) >> 32 & 0xffffffff);
						pSgList->MemParam.OverLen			= Lengthdev;
						pSgList->MemParam.ATNumber			= i;
						if (OverHalf == 0){
							OverHalf	= 1;
						} else {
							OverHalf	= 0;
						}
					} else {
						//--------------------------------------------
						// Specify the beginning of the next Descriptor Format in NextDescPtr
						//--------------------------------------------
						ListMove++;
						desc_cal	= (unsigned long long)(BM_MEM_OUT_NXT_DESC + (0x40 * (i + ListMove + k + 1)));
						ApiMemBmDivideAddress(desc_cal, &desc_low_o_n, &desc_high_o_n);
						//--------------------------------------------
						// Set Descriptor Format to Prefetcher FIFO
						//--------------------------------------------
						_outmd((BM_MEM_OUT_Write_SGAddr + (0x40 * (i + ListMove + k + 1))), add_low_o_dev);
						_outmd((BM_MEM_OUT_Write_SGAddr_high + (0x40 * (i + ListMove + k + 1))), add_high_o_dev);
						_outmd((BM_MEM_OUT_Read_SGAddr + (0x40 * (i + ListMove + k + 1))), (((sg_dma_address(&pSgList->List[i]) + (pSgList->List[i].length - Lengthdev)) & ~dwdata) + (BM_MEM_CRA_SELECT * (i + BM_MEM_AT_OFFSET))));
						_outmd((BM_MEM_OUT_Read_SGAddr_high + (0x40 * (i + ListMove + k + 1))), (sg_dma_address(&pSgList->List[i]) >> 32 & 0xffffffff));
						_outmd((BM_MEM_OUT_Length + (0x40 * (i + ListMove + k + 1))), Lengthdev);
						_outmd((BM_MEM_OUT_NextDescPtr + (0x40 * (i + ListMove + k + 1))), desc_low_o_n);
						_outmd((BM_MEM_OUT_NextDescPtr_high + (0x40 * (i + ListMove + k + 1))), desc_high_o_n);
						_outmw((BM_MEM_OUT_SequenceNum + (0x40 * (i + ListMove + k + 1))), (i + ListMove + k +1));
						_outmd((BM_MEM_OUT_Stride + (0x40 * (i + ListMove + k + 1))), 0x0);
						_outmw((BM_MEM_OUT_Burst + (0x40 * (i + ListMove + k + 1))), 0x101);
						_outmd((BM_MEM_OUT_Descriptor + (0x40 * (i + ListMove + k + 1))), 0x40000000);
						if (OverHalf == 0){
							//--------------------------------------------
							// Set FIFO address for next list
							//--------------------------------------------
							add_cal		= (unsigned long long)(add_cal_n + Lengthrem);
							ApiMemBmDivideAddress(add_cal, &add_low_o, &add_high_o);
							_outmd((BM_MEM_OUT_Write_SGAddr + (0x40 * (i + ListMove + k + 1))), add_low_o);
							_outmd((BM_MEM_OUT_Write_SGAddr_high + (0x40 * (i + ListMove + k + 1))), add_high_o);
							SetDesc		= 0;
							OverHalf	= 1;
						} else if (OverHalf == 1){
							//--------------------------------------------
							// Move FIFO address for next list
							//--------------------------------------------
							add_cal		= (unsigned long long)(add_cal_dev + Lengthdev);
							SetDesc		= 1;
							OverHalf	= 0;
						}
					}
					LengthStack	= Lengthdev;
					ListNum		= 1;
					n++;
				}	
			}
			Outlen	+= pSgList->List[i].length;
		}
		k	+= i;
	}
	//============================================
	// Set the descriptor for end discrimination at the beginning of the Descriptor List
	//============================================
	if(dwDir == BM_DIR_IN){
		//--------------------------------------------
		// Specify the beginning of prefetcher FIFO to NextDescPtr
		//--------------------------------------------
		desc_cal						= (unsigned long long)BM_MEM_INP_NXT_DESC;
		ApiMemBmDivideAddress(desc_cal, &desc_low_i_n, &desc_high_i_n);
		//--------------------------------------------
		// Set Descriptor Format to Prefetcher FIFO
		//--------------------------------------------
		_outmd((BM_MEM_INP_Read_SGAddr), 0x0);
		_outmd((BM_MEM_INP_Read_SGAddr_high), 0x0);
		_outmd((BM_MEM_INP_Write_SGAddr), 0x0);
		_outmd((BM_MEM_INP_Write_SGAddr_high), 0x0);
		_outmd((BM_MEM_INP_Length), 0x0);
		_outmd((BM_MEM_INP_NextDescPtr), desc_low_i_n);
		_outmd((BM_MEM_INP_NextDescPtr_high), desc_high_i_n);
		_outmw((BM_MEM_INP_SequenceNum), 0x0);
		_outmd((BM_MEM_INP_Stride), 0x0);
		_outmw((BM_MEM_INP_Burst), 0x101);
		_outmd((BM_MEM_INP_Descriptor), 0x0);
		//--------------------------------------------
		// Descriptor Store parameters for reconfiguring
		//--------------------------------------------
		pMasAddr->BmInp.resetchangedesc = 0;
		pMasAddr->BmInp.atstartlen		= Outlen;
		pMasAddr->BmInp.resetOverHalf	= OverHalf;
		if (pMasAddr->BmInp.sglen < 50){
			pMasAddr->BmInp.resetListMove	= 0;
			pSgList->ListNumCnt				= 0;
		} else {
			pMasAddr->BmInp.resetListMove	= ListMove;
			pSgList->ListNumCnt				= i * j;
		}
	}else {
		//--------------------------------------------
		// Specify the beginning of prefetcher FIFO to NextDescPtr
		//--------------------------------------------
		desc_cal	= (unsigned long long)BM_MEM_OUT_NXT_DESC;
		ApiMemBmDivideAddress(desc_cal, &desc_low_o_n, &desc_high_o_n);
		//--------------------------------------------
		// Set Descriptor Format to Prefetcher FIFO
		//--------------------------------------------
		_outmd((BM_MEM_OUT_Write_SGAddr), 0x0);
		_outmd((BM_MEM_OUT_Write_SGAddr_high), 0x0);
		_outmd((BM_MEM_OUT_Read_SGAddr), 0x0);
		_outmd((BM_MEM_OUT_Read_SGAddr_high), 0x0);
		_outmd((BM_MEM_OUT_Length), 0x0);
		_outmd((BM_MEM_OUT_NextDescPtr), desc_low_o_n);
		_outmd((BM_MEM_OUT_NextDescPtr_high), desc_high_o_n);
		_outmw((BM_MEM_OUT_SequenceNum), 0x0);
		_outmd((BM_MEM_OUT_Stride), 0x0);
		_outmw((BM_MEM_OUT_Burst), 0x101);
		_outmd((BM_MEM_OUT_Descriptor), 0x0);
		//--------------------------------------------
		// Descriptor Store parameters for reconfiguring
		//--------------------------------------------
		pMasAddr->BmOut.resetchangedesc = 0;
		pMasAddr->BmOut.atstartlen		= LengthStack;
		pMasAddr->BmOut.resetOverHalf	= OverHalf;
		if (pMasAddr->BmOut.sglen < 50){
			pMasAddr->BmOut.resetListMove	= 0;
		} else {
			pMasAddr->BmOut.resetListMove	= ListMove;
		}
		if (pMasAddr->BmOut.sglen < 50){
			pSgList->ListNumCnt	= 0;
		} else {
			pSgList->ListNumCnt	= i * j;
		}
	}
	if (OverFIFO == 1){
		pSgList->ReSetArray	= (0x4000 / (LenSize * 150)) + 1;
	} else {
		pSgList->ReSetArray	= n - 1;
	}
	pSgList->ATOverCnt	= 0;
	
	return	BM_ERROR_SUCCESS;
}

//========================================================================
// Function name  : MemBmReSetSGAddressRing
// Function		  : Reconfigure S/G list in hardware. (Infinite transfer)
// I/F	 	  	  : Internal
// In	 		  : pMasAddr			: Point to the MASTERADDR structure
//		   		 	dwDir				: BM_DIR_IN / BM_DIR_OUT
// Out			  : None
// Return value   : Normal completed	: BM_ERROR_SUCCESS
//		   			Abnormal completed	: BM_ERROR_PARAM / BM_ERROR_BUFF
// Addition  	  : 
//========================================================================
long MemBmReSetSGAddressRing(PMASTERADDR pMasAddr, unsigned long dwDir)
{
	unsigned long		i = 0;
	PSGLIST 			pSgList;
	unsigned long long	desc_cal;
	unsigned long long	add_cal, add_cal_n;
	unsigned long long	add_cal_res, add_cal_chk;
	unsigned long long	add_cal_dev;
	unsigned long		dwdata = 0;
	unsigned long		desc_low_o_n, desc_high_o_n;
	unsigned long		desc_low_i_n, desc_high_i_n;
	unsigned long		cradata, cradata2;
	unsigned long		add_low_i, add_high_i;
	unsigned long		add_low_o, add_high_o;
	unsigned long		add_low_o_chk, add_low_i_chk;
	unsigned long		add_high_o_chk, add_high_i_chk;
	unsigned long		add_low_i_dev, add_high_i_dev;
	unsigned long		add_low_o_dev, add_high_o_dev;
	unsigned long		ListSize;
	unsigned long		LengthStack, Lengthdev, Lengthrem;
	unsigned long		j, k, l;
	unsigned long		OverHalf, loop;
	unsigned long		LenSize;
	unsigned long		LenNum, RemSize;
	unsigned long		ListStart, ListEnd, HalfSize, CurrentSize;
	int					SetDesc, LastList;
	int					NextStart;
	
	//--------------------------------------------
	// Checking parameter
	//--------------------------------------------
	if(dwDir != BM_DIR_IN && dwDir != BM_DIR_OUT){
		return	BM_ERROR_PARAM;
	}
	if(dwDir == BM_DIR_IN){
		pSgList	= &pMasAddr->BmInp.SgList;
	}else{
		pSgList	= &pMasAddr->BmOut.SgList;
	}
	if(pSgList->nr_pages > BM_MEM_MAX_SG_SIZE){
		return	BM_ERROR_BUFF;
	}
	//---------------------------------------------
	// Setting FIFO address
	//---------------------------------------------
	if(dwDir & BM_DIR_IN){
		add_low_i	= pSgList->MemParam.FIFOOverAdd;
		add_high_i	= pSgList->MemParam.FIFOOverAdd_high;
		add_cal		= (unsigned long long)(add_low_i + ((unsigned long long)add_high_i << 32));
		pMasAddr->BmInp.addtablenum	= 0;
	} else {
		add_low_o	= pSgList->MemParam.FIFOOverAdd;
		add_high_o	= pSgList->MemParam.FIFOOverAdd_high;
		add_cal		= (unsigned long long)(add_low_o + ((unsigned long long)add_high_o << 32));
		pMasAddr->BmOut.addtablenum	= 0;
	}
	add_cal_n	= add_cal;
	ListSize	= pSgList->nr_pages;
	//--------------------------------------------
	// If the number of SGList exceeds a certain number, limit the number of lists for reconfiguration
	//--------------------------------------------
	if(dwDir & BM_DIR_IN){
		OverHalf	= pMasAddr->BmInp.resetOverHalf;
		ListEnd		= ListSize;
		LenSize		= 0;
		LengthStack	= 0;
		LenNum		= 1;
		CurrentSize	= pMasAddr->BmInp.atstartlen;
		for (loop = 0; loop < ListSize; loop++) {
			LenSize		+=  pMasAddr->BmInp.SgList.List[loop].length;
		}
		HalfSize	= LenSize;
		if ((CurrentSize + LenSize) >= 0x4000){
			LenNum++;
		}
		if (OverHalf == 1){
			l	= pMasAddr->BmInp.resetListMove;
			k	= 5;
		} else {
			k	= 0;
			l	= 0;
		}
		add_cal_dev	= (unsigned long long)BM_MEM_INP_ReadAdd;
		ApiMemBmDivideAddress(add_cal_dev, &add_low_i_dev, &add_high_i_dev);
	} else {
		OverHalf	= pMasAddr->BmOut.resetOverHalf;
		ListEnd		= ListSize;
		LenSize		= 0;
		LengthStack	= pMasAddr->BmOut.atstartlen;
		for (loop = 0; loop < ListSize; loop++) {
			LenSize		+=  pMasAddr->BmOut.SgList.List[loop].length;
		}
		HalfSize	= 75 * LenSize;
		if (LengthStack >= (HalfSize * 2)){
			LengthStack	= 0;
		}
		RemSize	= HalfSize - LengthStack;
		LenNum	= RemSize / LenSize;
		if ((RemSize % LenSize) != 0){
			LenNum++;
		}
		if (LenNum == 0){
			LenNum	= 1;
		} else if (LenNum > 75){
			LenNum	= 75;
		}
		if (LengthStack == 0){
			LengthStack	= LenSize;
		}
		if (OverHalf == 1){
			l	= pMasAddr->BmOut.resetListMove;
			k	= 5;
		} else {
			k	= 0;
			l	= 0;
		}
		add_cal_dev	= (unsigned long long)BM_MEM_OUT_WriteAdd;
		ApiMemBmDivideAddress(add_cal_dev, &add_low_o_dev, &add_high_o_dev);
	}
	ListStart	= pSgList->ListNumCnt;
	Lengthdev	= 0;
	Lengthrem	= 0;
	SetDesc		= 0;
	LastList	= 0;
	NextStart	= 0;
	j			= 0;
	//--------------------------------------------
	// Loop the number of times
	//--------------------------------------------
	for (j = 0; j < LenNum; j++) {
		//--------------------------------------------
		// Loop the number of times
		//--------------------------------------------
		if (j == 0){
			i	= ListStart;
		} else {
			i	= 0;
		}
		for (; i < ListEnd; i++) {
			//--------------------------------------------
			// Set in master register
			//--------------------------------------------
			if(dwDir == BM_DIR_IN){
				if (j == 0){
					//--------------------------------------------
					// Set the Address Translation Table
					//--------------------------------------------
					if (pMasAddr->add64flag == 1){
						_outmd((BM_MEM_CRA + (0x8 * i)), 0xFFFFFFFD);
						dwdata		= _inmd(BM_MEM_CRA + (0x8 * i));
						_outmd((BM_MEM_CRA + (0x8 * i)), ((dwdata & sg_dma_address(&pSgList->List[i])) | 0x1));
						_outmd((BM_MEM_CRA_HIGH + (0x8 * i)), (sg_dma_address(&pSgList->List[i]) >> 32 & 0xffffffff));
						cradata		= _inmd(BM_MEM_CRA + (0x8 * i));
						cradata2	= _inmd(BM_MEM_CRA_HIGH + (0x8 * i));
					} else {
						_outmd((BM_MEM_CRA + (0x8 * i)), 0xFFFFFFFC);
						dwdata		= _inmd(BM_MEM_CRA + (0x8 * i));
						_outmd((BM_MEM_CRA + (0x8 * i)), (dwdata & sg_dma_address(&pSgList->List[i])));
						_outmd((BM_MEM_CRA_HIGH + (0x8 * i)), (sg_dma_address(&pSgList->List[i]) >> 32 & 0xffffffff));
						cradata		= _inmd(BM_MEM_CRA + (0x8 * i));
						cradata2	= _inmd(BM_MEM_CRA_HIGH + (0x8 * i));
					}
				}
				LengthStack	+= pSgList->List[i].length;
				//--------------------------------------------
				// Move FIFO address for second and subsequent lists
				//--------------------------------------------
				if ((i > (ListStart + 1)) && (j == 0)){
					if (SetDesc == 1){
						SetDesc	= 0;
					} else {
						add_cal	= (unsigned long long)(add_cal_n + pSgList->List[i - 1].length);
					}
				} else if ((i == (ListStart + 1)) && (j == 0)){
					add_cal		= (unsigned long long)(add_cal_n + pSgList->MemParam.OverLen);
					NextStart	= 1;
				} else if (j > 0){
					if (SetDesc == 1){
						SetDesc		= 0;
					} else {
						if (i > 0){
							add_cal	= (unsigned long long)(add_cal_n + pSgList->List[i - 1].length);
						} else {
							add_cal	= (unsigned long long)(add_cal_n + pSgList->List[ListSize - 1].length);
						}
					}
				}
				ApiMemBmDivideAddress(add_cal, &add_low_i, &add_high_i);
				add_cal_n	= add_cal;
				//--------------------------------------------
				// Set the beginning of FIFO if the block size exceeds half of the transfer size
				//--------------------------------------------
				if (LengthStack >= HalfSize){
					Lengthdev	= LengthStack - HalfSize;
					Lengthrem	= HalfSize - (LengthStack - pSgList->List[i].length);
				}
				//--------------------------------------------
				// Specify the beginning of the next Descriptor Format in NextDescPtr
				//--------------------------------------------
				desc_cal	= (unsigned long long)(BM_MEM_INP_NXT_DESC + (0x40 * (l + k + 1)));
				ApiMemBmDivideAddress(desc_cal, &desc_low_i_n, &desc_high_i_n);
				//--------------------------------------------
				// Set Descriptor Format to Prefetcher FIFO
				//--------------------------------------------
				if ((j == 0) && (i == ListStart)){
					_outmd((BM_MEM_INP_Write_SGAddr + (0x40 * (l + k + 1))), ((pSgList->MemParam.SGOverAdd & ~dwdata) + (BM_MEM_CRA_SELECT * i)));
					_outmd((BM_MEM_INP_Write_SGAddr_high + (0x40 * (l + k + 1))), pSgList->MemParam.SGOverAdd_high);
					_outmd((BM_MEM_INP_Length + (0x40 * (l + k + 1))), pSgList->MemParam.OverLen);
					pSgList->MemParam.ATPreFetchAdd	= (BM_MEM_INP_NXT_DESC + (0x40 * (l + k)));
					NextStart	= 1;
				} else if ((i == 0) && (NextStart == 0) && (j == 1)){
					_outmd((BM_MEM_INP_Write_SGAddr + (0x40 * (l + k + 1))), ((pSgList->MemParam.SGOverAdd & ~dwdata) + (BM_MEM_CRA_SELECT * i)));
					_outmd((BM_MEM_INP_Write_SGAddr_high + (0x40 * (l + k + 1))), pSgList->MemParam.SGOverAdd_high);
					_outmd((BM_MEM_INP_Length + (0x40 * (l + k + 1))), pSgList->MemParam.OverLen);
					pSgList->MemParam.ATPreFetchAdd	= (BM_MEM_INP_NXT_DESC + (0x40 * (l + k)));
					NextStart	= 1;
				} else {
					_outmd((BM_MEM_INP_Write_SGAddr + (0x40 * (l + k + 1))), ((sg_dma_address(&pSgList->List[i]) & ~dwdata) + (BM_MEM_CRA_SELECT * i)));
					_outmd((BM_MEM_INP_Write_SGAddr_high + (0x40 * (l + k + 1))), (sg_dma_address(&pSgList->List[i]) >> 32 & 0xffffffff));
					_outmd((BM_MEM_INP_Length + (0x40 * (l + k + 1))), pSgList->List[i].length);
				}
				_outmd((BM_MEM_INP_Read_SGAddr + (0x40 * (l + k + 1))), add_low_i);
				_outmd((BM_MEM_INP_Read_SGAddr_high + (0x40 * (l + k + 1))), add_high_i);
				_outmd((BM_MEM_INP_NextDescPtr + (0x40 * (l + k + 1))), desc_low_i_n);
				_outmd((BM_MEM_INP_NextDescPtr_high + (0x40 * (l + k + 1))), desc_high_i_n);
				_outmw((BM_MEM_INP_SequenceNum + (0x40 * (l + k + 1))), (l+ k + 1));
				_outmd((BM_MEM_INP_Stride + (0x40 * (l + k + 1))), 0x0);
				_outmw((BM_MEM_INP_Burst + (0x40 * (l + k + 1))), 0x101);
				_outmd((BM_MEM_INP_Descriptor + (0x40 * (l + k + 1))), 0x40000000);
				//--------------------------------------------
				// If the block exceeds the 4kByte boundary, reconfigure from the beginning of FIFO
				//--------------------------------------------
				add_cal_chk		= (unsigned long long)(add_cal_n + pSgList->List[i].length);
				ApiMemBmDivideAddress(add_cal_chk, &add_low_i_chk, &add_high_i_chk);
				if (add_low_i_chk >= 0x7004000){
					Lengthdev	= add_low_i_chk - 0x7004000;
					Lengthrem	= pSgList->List[i].length - Lengthdev;
					if (add_low_i_chk > 0x7004000){
						_outmd((BM_MEM_INP_Length + (0x40 * (l + k + 1))), Lengthrem);
						k++;
						//--------------------------------------------
						// Specify the beginning of the next Descriptor Format in NextDescPtr
						//--------------------------------------------
						desc_cal	= (unsigned long long)(BM_MEM_INP_NXT_DESC + (0x40 * (l + k + 1)));
						ApiMemBmDivideAddress(desc_cal, &desc_low_i_n, &desc_high_i_n);
						//--------------------------------------------
						// Set Descriptor Format to Prefetcher FIFO
						//--------------------------------------------
						_outmd((BM_MEM_INP_Read_SGAddr + (0x40 * (l + k + 1))), add_low_i_dev);
						_outmd((BM_MEM_INP_Read_SGAddr_high + (0x40 * (l + k + 1))), add_high_i_dev);
						_outmd((BM_MEM_INP_Write_SGAddr + (0x40 * (l + k + 1))), (((sg_dma_address(&pSgList->List[i]) + (pSgList->List[i].length - Lengthdev)) & ~dwdata) + (BM_MEM_CRA_SELECT * i)));
						_outmd((BM_MEM_INP_Write_SGAddr_high + (0x40 * (l + k + 1))), (sg_dma_address(&pSgList->List[i]) >> 32 & 0xffffffff));
						_outmd((BM_MEM_INP_Length + (0x40 * (l + k + 1))), Lengthdev);
						_outmd((BM_MEM_INP_NextDescPtr + (0x40 * (l + k + 1))), desc_low_i_n);
						_outmd((BM_MEM_INP_NextDescPtr_high + (0x40 * (l + k + 1))), desc_high_i_n);
						_outmw((BM_MEM_INP_SequenceNum + (0x40 * (l + k + 1))), (l + k + 1));
						_outmd((BM_MEM_INP_Stride + (0x40 * (l + k + 1))), 0x0);
						_outmw((BM_MEM_INP_Burst + (0x40 * (l + k + 1))), 0x101);
						_outmd((BM_MEM_INP_Descriptor + (0x40 * (l + k + 1))), 0x40000000);
					}
					//--------------------------------------------
					// Descriptor Store parameters for reconfiguring
					//--------------------------------------------
					add_cal		= (unsigned long long)(add_cal_dev + Lengthdev);
					ApiMemBmDivideAddress(add_cal, &add_low_i, &add_high_i);
					if ((i == ListStart) && (j == 0)){
						add_cal_n					= add_cal_dev;
						pSgList->MemParam.OverLen	= Lengthdev;
					}
					SetDesc		= 1;
					if ((LengthStack >= HalfSize) && (i == (ListSize - 1)) && (j == (LenNum - 1))){
						if (OverHalf == 0){
							OverHalf	= 1;
						} else {
							OverHalf	= 0;
						}
						pSgList->MemParam.FIFOOverAdd		= add_low_i;
						pSgList->MemParam.FIFOOverAdd_high	= add_high_i;
						pSgList->MemParam.SGOverAdd			= sg_dma_address(&pSgList->List[0]);
						pSgList->MemParam.SGOverAdd_high	= (sg_dma_address(&pSgList->List[0]) >> 32 & 0xffffffff);
						pSgList->MemParam.OverLen			= pSgList->List[0].length;
						pSgList->MemParam.ATNumber			= 0;
						LengthStack							= Lengthdev;
						LastList							= 1;
						desc_cal							= (unsigned long long)BM_MEM_INP_TOP_DESC;
						ApiMemBmDivideAddress(desc_cal, &desc_low_i_n, &desc_high_i_n);
						_outmd((BM_MEM_INP_NextDescPtr + (0x40 * (l + k + 1))), desc_low_i_n);
						_outmd((BM_MEM_INP_NextDescPtr_high + (0x40 * (l + k + 1))), desc_high_i_n);
					}
				} else {
					//--------------------------------------------
					// Transfer setting of final transfer block
					//--------------------------------------------
					if ((LengthStack <= HalfSize) && (i == (ListSize - 1)) && (j == (LenNum - 1))){
						if ((LengthStack == HalfSize) && (OverHalf == 1)){
							add_cal_res	= (unsigned long long)BM_MEM_INP_ReadAdd;
						} else {
							add_cal_res	= (unsigned long long)(add_cal_n + pSgList->List[i].length);
						}
						pMasAddr->BmInp.resetadd	= add_cal_res;
						//--------------------------------------------
						//Set NextDescPtr to the beginning of Prefetcher
						//--------------------------------------------
						if (j == (LenNum - 1)) {
							desc_cal	= (unsigned long long)BM_MEM_INP_TOP_DESC;
							ApiMemBmDivideAddress(desc_cal, &desc_low_i_n, &desc_high_i_n);
							_outmd((BM_MEM_INP_NextDescPtr + (0x40 * (l + k + 1))), desc_low_i_n);
							_outmd((BM_MEM_INP_NextDescPtr_high + (0x40 * (l + k + 1))), desc_high_i_n);
						}
						//--------------------------------------------
						// Descriptor Store parameters for reconfiguring
						//--------------------------------------------
						if (LengthStack == HalfSize){
							if (OverHalf == 0){
								OverHalf	= 1;
							} else {
								OverHalf	= 0;
							}
							add_cal								= (unsigned long long)(add_cal_n + pSgList->List[i].length);
							ApiMemBmDivideAddress(add_cal, &add_low_i, &add_high_i);
							pSgList->MemParam.FIFOOverAdd		= add_low_i;
							pSgList->MemParam.FIFOOverAdd_high	= add_high_i;
							pSgList->MemParam.SGOverAdd			= sg_dma_address(&pSgList->List[0]);
							pSgList->MemParam.SGOverAdd_high	= (sg_dma_address(&pSgList->List[0]) >> 32 & 0xffffffff);
							pSgList->MemParam.OverLen			= pSgList->List[0].length;
							pSgList->MemParam.ATNumber			= 0;
							LengthStack							= Lengthdev;
							LastList							= 1;
						}
					}
					//--------------------------------------------
					// Split transfer size of 1 block
					//--------------------------------------------
					if ((LengthStack == HalfSize) && (j != (LenNum - 1))){
						desc_cal	= (unsigned long long)BM_MEM_INP_TOP_DESC;
						ApiMemBmDivideAddress(desc_cal, &desc_low_i_n, &desc_high_i_n);
						_outmd((BM_MEM_INP_NextDescPtr + (0x40 * (l + k + 1))), desc_low_i_n);
						_outmd((BM_MEM_INP_NextDescPtr_high + (0x40 * (l + k + 1))), desc_high_i_n);
						if (OverHalf == 0){
							OverHalf	= 1;
						} else {
							OverHalf	= 0;
						}
						LengthStack	= Lengthdev;
						LastList	= 1;
					} else if (LengthStack > HalfSize){
						Lengthdev	= LengthStack - HalfSize;
						Lengthrem	= HalfSize - (LengthStack - pSgList->List[i].length);
						_outmd((BM_MEM_INP_Length + (0x40 * (l + k + 1))), Lengthrem);
						//--------------------------------------------
						// Store parameters for final block
						//--------------------------------------------
						if (j == (LenNum - 1)){
							if (OverHalf == 0){
								OverHalf	= 1;
							} else {
								OverHalf	= 0;
							}
							add_cal								= (unsigned long long)(add_cal_n + Lengthrem);
							ApiMemBmDivideAddress(add_cal, &add_low_i, &add_high_i);
							pSgList->MemParam.FIFOOverAdd		= add_low_i;
							pSgList->MemParam.FIFOOverAdd_high	= add_high_i;
							pSgList->MemParam.SGOverAdd			= sg_dma_address(&pSgList->List[0]);
							pSgList->MemParam.SGOverAdd_high	= (sg_dma_address(&pSgList->List[0]) >> 32 & 0xffffffff);
							pSgList->MemParam.OverLen			= Lengthdev;
							pSgList->MemParam.ATNumber			= 0;
							LengthStack							= Lengthdev;
						} else {
							//--------------------------------------------
							// Specify the beginning of the next Descriptor Format in NextDescPtr
							//--------------------------------------------
							k++;
							desc_cal	= (unsigned long long)(BM_MEM_INP_NXT_DESC + (0x40 * (l + k + 1)));
							ApiMemBmDivideAddress(desc_cal, &desc_low_i_n, &desc_high_i_n);
							//--------------------------------------------
							// Set Descriptor Format to Prefetcher FIFO
							//--------------------------------------------
							_outmd((BM_MEM_INP_Read_SGAddr + (0x40 * (l + k + 1))), add_low_i_dev);
							_outmd((BM_MEM_INP_Read_SGAddr_high + (0x40 * (l + k + 1))), add_high_i_dev);
							_outmd((BM_MEM_INP_Write_SGAddr + (0x40 * (l + k + 1))), (((sg_dma_address(&pSgList->List[i]) + (pSgList->List[i].length - Lengthdev)) & ~dwdata) + (BM_MEM_CRA_SELECT * i)));
							_outmd((BM_MEM_INP_Write_SGAddr_high + (0x40 * (l + k + 1))), (sg_dma_address(&pSgList->List[i]) >> 32 & 0xffffffff));
							_outmd((BM_MEM_INP_Length + (0x40 * (l + k + 1))), Lengthdev);
							_outmd((BM_MEM_INP_NextDescPtr + (0x40 * (l + k + 1))), desc_low_i_n);
							_outmd((BM_MEM_INP_NextDescPtr_high + (0x40 * (l + k + 1))), desc_high_i_n);
							_outmw((BM_MEM_INP_SequenceNum + (0x40 * (l + k + 1))), (l + k + 1));
							_outmd((BM_MEM_INP_Stride + (0x40 * (l + k + 1))), 0x0);
							_outmw((BM_MEM_INP_Burst + (0x40 * (l + k + 1))), 0x101);
							_outmd((BM_MEM_INP_Descriptor + (0x40 * (l + k + 1))), 0x40000000);
							//--------------------------------------------
							// If the block exceeds the 4kByte boundary, reconfigure from the beginning of FIFO
							//--------------------------------------------
							add_cal		= (unsigned long long)(add_cal_n + Lengthrem);
							ApiMemBmDivideAddress(add_cal, &add_low_i, &add_high_i);
							if (add_low_i >= 0x7004000){
								add_cal		= (unsigned long long)(add_cal_dev + Lengthdev);
								if (OverHalf == 0){
									SetDesc		= 0;
									OverHalf	= 1;
								} else if (OverHalf == 1){
									SetDesc		= 1;
									OverHalf	= 0;
								}
							} else {
								if (OverHalf == 0){
									SetDesc		= 0;
									OverHalf	= 1;
								} else if (OverHalf == 1){
									SetDesc		= 1;
									OverHalf	= 0;
								}
							}
							//--------------------------------------------
							// Set FIFO address for next list
							//--------------------------------------------
							ApiMemBmDivideAddress(add_cal, &add_low_i, &add_high_i);
							_outmd((BM_MEM_INP_Read_SGAddr + (0x40 * (l + k + 1))), add_low_i);
							_outmd((BM_MEM_INP_Read_SGAddr_high + (0x40 * (l + k + 1))), add_high_i);
						}
						LengthStack	= Lengthdev;
						LastList	= 1;
					}
				}
			}else{
				if (j == 0){
					//--------------------------------------------
					// Set the Address Translation Table
					//--------------------------------------------
					if (pMasAddr->add64flag == 1){
						_outmd((BM_MEM_CRA + (0x8 * (i + BM_MEM_AT_OFFSET))), 0xFFFFFFFD);
						dwdata		= _inmd(BM_MEM_CRA + (0x8 * (i + BM_MEM_AT_OFFSET)));
						_outmd((BM_MEM_CRA + (0x8 * (i + BM_MEM_AT_OFFSET))), ((dwdata & sg_dma_address(&pSgList->List[i])) | 0x1));
						_outmd((BM_MEM_CRA_HIGH + (0x8 * (i + BM_MEM_AT_OFFSET))), (sg_dma_address(&pSgList->List[i]) >> 32 & 0xffffffff));
						cradata		= _inmd(BM_MEM_CRA + (0x8 * (i + BM_MEM_AT_OFFSET)));
						cradata2	= _inmd(BM_MEM_CRA_HIGH + (0x8 * (i + BM_MEM_AT_OFFSET)));
					} else {
						_outmd((BM_MEM_CRA + (0x8 * (i + BM_MEM_AT_OFFSET))), 0xFFFFFFFC);
						dwdata		= _inmd(BM_MEM_CRA + (0x8 * (i + BM_MEM_AT_OFFSET)));
						_outmd((BM_MEM_CRA + (0x8 * (i + BM_MEM_AT_OFFSET))), (dwdata & sg_dma_address(&pSgList->List[i])));
						_outmd((BM_MEM_CRA_HIGH + (0x8 * (i + BM_MEM_AT_OFFSET))), (sg_dma_address(&pSgList->List[i]) >> 32 & 0xffffffff));
						cradata		= _inmd(BM_MEM_CRA + (0x8 * (i + BM_MEM_AT_OFFSET)));
						cradata2	= _inmd(BM_MEM_CRA_HIGH + (0x8 * (i + BM_MEM_AT_OFFSET)));
					}
				}
				if ((i > ListStart) || (j != 0)){
					LengthStack	+= pSgList->List[i].length;
				}
				//--------------------------------------------
				// Move FIFO address for second and subsequent lists
				//--------------------------------------------
				if ((i > (ListStart + 1)) && (j == 0)){
					if (SetDesc == 1){
						SetDesc		= 0;
					} else {
						add_cal		= (unsigned long long)(add_cal_n + pSgList->List[i - 1].length);
					}
				} else if ((i == (ListStart + 1)) && (j == 0)){
					add_cal		= (unsigned long long)(add_cal_n + pSgList->MemParam.OverLen);
					NextStart	= 1;
				} else if (j > 0){
					if (SetDesc == 1){
						SetDesc		= 0;
					} else {
						if (i > 0){
							add_cal	= (unsigned long long)(add_cal_n + pSgList->List[i - 1].length);
						} else {
							add_cal	= (unsigned long long)(add_cal_n + pSgList->List[ListSize - 1].length);
						}
					}
				}
				ApiMemBmDivideAddress(add_cal, &add_low_o, &add_high_o);
				add_cal_n	= add_cal;
				//--------------------------------------------
				// Set the beginning of FIFO if the block size exceeds half of the transfer size
				//--------------------------------------------
				if (LengthStack >= HalfSize){
					Lengthdev	= LengthStack - HalfSize;
					Lengthrem	= HalfSize - (LengthStack - pSgList->List[i].length);
				}
				//--------------------------------------------
				// Specify the beginning of the next Descriptor Format in NextDescPtr
				//--------------------------------------------
				desc_cal	= (unsigned long long)(BM_MEM_OUT_NXT_DESC + (0x40 * (l + k + 1)));
				ApiMemBmDivideAddress(desc_cal, &desc_low_o_n, &desc_high_o_n);
				//--------------------------------------------
				// Set Descriptor Format to Prefetcher FIFO
				//--------------------------------------------
				if ((j == 0) && (i == ListStart)){
					_outmd((BM_MEM_OUT_Read_SGAddr + (0x40 * (l + k + 1))), ((pSgList->MemParam.SGOverAdd & ~dwdata) + (BM_MEM_CRA_SELECT * (i + BM_MEM_AT_OFFSET))));
					//_outmd((BM_MEM_OUT_Read_SGAddr_high + (0x40 * (l + k + 1))), pSgList->MemParam.SGOverAdd_high);
					_outmd((BM_MEM_OUT_Length + (0x40 * (l + k + 1))), pSgList->MemParam.OverLen);
					pSgList->MemParam.ATPreFetchAdd	= (BM_MEM_OUT_NXT_DESC + (0x40 * (l + k)));
					NextStart	= 1;
				} else if ((i == 0) && (NextStart == 0) && (j == 1)){
					_outmd((BM_MEM_OUT_Read_SGAddr + (0x40 * (l + k + 1))), ((pSgList->MemParam.SGOverAdd & ~dwdata) + (BM_MEM_CRA_SELECT * (i + BM_MEM_AT_OFFSET))));
					//_outmd((BM_MEM_OUT_Read_SGAddr_high + (0x40 * (l + k + 1))), pSgList->MemParam.SGOverAdd_high);
					_outmd((BM_MEM_OUT_Length + (0x40 * (l + k + 1))), pSgList->MemParam.OverLen);
					pSgList->MemParam.ATPreFetchAdd	= (BM_MEM_OUT_NXT_DESC + (0x40 * (l + k)));
					NextStart	= 1;
				} else {
					_outmd((BM_MEM_OUT_Read_SGAddr + (0x40 * (l + k + 1))), ((sg_dma_address(&pSgList->List[i]) & ~dwdata) + (BM_MEM_CRA_SELECT * (i + BM_MEM_AT_OFFSET))));
					_outmd((BM_MEM_OUT_Read_SGAddr_high + (0x40 * (l + k + 1))), (sg_dma_address(&pSgList->List[i]) >> 32 & 0xffffffff));
					_outmd((BM_MEM_OUT_Length + (0x40 * (l + k + 1))), pSgList->List[i].length);
				}
				_outmd((BM_MEM_OUT_Write_SGAddr + (0x40 * (l + k + 1))), add_low_o);
				_outmd((BM_MEM_OUT_Write_SGAddr_high + (0x40 * (l + k + 1))), add_high_o);
				_outmd((BM_MEM_OUT_NextDescPtr + (0x40 * (l + k + 1))), desc_low_o_n);
				_outmd((BM_MEM_OUT_NextDescPtr_high + (0x40 * (l + k + 1))), desc_high_o_n);
				_outmw((BM_MEM_OUT_SequenceNum + (0x40 * (l + k + 1))), (l + k + 1));
				_outmd((BM_MEM_OUT_Stride + (0x40 * (l + k + 1))), 0x0);
				_outmw((BM_MEM_OUT_Burst + (0x40 * (l + k + 1))), 0x101);
				_outmd((BM_MEM_OUT_Descriptor + (0x40 * (l + k + 1))), 0x40000000);
				//--------------------------------------------
				// If the block exceeds the 4kByte boundary, reconfigure from the beginning of FIFO
				//--------------------------------------------
				add_cal_chk	= (unsigned long long)(add_cal_n + pSgList->List[i].length);
				ApiMemBmDivideAddress(add_cal_chk, &add_low_o_chk, &add_high_o_chk);
				if (add_low_o_chk >= 0x9004000){
					Lengthdev	= add_low_o_chk - 0x9004000;
					Lengthrem	= pSgList->List[i].length - Lengthdev;
					if (add_low_o_chk > 0x9004000){
						_outmd((BM_MEM_OUT_Length + (0x40 * (l + k + 1))), Lengthrem);
						k++;
						//--------------------------------------------
						// Specify the beginning of the next Descriptor Format in NextDescPtr
						//--------------------------------------------
						desc_cal	= (unsigned long long)(BM_MEM_OUT_NXT_DESC + (0x40 * (l + k + 1)));
						ApiMemBmDivideAddress(desc_cal, &desc_low_o_n, &desc_high_o_n);
						//--------------------------------------------
						// Set Descriptor Format to Prefetcher FIFO
						//--------------------------------------------
						_outmd((BM_MEM_OUT_Write_SGAddr + (0x40 * (l + k + 1))), add_low_o_dev);
						_outmd((BM_MEM_OUT_Write_SGAddr_high + (0x40 * (l + k + 1))), add_high_o_dev);
						_outmd((BM_MEM_OUT_Read_SGAddr + (0x40 * (l + k + 1))), (((sg_dma_address(&pSgList->List[i]) + (pSgList->List[i].length - Lengthdev)) & ~dwdata) + (BM_MEM_CRA_SELECT * (i + BM_MEM_AT_OFFSET))));
						_outmd((BM_MEM_OUT_Read_SGAddr_high + (0x40 * (l + k + 1))), (sg_dma_address(&pSgList->List[i]) >> 32 & 0xffffffff));
						_outmd((BM_MEM_OUT_Length + (0x40 * (l + k + 1))), Lengthdev);
						_outmd((BM_MEM_OUT_NextDescPtr + (0x40 * (l + k + 1))), desc_low_o_n);
						_outmd((BM_MEM_OUT_NextDescPtr_high + (0x40 * (l + k + 1))), desc_high_o_n);
						_outmw((BM_MEM_OUT_SequenceNum + (0x40 * (l + k + 1))), (l + k + 1));
						_outmd((BM_MEM_OUT_Stride + (0x40 * (l + k + 1))), 0x0);
						_outmw((BM_MEM_OUT_Burst + (0x40 * (l + k + 1))), 0x101);
						_outmd((BM_MEM_OUT_Descriptor + (0x40 * (l + k + 1))), 0x40000000);
					}
					//--------------------------------------------
					// Descriptor Store parameters for reconfiguring
					//--------------------------------------------
					add_cal		= (unsigned long long)(add_cal_dev + Lengthdev);
					if ((i == ListStart) && (j == 0)){
						add_cal_n					= add_cal_dev;
						pSgList->MemParam.OverLen	= Lengthdev;
					}
					SetDesc		= 1;
					if ((LengthStack >= HalfSize) && (i == (ListSize - 1)) && (j == (LenNum - 1))){
						if (OverHalf == 0){
							OverHalf	= 1;
						} else {
							OverHalf	= 0;
						}
						ApiMemBmDivideAddress(add_cal, &add_low_o, &add_high_o);
						pSgList->MemParam.FIFOOverAdd		= add_low_o;
						pSgList->MemParam.FIFOOverAdd_high	= add_high_o;
						pSgList->MemParam.SGOverAdd			= sg_dma_address(&pSgList->List[0]);
						pSgList->MemParam.SGOverAdd_high	= (sg_dma_address(&pSgList->List[0]) >> 32 & 0xffffffff);
						pSgList->MemParam.OverLen			= pSgList->List[0].length;
						pSgList->MemParam.ATNumber			= 0;
						LengthStack							= Lengthdev;
						LastList							= 1;
						//--------------------------------------------
						// Specify the beginning of the next Descriptor Format in NextDescPtr
						//--------------------------------------------
						desc_cal							= (unsigned long long)BM_MEM_OUT_TOP_DESC;
						ApiMemBmDivideAddress(desc_cal, &desc_low_o_n, &desc_high_o_n);
						_outmd((BM_MEM_OUT_NextDescPtr + (0x40 * (l + k + 1))), desc_low_o_n);
						_outmd((BM_MEM_OUT_NextDescPtr_high + (0x40 * (l + k + 1))), desc_high_o_n);
					}
				} else {
					//--------------------------------------------
					// Transfer setting of final transfer block
					//--------------------------------------------
					if ((LengthStack <= HalfSize) && (i == (ListSize - 1)) && (j == (LenNum - 1))){
						if ((LengthStack == HalfSize) && (OverHalf == 1)){
							add_cal_res	= (unsigned long long)BM_MEM_OUT_WriteAdd;
						} else {
							add_cal_res	= (unsigned long long)(add_cal_n + pSgList->List[i].length);
						}
						pMasAddr->BmOut.resetadd	= add_cal_res;
						//--------------------------------------------
						// Set NextDescPtr to the beginning of Prefetcher
						//--------------------------------------------
						if (j == (LenNum - 1)) {
							desc_cal	= (unsigned long long)BM_MEM_OUT_TOP_DESC;
							ApiMemBmDivideAddress(desc_cal, &desc_low_o_n, &desc_high_o_n);
							_outmd((BM_MEM_OUT_NextDescPtr + (0x40 * (l + k + 1))), desc_low_o_n);
							_outmd((BM_MEM_OUT_NextDescPtr_high + (0x40 * (l + k + 1))), desc_high_o_n);
						}
						//--------------------------------------------
						// Descriptor Store parameters for reconfiguring
						//--------------------------------------------
						if (OverHalf == 0){
							OverHalf	= 1;
						} else {
							OverHalf	= 0;
						}
						add_cal								= (unsigned long long)(add_cal_n + pSgList->List[i].length);
						ApiMemBmDivideAddress(add_cal, &add_low_o, &add_high_o);
						pSgList->MemParam.FIFOOverAdd		= add_low_o;
						pSgList->MemParam.FIFOOverAdd_high	= add_high_o;
						pSgList->MemParam.SGOverAdd			= sg_dma_address(&pSgList->List[0]);
						pSgList->MemParam.SGOverAdd_high	= (sg_dma_address(&pSgList->List[0]) >> 32 & 0xffffffff);
						pSgList->MemParam.OverLen			= pSgList->List[0].length;
						pSgList->MemParam.ATNumber			= 0;
						LengthStack							= Lengthdev;
						LastList							= 1;
					}
					//--------------------------------------------
					// Split transfer size of 1 block
					//--------------------------------------------
					if ((LengthStack == HalfSize) && (j != (LenNum - 1))){
						desc_cal	= (unsigned long long)BM_MEM_OUT_TOP_DESC;
						ApiMemBmDivideAddress(desc_cal, &desc_low_o_n, &desc_high_o_n);
						_outmd((BM_MEM_OUT_NextDescPtr + (0x40 * (l + k + 1))), desc_low_o_n);
						_outmd((BM_MEM_OUT_NextDescPtr_high + (0x40 * (l + k + 1))), desc_high_o_n);
						if (OverHalf == 0){
							OverHalf	= 1;
						} else {
							OverHalf	= 0;
						}
						LengthStack	= Lengthdev;
						LastList							= 1;
					} else if (LengthStack > HalfSize){
						Lengthdev	= LengthStack - HalfSize;
						Lengthrem	= HalfSize - (LengthStack - pSgList->List[i].length);
						_outmd((BM_MEM_OUT_Length + (0x40 * (l + k + 1))), Lengthrem);
						//--------------------------------------------
						// Store parameters for final block
						//--------------------------------------------
						if (j == (LenNum - 1)){
							if (OverHalf == 0){
								add_cal								= (unsigned long long)(add_cal_n + Lengthrem);
								ApiMemBmDivideAddress(add_cal, &add_low_o, &add_high_o);
								pSgList->MemParam.FIFOOverAdd		= add_low_o;
								pSgList->MemParam.FIFOOverAdd_high	= add_high_o;
								OverHalf		= 1;
							} else {
								pSgList->MemParam.FIFOOverAdd		= add_low_o_dev;
								pSgList->MemParam.FIFOOverAdd_high	= add_high_o_dev;
								OverHalf		= 0;
							}
							pSgList->MemParam.SGOverAdd			= sg_dma_address(&pSgList->List[0]);
							pSgList->MemParam.SGOverAdd_high	= (sg_dma_address(&pSgList->List[0]) >> 32 & 0xffffffff);
							pSgList->MemParam.OverLen			= Lengthdev;
							pSgList->MemParam.ATNumber			= 0;	
							LengthStack							= Lengthdev;
						} else {
							//--------------------------------------------
							// Specify the beginning of the next Descriptor Format in NextDescPtr
							//--------------------------------------------
							k++;
							desc_cal	= (unsigned long long)(BM_MEM_OUT_NXT_DESC + (0x40 * (l + k + 1)));
							ApiMemBmDivideAddress(desc_cal, &desc_low_o_n, &desc_high_o_n);
							//--------------------------------------------
							// Set Descriptor Format to Prefetcher FIFO
							//--------------------------------------------
							_outmd((BM_MEM_OUT_Write_SGAddr + (0x40 * (l + k + 1))), add_low_o_dev);
							_outmd((BM_MEM_OUT_Write_SGAddr_high + (0x40 * (l + k + 1))), add_high_o_dev);
							_outmd((BM_MEM_OUT_Read_SGAddr + (0x40 * (l + k + 1))), (((sg_dma_address(&pSgList->List[i]) + (pSgList->List[i].length - Lengthdev)) & ~dwdata) + (BM_MEM_CRA_SELECT * (i + BM_MEM_AT_OFFSET))));
							_outmd((BM_MEM_OUT_Read_SGAddr_high + (0x40 * (l + k + 1))), (sg_dma_address(&pSgList->List[i]) >> 32 & 0xffffffff));
							_outmd((BM_MEM_OUT_Length + (0x40 * (l + k + 1))), Lengthdev);
							_outmd((BM_MEM_OUT_NextDescPtr + (0x40 * (l + k + 1))), desc_low_o_n);
							_outmd((BM_MEM_OUT_NextDescPtr_high + (0x40 * (l + k + 1))), desc_high_o_n);
							_outmw((BM_MEM_OUT_SequenceNum + (0x40 * (l + k + 1))), (l + k + 1));
							_outmd((BM_MEM_OUT_Stride + (0x40 * (l + k + 1))), 0x0);
							_outmw((BM_MEM_OUT_Burst + (0x40 * (l + k + 1))), 0x101);
							_outmd((BM_MEM_OUT_Descriptor + (0x40 * (l + k + 1))), 0x40000000);
							//--------------------------------------------
							// If the block exceeds the 4kByte boundary, reconfigure from the beginning of FIFO
							//--------------------------------------------
							add_cal		= (unsigned long long)(add_cal_n + Lengthrem);
							ApiMemBmDivideAddress(add_cal, &add_low_o, &add_high_o);
							if (add_low_o >= 0x9004000){
								add_cal		= (unsigned long long)(add_cal_dev + Lengthdev);
								if (OverHalf == 0){
									SetDesc		= 0;
									OverHalf	= 1;
								} else if (OverHalf == 1){
									SetDesc		= 1;
									OverHalf	= 0;
								}
							} else {
								if (OverHalf == 0){
									SetDesc		= 0;
									OverHalf	= 1;
								} else if (OverHalf == 1){
									SetDesc		= 1;
									OverHalf	= 0;
								}
								//--------------------------------------------
								// Set FIFO address for next list
								//--------------------------------------------
								ApiMemBmDivideAddress(add_cal, &add_low_o, &add_high_o);
								_outmd((BM_MEM_OUT_Write_SGAddr + (0x40 * (l + k + 1))), add_low_o);
								_outmd((BM_MEM_OUT_Write_SGAddr_high + (0x40 * (l + k + 1))), add_high_o);
							}
						}
						LengthStack	= Lengthdev;
						LastList	= 1;
					}
				}
			}
			l++;
			if (LastList == 1){
				break;
			}
		}
		if (LastList == 1){
			break;
		}
	}
	//============================================
	// Descriptor List Store parameters for reconfiguring
	//============================================
	if(dwDir == BM_DIR_IN){
		pMasAddr->BmInp.resetListMove	= l;
		pMasAddr->BmInp.atstartlen		= LengthStack;
		pMasAddr->BmInp.resetOverHalf	= OverHalf;
		if ((i == (ListSize - 1)) && (LastList == 1)){
			pSgList->ListNumCnt	= 0;
		} else {
			pSgList->ListNumCnt	= i;
		}
	}else {
		pMasAddr->BmOut.resetListMove	= l;
		pMasAddr->BmOut.atstartlen		= LengthStack;
		pMasAddr->BmOut.resetOverHalf	= OverHalf;
		pSgList->ListNumCnt				= i * j;
	}
	pSgList->ATOverCnt				= 0;
	return	BM_ERROR_SUCCESS;
}

//========================================================================
// Function name  : MemBmSetSGAddressRingEnd
// Function	 	  : Set the final S/G list in hardware. (Infinite transfer)
// I/F			  : Internal
// In			  : pMasAddr			: Point to the MASTERADDR structure
// 					dwDir				: BM_DIR_IN / BM_DIR_OUT
// Out			  : 
// Return value	  : Normal completed	: BM_ERROR_SUCCESS
// 					Abnormal completed	: BM_ERROR_PARAM / BM_ERROR_BUFF
//========================================================================
long MemBmSetSGAddressRingEnd(PMASTERADDR pMasAddr, unsigned long dwDir, unsigned long stopnum)
{
	unsigned long		i;
	PSGLIST 			pSgList = 0;
	unsigned long long	desc_cal;
	unsigned long long	add_cal = 0, add_cal_n = 0;
	unsigned long long	add_cal_res;
	unsigned long long	add_cal_dev;
	unsigned long		dwdata;
	unsigned long		desc_low_i_n, desc_high_i_n;
	unsigned long		cradata, cradata2;
	unsigned long		add_low_i, add_high_i;
	unsigned long		add_low_i_dev, add_high_i_dev;
	unsigned long		ListSize;
	unsigned long		LengthStack = 0, Lengthdev = 0, Lengthrem = 0, LengthStack_end = 0;
	unsigned long		j = 0, k = 0, l = 0;
	unsigned long		OverHalf = 0, loop = 0;
	unsigned long		LenSize;
	unsigned long		LenNum;
	unsigned long		ListStart = 0, ListEnd = 0, HalfSize = 0, CurrentSize = 0;
	int					SetDesc, LastList;
	int					NextStart, OverFIFO;
	
	//--------------------------------------------
	// Checking parameter
	//--------------------------------------------
	if(dwDir != BM_DIR_IN && dwDir != BM_DIR_OUT){
		return	BM_ERROR_PARAM;
	}
	if(dwDir == BM_DIR_IN){
		pSgList	= &pMasAddr->BmInp.SgList;
	}
	if(pSgList->nr_pages > BM_MEM_MAX_SG_SIZE){
		return	BM_ERROR_BUFF;
	}
	//---------------------------------------------
	// Setting FIFO address
	//---------------------------------------------
	if(dwDir & BM_DIR_IN){
		add_low_i	= pSgList->MemParam.FIFOOverAdd;
		add_high_i	= pSgList->MemParam.FIFOOverAdd_high;
		add_cal		= (unsigned long long)(add_low_i + ((unsigned long long)add_high_i << 32));
		pMasAddr->BmInp.addtablenum	= 0;
	}
	add_cal_n	= add_cal;
	ListSize	= pSgList->nr_pages;
	//--------------------------------------------
	// If the number of SGList exceeds a certain number, limit the number of lists for reconfiguration
	//--------------------------------------------
	if(dwDir & BM_DIR_IN){
		OverHalf	= pMasAddr->BmInp.resetOverHalf;
		LenSize		= 0;
		LenNum		= 1;
		CurrentSize	= add_low_i - 0x7000000;
		LengthStack	= CurrentSize;
		LengthStack_end	= stopnum;
		for (loop = 0; loop < ListSize; loop++) {
			LenSize		+=  pMasAddr->BmInp.SgList.List[loop].length;
			if (LenSize > stopnum){
				ListEnd		= loop + 1;
				if (loop > 0){
					LengthStack_end	= stopnum - (LenSize - pMasAddr->BmInp.SgList.List[loop].length);
				}
				break;
			}
			
		}
		HalfSize	= LenSize;
		OverFIFO	= 0;
		if ((CurrentSize + stopnum) > 0x4000){
			OverFIFO	= 1;
		}
		if (OverHalf == 1){
			l	= pMasAddr->BmInp.resetListMove;
			k	= 5;
		} else {
			k	= 0;
			l	= 0;
		}
		add_cal_dev	= (unsigned long long)BM_MEM_INP_ReadAdd;
		ApiMemBmDivideAddress(add_cal_dev, &add_low_i_dev, &add_high_i_dev);
	}
	ListStart	= pSgList->ListNumCnt;
	Lengthdev	= 0;
	Lengthrem	= 0;
	SetDesc		= 0;
	LastList	= 0;
	NextStart	= 0;
	j			= 0;
	//--------------------------------------------
	// Loop the number of times
	//--------------------------------------------
	for (j = 0; j < LenNum; j++) {
		for (i = 0; i < ListEnd; i++) {
			//--------------------------------------------
			// Set in master register
			//--------------------------------------------
			if(dwDir == BM_DIR_IN){
				if (j == 0){
					//--------------------------------------------
					// Set the Address Translation Table
					//--------------------------------------------
					if (pMasAddr->add64flag == 1){
						_outmd((BM_MEM_CRA + (0x8 * i)), 0xFFFFFFFD);
						dwdata		= _inmd(BM_MEM_CRA + (0x8 * i));
						_outmd((BM_MEM_CRA + (0x8 * i)), ((dwdata & sg_dma_address(&pSgList->List[i])) | 0x1));
						_outmd((BM_MEM_CRA_HIGH + (0x8 * i)), (sg_dma_address(&pSgList->List[i]) >> 32 & 0xffffffff));
						cradata		= _inmd(BM_MEM_CRA + (0x8 * i));
						cradata2	= _inmd(BM_MEM_CRA_HIGH + (0x8 * i));
					} else {
						_outmd((BM_MEM_CRA + (0x8 * i)), 0xFFFFFFFC);
						dwdata		= _inmd(BM_MEM_CRA + (0x8 * i));
						_outmd((BM_MEM_CRA + (0x8 * i)), (dwdata & sg_dma_address(&pSgList->List[i])));
						_outmd((BM_MEM_CRA_HIGH + (0x8 * i)), (sg_dma_address(&pSgList->List[i]) >> 32 & 0xffffffff));
						cradata		= _inmd(BM_MEM_CRA + (0x8 * i));
						cradata2	= _inmd(BM_MEM_CRA_HIGH + (0x8 * i));
					}
				}
				if (i == (ListEnd - 1)){
					LengthStack		+= LengthStack_end;
				} else {
					LengthStack	+= pSgList->List[i].length;
				}
				//--------------------------------------------
				// Move FIFO address for second and subsequent lists
				//--------------------------------------------
				if ((i > 1) && (j == 0)){
					if (SetDesc == 1){
						SetDesc	= 0;
					} else {
						add_cal	= (unsigned long long)(add_cal_n + pSgList->List[i - 1].length);
					}
				} else if ((i == 1) && (j == 0)){
					add_cal		= (unsigned long long)(add_cal_n + pSgList->List[i - 1].length);
					NextStart	= 1;
				} else if (j > 0){
					if (SetDesc == 1){
						SetDesc		= 0;
					} else {
						if (i > 0){
							add_cal	= (unsigned long long)(add_cal_n + pSgList->List[i - 1].length);
						} else {
							add_cal	= (unsigned long long)(add_cal_n + pSgList->List[ListSize - 1].length);
						}
					}
				}
				ApiMemBmDivideAddress(add_cal, &add_low_i, &add_high_i);
				add_cal_n	= add_cal;
				//--------------------------------------------
				// Set the beginning of FIFO if the block size exceeds half of the transfer size
				//--------------------------------------------
				if (LengthStack >= 0x4000){
					Lengthdev	= LengthStack - 0x4000;
					if (i == (ListEnd - 1)){
						Lengthrem	= 0x4000 - (LengthStack - LengthStack_end);
					} else {
						Lengthrem	= 0x4000 - (LengthStack - pSgList->List[i].length);
					}
				}
				//--------------------------------------------
				// Specify the beginning of the next Descriptor Format in NextDescPtr
				//--------------------------------------------
				desc_cal	= (unsigned long long)(BM_MEM_INP_NXT_DESC + (0x40 * (l + k + 1)));
				ApiMemBmDivideAddress(desc_cal, &desc_low_i_n, &desc_high_i_n);
				//--------------------------------------------
				// Set Descriptor Format to Prefetcher FIFO
				//--------------------------------------------
				if ((j == 0) && (i == 0)){
					_outmd((BM_MEM_INP_Write_SGAddr + (0x40 * (l + k + 1))), ((sg_dma_address(&pSgList->List[i]) & ~dwdata) + (BM_MEM_CRA_SELECT * i)));
					_outmd((BM_MEM_INP_Write_SGAddr_high + (0x40 * (l + k + 1))), (sg_dma_address(&pSgList->List[i]) >> 32 & 0xffffffff));
					_outmd((BM_MEM_INP_Length + (0x40 * (l + k + 1))), pSgList->List[i].length);
					pSgList->MemParam.ATPreFetchAdd	= (BM_MEM_INP_NXT_DESC + (0x40 * (l + k)));
					NextStart	= 1;
				} else if ((i == 0) && (NextStart == 0) && (j == 1)){
					_outmd((BM_MEM_INP_Write_SGAddr + (0x40 * (l + k + 1))), ((pSgList->MemParam.SGOverAdd & ~dwdata) + (BM_MEM_CRA_SELECT * i)));
					_outmd((BM_MEM_INP_Write_SGAddr_high + (0x40 * (l + k + 1))), pSgList->MemParam.SGOverAdd_high);
					_outmd((BM_MEM_INP_Length + (0x40 * (l + k + 1))), pSgList->MemParam.OverLen);
					pSgList->MemParam.ATPreFetchAdd	= (BM_MEM_INP_NXT_DESC + (0x40 * (l + k)));
					NextStart	= 1;
				} else {
					_outmd((BM_MEM_INP_Write_SGAddr + (0x40 * (l + k + 1))), ((sg_dma_address(&pSgList->List[i]) & ~dwdata) + (BM_MEM_CRA_SELECT * i)));
					_outmd((BM_MEM_INP_Write_SGAddr_high + (0x40 * (l + k + 1))), (sg_dma_address(&pSgList->List[i]) >> 32 & 0xffffffff));
					_outmd((BM_MEM_INP_Length + (0x40 * (l + k + 1))), pSgList->List[i].length);
				}
				_outmd((BM_MEM_INP_Read_SGAddr + (0x40 * (l + k + 1))), add_low_i);
				_outmd((BM_MEM_INP_Read_SGAddr_high + (0x40 * (l + k + 1))), add_high_i);
				_outmd((BM_MEM_INP_NextDescPtr + (0x40 * (l + k + 1))), desc_low_i_n);
				_outmd((BM_MEM_INP_NextDescPtr_high + (0x40 * (l + k + 1))), desc_high_i_n);
				_outmw((BM_MEM_INP_SequenceNum + (0x40 * (l + k + 1))), (l+ k + 1));
				_outmd((BM_MEM_INP_Stride + (0x40 * (l + k + 1))), 0x0);
				_outmw((BM_MEM_INP_Burst + (0x40 * (l + k + 1))), 0x101);
				_outmd((BM_MEM_INP_Descriptor + (0x40 * (l + k + 1))), 0x40000000);
				//--------------------------------------------
				// Transfer setting of final transfer block
				//--------------------------------------------
				if ((LengthStack <= 0x4000) && (i == (ListEnd - 1))){
					if ((LengthStack == HalfSize) && (OverHalf == 1)){
						add_cal_res	= (unsigned long long)BM_MEM_INP_ReadAdd;
					} else {
						add_cal_res	= (unsigned long long)(add_cal_n + pSgList->List[i].length);
					}
					pMasAddr->BmInp.resetadd	= add_cal_res;
					//--------------------------------------------
					//Set NextDescPtr to the beginning of Prefetcher
					//--------------------------------------------
					if (j == (LenNum - 1)) {
						desc_cal	= (unsigned long long)BM_MEM_INP_TOP_DESC;
						ApiMemBmDivideAddress(desc_cal, &desc_low_i_n, &desc_high_i_n);
						_outmd((BM_MEM_INP_NextDescPtr + (0x40 * (l + k + 1))), desc_low_i_n);
						_outmd((BM_MEM_INP_NextDescPtr_high + (0x40 * (l + k + 1))), desc_high_i_n);
						_outmd((BM_MEM_INP_Length + (0x40 * (l + k + 1))), (LengthStack_end));
						_outmd((BM_MEM_INP_Descriptor + (0x40 * (l + k + 1))), 0x40004000);
					}
					//--------------------------------------------
					// Descriptor Store parameters for reconfiguring
					//--------------------------------------------
					if (LengthStack == HalfSize){
						if (OverHalf == 0){
							OverHalf	= 1;
						} else {
							OverHalf	= 0;
						}
						add_cal								= (unsigned long long)(add_cal_n + pSgList->List[i].length);
						ApiMemBmDivideAddress(add_cal, &add_low_i, &add_high_i);
						pSgList->MemParam.FIFOOverAdd		= add_low_i;
						pSgList->MemParam.FIFOOverAdd_high	= add_high_i;
						pSgList->MemParam.SGOverAdd			= sg_dma_address(&pSgList->List[0]);
						pSgList->MemParam.SGOverAdd_high	= (sg_dma_address(&pSgList->List[0]) >> 32 & 0xffffffff);
						pSgList->MemParam.OverLen			= pSgList->List[0].length;
						pSgList->MemParam.ATNumber			= 0;
						LengthStack							= Lengthdev;
						LastList							= 1;
					}
				}
				//--------------------------------------------
				// Split transfer size of 1 block
				//--------------------------------------------
				if (LengthStack > 0x4000){
					Lengthdev	= LengthStack - 0x4000;
					if (i == (ListEnd - 1)){
						Lengthrem	= 0x4000 - (LengthStack - LengthStack_end);
					} else {
						Lengthrem	= 0x4000 - (LengthStack - pSgList->List[i].length);
					}
					_outmd((BM_MEM_INP_Length + (0x40 * (l + k + 1))), Lengthrem);
					//--------------------------------------------
					// Specify the beginning of the next Descriptor Format in NextDescPtr
					//--------------------------------------------
					k++;
					desc_cal	= (unsigned long long)(BM_MEM_INP_NXT_DESC + (0x40 * (l + k + 1)));
					ApiMemBmDivideAddress(desc_cal, &desc_low_i_n, &desc_high_i_n);
					//--------------------------------------------
					// Set Descriptor Format to Prefetcher FIFO
					//--------------------------------------------
					_outmd((BM_MEM_INP_Read_SGAddr + (0x40 * (l + k + 1))), add_low_i_dev);
					_outmd((BM_MEM_INP_Read_SGAddr_high + (0x40 * (l + k + 1))), add_high_i_dev);
					if (i == (ListEnd - 1)){
						_outmd((BM_MEM_INP_Write_SGAddr + (0x40 * (l + k + 1))), (((sg_dma_address(&pSgList->List[i]) + (LengthStack_end - Lengthdev)) & ~dwdata) + (BM_MEM_CRA_SELECT * i)));
					} else {
						_outmd((BM_MEM_INP_Write_SGAddr + (0x40 * (l + k + 1))), (((sg_dma_address(&pSgList->List[i]) + (pSgList->List[i].length - Lengthdev)) & ~dwdata) + (BM_MEM_CRA_SELECT * i)));
					}
					_outmd((BM_MEM_INP_Write_SGAddr_high + (0x40 * (l + k + 1))), (sg_dma_address(&pSgList->List[i]) >> 32 & 0xffffffff));
					_outmd((BM_MEM_INP_Length + (0x40 * (l + k + 1))), Lengthdev);
					_outmd((BM_MEM_INP_NextDescPtr + (0x40 * (l + k + 1))), desc_low_i_n);
					_outmd((BM_MEM_INP_NextDescPtr_high + (0x40 * (l + k + 1))), desc_high_i_n);
					_outmw((BM_MEM_INP_SequenceNum + (0x40 * (l + k + 1))), (l + k + 1));
					_outmd((BM_MEM_INP_Stride + (0x40 * (l + k + 1))), 0x0);
					_outmw((BM_MEM_INP_Burst + (0x40 * (l + k + 1))), 0x101);
					_outmd((BM_MEM_INP_Descriptor + (0x40 * (l + k + 1))), 0x40004000);
					//--------------------------------------------
					// If the block exceeds the 4kByte boundary, reconfigure from the beginning of FIFO
					//--------------------------------------------
					if (i == (ListEnd - 1)){
						desc_cal	= (unsigned long long)BM_MEM_INP_TOP_DESC;
						ApiMemBmDivideAddress(desc_cal, &desc_low_i_n, &desc_high_i_n);
						_outmd((BM_MEM_INP_NextDescPtr + (0x40 * (l + k + 1))), desc_low_i_n);
						_outmd((BM_MEM_INP_NextDescPtr_high + (0x40 * (l + k + 1))), desc_high_i_n);
					}
					LengthStack	= Lengthdev;
					LastList	= 1;
				}
			}
			l++;
			if (LastList == 1){
				break;
			}
		}
		if (LastList == 1){
			break;
		}
	}
	//============================================
	// Descriptor List Store parameters for reconfiguring
	//============================================
	if(dwDir == BM_DIR_IN){
		pMasAddr->BmInp.resetListMove	= l;
		pMasAddr->BmInp.atstartlen		= LengthStack;
		pMasAddr->BmInp.resetOverHalf	= OverHalf;
		if ((i == (ListSize - 1)) && (LastList == 1)){
			pSgList->ListNumCnt	= 0;
		} else {
			pSgList->ListNumCnt	= i;
		}
	}
	pSgList->ATOverCnt				= 0;
	return	BM_ERROR_SUCCESS;
}

//========================================================================
// Function name  : ApiMemBmUnlockMem
// Function		  : Unlock the memory.
// I/F			  : External
// In			  : pMasAddr			: Point to the MASTERADDR structure
// 					dwDir				: Transfer direction (BM_DIR_IN / BM_DIR_OUT)
// Out			  : pMasAddr			: Point to the MASTERADDR structure
// Return value	  : 
// Addition		  : Call at the end of transfer.
//========================================================================
long ApiMemBmUnlockMem(PMASTERADDR pMasAddr, unsigned long dwDir)
{
	PSGLIST			pSgList;
	int				direction;
	unsigned long	i;

	//--------------------------------------------
	// Check the parameters
	//--------------------------------------------
	if (dwDir != BM_DIR_IN && dwDir != BM_DIR_OUT) {
		return BM_ERROR_PARAM;
	}
	//--------------------------------------------
	// Get the SgList structure
	//--------------------------------------------
	if (dwDir == BM_DIR_IN) {
		pSgList	= &pMasAddr->BmInp.SgList;
	} else {
		pSgList	= &pMasAddr->BmOut.SgList;
	}
	//--------------------------------------------
	// Check if the SgList structure is in use
	//--------------------------------------------
	if(pSgList->dwBuffLen == 0){
		return	BM_ERROR_SUCCESS;
	}
	//--------------------------------------------
	// Specify the transfer direction
	//--------------------------------------------
	if(dwDir == BM_DIR_OUT){
		direction	= PCI_DMA_TODEVICE;
	}else{
		direction	= PCI_DMA_FROMDEVICE;
	}
	//--------------------------------------------
	// Unmap the scatter list
	//--------------------------------------------
	pci_unmap_sg(&pMasAddr->PciDev, pSgList->List, pSgList->nr_pages, direction);
	//---------------------------
	// Unmap the user I/O buffer
	//---------------------------
	for (i = 0; i < pSgList->nr_pages; i++) {
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,0) && LINUX_VERSION_CODE < KERNEL_VERSION(2,6,24))
		page_cache_release(pSgList->List[i].page);
#elif ((LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,24) && LINUX_VERSION_CODE < KERNEL_VERSION(2,7,0)) || \
	    (LINUX_VERSION_CODE >= KERNEL_VERSION(3,10,0) && LINUX_VERSION_CODE < KERNEL_VERSION(4,15,0)))
		page_cache_release((struct page*)pSgList->List[i].page_link);
#elif (LINUX_VERSION_CODE >= KERNEL_VERSION(4,15,0))
		put_hwpoison_page((struct page*)pSgList->List[i].page_link);
#endif
	}
	//---------------------------
	// Free the S/G list memory area
	//---------------------------
	Ccom_free_pages((unsigned long)&pSgList->List[0], (pSgList->nr_pages * sizeof(struct scatterlist)));
	//---------------------------
	// Initialize the number of used pages
	//---------------------------
	pSgList->nr_pages	= 0;
	//---------------------------
	// Initialize the size information of user memory
	//---------------------------
	pSgList->dwBuffLen	= 0;
	//---------------------------
	// Initialize the point to the user memory with NULL
	//---------------------------
	pSgList->Buff		= NULL;
	//---------------------------
	// Initialize the transfer direction
	//---------------------------
	pSgList->dwIsRing	= BM_WRITE_ONCE;
	return	BM_ERROR_SUCCESS;
}

//========================================================================
// Function name  : ApiMemBmStart
// Function	 	  : Start the BusMaster transfer.
// I/F			  : External
// In			  : pMasAddr			: Point to the MASTERADDR structure
// 					dwDir				: Transfer direction (BM_DIR_IN, BM_DIR_OUT or their logical sum)
// Out			  : Output to the transfer started board
// Return value	  : Normal completed	: BM_ERROR_SUCCESS
// 					Abnormal completed	: BM_ERROR_PARAM
//========================================================================
long ApiMemBmStart(PMASTERADDR pMasAddr, unsigned long dwDir)
{
	unsigned long		dwIntMask;
	unsigned long long	desc_cal;
	unsigned long		desc_low_i_n, desc_high_i_n;
	unsigned long		desc_low_o_n, desc_high_o_n;
	
	//---------------------------------------------
	// Check the parameters
	//---------------------------------------------
	if (dwDir != BM_DIR_IN &&
		dwDir != BM_DIR_OUT &&
		dwDir != (BM_DIR_IN | BM_DIR_OUT)) {
		return	BM_ERROR_PARAM;
	}
	//---------------------------------------------
	// Check the buffer setting
	//---------------------------------------------
	if (dwDir & BM_DIR_IN) {
		if (pMasAddr->BmInp.SgList.Buff == NULL) {
			return BM_ERROR_SEQUENCE;
		}
	}
	if (dwDir & BM_DIR_OUT) {
		if (pMasAddr->BmOut.SgList.Buff == NULL) {
			return BM_ERROR_SEQUENCE;
		}
	}
	//---------------------------------------------
	// Clearing interrupt and setting interrupt mask
	//---------------------------------------------
	if ((dwDir & BM_DIR_IN) && (pMasAddr->BmInp.dmastopflag == 1)) {
		//---------------------------------------------
		// Clear and set the variables
		//---------------------------------------------
		pMasAddr->BmInp.dwCarryCount	= 0;
		pMasAddr->BmInp.dwCarryCount2	= 0;
		pMasAddr->BmInp.dwIntSence		= 0;
		pMasAddr->BmInp.dwIntMask		= ~(	BM_STATUS_COUNT |
												BM_STATUS_TDCARRY |
												BM_STATUS_FIFO_REACH);
		_outmw(BM_MEM_INP_IntMask, pMasAddr->BmInp.dwIntMask);
		pMasAddr->BmInp.dwIntMask		= ~(	BM_STATUS_COUNT |
												BM_STATUS_TDCARRY |
												BM_STATUS_SG_OVER_IN |
												BM_STATUS_FIFO_FULL |
												BM_STATUS_FIFO_REACH |
												BM_STATUS_ALL_END);
		dwIntMask						= (pMasAddr->BmInp.dwIntMask & 0xffff);
		_outmw(BM_MEM_INP_IntClear, ~dwIntMask);
		_outmd((BM_MEM_INP_CSR + 0x4), 0x8);
		//---------------------------------------------
		//	Processing for clearing DMA controller and status
		//---------------------------------------------
		_outmd(BM_MEM_INP_CSR, 0x00000200);
		_outmd((BM_MEM_INP_CSR + 0x4), 0x02);
		_outmd(BM_MEM_INP_CSR, 0x0000);
	}
	if ((dwDir & BM_DIR_OUT) && (pMasAddr->BmOut.dmastopflag == 1)) {
		pMasAddr->BmOut.dwCarryCount	= 0;
		pMasAddr->BmOut.dwCarryCount2	= 0;
		pMasAddr->BmOut.dwIntSence		= 0;
		pMasAddr->BmOut.dwIntMask		= ~(	BM_STATUS_COUNT |
												BM_STATUS_TDCARRY |
												BM_STATUS_FIFO_REACH);
		_outmw(BM_MEM_OUT_IntMask, pMasAddr->BmOut.dwIntMask);
		pMasAddr->BmOut.dwIntMask		= ~(	BM_STATUS_COUNT |
												BM_STATUS_TDCARRY |
												BM_STATUS_FIFO_EMPTY |
												BM_STATUS_FIFO_REACH |
												BM_STATUS_ALL_END);
		dwIntMask						= (pMasAddr->BmOut.dwIntMask & 0xffff);
		_outmw(BM_MEM_OUT_IntClear, ~dwIntMask);
		_outmd((BM_MEM_OUT_CSR + 0x4), 0x8);
		//---------------------------------------------
		//	Processing for clearing DMA controller and status
		//---------------------------------------------
		_outmd(BM_MEM_OUT_CSR, 0x00000200);
		_outmd((BM_MEM_OUT_CSR + 0x4), 0x02);
		_outmd(BM_MEM_OUT_CSR, 0x0000);
	}
	//---------------------------------------------
	//	Processing for DMA start
	//---------------------------------------------
	if(((dwDir & BM_DIR_IN) == BM_DIR_IN) && (pMasAddr->BmInp.dmastopflag == 1)){
		//--------------------------------------------
		// Initializing the flag and counter
		//--------------------------------------------
		pMasAddr->BmInp.outcnt			= 0;
		pMasAddr->BmInp.atstartflag		= 0;
		pMasAddr->BmInp.dmastopflag		= 0;
		//---------------------------------------------
		// Set start address of Descriptor list
		//---------------------------------------------
		desc_cal		= (unsigned long long)BM_MEM_INP_NXT_DESC;
		ApiMemBmDivideAddress(desc_cal, &desc_low_i_n, &desc_high_i_n);
		_outmd((BM_MEM_INP_Prefetchar_CSR + 0x4), desc_low_i_n);
		_outmd((BM_MEM_INP_Prefetchar_CSR + 0x8), desc_high_i_n);
	}
	if(((dwDir & BM_DIR_OUT) == BM_DIR_OUT) && (pMasAddr->BmOut.dmastopflag == 1)){
		//--------------------------------------------
		// Initializing the flag and counter
		//--------------------------------------------
		pMasAddr->BmOut.endsmp			= 0;
		pMasAddr->BmOut.outcnt			= 1;
		pMasAddr->BmOut.atstartflag		= 0;
		pMasAddr->BmOut.cntflag			= 0;
		pMasAddr->BmOut.ringstartflag	= 0;
		pMasAddr->BmOut.dmastopflag		= 0;
		//--------------------------------------------
		// Set the interrupt flag of transfer completion
		//--------------------------------------------
		if (pMasAddr->BmOut.SgList.dwIsRing == 1){
			if (pMasAddr->BmOut.sglen > 2048){
				_outmd((pMasAddr->dwPort_mem + (pMasAddr->BmOut.SgList.ChangeDescAdd[1] + 0x3C)), 0x40004000);
			}
		} else {
			if (pMasAddr->BmOut.sglen > 2048){
				_outmd((pMasAddr->dwPort_mem + (pMasAddr->BmOut.SgList.ChangeDescAdd[1] + 0x3C)), 0x40004000);
			}
		}
		//---------------------------------------------
		// Set start address of Descriptor list
		//---------------------------------------------
		desc_cal		= (unsigned long long)BM_MEM_OUT_NXT_DESC;
		ApiMemBmDivideAddress(desc_cal, &desc_low_o_n, &desc_high_o_n);
		_outmd((BM_MEM_OUT_Prefetchar_CSR + 0x4), desc_low_o_n);
		_outmd((BM_MEM_OUT_Prefetchar_CSR + 0x8), desc_high_o_n);
		//---------------------------------------------
		// Set transfer start command to Prefetcher CSR
		//---------------------------------------------
		if (pMasAddr->BmOut.SgList.dwIsRing == 0){
			_outmd(BM_MEM_OUT_Prefetchar_CSR, 0x9);
		} else {
			if ((pMasAddr->BmOut.sglen % 32) == 0){
				_outmd(BM_MEM_OUT_Prefetchar_CSR, 0x19);
			} else {
				_outmd(BM_MEM_OUT_Prefetchar_CSR, 0x9);
			}
		}
	}
	if((dwDir & BM_DIR_IN) == 0){
		pMasAddr->BmInp.dwIntSence = 0;
		pMasAddr->BmInp.dwCountSenceNum = 0;
		pMasAddr->BmInp.dwEndSenceNum = 0;
	}
	if((dwDir & BM_DIR_OUT) == 0){
		pMasAddr->BmOut.dwIntSence = 0;
		pMasAddr->BmOut.dwCountSenceNum = 0;
		pMasAddr->BmOut.dwEndSenceNum = 0;
	}
	return	BM_ERROR_SUCCESS;
}

//========================================================================
// Function name  : ApiBmStop
// Function	      : Force exiting the BusMaster transfer.
// I/F			  : External
// In			  : pMasAddr			: Point to the MASTERADDR structure
// 				    dwDir				: Transfer direction (BM_DIR_IN, BM_DIR_OUT or their logical sum)
// Out			  : Output to the transfer ended board
// Return value	  : Normal completed	: BM_ERROR_SUCCESS
// 					Abnormal completed	: BM_ERROR_PARAM
//========================================================================
long ApiMemBmStop(PMASTERADDR pMasAddr, unsigned long dwDir)
{
	long			lret;

	//---------------------------------------------
	// Checking parameter
	//---------------------------------------------
	if (dwDir != BM_DIR_IN &&
		dwDir != BM_DIR_OUT &&
		dwDir != (BM_DIR_IN | BM_DIR_OUT)) {
		return	BM_ERROR_PARAM;
	}
	//---------------------------------------------
	// Stop DMA transfer
	//---------------------------------------------
	if((dwDir & BM_DIR_IN) == BM_DIR_IN){
		mdelay(10);
		_outmd(BM_MEM_INP_Prefetchar_CSR, 0x4);
		mdelay(1);
		_outmd(BM_MEM_INP_CSR + 0x4, BM_MEM_STOP_Dispatcher);
		mdelay(1);
		_outmd(BM_MEM_INP_CSR + 0x4, BM_MEM_RESET_Dispatcher);
		mdelay(1);
	}
	if((dwDir & BM_DIR_OUT) == BM_DIR_OUT){
		mdelay(10);
		_outmd(BM_MEM_OUT_Prefetchar_CSR, 0x4);
		mdelay(1);
		_outmd(BM_MEM_OUT_CSR + 0x4, BM_MEM_STOP_Dispatcher);
		mdelay(1);
		_outmd(BM_MEM_OUT_CSR + 0x4, BM_MEM_RESET_Dispatcher);
		mdelay(1);
	}
	lret = BM_ERROR_SUCCESS;
	return	lret;
}

//========================================================================
// Function name  : MemBmSubGetCount
// Function		  : Retrieve the transfer count after executed ApiBmStart (call synchronized with interrupt)
// I/F			  : Internal
// In			  : 
// Out			  : data				: parameter
// Return value	  : 
//========================================================================
void MemBmSubGetCount(void *data)
{
	PBM_GET_CNT_SUB	param;
	PMASTERADDR		pMasAddr;
	unsigned long	dwDir, *dwCount, *dwCarry;
	unsigned long	dwDataCount;
	PBMEMBER		pBm;
	unsigned long	dwIntSence;
	unsigned long	dwInSence;
	unsigned long	dwOutSence;

	//---------------------------------------------
	// Initialize the variables
	//---------------------------------------------
	param		= (PBM_GET_CNT_SUB)data;
	pMasAddr	= param->pMasAddr;
	dwDir		= param->dwDir;
	dwCount		= param->dwCount;
	dwCarry		= param->dwCarry;
	*dwCarry	= 0;
	//---------------------------------------------
	// Calculate the count value
	//---------------------------------------------
	if (dwDir == BM_DIR_IN) {
		dwDataCount	= _inmd(BM_MEM_INP_TotalDataLen);
		if (pMasAddr->BmInp.oddtransferflag == 1){
			dwDataCount--;
		}
		pBm = &pMasAddr->BmInp;
	} else {
		dwDataCount	= _inmd(BM_MEM_OUT_TotalDataLen);
		pBm = &pMasAddr->BmOut;
	}
	//-------------------------------------
	// Calculate the counter carry
	// When the interrupt delay of 100 us occurs
	// at the maximum sampling rate of 50 ns,
	// the counter value of 20000 come up.
	// The carry might be already on as stopping interrupt,
	// check the carry.
	// Since carry is set at 0x00FFFFFFFF, check also in this case.
	//-------------------------------------
	if ((dwDataCount < 20000) | (dwDataCount == 0x00FFFFFFFF)){
		//-------------------------------------
		// Retrieve the interrupt factor
		//-------------------------------------
		dwIntSence	= _inmd(BM_MEM_IntStatus);
		dwInSence	= dwIntSence & 0x7f;
		dwOutSence	= (dwIntSence >> 16) & 0x7f;
		//-------------------------------------
		// Reset interrupt status
		//-------------------------------------
		if (dwDir == BM_DIR_IN) {
			_outmd(BM_MEM_INP_IntClear, BM_STATUS_TDCARRY);
		} else {
			_outmd(BM_MEM_OUT_IntClear, (BM_STATUS_TDCARRY<<16));
		}
		//-------------------------------------
		// Calculate counter carry
		//-------------------------------------
		// Input
		if (dwDir == BM_DIR_IN && dwInSence & BM_STATUS_TDCARRY) {
			pMasAddr->BmInp.dwCarryCount++;
		}
		// Output
		if (dwDir == BM_DIR_OUT && dwOutSence & BM_STATUS_TDCARRY) {
			pMasAddr->BmOut.dwCarryCount++;
		}
	}
	// BM_MEM_COUNTER_BIT		32				// Features 32 bit counter
	// BM_MEM_COUNTER_MASK		0x00FFFFFFFF	// 32 bit counter mask
	//-------------------------------------
	// Because the hardware carry is set at 0x00FFFFFFFF,
	// when the transfer count value is 0x00FFFFFFFF,
	// the carray count value - 1 is the normal count value
	//-------------------------------------
	//-------------------------------------
	// When the count value is 0x00FFFFFFFF (32bit)
	//-------------------------------------
	if(dwDataCount == 0x00FFFFFFFF){
		//-------------------------------------
		// When the digit of pBm-> dwCarryCount is counting up
		//-------------------------------------
		*dwCount	= dwDataCount & BM_MEM_COUNTER_MASK;
		*dwCarry	= (pBm->dwCarryCount -1) & BM_MEM_COUNTER_MASK;
	//-------------------------------------
	// Others
	//-------------------------------------
	}else{
	*dwCount	= dwDataCount & BM_MEM_COUNTER_MASK;
	*dwCarry	= pBm->dwCarryCount & BM_MEM_COUNTER_MASK;
	}
	//-------------------------------------
	// Even if ResetDevice is executed, the hard count value of the output is not initialized,
	// so the count value is initialized here.
	//-------------------------------------
	if ((dwDir == BM_DIR_OUT) && (pMasAddr->BmOut.resetcountFlag == 1)){
		*dwCount	= 0;
		*dwCarry	= 0;
	}
	if ((dwDir == BM_DIR_IN) && (pMasAddr->BmInp.resetcountFlag == 1)){
		*dwCount	= 0;
		*dwCarry	= 0;
	}
	return;
}

//========================================================================
// Function name  : ApiBmGetCount
// Function		  : Retrieve the transfer count after executed ApiBmStart
// I/F			  : External
// In			  : pMasAddr			: Point to the MASTERADDR structure
// 				    dwDir				: Transfer direction (1: Mem->FIFO / 2: FIFO->Mem)
// Out			  : *dwCount			: Number of transferred data
// :			    *dwCarry			: Carry count
// Return value	  : Normal completed	: BM_ERROR_SUCCESS
// 					Abnormal completed	: BM_ERROR_PARAM
//========================================================================
long ApiMemBmGetCount(PMASTERADDR pMasAddr, unsigned long dwDir, unsigned long *dwCount, unsigned long *dwCarry)
{
	BM_GET_CNT_SUB	sub_data;

	//---------------------------------------------
	// Check the parameters
	//---------------------------------------------
	if (dwDir != BM_DIR_IN && dwDir != BM_DIR_OUT) {
		return	BM_ERROR_PARAM;
	}
	//---------------------------------------------
	// Set the parameters
	//---------------------------------------------
	sub_data.pMasAddr	= pMasAddr;
	sub_data.dwDir		= dwDir;
	sub_data.dwCount	= dwCount;
	sub_data.dwCarry	= dwCarry;
	//-------------------------------------
	// Call directly because the interrupt spinlock is on in the upper layer.
	//-------------------------------------
	MemBmSubGetCount((void *)&sub_data);

	return BM_ERROR_SUCCESS;
}
//========================================================================
// Function name  : ApiBmReset
// Function		  : 
// I/F			  : External
// In			  : pMasAddr			: Point to the MASTERADDR structure
// 					dwResetType			: Reset type
// Out			  : 
// Return value	  : 
//========================================================================
long ApiMemBmReset(PMASTERADDR pMasAddr, unsigned long dwResetType)
{
	long		loop;
	unsigned long 	SampEnable, dwdata;
	//---------------------------------------------
	// Check the parameters
	//---------------------------------------------
	if (dwResetType & ~0x1f) {
		return	BM_ERROR_PARAM;
	}
	//---------------------------------------------
	// Reset
	//---------------------------------------------
	//---------------------------------------------
	// Reset
	// If a register is accessed during generation, 
	// the DMA transfer may be incorrect, so the FIFO reset on the sampling side is not performed 
	// during operation on the generation side.
	// (DioTransferStart () resets the FIFO every time, so the operation itself is not affected)
	//---------------------------------------------
	if ((dwResetType == BM_RESET_FIFO_IN) &&
		(pMasAddr->BmOut.dmastopflag == 1)){
		_outmd(BM_MEM_Reset, (unsigned short)dwResetType);
	}
	if (dwResetType & BM_RESET_ALL) {
		//-----------------------------------------
		// Initializing mask data
		//-----------------------------------------
		pMasAddr->BmOut.dwIntMask	= 0x9f;
		pMasAddr->BmInp.dwIntMask	= 0x9f;
		_outmd(BM_MEM_INP_IntMask, 0xFFFFFFFF);
		_outmw(BM_MEM_BM_DIR, 0x00);
		//-----------------------------------------
		// Initializing interrupt status
		//-----------------------------------------
		pMasAddr->BmInp.dwIntSence		= 0;
		pMasAddr->BmOut.dwIntSence		= 0;
		//-----------------------------------------
		// Initializing status
		//-----------------------------------------
		_outmd(BM_MEM_INP_FifoReach, 0x0);
		_outmd(BM_MEM_OUT_FifoReach, 0x0);
		//-----------------------------------------
	}
	if (dwResetType & BM_DIR_IN) {
		//---------------------------------------------
		// If the stop condition is other than the number stop and the stop does not occur in the previous sampling,
		// the sampling stop flag is cleared because the sampling of the user circuit is not stopped.
		//---------------------------------------------
		SampEnable	= _inmw(BM_MEM_DIO_STOP);
		if ((SampEnable & 0x1) == 0x1){
			SampEnable	&= 0x2;
			_outmw(BM_MEM_DIO_STOP, SampEnable);
		}
	}
	if (pMasAddr->BmInp.SgList.Buff != NULL) {
		_outmd(BM_MEM_INP_Prefetchar_CSR, 0x4);
		dwdata	= _inmd(BM_MEM_INP_Prefetchar_CSR);
		if ((dwdata & 0x4) == 0x4){
			for (loop=0; loop < 1000; loop++){
				msleep(1);
				dwdata = _inmd(BM_MEM_INP_Prefetchar_CSR);
				if ((dwdata & 0x4) != 0x4){
					break;
				}
			}
		}
		_outmd(BM_MEM_INP_CSR, BM_MEM_RESET_Dispatcher);
	} 
	if (pMasAddr->BmOut.SgList.Buff != NULL) {
		_outmd(BM_MEM_OUT_Prefetchar_CSR, 0x4);
		dwdata	= _inmd(BM_MEM_OUT_Prefetchar_CSR);
		if ((dwdata & 0x4) == 0x4){
			for (loop=0; loop < 1000; loop++){
				msleep(1);
				dwdata = _inmd(BM_MEM_OUT_Prefetchar_CSR);
				if ((dwdata & 0x4) != 0x4){
					break;
				}
			}
		}
		_outmd(BM_MEM_OUT_CSR, BM_MEM_RESET_Dispatcher);
	}
	return BM_ERROR_SUCCESS;
}

//========================================================================
// Function name  : ApiBmCheckFifo
// Function		  : Return FIFO counter value
// I/F			  : External
// In			  : pMasAddr			: Point to the MASTERADDR structure
// 					dwDir   			: Transfer direction (BM_DIR_IN or BM_DIR_OUT)
// Out			  : 
// Return value	  : Counter value
//========================================================================
long ApiMemBmCheckFifo(PMASTERADDR pMasAddr, unsigned long dwDir, unsigned long *lpdwFifoCnt)
{
	unsigned long	dwFifoCnt;
	
	//---------------------------------------------
	// Check the parameters
	//---------------------------------------------
	if (dwDir != BM_DIR_IN &&
		dwDir != BM_DIR_OUT) {
		return	BM_ERROR_PARAM;
	}
	//---------------------------------------------
	// Read the counter value
	//---------------------------------------------
	if (dwDir == BM_DIR_IN) {
		dwFifoCnt	= _inmw(BM_MEM_INP_FifoConter);
	} else {
		dwFifoCnt	= _inmw(BM_MEM_OUT_FifoConter);
	}
	*lpdwFifoCnt = dwFifoCnt;
	return BM_ERROR_SUCCESS;
}
//========================================================================
// Function name  : ApiBmSetNotifyCount
// Function		  : Set the data to notify the busmaster transfer completion with the specified number
// I/F			  : External
// In			  : pMasAddr			: Point to the MASTERADDR structure
// Out			  : 
// Return value	  : Normal completed	: BM_ERROR_SUCCESS 
// 					Abnormal completed	: BM_ERROR_PARAM
//========================================================================
long ApiMemBmSetNotifyCount(PMASTERADDR pMasAddr, unsigned long dwDir, unsigned long dwCount)
{
	//---------------------------------------------
	// Check the parameters
	//---------------------------------------------
	if (dwDir != BM_DIR_IN &&
		dwDir != BM_DIR_OUT) {
		return	BM_ERROR_PARAM;
	}
	if (dwCount == 0 || dwCount > 0x00ffffff) {
		return	BM_ERROR_PARAM;
	}
	//---------------------------------------------
	// Set the data
	//---------------------------------------------
	if (dwDir == BM_DIR_IN) {
		_outmd(BM_MEM_INP_IntDataLen, dwCount);
	} else {
		_outmd(BM_MEM_OUT_IntDataLen, dwCount);
	}
	return BM_ERROR_SUCCESS;
}

//========================================================================
// Function name  : ApiBmInterrupt
// Function		  : Interrupt handling routine
// I/F			  : External
// In			  : pMasAddr			: Point to the MASTERADDR structure
// Out			  : 
// Return value	  : BM_INT_EXIST: own interrupt, BM_INT_NONE: other board interrupt
//========================================================================
long ApiMemBmInterrupt(PMASTERADDR pMasAddr, unsigned short *InStatus, unsigned short *OutStatus, unsigned long IntSenceBM)
{
	long			lret;
	unsigned long	dwIntSence, InpIntSence, OutIntSence;
	unsigned long	CarrySave;

	//-------------------------------------
	// Retrieve the interrupt factor
	//-------------------------------------
	dwIntSence	= IntSenceBM;
	//-------------------------------------
	// Reset interrupt status
	//-------------------------------------
	_outmd(BM_MEM_INP_IntClear, (dwIntSence & 0xFF7FFF7F));
	//-------------------------------------
	// Reset interrupt status
	//-------------------------------------
	if (pMasAddr->BmOut.endsmp == 1){
		pMasAddr->BmOut.end_ev_save	= 1;
		pMasAddr->BmOut.endsmp		= 0;
		dwIntSence &=  (~pMasAddr->BmInp.dwIntMask & 0x0077) |
					  ((~pMasAddr->BmOut.dwIntMask << 16) & 0x007f0000);
	} else {
		dwIntSence &=  (~pMasAddr->BmInp.dwIntMask & 0x0077) |
					  ((~pMasAddr->BmOut.dwIntMask << 16) & 0x00770000);
	} 
	if (dwIntSence) {
		lret = BM_INT_EXIST;
	} else {
		lret = BM_INT_NONE;
	}
	//-------------------------------------
	// Isolate interrupt status
	//-------------------------------------
	InpIntSence	= dwIntSence & 0x7f;
	OutIntSence	= (dwIntSence >> 16) & 0x7f;
	//-------------------------------------
	// Save interrupt status
	//-------------------------------------
	pMasAddr->BmInp.dwIntSence |= InpIntSence;
	pMasAddr->BmOut.dwIntSence |= OutIntSence;
	//-------------------------------------
	// Calculate counter carry
	//-------------------------------------
	// Input
	if (pMasAddr->BmInp.dwIntSence & BM_STATUS_TDCARRY) {
		pMasAddr->BmInp.dwIntSence &= ~BM_STATUS_TDCARRY;
		CarrySave = pMasAddr->BmInp.dwCarryCount;
		pMasAddr->BmInp.dwCarryCount++;
	}
	// Output
	if (pMasAddr->BmOut.dwIntSence & BM_STATUS_TDCARRY) {
		pMasAddr->BmOut.dwIntSence &= ~BM_STATUS_TDCARRY;
		CarrySave = pMasAddr->BmOut.dwCarryCount;
		pMasAddr->BmOut.dwCarryCount++;
	}

	if(pMasAddr->BmInp.endsmp == 1){
		pMasAddr->BmInp.dwEndSenceNum++;
		lret = BM_INT_EXIST;
		pMasAddr->BmInp.endsmp = 0;
		//-------------------------------------
		// Set the flag for completion event notification
		//-------------------------------------
		pMasAddr->BmInp.end_ev_flag	= 1;
		pMasAddr->BmInp.end_ev_save	= 1;
	}
	//-------------------------------------
	// Return interrupt status
	//-------------------------------------
	*InStatus	= (unsigned short)pMasAddr->BmInp.dwIntSence;
	*OutStatus	= (unsigned short)pMasAddr->BmOut.dwIntSence;
	if(lret == BM_INT_EXIST){
		//-------------------------------------
		// when I/O transfer count notification or input transfer completed
		//-------------------------------------
		//-------------------------------------
		// Increase each count
		//-------------------------------------
		if(*InStatus & BM_STATUS_COUNT){
			pMasAddr->BmInp.dwCountSenceNum++;
		}
		if(*OutStatus & BM_STATUS_COUNT){
			pMasAddr->BmOut.dwCountSenceNum++;
		}
		if (pMasAddr->BmInp.end_ev_flag == 1) {
			pMasAddr->BmInp.end_ev_flag	= 0;
			pMasAddr->BmInp.dwIntSence	|= BM_STATUS_ALL_END;
		}
	}
	//-------------------------------------
	// Wake up the interrupt thread
	//-------------------------------------
	if((lret == BM_INT_EXIST) && (
		(pMasAddr->BmInp.dwCountSenceNum != 0) ||
		(pMasAddr->BmOut.dwCountSenceNum != 0) ||
		(pMasAddr->BmInp.dwEndSenceNum != 0))){
		Ccom_wake_up(&pMasAddr->wait_obj);
	}
	return lret;
}
//========================================================================
// Function name  : ApiMemBmDivideAddress
// Function		  : Split 64-bit address and return higher and lower address
// I/F			  : External
// In			  : PreAddr				: Point to the MASTERADDR structure
// Out			  : LowAddr				: Lower address
//					HighAddr			: Higher address
// Return value	  : 
//========================================================================
void ApiMemBmDivideAddress(unsigned long long PreAddr, unsigned long *LowAddr, unsigned long *HighAddr)
{
	unsigned long long	add_cal;
	unsigned long		add_low, add_high;
	
	add_cal		= PreAddr;
	add_low		= (unsigned long)(add_cal & 0xffffffff);
	add_high	= (unsigned long)((add_cal & 0xffffffff00000000LL) >> 32);
	*LowAddr	= add_low;
	*HighAddr	= add_high;
	
	return;
}
#endif	// __BUSMASTERMEMORY_C__


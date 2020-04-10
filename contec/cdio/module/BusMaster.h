////////////////////////////////////////////////////////////////////////////////
/// @file   BusMaster.h
/// @brief  API-DIO(LNX) PCI Module - BusMaster library header file
/// @author &copy;CONTEC CO.,LTD.
/// @since  2003
////////////////////////////////////////////////////////////////////////////////

#ifndef _BUS_MASTER_
#define _BUS_MASTER_

#include <linux/mm.h>
#include <linux/pagemap.h>
//-------------------------------------------------
// Constant
//-------------------------------------------------
#define BM_MAX_SG_SIZE				0x4000			// The maximum number of S/G list on board
													// 64Mb/4096

// Error code (Return values)
#define	BM_ERROR_SUCCESS			0x0				// Normal Complete
#define	BM_ERROR_PARAM				0x1001			// Parameter error
#define	BM_ERROR_MEM				0x1002			// Not secure memory
#define	BM_ERROR_BUFF				0x1003			// The buffer was too big to set, 
													// the buffer was not set
#define	BM_ERROR_NO_BOARD			0x1004			// No board found in getting resource.
#define	BM_ERROR_ARRY_SIZE			0x1005			// The array size is not enough in getting resource.
#define	BM_ERROR_BOARDID			0x1006			// Board ID duplicated in getting resource.
#define	BM_ERROR_SEQUENCE			0x1007			// Procedure error of execution
#define	BM_ERROR_LOCK_MEM			0x1008			// Memory could not be locked

// Return value of interrupt handling routine
#define	BM_INT_NONE					0x01			// It has no BusMaster interrupt
#define	BM_INT_EXIST				0x02			// It has  BusMaster interrupt

// Return value of ApiBmGetStartEnabled
#define	BM_TRANS_ACTIVE				0x01			// During BusMaster transfer
#define	BM_TRANS_STOP				0x02			// Stop BusMaster transfer

// Status value (for library)
#define	BM_STATUS_COUNT				0x001			// Match transfer count
#define	BM_STATUS_TDCARRY			0x004			// Carry

// Status value (for driver)
#define	BM_STATUS_FIFO_EMPTY		0x008			// FIFO is empty
#define	BM_STATUS_INTL				0x002			// Transfer to the end of the list
#define	BM_STATUS_FIFO_FULL			0x010			// FIFO is full
#define	BM_STATUS_SG_OVER_IN		0x020			// S/G Over In (there is data in the FIFO after the completion of the transfer to the S/G list)
#define	BM_STATUS_END				0x040			// Transfer completed
#define	BM_STATUS_ALL_END			0x8000			// After hardware transfer completed, post processes are completed. The app converts this status to a transfer completion status and returns it.
#define	BM_STATUS_FIFO_REACH		0x080			// FIFO is reach

// Function parameters
#define	BM_DIR_IN					0x01			// Transfer direction (input)
#define	BM_DIR_OUT					0x02			// Transfer direction (output)

#define	BM_WRITE_ONCE				0x00			// Use buffer (once only)
#define	BM_WRITE_RING				0x01			// Use buffer (repetition)

#define	BM_RESET_ALL				0x01			// Reset (all)
#define	BM_RESET_FIFO_IN			0x02			// Reset (input FIFO)
#define	BM_RESET_FIFO_OUT			0x04			// Reset (output FIFO)
#define	BM_RESET_CNT_IN				0x08			// Reset (input counter)
#define	BM_RESET_CNT_OUT			0x10			// Reset (output counter)


//-------------------------------------------------
// Structure
//-------------------------------------------------
#pragma pack(push,4)

typedef struct {
	unsigned long		dwInCountSenceNum;		// Number of event notification count for input
	unsigned long		dwInEndSenceNum;		// Completion event notification count for input
	unsigned long		dwOutCountSenceNum;		// Number of event notification count for output
	unsigned long		dwOutEndSenceNum;		// Completion event notification count for output
} BM_GET_INT_CNT, *PBM_GET_INT_CNT;

// SGList parameter for reconfiguring infinite transfer
typedef struct _SGRESET {
	unsigned long		SGOverAdd;				// Store the lower address of SGList for 32DM3
	unsigned long		SGOverAdd_high;			// Store the upper address of SGList 
	unsigned long		FIFOOverAdd;			// Store the lower address of FIFO for 32DM3
	unsigned long		FIFOOverAdd_high;		// Store the upper address of FIFO for 32DM3
	unsigned long		OverLen;				// Store the data size for 32DM3
	unsigned long		ATNumber;				// Store the address conversion table number for 32DM3
	unsigned long		ATPreFetchAdd;			// Store the address for restart for 32DM3
} SGRESET, *PSGRESET;

// One data of Scatter/Gather
// Structure to manage user memory space
typedef	struct _SGLIST {
	struct scatterlist	*List;					// Scatter list that is set to the hard of 4K unit
	unsigned long		dwIsRing;				// Finite transfer or infinite transfer (TRUE: Finite, FALSE: Infinite)
	int					nr_pages;				// Number of pages in use
	unsigned char		*Buff;					// Pointer to user memory
	size_t				dwBuffLen;				// Size of user memory (number of data)
	SGRESET				MemParam;				// Parameter for infinite transfer reset for 32DM3
	unsigned long		ChangeDescAdd[1024];	// Store Descriptor address for 32DM3
	unsigned long		ListNumCnt;				// The number of SGList count for 32DM3
	unsigned long		ATOverCnt;				// Address table count for 32DM3
	unsigned long		ReSetArray;				// Address table count for 32DM3
} SGLIST, *PSGLIST;

// Data structure for input and output
typedef struct _BMEMBER {
	// Storage area of the user buffer
	SGLIST				SgList;				// Structure for managing user memory space
	// Carry
	unsigned long		dwCarryCount;		// Times of valid TDCarry
	unsigned long		dwCarryCount2;		// The second carry
	// About interrupt
	unsigned long		dwIntMask;			// Interrupt factor that has been set
	unsigned long		dwIntSence;			// Interrupt factor
	unsigned long		dwIntNumberOfCnt;	// Number (data number) of transfer to generate an interrupt
	unsigned long		dwCountSenceNum;	// Number of event notification count
	unsigned long		dwEndSenceNum;		// Completion event notification count
	unsigned long		outcnt;				// For Ring for 32DM3
	unsigned long		addtablenum;		// Store the number of address conversion table for 32DM3
	unsigned long long	resetadd;			// Address for resetting address conversion table for 32DM3
	unsigned long		resetchangedesc;	// Write address for resetting address conversion table for 32DM3
	unsigned long		sglen;				// Store the length of SGList for 32DM3
	unsigned long		athalflen;			// Store the transfer length of half address conversion table for 32DM3
	unsigned long		atstartnum;			// Number of list for resetting address conversion table for 32DM3
	unsigned long		atstartlen;			// Length for resetting address conversion table for 32DM3
	unsigned long		resetOverHalf;		// For resetting address conversion table (FIFO position selection flag) for 32DM3
	unsigned long		resetListSize;		// For resetting address conversion table (Descriptor Size) for 32DM3
	unsigned long		resetListMove;		// For resetting address conversion table (Descriptor Division number) for 32DM3
	unsigned long		fiforeachnum;		// Store the FIFO_REACH number for 32DM3
	unsigned long		remaingdatanum;		// Store the number of DMA transfer completed for remaining data transfer for 32DM3
	int					atstartflag;		// DIO re-output flag for 32DM3
	int					firstDMA;			// For resetting address conversion table (1st DMA flag) for 32DM3
	int					ringstartflag;		// Ring specified number stop flag for 32DM3
	int					dmastopflag;		// BusMaster stop flag for 32DM3
	int					cntflag;			// DMA start flag for 32DM3
	int					outflag;			// DIO output flag for 32DM3
	int					endsmp;				// Completion flag (Sampling) for 32DM3
	int					end_ev_flag;		// Completion event notification (Sampling) for 32DM3
	int					end_ev_save;		// Completion event flag holding (Sampling) for 32DM3
	int					sgoverflag;			// SGOverRun flag for 32DM3
	int					ringresetflag;		// Ring reset flag for 32DM3
	int					oddtransferflag;	// Odd transfer count flag for 32DM3
	int					oddbufflag;			// Odd buffer flag for 32DM3
	int					listoverflag;		// List over flag for 32DM3
	int					TimeOutFlag;		// DMA transfer timeout flag for 32DM3
	int					dmaactflag;			// DMA transfer in progress flag
	int					resetcountFlag;		// Reset transfer count initialization flag for 32DM3
} BMEMBER, *PBMEMBER;

// External interface
typedef struct _MASTERADDR{
	unsigned long		dwPortBmAddr;		// I/0 Address for BusMaster
	unsigned char *		dwPort_mem;			// Base memory address of board
	unsigned char *		dwPort2_mem;		// For PIO
	unsigned char *		dwPort3_mem;		// For PIO
	unsigned long		dwPortPciAddr;		// For PIO
	unsigned long		SlaveEnable;		// Slave signal state storage for 32DM3
	unsigned long		MasterEnable;		// Master signal state storage for 32DM3
	BMEMBER				BmInp;				// For input
	BMEMBER				BmOut;				// For output
	struct pci_dev		PciDev;				// Structure of PCI device
	CCOM_TASK_QUEUE		wait_obj;			// Wait structure
	int					add64flag;			// 64bitOS flag
	int					overlistsizeflag;	// Over ListSize flag
} MASTERADDR, *PMASTERADDR;

// Retrieve the count of the BusMaster interrupt subroutine
typedef struct {
	PMASTERADDR			pMasAddr;			// MasterAddr
	PBM_GET_INT_CNT		pIntCount;			// Event notification count
} BM_GET_INT_CNT_SUB, *PBM_GET_INT_CNT_SUB;

// Retrieve transfer count subroutine
typedef struct {
	PMASTERADDR			pMasAddr;			// MasterAddr
	unsigned long		dwDir;				// Transfer direction
	unsigned long		*dwCount;			// Number of transferred data
	unsigned long		*dwCarry;			// Carry count
} BM_GET_CNT_SUB, *PBM_GET_CNT_SUB;
#pragma pack(pop)

//-----------------------------------------------
// Prototype declaration (External interface)
//-------------------------------------------------
long ApiBmGetPciResource(	unsigned short vendor_id, unsigned short device_id, unsigned short board_id,
							unsigned long *io_addr, unsigned long *io_num, unsigned long *mem_addr, unsigned long *mem_num,
							unsigned long *irq, struct pci_dev *ppci_dev);
long ApiBmCreate(PMASTERADDR pMasAddr, struct pci_dev *ppci_dev, unsigned long dwPortBmAddr, unsigned long dwPortPciAddr);
long ApiBmDestroy(PMASTERADDR pMasAddr);
long ApiBmSetNotifyCount(PMASTERADDR pMasAddr, unsigned long dwDir, unsigned long dwCount);
long ApiBmClearBuffer(PMASTERADDR pMasAddr, unsigned long dwDir);
long ApiBmStart(PMASTERADDR pMasAddr, unsigned long dwDir);
long ApiBmStop(PMASTERADDR pMasAddr, unsigned long dwDir);
long ApiBmGetBufLen(PMASTERADDR pMasAddr, unsigned long dwDir, unsigned long *dwLen);
long ApiBmSetBuffer(PMASTERADDR pMasAddr, unsigned long dwDir, void *Buff, unsigned long dwLen, unsigned long dwIsRing);
long ApiBmGetCount(PMASTERADDR pMasAddr, unsigned long dwDir, unsigned long *dwCount, unsigned long *dwCarry);
long ApiBmGetStatus(PMASTERADDR pMasAddr, unsigned long dwDir, unsigned long *dwStatus);
long ApiBmReset(PMASTERADDR pMasAddr, unsigned long dwResetType);
long ApiBmInterrupt(PMASTERADDR pMasAddr, unsigned short *InStatus, unsigned short *OutStatus);
long ApiBmUnlockMem(PMASTERADDR pMasAddr, unsigned long dwDir);
long ApiBmCheckFifo(PMASTERADDR pMasAddr, unsigned long dwDir, unsigned long *lpdwFifoCnt);
long ApiBmGetIntCount(PMASTERADDR pMasAddr, PBM_GET_INT_CNT pIntCount);
long ApiBmGetStartEnabled(PMASTERADDR pMasAddr, unsigned long dwDir);
void ApiBmWaitEvent(PMASTERADDR pMasAddr);
void ApiBmWakeUpEvent(PMASTERADDR pMasAddr);
void ApiBmSetStopEvent(PMASTERADDR pMasAddr, unsigned long dwDir);

//--------------------------------------------------
// Prototype declaration (Internal interface)
//--------------------------------------------------
void BmSubGetCount(void *data);
void BmSubGetIntCount(void *data);
#endif	// _BUS_MASTER_


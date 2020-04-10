////////////////////////////////////////////////////////////////////////////////
///	@file	
///	@brief	BusMaster library header file for API-XXX(WDM) Driver
///	@author	CONTEC CO.,LTD.
///	@since	2002
////////////////////////////////////////////////////////////////////////////////

#ifndef __BUSMASTERMEMORY_H__
#define __BUSMASTERMEMORY_H__
#include "BusMaster.h"

//-------------------------------------------------
// Prototype declaration (External interface) for Memory
//-------------------------------------------------
long ApiMemBmCreate(PMASTERADDR pMasAddr, struct pci_dev *ppci_dev, unsigned char * dwPortBmAddr, unsigned char * dwPortPciAddr);
long ApiMemBmDestroy(PMASTERADDR pMasAddr);
long ApiMemBmSetBuffer(PMASTERADDR pMasAddr, unsigned long dwDir, void *Buff, unsigned long dwLen, unsigned long dwIsRing);
long ApiMemBmUnlockMem(PMASTERADDR pMasAddr, unsigned long dwDir);
long ApiMemBmStart(PMASTERADDR pMasAddr, unsigned long dwDir);
long ApiMemBmStop(PMASTERADDR pMasAddr, unsigned long dwDir);
long ApiMemBmGetCount(PMASTERADDR pMasAddr, unsigned long dwDir, unsigned long *dwCount, unsigned long *dwCarry);
long ApiMemBmReset(PMASTERADDR pMasAddr, unsigned long dwResetType);
long ApiMemBmSetNotifyCount(PMASTERADDR pMasAddr, unsigned long dwDir, unsigned long dwCount);
long ApiMemBmCheckFifo(PMASTERADDR pMasAddr, unsigned long dwDir, unsigned long *lpdwFifoCnt);
long ApiMemBmInterrupt(PMASTERADDR pMasAddr, unsigned short *InStatus, unsigned short *OutStatus, unsigned long IntSenceBM);
void ApiMemBmDivideAddress(unsigned long long PreAddr, unsigned long *LowAddr, unsigned long *HighAddr);

//-------------------------------------------------
// Prototype declaration (Internal interface) for Memory
//-------------------------------------------------
long MemBmSetSGAddress(PMASTERADDR pMasAddr, unsigned long dwDir);
long MemBmSetSGAddressRePlace(PMASTERADDR pMasAddr, unsigned long dwDir, unsigned long loop, unsigned long stopnum);
long MemBmReSetSGAddress(PMASTERADDR pMasAddr, unsigned long dwDir);
long MemBmSetSGAddressRing(PMASTERADDR pMasAddr, unsigned long dwDir);
long MemBmSetSGAddressRingPre(PMASTERADDR pMasAddr, unsigned long dwDir);
long MemBmSetSGAddressRingPreLittle(PMASTERADDR pMasAddr, unsigned long dwDir);
long MemBmReSetSGAddressRing(PMASTERADDR pMasAddr, unsigned long dwDir);
long MemBmSetSGAddressRingEnd(PMASTERADDR pMasAddr, unsigned long dwDir, unsigned long stopnum);
long MemBmMakeSGList(PSGLIST pSgList, unsigned long dwDir);
void MemBmSubGetCount(void *data);
void MemBmSubGetIntCount(void *data);

#endif	// __BUSMASTERMEMORY_H__

=====================================================================
=              Linux Driver for Digital Input/Output                =
=                                       API-DIO(LNX)      Ver6.80   =
=                                                  CONTEC Co.,Ltd.  =
=====================================================================

Contents
=======
  Introduction
  About the Product
  Restrictions
  Note
  How to install
  How to uninstall
  Sample programs
  Version up history


Introduction
============
  The explanation of API-DIO(LNX) is described as follows. You must read it because
some contents are not described in help.

About the Product
===========
- API-DIO(LNX) supplies the set of functions for controlling our digital I/O board by
  the driver and the shared library in module.

- The basic functionality of input and output, interrupt, trigger monitor with timer is provided.

- Setup and use the device to be used by using the setup program (config) and setup files.

- Setup program generates the setting file and driver start script file required for
  shifting to executing environment and stop script file.

- The source code of user interrupt processing which can execute with the driver is supplied.

Restrictions
============
- Object of product specification : Supported Linux version

  Distribution                  Kernel version
  ----------------------        ------------------
  RedHat Linux
     Enterprise Linux 6         2.6.32-71.el6.i686
     Enterprise Linux 6.7       2.6.32-573.el6.i686
     Enterprise Linux 6.7       2.6.32-573.el6.x86_64
     Enterprise Linux 7.0       3.10.0-123.el7.x86_64
     Enterprise Linux 7.1       3.10.0-229.el7.x86_64
     Enterprise Linux 7.2       3.10.0-327.el7.x86_64
     Enterprise Linux 8.0       4.18.0-80.el8.x86_64

- Not covered by product specifications : Operation check only.

  Distribution                  Kernel version
  ----------------------        ------------------
  CentOS
     7.0-1406                   3.10.0-123.el7.x86_64
     7.2-1511                   3.10.0-327.el7.x86_64
  Ubuntu
     14.04.3 LTS                3.19.0-25-generic
     16.04.1 LTS                4.4.0-31-generic (i686)
     16.04.1 LTS                4.4.0-31-generic (x86_64)
     16.04.5 LTS                4.15.0-29-generic (i686)
     18.04.1 LTS                4.15.0-29-generic (x86_64)
     18.04.3 LTS                5.0.0-23-generic (x86_64)
  Fedora
     Fedora 20                  3.11.10-301.fc20.x86_64
     Fedora 24                  4.5.5-300.fc24.i686
     Fedora 24                  4.5.5-300.fc24.x86_64

  Please understand that the behavior-verifications on the other than above versions
  have not be done. 

Note
==========
  To install and setup the driver, it is required to startup with root authority.

How to install
==================
  The driver is provided in a compressed file. You can use the following
command in shell to copy/extract it.
If you want to change the library install path to a location, such as /usr/local/lib,
you should find the Makefile and change the install path in advance.

-XXX is driver's version.

 # cd
 # mount /dev/cdrom /mnt/cdrom
 # cp /mnt/cdrom/linux/dio/cdioXXX.tgz ./
 # tar xvfz cdioXXX.tgz
 ................
 # cd contec/cdio
 # make
 ................
 # make install
 ................
 # cd config
 # ./config
 ..... Setup the following .........
 # ./contec_dio_start.sh

Start and stop the driver by executing contec_dio_start.sh and contec_dio_stop.sh.
If you want to start driver when the system started, please add the processing contents of
start script to /etc/rc.d/rc.local.

How to uninstall
======================
  Perform the shell script for uninstallation to uninstall this product.

 # cd contec/cdio
 # ./cdio_uninstall.sh
 ...................
 #
 For details, please refer to cdio_uninstall.sh script

Sample programs
====================
  The sample programs are contained in the following directory by language.
  /<User Directory>/contec/cdio/samples

 The explanation of directory

  /inc
    Include the function define file in C/C++. When you create the program in C/C++, incule this file.
    And, don't edit this file.

  /console
    Sample program source code in C language and Makefile.


Version up history API-DIO(LNX)
=========================================
  Ver6.71 -> Ver6.80 (Web Release)
  --------------------------------
  - Corresponded to a new distribution.
    RedHat Enterprise Linux 8.0

  - Tested on the following distributions.
    Ubuntu 18.04.3 LTS (x86_64)

  - Addition of the device which can be used
    An additional device CPS-RRY-4PCC
                         (Used in combination with CPS-BXC200)

  Ver6.60 -> Ver6.71
  --------------------------------
  - Fixed a bug that the transfer completion event set by DioDmSetStopEvent() may not occur when generating is stopped by DioDmTransferStop() on the following devices.
      - PIO-32DM(PCI)
      - DIO-32DM-PE
      - DIO-32DM2-PE

  Ver6.51 -> Ver6.60 (2019/10 Web Release)
  --------------------------------
  - Addition of the device which can be used
    An additional device DIO-CPS-BXC200 (DIO of CPS-BXC200)
                         CPS-DIO-0808L, CPS-DIO-0808BL, CPS-DIO-0808RL,
                         CPS-DI-16L, CPS-DI-16RL, CPS-DO-16L, CPS-DO-16RL
                         (Used in combination with CPS-BXC200)

  Ver6.50 -> Ver6.51 (2019/07 Web Release)
  --------------------------------
  - Fixed a problem that caused a kernel panic when executing the DioGet8255Mode function.

  Ver6.40 -> Ver6.50 (Web Release)
  --------------------------------
  - Tested on the following distributions.
    Ubuntu 16.04.5 LTS (i686)
    Ubuntu 18.04.1 LTS (x86_64)

  Ver6.20 -> Ver6.40 (2018/11 Web Release)
  --------------------------------
  - Addition of the device which can be used
    An additional device DIO-1616LN-USB, DIO-1616LN-ETH, DIO-0404LY-WQ
                         CPSN-DI-08L, CPSN-DI-08BL, CPSN-DO-08L, CPSN-DO-08BL

  - In the combination of USB and PCI / PCI Express bus devices
    Fixed a problem that caused 10003 error when executing DioInit() for PCI / PCI Express bus device.

  - Fixed a problem that multiple USB devices of the same model may not be controlled properly.

  Ver6.10 -> Ver6.20 (2017/12 Web Release)
  --------------------------------
  - Addition of the device which can be used
    An additional device DIO-24DY-USB, DIO-0808LY-USB, DIO-0808TY-USB
                         DI-16TY-USB, DO-16TY-USB, DIO-1616LX-USB
                         DIO-3232LX-USB, DIO-6464LX-USB, DIO-128SLX-USB
                         DIO-1616RYX-USB, DIO-1616BX-USB, DIO-48DX-USB
                         RRY-16CX-USB, DIO-0808RN-USB, DIO-1616HN-USB

  Ver6.00 -> Ver6.10 (Web Release)
  --------------------------------
  - Tested on the following distributions.
    Ubuntu 16.04.1 LTS
    Fedora 24

  Ver5.20 -> Ver6.00 (Web Release)
  --------------------------------
  - A part of driver source code is published

  - Tested on the following distributions.
    CentOS 7.0-1406
    CentOS 7.2-1511
    Ubuntu 14.04.3 LTS
    Fedora 20

  Ver4.50 -> Ver5.20 (Web Release)
  --------------------------------
  - Addition of the board which can be used
    An additional board DIO-1616L-LPE, DIO-1616B-LPE

  - Corresponded to a new distribution.
    Red Hat Enterprise Linux 6
    Red Hat Enterprise Linux 6.7

  - 64bit support was introduced in the following distributions
    Red Hat Enterprise Linux 6.7
    Red Hat Enterprise Linux 7
    Red Hat Enterprise Linux 7.1
    Red Hat Enterprise Linux 7.2

  - The support on the following distributions was terminated
    RedHat Linux 7.1
    RedHat Linux 7.2
    RedHat Linux 7.3
    RedHat Linux 8.0
    RedHat Linux 9
    Red Hat Professional Workstation
    Red Hat Enterprise Linux 4
    Red Hat Enterprise Linux 5
    TurboLinux   7.0
    TurboLinux   8
    TurboLinux   10 Server
    TurboLinux   11 Server

  Ver4.40->Ver4.50 (Ver Mar.2011)
  --------------------------------
  - Addition of the board which can be used
    An additional board DI-128RL-PCI, DO-128RL-PCI

  Ver4.30 -> Ver4.40 (Web Release)
  --------------------------------
  - Corresponded to a new distribution. (Include it for SMP. )
    Red Hat Enterprise Linux 5
    Turbo Linux 11 server

  Ver4.21->Ver4.30 (Ver Jan.2009)
  --------------------------------
  - Addition of the board which can be used
    An additional board DIO-32DM-PE
  - Trouble with the environment to which I/O cannot be normally done with the card bus device is corrected.

  Ver4.20->Ver4.21 (web release)
  --------------------------------
  - The bus master's forwarding completion notification corrects trouble from which dependence might be actually notified a lot.
  - Trouble that might cause the segmentation fault is corrected when automatic of the device detecting it with config.

  Ver4.10->Ver4.20 (Ver Apr.2008)
  --------------------------------
  - Addition of the board which can be used
    An additional board DI-64T-PE, DO-64T-PE, DI-32T-PE, DO-32T-PE
                        DIO-48D-PE, DIO-96D-LPE, DI-128T-PE, DO-128T-PE

  Ver4.00->Ver4.10 (Ver Jan.2008)
  --------------------------------
  - Addition of the board which can be used
    An additional board RRY-32-PE, RRY-16C-PE, DIO-1616RY-PE
  - Cause the kernel panic by the DioDmSetBuff function is corrected.

  Ver3.51->Ver4.00 (Ver Oct.2007)
  --------------------------------
  - Addition of the board which can be used
    An additional board DI-32B-PE, DO-32B-PE, DIO-1616H-PE, DIO-3232H-PE
                        DIO-1616RL-PE, DIO-3232RL-PE

  Ver3.50->Ver3.51 (web release)
  --------------------------------
  - It replaces it for driver file damage of Ver3.50

  Ver3.40->Ver3.50 (Ver Jun.2007)
  --------------------------------
  - Addition of the board which can be used
    An additional board DIO-96D2-LPCI

  Ver3.30->Ver3.40 (Ver Feb.2007)
  --------------------------------
  - Addition of the board which can be used
    An additional board DIO-3232B-PE, DIO-3232F-PE, DIO-1616B-PE, DIO-1616TB-PE
                        DI-64L-PE, DI-32L-PE, DO-64L-PE, DO-32L-PE
                        DIO-6464T2-PCI, DI-128T2-PCI, DO-128T2-PCI,DI-32T2-PCI
                        DO-32T2-PCI, DI-64T2-PCI, DO-64T2-PCI, DIO-48D2-PCI

  Ver3.21->Ver3.30 (Ver Nov.2006)
  --------------------------------
  - Addition of the board which can be used
    An additional board DIO-3232T-PE, DIO-1616T-PE, DIO-6464T-PE, DIO-6464L-PE, DI-128L-PE, DO-128L-PE

  Ver3.20->Ver3.21
  --------------------------------
  -When PIO-32DM(PCI) is used with PC equipped with the memory of 1GByte or more
   in 2.4 kernel factions, trouble might cause the kernel panic by the
   DioDmSetBuff function is corrected.
   (It is effective only in kernel version 2.4.13 or more. )

  Ver3.10->Ver3.20 (Ver Apr.2006)
  --------------------------------
  - Addition of the board which can be used
    An additional board DIO-48D-LPE, DIO-1616T-LPE, DIO-3232L-PE, DIO-1616L-PE

  Ver3.00->Ver3.10 (Ver Nov.2005)
  --------------------------------
  - Addition of the board which can be used
    An additional board PIO-32/32F(PCI)H, PIO-16/16T(PCI)H, PIO-16/16TB(PCI)H

  Ver2.20->Ver3.00 (Ver Aug.2005)
  --------------------------------
  - It corresponds to kernel 2.6.xx
  - It corresponds to Red Hat Enterprise Linux 4.
  - It corresponds to Turbo Linux 10 server.

  Ver.2.10 -> Ver.2.20 (Ver Jun.2005)
  --------------------------------
  - Addition of the board which can be used
    An additional board PIO-64/64L(PCI)H, PI-128L(PCI)H, PO-128L(PCI)H
  - Add function: DioDmGetWritePointerUserBuf()
  - To use DioDmGetWritePointerUserBuf() with the Infinite sample, it changes.
  - When a bus master transmission count carried out going up from 24 bits to 25 bits,
    the fault which is not counted normally was corrected.

  Ver.2.00 -> Ver.2.10 (Ver Apr.2005)
  --------------------------------
  - Addition of the board which can be used
    An additional board PIO-32/32T(PCI)H, PIO-32/32B(PCI)V, RRY-16C(PCI)H,
                        RRY-32(PCI)H, PIO-48D(LPCI)H

  Ver.1.50 -> Ver.2.00 (Ver Jan.2005)
  --------------------------------
  - Addition of the board which can be used
    An additional board PIO-48D(CB)H

  Ver.1.40 -> Ver.1.50 (Ver Oct.2004)
  --------------------------------
  - Addition of the board which can be used
    An additional board	  PIO-32/32H(PCI)H, PIO-16/16H(PCI)H, PIO-32/32RL(PCI)H
                          PIO-16/16RL(PCI)H, PIO-16/16L(CB)H

  Ver.1.30 -> Ver.1.40 (Web Release)
  --------------------------------
  - Addition of the board which can be used
    An additional board	 PIO-32DM(PCI)
  - The new function for PIO-32DM(PCI) was added.
  - It corresponds to Red Hat Professional Workstation.
  - If end processing is not performed normally and application is not ended at the time of DioExit execution
    The fault whose number of processes which can use the same board decreases is corrected.

  Ver.1.20 -> Ver.1.30 (Web Release)
  --------------------------------
  - Addition of the board which can be used
    An additional board	  PIO-48D(PCI)
  - The new function for PIO-48D(PCI) was added.
  - When the process treating other boards existed at the time of trigger use
    and all the processes treating the board were completed,
    the fault which a trigger stops was corrected.
  - At the time of interruption use, if it continues putting in the pulse of
    about 1kHZ or more, the fault into which interruption may stop going will be corrected.

  Ver.1.10 -> Ver.1.20 (Ver Nov.2003)
  --------------------------------
  - Addition of the board which can be used
    An additional board	  PIO-16/16B(PCI)H,PI-32B(PCI)H,PO-32B(PCI)H,
                          PIO-16/16L(LPCI)H,PIO-16/16B(LPCI)H,
                          PIO-16/16T(LPCI)H

  Ver.1.02 -> Ver.1.10 (Web Release)
  --------------------------------
  - It corresponds to Red Hat Linux 8.0 Red Hat Linux 9.

  Ver.1.01 -> Ver.1.02 (Web Release)
  --------------------------------
  - Interruption corrects the fault which is not notified normally
    by PIO-XX(PCI) H (except for PIO-32/32B(PCI) H), PI-XX(PCI) H, and PIO-16/16RY (PCI).

  Ver.1.00 -> Ver.1.01 (Ver. Dec.2002)
  --------------------------------
  - It corrects that the rate of CPU use had become 100% at the time of interruption use.
  - Modify Help

  Ver.1.00 (Ver. Dec.2002)
  --------------------------------
  - First release


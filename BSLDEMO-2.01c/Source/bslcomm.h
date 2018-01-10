/****************************************************************
*
* Copyright (C) 1999-2000 Texas Instruments, Inc.
* Author: Volker Rzehak
*
*----------------------------------------------------------------
* All software and related documentation is provided "AS IS" and 
* without warranty or support of any kind and Texas Instruments 
* expressly disclaims all other warranties, express or implied, 
* including, but not limited to, the implied warranties of 
* merchantability and fitness for a particular purpose. Under no 
* circumstances shall Texas Instruments be liable for any 
* incidental, special or consequential damages that result from 
* the use or inability to use the software or related 
* documentation, even if Texas Instruments has been advised of 
* the liability.
*
* Unless otherwise stated, software written and copyrighted by 
* Texas Instruments is distributed as "freeware". You may use 
* and modify this software without any charge or restriction. 
* You may distribute to others, as long as the original author 
* is acknowledged.
*
*****************************************************************
* 
* Project: MSP430 Bootstrap Loader Demonstration Program
*
* File:    BSLCOMM.H
*
* History:
*   Version 1.00 (05/2000)
*   Version 1.11 (09/2000)
*     - Added definition of BSL_CRITICAL_ADDR.
*   Version 1.12 (02/2001 FRGR)
*     - Added definition of (not released) BSL command
*       "Erase Check" BSL_ECHECK
*     
****************************************************************/

#ifndef BSLComm__H
#define BSLComm__H

#include "ssp.h"

/* Transmit password to boot loader: */
#define BSL_TXPWORD 0x10 
/* Transmit block    to boot loader: */
#define BSL_TXBLK   0x12 
/* Receive  block  from boot loader: */
#define BSL_RXBLK   0x14 
/* Erase one segment:                */
#define BSL_ERASE   0x16 
/* Erase complete FLASH memory:      */
#define BSL_MERAS   0x18 
/* Load PC and start execution:      */
#define BSL_LOADPC  0x1A 
/* Erase Check Fast:                 */
#define BSL_ECHECK  0x1C 
/* Receive ID:                       */
#define BSL_RXID    0x1E
/* Change Speed:                     */
#define BSL_SPEED   0x20  
/* Setg Memory Offset (for devices with >64k Mem  */
#define BSL_MEMOFFSET 0x21  
   

/* Bootstrap loader syncronization error: */
#define ERR_BSL_SYNC      99

/* Upper limit of address range that might be modified by 
 * "BSL checksum bug".
 */
#define BSL_CRITICAL_ADDR 0x0A00

#ifdef __cplusplus
extern "C" {
#endif

extern int BSLMemAccessWarning;

/*-------------------------------------------------------------*/
void bslReset(BOOL invokeBSL);
/* Applies BSL entry sequence on RST/NMI and TEST/VPP pins
 * Parameters: invokeBSL = TRUE:  complete sequence
 *             invokeBSL = FALSE: only RST/NMI pin accessed
 */

/*-------------------------------------------------------------*/
int bslSync();
/* Transmits Synchronization character and expects to
 * receive Acknowledge character
 * Return == 0: OK
 * Return == 1: Sync. failed.
 */

/*-------------------------------------------------------------*/
int bslTxRx(BYTE cmd, unsigned long addr, WORD len, 
            BYTE blkout[], BYTE blkin[]);
/* Transmits a command (cmd) with its parameters: 
 * start-address (addr), length (len) and additional 
 * data (blkout) to boot loader. 
 * Parameters return by boot loader are passed via blkin.
 * Return == 0: OK
 * Return != 0: Error!
 */

#ifdef __cplusplus
}
#endif

#endif

/* EOF */
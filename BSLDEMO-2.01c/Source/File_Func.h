/*========================================================================*\
|                                                                          |
| FileFunc.h                                                               |
|                                                                          |
|                                                                          |
|--------------------------------------------------------------------------|
| Project:              MSP430 JTAG interfcae                              |
| Developed using:      MS Visual C++ 5.0                                  |
|--------------------------------------------------------------------------|
| Author:               Dale Wellborn                                      |
| Version:              0.00                                               |
| Initial Version:      20 / 02 / 02                                       |
| Last Change:                                                             |
|--------------------------------------------------------------------------|
| Version history:                                                         |
|                                                                          |
|--------------------------------------------------------------------------|
| Designed 2002 by Texas Instruments                                       |
\*========================================================================*/

/*------------------------------------------------------------------------*\
| Remarks:                                                                 |
|                                                                          |
\*------------------------------------------------------------------------*/

#ifndef _File_Func_H_
#define _File_Func_H_

#include "MSP430.h"
#include "TI_TXT_Files.h"

#define MAX_LINE_SIZE 1024
#define MAX_MEM_SIZE 65536

#define LASTPERIPHERALADDR 0x1ff

extern const long ramStart;
extern const long ramEnd;
extern const long ram2Start;
extern const long ram2End;
extern const long infoStart;
extern const long infoEnd;
extern const long mainStart;
extern const long mainEnd;


typedef BYTE BYTEARRAY[];
typedef BYTEARRAY *LPBYTEARRAY;

struct downloadSegment
{
  struct downloadSegment* next;
  BYTE* data;
  long startAddress;
  long size;
};

extern struct downloadSegment* ramSegmentList;
extern struct downloadSegment* flashSegmentList;

//-- FreeSegBuffer -----------------------------------------------------------
// Free Allocated Buffer for Flash and Ram Data
// Arguments: 
// Result:    
void FreeSegBuffer(void);

STATUS_T MSP430_ReadOutFile(LONG wStart, LONG wLength, 
                  LPTSTR lpszFileName, LONG iFileType);

#endif /* _File_Func_H_ */

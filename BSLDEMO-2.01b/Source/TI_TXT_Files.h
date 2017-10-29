/*========================================================================*\
|                                                                          |
| TI_TXT_Files.h                                                           |
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

#ifndef _TI_TXT_Files_H_
#define _TI_TXT_Files_H_

/***********************/
/* Function prototypes */
/***********************/

/*char* strtrim(char* lpszA);*/
BOOL GetAddress(char* Record, unsigned long* ulAddress);
BOOL GetBytes(char* Record, BYTE Bytes[], unsigned int* ByteCnt);

long Load_Ti_Txt(LPTSTR File);

//-- StartTITextOutput --------------------------------------------------------
// StartTIText starts the output in a ti text file
// Arguments: char *lpszFileName (the name of the file)
// Result:    bool (true if success)

BOOL StartTITextOutput(char *lpszFileName);

//-- WriteTITextBytes ---------------------------------------------------------
// Writes one or more data records into the ti Text file
// Arguments: unsigned long ulAddress (start address of the data bytes for the file)
//            WORD wWordCount (number of words (16bit))
//            void *lpData (pointer to data words)
// Result:    bool (true if success)

BOOL WriteTITextBytes(unsigned long ulAddress, WORD wWordCount, void *lpData);

//-- FinishTITextOutput -------------------------------------------------------
// FinishTITextOutput closes the output file.
// Arguments: none
// Result:    bool (true if success)

BOOL FinishTITextOutput(void);

#endif /* _TI_TXT_Files_H_ */

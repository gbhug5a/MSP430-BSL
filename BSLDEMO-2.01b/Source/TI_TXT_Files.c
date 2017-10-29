/*========================================================================*\
|                                                                          |
| TI_TXT_Files.c                                                           |
|                                                                          |
| Software to read the TI-TXT file format                                  |
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
| 11.10.2002 UPSF freeing of File Buffer after error added                 |
| 11.10.2002 UPSF fixed bug within read file - struct placed on wrong pos. |
| Version: 1.1.5                                                           |
| 01/20/2004 UPSF Added support for MSP430F161x                            |
|                                                                          |
|--------------------------------------------------------------------------|
| Designed 2002 by Texas Instruments                                       |
\*========================================================================*/

/*------------------------------------------------------------------------*\
| Remarks:                                                                 |
|                                                                          |
\*------------------------------------------------------------------------*/

/*==========================================================================
| I N C L U D E S                                                         */

//#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "MSP430.h"
#include "File_Func.h"

//-----------------------------------------------------------------------------

//-- StartTITextOutput --------------------------------------------------------
// StartTIText starts the output in a ti text file
// Arguments: char *lpszFileName (the name of the file)
// Result:    bool (true if success)

FILE *fOutStream = NULL;			// file handle
int iPosition = 0;					// position in line
unsigned int uiAddress = 0x10000;	// last address

BOOL StartTITextOutput(char *lpszFileName)
{
	iPosition = 0;
	fOutStream = fopen(lpszFileName, "w");
	return (fOutStream != NULL);
}

//-- WriteTITextBytes ---------------------------------------------------------
// Writes one or more data records into the ti Text file
// Arguments: unsigned long ulAddress (start address of the data bytes for the file)
//            WORD wWordCount (number of words (16bit))
//            void *lpData (pointer to data words)
// Result:    bool (true if success)

BOOL WriteTITextBytes(unsigned long ulAddress, WORD wWordCount, void *lpData)
{
	unsigned int i;

	if (fOutStream == NULL)
		return FALSE;

	// starts with new address ?
	if (ulAddress != uiAddress)
	{
		if (iPosition)
			fprintf(fOutStream, "\n");

		fprintf(fOutStream, "@%04X\n", ulAddress);
		uiAddress = ulAddress;
		iPosition = 0;
	}


	for (i = 0; i < (unsigned int) (wWordCount * 2); i++)
	{
		fprintf(fOutStream, "%02X", (*(LPBYTEARRAY)lpData)[i]);
		iPosition++;
		if (iPosition == 16)
		{
			fprintf(fOutStream, "\n");
			iPosition = 0;
		}
		else
			fprintf(fOutStream, " ");
	}

	uiAddress += wWordCount * 2;


	return TRUE;
}

//-- FinishTITextOutput -------------------------------------------------------
// FinishTITextOutput closes the output file.
// Arguments: none
// Result:    bool (true if success)

BOOL FinishTITextOutput(void)
{
	if (fOutStream)
	{
		// write NL if line is not ended
		if (iPosition)
			fprintf(fOutStream, "\n");

		fprintf(fOutStream, "q\n");
		fclose(fOutStream);
		fOutStream = NULL;
		return TRUE;
	}
	return FALSE;
}

//-----------------------------------------------------------------------------

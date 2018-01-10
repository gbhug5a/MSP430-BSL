/****************************************************************
*
* Copyright (C) 1999-2000 Texas Instruments, Inc.
* Author: Volker Rzehak
* Co-author: FRGR
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
****************************************************************
*
* Project: MSP430 Bootstrap Loader Demonstration Program
*
* File:BSLDEMO.C
*
* Description:
*	This is the main program of the bootstrap loader
*	demonstration.
*	The main function holds the general sequence to access the
*	bootstrap loader and program/verify a file.
*	The parsing of the TI TXT file is done in a separate
*	function.
*
*	A couple of parameters can be passed to the program to
*	control its functions.	For a detailed description see the
*	appendix of the corresponding application note.
*
* History:
*	Version 1.00 (05/2000)
*	Version 1.10 (08/2000)
*	- Help screen added.
*	- Additional mass erase cycles added
*		(Required for larger flash memories)
*		Defined with: #define ADD_MERASE_CYCLES 20
*	- Possibility to load a completely new BSL into RAM
*		(supposing there is enough RAM) - Mainly a test feature!
*	- A new workaround method to cope with the checksum bug
*		established. Because this workaround is incompatbile with
*		the former one the required TI TXT file is renamed to
*		"PATCH.TXT".
*	Version 1.12 (09/2000)
*	- Added handling of frames with odd starting address
*		to BSLCOMM.C. (This is required for loaders with word
*		programming algorithm! > BSL-Version >= 1.30)
*	- Changed default number of data bytes within one frame
*		to 240 bytes (old: 64). Speeds up programming.
*	- Always read BSL version number (even if new one is loaded
*		into RAM.
*	- Fixed setting of warning flag in conjunction with loading
*		a new BSL into RAM.
*	- Added a byte counter to the programTIText function.
*	- Number of mass erase cycles can be changed via command
*		line option (-m)
*	Version 1.14
*		11/2000 FRGR:
*	- Minor text bug fixes and cosmetics (title aso...).
*	- Read startaddress for loadable BSL from file
*		05/2001 FRGR:
*	- Implementation of "+f" parameter for Fast Erase Check by
*		File (BSL cmd "0x1C"). Could be speeded up for Mass
*		Erase (one single command with whole address range).
*	- Bargraph for displaying progress of read/write activities.
*		08/2001 FRGR:
*	- See test sequences for new features at file end.
*	- Skip verify if BSL with online verification is used.
*	- Implementation of "-s{0,1,2}" parameter for Change Baudrate
*		command (BSL cmd "0x20").
*	- Implementation of "+#" parameter for executing test sequences
*	- Time measurement for "Prog/Verify" and "Over all" (in sec).
*	Version 1.15
*		01/2004 FRGR
*	- Test sequence: Wrong password performs a Mass Erase.
*	- Implementation of "+u" parameter for "User Call" simulation
*	- Open Help screen also if no cmdline parameter.
*		UPSF
*	- Fixed bug in end detection of help
*	Version 2.00
*		10/2004 FRGR
*	- added support for 2.xx
*		03/2005 UPSF
*	- added updated support for 2.xx
*	- added read file function
*   - added support for higher COM ports ( > COM4 )
*	- added Erase Segment function
*	- added support for Extended Memory (MSP430X)
*	- added restore function for InfoA Segment
*	- fixed handling of wait option - it got ignored in +option
*	- changed parsing of command line so that a parameter without + or - is used as filename
*	Version 2.01
*	- fixed change baud rate for G2xx3 parts (handles device ID starting with 0x25)
*
*   Change by GH:
*
*   Version 2.01b
*   - added   +i Program Flow Specifier to invert DTR line
*
*   Version 2.01c
*   - deleted +i Program Flow Operator - replaced by -i Option
*
*   - added -i Option to invert DTR line (for use with USB-to-Serial adapters)
*   - added -j Option to invert RTS line (for parts with dedicated JTAG pins)
*
****************************************************************/

#include <string.h>
#include <stdio.h>
#include <conio.h>
#include <windows.h>

#include "bslcomm.h"
#include "TI_TXT_Files.h"

/*---------------------------------------------------------------
* Defines:
*---------------------------------------------------------------
*/

/* This definition includes code to load a new BSL into RAM:
* NOTE: Can only be used with devices with sufficient RAM!
* The program flow is changed slightly compared to a version
* without "NEW_BSL" defined.
* The definition also defines the filename of the TI-TXT file
* with the new BSL code.
*/
#define NEW_BSL

/* The "WORKAROUND" definition includes code for a workaround
* required by the first version(s) of the bootstrap loader.
*/
#define WORKAROUND

/* If "DEBUGDUMP" is defined, all checked and programmed blocks are
* logged on the screen.
*/
#define DEBUGDUMP

/* Additional mass erase cylces required for (some) F149 devices.
* If ADD_MERASE_CYCLES is not defined only one mass erase
* cycle is executed.
* Remove #define for fixed F149 or F11xx devices.
*/
#define ADD_MERASE_CYCLES		20
/* Error: verification failed:		*/
#define ERR_VERIFY_FAILED		98
/* Error: erase check failed:		*/
#define ERR_ERASE_CHECK_FAILED	97
/* Error: unable to open input file: */
#define ERR_FILE_OPEN			96

/* Mask: program data:	*/
#define ACTION_PROGRAM			0x01
/* Mask: verify data:	*/
#define ACTION_VERIFY			0x02
/* Mask: erase check:	*/
#define ACTION_ERASE_CHECK		0x04
/* Mask: transmit password:  */
/* Note: Should not be used in conjunction with any other action! */
#define ACTION_PASSWD			0x08
/* Mask: erase check fast:	*/
#define ACTION_ERASE_CHECK_FAST	0x10


/*---------------------------------------------------------------
* Global Variables:
*---------------------------------------------------------------
*/

char *programName=	"MSP430 Bootstrap Loader Communication Program";
char *programVersion= "Version 2.01c";

/* Max. bytes sent within one frame if parsing a TI TXT file.
* ( >= 16 and == n*16 and <= MAX_DATA_BYTES!)
*/
int maxData= 240;

/* Change by GH */

BOOL InvertDTR = FALSE;      /* New flag for -i Option to invert DTR */
BOOL InvertRTS = FALSE;      /* New flag for -j Option to invert RTS */

/* Buffers used to store data transmitted to and received from BSL: */
BYTE blkin [MAX_DATA_BYTES]; /* Receive buffer	*/
BYTE blkout[MAX_DATA_BYTES]; /* Transmit buffer */

#ifdef WORKAROUND
char *patchFilename = "PATCH.TXT";
char *patchFile = NULL;
#endif /* WORKAROUND */

BOOL patchRequired = FALSE;
BOOL patchLoaded = FALSE;

WORD bslVer = 0;
WORD _addr, _len, _err;
BYTE speed	= 0;		/* default 9600 Baud		*/
DWORD Time_BSL_starts, Time_PRG_starts, Time_BSL_stops;

char *newBSLFile= NULL;
char newBSLFilename[256];


struct toDoList
	{
	unsigned MassErase : 1;
	unsigned EraseCheck: 1;
	unsigned FastCheck : 1;
	unsigned Program : 1;
	unsigned Verify: 1;
	unsigned Reset	: 1;
	unsigned Wait	: 1;    /* Wait for <Enter> at end of program */
	                        /* (0: no; 1: yes):                   */
	unsigned OnePass : 1;   /* Do EraseCheck, Program and Verify  */
	                        /* in one pass (TI TXT file is read   */
	                        /* only once)                         */
	unsigned SpeedUp : 1;   /* Change Baudrate                    */
	unsigned UserCalled: 1; /* Second run without entry sequence  */
	unsigned BSLStart: 1;   /* Start BSL                          */
	unsigned Dump2file:1;   /* Dump Memory to file                */
	unsigned EraseSegment:1;/* Erase Segment                      */
	unsigned MSP430X:1;     /* Enable MSP430X Ext.Memory support  */
	unsigned RestoreInfoA:1;/* Save InfoA before mass erase       */
	} toDo;


int error= ERR_NONE;
int i, j;
char comPortName[20]= "COM1"; // Default setting.
char *filename= NULL;
char *passwdFile= NULL;
char passwdFilename[256];

BYTE bslVerHi, bslVerLo, devTypeHi, devTypeLo, devProcHi, devProcLo;
WORD bslerrbuf;

#ifdef ADD_MERASE_CYCLES
int meraseCycles= ADD_MERASE_CYCLES;
#else
const int meraseCycles= 1;
#endif // ADD_MERASE_CYCLES


long readStart = 0;
long readLen = 0;
char *readfilename = NULL;


void *errData= NULL;
int byteCtr= 0;

/*---------------------------------------------------------------
* Functions:
*---------------------------------------------------------------
*/

int preparePatch()
	{
	int error= ERR_NONE;

#ifdef WORKAROUND
	if (patchLoaded)
		{
		/* Load PC with 0x0220.
		* This will invoke the patched bootstrap loader subroutines.
		*/
		error= bslTxRx(BSL_LOADPC,	/* Command: Load PC 	*/
			0x0220, 	/* Address to load into PC */
			0,			/* No additional data! 	*/
			NULL, blkin);
		if (error != ERR_NONE) return(error);
		BSLMemAccessWarning= 0; /* Error is removed within workaround code */
		}
#endif /* WORKAROUND */

	return(error);
	}

void postPatch()
	{
#ifdef WORKAROUND
	if (patchLoaded)
		{
		BSLMemAccessWarning= 1; /* Turn warning back on. */
		}
#endif /* WORKAROUND */
	}

int verifyBlk(unsigned long addr, WORD len, unsigned action)
	{
	int i= 0;
	int error= ERR_NONE;

	if ((action & (ACTION_VERIFY | ACTION_ERASE_CHECK)) != 0)
		{

#ifdef DEBUGDUMP
		printf("Check starting at %x, %i bytes... ", addr, len);
#endif /* DEBUGDUMP */

		error= preparePatch();
		if (error != ERR_NONE) return(error);

        if (toDo.MSP430X) {
			if (error = bslTxRx(BSL_MEMOFFSET, 0, (WORD)(addr>>16),	NULL, blkin) !=0)  return (error);
			addr = addr & 0xFFFF;
		}

		error= bslTxRx(BSL_RXBLK, addr, len, NULL, blkin);

		postPatch();

#ifdef DEBUGDUMP
		printf("Error: %i\n", error);
#endif /* DEBUGDUMP */

		if (error != ERR_NONE)
			{
			return(error); /* Cancel, if read error */
			}
		else
			{
			for (i= 0; i < len; i++)
				{
				if ((action & ACTION_VERIFY) != 0)
					{
					/* Compare data in blkout and blkin: */
					if (blkin[i] != blkout[i])
						{
						printf("Verification failed at %x (%x, %x)\n", addr+i, blkin[i], blkout[i]);
						return(ERR_VERIFY_FAILED); /* Verify failed! */
						}
					continue;
					}
				if ((action & ACTION_ERASE_CHECK) != 0)
					{
					/* Compare data in blkin with erase pattern: */
					if (blkin[i] != 0xff)
						{
						printf("Erase Check failed at %x (%x)\n", addr+i, blkin[i]);
						return(ERR_ERASE_CHECK_FAILED); /* Erase Check failed! */
						}
					continue;
					} /* if ACTION_ERASE_CHECK */
				} /* for (i) */
			} /* else */
        if (toDo.MSP430X)
			if (error = bslTxRx(BSL_MEMOFFSET, 0, (WORD)(0),NULL, blkin) !=0)  return (error);
		} /* if ACTION_VERIFY | ACTION_ERASE_CHECK */


	else if ((action & ACTION_ERASE_CHECK_FAST) != 0) /* FRGR 02/01 */
		{

#ifdef DEBUGDUMP
		printf("Fast Check starting at %x, %i bytes... ", addr, len);
#endif /* DEBUGDUMP */

		error= preparePatch();
		if (error != ERR_NONE) return(error);

		error= bslTxRx(BSL_ECHECK, addr, len, NULL, blkin);

		postPatch();

#ifdef DEBUGDUMP
		printf("Error: %i\n", error);
#endif /* DEBUGDUMP */

		if (error != ERR_NONE)
			{
			return(ERR_ERASE_CHECK_FAILED); /* Erase Check failed! */
			}
		} /* if ACTION_ERASE_CHECK_FAST */


	return(error);
	}

int programBlk(unsigned long addr, WORD len, unsigned action)
	{
	int i= 0;
	int error= ERR_NONE;

	if ((action & ACTION_PASSWD) != 0)
		{
		return(bslTxRx(BSL_TXPWORD, /* Command: Transmit Password*/
			addr,		/* Address of interupt vectors */
			len,		/* Number of bytes 			*/
			blkout, blkin));
		} /* if ACTION_PASSWD */

	/* Check, if specified range is erased: */
	if (action & ACTION_ERASE_CHECK)
		error= verifyBlk(addr, len, action & ACTION_ERASE_CHECK);
	else if (action & ACTION_ERASE_CHECK_FAST)
		error= verifyBlk(addr, len, action & ACTION_ERASE_CHECK_FAST);
	if (error != ERR_NONE)
		{
		return(error);
		}

	if ((action & ACTION_PROGRAM) != 0)
		{

#ifdef DEBUGDUMP
		printf("Program starting at %x, %i bytes... ", addr, len);
#endif /* DEBUGDUMP */

		error= preparePatch();
		if (error != ERR_NONE) return(error);

		/* Set Offset: */
		if (toDo.MSP430X) error= bslTxRx(BSL_MEMOFFSET, 0, (WORD)(addr>>16), blkout, blkin);
		if (error != ERR_NONE) return(error);

		/* Program block: */
		error= bslTxRx(BSL_TXBLK, addr, len, blkout, blkin);

		postPatch();

#ifdef DEBUGDUMP
		printf("Error: %i\n", error);
#endif /* DEBUGDUMP */

		if (error != ERR_NONE)
			{
			return(error); /* Cancel, if error (ACTION_VERIFY is skipped!) */
			}
        if (toDo.MSP430X)
			if (error = bslTxRx(BSL_MEMOFFSET, 0, (WORD)(0),NULL, blkin) !=0)  return (error);

		} /* if ACTION_PROGRAM */

	/* Verify block: */
	error= verifyBlk(addr, len, action & ACTION_VERIFY);
	if (error != ERR_NONE)
		{
		return(error);
		}

	return(error);
	} /* programBlk */

unsigned int readStartAddrTIText(char *filename) /* FRGR */
	{
	WORD startAddr=0;
	char strdata[128];
	FILE* infile;

	if ((infile = fopen(filename, "rb")) == 0)
		{
		errData= filename;
		return(ERR_FILE_OPEN);
		}

	/* TXT-File is parsed for first @Addr occurence: */
	while (TRUE)
		{
		/* Read one line: */
		if (fgets(strdata, 127, infile) == 0)   /* if End Of File                */
			{
			break;
			}
		if (strdata[0] == '@')				    /* if @ => start address         */
			{
			sscanf(&strdata[1], "%x\n", &startAddr);
			break;
			}
		}
	fclose(infile);
	return(startAddr);
	} /* readStartAddrTIText */

int programTIText (char *filename, unsigned action)
	{
	int next= 1;
	int error= ERR_NONE;
	int linelen= 0;
	int linepos= 0;
	int i, KBytes, KBytesbefore= -1;
	WORD dataframelen=0;
	unsigned long currentAddr;
	char strdata[128];

	FILE* infile;

	byteCtr= 0;

	if ((infile = fopen(filename, "rb")) == 0)
		{
		errData= filename;
		return(ERR_FILE_OPEN);
		}

	/* Convert data for MSP430, TXT-File is parsed line by line: */
	while (TRUE) /* FRGR */
		{
		/* Read one line: */
		if ((fgets(strdata, 127, infile) == 0) ||
			/* if End Of File				or */
			(strdata[0] == 'q'))
			/* if q (last character in file)	*/
			{	/* => send frame and quit			*/
			if (dataframelen > 0) /* Data in frame? */
				{
				error= programBlk(currentAddr, dataframelen, action);
				byteCtr+= dataframelen; /* Byte Counter */
				dataframelen=0;
				}
			break;	   /* FRGR  */
			}

		linelen= strlen(strdata);

		if (strdata[0] == '@')
			/* if @ => new address => send frame and set new addr. */
			{
			if (dataframelen > 0)
				{
				error= programBlk(currentAddr, dataframelen, action);
				byteCtr+= dataframelen; /* Byte Counter */
				dataframelen=0;
				}
			sscanf(&strdata[1], "%lx\n", &currentAddr);
			continue;
			}

		/* Transfer data in line into blkout: */
		for(linepos= 0;
		linepos < linelen-3; linepos+= 3, dataframelen++)
			{
			sscanf(&strdata[linepos], "%3x", &blkout[dataframelen]);
			/* (Max 16 bytes per line!) */
			}

		if (dataframelen > maxData-16)
			/* if frame is getting full => send frame */
			{
			error= programBlk(currentAddr, dataframelen, action);
			byteCtr+= dataframelen; /* Byte Counter */
			currentAddr+= dataframelen;
			dataframelen=0;

			/* bargraph: indicates succession, actualize only when changed. FRGR */
			KBytes = (byteCtr+512)/1024;
			if (KBytesbefore != KBytes)
				{
				KBytesbefore = KBytes;
				printf("\r%02d KByte ", KBytes);
				printf("\xDE");
				for (i=0;i<KBytes;i+=1) printf("\xB2");
				printf("\xDD");
				}
			}

		if (error != ERR_NONE)
			{
			break;	/* FRGR */
			}
		}
	/* clear bargraph, go to left margin */
	printf("\r \r");

	fclose(infile);

	return(error);
	} /* programTIText */

int txPasswd(char* passwdFile)
	{
	int i;

	if (passwdFile == NULL)
		{
		/* Send "standard" password to get access to protected functions. */
		printf("Transmit standard password...\n");
		/* Fill blkout with 0xff
		* (Flash is completely erased, the contents of all Flash cells is 0xff)
		*/
		for (i= 0; i < 0x20; i++)
			{
			blkout[i]= 0xff;
			}
		return(bslTxRx(BSL_TXPWORD, /* Command: Transmit Password  */
			0xffe0, 	            /* Address of interupt vectors */
			0x0020, 	            /* Number of bytes             */
			blkout, blkin));
		}
	else
		{
		/* Send TI TXT file holding interrupt vector data as password: */
		printf("Transmit PSW file \"%s\"...\n", passwdFile);
		return(programTIText(passwdFile, ACTION_PASSWD));
		}
	} /* txPasswd */

void WaitForKey() /* FRGR */
	{
	printf("----------------------------------------------------------- ");
	printf("Press any key ... "); getch(); printf("\n");
	}

int signOff(int error, BOOL passwd)
	{
	if (toDo.MSP430X) error= bslTxRx(BSL_MEMOFFSET, 0, (WORD)(0), blkout, blkin);

	if (toDo.Reset)
		{
		bslReset(0); /* Reset MSP430 and start user program. */
		}

	switch (error)
		{
		case ERR_NONE:
			if (toDo.Program) printf("Programming completed.");
				else if (toDo.Verify)printf("Verification successful.");
			printf("Prog/Verify: %.1f sec",(float)(Time_BSL_stops-Time_PRG_starts)/1000.0);
			printf(" - Over all: %.1f sec\n",(float)(Time_BSL_stops-Time_BSL_starts)/1000.0);
			break;
		case ERR_BSL_SYNC:
			printf("ERROR: Synchronization failed!\n");
			printf("Device with boot loader connected?\n");
			break;
		case ERR_VERIFY_FAILED:
			printf("ERROR: Verification failed!\n");
			break;
		case ERR_ERASE_CHECK_FAILED:
			printf("ERROR: Erase check failed!\n");
			break;
		case ERR_FILE_OPEN:
			printf("ERROR: Unable to open input file \"%s\"!\n", (char*)errData);
			break;
		default:
			if ((passwd) && (error == ERR_RX_NAK))
				/* If last command == transmit password && Error: */
				printf("ERROR: Password not accepted!\n");
			else
				printf("ERROR: Communication Error!\n");
		} /* switch */

	if (toDo.Wait)
		{
		WaitForKey();
		}

	comDone();	/* Release serial communication port.	*/
				/* After having released the serial port,
				 * the target is no longer supplied via this port!
	             */
	if (error == ERR_NONE)
		return(0);
	else
		return(1);
	} /* signOff */


void showHelp()
	{
	char *help[]=
		{
		"BSLDEMO-2.01c [-h][-c{port}][-p{file}][-w][-1][-m{num}][+aecpvruw] {file}",
			"",
			/*
			"The last parameter is required: file name of TI-TXT file to be programmed.",
			"",
			*/
			"Options:",
			"-h       Shows this help screen.",
			"-c{port} Specifies the communication port to be used (e.g. -cCOM2).",
#ifdef WORKAROUND
			"-a{file} Filename of workaround patch (e.g. -aWAROUND.TXT).",
#endif
			"-b{file} Filename of complete loader to be loaded into RAM (e.g. -bBSL.TXT).",
			"-e{startnum}",
			"         Erase Segment where address does point to.",
			/*
			"-f{num}  Max. number of data bytes within one transmitted frame (e.g. -f240).",
			*/

/*  Change by GH */

			"-i       Invert polarity of DTR line.",
			"-j       Invert polarity of RTS line.",

#ifdef ADD_MERASE_CYCLES
			"-m{num}  Number of mass erase cycles (e.g. -m20).",
#endif /* ADD_MERASE_CYCLES */
			"-p{file} Specifies a TI-TXT file with the interrupt vectors that are",
			"         used as password (e.g. -pINT_VECT.TXT).",
			"-r{startnum} {lennum} {file}",
			"         Read memory from startnum till lennum and write to file as TI.TXT.",
			"         (Values in hex format.) ",
			"-s{num}  Changes the baudrate; num=0:9600, 1:19200, 2:38400 (e.g. -s2).",
			"-w       Waits for <ENTER> before closing serial port.",
			"-x       Enable MSP430X Extended Memory support.",
			"-1       Programming and verification is done in one pass through the file.",
			"",
			"Program Flow Specifiers [+aecipvruw]",
			" a       Restore InfoA after mass erase (only with Mass Erase)",
			" e       Mass Erase",
			" c       Erase Check by file {file}",
			" p       Program file {file}",
			" v       Verify by file {file}",
			" r       Reset connected MSP430. Starts application.",
			" u       User Called - Skip entry Sequence.",
			" w       Wait for <ENTER> before closing serial port.",
			" Only the specified actions are executed!",
			"",
			"Default Program Flow Specifiers (if not explicitly given): +ecpvr",
			"",
			"\x1B " /* Marks end of help! <ESC>*/
		};

	int i= 0;

	while (*help[i] != 0x1B)
		printf("%s\n", help[i++]);
	printf("Press any key to exit... ");
	getch();
	}


/*-------------------------------------------------------------
* Parse Command Line Parameters ...
*-------------------------------------------------------------
*/
int parseCMDLine(int argc, char *argv[])
{
   /* Default: all actions turned on: */
   toDo.MassErase = 1;
   toDo.EraseCheck= 1;
   toDo.FastCheck = 0;
   toDo.Program = 1;
   toDo.Verify= 1;
   toDo.Reset   = 1;
   toDo.SpeedUp = 0; /* Change Baudrate */
   toDo.UserCalled= 0;
   toDo.Wait   = 0;  /* Do not wait for <Enter> at the end! */
   toDo.OnePass = 0; /* Do erase check, program and verify*/
   toDo.BSLStart= 1;
   toDo.Dump2file = 0;
   toDo.EraseSegment = 0;
   toDo.MSP430X = 0;
   toDo.RestoreInfoA = 0;

   filename   = NULL;
   passwdFile = NULL;
   patchFile = patchFilename;


   if (argc > 1)
      {
      for (i= 1; i < (argc); i++)
         {
         switch (argv[i][0])
            {
            case '-':
               switch (argv[i][1])
                  {
                  case 'h': case 'H':
                     showHelp(); /* Show help screen and */
                     return(1);   /* exit program.      */
                     break;

                  case 'c': case 'C':
                     strcpy(comPortName, "\\\\.\\");  /* Required by Windows */
                     memcpy(&comPortName[4], &argv[i][2], strlen(argv[i])-2);
                     break;

                  case 'p': case 'P':
                     strcpy(passwdFilename, &argv[i][2]);
                     passwdFile = passwdFilename;
                     break;

                  case 'w': case 'W':
                     toDo.Wait= 1; /* Do wait for <Enter> at the end! */
                     break;

                  case '1':
                     toDo.OnePass= 1;
                     break;

/*  Change by GH */

                  case 'i':
                     InvertDTR = TRUE;
                     break;

                  case 'j':
                     InvertRTS = TRUE;
                     break;


                  case 's': case 'S':
                     if ((argv[i][2] >= '0') && (argv[i][2] <= '9'))
                        {
                        speed = argv[i][2] - 0x30;   /* convert ASCII to number */
                        toDo.SpeedUp= 1;
                        }
                     break;

                  case 'f': case 'F':
                     if (argv[i][2] != 0)
                        {
                        sscanf(&argv[i][2], "%i", &maxData);
                        /* Make sure that conditions for maxData are met:
                        * ( >= 16 and == n*16 and <= MAX_DATA_BYTES!)
                        */
                        maxData= (maxData > MAX_DATA_BYTES) ? MAX_DATA_BYTES : maxData;
                        maxData= (maxData <          16) ?         16 : maxData;
                        maxData= maxData - (maxData % 16);
                        printf("Max. number of data bytes within one frame set to %i.\n",
                           maxData);
                        }
                     break;

#ifdef ADD_MERASE_CYCLES
                  case 'm': case 'M':
                     if (argv[i][2] != 0)
                        {
                        sscanf(&argv[i][2], "%i", &meraseCycles);
                        meraseCycles= (meraseCycles < 1) ? 1 : meraseCycles;
                        printf("Number of mass erase cycles set to %i.\n", meraseCycles);
                        }
                     break;
#endif /* ADD_MERASE_CYCLES */

#ifdef WORKAROUND
                  case 'a': case 'A':
                     strcpy (patchFilename ,&argv[i][2]);
                            patchFile=patchFilename;
                     break;
#endif /* WORKAROUND */

#ifdef NEW_BSL
                  case 'b': case 'B':
                     strcpy (newBSLFilename ,&argv[i][2]);
                            newBSLFile=newBSLFilename;
                     break;
#endif /* NEW_BSL */
                  case 'r': case 'R':
                     toDo.MassErase = 0;
                     toDo.EraseCheck= 0;
                     toDo.FastCheck = 0;
                     toDo.Program = 0;
                     toDo.Verify= 0;

                     toDo.Dump2file = 1;
                     sscanf(&argv[i][2], "%X", &readStart);
                     i++;
                     sscanf(&argv[i][0], "%X", &readLen);
                     i++;
                     readfilename = &argv[i][0];
                     break;
                  case 'e': case 'E':
                     toDo.MassErase = 0;
                     toDo.EraseCheck= 0;
                     toDo.FastCheck = 0;
                     toDo.Program = 0;
                     toDo.Verify= 0;
                     toDo.Reset   = 0;
                     toDo.UserCalled= 0;
                     toDo.BSLStart= 1;
                     toDo.Dump2file = 0;

                     toDo.EraseSegment = 1;
                     sscanf(&argv[i][2], "%X", &readStart);
                     i++;
                     break;
                  case 'x': case 'X':
                     toDo.MSP430X = 1;
                     break;

                  default:
                     printf("ERROR: Illegal command line parameter!\n");
                  } /* switch argv[i][1] */
               break; /* '-' */

            case '+':
                     /* Turn all actions off: */
                     toDo.MassErase = 0;
                     toDo.EraseCheck= 0;
                     toDo.FastCheck = 0;
                     toDo.Program = 0;
                     toDo.Verify= 0;
                     toDo.Reset   = 0;
                     toDo.UserCalled= 0;
                     toDo.BSLStart= 1;
					 toDo.RestoreInfoA = 0;

                     /* Turn only specified actions back on:             */
                     for (j= 1; j < (int)(strlen(argv[i])); j++)
                        {
                        switch (argv[i][j])
                           {
                           case 'a': case 'A':
                              /* Restore InfoA Segment              */
							  toDo.RestoreInfoA = 1;
                           case 'e': case 'E':
                              /* Erase Flash                        */
                              toDo.MassErase = 1;
                              break;
                           case 'c': case 'C':
                              /* Erase Check (by file)               */
                              toDo.EraseCheck= 1;
                              break;
                           case 'f': case 'F':
                              /* Fast Erase Check (by file)            */
                              toDo.FastCheck= 1;
                              break;
                           case 'p': case 'P':
                              /* Program file                      */
                              toDo.Program = 1;
                              break;
                           case 'r': case 'R':
                              /* Reset MSP430 before waiting for <Enter>   */
                              toDo.Reset   = 1;
                              break;
                           case 'u': case 'U':
                              /* Second run without entry sequence      */
                              toDo.UserCalled= 1;
                              break;
                           case 'v': case 'V':
                              /* Verify file                        */
                              toDo.Verify= 1;
                              break;
                           case 'w': case 'W':
                              /* Wait for <Enter> before closing serial port */
                              toDo.Wait   = 1;
                              break;
                           case 'x': case 'X':
                              /* Start BSL ??? */
                              toDo.BSLStart   = 0;
                              break;
                           default:
                              printf("ERROR: Illegal action specified!\n");
                           } /* switch */
                        } /* for (j) */
                     break; /* '+' */

                  default:
                       filename= argv[i];
         } /* switch argv[i][0] */
      } /* for (i) */
   }
   else
   {
      showHelp();
      return(1);
   }

   return(0);
}

/*---------------------------------------------------------------
* Main:
*---------------------------------------------------------------
*/
int main(int argc, char *argv[])
{
	const WORD SMALL_RAM_model = 0;
	const WORD LARGE_RAM_model = 1;
	const WORD ROM_model = 0x4567;
	WORD loadedModel = ROM_model;
	int stat = 0;
	unsigned char infoA[0x40];

#ifdef NEW_BSL
	newBSLFile= NULL;
#endif // NEW_BSL


#ifdef WORKAROUND
	/* Show memory access warning, if working with bootstrap
	* loader version(s) requiring the workaround patch.
	* Turn warning on by default until we can determine the
	* actual version of the bootstrap loader.
	*/
	BSLMemAccessWarning= 1;
#endif /* WORKAROUND */

	printf("%s (%s)\n", programName, programVersion);

    stat = parseCMDLine(argc, argv);
    if (stat != 0) return(stat);


/*-------------------------------------------------------
* Communication with Bootstrap Loader ...
*-------------------------------------------------------
*/


	/* Open COMx port (Change COM-port name to your needs!): */
	if (comInit(comPortName, DEFAULT_TIMEOUT, 4) != 0)
		{
		printf("ERROR: Opening COM-Port failed!\n");
		if (toDo.Wait)
			{
			WaitForKey(); /* FRGR */
			}
		return(0);
		}

	if (toDo.UserCalled)
	{ }
    else
	{

		bslReset(toDo.BSLStart); /* Invoke the boot loader. */
		//delay(2000);
		Time_BSL_starts = GetTickCount();
	}


	Repeat:

#ifdef NEW_BSL



if ((newBSLFile == NULL) || (passwdFile == NULL))
	{
	/* If a password file is specified the "new" bootstrap loader can be loaded
	* (if also specified) before the mass erase is performed. Then the mass
	* erase can be done using the "new" BSL. Otherwise the mass erase is done
	* now!
	*/
#endif /* NEW_BSL */


	if (toDo.RestoreInfoA && toDo.MassErase)
		{

		/* Read actual InfoA segment Content. */
		printf("Read InfoA Segment...\n");
		/* Transmit password to get access to protected BSL functions. */
		if ((error= txPasswd(passwdFile)) != ERR_NONE)
			{
			return(signOff(error, TRUE)); /* Password was transmitted! */
			}
		if (toDo.MSP430X) if (bslTxRx(BSL_MEMOFFSET, 0, 0, NULL, blkin) !=0)  return (signOff(error, TRUE));
		if ((error= bslTxRx(BSL_RXBLK, /* Command: Read/Receive Block 	*/
			0x010C0,	/* Start address					*/
			0x40,		/* No. of bytes to read			*/
			NULL, infoA)) != ERR_NONE)
			{
				return(signOff(error, FALSE));
			}
		}
	else
		{
		toDo.RestoreInfoA = 0;
		}


	if (toDo.MassErase)
		{
		int i;
		/* Erase the flash memory completely (with mass erase command): */
		printf("Mass Erase...\n");
		for (i= 0; i < meraseCycles; i++)
			{
			if (i == 1)
				{
				printf("Additional mass erase cycles...\n");
				}
			if ((error= bslTxRx(BSL_MERAS, /* Command: Mass Erase 			*/
				0xff00,	/* Any address within flash memory. */
				0xa506,	/* Required setting for mass erase! */
				NULL, blkin)) != ERR_NONE)
				{
				return(signOff(error, FALSE));
				}
			}
		passwdFile= NULL; /* No password file required! */
		}

#ifdef NEW_BSL
	} /* if ((newBSLFile == NULL) || (passwdFile == NULL)) */
#endif /* NEW_BSL */

/* Transmit password to get access to protected BSL functions. */
	if (!(toDo.UserCalled))
		if ((error= txPasswd(passwdFile)) != ERR_NONE)
			{
			return(signOff(error, TRUE)); /* Password was transmitted! */
			}

/* Read actual bootstrap loader version (FRGR: complete Chip ID). */
    if (toDo.MSP430X) if (bslTxRx(BSL_MEMOFFSET, 0, 0, NULL, blkin) !=0)  return (signOff(error, TRUE));
	if ((error= bslTxRx(BSL_RXBLK, /* Command: Read/Receive Block 	*/
		0x0ff0,	/* Start address					*/
		14,		/* No. of bytes to read			*/
		NULL, blkin)) == ERR_NONE)
	{
	memcpy(&devTypeHi,&blkin[0x00], 1);
	memcpy(&devTypeLo,&blkin[0x01], 1);
	memcpy(&devProcHi,&blkin[0x02], 1);
	memcpy(&devProcLo,&blkin[0x03], 1);
	memcpy(&bslVerHi, &blkin[0x0A], 1);
	memcpy(&bslVerLo, &blkin[0x0B], 1);

	printf("BSL version: %X.%02X",	bslVerHi,bslVerLo);
	printf(" - Family member: %02X%02X", devTypeHi, devTypeLo);
	printf(" - Process: %02X%02X\n",	devProcHi, devProcLo);

	bslVer= (bslVerHi << 8) | bslVerLo;

	if (bslVer < 0x0150) bslerrbuf = 0x021E; else bslerrbuf = 0x0200;

	if (bslVer <= 0x0110)
		{

#ifdef WORKAROUND
#ifdef NEW_BSL
		if (newBSLFile == NULL)
			{
#endif /* NEW_BSL */
			printf("Patch for flash programming required!\n");
			patchRequired= TRUE;
#ifdef NEW_BSL
			}
#endif /* NEW_BSL */
#endif /* WORKAROUND */

		BSLMemAccessWarning= 1;
		}
	else
		{
		BSLMemAccessWarning= 0; /* Fixed in newer versions of BSL. */
		}
	}


	if (patchRequired || ((newBSLFile != NULL) && (bslVer <= 0x0110)))
	{
	/* Execute function within bootstrap loader
	* to prepare stack pointer for the following patch.
	* This function will lock the protected functions again.
	*/
	printf("Load PC with 0x0C22...\n");
	if ((error= bslTxRx(BSL_LOADPC, /* Command: Load PC		*/
		0x0C22,	/* Address to load into PC */
		0,		/* No additional data!	*/
		NULL, blkin)) != ERR_NONE)
		{
		return(signOff(error, FALSE));
		}

	/* Re-send password to re-gain access to protected functions. */
	if ((error= txPasswd(passwdFile)) != ERR_NONE)
		{
		return(signOff(error, TRUE)); /* Password was transmitted! */
		}
	}

#ifdef NEW_BSL
	if (newBSLFile != NULL)
	{
	WORD startaddr; /* used twice: vector or start address */
	startaddr = readStartAddrTIText(newBSLFile);
	if (startaddr == 0)
		{
		startaddr = 0x0300;
		}

	printf("Load");
	if (bslVer >= 0x0140) printf("/Verify");
	printf(" new BSL \"%s\" into RAM at 0x%04X...\n", newBSLFile, startaddr);
	if ((error= programTIText(newBSLFile, /* File to program */
		ACTION_PROGRAM)) != ERR_NONE)
		{
		return(signOff(error, FALSE));
		}
	if (bslVer < 0x0140)
		{
		printf("Verify new BSL \"%s\"...\n", newBSLFile);
		if ((error= programTIText(newBSLFile, /* File to verify */
			ACTION_VERIFY)) != ERR_NONE)
			{
			return(signOff(error, FALSE));
			}
		}

	/* Read startvector/loaded model of NEW bootstrap loader: */
	if ((error= bslTxRx(BSL_RXBLK, startaddr, 4, NULL, blkin)) == ERR_NONE)
		{
		memcpy(&startaddr, &blkin[0], 2);
		memcpy(&loadedModel, &blkin[2], 2);
		if (loadedModel != SMALL_RAM_model)
			{
			loadedModel= LARGE_RAM_model;
			bslerrbuf = 0x0200;
			printf("Start new BSL (LARGE model > 1K Bytes) at 0x%04X...\n", startaddr);
			}
		else
			printf("Start new BSL (SMALL model < 512 Bytes) at 0x%04X...\n", startaddr);

		error= bslTxRx(BSL_LOADPC, /* Command: Load PC		*/
			startaddr,/* Address to load into PC */
			0,		/* No additional data!	*/
			NULL, blkin);
		}
	if (error != ERR_NONE)
		{
		return(signOff(error, FALSE));
		}

	/* BSL-Bugs should be fixed within "new" BSL: */
	BSLMemAccessWarning= 0;
	patchRequired= FALSE;
	patchLoaded= FALSE;

	if (loadedModel != SMALL_RAM_model)
		{
		/* Re-send password to re-gain access to protected functions. */
		if ((error= txPasswd(passwdFile)) != ERR_NONE)
			{
			return(signOff(error, TRUE)); /* Password was transmitted! */
			}
		}
	}
#endif/* NEW_BSL */

#ifdef WORKAROUND
	if (patchRequired)
	{
	printf("Load and verify patch \"%s\"...\n", patchFile);
	/* Programming and verification is done in one pass.
	* The patch file is only read and parsed once.
	*/
	if ((error= programTIText(patchFile, /* File to program */
		ACTION_PROGRAM | ACTION_VERIFY)) != ERR_NONE)
		{
		return(signOff(error, FALSE));
		}
	patchLoaded= TRUE;
	}
#endif /* WORKAROUND */

#ifdef NEW_BSL
	if ((newBSLFile != NULL) && (passwdFile != NULL) && toDo.MassErase)
	{
	/* Erase the flash memory completely (with mass erase command): */
	printf("Mass Erase...\n");
	if ((error= bslTxRx(BSL_MERAS, /* Command: Mass Erase 			*/
		0xff00,	/* Any address within flash memory. */
		0xa506,	/* Required setting for mass erase! */
		NULL, blkin)) != ERR_NONE)
		{
		return(signOff(error, FALSE));
		}
	passwdFile= NULL; /* No password file required! */
	}
#endif /* NEW_BSL*/


/* FRGR */
	if (toDo.SpeedUp) // 0:9600, 1:19200, 2:38400 (3:56000 not applicable)
	{
	DWORD BR; 					// Baudrate
	if ((devTypeHi == 0xF1) || (devTypeHi == 0x12)) // F1232 / F1xx
		{
		BYTE BCSCTL1, DCOCTL; 		// Basic Clock Module Registers
		switch (speed)		    	// for F148, F149, F169
			{ 												//Rsel DCO
			case 0:  BR = CBR_9600;  BCSCTL1 = 0x85; DCOCTL = 0x80; break;// 5	4
			case 1:  BR = CBR_19200; BCSCTL1 = 0x86; DCOCTL = 0xE0; break;// 6	7
			case 2:  BR = CBR_38400; BCSCTL1 = 0x87; DCOCTL = 0xE0; break;// 7	7
			default: BR = CBR_9600;  BCSCTL1 = 0x85; DCOCTL = 0x80; speed = 0;
			}
		_addr = (BCSCTL1 << 8) + DCOCTL;// D2, D1: values for CPU frequency
		}
	else if (devTypeHi == 0xF4)
		{
		BYTE SCFI0, SCFI1;			// FLL+ Registers
		switch (speed)			    // for F448, F449
			{ 												//NDCO FN_x
			case 0:  BR = CBR_9600;  SCFI1 = 0x98; SCFI0= 0x00; break;// 19	0
			case 1:  BR = CBR_19200; SCFI1 = 0xB0; SCFI0= 0x00; break;// 22	0
			case 2:  BR = CBR_38400; SCFI1 = 0xC8; SCFI0= 0x00; break;// 25	0
			default: BR = CBR_9600;  SCFI1 = 0x98; SCFI0= 0x00; speed = 0;
			}
		_addr = (SCFI1 << 8) + SCFI0; // D2, D1: values for CPU frequency
		}
	else if (devTypeHi == 0xF2 || devTypeHi == 0x25)
		{
		BYTE BCSCTL1, DCOCTL; 		// Basic Clock Module Registers
		switch (speed)			    // for F2xx
			{ 												//Rsel DCO
			case 0:  BR = CBR_9600;  BCSCTL1 = 0x88; DCOCTL = 0x80; break;// 5	4
			case 1:  BR = CBR_19200; BCSCTL1 = 0x8B; DCOCTL = 0x80; break;// 6	7
			case 2:  BR = CBR_38400; BCSCTL1 = 0x8C; DCOCTL = 0x80; break;// 7	7
			default: BR = CBR_9600;  BCSCTL1 = 0x88; DCOCTL = 0x80; speed = 0;
			}
		_addr = (BCSCTL1 << 8) + DCOCTL;// D2, D1: values for CPU frequency
		}
	_len	= speed;				// D3: index for baudrate (speed)

	if (BR != comGetBaudrate()) 	// change only if not same speed
		{
		printf("Change Baudrate ");
		error= bslTxRx(BSL_SPEED, 	// Command: Change Speed
			_addr,		// Mandatory code
			_len, 		// Mandatory code
			NULL, blkin);

		if (error == ERR_NONE)
			{
			printf("from %d ", comGetBaudrate());
			comChangeBaudrate(BR);
			delay(10);
			printf("to %d Baud (Mode: %d)\n", comGetBaudrate(),speed);
			}
		else
			{
			printf("command not accepted. Baudrate remains at %d Baud\n", comGetBaudrate());
			}
		}
	}




 Time_PRG_starts = GetTickCount();
 //printf("Start time measurement for pure Prog/Verify cycle...\n");

 if (!toDo.OnePass)
	{
	if (toDo.EraseCheck)
		{
		/* Parse file in TXT-Format and check the erasure of required flash cells. */
		printf("Erase Check by file \"%s\"...\n", filename);
		if ((error= programTIText(filename, ACTION_ERASE_CHECK)) != ERR_NONE)
			{
			return(signOff(error, FALSE));
			}
		}

	if (toDo.FastCheck)
		{
		/* Parse file in TXT-Format and check the erasure of required flash cells. */
		printf("Fast E-Check by file \"%s\"...\n", filename);
		if ((error= programTIText(filename, ACTION_ERASE_CHECK_FAST)) != ERR_NONE)
			{
			return(signOff(error, FALSE));
			}
		}

	if (toDo.Program)
		{
		/* Parse file in TXT-Format and program data into flash memory. */
		printf("Program \"%s\"...\n", filename);
		if ((error= programTIText(filename, ACTION_PROGRAM)) != ERR_NONE)
			{
			if (newBSLFile == NULL)
				return(signOff(ERR_VERIFY_FAILED, FALSE));
			else
				{
				// read out error address+3 from RAM (error address buffer)
				if ((loadedModel == LARGE_RAM_model) || (loadedModel == SMALL_RAM_model))
					{
					if (toDo.MSP430X)
						if (bslTxRx(BSL_MEMOFFSET, 0, 0, NULL, blkin) !=0) signOff(error, TRUE);
					if ((error= bslTxRx(BSL_RXBLK, bslerrbuf, 2, NULL, blkin)) == ERR_NONE)
						{
						_err = (blkin[1] << 8) + blkin[0];
						printf("Verification Error at 0x%04X\n", _err-3);
						}
					else
						return(signOff(error, FALSE));
					}
				else
					return(signOff(ERR_VERIFY_FAILED, FALSE));
				}
			}
		else
			{
			printf("%i bytes programmed.\n", byteCtr);
			}
		}

	if (toDo.Verify)
		{
		if ((toDo.Program) && ((bslVer >= 0x0140) || (newBSLFile != NULL)))
			{
			printf("Verify... already done during programming.\n");
			}
		else
			{
			/* Verify programmed data: */
			printf("Verify\"%s\"...\n", filename);
			if ((error= programTIText(filename, ACTION_VERIFY)) != ERR_NONE)
				{
				return(signOff(error, FALSE));
				}
			}
		}
	}
	else
	{
	unsigned action= 0;
	if (toDo.EraseCheck)
		{
		action |= ACTION_ERASE_CHECK;	printf("EraseCheck ");
		}
	if (toDo.FastCheck)
		{
		action |= ACTION_ERASE_CHECK_FAST; printf("EraseCheckFast ");
		}
	if (toDo.Program)
		{
		action |= ACTION_PROGRAM;		printf("Program ");
		}
	if (toDo.Verify)
		{
		action |= ACTION_VERIFY; 		printf("Verify ");
		}

	if (action != 0)
		{
		printf("\"%s\" ...\n", filename);
		error= programTIText(filename, action);
		if (error != ERR_NONE)
			{
			return(signOff(error, FALSE));
			}
		else
			{
			printf("%i bytes programmed.\n", byteCtr);
			}
		}
	}

	if (toDo.RestoreInfoA)
		{

		unsigned long startaddr = 0x10C0;
		WORD len = 0x40;

		printf("Restore InfoA Segment...\n");
		/* Restore actual InfoA segment Content. */
		if (toDo.MSP430X) if (bslTxRx(BSL_MEMOFFSET, 0, 0, NULL, blkin) !=0)  return (signOff(error, TRUE));

		while ((len > 0) && (infoA[0x40-len] == 0xff))
			{
			len --;
			startaddr++;
			}

        if (len > 0)
			{
			memcpy(blkout, &infoA[startaddr - 0x10C0], len);
			if (error= programBlk(startaddr, len, ACTION_PROGRAM))
				{
					return(signOff(error, FALSE));
				}
			}
		}

	if (toDo.Dump2file)
	{
		BYTE* DataPtr = NULL;
		BYTE* BytesPtr = NULL;
		long byteCount = readLen;
		long addrCount = readStart;
		BytesPtr = DataPtr = (BYTE*) malloc(sizeof(BYTE) * readLen);
		printf("Read memory to file: %s Start: 0x%-4X Length 0x%-4X\n", readfilename, readStart, readLen);

		if (toDo.MSP430X) {
			if (error = bslTxRx(BSL_MEMOFFSET, 0, (WORD)(addrCount>>16), NULL, blkin) !=ERR_NONE) return(signOff(error, FALSE));
			addrCount = addrCount & 0xFFFF;
		}
        while (byteCount > 0)
		{
			if (byteCount > maxData)
			{	/* Read data. */
				printf("  Read memory Start: 0x%-4X Length %d\n", (readStart & 0xFFFF0000)+addrCount, maxData);
				if ((error= bslTxRx(BSL_RXBLK,	/* Command: Read/Receive Block 	*/
					(WORD)addrCount,			/* Start address					*/
					(WORD)maxData,	   			/* No. of bytes to read			*/
					NULL, BytesPtr)) != ERR_NONE) return(signOff(error, FALSE));
			}
			else
			{
				/* Read data. */
				printf("  Read memory Start: 0x%-4X Length %d\n", (readStart & 0xFFFF0000)+addrCount, byteCount);
				if ((error= bslTxRx(BSL_RXBLK,	/* Command: Read/Receive Block 	*/
					(WORD)addrCount,			/* Start address					*/
					(WORD)byteCount,			/* No. of bytes to read			*/
					NULL, BytesPtr)) != ERR_NONE) return(signOff(error, FALSE));
			}
			byteCount -= maxData;
			addrCount += maxData;
			BytesPtr  += maxData;
		}

		StartTITextOutput(readfilename);
		WriteTITextBytes((unsigned long) readStart, (WORD) (readLen/2), DataPtr);
		FinishTITextOutput();

		if (DataPtr != NULL) free (DataPtr);
        if (toDo.MSP430X)
			if (error = bslTxRx(BSL_MEMOFFSET, 0, (WORD)(0),NULL, blkin) !=0)  return (error);

	}

	if (toDo.EraseSegment)
	{
		long addrCount = readStart;
		printf("Erase Segment: 0x%-4X\n", readStart);

		if (toDo.MSP430X) {
			if (error = bslTxRx(BSL_MEMOFFSET, 0, (WORD)(addrCount>>16), NULL, blkin) !=ERR_NONE) return(signOff(error, FALSE));
			addrCount = addrCount & 0xFFFF;
		}

		if (error = bslTxRx(BSL_ERASE, addrCount, 0xA502, NULL, blkin) !=ERR_NONE) return(signOff(error, FALSE));
        if (toDo.MSP430X)
			if (error = bslTxRx(BSL_MEMOFFSET, 0, (WORD)(0),NULL, blkin) !=0)  return (error);
	}

	if (toDo.UserCalled)
	{
		toDo.UserCalled = 0;
		//bslReset(0); /* Reset MSP430 and start user program. */
		printf("No Device reset - BSL called from user program ---------------------\n");
		//delay(100);
		goto Repeat;
	}

	Time_BSL_stops = GetTickCount();

    return(signOff(ERR_NONE, FALSE));
}

/* EOF */

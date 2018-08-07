/*

BSLG2xx12.exe COMn              (Read BSL version and AppStart location)
BSLG2xx12.exe COMn filename     (Flash new firmware)

This is software for the Windows console that flashes firmware to MSP430G
flash-memory Value Line microcontrollers in which the matching "custom" BSL
firmware code has been installed. The command line inputs are the COM port of
the USB-to-UART adapter and the filename containing the new firmware to be
flashed.  A sync protocol has been added to the original TI custom BSL which
establishes communications between the two ends, and identifies which version of
the custom BSL is installed in the  controller, the beginning address of MAIN
memory, and the address where the app must start.

This system works for all MSP430G parts with these characteristics:

 - No built-in BSL code                G2xx1 and G2xx2
 - 1K to 8K of flash MAIN memory       excluding G20xx - only 512B of flash
 - At least 1 MHz DCO calibration
 - INFOA-stored calibration data limited to ADC10 and DCO, beginning at 0x10DC
 - The TA0 CCIS0 input must be on P1.1

This includes the G21x1, G22x1, G2x02, G2x12, G2x52, and G2x32 parts, including
the popular G2231 and G2452.

The "INFO" version of BSL is located entirely in INFO memory beginning at 0x1000
at the bottom of INFOD, and continuing up through most of INFOA. Firmware
compatible with this version must always begin at the start of MAIN memory.

The "Split" version is located in the first 96 (0x60) bytes of MAIN memory, and
in INFOA through INFOC.  INFOD is not used for BSL, and can be erased and
written to by the application.  Firmware compatible with the Split version must
always begin at the start of MAIN + 0x60.  [This program will also work with an
earlier BSL written specifically for the G2231, the Split version of which only
used 80 (0x50) bytes of MAIN memory.]

The firmware file may be in Intel-HEX or TI-TXT format. The program will read in
from the chip and display the BSL version installed and the address where the
app must begin, and then perform error checking to make sure the firmware file
complies.  The reset vector at 0xFFFE as specified in the firmware file must
always point to MAIN, or to MAIN + 0x60 for Split BSL.

A USB driver will also have to be installed for the USB-to-UART adapter being
used.

This program pulses the DTR line low at the very beginning, which will reset the
processor if DTR is connected to its /Reset pin.  The special signalling pattern
on /Reset and Test or TCK, used to initiate BSL in parts with embedded BSLs, is
not used in this system.

A PDF file with further information accompanies this program.

This program was written in C for the LCC-Win32 compiler.

*/

#include <stdbool.h>
#include <stdio.h>
#include <conio.h>

long MainStart = 0xE000;
long splitsize = 0x60;
long SplitStart = 0xE060;
int firmwarelen = 8191;
int comport = 0;
int filearg = 0;
int filelen = 0;
unsigned char cmdbyte = 0xBA;
unsigned char ACK = 0xF8;
unsigned char NACK = 0xFE;

unsigned char Response[9] = {0xFF,0,0xC0,0x80,0xFC,0xF8,0xF0,0xE0,0xFE};   /* Split/Info, 1k-8k */
long MainBeg[9] = {0xFC00,0xF800,0xF000,0xE000,0xFC00,0xF800,0xF000,0xE000,0xF800};
int BSLType[9] = {1,1,1,1,0,0,0,0,2};
int BSL = 3;                /* 0 = INFO, 1 = Split, 2 = old 2231Split-0x50, 3 = undefined */

/*======== Receive data from COM port - code per Silicon Labs AN197.pdf. ====*/

bool ReadData(HANDLE handle, BYTE* data, DWORD length, DWORD* dwRead, UINT timeout)
{
	bool success = false;
	OVERLAPPED o = {0};
	o.hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
	if (!ReadFile(handle, data, length, dwRead, &o))
	{
		if (GetLastError() == ERROR_IO_PENDING)
		if (WaitForSingleObject(o.hEvent, timeout) == WAIT_OBJECT_0)
			success = true;
		GetOverlappedResult(handle, &o, dwRead, FALSE);
	}
	else success = true;
	CloseHandle(o.hEvent);
	return success;
}

/*======= Transmit data through COM port - code per Silicon Labs AN197.pdf. =====*/

bool WriteData(HANDLE handle, BYTE* data, DWORD length, DWORD* dwWritten)
{
	bool success = false;
	OVERLAPPED o = {0};
	o.hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
	if (!WriteFile(handle, (LPCVOID)data, length, dwWritten, &o))
	{
		if (GetLastError() == ERROR_IO_PENDING)
		if (WaitForSingleObject(o.hEvent, INFINITE) == WAIT_OBJECT_0)
		if (GetOverlappedResult(handle, &o, dwWritten, FALSE))
			success = true;
	}
	else success = true;
	if (*dwWritten != length) success = false;
	CloseHandle(o.hEvent);
	return success;
}

/*======== Display proper usage of program if /? or error. ===========*/

void Usage(char *programName)
{
	printf("\n%s usage:\n \n",programName);
	printf("Get BSL info:      %s COMn \n",programName);
	printf("Flash firmware:    %s COMn filename \n",programName);
}

/*======== Process command line arguments. ==========*/

void HandleOptions(int argc,char *argv[])
{
	int i;

	for (i=1; i< argc;i++)
	{
		if (argv[i][0] == '/' || argv[i][0] == '-')
		{
			continue;
		}
		else if (strnicmp(argv[i], "COM", 3) == 0)
		{
			comport = i;
		}

		else if (strlen(argv[i]) > 3)
		{
			filearg = i;
		}
		else return;
	}
	return;
}

/*============MAIN==========*/

int main(int argc,char *argv[])
{

	/*handle the program options*/
	HandleOptions(argc,argv);
	if (comport == 0)
	{
		Usage(argv[0]);
		return 0;
	}

	/*Init 8K buffer to all FFs */

	int i;
	int j;
	unsigned char buf[firmwarelen+1];

	for (i = 0; i < firmwarelen+1; i++)   /* create binary 8K MAIN image*/
    {									  /*  with all FFs*/
		buf[i] = 0xff;
	}

	/*Open port, send NAK, read Response byte*/

	unsigned char Outbyte = NACK;   /* anything but the command byte */
	char *pOutbyte = &Outbyte;
	int Outwrote = 0;
	int *pOutwrote = &Outwrote;
	unsigned char Inbyte = 0;
	char *pInbyte = &Inbyte;
	int Fetched = 0;
	int *pFetched = &Fetched;

	char CFcomport[20] = "\\\\.\\";
	strcat (CFcomport,argv[comport]);


	/* Open COM port */

	HANDLE hMasterCOM = CreateFile(CFcomport,
    GENERIC_READ | GENERIC_WRITE,
    0,
    0,
    OPEN_EXISTING,
    FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED,
    0);

	if (hMasterCOM == INVALID_HANDLE_VALUE)
	{
		int err = GetLastError();
		printf("Error opening port \n");
		return err;
	}
	else
	{
		printf("COM port Opened \n");
	}

	PurgeComm(hMasterCOM, PURGE_TXABORT | PURGE_RXABORT | PURGE_TXCLEAR | PURGE_RXCLEAR);

	DCB dcbMasterInitState;
	GetCommState(hMasterCOM, &dcbMasterInitState);

	DCB dcbMaster = dcbMasterInitState;
	dcbMaster.BaudRate = 9600;
	dcbMaster.Parity = NOPARITY;
	dcbMaster.ByteSize = 8;
	dcbMaster.StopBits = ONESTOPBIT;
	SetCommState(hMasterCOM, &dcbMaster);

	sleep(400);

	printf("Resetting MCU via DTR, if connected to /Reset \n");
	EscapeCommFunction(hMasterCOM, SETDTR);						/* toggle DTR (Reset) */
	EscapeCommFunction(hMasterCOM, CLRDTR);
	sleep(100);

	do															/* flush input buffer */
	{
		ReadData(hMasterCOM,pInbyte,1,pFetched,100);
	}
	while (Fetched > 0);

	j=0;

	do
	{
		printf("Sending sync \n");
		WriteData(hMasterCOM,pOutbyte,1,pOutwrote);			/* send wrong byte, wait for ACK/NACK */
		ReadData(hMasterCOM,pInbyte,1,pFetched,1000);
		if (Fetched == 0) printf("No response \n");

		else
		{
			for (i=0; i<9; i++)
			{
				if (Inbyte == Response[i])
				{
					BSL = BSLType[i];
					MainStart = MainBeg[i];
					if (BSL==2) splitsize = 0x50;
					SplitStart = MainStart + splitsize;
					firmwarelen = 0xFFFF - MainStart;
				}
			}

			if (BSL == 0) printf("Sync acknowledged - INFO BSL, MAIN = %X, AppStart = %X \n", MainStart, MainStart);
			else if (BSL == 3) printf("Invalid response %X \n", Inbyte);
			else printf("Sync acknowledged - Split BSL, MAIN = %X, AppStart = %X \n", MainStart,SplitStart);

		}
		j = j + 1;
	}

	while ((BSL==3) && (j<3));

	if (BSL == 3) goto CloseExit;

	if (filearg == 0) goto CloseExit;

	/*Open firmware new firmware file for reading, process contents*/

	int linelen= 0;
	int linepos= 0;
	unsigned long currentAddr = MainStart;
	unsigned long netAddr = 0;
	unsigned char strdata[128];
	unsigned char xorsum = 0;
	unsigned long resetAdr = 0xE000;
	unsigned char temp = 0;
	unsigned char temp1 = 0;

	FILE *infile;						//open the file

	infile = fopen(argv[filearg], "rb");
	if (infile == NULL)
	{
	    printf("File Not Found.\n");
		Usage(argv[0]);
		goto CloseExit;
	}

	/* Convert data for MSP430, file is parsed line by line: */
	while (TRUE)
	{
		/* Read one line: */
		if ((fgets(strdata, 127, infile) == 0) || (strdata[0] == 'q'))
			/* if End Of File or if q (last character in file)	*/
		{
			fclose(infile);
			break;
		}

		if (strdata[0] == ':')                  /* is this a hex file */
		{									    /* yes - process the lines */
			sscanf(&strdata[1], "%02x", &linelen);  /*number of data bytes in line */
			if (linelen == 0)					/* if zero, it's the last line */
			{
				fclose(infile);
				break;
			}
			sscanf(&strdata[3], "%04x", &currentAddr);  /* the address for this line's data */
			if (currentAddr < MainStart)				/* must not be below F800 */
			{
		    	printf("File locates data below %X \n", MainStart);
				fclose(infile);
				goto CloseExit;
			}

			netAddr = currentAddr - MainStart;			/* calculate position in binary image */

			for (linepos= 9; linepos < 9+(linelen*2); linepos+= 2, netAddr++)
			{
				xorsum = xorsum ^ buf[netAddr];		/* take out existing byte */

				temp = strdata[linepos] - 0x30;		/* replace FF with new byte */
				if (temp > 9) temp -= 7;			/* hex to binary */
				temp1 = strdata[linepos+1] - 0x30;
				if (temp1 > 9) temp1 -= 7;
				buf[netAddr] = (temp << 4) + temp1;

				xorsum = xorsum ^ buf[netAddr];		/* add in replacement byte */
			}
			continue;								/* go back to *while* */
		}

		else										/* if a TI-TXT file */
		{
			linelen= strlen(strdata);				/* basically same process */

			if (strdata[0] == '@')
			{
				sscanf(&strdata[1], "%lx\n", &currentAddr);
				if (currentAddr < MainStart)
				{
			    	printf("File locates data below %X \n", MainStart);
					fclose(infile);
					goto CloseExit;
				}
				netAddr = currentAddr - MainStart;
				continue;
			}

			/* Transfer data in line into binary inage: */
			for (linepos= 0; linepos < linelen-3; linepos+= 3, netAddr++)
			{
				xorsum = xorsum ^ buf[netAddr];    /* take out existing byte */

				temp = strdata[linepos] - 0x30;
				if (temp > 9) temp -= 7;
				temp1 = strdata[linepos+1] - 0x30;
				if (temp1 > 9) temp1 -= 7;
				buf[netAddr] = (temp << 4) + temp1;

				xorsum = xorsum ^ buf[netAddr];    /* add in replacement byte */
			}
		}
	}			/* end of While */

	/* "break" goes here */

	/* now check to see if data makes sense */

	resetAdr = buf[firmwarelen-1] + (buf[firmwarelen] * 256);
	temp = 0xFF;

	if ((resetAdr != MainStart) && (BSL == 0))
	{
		printf("Reset vector %X must show program starts at %X for INFO BSL \n",
				resetAdr, MainStart);
		goto CloseExit;
	}

	if ((resetAdr != SplitStart) && (BSL > 0))
	{
		printf("Reset vector %X must show program starts at %X for Split BSL \n",
				resetAdr, SplitStart);
		goto CloseExit;
	}
	for (i=0; i<4; i++)
	{
		temp = temp & buf[resetAdr-MainStart + i];
	}

	if (temp == 0xFF)
	{
		printf("No code stored where reset vector points - %X \n", resetAdr);
		goto CloseExit;
	}

	if (resetAdr == SplitStart)
	{
		temp = 0xFF;
		for (i=0; i<splitsize; i++)
		{
			temp = temp & buf[i];                      /* should be no bytes below SplitStart */
		}

		if (temp != 0xFF)
		{
			printf("Code stored below where reset vector points - %X \n",resetAdr);
			goto CloseExit;
		}
	}

	xorsum = xorsum ^ buf[firmwarelen-1] ^ buf[firmwarelen];	/* Remove reset vector from checksum */
	buf[firmwarelen-1] = xorsum;								/* place checksum as last byte */
	filelen = firmwarelen;

	if (resetAdr == SplitStart)                                 /* if Split, move code to buf beginning */
	{
		filelen = filelen - splitsize;
		for (i=0, j=splitsize; i<(filelen+1); i++,j++)
		{
			buf[i] = buf[j];
		}
	}
/*
	FILE *fp;
	fp = fopen("file.bin" , "wb" );
	fwrite(buf, 1 , filelen, fp );
	fclose(fp);
*/

	Outbyte = cmdbyte;				/* now send correct byte */

	printf("Sending command byte \n");

	WriteData(hMasterCOM,pOutbyte,1,pOutwrote);

	sleep(2000);

	printf("Sending firmware data and checksum \n");

	WriteData(hMasterCOM,buf,filelen,pOutwrote);

	printf("%d bytes sent \n",Outwrote);
	if(filelen != Outwrote) printf("Should have sent %d \n",filelen);

	ReadData(hMasterCOM,pInbyte,1,pFetched,2000);

	if(Fetched == 0) printf("No response received \n");
	else if (Inbyte == ACK) printf("Update Successful \n");
	else if (Inbyte == NACK) printf("Flashing performed, but checksum did not match \n");
	else printf("Invalid response received: %d \n", Inbyte);

CloseExit:

	SetCommState(hMasterCOM, &dcbMasterInitState);

	sleep(60);

	CloseHandle(hMasterCOM);

	hMasterCOM = INVALID_HANDLE_VALUE;

	return 0;
}

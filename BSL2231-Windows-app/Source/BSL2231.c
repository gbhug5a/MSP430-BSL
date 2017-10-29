/*

BSL2231.exe COMn filename

This is software for the Windows console that flashes firmware to an MSP430G2231
microcontroller in which the matching custom BSL firmware code has been installed.
The command line inputs are the COM port of the USB-to-UART adapter and the filename
containing the new firmware to be flashed.  A sync protocol has been added to the
original TI custom BSL which establishes communications between the two ends, and
identifies which version of the custom BSL is installed in the G2231.

The "INFO" version of BSL is located entirely in INFO memory beginning at 0x1000 at
the bottom of INFOD, and continuing up through most of INFOA.  Firmware compatible
with this version will always begin at the start of MAIN memory at 0xF800.

The "Split" version is located in the first 80 bytes of MAIN memory from 0xF800 -
0xF84F, and in INFOA through INFOC.  INFOD is not used for BSL, and can be erased
and written to by the application.  Firmware compatible with the Split version will
always begin at 0xF850.

The firmware file may be in Intel-Hex or TI-TXT format. The program will perform
error checking to make sure the firmware complies with the BSL version in the chip.

The USB driver for the USB-to-UART adapter being used will also have to be installed.
This program pulses the DTR line low at the very beginning, which will reset the G2231
if DTR is connected to its /Reset pin.  A PDF file with further information accompanies
this program.

This program was written in C for the LCC-Win32 compiler.

*/

#include <stdbool.h>
#include <stdio.h>
#include <conio.h>

long MainStart = 0xF800;
long splitsize = 0x50;
long SplitStart = 0xF850;
int firmwarelen = 2047;
int comport = 0;
int filearg = 0;
int filelen = 0;
unsigned char cmdbyte = 186;
unsigned char ACK = 248;
unsigned char NACK = 254;

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
	printf("%s usage:\n",programName);
	printf("%s COMn filename \n",programName);
}

/*======== Process command line arguments. ==========*/

void HandleOptions(int argc,char *argv[])
{
	int i;

	for (i=1; i< argc;i++)
	{
		if (argv[i][0] == '/' || argv[i][0] == '-')
		{
			return;
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
	if (argc != 3)
	{
		/* If wrong number of arguments, call the Usage routine and exit */
		Usage(argv[0]);
		return 0;
	}

	/*handle the program options*/
	HandleOptions(argc,argv);
	if (comport == 0 || filearg == 0 )
	{
		Usage(argv[0]);
		return 0;
	}

	/*Open firmware new firmware file for reading, process contents*/

	int linelen= 0;
	int linepos= 0;
	int i;
	int j;
	unsigned long currentAddr = MainStart;
	unsigned long netAddr = 0;
	unsigned char strdata[128];
	unsigned char buf[firmwarelen+1];
	unsigned char xorsum = 0;
	unsigned long resetAdr = 0xF800;
	unsigned char temp = 0;
	unsigned char temp1 = 0;

	for (i = 0; i < firmwarelen+1; i++)   /* create binary MAIN image*/
    {									  /*  with all FFs*/
		buf[i] = 0xff;
	}

	FILE *infile;						//open the file

	infile = fopen(argv[filearg], "rb");
	if (infile == NULL)
	{
	    printf("File Not Found.\n");
		Usage(argv[0]);
		return 1;
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
				return 1;
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
					return 1;
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

	if ((resetAdr != MainStart) && (resetAdr != SplitStart))
	{
		printf("Reset vector %X must show program starts at %X, or %X for Split BSL \n",
				resetAdr, MainStart, SplitStart);
		return 1;
	}

	for (i=0; i<4; i++)
	{
		temp = temp & buf[resetAdr-MainStart + i];
	}

	if (temp == 0xFF)
	{
		printf("No code stored where reset vector points - %X \n",resetAdr);
		return 1;
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
			return 1;
		}
	}

	xorsum = xorsum ^ buf[firmwarelen-1] ^ buf[firmwarelen];	/* Remove reset vector from checksum */
	buf[firmwarelen-1] = xorsum;								/* place checksum as last byte */
	filelen = firmwarelen;

	if (resetAdr == SplitStart)
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
	unsigned char Outbyte = NACK;   /* command byte: firmware to follow */
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

	printf("Resetting G2231 via DTR, if connected \n");
	EscapeCommFunction(hMasterCOM, SETDTR);						/* toggle DTR (Reset) */
	EscapeCommFunction(hMasterCOM, CLRDTR);
	sleep(100);

	do															/* flush input buffer */
	{
		ReadData(hMasterCOM,pInbyte,1,pFetched,100);
	}
	while (Fetched > 0);

	do
	{
		printf("Sending sync \n");
		WriteData(hMasterCOM,pOutbyte,1,pOutwrote);			/* send wrong byte, wait for ACK/NACK */
		ReadData(hMasterCOM,pInbyte,1,pFetched,1000);
		if (Fetched == 0) printf("No response \n");
		else if (Inbyte == ACK) printf("Sync acknowledged - INFO BSL \n");
		else if (Inbyte == NACK) printf("Sync acknowledged - Split BSL \n");
		else printf("Invalid response \n");
		sleep(1000);
	}
	while ((Inbyte != ACK) && (Inbyte != NACK));

	if ((Inbyte == ACK) && (resetAdr == SplitStart))		/* ACK means INFO version installed */
	{
		printf("INFO BSL installed, but firmware file is for Split version \n");
	}

	else if ((Inbyte == NACK) && (resetAdr == MainStart))	/* NACK means Split version installed */
	{
		printf("Split BSL installed, but firmware file is for INFO version \n");
	}

	else
	{
		Outbyte = cmdbyte;				/* now send correct byte */

		printf("Sending command byte \n");

		WriteData(hMasterCOM,pOutbyte,1,pOutwrote);

		sleep(2000);

		printf("Sending firmware data \n");

		WriteData(hMasterCOM,buf,filelen,pOutwrote);

		printf("%d bytes sent \n",Outwrote);
		if(filelen != Outwrote) printf("Should have sent %d \n",filelen);

		ReadData(hMasterCOM,pInbyte,1,pFetched,2000);

		if(Fetched == 0) printf("No response received \n");
		else if (Inbyte == ACK) printf("Update Successful \n");
		else if (Inbyte == NACK) printf("Flashing performed, but checksum did not match \n");
		else printf("Invalid response received: %d \n", Inbyte);
	}

	SetCommState(hMasterCOM, &dcbMasterInitState);

	sleep(60);

	CloseHandle(hMasterCOM);

	hMasterCOM = INVALID_HANDLE_VALUE;

	return 0;
}

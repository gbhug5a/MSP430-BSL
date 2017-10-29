This project provides support for BSL flashing of Texas
Instruments MSP430G "Value Line" processors using an
embedded USB-to-Serial adapter. It includes firmware for
the MSP430 parts and the Windows flashing executable.  

Since this method eliminates the need for a Launchpad, the
user need only have a USB cable and the supporting software
to flash new firmware versions to the device.  The G2553 and
G2231 are specifically addressed, but methods described here
may be relevant to other MSP430 parts.

The project is described in detail in

   "MSP430G-BSL-Using-Embedded-Adapter.pdf"

which is the place to begin.  It includes protocols, software,
suggested parts, and circuit design options to be considered.

Additional software files include:

1. Alternate BSL entry code for the G2553 which avoids the
need for the special signalling pattern on Test and /Reset,
and which can allow BSL to run with LOCKA set.

2. A recompiled version of TI's BSLDEMO2.exe for the G2553,
which allows setting the DTR line to the correct polarity
when used with currently available USB-to-Serial adapters.

3. A VBS script that converts IntelHEX files to TI-TXT
format (16-bit only) as required for BSLDEMO2.

4. Two versions of a custom BSL for the G2231.  One
version resides only in INFO memory.  The other uses a
small amount of MAIN memory, and leaves INFOD available for
use by applications.

5. A new Windows console program that functions as the
master for the G2231 custom BSL process (the equivalent
of BSLDEMO.exe).

The installers for the two G2231 BSL versions also derive
by successive approximation the missing calibration values
for 8, 12 and 16 MHz and save those in the usual locations
at the top of INFOA.  This does not require a crystal.
Thanks to Steve Gibson for this idea and for the original
code, included in this project with permission.

All software includes both source code and executables.
Windows programs are compiled with the LCC-win32 C compiler.
All MSP430 code is assembly language written for Michael Kohn's
Naken Assembler (http://mikekohn.net).

Some of the code included here represents modifications of code
provided as "freeware" by Texas Instruments Incorporated, which
is Copyright (c) various dates by Texas Instruments Incorporated.
Files with such code include, or are accompanied by, notices and
other disclosures required by Texas Instruments with respect to
the conditions for use and distribution of such code, with or
without modification, and such requirements are not modified in
any way by the current author's grant of the MIT license as set
forth in the "LICENSE" file.


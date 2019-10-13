
NOTE:  Texas Instruments has now modified BSLDEMO to support the use of
generic USB-to-Serial adapters.  The new version is found in the DEPRECATED
folder of the BSL-Scripter installer, v3.4.0.2, with file dates of 9/26/19.
A new "-t" (for Tool) option has been added, with "-tUSB" used for generic
adapters. Since official signed TI software can now be used, the revised
version of BSLDEMO included in this repo is no longer needed.

NOTE:  In August, 2018, this project was reworked to make everything work
with a wider range of MSP430G parts.  So what was specifically for the
G2553 now works for G2xx3 parts.  And G2231 is now G2xx1 and Gxx2 parts,
referred to here collectively as "G2xx12".


## MSP430G BSL Using an Embedded USB-to-Serial Adapter


This project provides support for BSL flashing of Texas Instruments MSP430G
"Value Line" processors using an embedded USB-to-Serial adapter. It
includes firmware for the MSP430 parts and a Windows flashing executable.

Since this method eliminates the need for a Launchpad, the user need only
have a USB cable and the supporting software to flash new firmware versions
to the device.  The G2xx1, G2xx2 and G2xx3 parts are specifically addressed,
but methods described here may be relevant to other MSP430 parts.

The project is described in detail in

   "MSP430G-BSL-Using-Embedded-Adapter.pdf"

which is the place to begin.  It includes protocols, software, suggested
parts, and circuit design options to be considered.

Additional software files include:

1. Alternate BSL entry code for G2xx3 parts which avoids the need for the
special signalling pattern on Test and /Reset, and which can be configured
to allow BSL to run with LOCKA set, so firmware can be flashed to MAIN
without knowing the password.

2. A recompiled version of TI's BSLDEMO2.exe with a new option to set the
DTR line to the correct polarity when used with currently available USB-to-
Serial adapters, such as the FT232RL, CP2102, and CH340G. [No longer needed -
see NOTE above.]

3. A VBS script for Windows that converts IntelHEX files to TI-TXT format (
16-bit only) as required for BSLDEMO2.

4. Two versions of a custom BSL for the G2xx12 parts (shorthand for G2xx1
and G2xx2).  One version resides only in INFO memory.  The other uses a
small amount of MAIN memory, and leaves INFOD available for use by
applications.

5. A new Windows console program that functions as the master for the
G2xx12 custom BSL process (the equivalent of BSLDEMO.exe).

6. An examnple of using an intermediate jump table for the G2553 interrupt
vectors, which allows the BSL password to remain unchanged for all firmware
versions of a project.

The installers for the two G2xx12 BSL versions also derive by successive
approximation any missing calibration values for 8, 12 and 16 MHz, based on
the existing 1 MHz factory calibration, and save those in the usual
locations near the top of INFOA.  This does not require a crystal. Thanks
to Steve Gibson for this idea and for the original code on which this code
is based, included here with permission.

All software includes both source code and executables. Windows programs
are compiled with the LCC-win32 C compiler. All MSP430 code is assembly
language written for Michael Kohn's Naken Assembler (http://mikekohn.net).

Some of the code included here represents modifications of code provided as
"freeware" by Texas Instruments Incorporated, which is Copyright (c)
various dates by Texas Instruments Incorporated. Files with such code
include, or are accompanied by, notices and other disclosures required by
Texas Instruments with respect to the conditions for use and distribution
of such code, with or without modification, and such requirements are not
modified in any way by the current author's grant of the MIT license with
respect to his own work as set forth in the "LICENSE" file.

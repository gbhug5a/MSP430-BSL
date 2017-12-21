This folder contains an example in assembler showing the
interrupt vectors beginning at 0xFFE0 pointing to an
intermediate jump table containing BRanch instructions to
the various service routines.  The jump table is used to
simplify password management for G2553 BSL by keeping the
same password for all versions of the firmware.

The interrupt vectors at 0xFFE0 - 0xFFFF constitute the
password for BSL commands.  Those vectors now point to
intermedate jump table BRanch instructions to the actual
service routines.  So long as the location and structure of
that table remain unchanged, the original interrupt
vectors, and hence the password, will remain unchanged.
But the addresses BRanched to can change from version to
version.

Included here are:

example-asm.m43 - The assembly source code for the example,
                  for the Naken assembler

example.hex - The Intel-hex file produced by the assembler

example-TI-TXT.txt - The TI-TXT version of the hex file

example-pwd.txt - The TI-TXT password file resulting from
                  the example

example-disasm.txt - Disassembly of the hex file showing
                     where everything ends up


The code also writes a null word at 0xFFDE, which will
prevent BSL from performing a mass erase if a password
provided to it is wrong.

The example shows setting up table entries for some, but
not all, of the 16 possible interrupt vectors.  It is only
necessary to set up entries for those that are actually
used.  But if there is any chance that a particular
interrupt will ever be used on the current project,
providing for it in the beginning will allow the password
to remain unchanged if it is ultimately used.  Four bytes
per table BRanch instruction are needed.

Doing this in C is left as an exercise for the reader.

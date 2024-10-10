@echo off
copy "PinkEye_Shellcode.vmp.dll" "shellcode.bin"
AESEncrypt_PinkEye.exe
pause
exit
@echo off
echo Build script for Windows
echo.

echo Assembling bootloader...
cd source\bootload
nasm -O0 -f bin -o bootload.bin bootload.asm
cd ..

echo Assembling MikeOS kernel...
nasm -O0 -f bin -o kernel.bin kernel.asm
cd ..

echo Adding bootsector to disk image...
cd disk_images
dd count=2 seek=0 bs=512 if=..\source\bootload\bootload.bin of=.\mikeos.flp
cd ..

echo Mounting disk image...
imdisk -a -f disk_images\mikeos.flp -s 1440K -m B:

echo Copying kernel and applications to disk image...
copy source\kernel.bin b:\
copy programs\*.bas b:\

echo Dismounting disk image...
imdisk -D -m B:

echo Done!

#!/bin/sh
#
#Builds the bootstrap kernel, whose goal is to load the actual kernel
#and displays its starting block and block size on the floppy.
#
#Copyright (C) 2010  Hadrien Grasland
#
#    This program is free software; you can redistribute it and/or modify
#    it under the terms of the GNU General Public License as published by
#    the Free Software Foundation; either version 2 of the License, or
#    (at your option) any later version.
#
#    This program is distributed in the hope that it will be useful,
#    but WITHOUT ANY WARRANTY; without even the implied warranty of
#    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#    GNU General Public License for more details.
#
#    You should have received a copy of the GNU General Public License
#    along with this program; if not, write to the Free Software
#    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA

#Clean
rm -f *~
rm -f arch/x86_64/bootstrap/*~ arch/x86_64/debug/*~ arch/x86_64/include/*~ arch/x86_64/bootstrap/lib/*~ arch/x86_64/bootstrap/include/*~
rm -f include/*~ init/*~ lib/*~ support/*~
rm -f bin/bootstrap/*.o bin/kernel/*.o
rm -f bin/bootstrap/*.bin bin/kernel/*.bin
rm -f floppy.img

#Compile
AS32=i686-elf-as
AS=x86_64-elf-as
CC32=i686-elf-gcc
CXX=x86_64-elf-g++
LD32=i686-elf-ld
LD=x86_64-elf-ld
CFLAGS="-Wall -Wextra -Werror -nostdlib -nostartfiles -nodefaultlibs -fno-builtin -std=c99 -pedantic -ffreestanding"
CXXFLAGS="-Wall -Wextra -Werror -nostdlib -nostartfiles -nodefaultlibs -fno-builtin -fno-exceptions -fno-rtti -fstack-protector-all \
-mno-red-zone -mno-mmx -mno-sse -mno-sse2 -mno-sse3 -mno-3dnow"
LFLAGS="-z max-page-size=0x1000" #Maximum page size usable by the kernel (4 KB at the moment). Add -s for reduced size when out of debugging
INCLUDES="-I../../arch/x86_64/debug/ -I../../arch/x86_64/include/ -I../../include/ -I../../arch/x86_64/bootstrap/include"
echo \* Making bootstrap kernel...
cd bin/bootstrap
$AS32 ../../arch/x86_64/bootstrap/bs_multiboot.s -o bs_multiboot.o
$AS32 ../../arch/x86_64/bootstrap/enable_longmode.s -o enable_longmode.o
$CC32 -c ../../arch/x86_64/bootstrap/*.c $CFLAGS $INCLUDES
$CC32 -c ../../arch/x86_64/debug/*.c $CFLAGS $INCLUDES
$CC32 -c ../../arch/x86_64/bootstrap/lib/*.c $CFLAGS $INCLUDES
$LD32 -T ../../support/bs_linker.lds -o bs_kernel.bin *.o $LFLAGS
cd ../..

echo \* Making main kernel...
cd bin/kernel
$AS ../../arch/x86_64/init/init_kernel.s -o init_kernel.o
$CXX -c ../../init/kernel.cpp $CXXFLAGS $INCLUDES
$CXX -c ../../lib/*.cpp $CXXFLAGS $INCLUDES
$LD -T ../../support/kernel_linker.lds -o kernel.bin *.o $LFLAGS
cd ../..

echo

#Create floppy image
echo \* Creating floppy image...
#Create empty image file
dd if=/dev/zero of=floppy.img bs=1k count=1440
#Format it to FAT, copy grub files, install grub
mformat :: -f 1440 -i floppy.img
#Copy file system files
mmd ::/system -i floppy.img
mcopy bin/bootstrap/bs_kernel.bin ::/system -i floppy.img
mcopy bin/kernel/kernel.bin ::/system -i floppy.img
#Install GRUB on floppy
mmd ::/boot -i floppy.img
mmd ::/boot/grub -i floppy.img
mcopy support/stage1 ::/boot/grub -i floppy.img
mcopy support/stage2 ::/boot/grub -i floppy.img
mcopy support/menu.lst ::/boot/grub -i floppy.img
grub --batch < support/grub.batch
echo
echo \*\*\*REMEMBER TO RUN svn commit -m \"What happened\" ONCE SOMETHING NEW IS ON THE WAY !\*\*\*

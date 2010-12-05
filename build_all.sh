#!/bin/sh
#
#Builds the bootstrap kernel, the actual kernel, and makes a floppy image.
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

#Flags
Fdebug=1 #Enable debug mode (access to various debugging features, like the debug stream)
Fbuild_bs=0 #Build the bootstrap code
Fbuild_knl=1 #Build the micro-kernel

#Cleanup
echo "* Cleaning up..."
sh cleanup_temp.sh
if [ $Fbuild_bs -eq 1 ]
then
  rm -f bin/bootstrap/*.o bin/bootstrap/*.bin bin/bootstrap/*.gz
fi
if [ $Fbuild_knl -eq 1 ]
then
  rm -f bin/kernel/*.o bin/kernel/*.bin
fi
rm -f floppy.img

#Compilation parameters definition
AS32=i686-elf-as
AS=x86_64-elf-as
CC32=i686-elf-gcc
CXX=x86_64-elf-g++
LD32=i686-elf-ld
LD=x86_64-elf-ld
CFLAGS="-Wall -Wextra -Werror -nostdlib -nostartfiles -nodefaultlibs -fno-builtin -std=c99 -ffreestanding"
CXXFLAGS="-Wall -Wextra -nostdlib -nostartfiles -nodefaultlibs -fno-builtin -fno-exceptions -fno-rtti -fno-stack-protector \
-mno-red-zone -mno-mmx -mno-sse -mno-sse2 -mno-sse3 -mno-3dnow -std=c++98"
LFLAGS="-s --warn-common --warn-once -zmax-page-size=0x1000" #Maximum page size usable by the kernel (4 KB at the moment).
INCLUDES="-I../../arch/x86_64/include/ -I../../include/"
INCLUDES_BST="-I../../arch/x86_64/bootstrap/include "$INCLUDES
#Modification of parameters depending on debugging status
if [ $Fdebug -eq 0 ]
then
  CFLAGS=$CFLAGS" -O3"
  CXXFLAGS=$CXXFLAGS" -O3"
else
  INCLUDES="-I../../arch/x86_64/debug/ -I../../debug/ "$INCLUDES
  INCLUDES_BST="-I../../arch/x86_64/bootstrap/debug/ "$INCLUDES_BST
  CFLAGS=$CFLAGS" -O0 -DDEBUG"
  CXXFLAGS=$CXXFLAGS" -O0 -DDEBUG"
fi

#Bootstrap code compilation
if [ $Fbuild_bs -eq 1 ]
then
  echo \* Building bootstrap code...
  cd bin/bootstrap
  $AS32 ../../arch/x86_64/bootstrap/bs_multiboot.s -o bs_multiboot.o
  $CC32 -c ../../arch/x86_64/bootstrap/*.c $CFLAGS $INCLUDES_BST
  $AS32 ../../arch/x86_64/bootstrap/lib/cpuid_check.s -o cpuid_check.o
  $AS32 ../../arch/x86_64/bootstrap/lib/enable_longmode.s -o enable_longmode.o
  $AS32 ../../arch/x86_64/bootstrap/lib/run_kernel.s -o run_kernel.o
  $CC32 -c ../../arch/x86_64/bootstrap/lib/*.c $CFLAGS $INCLUDES_BST
  #Compiling debugging source files
  if [ $Fdebug -eq 1 ]
  then
    $CC32 -c ../../arch/x86_64/bootstrap/debug/*.c $CFLAGS $INCLUDES_BST
  fi
  #Linking
  $LD32 -T ../../support/bs_linker.lds -o bs_kernel.bin *.o $LFLAGS
  #Compression (reduces loading times)
  gzip bs_kernel.bin
  cd ../..
fi

#Kernel compilation
if [ $Fbuild_knl -eq 1 ]
then
  echo \* Building kernel...
  cd bin/kernel
  $AS ../../arch/x86_64/init/init_kernel.s -o init_kernel.o
  $CXX -c ../../arch/x86_64/interrupts/*.cpp $CXXFLAGS $INCLUDES
  $CXX -c ../../arch/x86_64/memory/*.cpp $CXXFLAGS $INCLUDES
  $CXX -c ../../arch/x86_64/synchronization/*.cpp $CXXFLAGS $INCLUDES
  $CXX -c ../../init/*.cpp $CXXFLAGS $INCLUDES
  $CXX -c ../../memory/*.cpp $CXXFLAGS $INCLUDES
  $CXX -c ../../process/*.cpp $CXXFLAGS $INCLUDES
  if [ $Fdebug -eq 1 ]
  then
    $CXX -c ../../arch/x86_64/debug/*.cpp $CXXFLAGS $INCLUDES
  fi
  $LD -T ../../support/kernel_linker.lds -o kernel.bin *.o $LFLAGS
  cd ../..
fi

#Create a system floppy image
echo \* Creating floppy image...
#Create a copy of the floppy image where GRUB is installed
cp support/grub_floppy.img floppy.img
#Copy system files
export MTOOLSRC=support/mtoolsrc.txt
mcopy bin/bootstrap/bs_kernel.bin.gz K:/system
mcopy bin/kernel/kernel.bin K:/system
mcopy support/menu.lst K:/boot/grub

echo
echo "***REMEMBER TO RUN svn update, status, and commit FREQUENTLY !***"

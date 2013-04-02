BUILDING REQUIREMENTS :
-> A POSIX-compatible system with a bash-compatible shell
-> Make
* For x86_64 target
-> i686 and x86_64 cross-compiling GCC >= 4.5.0 and binutils
-> genisoimage
-> Grub2

BUILDING PROCEDURE
We now use makefiles. Available make commands are :
-make (= make all) : Build everything
-make run : Build everything and run it in an emulator
-make clean : Clean up intermediary build products
-make mrproper : Clean up everything including final products

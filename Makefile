#Feature enable/disable flags : debug mode, kernel test suite
Fdebug = 1
Ftests = 1

#Target architecture and arch-specific data go here
ARCH = x86_64
BS_ARCH = i686
CXX_ARCH = -mcmodel=small -mno-red-zone -mno-mmx -mno-sse -mno-sse2 -mno-sse3 -mno-3dnow
L_ARCH = -zmax-page-size=0x1000
GENISO_PARAMS = -R -b boot/grub/stage2_eltorito -no-emul-boot -boot-load-size 4 -boot-info-table
GENISO_PARAMS += -quiet -A "The OS-periment"

#Source files go here
BS_ASM_SRC = $(wildcard arch/$(ARCH)/bootstrap/*.s arch/$(ARCH)/bootstrap/lib/*.s)
BS_C_SRC = $(wildcard arch/$(ARCH)/bootstrap/*.c arch/$(ARCH)/bootstrap/lib/*.c)
KNL_ASM_SRC=$(wildcard arch/$(ARCH)/init/*.s)
KNL_CPP_SRC = $(wildcard arch/$(ARCH)/interrupts/*.cpp arch/$(ARCH)/memory/*.cpp)
KNL_CPP_SRC += $(wildcard arch/$(ARCH)/synchronization/*.cpp init/*.cpp memory/*.cpp)
KNL_CPP_SRC += $(wildcard process/*.cpp lib/*.cpp)
ifeq ($(Fdebug),1)
    BS_C_SRC += $(wildcard arch/$(ARCH)/bootstrap/opt/debug/*.c)
    KNL_CPP_SRC += $(wildcard arch/$(ARCH)/opt/debug/*.cpp opt/debug/*.cpp)
endif
ifeq ($(Ftests),1)
    KNL_CPP_SRC += $(wildcard opt/tests/common/*.cpp arch/$(ARCH)/opt/tests/common/*.cpp)
    KNL_CPP_SRC += $(wildcard opt/tests/memory/*.cpp arch/$(ARCH)/opt/tests/memory/*.cpp)
    KNL_CPP_SRC += $(wildcard opt/tests/rpcbench/*.cpp)
endif

#Headers go there (Yeah, duplication sucks. If you know how to avoid it...)
HEADERS=$(wildcard arch/$(ARCH)/include/*.h include/*.h)
BS_HEADERS=$(wildcard arch/$(ARCH)/bootstrap/include $(HEADERS))
INCLUDES=-Iarch/$(ARCH)/include/ -Iinclude/
BS_INCLUDES=-Iarch/$(ARCH)/bootstrap/include $(INCLUDES)
ifeq ($(Fdebug),1)
    HEADERS += $(wildcard arch/$(ARCH)/opt/debug/*.h opt/debug/*.h)
    BS_HEADERS += $(wildcard arch/$(ARCH)/bootstrap/opt/debug/*.h)
    INCLUDES += -Iarch/$(ARCH)/opt/debug/ -Iopt/debug/
    BS_INCLUDES += -Iarch/$(ARCH)/bootstrap/opt/debug/
endif
ifeq ($(Ftests),1)
    HEADERS += $(wildcard arch/$(ARCH)/opt/tests/include/*.h opt/tests/include/*.h)
    INCLUDES += -Iarch/$(ARCH)/opt/tests/include -Iopt/tests/include
endif

#Compilation parameters for the bootstrap part
AS32=$(BS_ARCH)-elf-as
CC32=$(BS_ARCH)-elf-gcc
LD32=$(BS_ARCH)-elf-ld
C_WARNINGS=-Wall -Wextra -Werror
C_LIBS=-nostdlib -nostartfiles -nodefaultlibs -fno-builtin -ffreestanding
C_STD=-std=c99
CFLAGS=$(C_WARNINGS) $(C_LIBS) $(C_STD)
ifeq ($(Fdebug),1)
    CFLAGS += -O0 -DDEBUG
else
    CFLAGS += -O2
endif

#Compilation parameters for the kernel
AS=$(ARCH)-elf-as
CXX=$(ARCH)-elf-g++
LD=$(ARCH)-elf-ld
CXX_WARNINGS=-Wall -Wextra
CXX_LIBS=-nostdlib -nostartfiles -nodefaultlibs -fno-builtin
CXX_FEATURES=-fno-exceptions -fno-rtti -fno-stack-protector -fno-threadsafe-statics
CXX_STD=-std=c++98
CXXFLAGS=$(CXX_WARNINGS) $(CXX_LIBS) $(CXX_FEATURES) $(CXX_ARCH) $(CXX_STD)
ifeq ($(Fdebug),1)
    CXXFLAGS += -O0 -DDEBUG
else
    CXXFLAGS += -O3
endif

#Linking flags
LFLAGS = -s --warn-common --warn-once $(L_ARCH)

#Definition of targets
CDIMAGE = cdimage.iso
BS_BIN = bin/bs_kernel.bin
BS_GZ = $(BS_BIN).gz
KNL_BIN = bin/kernel.bin
BS_ASM_OBJ = $(BS_ASM_SRC:.s=.bsasm.o)
BS_C_OBJ = $(BS_C_SRC:.c=.bsc.o)
KNL_ASM_OBJ = $(KNL_ASM_SRC:.s=.knlasm.o)
KNL_CPP_OBJ = $(KNL_CPP_SRC:.cpp=.knlcpp.o)

#Make rules
all: cdimage
	@echo "Reminder : remember to run svn update, status, and commit frequently !"

run: all
	@echo c > comm_test
	@nice -n 7 bochsdbg -qf support/bochsrc.txt -rc comm_test
	@rm -f comm_test

cdimage: $(CDIMAGE)

$(CDIMAGE): $(BS_GZ) $(KNL_BIN)
	@rm -rf $(CDIMAGE)
	@rm -rf bin/cdimage/*
	@mkdir bin/cdimage/boot
	@mkdir bin/cdimage/boot/grub
	@cp support/stage2_eltorito bin/cdimage/boot/grub
	@cp $(BS_GZ) bin/cdimage/boot
	@cp $(KNL_BIN) bin/cdimage/boot
	@cp support/menu.lst bin/cdimage/boot/grub
	@genisoimage $(GENISO_PARAMS) -o $(CDIMAGE) bin/cdimage

bootstrap: $(BS_GZ)

kernel: $(KNL_BIN)

$(BS_BIN): $(BS_ASM_OBJ) $(BS_C_OBJ) $(BS_HEADERS)
	@$(LD32) -T support/bs_linker.lds -o $@ $(BS_ASM_OBJ) $(BS_C_OBJ) $(LFLAGS)

$(KNL_BIN): $(KNL_ASM_OBJ) $(KNL_CPP_OBJ) $(HEADERS)
	@$(LD) -T support/kernel_linker.lds -o $@ $(KNL_ASM_OBJ) $(KNL_CPP_OBJ) $(LFLAGS)

%.bsasm.o: %.s
	@$(AS32) $< -o $@

%.bsc.o: %.c $(BS_HEADERS)
	@$(CC32) -o $@ -c $< $(CFLAGS) $(BS_INCLUDES)

%.knlasm.o: %.s
	@$(AS) $< -o $@

%.knlcpp.o: %.cpp $(HEADERS)
	@$(CXX) -o $@ -c $< $(CXXFLAGS) $(INCLUDES)

%.bin.gz: %.bin
	@gzip -c $< > $@

clean:
	@rm -f $(BS_GZ) $(BS_BIN) $(BS_ASM_OBJ) $(BS_C_OBJ) $(KNL_BIN) $(KNL_ASM_OBJ) $(KNL_CPP_OBJ)
	@rm -rf cdimage/*

mrproper: clean
	@rm -f $(CDIMAGE)


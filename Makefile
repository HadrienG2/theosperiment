#Feature enable/disable flags : debug mode, kernel test suite
Fdebug = 1
Ftests = 1

#Target architecture and arch-specific data go here
ARCH = x86_64
BS_ARCH = i686
CXX_ARCH = -mcmodel=small -mno-red-zone -mno-mmx -mno-sse3 -mno-3dnow
L_ARCH = -zmax-page-size=0x1000
GENISO_PARAMS = -r -no-emul-boot -boot-info-table -quiet -A "The OS|periment" --boot-load-size 4
GRUB2_LOCAL_PREFIX = /usr/lib/grub2

#Source files go here
BS_ASM_SRC = $(wildcard arch/$(ARCH)/bootstrap/*.s arch/$(ARCH)/bootstrap/lib/*.s)
BS_C_SRC = $(wildcard arch/$(ARCH)/bootstrap/*.c arch/$(ARCH)/bootstrap/lib/*.c)
KNL_ASM_SRC=$(wildcard arch/$(ARCH)/init/*.s)
KNL_CPP_SRC = $(wildcard arch/$(ARCH)/memory/*.cpp arch/$(ARCH)/synchronization/*.cpp)
KNL_CPP_SRC += $(wildcard init/*.cpp memory/*.cpp process/*.cpp lib/*.cpp)
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
C_LIBS=-nostdlib -ffreestanding
C_STD=-std=c99
CFLAGS=$(C_WARNINGS) $(C_LIBS) $(C_STD)
ifeq ($(Fdebug),1)
    CFLAGS += -O0 -DDEBUG
else
    CFLAGS += -O3
endif

#Compilation parameters for the kernel
AS=$(ARCH)-elf-as
CXX=$(ARCH)-elf-g++
LD=$(ARCH)-elf-ld
CXX_WARNINGS=-Wall -Wextra
CXX_LIBS=-nostdlib -ffreestanding
CXX_FEATURES=-fno-exceptions -fno-rtti -fno-stack-protector -fno-threadsafe-statics
CXX_STD=-std=c++0x
CXXFLAGS=$(CXX_WARNINGS) $(CXX_LIBS) $(CXX_FEATURES) $(CXX_ARCH) $(CXX_STD)
ifeq ($(Fdebug),1)
    CXXFLAGS += -O0 -DDEBUG
else
    CXXFLAGS += -O3
endif

#Linking flags
LFLAGS = -s --warn-common --warn-once $(L_ARCH)

#Abstracting away filenames
CDIMAGE = cdimage.iso
CDIMAGE_ROOT = bin/cdimage
BS_BIN = bin/bs-kernel.bin
KNL_BIN = bin/kernel.bin
BS_ASM_OBJ = $(BS_ASM_SRC:.s=.bsasm.o)
BS_C_OBJ = $(BS_C_SRC:.c=.bsc.o)
KNL_ASM_OBJ = $(KNL_ASM_SRC:.s=.knlasm.o)
KNL_CPP_OBJ = $(KNL_CPP_SRC:.cpp=.knlcpp.o)
TMP_FILES = $(BS_ASM_SRC:.s=.s~) $(BS_C_SRC:.c=.c~) $(KNL_ASM_SRC:.s=.s~) $(KNL_CPP_SRC:.cpp=.cpp~)
TMP_FILES += Makefile~

#GRUB image generation parameters
GRUB2_DEST_PREFIX = System/boot/grub2
GRUB2_CORE_IMG = bin/grub2-core.img
GRUB2_ELTORITO_IMG = bin/grub2-eltorito.img
GRUB2_STATIC_MODULES = biosdisk iso9660 configfile

#Make rules
all: cdimage Makefile
	@echo "Reminder : remember to run git pull, git status, and git commit -a && git push frequently !"

run: all Makefile
	@echo c > comm_test
	@nice -n 7 bochsdbg -qf support/bochsrc.txt -rc comm_test
	@rm -f comm_test

cdimage: $(CDIMAGE) Makefile

bootstrap: $(BS_BIN) Makefile

kernel: $(KNL_BIN) Makefile

clean: Makefile
	@rm -f $(BS_BIN) $(BS_ASM_OBJ) $(BS_C_OBJ) $(KNL_BIN) $(KNL_ASM_OBJ) $(KNL_CPP_OBJ)
	@rm -rf $(GRUB2_ELTORITO_IMG) $(GRUB2_CORE_IMG) $(CDIMAGE_ROOT)/* $(TMP_FILES)

mrproper: clean Makefile
	@rm -f $(CDIMAGE)

$(CDIMAGE): $(BS_BIN) $(KNL_BIN) $(GRUB2_ELTORITO_IMG) Makefile
	@rm -rf $(CDIMAGE)
	@rm -rf $(CDIMAGE_ROOT)/*
	@mkdir -p $(CDIMAGE_ROOT)/$(GRUB2_DEST_PREFIX)/i386-pc
	@cp $(GRUB2_LOCAL_PREFIX)/i386-pc/* $(CDIMAGE_ROOT)/$(GRUB2_DEST_PREFIX)/i386-pc
	@mkdir -p $(CDIMAGE_ROOT)/$(GRUB2_DEST_PREFIX)/locale
	@cp support/grub2/grub2.cfg $(CDIMAGE_ROOT)/$(GRUB2_DEST_PREFIX)
	@cp $(GRUB2_ELTORITO_IMG) $(CDIMAGE_ROOT)/$(GRUB2_DEST_PREFIX)/grub2_eltorito.img
	@cp $(BS_BIN) bin/cdimage/System/boot
	@cp $(KNL_BIN) bin/cdimage/System/boot
	@genisoimage -o $(CDIMAGE) -b $(GRUB2_DEST_PREFIX)/grub2_eltorito.img $(GENISO_PARAMS) bin/cdimage

$(GRUB2_ELTORITO_IMG): $(GRUB2_CORE_IMG) Makefile
	@cat $(GRUB2_LOCAL_PREFIX)/i386-pc/cdboot.img $(GRUB2_CORE_IMG) > $(GRUB2_ELTORITO_IMG)

$(GRUB2_CORE_IMG): support/grub2/grub2.cfg Makefile
	@echo "configfile /System/boot/grub2/grub2.cfg" > grub2_tmp.cfg
	@grub2-mkimage -p '/$(GRUB2_DEST_PREFIX)/i386-pc' -c grub2_tmp.cfg -o $(GRUB2_CORE_IMG) -O i386-pc $(GRUB2_STATIC_MODULES)
	@rm -r grub2_tmp.cfg

$(BS_BIN): $(BS_ASM_OBJ) $(BS_C_OBJ) $(BS_HEADERS) Makefile
	@$(LD32) -T support/bs_linker.lds -o $@ $(BS_ASM_OBJ) $(BS_C_OBJ) $(LFLAGS)

$(KNL_BIN): $(KNL_ASM_OBJ) $(KNL_CPP_OBJ) $(HEADERS) Makefile
	@$(LD) -T support/kernel_linker.lds -o $@ $(KNL_ASM_OBJ) $(KNL_CPP_OBJ) $(LFLAGS)

%.bsasm.o: %.s Makefile
	@$(AS32) $< -o $@

%.bsc.o: %.c $(BS_HEADERS) Makefile
	@$(CC32) -o $@ -c $< $(CFLAGS) $(BS_INCLUDES)

%.knlasm.o: %.s Makefile
	@$(AS) $< -o $@

%.knlcpp.o: %.cpp $(HEADERS) Makefile
	@$(CXX) -o $@ -c $< $(CXXFLAGS) $(INCLUDES)

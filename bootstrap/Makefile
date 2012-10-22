#The bootstrap component for an x86_64 kernel is built for the i686 architecture
BS_ARCH = $(ARCH:x86_64=i686)

#Source files go here
BS_ASM_SRC = $(wildcard bootstrap/arch/$(BS_ARCH)/*.s bootstrap/arch/$(BS_ARCH)/lib/*.s)
BS_C_SRC = $(wildcard bootstrap/arch/$(BS_ARCH)/*.c bootstrap/arch/$(BS_ARCH)/lib/*.c)
ifeq ($(Fdebug),1)
    BS_C_SRC += $(wildcard bootstrap/arch/$(BS_ARCH)/opt/debug/*.c)
endif

#Headers go there (Yeah, duplication sucks. If you know how to avoid it...)
BS_HEADERS=$(wildcard bootstrap/arch/$(BS_ARCH)/include)
BS_INCLUDES=-Ibootstrap/arch/$(BS_ARCH)/include
ifeq ($(Fdebug),1)
    BS_HEADERS += $(wildcard bootstrap/arch/$(BS_ARCH)/opt/debug/*.h)
    BS_INCLUDES += -Ibootstrap/arch/$(BS_ARCH)/opt/debug/
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

#Abstracting away filenames
BS_ASM_OBJ = $(BS_ASM_SRC:.s=.bsasm.o)
BS_C_OBJ = $(BS_C_SRC:.c=.bsc.o)
TMP_FILES += $(BS_ASM_SRC:.s=.s~) $(BS_C_SRC:.c=.c~) bootstrap/Makefile~
BIN_OBJECTS += $(BS_ASM_OBJ) $(BS_C_OBJ)

#Make rules
bootstrap: $(BS_BIN) Makefile bootstrap/Makefile

$(BS_BIN): $(BS_ASM_OBJ) $(BS_C_OBJ) $(BS_HEADERS) Makefile bootstrap/Makefile
	@$(LD32) -T bootstrap/arch/$(BS_ARCH)/support/bs_linker.lds -o $@ $(BS_ASM_OBJ) $(BS_C_OBJ) $(LFLAGS)

%.bsasm.o: %.s Makefile bootstrap/Makefile
	@$(AS32) $< -o $@

%.bsc.o: %.c $(BS_HEADERS) Makefile bootstrap/Makefile
	@$(CC32) -o $@ -c $< $(CFLAGS) $(BS_INCLUDES)

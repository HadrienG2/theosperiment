#An important peculiarity of x86_64 is that bootstrap code must be built for the i686 architecture.
#This bit cannot be put in the arch-specific makefile since it is used to locate it.
BS_ARCH:= $(ARCH:x86_64=i686)

#Source files go here
BS_ASM_SRC:= $(shell find bootstrap/ -path '*/opt/*' -prune -o -name '*.s' -print)
BS_C_SRC:= $(shell find bootstrap/ -path '*/opt/*' -prune -o -name '*.c' -print)
ifeq ($(Fdebug),1)
	BS_ASM_SRC+= $(shell find bootstrap/ -path '*/opt/debug/*' -name '*.s' -print)
    BS_C_SRC+= $(shell find bootstrap/ -path '*/opt/debug/*' -name '*.c' -print)
endif

#Exported headers go there
BS_INCLUDES:= $(shell find bootstrap/ -path '*/opt/*' -prune -o -path '*/include' -printf '-I%p ')
ifeq ($(Fdebug),1)
    BS_INCLUDES+= $(shell find bootstrap/ -path '*/opt/debug/*' -path '*/include' -printf '-I%p ')
endif

#Compilation parameters for the bootstrap part
BS_AS:= $(BS_ARCH)-elf-as
BS_CC:= $(BS_ARCH)-elf-gcc
BS_LD:= $(BS_ARCH)-elf-ld
BS_C_WARNINGS:= -Wall -Wextra -Werror
BS_C_LIBS:= -nostdlib -ffreestanding
BS_C_STD:= -std=c11
BS_CFLAGS:= $(BS_C_WARNINGS) $(BS_C_LIBS) $(BS_C_STD)
ifeq ($(Fdebug),1)
    BS_CFLAGS+= -O0 -DDEBUG
else
    BS_CFLAGS+= -O0
endif

#Abstracting away filenames
BS_MAKEFILES:= bootstrap/Makefile
BS_ASM_OBJ:= $(BS_ASM_SRC:.s=.bsasm.o)
BS_C_OBJ:= $(BS_C_SRC:.c=.bsc.o)
BS_DEPEND_FILES:= $(BS_C_OBJ:.o=.d)
BIN_OBJECTS+= $(BS_ASM_OBJ) $(BS_C_OBJ) $(BS_DEPEND_FILES)

#Arch-specific definitions belong to a separate makefile.
include bootstrap/arch/$(BS_ARCH)/Makefile

#Make rules
$(BS_BIN): $(BS_ASM_OBJ) $(BS_C_OBJ) $(MAKEFILES) $(BS_MAKEFILES)
	@$(BS_LD) -T bootstrap/arch/$(BS_ARCH)/support/bs_linker.lds -o $@ $(BS_ASM_OBJ) $(BS_C_OBJ) $(LFLAGS)

%.bsasm.o: %.s $(MAKEFILES) $(BS_MAKEFILES)
	@$(BS_AS) $< -o $@

%.bsc.o: %.c $(MAKEFILES) $(BS_MAKEFILES)
	@$(BS_CC) -MD -MP -o $@ -c $< $(BS_CFLAGS) $(BS_INCLUDES)

-include $(BS_DEPEND_FILES)

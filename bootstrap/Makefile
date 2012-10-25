#The bootstrap component for an x86_64 kernel is built for the i686 architecture
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
BS_C_STD:= -std=c99
BS_CFLAGS:= $(BS_C_WARNINGS) $(BS_C_LIBS) $(BS_C_STD)
ifeq ($(Fdebug),1)
    BS_CFLAGS+= -O0 -DDEBUG
else
    BS_CFLAGS+= -O0
endif

#Abstracting away filenames
BS_ASM_OBJ:= $(BS_ASM_SRC:.s=.bsasm.o)
BS_C_OBJ:= $(BS_C_SRC:.c=.bsc.o)
BS_DEPEND_FILES:= $(BS_C_OBJ:.o=.d)
TMP_FILES+= $(BS_ASM_SRC:.s=.s~) $(BS_C_SRC:.c=.c~) bootstrap/Makefile~
BIN_OBJECTS+= $(BS_ASM_OBJ) $(BS_C_OBJ) $(BS_DEPEND_FILES)

#Make rules
.PHONY: bootstrap

bootstrap: $(BS_BIN) Makefile bootstrap/Makefile

$(BS_BIN): $(BS_ASM_OBJ) $(BS_C_OBJ) Makefile bootstrap/Makefile
	@$(BS_LD) -T bootstrap/arch/$(BS_ARCH)/support/bs_linker.lds -o $@ $(BS_ASM_OBJ) $(BS_C_OBJ) $(LFLAGS)

%.bsasm.o: %.s Makefile bootstrap/Makefile
	@$(BS_AS) $< -o $@

%.bsc.o: %.c Makefile bootstrap/Makefile
	@$(BS_CC) -MD -MP -o $@ -c $< $(BS_CFLAGS) $(BS_INCLUDES)

-include $(BS_DEPEND_FILES)

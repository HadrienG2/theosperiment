#Target architecture and arch-specific data go here
ifeq ($(ARCH),x86_64)
	KNL_CXX_ARCH:= -mcmodel=small -mno-red-zone -mno-mmx -msse2 -mno-sse3 -mno-3dnow
endif

#Source files go here
KNL_ASM_SRC:= $(shell find kernel/ -path '*/opt/*' -prune -o -name '*.s' -print)
KNL_CPP_SRC:= $(shell find kernel/ -path '*/opt/*' -prune -o -name '*.cpp' -print)
ifeq ($(Fdebug),1)
	KNL_ASM_SRC+= $(shell find kernel/ -path '*/opt/debug/*' -name '*.s' -print)
    KNL_CPP_SRC+= $(shell find kernel/ -path '*/opt/debug/*' -name '*.cpp' -print)
endif
ifeq ($(Ftests),1)
	KNL_ASM_SRC+= $(shell find kernel/ -path '*/opt/tests/*' -name '*.s' -print)
    KNL_CPP_SRC+= $(shell find kernel/ -path '*/opt/tests/*' -name '*.cpp' -print)
endif

#Exported headers go there
KNL_INCLUDES:= $(shell find kernel/ -path '*/opt/*' -prune -o -path '*/include' -printf '-I%p ')
ifeq ($(Fdebug),1)
    KNL_INCLUDES+= $(shell find kernel/ -path '*/opt/debug/*' -path '*/include' -printf '-I%p ')
endif
ifeq ($(Ftests),1)
    KNL_INCLUDES+= $(shell find kernel/ -path '*/opt/tests/*' -path '*/include' -printf '-I%p ')
endif

#Compilation parameters for the kernel
KNL_AS:= $(ARCH)-elf-as
KNL_CXX:= $(ARCH)-elf-g++
KNL_LD:= $(ARCH)-elf-ld
KNL_CXX_WARNINGS:= -Wall -Wextra
KNL_CXX_LIBS:= -nostdlib -ffreestanding
KNL_CXX_FEATURES:= -fno-exceptions -fno-rtti -fno-stack-protector -fno-threadsafe-statics
KNL_CXX_STD:= -std=c++0x
KNL_CXXFLAGS:= $(KNL_CXX_WARNINGS) $(KNL_CXX_LIBS) $(KNL_CXX_FEATURES) $(KNL_CXX_ARCH) $(KNL_CXX_STD)
ifeq ($(Fdebug),1)
    KNL_CXXFLAGS+= -O0 -DDEBUG
else
    KNL_CXXFLAGS+= -O0
endif

#Abstracting away filenames
KNL_ASM_OBJ:= $(KNL_ASM_SRC:.s=.knlasm.o)
KNL_CPP_OBJ:= $(KNL_CPP_SRC:.cpp=.knlcpp.o)
KNL_DEPEND_FILES:= $(KNL_CPP_OBJ:.o=.d)
TMP_FILES+= $(KNL_ASM_SRC:.s=.s~) $(KNL_CPP_SRC:.cpp=.cpp~) kernel/Makefile~
BIN_OBJECTS+= $(KNL_ASM_OBJ) $(KNL_CPP_OBJ) $(KNL_DEPEND_FILES)

#Make rules
.PHONY: kernel

kernel: $(KNL_BIN) Makefile kernel/Makefile

$(KNL_BIN): $(KNL_ASM_OBJ) $(KNL_CPP_OBJ) Makefile kernel/Makefile
	@$(KNL_LD) -T kernel/arch/$(ARCH)/support/kernel_linker.lds -o $@ $(KNL_ASM_OBJ) $(KNL_CPP_OBJ) $(LFLAGS)

%.knlasm.o: %.s Makefile kernel/Makefile
	@$(KNL_AS) $< -o $@

%.knlcpp.o: %.cpp Makefile kernel/Makefile
	@$(KNL_CXX) -MD -MP -o $@ -c $< $(KNL_CXXFLAGS) $(KNL_INCLUDES)

-include $(KNL_DEPEND_FILES)

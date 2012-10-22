#Target architecture and arch-specific data go here
ifeq ($(ARCH),x86_64)
	CXX_ARCH = -mcmodel=small -mno-red-zone -mno-mmx -mno-sse3 -mno-3dnow
endif

#Source files go here
KNL_ASM_SRC=$(wildcard kernel/arch/$(ARCH)/init/*.s)
KNL_CPP_SRC = $(wildcard kernel/arch/$(ARCH)/memory/*.cpp kernel/arch/$(ARCH)/synchronization/*.cpp)
KNL_CPP_SRC += $(wildcard kernel/init/*.cpp kernel/memory/*.cpp kernel/process/*.cpp kernel/lib/*.cpp)
ifeq ($(Fdebug),1)
    KNL_CPP_SRC += $(wildcard kernel/arch/$(ARCH)/opt/debug/*.cpp kernel/opt/debug/*.cpp)
endif
ifeq ($(Ftests),1)
    KNL_CPP_SRC += $(wildcard kernel/opt/tests/common/*.cpp kernel/arch/$(ARCH)/opt/tests/common/*.cpp)
    KNL_CPP_SRC += $(wildcard kernel/opt/tests/memory/*.cpp kernel/arch/$(ARCH)/opt/tests/memory/*.cpp)
    KNL_CPP_SRC += $(wildcard kernel/opt/tests/rpcbench/*.cpp)
endif

#Headers go there (Yeah, duplication sucks. If you know how to avoid it...)
HEADERS=$(wildcard kernel/arch/$(ARCH)/include/*.h kernel/include/*.h)
INCLUDES=-Ikernel/arch/$(ARCH)/include/ -Ikernel/include/
ifeq ($(Fdebug),1)
    HEADERS += $(wildcard kernel/arch/$(ARCH)/opt/debug/*.h kernel/opt/debug/*.h)
    INCLUDES += -Ikernel/arch/$(ARCH)/opt/debug/ -Ikernel/opt/debug/
endif
ifeq ($(Ftests),1)
    HEADERS += $(wildcard kernel/arch/$(ARCH)/opt/tests/include/*.h kernel/opt/tests/include/*.h)
    INCLUDES += -Ikernel/arch/$(ARCH)/opt/tests/include -Ikernel/opt/tests/include
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

#Abstracting away filenames
KNL_ASM_OBJ = $(KNL_ASM_SRC:.s=.knlasm.o)
KNL_CPP_OBJ = $(KNL_CPP_SRC:.cpp=.knlcpp.o)
TMP_FILES += $(KNL_ASM_SRC:.s=.s~) $(KNL_CPP_SRC:.cpp=.cpp~) kernel/Makefile~
BIN_OBJECTS += $(KNL_ASM_OBJ) $(KNL_CPP_OBJ)

#Make rules
kernel: $(KNL_BIN) Makefile kernel/Makefile

$(KNL_BIN): $(KNL_ASM_OBJ) $(KNL_CPP_OBJ) $(HEADERS) Makefile kernel/Makefile
	@$(LD) -T kernel/arch/$(ARCH)/support/kernel_linker.lds -o $@ $(KNL_ASM_OBJ) $(KNL_CPP_OBJ) $(LFLAGS)

%.knlasm.o: %.s Makefile kernel/Makefile
	@$(AS) $< -o $@

%.knlcpp.o: %.cpp $(HEADERS) Makefile kernel/Makefile
	@$(CXX) -o $@ -c $< $(CXXFLAGS) $(INCLUDES)

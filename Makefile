#Feature enable/disable flags : debug mode, kernel test suite
Fdebug:= 1
Ftests:= 1

#Specify here the architecture that the OS is being built for
ARCH:= x86_64

#Linking flags
LFLAGS:= -s --warn-common --warn-once

#Abstracting away filenames
MAKEFILES:= Makefile
BS_BIN:= arch/$(ARCH)/bin/bs-kernel.bin
KNL_BIN:= arch/$(ARCH)/bin/kernel.bin
BIN_OBJECTS:= $(BS_BIN) $(KNL_BIN)

#Arch-specific definitions belong to a separate makefile
include arch/$(ARCH)/Makefile

#Also include top-level makefiles from the various project components here
include */Makefile

#Project-wide make rules
.DEFAULT_GOAL:= all
.PHONY: all hello run clean mrproper

all: hello all_arch $(BS_BIN) $(KNL_BIN) $(MAKEFILES)
	@echo "*                                                                          *"
	@echo "*** Build complete! Please remember to keep in sync with the git repo... ***"

hello:
	@echo "********** Activating the OS|perimental automated build system... **********"
	@echo "*                                                                          *"

run: run_arch $(MAKEFILES)

clean: clean_arch $(MAKEFILES)
	@echo "* Cleaning up arch-agnostic files"
	@rm -rf $(BIN_OBJECTS)

mrproper: mrproper_arch clean $(MAKEFILES)

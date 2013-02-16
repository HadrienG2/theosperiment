#Architecture-specific flags for x86_64
LFLAGS+= -zmax-page-size=0x1000

#GRUB image generation parameters
GENISO_PARAMS:= -r -no-emul-boot -boot-info-table -quiet -A "The OS|periment" --boot-load-size 4
GRUB2_BIN_PREFIX:= grub
GRUB2_LOCAL_PREFIX:= /usr/lib/$(GRUB2_BIN_PREFIX)
GRUB2_DEST_PREFIX:= System/boot/grub2
GRUB2_STATIC_MODULES:= biosdisk iso9660 configfile

#Filename abstractions
MAKEFILES+= arch/$(ARCH)/Makefile
CDIMAGE:= cdimage.iso
CDIMAGE_ROOT:= arch/$(ARCH)/bin/cdimage
GRUB2_CONFIG:= arch/$(ARCH)/support/grub2/grub2.cfg
GRUB2_CORE_IMG:= arch/$(ARCH)/bin/grub2-core.img
GRUB2_ELTORITO_IMG:= arch/$(ARCH)/bin/grub2-eltorito.img
BIN_OBJECTS+= $(GRUB2_ELTORITO_IMG) $(GRUB2_CORE_IMG)

#Architecture-specific make rules
.PHONY: all_arch run_arch clean_arch mrproper_arch cdimage

all_arch: $(CDIMAGE) $(MAKEFILES)

run_arch: $(CDIMAGE) $(MAKEFILES)
	@echo "* Running the OS in an emulator"
	@echo c > comm_test
	@nice -n 7 bochsdbg -qf arch/$(ARCH)/support/bochsrc.txt -rc comm_test
	@rm -f comm_test

clean_arch: $(MAKEFILES)
	@echo "* Cleaning up arch-specific files"
	@rm -rf $(CDIMAGE_ROOT)/*

mrproper_arch: clean_arch $(MAKEFILES)
	@echo "* Deleting all programmatically generated arch-specific files"
	@rm -f $(CDIMAGE)

$(CDIMAGE): $(BS_BIN) $(KNL_BIN) $(GRUB2_ELTORITO_IMG) $(GRUB2_CONFIG) $(MAKEFILES)
	@echo "* Generating a CD image"
	@rm -f $(CDIMAGE)
	@rm -rf $(CDIMAGE_ROOT)/*
	@mkdir -p $(CDIMAGE_ROOT)/$(GRUB2_DEST_PREFIX)/i386-pc
	@cp $(GRUB2_LOCAL_PREFIX)/i386-pc/* $(CDIMAGE_ROOT)/$(GRUB2_DEST_PREFIX)/i386-pc
	@mkdir -p $(CDIMAGE_ROOT)/$(GRUB2_DEST_PREFIX)/locale
	@cp $(GRUB2_CONFIG) $(CDIMAGE_ROOT)/$(GRUB2_DEST_PREFIX)
	@cp $(GRUB2_ELTORITO_IMG) $(CDIMAGE_ROOT)/$(GRUB2_DEST_PREFIX)/grub2_eltorito.img
	@cp $(BS_BIN) $(CDIMAGE_ROOT)/System/boot
	@cp $(KNL_BIN) $(CDIMAGE_ROOT)/System/boot
	@cp $(KERNEL_DATAFILES) $(CDIMAGE_ROOT)/System/boot
	@genisoimage -o $(CDIMAGE) -b $(GRUB2_DEST_PREFIX)/grub2_eltorito.img $(GENISO_PARAMS) $(CDIMAGE_ROOT)

$(GRUB2_ELTORITO_IMG): $(GRUB2_CORE_IMG) $(MAKEFILES)
	@echo "* Making a bootable GRUB 2 image"
	@cat $(GRUB2_LOCAL_PREFIX)/i386-pc/cdboot.img $(GRUB2_CORE_IMG) > $(GRUB2_ELTORITO_IMG)

$(GRUB2_CORE_IMG): $(MAKEFILES)
	@echo "* Making a core GRUB 2 image"
	@echo "configfile /System/boot/grub2/grub2.cfg" > grub2_tmp.cfg
	@$(GRUB2_BIN_PREFIX)-mkimage -p '/$(GRUB2_DEST_PREFIX)/i386-pc' -c grub2_tmp.cfg -o $(GRUB2_CORE_IMG) -O i386-pc $(GRUB2_STATIC_MODULES)
	@rm -f grub2_tmp.cfg

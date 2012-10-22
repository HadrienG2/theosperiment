#Feature enable/disable flags : debug mode, kernel test suite
Fdebug = 1
Ftests = 1

#Target architecture and arch-specific data go here
ARCH = x86_64
L_ARCH = -zmax-page-size=0x1000
GENISO_PARAMS = -r -no-emul-boot -boot-info-table -quiet -A "The OS|periment" --boot-load-size 4
GRUB2_LOCAL_PREFIX = /usr/lib/grub2

#Linking flags
LFLAGS = -s --warn-common --warn-once $(L_ARCH)

#Abstracting away filenames
BS_BIN = arch/$(ARCH)/bin/bs-kernel.bin
KNL_BIN = arch/$(ARCH)/bin/kernel.bin
CDIMAGE = cdimage.iso
CDIMAGE_ROOT = arch/$(ARCH)/bin/cdimage
TMP_FILES = Makefile~
GRUB2_CORE_IMG = arch/$(ARCH)/bin/grub2-core.img
GRUB2_ELTORITO_IMG = arch/$(ARCH)/bin/grub2-eltorito.img
BIN_OBJECTS = $(KNL_BIN) $(BS_BIN) $(GRUB2_ELTORITO_IMG) $(GRUB2_CORE_IMG)

#GRUB image generation parameters
GRUB2_DEST_PREFIX = System/boot/grub2
GRUB2_STATIC_MODULES = biosdisk iso9660 configfile

#"all" rule must be first for "make" without additional arguments to work as expected
all: cdimage Makefile
	@echo "Reminder : remember to run git pull, git status, and git commit -a && git push frequently !"

#Include bootstrap and kernel makefiles
include kernel/Makefile
include bootstrap/Makefile

#Make rules
run: all Makefile
	@echo c > comm_test
	@nice -n 7 bochsdbg -qf arch/$(ARCH)/support/bochsrc.txt -rc comm_test
	@rm -f comm_test

cdimage: $(CDIMAGE) Makefile

clean: Makefile
	@rm -rf $(BIN_OBJECTS) $(CDIMAGE_ROOT)/* $(TMP_FILES)

mrproper: clean Makefile
	@rm -f $(CDIMAGE)

$(CDIMAGE): $(BS_BIN) $(KNL_BIN) $(GRUB2_ELTORITO_IMG) Makefile
	@rm -rf $(CDIMAGE)
	@rm -rf $(CDIMAGE_ROOT)/*
	@mkdir -p $(CDIMAGE_ROOT)/$(GRUB2_DEST_PREFIX)/i386-pc
	@cp $(GRUB2_LOCAL_PREFIX)/i386-pc/* $(CDIMAGE_ROOT)/$(GRUB2_DEST_PREFIX)/i386-pc
	@mkdir -p $(CDIMAGE_ROOT)/$(GRUB2_DEST_PREFIX)/locale
	@cp arch/$(ARCH)/support/grub2/grub2.cfg $(CDIMAGE_ROOT)/$(GRUB2_DEST_PREFIX)
	@cp $(GRUB2_ELTORITO_IMG) $(CDIMAGE_ROOT)/$(GRUB2_DEST_PREFIX)/grub2_eltorito.img
	@cp $(BS_BIN) $(CDIMAGE_ROOT)/System/boot
	@cp $(KNL_BIN) $(CDIMAGE_ROOT)/System/boot
	@genisoimage -o $(CDIMAGE) -b $(GRUB2_DEST_PREFIX)/grub2_eltorito.img $(GENISO_PARAMS) $(CDIMAGE_ROOT)

$(GRUB2_ELTORITO_IMG): $(GRUB2_CORE_IMG) Makefile
	@cat $(GRUB2_LOCAL_PREFIX)/i386-pc/cdboot.img $(GRUB2_CORE_IMG) > $(GRUB2_ELTORITO_IMG)

$(GRUB2_CORE_IMG): arch/$(ARCH)/support/grub2/grub2.cfg Makefile
	@echo "configfile /System/boot/grub2/grub2.cfg" > grub2_tmp.cfg
	@grub2-mkimage -p '/$(GRUB2_DEST_PREFIX)/i386-pc' -c grub2_tmp.cfg -o $(GRUB2_CORE_IMG) -O i386-pc $(GRUB2_STATIC_MODULES)
	@rm -r grub2_tmp.cfg

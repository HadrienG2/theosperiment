#The kernel cannot rely on processor features which may not exist
KNL_CXXFLAGS+= -mcmodel=small -mno-red-zone -mno-mmx -msse2 -mno-sse3 -mno-3dnow

#Abstracting away filenames
KNL_MAKEFILES+= kernel/arch/$(ARCH)/Makefile

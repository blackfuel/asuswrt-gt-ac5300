# File: bcmdrivers/char/xtmcfg/bcm963xx/Makefile
#
# Makefile for the BCM63xx bcmxtmcfg driver.
#

obj-$(CONFIG_BCM_XTMCFG) += bcmxtmcfg.o
bcmxtmcfg-objs := xtmcfgdrv.o xtmcfgmain.o xtmprocessor.o xtminterface.o xtmconnection.o xtmoamhandler.o xtmoslinux.o xtmasmhandler.o

EXTRA_CFLAGS += -DCONFIG_$(BRCM_CHIP) -fno-exceptions -fno-rtti -I. -I$(INC_BRCMDRIVER_PUB_PATH)/$(BRCM_BOARD) -I$(INC_BRCMDRIVER_PRIV_PATH)/$(BRCM_BOARD) -I$(INC_BRCMSHARED_PUB_PATH)/$(BRCM_BOARD) -I$(INC_BRCMSHARED_PRIV_PATH)/$(BRCM_BOARD) -I$(INC_ADSLDRV_PATH) -I$(INC_BRCMBOARDPARMS_PATH)/$(BRCM_BOARD)
EXTRA_CFLAGS += -DPTMBOND_DS_UNI_CHANNEL -DASM_GFC_IGNORE
EXTRA_CFLAGS += -Werror -Wfatal-errors
EXTRA_CFLAGS += ${DSL_EXT_BONDING_DRIVER_DEFINES}
ifeq "$(BRCM_CHIP)" "6368"
EXTRA_CFLAGS += -DCONFIG_VDSL_SUPPORTED
endif
EXTRA_CFLAGS += ${BUILD_DSL_FLAGS}

# no-rtti is not applicable to c files
CFLAGS_REMOVE_xtmcfgdrv.o := -fno-rtti
CFLAGS_REMOVE_xtmoslinux.o := -fno-rtti

# Some compiler flags which are defined at the top level Makefile are not
# applicable to C++ files.  Filter them out to avoid warnings.  This is a
# brute force approach, but is safer than modifying the top level Makefiles.
STRIP_FROM_CPP := -Wstrict-prototypes -Werror-implicit-function-declaration \
                  -Wdeclaration-after-statement -Wno-pointer-sign -std=gnu90 -std=gnu89
CFLAGS_REMOVE_xtmcfgmain.o := $(STRIP_FROM_CPP)
CFLAGS_REMOVE_xtmprocessor.o := $(STRIP_FROM_CPP)
CFLAGS_REMOVE_xtminterface.o := $(STRIP_FROM_CPP)
CFLAGS_REMOVE_xtmconnection.o := $(STRIP_FROM_CPP)
CFLAGS_REMOVE_xtmoamhandler.o := $(STRIP_FROM_CPP)
CFLAGS_REMOVE_xtmasmhandler.o := $(STRIP_FROM_CPP)


clean:
	rm -f core *.o *.a *.s


# File: bcmdrivers/char/omcipm/bcm963xx/Makefile
#
# Makefile for the BCM68xx Performance Monitor driver.
#

obj-$(CONFIG_BCM_OMCIPM) += bcmomcipm.o
bcmomcipm-objs += bcm_omcipm.o bcm_omcipm_user.o bcm_omcipm_threshold.o \
		bcm_omcipm_llist.o bcm_omcipm_gpon.o bcm_omcipm_dev.o \
		bcm_omcipm_enet.o

EXTRA_CFLAGS += -I$(obj)
EXTRA_CFLAGS += -I$(INC_BRCMDRIVER_PUB_PATH)/$(BRCM_BOARD)
EXTRA_CFLAGS += -I$(INC_BRCMDRIVER_PRIV_PATH)/$(BRCM_BOARD)
EXTRA_CFLAGS += -I$(INC_BRCMBOARDPARMS_PATH)/$(BRCM_BOARD)
EXTRA_CFLAGS += -I$(INC_BRCMSHARED_PUB_PATH)/$(BRCM_BOARD)
EXTRA_CFLAGS += -I$(INC_BRCMSHARED_PRIV_PATH)/$(BRCM_BOARD)

EXTRA_CFGLAG += -Werror

-include $(TOPDIR)/Rules.make

clean:
	rm -f core *.o *.a *.s .*.cmd *.ko

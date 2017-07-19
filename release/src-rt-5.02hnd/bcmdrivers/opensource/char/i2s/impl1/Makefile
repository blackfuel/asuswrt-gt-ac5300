# File: bcmdrivers/opensource/char/i2s/impl1/Makefile
#
# Makefile for the i2s driver.
#

obj-$(CONFIG_BCM_I2S) += i2s.o
# i2s-objs := i2s.o

EXTRA_CFLAGS += -Werror -Wall
EXTRA_CFLAGS += -I$(INC_BRCMDRIVER_PUB_PATH)/$(BRCM_BOARD) -I$(INC_BRCMSHARED_PUB_PATH)/$(BRCM_BOARD) -I$(INC_BRCMSHARED_PUB_PATH)/pmc

ifneq ($(strip $(BRCM_CHIP)),6858)
EXTRA_CFLAGS += -DI2S_TRY_PLL_CLK
endif

-include $(TOPDIR)/Rules.make

clean:
	rm -r core *.o *.a *.s .*.cmd *.ko
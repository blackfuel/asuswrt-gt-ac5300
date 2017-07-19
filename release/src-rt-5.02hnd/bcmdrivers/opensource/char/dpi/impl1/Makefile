dpicore-objs := dpi.o dpimgmt.o

obj-$(CONFIG_BCM_DPI) += dpicore.o

EXTRA_CFLAGS += -I$(INC_BRCMDRIVER_PUB_PATH)/$(BRCM_BOARD)
EXTRA_CFLAGS += -I$(BRCMDRIVERS_DIR)/broadcom/char/dpiengine/impl$(CONFIG_BCM_DPI_IMPL)/include
EXTRA_CFLAGS += -I$(INC_BRCMSHARED_PUB_PATH)/$(BRCM_BOARD)
EXTRA_CFLAGS += -Werror -Wall

clean:
	rm -f core *.o *.a *.s .*.cmd *.ko

# File: bcmdrivers/opensource/net/xtmrt/Makefile
#
# Makefile for the BCM63xx bcmxtmrt driver.
#

obj-$(CONFIG_BCM_XTMRT) += bcmxtmrtdrv.o
bcmxtmrtdrv-objs := bcmxtmrt.o bcmxtmrtbond.o

EXTRA_CFLAGS += -I$(INC_BRCMDRIVER_PUB_PATH)/$(BRCM_BOARD) -I$(INC_BRCMSHARED_PUB_PATH)/$(BRCM_BOARD) -I$(INC_BRCMBOARDPARMS_PATH)/$(BRCM_BOARD) \
-I$(INC_BRCMSHARED_PRIV_PATH)/$(BRCM_BOARD) -I$(INC_BRCMDRIVER_PRIV_PATH)/$(BRCM_BOARD) -DDPTM_BONDING_DEBUG  -DDPERF_MON_BONDING_US -DDPERF_MON_BONDING_DS -DPTMBOND_DS_UNI_CHANNEL -DDPTMBOND_US_PRIO_TRAFFIC_SPLIT
EXTRA_CFLAGS += -I$(BRCMDRIVERS_DIR)/opensource/net/enet/shared
EXTRA_CFLAGS += -I$(BRCMDRIVERS_DIR)/opensource/net/xtmrt/impl$(CONFIG_BCM_XTMRT_IMPL)
EXTRA_CFLAGS += -I$(BRCMDRIVERS_DIR)/broadcom/char/fap/impl$(CONFIG_BCM_FAP_IMPL)
EXTRA_CFLAGS += -I$(BRCMDRIVERS_DIR)/broadcom/char/fap/impl$(CONFIG_BCM_FAP_IMPL)/4ke
EXTRA_CFLAGS += -Werror -Wfatal-errors 
ifeq "$(BRCM_PHY_LOOPBACK)" "y"
EXTRA_CFLAGS += -DPHY_LOOPBACK
endif


clean:
	rm -f core *.o *.a *.s


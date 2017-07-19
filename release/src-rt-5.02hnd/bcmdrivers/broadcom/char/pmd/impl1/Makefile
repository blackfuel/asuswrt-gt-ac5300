# File: bcmdrivers/broadcom/char/pmd/Makefile
#
# Makefile for generic Laser driver
#

pmd-objs = pmd_dev.o reg/ru.o pmd_msg.o pmd_op.o pmd_cal.o reg/HostCommon.o pmd_temp_cal.o pmd_data_col.o
obj-$(CONFIG_BCM_PMD) += pmd.o

EXTRA_CFLAGS += -Werror -Wfatal-errors

EXTRA_CFLAGS += -I$(INC_BRCMDRIVER_PUB_PATH)/$(BRCM_BOARD)
EXTRA_CFLAGS += -I$(INC_BRCMDRIVER_PRIV_PATH)/$(BRCM_BOARD)
EXTRA_CFLAGS += -I$(INC_BRCMBOARDPARMS_PATH)/$(BRCM_BOARD)
EXTRA_CFLAGS += -I$(INC_BRCMSHARED_PUB_PATH)/$(BRCM_BOARD)
EXTRA_CFLAGS += -I$(INC_BRCMSHARED_PRIV_PATH)/$(BRCM_BOARD)
#EXTRA_CFLAGS += -I$(src)
EXTRA_CFLAGS += -I$(BRCMDRIVERS_DIR)/opensource/char/rdpa_gpl/impl1/include

GPON_STACK_DIR = $(BRCMDRIVERS_DIR)/broadcom/char/gpon/impl$(CONFIG_BCM_GPON_IMPL)/stack
EXTRA_CFLAGS += -I$(GPON_STACK_DIR)/stack
EXTRA_CFLAGS += -I$(GPON_STACK_DIR)/include
EXTRA_CFLAGS += -I$(GPON_STACK_DIR)/driver

EPON_STACK_DIR = $(BRCMDRIVERS_DIR)/broadcom/char/xenu_epon_stack/impl$(CONFIG_BCM_EPON_STACK_IMPL)
EXTRA_CFLAGS += -I$(EPON_STACK_DIR)/include

PMD_DIR = $(BRCMDRIVERS_DIR)/broadcom/char/pmd/impl$(CONFIG_BCM_PMD_IMPL)
EXTRA_CFLAGS += -I$(PMD_DIR)
EXTRA_CFLAGS += -I$(PMD_DIR)/reg
EXTRA_CFLAGS += $(INC_RDP_FLAGS)

$(obj)/pmd.o: install

install:
	$(Q)install $(src)/fw_img/pmd_fw_img.bin $(INSTALL_DIR)/etc
	
clean:
	rm -f core *.o *.a *.s .*.cmd *.ko

#
# Makefile for the BCM Pon Drv module
#

SERDES_SDK_SRC_DIR = $(src)/serdes/eagle_onu10g_src
SERDES_SDK_INC_DIR = $(src)/serdes/eagle_onu10g_include

WAN_TOP_RU_GEN_SRCS = $(notdir $(shell ls $(src)/wan_top/ru_gen/*.c))
WAN_TOP_RU_GEN_OBJS = $(addprefix wan_top/ru_gen/,$(WAN_TOP_RU_GEN_SRCS:.c=.o))

WAN_TOP_HAL_SRCS = $(notdir $(shell ls $(src)/wan_top/hal/*.c))
WAN_TOP_HAL_OBJS = $(addprefix wan_top/hal/, $(WAN_TOP_HAL_SRCS:.c=.o))


obj-$(CONFIG_BCM_PON_DRV) += bcm_pondrv.o


bcm_pondrv-objs := pon_drv.o
bcm_pondrv-objs += serdes_wrapper.o
bcm_pondrv-objs += serdes_debug.o
bcm_pondrv-objs += pon_drv_gpon_init.o
bcm_pondrv-objs += pon_drv_epon_init.o
bcm_pondrv-objs += pon_drv_xgpon_init.o
bcm_pondrv-objs += pon_drv_ngpon_10_10_init.o


bcm_pondrv-objs += $(WAN_TOP_RU_GEN_OBJS)
bcm_pondrv-objs += $(WAN_TOP_HAL_OBJS)

EXTRA_CFLAGS += -I$(src)/wan_top/hal -I$(src)/wan_top/ru_gen -I$(src)/wan_top
EXTRA_CFLAGS += -I$(SERDES_SDK_INC_DIR)
EXTRA_CFLAGS += -I$(SERDES_SDK_SRC_DIR)
EXTRA_CFLAGS += -I$(INC_BRCMDRIVER_PUB_PATH)/$(BRCM_BOARD)
EXTRA_CFLAGS += -I$(INC_BRCMSHARED_PUB_PATH)/$(BRCM_BOARD)
EXTRA_CFLAGS += $(INC_RDP_FLAGS)
EXTRA_CFLAGS += -Werror -Wfatal-errors

EXTRA_CFLAGS += -I$(INC_BDMF_PATH)/framework -I$(INC_BDMF_PATH)/system 
EXTRA_CFLAGS += -I$(INC_BDMF_PATH)/system/linux -I$(INC_BDMF_PATH)/system/linux/cm 
EXTRA_CFLAGS += -I$(INC_BDMF_PATH)/shell
#EXTRA_CFLAGS += -DUSE_BDMF_SHELL
EXTRA_CFLAGS += -Wno-error=pointer-to-int-cast -Wno-error=int-to-pointer-cast

-include $(TOPDIR)/Rules.make

$(obj)/bcm_pondrv.o: install

install:
	$(Q)install -m 755 $(src)/scripts/serdesctrl $(INSTALL_DIR)/bin

clean:
	rm -rf core *.o *.a *.s .*.cmd *.ko

# Makefile trick: breaking the following into two rules allows
# the "%.o : %.c" rule (defined earlier by kbuild), to take precidence
%.o : %.o_tmp
	$(Q)mv $< $@

%.o_tmp : %.$(BCMARCH).o_saved
	@echo "Using $<"
	$(Q)cp $< $@

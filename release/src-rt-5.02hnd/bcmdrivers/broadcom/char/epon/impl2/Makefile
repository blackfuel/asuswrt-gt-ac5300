#
# Makefile for the Epon Stack driver.
#

obj-$(CONFIG_BCM_EPON_STACK) += bcmepon.o

# Files belong to 6858
IMPL2_FILES:=$(shell cd $(src) && find . -type f -name '*.c')
bcmepon-objs += $(patsubst %.c, %.o, $(IMPL2_FILES)) 

# Files shared with other EPON platform
bcmepon-objs += ../shared/rdpa_epon.o
bcmepon-objs += ../shared/EponDriver.o
bcmepon-objs += ../shared/Stream.o
bcmepon-objs += ../shared/Mpcp.o
bcmepon-objs += ../shared/CtcStats.o
bcmepon-objs += ../shared/CtcAlarms.o
bcmepon-objs += ../shared/OntDirector.o
bcmepon-objs += ../shared/PonConfigDb.o
bcmepon-objs += ../shared/PonMgrFec.o
bcmepon-objs += ../shared/PonMgrEncrypt.o
bcmepon-objs += ../shared/OntmMpcp.o
bcmepon-objs += ../shared/EponCtrl.o
bcmepon-objs += ../shared/EponUser.o
bcmepon-objs += ../shared/EponTimer.o
bcmepon-objs += ../shared/EponFrame.o
bcmepon-objs += ../shared/Holdover.o
bcmepon-objs += ../shared/EponTypes.o
bcmepon-objs += ../shared/EponStats.o
bcmepon-objs += ../shared/OptCtrl.o
bcmepon-objs += ../shared/OpticCmds.o
bcmepon-objs += ../shared/OsAstMsgQ.o
bcmepon-objs += ../shared/ProtSwitch.o
bcmepon-objs += ../shared/EponRogueDrv.o
ifeq ("$(CONFIG_EPON_CLOCK_TRANSPORT)" ,"y")
bcmepon-objs += ../shared/ClockTransport.o
endif
ifneq ($(strip $(CONFIG_COMPAT)),)
bcmepon-objs += ../shared/EponCtrlCompat.o
endif

EXTRA_CFLAGS += $(EXTRA_CFLAGS_BDMF)
EXTRA_CFLAGS += -DUSE_BDMF_SHELL
EXTRA_CFLAGS += -DPHYS_ADDR_64BIT

EXTRA_CFLAGS += -I$(src)/../shared/include
EXTRA_CFLAGS += -I$(src)/include
EXTRA_CFLAGS += -I$(src)/include/platform
EXTRA_CFLAGS += -I$(src)/driver/hal
EXTRA_CFLAGS += -I$(src)/driver/ru_gen
EXTRA_CFLAGS += -I$(INC_BRCMDRIVER_PUB_PATH)/$(BRCM_BOARD)
EXTRA_CFLAGS += -I$(INC_BRCMDRIVER_PRIV_PATH)/$(BRCM_BOARD)
EXTRA_CFLAGS += -I$(INC_BRCMBOARDPARMS_PATH)/$(BRCM_BOARD)
EXTRA_CFLAGS += -I$(INC_BRCMSHARED_PUB_PATH)/$(BRCM_BOARD)
EXTRA_CFLAGS += -I$(INC_BRCMSHARED_PRIV_PATH)/$(BRCM_BOARD)
EXTRA_CFLAGS += -I$(INC_BRCMSHARED_PRIV_PATH)/$(BRCM_BOARD)
EXTRA_CFLAGS += -I$(INC_RDPA_MW_PATH) 
EXTRA_CFLAGS += $(INC_RDP_FLAGS)
EXTRA_CFLAGS += -I$(BRCMDRIVERS_DIR)/opensource/char/rdpa_gpl_ext/impl1/include/
EXTRA_CFLAGS += -I$(BRCMDRIVERS_DIR)/opensource/char/rdpa_gpl/impl1/include/
EXTRA_CFLAGS += -I$(BRCMDRIVERS_DIR)/opensource/net/enet/shared

ifneq (,$(filter "y","$(CONFIG_BCM96838)" "$(CONFIG_BCM96848)" "$(CONFIG_BCM96858)"))
EXTRA_CFLAGS += -I$(INC_BRCMSHARED_PUB_PATH)/drv/egphy
EXTRA_CFLAGS += -I$(INC_BRCMSHARED_PUB_PATH)/drv
endif

ifneq (,$(filter "y","$(CONFIG_BCM96838)" "$(CONFIG_BCM96848)"))
	CONFIG_PLATFORM=oren
endif

EXTRA_CFLAGS += -DBDMF_DRIVER_GPL_LAYER

EXTRA_CFLAGS += -Werror -Wall -Wno-array-bounds -Wno-int-to-pointer-cast -Wno-pointer-to-int-cast
EXTRA_CFLAGS += -DLINUX_KERNEL -DINCLUDE_LOGS -DDBA_DEBUG_STATISTICS

-include $(TOPDIR)/Rules.make

clean:
	rm -rf `find . -name '*.[o,d]'`
	rm -rf `find ../shared/ -name '*.[o,d]'`
	rm -rf `find . -name '*~'`
	rm -rf `find ../shared/ -name '*~'`
	rm -rf `find . -name '*.ko'`
	rm -rf `find ../shared/ -name '*.ko'`
	rm -rf `find . -name '.*.cmd'`
	rm -rf `find ../shared/ -name '.*.cmd'`
	rm -rf $(MODULE_NAME).mod.c modules.order Module.symvers .tmp_versions


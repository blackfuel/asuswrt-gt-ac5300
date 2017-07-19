#***********************************************************************
#
#  Copyright (c) 2015  Broadcom Corporation
#  All Rights Reserved
#
# <:label-BRCM:2015:proprietary:standard
# 
#  This program is the proprietary software of Broadcom and/or its
#  licensors, and may only be used, duplicated, modified or distributed pursuant
#  to the terms and conditions of a separate, written license agreement executed
#  between you and Broadcom (an "Authorized License").  Except as set forth in
#  an Authorized License, Broadcom grants no license (express or implied), right
#  to use, or waiver of any kind with respect to the Software, and Broadcom
#  expressly reserves all rights in and to the Software and all intellectual
#  property rights therein.  IF YOU HAVE NO AUTHORIZED LICENSE, THEN YOU HAVE
#  NO RIGHT TO USE THIS SOFTWARE IN ANY WAY, AND SHOULD IMMEDIATELY NOTIFY
#  BROADCOM AND DISCONTINUE ALL USE OF THE SOFTWARE.
# 
#  Except as expressly set forth in the Authorized License,
# 
#  1. This program, including its structure, sequence and organization,
#     constitutes the valuable trade secrets of Broadcom, and you shall use
#     all reasonable efforts to protect the confidentiality thereof, and to
#     use this information only in connection with your use of Broadcom
#     integrated circuit products.
# 
#  2. TO THE MAXIMUM EXTENT PERMITTED BY LAW, THE SOFTWARE IS PROVIDED "AS IS"
#     AND WITH ALL FAULTS AND BROADCOM MAKES NO PROMISES, REPRESENTATIONS OR
#     WARRANTIES, EITHER EXPRESS, IMPLIED, STATUTORY, OR OTHERWISE, WITH
#     RESPECT TO THE SOFTWARE.  BROADCOM SPECIFICALLY DISCLAIMS ANY AND
#     ALL IMPLIED WARRANTIES OF TITLE, MERCHANTABILITY, NONINFRINGEMENT,
#     FITNESS FOR A PARTICULAR PURPOSE, LACK OF VIRUSES, ACCURACY OR
#     COMPLETENESS, QUIET ENJOYMENT, QUIET POSSESSION OR CORRESPONDENCE
#     TO DESCRIPTION. YOU ASSUME THE ENTIRE RISK ARISING OUT OF USE OR
#     PERFORMANCE OF THE SOFTWARE.
# 
#  3. TO THE MAXIMUM EXTENT PERMITTED BY LAW, IN NO EVENT SHALL BROADCOM OR
#     ITS LICENSORS BE LIABLE FOR (i) CONSEQUENTIAL, INCIDENTAL, SPECIAL,
#     INDIRECT, OR EXEMPLARY DAMAGES WHATSOEVER ARISING OUT OF OR IN ANY
#     WAY RELATING TO YOUR USE OF OR INABILITY TO USE THE SOFTWARE EVEN
#     IF BROADCOM HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGES;
#     OR (ii) ANY AMOUNT IN EXCESS OF THE AMOUNT ACTUALLY PAID FOR THE
#     SOFTWARE ITSELF OR U.S. $1, WHICHEVER IS GREATER. THESE LIMITATIONS
#     SHALL APPLY NOTWITHSTANDING ANY FAILURE OF ESSENTIAL PURPOSE OF ANY
#     LIMITED REMEDY.
# :>
#
#***********************************************************************/

#4.16L.03_NPI PLOAM API version is 7.0
BCM_PLOAM_VERSION_API_MAJOR=7
BCM_PLOAM_VERSION_API_MINOR=0

#4.16L.03_NPI PLOAM driver version is 2.0.0
BCM_PLOAM_VERSION_DRIVER_MAJOR=2
BCM_PLOAM_VERSION_DRIVER_MINOR=0
BCM_PLOAM_VERSION_DRIVER_FIX=0

#4.16L03 OMCI API version is 1.4
BCM_OMCI_VERSION_API_MAJOR=1
BCM_OMCI_VERSION_API_MINOR=4

#4.16L03 OMCI Driver version is 1.4.0
BCM_OMCI_VERSION_DRIVER_MAJOR=1
BCM_OMCI_VERSION_DRIVER_MINOR=4
BCM_OMCI_VERSION_DRIVER_FIX=0

ifeq ("$(CONFIG_BCM96838)" ,"y")
	CONFIG_PLATFORM=oren
endif

obj-$(CONFIG_BCM_GPON) += bcmgpon.o
obj-$(CONFIG_BCM_GPON) += gponstack.o
obj-$(CONFIG_BCM_GPON) += ngponstack.o

#
# The following lines will be removed  after cleanup of 'proprietary' accessors will be completed
#

BCM_FORCE_REMOVE_HAL_GPON_RX_GRX_DEBUG_DBG_SEL=$(shell $(src)/stack/gpon/driver/hal && rm -f gpon_rx_grx_debug_dbg_sel.*)
$(warning Files gpon_rx_grx_debug_dbg_sel.* will be deleted $(BCM_FORCE_REMOVE_HAL_GPON_RX_GRX_DEBUG_DBG_SEL))
BCM_FORCE_REMOVE_HAL_GPON_RX_SHORT_FRAME=$(shell $(src)/stack/gpon/driver/hal && rm -f gpon_rx_short_frame.*)
$(warning Files gpon_rx_short_frame.* will be deleted $(BCM_FORCE_REMOVE_HAL_GPON_RX_SHORT_FRAME))
BCM_FORCE_REMOVE_RU_GPON_RX_GRX_DEBUG_DBG_SEL=$(shell $(src)/stack/gpon/driver/ru_gen && rm -f GPON_RX_GRX_DEBUG_DBG_SEL.*)
$(warning Files gpon_rx_short_frame.* will be deleted $(BCM_FORCE_REMOVE_RU_GPON_RX_GRX_DEBUG_DBG_SEL))
BCM_FORCE_REMOVE_RU_GPON_RX_SHORT_FRAME=$(shell $(src)/stack/gpon/driver/ru_gen && rm -f GPON_RX_SHORT_FRAME.*)
$(warning Files gpon_rx_short_frame.* will be deleted $(BCM_FORCE_REMOVE_RU_GPON_RX_SHORT_FRAME))

BCM_FORCE_REMOVE_HAL_NGPON_RX_BWMAP2=$(shell $(src)/stack/ngpon/driver/hal && rm -f ngpon_rx_bwmap2.*)
$(warning Files ngpon_rx_bwmap2.* will be deleted $(BCM_FORCE_REMOVE_HAL_NGPON_RX_BWMAP2))
BCM_FORCE_REMOVE_HAL_NGPON_RX_PROPRIETARY=$(shell $(src)/stack/ngpon/driver/hal && rm -f ngpon_rx_proprietary.*)
$(warning Files ngpon_rx_proprietary.* will be deleted $(BCM_FORCE_REMOVE_HAL_NGPON_RX_PROPRIETARY))
BCM_FORCE_REMOVE_HAL_NGPON_TX_DEBUG=$(shell $(src)/stack/ngpon/driver/hal && rm -f ngpon_tx_debug.*)
$(warning Files ngpon_tx_debug.* will be deleted $(BCM_FORCE_REMOVE_HAL_NGPON_TX_DEBUG))
BCM_FORCE_REMOVE_RU_NGPON_RX_BWMAP2=$(shell $(src)/stack/ngpon/driver/ru_gen && rm -f NGPON_RX_BWMAP2.c)
$(warning Files NGPON_RX_BWMAP2.* will be deleted $(BCM_FORCE_REMOVE_RU_NGPON_RX_BWMAP2))
BCM_FORCE_REMOVE_RU_NGPON_RX_PROPRIETARY=$(shell $(src)/stack/ngpon/driver/ru_gen && rm -f NGPON_RX_PROPRIETARY.c)
$(warning Files NGPON_RX_PROPRIETARY.* will be deleted $(BCM_FORCE_REMOVE_RU_NGPON_RX_PROPRIETARY))
BCM_FORCE_REMOVE_RU_NGPON_TX_DEBUG=$(shell $(src)/stack/ngpon/driver/ru_gen && rm -f NGPON_TX_DEBUG.c)
$(warning Files NGPON_TX_DEBUG.* will be deleted $(BCM_FORCE_REMOVE_RU_NGPON_TX_DEBUG))


BCM_SRC_FILES:=$(shell cd $(src) && find gpon_driver  -type f -name '*.c')
COMMON_SRC_FILES:=$(shell cd $(src) && find stack/common  -type f -name '*.c')
GPON_SRC_FILES:=$(shell cd $(src) && find stack/gpon -type f -name '*.c')
NGPON_SRC_FILES:=$(shell cd $(src) && find stack/ngpon -type f -name '*.c')

bcmgpon-objs += rdpa/rdpa_gpon.o
bcmgpon-objs += $(patsubst %.c, %.o, $(BCM_SRC_FILES)) 

gponstack-objs += $(patsubst %.c, %.o, $(COMMON_SRC_FILES))
gponstack-objs += $(patsubst %.c, %.o, $(GPON_SRC_FILES)) 

ngponstack-objs += $(patsubst %.c, %.o, $(COMMON_SRC_FILES))
ngponstack-objs += $(patsubst %.c, %.o, $(NGPON_SRC_FILES)) 


ifeq ("$(CONFIG_BCM96858)" ,"y")
	EXTRA_CFLAGS += -DPHYS_ADDR_64BIT
endif

EXTRA_CFLAGS += -DUSE_BDMF_SHELL
EXTRA_CFLAGS += -Wno-error=pointer-to-int-cast -Wno-error=int-to-pointer-cast
EXTRA_CFLAGS += -I$(src)/stack/include
EXTRA_CFLAGS += -I$(src)/stack/gpon/pwm
EXTRA_CFLAGS += -I$(src)/stack/common/driver -I$(src)/stack/gpon/driver -I$(src)/stack/ngpon/driver -I$(src)/stack/ngpon/state_machine/crypto -I$(src)/ngpon/state_machine/stack_shell
EXTRA_CFLAGS += -I$(src)/stack/gpon/driver/ru_gen -I$(src)/stack/gpon/driver/hal
EXTRA_CFLAGS += -I$(src)/stack/ngpon/driver/ru_gen -I$(src)/stack/ngpon/driver/hal 
EXTRA_CFLAGS += -I$(src)/stack/gpon/state_machine -I$(src)/gpon/state_machine/stack_shell
EXTRA_CFLAGS += -I$(src)/stack/common/logger -I$(src)/stack/common/logger_drv/include
EXTRA_CFLAGS += -I$(src)/stack/common/rogue_onu
EXTRA_CFLAGS += -I$(src)/stack/gpon/todd
EXTRA_CFLAGS += -I$(INC_BRCMBOARDPARMS_PATH)/$(BRCM_BOARD)
EXTRA_CFLAGS += -I$(INC_RDPA_MW_PATH)
EXTRA_CFLAGS += -I$(BRCMDRIVERS_DIR)/opensource/char/rdpa_gpl_ext/impl1/include/gpon_stack
EXTRA_CFLAGS += -I$(src)/rdpa
EXTRA_CFLAGS += $(INC_RDP_FLAGS)
EXTRA_CFLAGS += -Werror -Wall
EXTRA_CFLAGS += -DBCM_PLOAM_VERSION_API_MAJOR=$(BCM_PLOAM_VERSION_API_MAJOR) 
EXTRA_CFLAGS += -DBCM_PLOAM_VERSION_API_MINOR=$(BCM_PLOAM_VERSION_API_MINOR)
EXTRA_CFLAGS += -DBCM_PLOAM_VERSION_DRIVER_MAJOR=$(BCM_PLOAM_VERSION_DRIVER_MAJOR)
EXTRA_CFLAGS += -DBCM_PLOAM_VERSION_DRIVER_MINOR=$(BCM_PLOAM_VERSION_DRIVER_MINOR)
EXTRA_CFLAGS += -DBCM_PLOAM_VERSION_DRIVER_FIX=$(BCM_PLOAM_VERSION_DRIVER_FIX)
EXTRA_CFLAGS += -DBCM_OMCI_VERSION_API_MAJOR=$(BCM_OMCI_VERSION_API_MAJOR) 
EXTRA_CFLAGS += -DBCM_OMCI_VERSION_API_MINOR=$(BCM_OMCI_VERSION_API_MINOR)
EXTRA_CFLAGS += -DBCM_OMCI_VERSION_DRIVER_MAJOR=$(BCM_OMCI_VERSION_DRIVER_MAJOR)
EXTRA_CFLAGS += -DBCM_OMCI_VERSION_DRIVER_MINOR=$(BCM_OMCI_VERSION_DRIVER_MINOR)
EXTRA_CFLAGS += -DBCM_OMCI_VERSION_DRIVER_FIX=$(BCM_OMCI_VERSION_DRIVER_FIX)
EXTRA_CFLAGS += -DBCM_INCLUDE_SWITCH_DEPS
EXTRA_CFLAGS += -DBCM_GPON_USE_NVRAM
EXTRA_CFLAGS += -DINCLUDE_LOGS -DUSE_LOGGER -DRU_INCLUDE_FIELD_DB
#EXTRA_CFLAGS += -DAKIVAS_DBG
# Uncomment in order to enable dba statistic
#EXTRA_CFLAGS += -DDBA_DEBUG_STATISTICS 

EXTRA_CFLAGS += -DBDMF_DRIVER_GPL_LAYER

# Uncomment to support OMCI extended features 
# EXTRA_CFLAGS += -DBCM_OMCI_TEST
# EXTRA_CFLAGS += -DGPON_OMCI_DUMP_DATA
# EXTRA_CFLAGS += -DGPON_OMCI_DUMP_TRACE

-include $(TOPDIR)/Rules.make

clean:
	rm -rf core *.o *.a *.s .*.cmd *.ko

# Makefile trick: breaking the following into two rules allows
# the "%.o : %.c" rule (defined earlier by kbuild), to take precidence
%.o : %.o_tmp
	$(Q)mv $< $@

%.o_tmp : %.$(BCMARCH).o_saved
	@echo "Using $<"
	$(Q)cp $< $@

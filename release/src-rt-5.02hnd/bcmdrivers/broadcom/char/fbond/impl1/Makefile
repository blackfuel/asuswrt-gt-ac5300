BCMARCH=$(subst ",,$(CONFIG_BCM_CPU_ARCH_NAME))
# " #(matching quote so some editors don't complain)

flowbond-objs := fbond.o fbonddrv.o

obj-$(CONFIG_BCM_FBOND) += flowbond.o

EXTRA_CFLAGS += -I. -I$(INC_BRCMDRIVER_PUB_PATH)/$(BRCM_BOARD) -I$(INC_BRCMDRIVER_PRIV_PATH)/$(BRCM_BOARD) -I$(INC_BRCMSHARED_PUB_PATH)/$(BRCM_BOARD) -I$(INC_BRCMSHARED_PRIV_PATH)/$(BRCM_BOARD) -I$(INC_BRCMBOARDPARMS_PATH)/$(BRCM_BOARD)
EXTRA_CFLAGS += -Werror -Wfatal-errors 

ifeq  ($(strip $(BRCM_DRIVER_FBOND_DEBUG)),y)
EXTRA_CFLAGS += -DPKTDBG
endif

# -include $(TOPDIR)/Rules.make

clean:
	rm -f core *.o *.a *.s *.ko


# Makefile trick: breaking the following into two rules allows
# the "%.o : %.c" rule (defined earlier by kbuild), to take precedence

%.o : %.o_tmp
	$(Q)mv $< $@

%.o_tmp : %.$(BCMARCH).o_saved
	@echo "Using $<"
	$(Q)cp $< $@


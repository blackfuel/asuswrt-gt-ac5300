include ../common.mak

all: 
	@echo "do nothing"

install:
	mkdir -p $(INSTALLDIR)/lib/
	ln -sf /var/firmware $(INSTALLDIR)/lib/firmware
	install -D ./serial/rtl8822b/rtl8822b_fw $(INSTALLDIR)/rom/etc/rtl8822b_fw
	install -D ./serial/rtl8822b/rtl8822b_config $(INSTALLDIR)/rom/etc/rtl8822b_config

clean:
	@echo "do nothing"

PHONY += FORCE
FORCE:

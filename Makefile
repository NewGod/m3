# ShiftAC's C/C++ makefile template for mkcproj 1.2

INSTALL_PATH := /usr/bin
PACKAGE_PREFIX := m3
TARGET := bin

.PHONY: all
all: init
	$(MAKE) -C src TARGET="../$(TARGET)" PACKAGE_PREFIX=$(PACKAGE_PREFIX)
	$(MAKE) -C dataset

.PHONY: netemu
netemu:
	$(MAKE) -C src NetEmulator TARGET="../$(TARGET)" PACKAGE_PREFIX=$(PACKAGE_PREFIX)

.PHONY: init
init:
	-mkdir bin
	-mkdir temp
	-mkdir log

.PHONY: install
install:
	cp bin/* $(INSTALL_PATH)/

.PHONY: uninstall
uninstall:
	-rm $(INSTALL_PATH)/$(PACKAGE_PREFIX)-*

.PHONY: clean
clean:
	-rm bin/* -r
	$(MAKE) -C src clean PACKAGE_PREFIX=$(PACKAGE_PREFIX)
	$(MAKE) -C dataset clean

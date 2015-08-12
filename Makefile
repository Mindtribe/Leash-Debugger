#SDK stuff
SDK_DIR=./cc3200-sdk

#wifidebugger
WIFIDEBUGGER_DIR=./wifidebugger

#testapp
TESTAPP_DIR = ./testapp

#base directory
BASE_DIR = $(shell pwd)

#rules
.PHONY: all clean distclean wifidebugger testapp

all: wifidebugger testapp

wifidebugger:
	cd $(WIFIDEBUGGER_DIR) && make

testapp:
	cd $(TESTAPP_DIR) && make

driverlib:
	cd $(SDK_DIR)/driverlib/gcc && make

clean:
	@- cd $(WIFIDEBUGGER_DIR) && make clean
	@- cd $(TESTAPP_DIR) && make clean
	@- cd $(SDK_DIR)/driverlib/gcc && make clean

distclean: clean

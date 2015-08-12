# Wi-Fi Debug Adapter #

This is the code for Mindtribe's CC3200 LaunchPad-based debug adapter.

The main project is in the "wifidebugger" folder.
The "testapp" folder holds a small test application for use on the target when testing the debugger.


The repo lives here: https://github.com/Mindtribe/Wi-Fi-Debug-Adapter

# Instructions for building and running #

Install arm-none-eabi-gcc. Project should then compile.
The most straightforward way to run this on the board is using OpenOCD+GDB (see below).

To test the OpenOCD connection to the target board, try the following:

cd scripts
sudo ./openocd_test

if successful, this should lead to OpenOCD reporting a connection to the device, and giving information about it (number of break/watchpoints etc).

To debug an .axf file, try:

cd scripts
sudo ./gdb_debug ./../wifidebugger/exe/wifidebugger.axf

this should start a GDB debugging session using openOCD.

# Misc #

Depends on the TI CC3200 SDK, which can be installed in Windows and copied to whatever OS, or installed in linux using wine. Its relevant parts are included in this repo.
Toolchain is arm-none-eabi-gcc. Tested on Ubuntu only. Rough set-up instructions for the toolchain found here: 
	http://azug.minpet.unibas.ch/~lukas/bricol/ti_simplelink/CC3200-LaunchXL.html
...But using the OpenOCD instructions (using libusb instead of ft2232 driver) from here: 
	https://hackpad.com/Using-the-CC3200-Launchpad-Under-Linux-Rrol11xo7NQ

Current build system is a makefile, future plans to switch to MTbuild.

# Project Status #

In implementation phase of JTAG master functionality. Lowest level is bitbanged GPIO, one level higher is performing scanchain operations. 
A cc3200_icepick layer is finished. Work being done on Cortex M4 debug operations.

Future steps:
- direct serial link to/from host PC with abstraction layer.
- Wi-Fi serial link to/from host PC using same abstraction layer as direct link.
- gdbserver stub.
- (possibly) OpenOCD "remote_bitbang" protocol stub.

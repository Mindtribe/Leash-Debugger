# Wi-Fi Debug Adapter #

This is the code for Mindtribe's CC3200 LaunchPad-based debug adapter.

The repo lives here: https://github.com/Mindtribe/Wi-Fi-Debug-Adapter

Depends on the TI CC3200 SDK, which can be installed in Windows and copied to whatever OS, or installed in linux using wine. Its relevant parts are included in this repo.
Toolchain is arm-none-eabi-gcc. Tested on Ubuntu only. Rough set-up instructions for the toolchain found here: 
	http://azug.minpet.unibas.ch/~lukas/bricol/ti_simplelink/CC3200-LaunchXL.html
...But using the OpenOCD instructions (using libusb instead of ft2232 driver) from here: 
	https://hackpad.com/Using-the-CC3200-Launchpad-Under-Linux-Rrol11xo7NQ

Current build system is a makefile, future plans to switch to MTbuild.

Latest project status:
In implementation phase of JTAG master functionality. Lowest level is bitbanged GPIO, one level higher is performing scanchain operations. First versions of these layers are ready.

Future steps:
- another layer of JTAG abstraction, for performing ICEPICK router configuration.
- another layer of JTAG abstraction, for performing Cortex M4-specific debug operations.
- direct serial link to/from host PC with abstraction layer.
- Wi-Fi serial link to/from host PC using same abstraction layer as direct link.
- gdbserver stub.
- (possibly) OpenOCD "remote_bitbang" protocol stub.

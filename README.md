# Wi-Fi Debug Adapter #

This is the code for Mindtribe's CC3200 LaunchPad-based debug adapter.

The repo lives here: https://github.com/Mindtribe/Wi-Fi-Debug-Adapter

Depends on the TI CC3200 SDK, which can be installed in Windows and copied to whatever OS, or installed in linux using wine.

Current build system is a makefile, future plans to switch to MTbuild.


Some notes about the set-up:
- This was done on Ubuntu.
- Rough set-up instructions for the toolchain found here: 
	http://azug.minpet.unibas.ch/~lukas/bricol/ti_simplelink/CC3200-LaunchXL.html
...But using the OpenOCD instructions (using libusb instead of ft2232 driver) from here: 
	https://hackpad.com/Using-the-CC3200-Launchpad-Under-Linux-Rrol11xo7NQ


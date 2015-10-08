### Prerequisites

The primary build system Leash Debugger is developed with is **MTBuild** (link). For installation instructions, please see (link). Makefile-based building is also an option, but the Makefiles provided with the project are prone to go out of date since they are not actively used.

In order to debug using Leash Debugger, assuming an ARM-based target, **arm-none-eabi-gdb** is required.

If you wish to debug Leash Debugger itself using a USB interface, you can use **arm-none-eabi-gdb** in conjunction with **OpenOCD** (link).

Depends on the **TI CC3200 SDK**, which can be installed in Windows and copied to whatever OS, or installed in linux using wine. Its relevant parts are included in this repo, but certain unneeded parts, such as example code, has been removed.

Rough set-up instructions for the ARM toolchain can be found here: 
	http://azug.minpet.unibas.ch/~lukas/bricol/ti_simplelink/CC3200-LaunchXL.html
...But for debugging from Ubuntu, using the OpenOCD instructions (using libusb instead of ft2232 driver) from here: 
	https://hackpad.com/Using-the-CC3200-Launchpad-Under-Linux-Rrol11xo7NQ

### Building

```sh
MTBuild:           $ mtbuild
Makefile build:    $ make
```

The build output files are stored in a **build/** folder in the project base directory.

### Running/Flashing

The **scripts** folder provides a few GDB and sh scripts which are useful for running and debugging both Leash Debugger and other targets via Leash Debugger. These are subject to change, and may need to be modified for your particular toolchain setup.

At the time of writing this, the only tool capable of flashing .bin firmware images onto the CC3200 external flash is **TI Uniflash** (link). Leash Debugger aims to support flashing in the future, using a flashing stub.

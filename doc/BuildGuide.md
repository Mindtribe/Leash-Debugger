Doc pages:
* [**Build Guide**](BuildGuide.md)
* [**User Guide**](UserGuide.md)
* [**Wiring**](Wiring.md)
* [**Report**](Report.md)
* [**Known Issues**](KnownIssues.md)

# Leash Debugger: Build Guide
Jump to:
* [**Prerequisites**](###Prerequisites)
* [**Building**](###Building)
* [**Running/Flashing**](###Running/Flashing)

### Prerequisites

The primary build system Leash Debugger is developed with is **MTBuild** (link). For installation instructions, please see: https://github.com/Mindtribe/MTBuild.

In order to debug using Leash Debugger, assuming an ARM-based target, **arm-none-eabi-gdb** is required.
If you wish to debug Leash Debugger itself using a USB interface, you can use **arm-none-eabi-gdb** in conjunction with **OpenOCD**.

Leash depends on the **TI CC3200 SDK**. Its relevant parts are included in this repo, but certain unneeded parts, such as example code, have been removed.

Rough set-up instructions for the ARM toolchain can be found here: 
	http://azug.minpet.unibas.ch/~lukas/bricol/ti_simplelink/CC3200-LaunchXL.html
...But for debugging from Ubuntu, using the OpenOCD instructions (using libusb instead of ft2232 driver) from here: 
	https://hackpad.com/Using-the-CC3200-Launchpad-Under-Linux-Rrol11xo7NQ

### Building

```sh
MTBuild:           $ mtbuild
```

The build output files are stored in a **build/** folder in the project base directory. Leash Debugger's binary files are stored in **build/leash/leash/**.

### Running/Flashing

The **scripts** folder provides a few GDB and sh scripts which are useful for running and debugging both Leash Debugger and other targets via Leash Debugger. These are subject to change, and may need to be slightly modified for your particular toolchain setup.

At the time of writing this, the only tool capable of flashing .bin firmware images onto the CC3200 external flash is **TI Uniflash**. Leash Debugger aims to support flashing in the future, using a flashing stub.

**scripts/OpenOCD/openocd_test** can be used to check that your OpenOCD setup is working correctly to connect to a LaunchpadXL CC3200 board over USB. If successful, it will just display some information about the target device, after which you can exit by pressing **CTRL-C**.

**scripts/debug_leash_openocd** will use OpenOCD over USB to load and run Leash Debugger in a GDB session on the LaunchpadXL board.
**scripts/debug_testapp** will connect to a running Leash Debugger over a TCP connection (WiFi), start a GDB session and load the ``testapp'' which is included in this project. OpenOCD is not required for this, however the target board should be connected using JTAG to the Leash Debugger board. See the User Guide for details. **Note: you will need to change the IP address in scripts/GDB/gdbinit_testapp_tcp to your Leash Debugger's IP for this script to work properly.**


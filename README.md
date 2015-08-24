# Wi-Fi Debug Adapter #

This is the code for Mindtribe's CC3200 LaunchPad-based debug adapter.

The main project is in the "wifidebugger" folder.
The "testapp" folder holds a small test application for use on the target when testing the debugger.


The repo lives here: https://github.com/Mindtribe/Wi-Fi-Debug-Adapter

# Instructions for building and running #

Install arm-none-eabi-gcc. Project should then compile.
The most straightforward way to run this on the board is using OpenOCD+GDB (see below).

To test the OpenOCD connection to the target board, try the following:

```
cd scripts
./openocd_test
```

if successful, this should lead to OpenOCD reporting a connection to the device, and giving information about it (number of break/watchpoints etc).

To debug an .axf file (the debugger code), try:

```
cd scripts
./gdb_debug_debugger ./../wifidebugger/exe/wifidebugger.axf
```

this should start a GDB debugging session using openOCD, running the debug adapter code on the board.

Starting a session to debug the target via the running debugger can be done by having another instance of GDB:

```
cd scripts
./gdb_debug_debuggee ./../testapp/exe/testapp.axf
```

...After which GDB commands have to be used to actually load code, set pc/sp and execute:

```
(gdb) target remote /dev/ttyUSB1
(gdb) load
(gdb) compare-sections
(gdb) set $sp = g_pfnVectors[0]
(gdb) set $pc = g_pfnVectors[1]
(gdb) continue
```

NOTE: in order to gain non-root access to ttyUSB for debugging, add your user to the dialout group:

```
sudo usermod -a -G dialout $USER
```

... and also in Ubuntu, this only seems to work after removing software called 'modemmanager':

```
sudo apt-get remove modemmanager
```


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

Also it uses UART to communicate with GDB using a basic GDBserver stub. Packet support so far includes:
- memory read/write
- register read/write (although xPSR register writes are ignored - they seem to lead to hard faults when writing to them)
- interrupt/continue
- CRC checks on memory regions (allowing GDB to verify RAM loads)

The main missing ones are:
- step
- reset

With the current functionality, it is able to load a program into RAM, set stack pointer/program counter and execute it.

RAM loading is slow: less than 300 Bytes/s.

Future steps:
- Wi-Fi serial link to/from host PC using same abstraction layer as direct link.
- extended support for packets (symbol reading, binary transfer...)
- (possibly) OpenOCD "remote_bitbang" protocol stub.

# Other notes #

ERROR DETECTION AND RECOVERY

Basic error detection is in place, but recovery from errors is not present. Example: after a hard reset, debug hardware won't be accessible for (seen in trial and error) a second. This, in the past, has resulted in failure to read out the CSW register of the MEM-AP, or failure in earlier stages depending on the amount of time after the reset. It should be possible to recover from such errors and wait until the reset is complete.

RESET HARDWARE

The LaunchpadXL does not offer direct access to the CC3200 RESET line (the "RST" pin is output-only). A wire needs to be soldered to perform hard resets over the JTAG interface.

FLASHING

As far as I've been able to find, the only known method to program the external flash on the Launchpad boards is to use TI's Uniflash tool (other than trying to program the flash chip directly). OpenOCD does not support flashing this board, only loading directly into RAM. It would be nice to extend the scope of the project with a stub program that can be loaded into the RAM, and then acts as a proxy for writing to the flash.

CACHING

As of now, the code is being built iteratively, solving problems on each iteration. To provide better robustness and better insight into root causes when errors do occur in various layers of the JTAG interface, it would be good to cache certain registers on the debugger. For example, the control/status registers of the ICEPICK, of the JTAG_DP and the MEM_AP.

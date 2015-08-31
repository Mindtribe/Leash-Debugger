Scripts in this directory:

cc3200_cfg: cfg file for OpenOCD to connect to the CC3200 over USB.

openocd_test: attempts to connect to CC3200 over USB using OpenOCD. If successful, OpenOCD will report some properties of the device.

gdb_debug_debugger: uses OpenOCD to connect to CC3200 over USB (typically used to debug the wi-fi debugger). Uses gdbinit_debugger GDB script. Specify the input AXF file to use.

gdb_debug_debuggee: connects GDB to a serial port on ttyUSB1. Assuming the debugger is working and properly configured, this will allow GDB to connect to the wi-fi debugger to debug the debuggee (for now, over a serial link instead of an actual wi-fi connection). Uses gdbinit_debuggee script. (Work in progress)

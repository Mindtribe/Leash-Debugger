Doc pages:
* [**Quick-start Guide**](QuickStart.md)
* [**Build Guide**](BuildGuide.md)
* [**User Guide**](UserGuide.md)
* [**Wiring**](Wiring.md)
* [**Report**](Report.md)
* [**Known Issues**](KnownIssues.md)

# Leash Debugger: User's Guide

Leash Debugger is a JTAG debug adapter for use over a WiFi connection with GDB. It allows GDB to connect directly to it, as opposed to using additional debugging software such as OpenOCD. This document aims to give a complete overview of its features and instructions on how to use them. For using Leash Debugger for the first time, in most cases it is easier to start by reading the [**Quick-start Guide**](QuickStart.md).

### Platform

Leash debugger runs on the **Texas Instruments CC3200 SoC**, and was designed for running on the **LaunchpadXL** development board.

### Supported Target Platforms

Currently, the only supported target configuration is a **Texas Instruments CC3200 SoC** over a **4-wire JTAG connection**.

# User's Guide
## Basic Operation

Leash Debugger is designed to accept a connection from GDB over TCP. The workflow is the same as a typical setup using OpenOCD, except OpenOCD is not required. Although the TCP connection to the host workstation can be made using WiFi, Leash Debugger still needs to be wired up to the target to be debugged using a wired JTAG connection (see [Wiring](doc/Wiring.md)).

### Modes

Leash Debugger has two modes: AP and Station. 

In AP mode, it starts its own WiFi access point which the user can connect to. This allows the user to debug, or alternatively to configure network profiles which the debugger can connect to in Station mode. 

In Station mode, the debugger tries to connect to one of the stored network profiles.

### Buttons

Only the LaunchpadXL **SW3** button is used. If this is held down during boot, the Leash Debugger will run in **AP mode** (its own access point). Otherwise, it will run in **Station mode** (connect to an existing WiFi network).

### LEDs

The LaunchpadXL board has three user LEDs: red, orange and green. Each LED corresponds to an aspect of functionality:

* **Red**: error state.
* **Orange**: JTAG state.
* **Green**: WiFi state.

Blink codes are used to indicate the state. They are as follows:

##### Red (error) blink codes
* **Off**: no error.
* **On**: error(s) encountered - check log for details.

##### Orange (JTAG) blink codes
* **Off**: JTAG inactive.
* **Steady blink**: JTAG connecting to target.
* **Fully on**: Connected - target is halted.
* **Short blink @ long interval**: Connected - target is running.
* **Short fast blink**: JTAG failure.

##### Green (WiFi) blink codes
* **Off**: WiFi inactive.
* **On**: WiFi connected to AP in Station mode.
* **Steady blink**: WiFi scanning for networks.
* **Blink short on, long off**: WiFi connecting (in either AP or Station mode).
* **Blink long on, short off**: AP mode active.
* **Short fast blink**: WiFi error.

## Connecting to Leash Debugger
### Sockets
Leash debugger has three sockets to connect to:
* **Log socket (49152)**: Serial socket for viewing log information and configuring Leash Debugger
* **Target socket (49153)**: Serial socket for UART-style communication with the debug target (currently not yet supported)
* **GDB socket (49154)**: Socket which GDB on host machine can connect to for a debugging session.

These sockets are available in both AP and Station modes.

### IP Addresses and Name Resolution

In AP mode, Leash Debugger's IP address is **192.168.1.1**. In Station mode, Leash Debugger assumes a DHCP server is available to assign it an IP address. To be discoverable on the local network, it uses **mDNS** (a.k.a. **Apple Bonjour**). (link to section)

### Connection Process

For the **Log socket** and the **Target socket**, the intended use-case is to use a raw TCP/IP serial terminal. On Linux or Mac systems, **netcat (aka nc)** can be used. In general, any terminal tool can be used which supports raw TCP communication (for example, PuTTY on Windows). Assuming Leash Debugger's IP is 192.168.1.1, the following command will connect to the **Log socket**:

```sh
nc 192.168.1.1 49152
```
Upon the first connection to this socket, all log information produced so far should immediately start appearing in the terminal. Note that using the log socket is not necessary, but it may give useful information about the state of the debugger.

For the **GDB socket**, GDB should be configured to connect to the socket. In typical projects using OpenOCD instead of Leash Debugger, the GDB connection would be to OpenOCD using a pipe, usually using a line resembling the following in the GDB initialization script:

```
target remote | openocd -c "gdb_port pipe; log_output ~/openocd.log" -f cc3200.cfg
```

For Leash Debugger, this line can be changed to the following, assuming Leash Debugger has IP 192.168.1.1:

```
target remote tcp:192.168.1.1:49154
```

Setting the baud rate, as you would typically do for an OpenOCD session, is not required.
If you want to test whether Leash Debugger is responding at all to GDB commands, you can enable GDB's remote protocol verbose debugging to see the individual messages (also in the GDB script or from the GDB command line):

```
(gdb) set debug remote 1
```

Also, it is recommended to increase GDB's remote time-out value to deal with possible long network latencies. For example:

```
(gdb) set remote timeout 30
```

The above steps and settings can be put in a GDB script file, for example for use with GDB-supporting IDE's like Eclipse.

**Note: make sure that you use the GDB version applicable for your target architecture. For debugging the CC3200 (ARM Cortex-M4-based), that means installing and using arm-none-eabi-gdb.**

## Usage

## Network Configuration

In Station Mode, Leash Debugger will try to connect to a WiFi network. For this, it uses the CC3200's WiFi network profiles stored on the CC3200 filesystem. It is also possible to add/remove network profiles over a TCP connection. So if there are no network profiles stored yet in your CC3200 running Leash Debugger, the following steps can be taken to configure new networks:

* Start Leash Debugger in AP mode (by holding down SW3 during boot)
* Connect your workstation to Leash Debugger's access point
* Connect to the **Log port** using a raw TCP terminal
* Use the configuration commands below to add network profile(s)
* Reboot Leash Debugger in Station mode

### Configuration Commands

When a connection to the **Log port** is opened, you should see log information appearing on-screen. Now you can issue network configuration commands. the commands are as follows:

* **mac**: Show the Leash Debugger device's MAC address.
* **network**: Add a new network SSID profile or replace the one already stored by the same name.
* **delete**: Delete a network SSID profile.
* **deleteall**: Delete all network SSID profiles stored in the device.
* **list**: List the SSID's currently stored in the device.
* **help**: Show a list of these supported commands.

It is not necessary to supply arguments when issuing these commands: you will be prompted to enter the required information, such as SSID, security type (open, WEP and WPA are supported) and security key.

## Flashing the target

Leash Debugger supports a few commands to manipulate the flash memory on the target CC3200. However, its features are limited, mostly due to the fact that Texas Instruments operates a proprietary filesystem on the CC3200's external flash. Only a subset of functionality is exposed to the public. Leash Debugger supports:

* Writing files to flash
* Reading files from flash
* Deleting specific files from flash

Leash Debugger **does not** support:

* Getting a list of files on the flash (TI Uniflash cannot do this either)
* Formatting flash memory (TI Uniflash is able to do this, and it is sometimes needed to recover a non-bootable device).

**Before using the flashing features, please carefully read the instructions below.**

### Flash mode and the Flash stub

The method Leash Debugger uses to perform flash operations is as follows: it loads a small program, called the **flash stub**, onto the target RAM and executes it. Then it can communicate to the **flash stub** using the JTAG link, and instruct it to perform flash operations. That means that in order to perform flash operations, the current debug session will be aborted - after all, whatever is running on the target will first need to be replaced by the **flash stub**.

In practice, this means that Leash Debugger needs to be instructed by the user to enter "**flash mode**" (loading the stub and executing it) before operations can be performed. A special GDB command is used for this:

```
(gdb) monitor flashmode=1
```

A message should appear after several seconds, notifying you that flash mode has been entered. If you need to see more verbose information, the **Log Socket** can be inspected during this operation.
At this moment, **it is not possible to exit flash mode once entered** - to start a new debugging session, Leash Debugger and the target must be reset.

**Note: Leash Debugger needs to have access to the flash stub binary file. It attempts to find this file on external flash memory *of the debugger*. In other words, before using this feature, it is necessary to store the flash stub binary (found under build/leash/cc3200_flashstub/debug/leash-cc3200_flashstub-Debug.bin) on the Leash Debugger flash using TI Uniflash. The name of the binary file on the CC3200 flash should be "cc3200_flashstub.bin".**

Once in flash mode, GDB's remote filesystem commands can be used to manipulate flash. The supported operations are:

```
(gdb) remote put (localfile) (remotefile)
(gdb) remote get (remotefile) (localfile)
(gdb) remote delete (remotefile)
```
where (localfile) is the filename on the host system, and (remotefile) the filename on the CC3200 (don't include the parentheses). The file which gets booted by the bootloader on CC3200 startup is called **/sys/mcuimg.bin**.

**Note: an important extra step is required to write files using remote put.** This has to do with TI's proprietary filesystem: it is necessary to specify what will be the maximum size the file will take on flash, before writing it. By default, Leash Debugger uses 176kB as the maximum file size when writing a new file, because this is the maximum size that is guaranteed to be bootable as a .bin image. If a different size is required, you will have to specify this **before using remote put**:

```
(gdb) monitor setmaxalloc=(bytes)
```
where (bytes) is the number of bytes required, in decimal notation (don't include the parentheses). After using this command, all subsequent files which are opened for writing will use this maximum size.

If you wish to verify that a flash file write was successful, there are multiple approaches:
* You can read back the file you wrote using **remote get**, and do a binary comparison to the file you sent. If they are not equal, corruption has occurred somewhere.
* An easier method is built in to Leash Debugger. As Leash Debugger receives data from GDB, it keeps an internal CRC value, which it remembers for the most recent file that was written. After writing is complete, Leash Debugger can request the flash stub to compute the CRC from flash memory, and compare it to this internal CRC.
If they match, that means no corruption occurred between Leash Debugger and the target. It does not exclude corruption between GDB (the workstation) and Leash Debugger - but corruption occurring there is very unlikely, since the communication protocol between them has its own, separate CRC checking built in.
To use this feature, right after transfering your file to the target, use the following command:

```
(gdb) monitor checkcrc=(filename)
```
Where (filename) is the exact same remote filesystem name of the file you just transfered (no parentheses). The response should be "CRC Match" - otherwise, try transfering the file again, making sure you choose the correct allocation size.

## Name Resolution using mDNS/Apple Bonjour

If you do not know Leash Debugger's IP on the WiFi network, you can use **mDNS** (a.k.a. **Apple Bonjour**) to find its advertised services. MAC OS and Linux readily provide mDNS services, whereas for Windows services need to be installed.

### Linux mDNS

Most Linux distributions have mDNS services pre-installed in the form of **avahi**. If not, you can install it manually. 

### MAC OS mDNS

MAC OS offers a tool called **dns-sd** to browse mDNS services.

### Windows mDNS

**Bonjour for Windows**, by Apple, can be used to enable mDNS support on Windows systems.

### Name Resolution Commands

To confirm Leash Debugger's existence on the network, run

```sh
Linux:    avahi-browse -r _http._tcp
MAC OS:   dns-sd ???
Windows:  ???
```

In the resulting list of services, all Leash Debugger devices should show up as "LeashDebug{MAC}", where {MAC} is the debugger's MAC address. Note that Leash Debugger does NOT offer a HTTP server - however, its IP can be found through resolving this mDNS entry.

In case you want to double-check which ports to connect to, Leash Debug also advertises its ports separately. The **Log socket** and the **Target socket** are of type "_raw-serial._tcp", while the **GDB socket** is of type "_gdbremote._tcp". The description fields of the sockets are as follows:

* **Log socket**: Leash Debugger Log Socket
* **Target socket**: Leash Debugger Target Socket
* **GDB socket**: Leash Debugger GDB Socket

For example, for finding the **GDB socket**'s IP and port, run:

```sh
Linux:    avahi-browse -r _gdbremote._tcp
MAC OS:   dns-sd ???
Windows:  ???
```

and inspect the result.

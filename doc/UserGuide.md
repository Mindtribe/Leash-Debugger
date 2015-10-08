Doc pages:
* [**Build Guide**](doc/BuildGuide.md)
* [**User Guide**](doc/UserGuide.md)
* [**Wiring**](doc/Wiring.md)
* [**Report**](doc/Report.md)
* [**Known Issues**](doc/KnownIssues.md)

# Leash Debugger: User's Guide

Leash Debugger is a JTAG debug adapter for use over a WiFi connection with GDB. It allows GDB to connect directly to it, as opposed to using additional debugging software such as OpenOCD.

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
Upon the first connection to this socket, all log information produced so far should immediately start appearing in the terminal.

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

**Note: make sure that you use the GDB version applicable for your target architecture. For debugging the CC3200 (ARM Cortex-M4-based), that means installing and using arm-none-eabi-gdb.**

## Network Configuration

In Station Mode, Leash Debugger will try to connect to a WiFi network. For this, it uses the CC3200's WiFi network profiles stored on the CC3200 filesystem. It is also possible to add/remove network profiles over a TCP connection. So if there are no network profiles stored yet in your CC3200 running Leash Debugger, the following steps can be taken to configure new networks:

* Start Leash Debugger in AP mode (by holding down SW3 during boot)
* Connect your workstation to Leash Debugger's access point
* Connect to the **Log port** using a raw TCP terminal
* Use the configuration commands below to add network profile(s)
* Reboot Leash Debugger in Station mode

### Configuration Commands

When a connection to the **Log port** is opened, you should see log information appearing on-screen. Now you can issue network configuration commands. the commands are as follows:

* **network**: Add a new network SSID profile or replace the one already stored by the same name.
* **delete**: Delete a network SSID profile.
* **deleteall**: Delete all network SSID profiles stored in the device.
* **list**: List the SSID's currently stored in the device.
* **help**: Show a list of these supported commands.

It is not necessary to supply arguments when issuing these commands: you will be prompted to enter the required information, such as SSID, security type (open, WEP and WPA are supported) and security key.

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

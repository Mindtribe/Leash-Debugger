# Leash Debugger

Leash Debugger is a JTAG debug adapter for use over a WiFi connection with GDB. It allows GDB to connect directly to it, as opposed to using additional debugging software such as OpenOCD.

### Platform

Leash debugger runs on the Texas Instruments CC3200 SoC, and was designed for running on the LaunchpadXL development board. (link)

### Supported Target Platforms

Currently, the only supported target configuration is a Texas Instruments CC3200 SoC over a 4-wire JTAG connection.

# User's Guide
## Basic Operation

### Modes

Leash Debugger has two modes: AP and Station. 

In AP mode, it starts its own WiFi access point which the user can connect to. This allows the user to debug, or alternatively to configure network profiles which the debugger can connect to in Station mode. 

In Station mode, the debugger tries to connect to one of the stored network profiles.

### Buttons

Only the LaunchpadXL SW3 button is used. If this is held down during boot, the Leash Debugger will run in AP mode. Otherwise, it will run in Station mode.

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
* **Log socket (...)**: Serial socket for viewing log information and configuring Leash Debugger
* **Target socket (...)**: Serial socket for UART-style communication with the debug target (currently not yet supported)
* **GDB socket (...)**: Socket which GDB on host machine can connect to for a debugging session.

These sockets are available in both AP and Station modes.

### IP Addresses and Name Resolution

In AP mode, Leash Debugger's IP address is **192.168.1.1**. In Station mode, Leash Debugger assumes a DHCP server is available to assign it an IP address. To be discoverable on the local network, it uses **mDNS** (a.k.a. **Apple Bonjour**). (link to section)

### Connection Process

For the **Log socket** and the **Target socket**, the intended use-case is to use a raw TCP/IP serial terminal. On Linux systems, **netcat (aka nc)** can be used. Assuming Leash Debugger's IP is 192.168.1.1, the following command will connect to the **Log socket**:

```sh
nc 192.168.1.1 (...)
```
Upon the first connection to this socket, all log information produced so far should immediately start appearing in the terminal.

For the **GDB socket**, GDB should be configured to connect to the socket. In typical projects using OpenOCD instead of Leash Debugger, the GDB connection would be to OpenOCD using a pipe, usually using a line resembling the following in the GDB initialization script:

```sh
target remote | openocd -c "gdb_port pipe; log_output ~/openocd.log" -f cc3200.cfg
```

For Leash Debugger, this line can be changed to the following, assuming Leash Debugger has IP 192.168.1.1:

```sh
target remote tcp:192.168.1.1:(...)
```

Setting the baud rate, as you would typically do for an OpenOCD session, is not required.
If you want to test whether Leash Debugger is responding at all to GDB commands, you can enable GDB's remote protocol verbose debugging to see the individual messages (also in the GDB script or from the GDB command line):

```sh
(gdb) set debug remote 1
```

## Name Resolution using mDNS/Apple Bonjour

If you do not know Leash Debugger's IP on the WiFi network, you can use **mDNS** (a.k.a. **Apple Bonjour**) to find its advertised services. MAC OS and Linux readily provide mDNS services, whereas for Windows services need to be installed.

### Linux mDNS

Most Linux distributions have mDNS services pre-installed in the form of **avahi**. If not, you can install it manually. 

### MAC OS mDNS

MAC OS offers a tool called **dns-sd** to browse mDNS services.

### Windows mDNS

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

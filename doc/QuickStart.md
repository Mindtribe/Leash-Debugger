Doc pages:
* [**Quick-start Guide**](QuickStart.md)
* [**Build Guide**](BuildGuide.md)
* [**User Guide**](UserGuide.md)
* [**Wiring**](Wiring.md)
* [**Report**](Report.md)
* [**Known Issues**](KnownIssues.md)

# Leash Debugger: Quick-start Guide

This guide aims to get you up and running with Leash Debugger in the shortest possible time. For more detailed information, please refer to the other guides.

## Required Hardware

* A Texas Instruments CC3200 Launchpad development board.
* A CC3200 target to debug (this could be another development board, or any CC3200 board with available 4-pin JTAG connections).
* Jumper wires for making the JTAG connections.
* A USB-micro cable.

## 1: Installing Tools and Building

* [Download and install the **arm-none-eabi**](https://launchpad.net/gcc-arm-embedded/+download) toolchain for your operating system.
* Check your version of Ruby ("ruby --version" on the command line). If it is not present, or the version is below v2.x.x, [download and install Ruby](https://www.ruby-lang.org/en/downloads/). Make sure the version being used is v2.x.x by running "ruby --version" again.
* Download and install **MTBuild**, the build tool used for building Leash Debugger. For installing instructions, see [MTBuild's Ruby gem page](https://rubygems.org/gems/mtbuild/).
* If you haven't already, clone (*git clone https://github.com/Mindtribe/Leash-Debugger*) or download the Leash Debugger source.

Now, you should have all the tools necessary to build Leash Debugger:

```
Leash-Debugger$ mtbuild
```

This should result in a folder called **build** being created. If the build succeeded, there should be .bin, .hex and .elf files inside the directories **Leash-Debugger/build/leash/leash/Debug**, **Leash-Debugger/build/leash/testapp/Debug** and **Leash-Debugger/build/leash/cc3200_flashstub/Debug**.

## 2: Flashing Leash Debugger onto the Launchpad

The only tool available for flashing Leash Debugger onto the Launchpad board is **TI Uniflash**. If you are not familiar with the tool yet, follow [this Quick-start guide](http://processors.wiki.ti.com/index.php/CC31xx_%26_CC32xx_UniFlash_Quick_Start_Guide#CC32xx_MCU_image_flashing) to get going.

The Launchpad development board boasts a 1MB external flash.

Before flashing, make sure the SOP2 header is connected and reset the board.

* Use Uniflash to format the flash to 1MB.
* Program **Leash-Debugger/build/leash/leash/Debug/leash-leash-Debug.bin** onto the board as **/sys/mcuimg.bin**.
* Program **Leash-Debugger/build/leasn/testapp/Debug/leash-testapp-Debug.bin** onto the board as **/cc3200_flashstub.bin**. *Note: this is not strictly required, but necessary if you ever want to use Leash Debugger to permanently flash CC3200 targets.*

After the programming has completed, remove the SOP2 header and reset the board. After about a second, LEDs should start flashing on the board - this means Leash Debugger has booted up successfully. Most likely the orange and red LEDs will be flashing, indicating a JTAG error (no target is connected).

## 3: Connecting to / Configuring Leash Debugger

To use Leash Debugger over a WiFi connection, it will need to be connected. There are two ways to do this: 

1. Start Leash Debugger as a WiFi Access Point, then connect to it using your host machine.

2. Configure Leash Debugger to connect to your local network (Station Mode) through an existing WiFi access point, and connect your host machine to the same local network.

Approach 1 requires the extra step of connecting to the AP using your host machine, but has the advantage that no pre-existing WiFi network is required. Both approaches first require us to start Leash Debugger in AP mode. To do this:

* hold down SW3 on the Launchpad while, at the same time, resetting the board. You can let go of SW3 after about a second after you release the reset button. 

After some time, the green LED should be lit, while repeatedly going briefly off (a long-on, short-off flashing pattern). This means Leash Debugger has started in AP mode. If you rescan for WiFi networks using your host machine, there should be an access point called "**LeashDebugger**" in the list. If you want to use Leash Debugger in AP mode, you can move on and skip to step 4. In that case, just make sure to always boot Leash Debugger with SW3 held down, to ensure it always starts in AP mode.

#### 3.1: Station mode

* Connect to the "**LeashDebugger**" access point.
* Use a raw TCP terminal client to connect to IP address 192.168.1.1, port 49152 (the **Log Socket**). On most Linux or MAC systems, "*nc 192.168.1.1 49152*" can be used. On Windows, [PuTTy](http://www.chiark.greenend.org.uk/~sgtatham/putty/) can be used (look in the documentation for "Making raw TCP connections").
You should now see a stream of log data appearing in your terminal.
* In the raw terminal, type **help** and press Return. A list of possible commands should appear.
* To add a network profile to Leash Debugger, type **network** and hit Return. Follow the prompts to configure your network's parameters.

After adding your network profile, reset Leash Debugger *without* holding down SW3. Leash Debugger will now start in Station mode, and attempt to connect to one of the networks in its profile list. If you configured your network properly, after some time the green LED should turn completely ON, indicating Leash Debugger successfully connected to the network.

#### 3.2: Finding Leash Debugger on the network

Now that Leash Debugger is on your local network, you will need to find its IP address. You can use any method you wish for this, including:
* Configuring your network router's DHCP service to always give Leash Debugger a certain IP.
* Going into your network router configuration page and manually looking up the IP given to Leash Debugger.
* Using the fact that Leash Debugger advertises itself on the local network using mDNS. This is the most straightforward as it does not require any router configuration - however, Windows requires some extra steps to get mDNS working. Please see the [**User Guide**](UserGuide.md)'s section on mDNS for instructions.

Once you have found Leash Debugger's IP address, you should be able to connect to it in the same way you did before in AP mode. Just use the IP address given to Leash Debugger, instead of 192.168.1.1.

## 4 Connecting Leash Debugger to a target

To start debugging using Leash Debugger, it should first be wired to a target board which is to be debugged. Please see the [**Wiring**](Wiring.md) document to find the pin assignments and a diagram for wiring it up to another Launchpad board.

* Wire up Leash Debugger to the target.
* Reboot the target with SOP2 connected (this ensures the target hardware is in a debuggable state)
* (Re-)boot Leash Debugger.

The orange LED on Leash Debugger should light up solid (indicating that a target was found on the 4-wire JTAG connection).

## 5 Start Debugging!

Now that all preparations are made, it's time to start an actual debugging session. A program called "**testapp**" is included in the Leash Debugger package, which demonstrates its feature to communicate to the target in printf-style on the GDB command line. On Linux, the easiest way to get going is by using the GDB scripts included in the Leash Debugger package:

* In **Leash-Debugger/scripts/GDB/gdbinit_tcp**, replace the IP address by the IP address used on your Leash Debugger.
* Execute the following command:
```
Leash-Debugger/scripts/GDB/$  arm-none-eabi-gdb -x gdbinit_tcp ./../../build/leash/testapp/leash-testapp-Debug.elf
```
Alternatively, you could use the **debug_testapp** script in **Leash-Debugger/scripts** to do this.
* A GDB session will start. If all goes well, it will report successfully loading code onto the target, and break at testapp's **main()** function. After continuing execution (press 'c' and hit Return), you should get into a cycle where testapp asks you for input, then echoes it back onto the GDB terminal. From here, you can debug using GDB as you normally would.

Note that testapp is able to request user input "through" Leash Debugger, on the GDB command line. This is a special feature of Leash Debugger based on ARM's semihosting protocol, further explained in the [**User Guide**](UserGuide.md).

## 6 (Optional) Integrating Leash Debugger into an IDE

There is much information available on debugging using GDB, connected to an IDE. Usually, this involves a setting somewhere, which invokes GDB with a certain startup script file. A common configuration has GDB connecting to OpenOCD for debugging. In those cases, the GDB script will contain a line similar to:

```
target remote | openocd -c "gdb_port pipe; log_output ~/openocd.log" -f ./OpenOCD/cc3200.cfg
```

In order to make the script suitable for Leash Debugger, this line should be replaced by:

```
target remote tcp:192.168.1.1:49154
```

Make sure to use the IP address your Leash Debugger can be found on. Adapting the script this way should allow you to use GDB from your IDE in the same way you did before using OpenOCD.

## 7 (Optional) Flashing your target using Leash Debugger

Leash Debugger supports a limited set of features to access the target's external flash memory. **Note that to use these features, you must have programmed cc3200_flashstub.bin onto Leash Debugger's flash as outlined in step 2 of this guide.**
It is recommended to read the [**User Guide**](UserGuide.md)'s section on Flash programming if you plan to use this feature. Here, an example is given programming a **blinky** program onto flash and booting it.

* Navigate to **Leash-Debugger/scripts/GDB**.
* Make sure the correct IP address is listed in the **gdbinit_tcp_donothing** script.
* (Optional) In a separate terminal window or TCP client terminal, connect to Leash Debugger's **Log Socket** (port 49152) to see detailed information about proceedings.
* Execute:
```
arm-none-eabi-gdb -x gdbinit_tcp_donothing
```
* GDB should have started a debugging session.
* On the GDB command line, execute the following commands in order:
```
(gdb) monitor flashmode=1
(gdb) remote put ./../blinky/blinky.bin /sys/mcuimg.bin
```
* You should receive confirmation at each of the above commands: first of entering flash mode, then of the successful file transfer.
* (Optional) Read the written file back to another file for verification:
```
(gdb) remote get /sys/mcuimg.bin ./readback.bin
```
You can use a binary comparison tool to check whether readback.bin is equal to ./../blinky/blinky.bin. If they are equal, this confirms the file transfer succeeded without corruption.
* Disconnect or power off Leash Debugger.
* Remove the SOP2 header from the target and reset it.
* The blinky program should now execute from flash, blinking all LEDs one by one.

The flash filesystem has a tendency to get corrupted, making the system unbootable even if GDB reports the flashing was successful. This can happen due to writing files too large for the filesystem or writing too many files. If this happens, unfortunately, you will have to format the external flash using TI's Uniflash tool. Leash Debugger itself is not able to format the external flash.




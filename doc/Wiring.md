Doc pages:
* [**Quick-start Guide**](QuickStart.md)
* [**Build Guide**](BuildGuide.md)
* [**User Guide**](UserGuide.md)
* [**Wiring**](Wiring.md)
* [**Report**](Report.md)
* [**Known Issues**](KnownIssues.md)

# Leash Debugger: Wiring

Leash debugger uses 4-wire JTAG with an optional nRST connection. The wiring connections between Leash Debugger and the target are shown below.

The 4-pin JTAG connections to the CC3200 target device will need to be exposed on your target board for it to work with Leash Debugger. The image below shows the wiring diagram assuming that the target is also a CC3200 LaunchPad. If this is the case, you can remove the four JTAG jumpers and wire the JTAG connections to those pins using jumper wires.

## Which one should I choose?

If you are trying to choose between a Launchpad or WiFi Mini for using as a debugger, here are some pros and cons to each:

* **Perhaps the biggest factor in this choice is that we are still dealing with an unsolved crash issue on the RedBearLab WiFi Mini. Please check [Known Issues](KnownIssues.md) for details.**

* The WiFi Mini's small size and low power consumption make it very convenient for clipping on to the target (see pics below).
* The Launchpad has three LEDs, which show JTAG, WiFi and error state respectively. On the WiFi mini, only one user LED is present (showing WiFi state).
* The Launchpad has an on-board debug interface, meaning that if you want to make changes to Leash Debugger, you can use a debugger to try your changes out. On the WiFi Mini, new firmware can only be programmed directly to flash. This may be relevant if you plan to develop this project further.

## TI CC3200 LaunchPad

- **TDI**: pin 15
- **TDO**: pin 22
- **TMS**: pin 17
- **TCK**: pin 28
- **nRST**: pin 16 (optional)
- **Power**: Connect the grounds of debugger and target together.

*A note about the nRST connection: the Launchpad does not offer a pin that goes directly to the reset signal. In order to make this optional connection, a wire can be soldered to the appropriate pin on the RST button of the target. nRST is only required if you wish to make hard resets of the target.*

Wiring can be done as follows:

![Leash Debugger wiring for CC3200 Launchpad](/doc/images/Wiring_Launchpad.png)
![A configuration using two CC3200 LaunchPad boards.](/doc/images/SetupPic_Launchpad.jpg)

## RedBearLab WiFi Mini

- **TDI**: pin D22
- **TDO**: pin D21
- **TMS**: pin D23
- **TCK**: pin D24
- **AP/Station select**: pin D9. D8 is always LOW, D10 is always HIGH.
- **nRST**: n/c
- **Power**: Ground should be connected to that of the target. WiFi Mini can be powered off its own USB or using a jumper cable to the target.

*Note: the wiring diagram below shows connections as wires. In practice, the easiest is to solder a female pin header onto the WiFi Mini so that it can clip directly onto the target, as in the picture.*

*Note 2: The WiFi Mini does not have a user button. To select AP or Station mode, a jumper is used. Solder a 3-pin male header accross pins D8, D9 and D10. Placing a header which connects D9 to either D10 or D8 will choose AP or Station mode.*

![Leash Debugger wiring for RedBearLab WiFi Mini](/doc/images/Wiring_RBL_WifiMini.png)
![A convenient configuration using a RedBearLab Wifi Mini on a Launchpad target, done using a female pin header and jumper cable for power.](/doc/images/SetupPic_RBL_WifiMini.jpg)








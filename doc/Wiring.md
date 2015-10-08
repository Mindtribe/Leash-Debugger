Doc pages:
* [**Build Guide**](doc/BuildGuide.md)
* [**User Guide**](doc/UserGuide.md)
* [**Wiring**](doc/Wiring.md)
* [**Report**](doc/Report.md)

# Leash Debugger: Wiring

Leash debugger uses 4-wire JTAG with an optional nRST connection. The pin assignments on the Leash Debugger side are as follows:

- **TDI**: pin 15
- **TDO**: pin 22
- **TMS**: pin 17
- **TCK**: pin 28
- **nRST**: pin 16 (optional)
- **GND**: The grounds of debug adapter and target should be connected in some way.

# Target Wiring

The 4-pin JTAG connections to the CC3200 target device will need to be exposed on your target board for it to work with Leash Debugger. The image below shows the wiring diagram assuming that the target is also a CC3200 LaunchPadXL. If this is the case, you can remove the four JTAG jumpers and wire the JTAG connections to those pins using jumper wires.

![Leash Debugger wiring for CC3200 LaunchpadXL](/doc/images/Wiring.png)

A note about the nRST connection: the LaunchpadXL does not offer a pin that goes directly to the reset signal. In order to make this optional connection, a wire can be soldered to the appropriate pin on the RST button of the target. nRST is only required if you wish to make hard resets of the target.


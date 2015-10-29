Doc pages:
* [**Quick-start Guide**](QuickStart.md)
* [**Build Guide**](BuildGuide.md)
* [**User Guide**](UserGuide.md)
* [**Wiring**](Wiring.md)
* [**Report**](Report.md)
* [**Known Issues**](KnownIssues.md)

# Leash Debugger: Compatibility Status

## Texas Instruments CC3200 LaunchPad

These LaunchPad boards come in various versions which host various versions of the chip. 

**Tested and working:**

* LaunchPad with CC3200R1M2 chip and external flash
* LaunchPad with XCC3200HZ chip and external flash

Note that none of the module versions (CC3200MODxxxx) have been tested on, except in combination with the RedBearLab WiFi Mini board (see below).

## RedBearLab WiFi Mini board

**WARNING: SUPPORT FOR THIS BOARD HAS ISSUES!**

Code has been adapted to support this board. It uses a CC3200MODR1M2 chip, which should for all practical purposes perform equal to the CC3200R1M2 on the LaunchPad which works.
However, random crashes were experienced when trying to use this board, usually during periods of high WiFi communications volume.

Testing has only been done on one specific board, so there is a possibility that it's a board failure. Figuring out the root cause is complicated by the fact that this board does not have access to debug support.

*If you have a RedBearLab WiFi Mini or a Launchpad with a CC3200MODR1M2 on it, any help in reproducing and/or fixing this issue would be greatly appreciated!*


# Leash Debugger: Known Issues

**Note: This list is supposed to be up-to-date, but just in case, it is best to check the latest issues in the GitHub issues page for the project.**

* **JTAG Sticky Errors:** Every once in a while, a JTAG transaction may fail. A certain class of JTAG failures called **Sticky errors** are not recovered from properly by Leash Debugger - in that situation, it needs to be reset.

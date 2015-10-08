Doc pages:
* [**Build Guide**](doc/BuildGuide.md)
* [**User Guide**](doc/UserGuide.md)
* [**Wiring**](doc/Wiring.md)
* [**Report**](doc/Report.md)
* [**Known Issues**](doc/KnownIssues.md)

# Leash Debugger: Known Issues

**Note: This list is supposed to be up-to-date, but just in case, it is best to check the latest issues in the GitHub issues page for the project.**

* **Socket Disconnections:** A failure is encountered when any client disconnects from a socket. This includes GDB and raw TCP terminals listening to log messages. Leash Debugger needs to be reset in this situation.
* **JTAG Sticky Errors:** Every once in a while, a JTAG transaction may fail. A certain class of JTAG failures called **Sticky errors** are not recovered from properly by Leash Debugger - in that situation, it needs to be reset.
* **Heap Overflow:** In some cases, a bug has been encountered causing the heap to overflow.
* **Warm Reset:** By default, Leash Debugger is set to issue a ``warm reset'' upon connecting to the target: this resets everything except JTAG logic inside the target device. This feature does not seem to work properly (the target is not reset). In most cases, this does not matter for debugging purposes - however a hard reset may be preferable for the time being.

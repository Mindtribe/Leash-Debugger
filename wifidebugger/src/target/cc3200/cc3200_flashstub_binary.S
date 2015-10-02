.section .cc3200_flashstub_binary
.global CC3200_FLASHSTUB_BINARY

CC3200_FLASHSTUB_BINARY:
.incbin "build/wifidebugger/cc3200_flashstub/Debug/wifidebugger-cc3200_flashstub-Debug.bin"

# Xtensa-llvm

This is a preliminary and restricted version of LLVM backend for Tensilica LX6
processor family. The most well-known commercial processors in this femity are
ESP32 from Espressif (more simple ESP8266/ESP8266EX uses a subset of LX 
instructions).

LLVM vesrion - 3.8 or higher.

Restrictions in this version:

No register windows.

No machine-dependent 64-bit instructions (mostly library functions).

No hardware floating-point instructions (only FP library functions).

For full version ask

Andrei Safronov   andrei@kudeyar.com
Alexey Shistko    alexey@kudeyar.com

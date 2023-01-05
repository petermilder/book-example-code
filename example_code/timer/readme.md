# Simple AXI Timer Example Code

This directory contains example code for using the AXI Timer module to measure the runtime of your program.

* `petalinux.c` gives an example you can use with PetaLinux
* 'baremetal.c` gives an example for running bare metal

Both make assumptions of your timer's base address and clock frequency. Please double check the 
macros `TIMER_BASE` and `TIMER_FREQ` and adjust as necessary.

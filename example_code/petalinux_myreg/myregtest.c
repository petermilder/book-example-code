/*   
    Example code to test the "myreg" IP, an example AXI4-Lite peripheral,
    in PetaLinux
                    
    From "Getting Started with the Xilinx Zynq FPGA and Vivado" 
    by Peter Milder (peter.milder@stonybrook.edu)

    Copyright (C) 2018 Peter Milder

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>    // for close()
#include <fcntl.h>     // for open()
#include <sys/mman.h>  // for mmap()


#define MYREGBASE 0x43C10000 // change this if your base address is different
#define MYREGSIZE 4096       // minimum size to MMAP is 4096; we only need to use 32 bytes of this

#define GPIOBASE 0x41200000  // change this if the base address of your GPIO (LEDs) is different

int main(int argc, char **argv) {
    
    // Open /dev/mem into file descriptor memfd
    int memfd;
    memfd = open("/dev/mem", O_RDWR | O_SYNC);
    if (memfd == -1) {
        printf("ERROR: failed to open /dev/mem.\n");
        exit(0);
    }

    // base address of your IP:
    off_t base_addr = MYREGBASE;

    // mmap the base address to myreg and check
    int *myreg = (int*) mmap(NULL, MYREGSIZE, PROT_READ | PROT_WRITE, MAP_SHARED, memfd, base_addr);
    if (myreg == (void*) -1) {
        printf("ERROR: Failed to mmap the IP base address");
        if (memfd >= 0) 
            close(memfd);        
        exit(0);
    }
    
    // We are interacting with myreg by first, mapping it to be an integer poiner with ((int*)myreg).
    // Then, we can index it as an array, so setting or reading ((int*)myreg)[0] is the same as
    // writing/reading to the first register, and so on.
    
    myreg[0] = 27;
    myreg[1] = 42;

    // Now, read the result from all 8 registers
    for (int i=0; i<8; i++) {
        int x = myreg[i];
        printf("%d: 0x%08x = %d\n", i, x, x); // Printing result in hex and decimal
    }
    
    // We are done with this mapping, so we unmap it.
    munmap(myreg, MYREGSIZE);

    // -------------------------------------------------------------------------------
    // Another small example: toggle LEDs
    // This code mmaps the base address of the GPIO controller connected to the LEDs
    // Then it gets the current values, and toggles them.

    // Base address of GPIO for LEDs
    off_t gpio_base_addr = GPIOBASE;

    // mmap the base address of GPIO and check
    void *leds = mmap(NULL, MYREGSIZE, PROT_READ | PROT_WRITE, MAP_SHARED, memfd, gpio_base_addr);
    if (leds == (void*) -1) {
        printf("ERROR: Failed to mmap the IP base address");
        if (memfd >= 0) 
            close(memfd);        
        exit(0);
    }
    
    // Get the current LED values. 
    // Previously we interacted with the device by using array indexing.
    // This demonstrates a different (but closely related) style. First, we remember that
    // the variable leds is a pointer to the base address of the IP. 

    // So, we can just cast that pointer to the datatype we want (here unsigned int). 
    // So now, variable led0 will be a pointer to the base address.
    unsigned int *led0 = (unsigned int *)(leds);

    // Then, we also remember that the second output (the other LED) is at the
    // base address+8. So, we can add 8 to the leds pointer, and then cast it:
    unsigned int *led1 = (unsigned int *)(leds+8);

    // Now, led1 is a pointer to the control register for the second LED.
    
    // To read from these registers, we just need to dereference the pointers:
    int led0Val = *(led0);
    int led1Val = *(led1);

    // And if we want to write to them, we can dereference the pointer and assign.
    // Here we are assigning the complement of the value we previously read:
    *(led0) = ~led0Val;
    *(led1) = ~led1Val;

    // You should notice that both LEDs toggled their values.

    // We are done with this mapping, so we unmap it.
    munmap(leds, MYREGSIZE);
    
    // --------------------------- Cleanup --------------------------

    // We are done with the /dev/mem file descriptor, so we close it.
    close(memfd);

    return 0;
}

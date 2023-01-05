/*   
    Example software demonstrating basic operation of AXI Timer
                    
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

// This is a small standalone example of how to use the AXI timer in a PetaLinux project

// It assumes that you have added an AXI Timer to your project, at 0x42800000. (If
// necessary, adjust the TIMER_BASE macro below.

// It also assumes the AXI Timer is clocked at 100 MHz. If your timer's clock
// frequency has changed, adjust the TIMER_FREQ macro below

#define TIMER_BASE 0x42800000
#define TIMER_FREQ 100         // in MHz

#include <stdlib.h>
#include <stdio.h>  
#include <fcntl.h>    // file operations (open, close)
#include <sys/mman.h> // memory management (mmap, munmap)

int main(int argc, char **argv) {

    //////////////////////////////////////////////////
    // open and mmap the timer
    int mem_fd = open("/dev/mem", O_RDWR | O_SYNC);
    
    if (mem_fd == -1) {
        printf("ERROR: Could not open /dev/mem\n");
        return -1;
    }

    volatile unsigned int *timer = (unsigned int*)mmap(NULL, 64, PROT_READ | PROT_WRITE, MAP_SHARED, mem_fd, TIMER_BASE);
    
    if (timer == NULL) {
        printf("ERROR: Could not mmap timer");
        return -1;
    }

    //////////////////////////////////////////////////
    // clear the timer; take the first reading; start the timer
    timer[0] = 0x20;                      // clear the timer
    unsigned int time0 = timer[2];        // read the time 
    timer[0] = 0x80;                      // start the timer


    // now, perform whatever operations you want to time


    // just to waste time for our example...
    sleep(1); // sleep for one second
 
    unsigned int time1 = timer[2];       // read the current time

    if (time1 == 0)
        printf("ERROR: Timer reported 0 cycles elapsed. This is either due to a configuration error or the time you measured was over 2^32 cycles == 42.94 seconds\n");
    
    printf("Measured %d clock cycles  == %g seconds\n", (time1-time0), ((double)(time1-time0))/(TIMER_FREQ*1000000));

    munmap((void*)timer, 64);
    close(mem_fd);

    return 0;
}



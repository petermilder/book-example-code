/*   
    Test program for "BRAM Reverse" IP. 

    From "Getting Started with the Xilinx Zynq FPGA and Vivado" 
    by Peter Milder (peter.milder@stonybrook.edu)

    Copyright (C) 2020 Peter Milder

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
#include "platform.h"
#include "xil_printf.h"

int main() {
    init_platform();

    // Pointers to our BRAM and the control interface of our custom hardware (hw)
    volatile unsigned int* bram = (unsigned int*)XPAR_AXI_BRAM_CTRL_0_S_AXI_BASEADDR;
    volatile unsigned int* hw = (unsigned int*)XPAR_BRAM_REVERSE_0_S00_AXI_BASEADDR;

    // So, hw[0] will access the first register of our IP, which is ps_control,
    // and hw[1] will access the second register: pl_status


    // First, test that we can read and write everywhere
    for (int i=0; i<4096; i++) {
    	bram[i] = 4000+i;
    }

    int errors=0;

    for (int i=4095; i>=0; i--) {
    	if (bram[i] != 4000+i) {
    		xil_printf("ERROR. Expected bram[%d] = %d, result was %d\r\n", i, 4000+i, bram[i]);
    		errors++;
    	}
    }

    if (!errors)
    	xil_printf("Read/write test successful\r\n");

    // Now test that the hw design works
    for (int i=0; i<2048; i++) {
        bram[i] = i+1;
    }

    // Assert start signal
    hw[0] = 1;

    // Wait for done signal
    while ( (hw[1] & 0x1) == 0) {
        ;
    }

    // Deassert start signal
    hw[0] = 0;

    errors=0;

    for (int i=0; i<2048; i++) {
        if (bram[2048+i] != 2047-i+1) {
            xil_printf("ERROR: bram[%d] = %d; expected %d\r\n", 2048+i, bram[2048+i], 2047-i+1);
            errors++;
        }
    }

    if (errors == 0)
    	xil_printf("Reverse test successful\r\n");

    while ( (hw[1] & 0x1) != 0) {
        ;
    }


    print("-------------- Done ------------\r\n\n\n\n");

    cleanup_platform();
    return 0;
}

/*   
    A simple test of BRAM read/write 

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
#include "platform.h"
#include "xil_printf.h"

int main() {
    init_platform();

    xil_printf("-------------- Starting Simple BRAM Read/Write Test ------------\n\r");

    // Pointer to our BRAM.
    volatile int* bram = (int*)XPAR_AXI_BRAM_CTRL_0_S_AXI_BASEADDR;
    // Since we declared this as a in integer pointer, we can now read and write from this block RAM
    // as bram[0], etc.

    // Let's write some basic test data here
    for (int i=0; i<2048; i++) {
        bram[i] = 0x70000000 + i;
    }

    xil_printf("Wrote data to BRAM. Now reading it back.\r\n");

    int errors = 0;

    for (int i=0; i<2048; i++) {
        if (bram[i] != (0x70000000 + i)) {
            xil_printf("ERROR: Expected bram[%d] = %x but instead got %x\r\n", i, 0x70000000+i, bram[i]);
            errors++;
        }
    }

    xil_printf("%d errors\r\n", errors);

    xil_printf("-------------- Done ------------\n\r");

    cleanup_platform();
    return 0;
}


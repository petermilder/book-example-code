/*
    Test program for "BRAM Max Value" IP.

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

    int randomtests = 1000;

    // Pointers to our BRAM and the control interface of our custom hardware (hw)
    volatile unsigned int* bram = (unsigned int*)XPAR_AXI_BRAM_CTRL_0_S_AXI_BASEADDR;
    volatile unsigned int* hw = (unsigned int*)XPAR_BRAM_INT_MAX_VAL_0_S00_AXI_BASEADDR;

    // So, hw[0] will access the first register of our IP, which is ps_control,
    // and hw[1] will access the second register: pl_status

    print("-------------- Starting BRAM Test ------------\n\r");

    xil_printf("Test 0: Testing read/write of memory\r\n");
    for (int i=0; i<2047; i++) {
    	bram[i] = i;
    }

    int errors=0;
    for (int i=0; i<2047; i++) {
    	if (bram[i] != i) {
    		errors++;
    		xil_printf("ERROR: expected bram[%d] = %d, but got %d\r\n", i, i, bram[i]);
    	}
    }
    if (errors == 0)
    	xil_printf("SUCCESS: Read/write test passed\r\n");



    xil_printf("Test 1: Testing maximum value in last location\r\n");

    // The idea here is to make sure that our maxval system is correctly
    // checking *all* 2048 words. So, we can test this by:
    //     - writing the max value 0xffff into the last location in memory
    //     - making sure that none of the other words in memory are that large

    for (int i=0; i<2047; i++) {
        bram[i] = 0;
    }
    bram[2047] = 0xffffffff;

    // Assert start signal
    hw[0] = 1;

    // Wait for done signal
    while ( (hw[1] & 0x1) == 0) {
        ;
    }

    // Deassert start signal
    hw[0] = 0;


    if (bram[0] != 0xffffffff) {
        xil_printf("ERROR: bram[0] = 0x%x; expected 0x%x\r\n", bram[0], 0xffffffff);
    }
    else{
        xil_printf("SUCCESS: bram[0] = 0x%x; expected 0x%x\r\n", bram[0], 0xffffffff);
    }

    // Now, get ready for test 2. To make sure our IP is ready for a new input,
    // we need to wait until it sets pl_status back to 0:
    while ( (hw[1] & 0x1) != 0) {
        ;
    }


    xil_printf("Tests 2 through %d: Psuedorandom input\r\n", randomtests+1);

    // ---------------------------------------------
    // Use a linear feedback shift register to
    // generate a pseudorandom input sequence.
    // Keep track of the largest value seen.

    unsigned int v = 12347;   // start with v = any number except 0
    unsigned int feed = 0x8c000001; // Char. polynomial x^32 + x^28 + x^27 + x^1 + 1


    errors=0;


    for (int test=0; test < randomtests; test++) {
        unsigned int largest = 0;

        // Store 2048 pseudorandom numbers to BRAM. Keep track of largest
        // (this is our expected value for bram[0]).
        for (int i=0; i<2048; i++) {
            // Update LFSR
            v = (v&1) ? (v>>1)^feed : v>>1;

            // Store pseudorandom number in BRAM
            bram[i] = v;

            if (v > largest) {
                largest = v;
            }
        }

        // ---------------------------------
        // Tell the hardware to start processing by setting ps_control = 1
        hw[0] = 1;

        // Wait until our IP sets pl_status == 1
        while ( (hw[1] & 0x1) == 0) {
            ;
        }

        // Now, set our control signal back to 0.
        hw[0] = 0;

        // Check that bram[0] is equal to the largest value we found
        if (bram[0] != largest) {
            errors++;
            xil_printf("ERROR: bram[0] = %u; expected %u\r\n", bram[0], largest);
        }

        while ( (hw[1] & 0x1) != 0)
            ;
    }

    if (errors == 0)
        xil_printf("SUCCESS: Completed %d tests. No errors detected.\r\n", randomtests);

    print("-------------- Done ------------\r\n\n\n\n");

    cleanup_platform();
    return 0;
}

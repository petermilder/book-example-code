/*   
    Test program to demonstrate DMA driver, with DMA loopback
                                    
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

// This is a small standalone DMA test program, demonstrating how to use the DMA driver (dma.c and .h).
// See comments in those files also.

// Assumptions:
//    1. The DMA module's base address is 0x40400000. If this does not match, change the
//       DMA_BASE macro in dma.c.
//    2. Your system uses an AXI DMA module configured in a loopback
//    3. The DMA is configured to have a 14 bit length register
//    4. The DMA is configured to run in "simple mode" (not scatter/gather)
//    5. You are using the dmabuffer kernel module and you have inserted it with modprobe dmabuffer
//    6. Your dmabuffer module is confiugred to have a buffer of size 2^20. (If this is not true,
//       then change macro DMABUFFER_LEN in dma.h to match the actual buffer length.)

// This test will:
//    - initialize the buffers and DMA
//    - write test data into the Tx buffer
//    - run the DMA
//    - check that the Rx buffer matches


#include <stdio.h>
#include <fcntl.h>  // file operations
#include <stdlib.h>
#include <sys/mman.h>
#include <unistd.h>
#include "dma.h"

int main(int argc, char **argv) {
    int txsize; // number of integers to test
    if (argc == 2) 
        txsize = atoi(argv[1]);
    else
        txsize = 16;

    // Step 1: Initialize the DMA driver. If its return value != 0, there was an error.
    int res = dma_init(txsize*sizeof(int));
    if (res != 0) 
        return res;

    // Step 2: Get pointers to the tx and rx buffers from DMA driver. Cast them
    // to whatever datatype you need for your application (here int pointers)
    int* txbase = (int*) getTxBuffer();
    int* rxbase = (int*) getRxBuffer();

    if ((txbase == NULL) || (rxbase == NULL)) {
        printf("ERROR: Null pointer to txbase/rxbase.");
    }
        

    // Step 3: Write data into Tx buffer. Clear Rx buffer if you want.
    for (int i=0; i<txsize; i++) {
        // Location 0 would have value 0x70000000.
        // Location 1 would have 0x70000001, etc.
        txbase[i] = 0x70000000 + i;
        rxbase[i] = 0; // Just clearing rxbase; not strictly needed but
                       // this lets us test that new data comes in
    }

    // Step 4: Reset the DMA. As long as it is working without errors, you don't need to do this before
    // every transfer.
    dma_reset();

    // Step 5: Set up the DMA's Rx and Tx configurations by telling the DMA driver the length of 
    // each transfer in bytes. Here we are transferring txsize ints.
    res = dma_rx(txsize*sizeof(int));
    if (res != 0) {
        return res;
    }

    res = dma_tx(txsize*sizeof(int));
    if (res != 0) {
        return res;
    }
    
    // Step 6: Call dma_sync() to wait until all DMA transfers are complete.
    res = dma_sync();
    if (res != 0) {
        return res;
    }
 
    // Step 7: Your data is now in the RX buffer. Here, we check it for correct 
    // values:
    int errors=0;
    for (int i=0; i<txsize; i++) {
        if (txbase[i] != rxbase[i]) {
            errors++;
            printf("Error on word %d: Expected 0x%x, received 0x%x\r\n", i, txbase[i], rxbase[i]);
        }
    }
    if (errors)
        printf("%d errors\r\n", errors);
    else
        printf("All data (%d ints) received successfully.\r\n", txsize);

    // Step 8: Call the dma_cleanup() function when done using the DMA.
    dma_cleanup();
    return 0;
}



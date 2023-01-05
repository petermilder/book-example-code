/*   
    Test program to test streammult multiplier system
                    
    From Section 8.5 of "Getting Started with the Xilinx Zynq 
    FPGA and Vivado" by Peter Milder (peter.milder@stonybrook.edu)

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

// For comments regarding the use of DMA, please see example_code/dma_loopback/dma_loopback.c

#include <stdio.h>
#include "xaxidma.h"
#include "platform.h"
#include "xil_printf.h"

#define TXSIZE 2048 // in integers
// Requirements on the size: TXSIZE must be <= 4088 and must be divisible by 8.
// (See dma_loopback/dma_loopback.c for explanation)

int main() {

    init_platform();

    xil_printf("-----------------------------------\r\n");
    xil_printf("Starting custom AXI4-stream multiplier test\r\n");

    // For documentation of all XAxiDma_* functions, please see xaxidma.h,
    // which should be in the BSP

    // Setup the DMA config data structure; XPAR_AXIDMA_0_DEVICE_ID is
    // set in xparameters.h in the BSP
    XAxiDma_Config *dma_cfg;
    dma_cfg = XAxiDma_LookupConfig(XPAR_AXIDMA_0_DEVICE_ID);
    if (!dma_cfg) {
        xil_printf("ERROR: Cannot find configuration for device %d\r\n", XPAR_AXIDMA_0_DEVICE_ID);
        return XST_FAILURE;
    }


    // Initialize the DMA instance struct
    XAxiDma dma; // Struct that holds DMA instance data
    int status = XAxiDma_CfgInitialize(&dma, dma_cfg);
    if (status != XST_SUCCESS) {
        xil_printf("ERROR: Initialization failed\r\n");
        return XST_FAILURE;
    }

    // Disable interrupts because we will not use them in this example
    XAxiDma_IntrDisable(&dma, XAXIDMA_IRQ_ALL_MASK, XAXIDMA_DEVICE_TO_DMA);
    XAxiDma_IntrDisable(&dma, XAXIDMA_IRQ_ALL_MASK, XAXIDMA_DMA_TO_DEVICE);

    // Set up the transmit buffer and put some test data into it.
    // Since we configured our FIFO and DMA to have a data width of 32 bits,
    // we can treat the I/O data as integers.
    int TxBuff[TXSIZE]__attribute__((aligned(32)));
    int RxBuff[TXSIZE]__attribute__((aligned(32)));
    int expected[TXSIZE];

    // For each number, we will set two shorts (16 bit values)
    // Then we will shift one and OR them together to make
    // a 32 bit word to send to our multiplier.
    // We will also compute the expected output value
    for (int i=0; i<TXSIZE; i++) {
        short a = 100+i;
        short b = -500-i;
        TxBuff[i] = a<<16 | (b&0xffff);
        expected[i] = (int)a * (int)b;
    }


    // One potential problem: we need to make sure that the data we stored
    // in does not just sit in the cache -- we will flush that
    // cache range to make sure it is written back to DRAM.
    // This is requried because the PL can read data from the DRAM, but not
    // from the CPU's cache.
    Xil_DCacheFlushRange((UINTPTR)TxBuff, TXSIZE*sizeof(int));
    Xil_DCacheFlushRange((UINTPTR)RxBuff, TXSIZE*sizeof(int));

    // Configure the DMA to perform a simple transfer from the
    // device to memory consisting of TXSIE*4 bytes, placing the
    // results in memory starting at RxBuff.
    status = XAxiDma_SimpleTransfer(&dma, (UINTPTR)RxBuff, TXSIZE*sizeof(int), XAXIDMA_DEVICE_TO_DMA);
    if (status != XST_SUCCESS) {
        xil_printf("ERROR: Setting up Rx failed.\r\n");
        return XST_FAILURE;
    }

    // Now, we can set up the DMA to transfer TXSIZE*4 bytes starting
    // from TxBuff.
    status = XAxiDma_SimpleTransfer(&dma, (UINTPTR)TxBuff, TXSIZE*sizeof(int), XAXIDMA_DMA_TO_DEVICE);
    if (status != XST_SUCCESS) {
        xil_printf("ERROR: Setting up Tx failed.\r\n");
        return XST_FAILURE;
    }

    // Now, because we are in polling mode, we will use a while loop that
    // will iterate until neither the Tx or Rx channels are busy
    while (XAxiDma_Busy(&dma, XAXIDMA_DEVICE_TO_DMA) || XAxiDma_Busy(&dma, XAXIDMA_DMA_TO_DEVICE)) {
        ;
    }

    // Next, we will invalidate all addresses in the range of the RxBuff. This is needed
    // to ensure that when we read the data from the RxBuff, it is reading the newly-written
    // data from DRAM, and not data already stored in the cache.
    Xil_DCacheInvalidateRange((UINTPTR)RxBuff, TXSIZE*sizeof(int));

    xil_printf("Checking received data\r\n");

    int errors=0;
    for (int i=0; i<TXSIZE; i++) {
        if (expected[i] != RxBuff[i]) {
            errors++;
            xil_printf("Error on word %d: Expected %d = 0x%x, received %d = 0x%x\r\n", i, expected[i], expected[i], RxBuff[i], RxBuff[i]);
        }
    }

    if (errors)
        xil_printf("%d errors\r\n", errors);
    else
        xil_printf("All %d data received successfully.\r\n", TXSIZE);

    cleanup_platform();
    return 0;
}

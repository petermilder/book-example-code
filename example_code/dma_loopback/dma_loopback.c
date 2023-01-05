/*   
    Small example and test for the DMA loopback system.
        
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
#include "xaxidma.h"
#include "platform.h"
#include "xil_printf.h"

#define TXSIZE (4088)    // in integers
// Requirements on the size:
//   1. By default, the DMA controller's buffer length is 14 bits, meaning it can
//      work with DMAs of length < (2^14) bytes = 2^12 integers
//   2. The ZYNQ's cache lines are each 32 bytes (which holds 8 integers). To work properly, the
//      buffers you allocate for the DMA must be a multiple of 8 integers in length.
//
//   So, this code will work if you set TXSIZE to any integer <= 4088 that is a multiple of 8.
//   If your application would benefit from DMAs larger than this, you can simply increase the
//   width of the max buffer length register (which you can configure by double-clicking on
//   the DMA block in your block diagram.)


int main() {

    init_platform();

    xil_printf("-----------------------------------\r\n");
    xil_printf("Starting loopback test\r\n");

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

    // Here you can see that we are using Xilinx-provided functions to
    // "talk" to the DMA. Alternatively, you could choose to do all of
    // these operations simply by reading and writing to the AXI4-Lite
    // control/status registers on the DMA module. To see an example
    // of this, please see the testbench tb.sv. 


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
    // An important consideration here is cache alignment. The Tx and Rx buffers
    // must start at the beginning of a cache line. The ZYNQ's cache lines are
    // 32 bytes, so we use the aligned(32) attribute here. This ensures that
    // the starting address of each array is divisible by 32 and sits at
    // the beginning of a cache line.
    int TxBuff[TXSIZE]__attribute__((aligned(32)));
    int RxBuff[TXSIZE]__attribute__((aligned(32)));

    // Let's transmit TXSIZE ints. This macro and its explanation are above.
    for (int i=0; i<TXSIZE; i++) {
        // So location 0 would have value 0x70000000.
        // Location 1 would have 0x70000001, etc.
        TxBuff[i] = 0x70000000 + i;
        RxBuff[i] = 0; // Just clearing RxBuff; not strictly needed but
        // this lets us test that new data comes in every time
    }

    // One potential problem: we need to make sure that the data we stored
    // in does not just sit in the cache -- we will flush that
    // cache range to make sure it is written back to DRAM.
    // This is requried because the PL can read data from the DRAM, but not
    // from the CPU's cache.
    Xil_DCacheFlushRange((UINTPTR)TxBuff, TXSIZE*sizeof(int));
    Xil_DCacheFlushRange((UINTPTR)RxBuff, TXSIZE*sizeof(int));


    // Before we transmit, we need to set the DMA up to receive.
    // This may feel counter-intuitive, but the idea is that the
    // DMA needs to know what to do with the data it gets from the
    // FIFO *before* that data gets there. So, we will set up the
    // receive-path, then set up the transmit.

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

    // Now, we just need to check that the data in RxBuff matches the
    // data in TxBuff
    xil_printf("Checking received data\r\n");
    int errors=0;
    for (int i=0; i<TXSIZE; i++) {
        if (TxBuff[i] != RxBuff[i]) {
            errors++;
            xil_printf("Error on word %d: Expected 0x%x, received 0x%x\r\n", i, TxBuff[i], RxBuff[i]);
        }
    }
    if (errors)
        xil_printf("%d errors\r\n", errors);
    else
        xil_printf("All data (%d ints) received successfully.\r\n", TXSIZE);

    xil_printf("-----------------------------------\r\n");
    cleanup_platform();
    return 0;
}

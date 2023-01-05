/*   
    A program to test the AXI CORDIC and AXI DMA module.
    
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

#define TXSIZE (8)    // in integers
// Requirements on the size: TXSIZE must be <= 4088 and must be divisible by 8.
// (See dma_loopback/dma_loopback.c for explanation)

// The CORDIC is configured as follows:
//    Function:        Sin and Cos
//    Arch. Confi.:   Parallel
//    Pipelining Mode: Maximum
//    Phase Format:    Radians
//    Input Width:     32 bits
//    Output Width:    16 bits (this is 16 bits each for cos and sin)
//    Adv. Config:     [Defaults, including Coarse Rotation]
// AXI Stream Options:
//    Phase Channel:   Has TLAST
//    Flow Control:    Blocking
//    Output has TREADY: Selected
//    Output TLAST Behavior: Pass Phase TLAST

// The DMA is configured in "Simple mode"

// Data representation:
// The phase uses number format XXX.XX... (3 bits integer, 29 bits
//                                                  fraction)
//          This means it can hold a number between -4 and almost 4.
//          To represent number N compute: round(N * 2^29). This will give
//          a number of up to 32 bits, which can be transmitted to the CORDIC.
//          For example, to represent pi: round(pi * 2^29) = 1686629713.

// The output (cosine and sine) are 16 bits each, in format XX.XX... (2 bits
//          integer, 14 bits fraction). This allow it to hold a number between
//          -2 and almost 2. If it outputs a number N, to convert it to the real
//          number equivalent, multiply N * 2^(-14).
//          For example if, the system outputs 11585, this is equivalent to
//          11585 * 2^(-14) = 0.7070922852 (approx. sqrt(2)/2).


int main()
{
    xil_printf("Starting CORDIC test\r\n");

    init_platform();

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
    // An important consideration here is cache alignment. The Tx and Rx buffers
    // must start at the beginning of a cache line. The ZYNQ's cache lines are
    // 32 bytes, so we use the aligned(32) attribute here. This ensures that
    // the starting address of each array is divisible by 32 and sits at
    // the beginning of a cache line.
    int TxBuff[TXSIZE]__attribute__((aligned(32)));
    int RxBuff[TXSIZE]__attribute__((aligned(32)));


    TxBuff[0] = 0x00000000;  // == 0 (computed as 0*2^29)
    TxBuff[1] = 0x20000000;  // == 1
    TxBuff[2] = 0xe0000000;  // == -1
    TxBuff[3] = 843314857;   // ==  pi/2 [computed as  (pi/2)*2^29]
    TxBuff[4] = -843314857;  // == -pi/2 [computed as (-pi/2)*2^29]
    TxBuff[5] = 421657428;   // ==  pi/4 [computed as  (pi/4)*2^29]
    TxBuff[6] = -421657428;  // == -pi/4 [computed as (-pi/4)*2^29]
    TxBuff[7] = 281104952;   // ==  pi/6 [computed as  (pi/6)*2^29]
    

    // One potential problem: we need to make sure that the data we stored
    // in TxBuff does not just sit in the cache -- we will flush that
    // cache range to make sure it is written back to DRAM.
    Xil_DCacheFlushRange((UINTPTR)TxBuff, TXSIZE*sizeof(int));
    Xil_DCacheFlushRange((UINTPTR)RxBuff, TXSIZE*sizeof(int));


    // Before we transmit, we need to set the DMA up to receive.
    // This may feel counter-intuitive, but the idea is that the
    // DMA needs to know what to do with the data it gets from the
    // FIFO *before* that data gets there. So, we will set up the
    // receive-path, then set up the transmit.

    // Configure the DMA to perform a simple transfer from the
    // device to memory consisting of 128*4 bytes, placing the
    // results in memory starting at RxBuff.
    status = XAxiDma_SimpleTransfer(&dma, (UINTPTR)RxBuff, TXSIZE*sizeof(int), XAXIDMA_DEVICE_TO_DMA);
    if (status != XST_SUCCESS) {
        xil_printf("ERROR: Setting up Rx failed.\r\n");
        return XST_FAILURE;
    }

    // Now, we can set up the DMA to transfer 128*4 bytes starting
    // from TxBuff.
    status = XAxiDma_SimpleTransfer(&dma, (UINTPTR)TxBuff, TXSIZE*sizeof(int), XAXIDMA_DMA_TO_DEVICE);
    if (status != XST_SUCCESS) {
        xil_printf("ERROR: Setting up Tx failed.\r\n");
        return XST_FAILURE;
    }

    // Now, because we are in polling mode, we will use a whole loop that
    // will iterate until neither the Tx or Rx channels are busy
    while (XAxiDma_Busy(&dma, XAXIDMA_DEVICE_TO_DMA) || XAxiDma_Busy(&dma, XAXIDMA_DMA_TO_DEVICE)) {
        ;
    }


    // Next, we will invalidate all addresses in the range of the RxBuff. This is needed
    // to ensure that when we read the data from the RxBuff, it is reading the newly-written
    // data from DRAM, and not data already stored in the cache.
    Xil_DCacheInvalidateRange((UINTPTR)RxBuff, TXSIZE*sizeof(int));


    xil_printf("Displaying received data\r\n");

    // In each loop iteration, let's read 4 bytes, then mask out the
    // 16 LSBs (which represent the cos) and the 16 MSBs (which represent
    // the sin).
    for(int Index = 0; Index < TXSIZE; Index++) {

      u32 readValTmp = (u32)RxBuff[Index]; //Read 32 bits
      short outCos = (short)(readValTmp&0xffff); // Mask out 16 MSBs and cast as short
      short outSin = (short)(readValTmp>>16);    // Mask out 16 LSBs and cast as short

      int angle = (int)TxBuff[Index]; // Get angle we used as input (just for printing)

      // Convert to floating point so we can display nicely.
      float realCos = (float)(outCos) / (float)(1<<14);
      float realSin = (float)(outSin) / (float)(1<<14);
      float realAngle = (float)angle / (float)(1<<29);

      // Print the cosine, sine, and angle
      printf("cos(%9f) = %f\t sin(%9f) = %9f\r\n", realAngle, realCos, realAngle, realSin);

    }
    xil_printf("\r\n");

    cleanup_platform();
    return 0;
}

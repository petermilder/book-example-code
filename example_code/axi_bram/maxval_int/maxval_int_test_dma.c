/*   
    Test program for "BRAM Max Value" IP with DMA

    From "Getting Started with the Xilinx Zynq FPGA and Vivado" 
    by Peter Milder (peter.milder@stonybrook.edu)

    Some code based on Xilinx example code:
    https://github.com/Xilinx/embeddedsw/blob/master/XilinxProcessorIPLib/drivers/dmaps/examples/xdmaps_example_w_intr.c

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

#include "xscugic.h" // interrupt handler
#include "xdmaps.h"  // DMA driver

#define TXSIZE 2048

#define TMRCTR_DEVICE_ID        XPAR_TMRCTR_0_DEVICE_ID
#define TIMEOUT_LIMIT           0x2000  

int SetupInterruptSystem(XScuGic *GicPtr, XDmaPs *DmaPtr);
void DmaDoneHandler(unsigned int Channel, XDmaPs_Cmd *DmaCmd, void *CallbackRef);
int checkCorrectness(int size, int* src, int* dst);

XDmaPs DmaInstance;
XScuGic GicInstance;

int main() {
    init_platform();

    xil_printf("Testing read/write of memory using DMA\r\n");

    // Pointer to our BRAM.
    volatile int* bram = (int*)XPAR_AXI_BRAM_CTRL_0_S_AXI_BASEADDR;
    // Since we declared this as a in integer pointer, we can now read and write from this block RAM
    // as bram[0], etc.

    // DRAM buffers to read/write data
    int txBuff[TXSIZE]__attribute__((aligned(4096)));

    // Let's initialize the DRAM buffers and BRAM before our test
    for (int i=0; i<TXSIZE; i++) {
        txBuff[i] = i;
        rxBuff[i] = 0;
        bram[i] = 0;
    }
    txBuff[TXSIZE-1] = 0xFFFFFFFF;

    // First, we will use the PS DMA to transfer data from txBuff to bram.
    volatile int* txDone = malloc(sizeof(int));
    *txDone = 0;

    // Configure DMA and its driver
    u16 DeviceId = XPAR_XDMAPS_1_DEVICE_ID;
    XDmaPs_Config *DmaCfg;
    XDmaPs *DmaInst = &DmaInstance;
    XDmaPs_Cmd DmaCmd;

    memset(&DmaCmd, 0, sizeof(XDmaPs_Cmd));

    DmaCmd.ChanCtrl.SrcBurstSize = 4;
    DmaCmd.ChanCtrl.SrcBurstLen = 4;
    DmaCmd.ChanCtrl.SrcInc = 1;
    DmaCmd.ChanCtrl.DstBurstSize = 4;
    DmaCmd.ChanCtrl.DstBurstLen = 4;
    DmaCmd.ChanCtrl.DstInc = 1;
    DmaCmd.BD.SrcAddr = (u32) txBuff;  // source = txBuff
    DmaCmd.BD.DstAddr = (u32) bram;    // destination = bram
    DmaCmd.BD.Length = TXSIZE * sizeof(int);   // length of data to transfer

    // Initialize DMA driver
    DmaCfg = XDmaPs_LookupConfig(DeviceId);
    if (DmaCfg == NULL) {
        return XST_FAILURE;
    }

    int Status = XDmaPs_CfgInitialize(DmaInst, DmaCfg, DmaCfg->BaseAddress);
    if (Status != XST_SUCCESS) {
        return XST_FAILURE;
    }   

    // Setup interrupts
    Status = SetupInterruptSystem(&GicInstance, DmaInst);
    if (Status != XST_SUCCESS) {
        return XST_FAILURE;
    }

    // Enable the interrupt handler
    XDmaPs_SetDoneHandler(DmaInst, 0, DmaDoneHandler, (void *)txDone);
        
    // Start the DMA
    Status = XDmaPs_Start(DmaInst, 0, &DmaCmd, 0);
    if (Status != XST_SUCCESS) {
        return XST_FAILURE;
    }

    // Loop until the DMA is done --  txDone will be set in interrupt handler
    int TimeOutCnt=0;
    while (!(*txDone) && TimeOutCnt < TIMEOUT_LIMIT) {
        TimeOutCnt++;
    }

    if (TimeOutCnt >= TIMEOUT_LIMIT) {
        printf("timeout\r\n");
        return XST_FAILURE;       
    }

    // Check that the data was copied correctly
    if (!checkCorrectness(TXSIZE, (int*)DmaCmd.BD.SrcAddr, (int*)DmaCmd.BD.DstAddr)) {
        printf("Data error");
        return XST_FAILURE;
    }
    else {
        printf("All data copied correctly\r\n");
    }

    // Activating Maxval IP
    volatile unsigned int* hw = (unsigned int*)XPAR_BRAM_INT_MAX_VAL_0_S00_AXI_BASEADDR;

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

    while ( (hw[1] & 0x1) != 0) {
        ;
    }

    cleanup_platform();
    return 0;
}


// Lightly modified from Xilinx example code
int SetupInterruptSystem(XScuGic *GicPtr, XDmaPs *DmaPtr)
{
    int Status;
    XScuGic_Config *GicConfig;

    Xil_ExceptionInit();

    /*
     * Initialize the interrupt controller driver so that it is ready to
     * use.
     */
    GicConfig = XScuGic_LookupConfig(XPAR_SCUGIC_SINGLE_DEVICE_ID);
    if (NULL == GicConfig) {
        return XST_FAILURE;
    }

    Status = XScuGic_CfgInitialize(GicPtr, GicConfig,
                       GicConfig->CpuBaseAddress);
    if (Status != XST_SUCCESS) {
        return XST_FAILURE;
    }

    /*
     * Connect the interrupt controller interrupt handler to the hardware
     * interrupt handling logic in the processor.
     */
    Xil_ExceptionRegisterHandler(XIL_EXCEPTION_ID_IRQ_INT,
                 (Xil_ExceptionHandler)XScuGic_InterruptHandler,
                 GicPtr);

    /*
     * Connect the device driver handlers that will be called when an interrupt
     * for the device occurs, the device driver handler performs the specific
     * interrupt processing for the device
     */

    /*
     * Connect the Fault ISR
     */
    Status = XScuGic_Connect(GicPtr,
                 XPAR_XDMAPS_0_FAULT_INTR,
                 (Xil_InterruptHandler)XDmaPs_FaultISR,
                 (void *)DmaPtr);
    if (Status != XST_SUCCESS) {
        return XST_FAILURE;
    }

    /*
     * Connect the Done ISR for all 8 channels of DMA 0
     */
    Status = XScuGic_Connect(GicPtr,
                 XPAR_XDMAPS_0_DONE_INTR_0,
                 (Xil_InterruptHandler)XDmaPs_DoneISR_0,
                 (void *)DmaPtr);

    if (Status != XST_SUCCESS)
        return XST_FAILURE;

    /*
     * Enable the interrupts for the device
     */
    XScuGic_Enable(GicPtr, XPAR_XDMAPS_0_DONE_INTR_0);
    XScuGic_Enable(GicPtr, XPAR_XDMAPS_0_FAULT_INTR);

    Xil_ExceptionEnable();

    return XST_SUCCESS;

}


// modified from Xilinx example code
void DmaDoneHandler(unsigned int Channel, XDmaPs_Cmd *DmaCmd, void *CallbackRef)
{
    volatile int *done = (volatile int *)CallbackRef;
    *done = 1;
    return;
}



int checkCorrectness(int size, int* src, int* dst) {
  for (int i=0; i<size; i++) {
    if (src[i] != dst[i]) {
      printf("Error at dst[%d]: %x vs %x\r\n", i, src[i], dst[i]);
      return 0;
    }
  }
  return 1;
}


/*   
    Simple DMA driver for PetaLinux
                    
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

// To use the DMA in PetaLinux:
//    1. Set up the system as explained in the PetaLinux chapter of the tutorial
//    2. Insert the memalloc kernel module by running "modprobe memalloc"
//    3. Your program needs to include this dma.h, the accompanying dma.c file, and
//       memalloc.h from the memalloc/ directory.
//    4. Basic program flow:
//          - call dma_init(size) to initialize the DMA, where size is the desired buffer
//            size in bytes
//          - call getTxBuffer() and getRxBuffer() to get pointers to the Tx and Rx buffers
//          - set up your data buffers
//          - call dma_reset() to reset the DMA module
//          - call dma_rx(size) to set up the DMA to receive data
//          - call dma_tx(size) to set up the DMA to send data
//          - call dma_sync() to wait for DMA to finish
//       See dmatest.c for an example.


// If you want to extend the functionality of this driver, it should be fairly
// trivial to do several things:
//     - have separate functions for waiting for the Tx and Rx channels to finish
//     - have more buffers or use those buffers more flexibly (e.g. receive data into a
//       buffer; modify that data, then use the same buffer to Tx)


// To use this, copy in memalloc.h from the memalloc/ module
#include "memalloc.h"

// ------------- Configuration macros ---------------------------------
#define DMA_BASE 0x40400000    // must match your address mapping in Vivado
#define MAX_DMA_LEN_BITS 14    // must match the DMA configuration in Vivado
#define DMA_MMAP_LEN 4096
// -------------------------------------------------------------------

// ----- Macros for DMA control and status reg interfaces ---------
#define MM2S_CNTL_REG       0x00
#define MM2S_STATUS_REG     0x04
#define MM2S_SRC_ADDR_REG   0x18
#define MM2S_LEN_REG        0x28

#define S2MM_CNTL_REG       0x30
#define S2MM_STATUS_REG     0x34
#define S2MM_DEST_ADDR_REG  0x48
#define S2MM_LEN_REG        0x58

// Macros for DMA control and status signals 
#define DMA_HALT            0
#define DMA_START           1
#define DMA_RESET           4
#define DMA_IDLE            2

// Macros to ease setting and reading DMA control/status regs and polling
#define set_dma_reg(offset,value) dma_cfg_base[offset/4] = value
#define get_dma_reg(offset)       dma_cfg_base[offset/4]
#define s2mm_busy() ((get_dma_reg(S2MM_STATUS_REG) & DMA_IDLE) == 0)
#define mm2s_busy() ((get_dma_reg(MM2S_STATUS_REG) & DMA_IDLE) == 0)
// -------------------------------------------------------------------



// Function prototypes

// --------------------------------------------------------------------
// User-facing functions:

/* Initialize the DMA, with buffers of the given size (in bytes)
 * Returns: 0 on success; -1 on error
 */
int dma_init(int);        

/* Returns a pointer to the start of the Tx buffer
 * Note: you should cast this to your application-appropriate datatype, e.g. int*
 */
void* getTxBuffer();   

/* Returns a pointer to the start of the Rx buffer
 * Note: you should cast this to your application-appropriate datatype, e.g. int*
 */
void* getRxBuffer();   

/* Reset the DMA module */
void dma_reset();      

/* Set up DMA to receive "size" bytes of data into start of RxBuffer.
 * Returns: 0 on success; -1 on error
 */
int dma_rx(int size);  

/* Set up DMA to send "size" bytes of data from start of TxBuffer 
 * Returns: 0 on success; -1 on error
 */
int dma_tx(int size);  

/* Blocks until DMA Tx and Rx operations are done. (Or a timeout if DMA is stuck.)
 * Returns: 0 on success, -1 on error
 */
int dma_sync();

/* Cleanup and unmap everything */
void dma_cleanup();        



// -----------------------------------------
// Internal functions 
static int check_size(int size); /* Checks transfer size is legal */

// Global variables for devices
static int mem_fd;                 // file descriptor for /dev/mem
static int memalloc_dev_fd;       // file descriptor for /dev/dmabuffer
volatile int *dma_cfg_base; // Pointer to base address of DMA control/status regs

// Global variables for dma
/* static void *dmabuffer;      // Pointer to base address of dmabuffer */
static void *txbase;         // Pointer to tx buffer base address (== dmabuffer)
static void *rxbase;         // Pointer to rx buffer base address (== dmabuffer + DMABUFFER_LEN/2)

int tx_buffer_id;
int rx_buffer_id;
static unsigned int tx_phy_addr;
static unsigned int rx_phy_addr;



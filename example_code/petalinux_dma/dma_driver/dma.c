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

// Assumptions:
//    1. The DMA module's base address is 0x40400000. If this does not match, change the
//       DMA_BASE macro in dma.h
//    2. Your system uses an AXI DMA module loopback
//    3. The DMA is configured to have a 14 bit length register. If this does not match, change
//       the macro MAX_DMA_LEN_BITS in dma.h
//    4. The DMA is configured to run in "simple mode" (not scatter/gather)
//    5. You are using the memalloc kernel module and you have inserted it with modprobe memalloc


#include <stdio.h>
#include <fcntl.h>  // file operations
#include <stdlib.h>
#include <sys/mman.h>
#include <unistd.h>
#include "dma.h"

// Initialize the DMA
int dma_init(int size) {
    
    // Check size (bytes) 
    int res = check_size(size);
    if (res)
        return res;
    
    
    ///////////////////////////////////////////////////
    // mmap the DMA control interface
    mem_fd = open("/dev/mem", O_RDWR | O_SYNC);
    if (mem_fd == -1) {
        printf("ERROR: failed to open /dev/mem\n");
        dma_cleanup();
        return -1;
    }
    
    dma_cfg_base = (unsigned int*)mmap(NULL, DMA_MMAP_LEN, PROT_READ | PROT_WRITE, MAP_SHARED, mem_fd, DMA_BASE);
    if (dma_cfg_base == NULL) {
        printf("ERROR: Failed to mmap DMA control registers\n");
        dma_cleanup();
        return -1;
    }
    
    // Open the /dev/memalloc file
    memalloc_dev_fd = open("/dev/memalloc", O_RDWR);
    if (memalloc_dev_fd == -1) {
        printf("ERROR: failed to open /dev/memalloc. Try running 'modprobe memalloc'\n");
        dma_cleanup();
        return -1;
    }

    // Reserve the tx buffer. 
    struct ioctl_arg_t ioctl_arg;
    ioctl_arg.buffer_size = size;
    int status = ioctl(memalloc_dev_fd, MEMALLOC_RESERVE_CMD, &ioctl_arg);
    if (status) 
        return status;      

    // Record its ID and physical address
    tx_buffer_id = ioctl_arg.buffer_id;
    tx_phy_addr = ioctl_arg.phys_addr;
    
    if (tx_buffer_id < 0 || status < 0) {
        printf("ERROR: memalloc (tx) reserve failed (id %d, status %d)\n", tx_buffer_id, status);
        return(-1);
    }

    status = ioctl(memalloc_dev_fd, MEMALLOC_GET_PHYSICAL_CMD, &ioctl_arg);
    if (status)
        return status;    
    tx_phy_addr = ioctl_arg.phys_addr;

    // Activate the buffer we just created
    status = ioctl(memalloc_dev_fd, MEMALLOC_ACTIVATE_BUFFER_CMD, &ioctl_arg);
    if (status)
        return status;

    // mmap it and record its virtual address
    txbase = mmap(0, size, PROT_READ | PROT_WRITE, MAP_SHARED, memalloc_dev_fd, 0);
    if (txbase == NULL) {
        printf("ERROR: mmap tx buffer failed");
        return(-1);
    }
     
    // Do the same thing the rx buffer: reserve it, record its physical address and id, and mmap it.
    status = ioctl(memalloc_dev_fd, MEMALLOC_RESERVE_CMD, &ioctl_arg);
    if (status)
        return status;

    rx_buffer_id = ioctl_arg.buffer_id;    

    status = ioctl(memalloc_dev_fd, MEMALLOC_GET_PHYSICAL_CMD, &ioctl_arg);
    if (status)
        return status;    
    rx_phy_addr = ioctl_arg.phys_addr;


    status = ioctl(memalloc_dev_fd, MEMALLOC_ACTIVATE_BUFFER_CMD, &ioctl_arg);
    if (status)
        return status;

    if (rx_buffer_id < 0 || status < 0) {
        printf("ERROR: memalloc (rx) reserve failed (id %d, status %d)\n", rx_buffer_id, status);
        return(-1);
    }

    rxbase = mmap(0, size, PROT_READ | PROT_WRITE, MAP_SHARED, memalloc_dev_fd, 0);
    if (rxbase == NULL) {
        printf("ERROR: mmap rx buffer failed");
        return(-1);
    }
            
    return 0;
}

// Return a pointer to the TxBuffer. User code an call this function, cast the pointer to
// the desired datatype, and interact with the buffer.
void* getTxBuffer() {
    return txbase;
}

// Return a pointer to the RxBuffer. User code can call this function, cast the pointer to
// the desired datatype, and interact with the buffer.
void* getRxBuffer() {
    return rxbase;
}

// Reset the DMA
void dma_reset() {
    set_dma_reg(MM2S_CNTL_REG, DMA_RESET);
    set_dma_reg(S2MM_CNTL_REG, DMA_RESET);   
}

// Set up a DMA "receive" on the S2MM channel
int dma_rx(int size) {
    // Check size is legal
    int res = check_size(size);
    if (res)
        return res;

    // Halt the DMA if necessary
    set_dma_reg(S2MM_CNTL_REG, 0);

    // Write the destination address
    set_dma_reg(S2MM_DEST_ADDR_REG, rx_phy_addr); 

    // Set the start bit
    set_dma_reg(S2MM_CNTL_REG, DMA_START);

    // Set the Rx length
    set_dma_reg(S2MM_LEN_REG, size);
    
    return 0;
}

// Set up a DMA "transmit" on the MM2S channel
int dma_tx(int size) {

    // Check size is legal
    int res = check_size(size);
    if (res)
        return res;

    // Halt the DMA if necessary
    set_dma_reg(MM2S_CNTL_REG, 0);

    // Write the destination address
    set_dma_reg(MM2S_SRC_ADDR_REG, tx_phy_addr); 

    // Set the start bit
    set_dma_reg(MM2S_CNTL_REG, DMA_START);

    // Set the Rx length
    set_dma_reg(MM2S_LEN_REG, size);
    
    return 0;
}

// Block until the MM2S and S2MM channels are both idle
int dma_sync() {
    int its=0;

    // while loop to check for completion (done when status reg & 0x2 != 0)
    while (s2mm_busy() || mm2s_busy()) {
        its++;
        if (its == 1000000) {
            printf("ERROR: Timeout waiting for DMA.");
            printf("mm2s status: %x\n", get_dma_reg(MM2S_STATUS_REG));
            printf("s2mm status: %x\n", get_dma_reg(S2MM_STATUS_REG));
            return -1;
        }
    }
    return 0;
}


// A cleanup function. If files are open; close them. If regions are mmap-ed, munmap them.
void dma_cleanup() {
    // release Tx buffer
    // release Rx buffer

    struct ioctl_arg_t ioctl_arg;
    ioctl_arg.buffer_id = tx_buffer_id;
    int status = ioctl(memalloc_dev_fd, MEMALLOC_RELEASE_CMD, &ioctl_arg);
    if (status < 0) {
        printf("ERROR: failed to release buffer %d. Status: %d\n", tx_buffer_id, status);
    }

    ioctl_arg.buffer_id = rx_buffer_id;
    status = ioctl(memalloc_dev_fd, MEMALLOC_RELEASE_CMD, &ioctl_arg);
    if (status < 0) {
        printf("ERROR: failed to release buffer %d. Status: %d\n", rx_buffer_id, status);
    }
    

    if (memalloc_dev_fd > -1) 
        close(memalloc_dev_fd);

    if (dma_cfg_base)
        munmap((void*)dma_cfg_base, DMA_MMAP_LEN);

    if (mem_fd > -1) 
        close(mem_fd);

}

static int check_size(int size) {
    // The DMA's buffer length register is MAX_DMA_LEN_BITS bits, so the size 
    //    must be strictly < 2^MAX_DMA_LEN_BITS
    // All accesses must be at least word width (4 bytes), and must be 4-byte aligned.
    //    So, size must be a multiple of 4.

    if (size < 4) {
        printf("ERROR: Requested DMA transfer size (%d bytes) is not legal\n", size);
        return -1;
    }

    if ((size & 0x3) != 0) {
        printf("ERROR: Requested DMA transfer size (%d bytes) is not a multiple of 4\n", size);
        return -1;
    }
        
    
    if (size >= (1<<MAX_DMA_LEN_BITS)) {
        printf("ERROR: Requested DMA transfer size (%d bytes) is larger than maximum size %d\n", size, ((1<<MAX_DMA_LEN_BITS)-1));
        return -1;
    }

    return 0;
}

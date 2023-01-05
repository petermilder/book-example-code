/*   
        Example program to test memory restore/dump function in SDK

        From "Getting Started with the Xilinx Zynq FPGA and Vivado" 
        by Peter Milder (peter.milder@stonybrook.edu)

        Copyright (C) 2019 Peter Milder

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
#include "xil_cache.h"

#define RESERVED_MEM 0x00100000


// This program will test the restore/dump memory functionality in Vivado SDK.
// This program requires that you edited the linker script to reserve at least 16 MB 
// of memory starting from address 0x00100000, and that you used the memory restore
// function to load the 8 MB test data mydata.bin into that location.

int main()
{
    init_platform();

    // 2^21 integers in our test set
    int numInts = (1<<21);

    // Pointer to the beginning of our reserved memory
    int *inputLocation = (int*)RESERVED_MEM;

    // We wrote 2^23 bytes (2^21 ints) into that memory. Let's store our output
    // values in the 2^23 bytes (2^21 ints) *after* that location
    int *outputLocation = (int*)(RESERVED_MEM + 4*numInts);


    printf("Storing results to array starting at memory address 0x%08x\n\r", (unsigned int)outputLocation);
    printf("0x%08x\n\r", 0x00100000 + 4*(1<<21));

    printf("Reading integer stored at memory address 0x%08x: %d (expected value = 9000)\n\n\r", (unsigned int)inputLocation, inputLocation[0]);

    printf("Checking reservedMem[i] from i=0 to %d\n\r", numInts-1);

    int errors = 0;
    for (int i=0; i<numInts; i++) {
        if (inputLocation[i] != 9000+i)
            errors++;
    }

    printf("\t%d errors found\n\n\r", errors);

    printf("Storing results to array starting at memory address 0x%08x\n\r", (unsigned int)outputLocation);

    printf("outputLocation[i] = reservedMem[i] + 27, for i=0 to %d\n\n\r", numInts-1);

    for (int i=0; i<numInts; i++) {
        outputLocation[i] = inputLocation[i] + 27;
    }

    printf("Checking outputLocation[i] from i=0 to %d\n\r", numInts-1);

    errors = 0;
    for (int i=0; i<numInts; i++) {
        if (outputLocation[i] != 9027+i)
            errors++;
    }

    // Flushing the cache to ensure that all values go back to DRAM
    Xil_DCacheFlushRange((UINTPTR)outputLocation, numInts*sizeof(int));

    printf("\t%d errors found\n\n\r", errors);

    printf("Now, use Vivado SDK to dump %d bytes of data starting from location 0x%08x\n\n\n\r", 2*numInts*sizeof(int), (unsigned int)inputLocation);

    cleanup_platform();
    return 0;
}

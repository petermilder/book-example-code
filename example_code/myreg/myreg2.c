/*   
    Example code to test the "myreg" IP, an example AXI4-Lite peripheral 
            
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

int main()
{
    init_platform();

    xil_printf("-----------------------------------------\r\n");

    // For more details about how this code is structured, first
    // see myreg1.c.

    // Just like in the last example, set up a pointer to base address
    int* myreg = XPAR_MYREG1_0_S00_AXI_BASEADDR;

    // Our IP has 8 memory-mapped registers. So, myreg[0] points to
    // the first register, myreg[1] points to the second, and so on.

    // However, now we can only write to the first 2 registers,
    // and the hardware itself will fill in the values on the other 6.

    // So, let's write values into register 0 and 1, and then observe
    // what values the system fills in to the others.
    myreg[0] = 0xf0;
    myreg[1] = 0x0f;

    // If you were to try to write into any of the *other* registers,
    // the system would ignore it. For example, try un-commenting the 
    // following line and observe that it doesn't have any effect, 
    // because you changed the code in your IP to ignore writes to 
    // registers 2 through 7:
    myreg[2] = 0xff;

    // Now, let's read from all 8 registers. We will print the results
    // in both decimal and hex. (The decimal representation makes it easier
    // to look at the arithmetic results, while the hex representation
    // makes it easier to look at the results from the logical operations.)
    for (int i=0; i<8; i++) {
      int readVal = myreg[i];        
      // print the value in both decimal and hex
      xil_printf("reg[%d] = %d = 0x%x\n\r", i, readVal, readVal);
    }

    cleanup_platform();
    return 0;
}

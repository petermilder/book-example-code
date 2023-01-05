/*   
    Example code to test an AXI4-Lite peripheral with 8 registers.
            
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
#include "xil_io.h" // used for reading and writing data

int main() {
  init_platform();
  
  print("Starting myreg1\n\r");
  
  // u32 is an unsigned 32 bit int (here we are using this
  // to represent the memory address
  // the "XPAR_..." macro is defined in xparameters.h in the BSP
  // We're just assigning it to a u32 here for convenience.
  u32 base_addr = XPAR_MYREG1_0_S00_AXI_BASEADDR;
  
  // Or, equivalently, you can just look up the base address from
  // Vivado's Address Editor, and set it here. In my case this would
  // be:
  // u32 base_addr = 0x43C00000;
        
  // Our IP has 8 memory-mapped registers. Variable base_addr
  // holds the memory address of the first one. The next
  // will be at base_addr+4, base_addr+8, etc. (It increments by
  // 4 because each register holds 4 bytes.)

  // So, let's write a loop where we will write number 100 into register
  // 0, 200 to register 1, etc.
  // We will use a function called Xil_Out32(addr, data), which
  // writes 32 bits (data) into the given address
  for (int i=0; i<8; i++) {
    Xil_Out32(base_addr + 4*i, 100*i);
  }

  // We can similarly use Xil_In32(addr) to read that data
  for (int i=0; i<8; i++) {
    int readVal = Xil_In32(base_addr + 4*i);
    xil_printf("reg[%d] = %d\n\r", i, readVal);
  }

  // If everything works correctly, the system should print the
  // numbers we just wrote.

  cleanup_platform();
  return 0;
}

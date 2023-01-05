/*   
    Example software demonstrating basic operation of AXI Timer
                    
    From "Getting Started with the Xilinx Zynq FPGA and Vivado" 
    by Peter Milder (peter.milder@stonybrook.edu)

    Copyright (C) 2018-2020 Peter Milder

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


// This is a small standalone example of how to use the AXI timer in a bare metal project.

// It assumes that you have added an AXI Timer to your project, at 0x42800000. (If
// necessary, adjust the TIMER_BASE macro below.

// It also assumes the AXI Timer is clocked at 100 MHz. If your timer's clock
// frequency has changed, adjust the TIMER_FREQ macro below

#define TIMER_BASE 0x42800000  // change to match base address of your AXI timer if necessary
#define TIMER_FREQ 100         // in MHz

#include <stdio.h>
#include "platform.h"
#include "xil_printf.h"
#include "xtmrctr.h"  // timer

#define TMRCTR_DEVICE_ID        XPAR_TMRCTR_0_DEVICE_ID

int main() {

    init_platform();
    
    XTmrCtr TimerCounter;
    int Status = XTmrCtr_Initialize(&TimerCounter, TMRCTR_DEVICE_ID);
    if (Status != XST_SUCCESS) {
        return XST_FAILURE;
    }

    // Set up timer. Clear it. Take the first reading; start the timer.
    XTmrCtr_SetOptions(&TimerCounter, 0, XTC_AUTO_RELOAD_OPTION);
    XTmrCtr_Reset(&TimerCounter, 0);                     // reset timer

    int time0 = XTmrCtr_GetValue(&TimerCounter, 0);      // read timer value

    XTmrCtr_Start(&TimerCounter, 0);                     // start timer

    
    // Now, perform whatever operations you want to measure the time of.
    
    // Read the timer value again
    int time1 = XTmrCtr_GetValue(&TimerCounter, 0);

    printf("Measured %d clock cycles == %d seconds\n", (time1-time0),((double)(time1-time0))/(TIMER_FREQ*1000000));

    cleanup_platform();
    return 0;
}

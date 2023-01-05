/*   
    A simple test of AXI GPIO modules

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
#include <xgpio.h>  // used to interact with your GPIO modules
#include <sleep.h>  // used to allow our loop to pause


int main() {
    init_platform();

    XGpio mygpio, mygpio2; // Objects for each of our GPIO modules

    // Initialize the two GPIO devices; note we are using the
    // macros (..._DEVICE_ID) that we observed in xparameters.h
    XGpio_Initialize(&mygpio, XPAR_AXI_GPIO_0_DEVICE_ID);
    XGpio_Initialize(&mygpio2, XPAR_AXI_GPIO_1_DEVICE_ID);

    // Next, we need to tell the system which channels of our GPIOs
    // are inputs and which are outputs.
    // GPIO  channel 1: outputs (LEDs)
    // GPIO  channel 2: inputs  (switches)
    // GPIO2 channel 1: inputs  (btns)

    // "Set channel 1 of mygpio to be an output."
    XGpio_SetDataDirection(&mygpio, 1, 0x0);

    // "Set channel 2 of mygpio to be an input."
    XGpio_SetDataDirection(&mygpio, 2, 0xff);

    //etc.
    XGpio_SetDataDirection(&mygpio2, 1, 0xff);

    int switch_data, button_data;

    while(1) { // loop forever
        // read data from the switches (mygpio, channel 2) and
        // store it in switch_data
        switch_data = XGpio_DiscreteRead(&mygpio, 2);

        // The result will be an 8-bit number corresponding
        // to the 8 switches.

        // Now, write that number to the LEDs (mygpio, channel 1)
        XGpio_DiscreteWrite(&mygpio, 1, switch_data);

        // Read the button data. The result will be five-bit
        // number, where each bit represents one of the buttons.
        // If the number is non-zero, print the number to your
        // terminal.
        button_data = XGpio_DiscreteRead(&mygpio2, 1);
        if (button_data != 0)
            printf("Button: %x\n\r", button_data);

        // Sleep for 200 milliseconds (200,000 microseconds)
        // before looping again.
        usleep(200000);

    }
    cleanup_platform();
    return 0;
}

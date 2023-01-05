/*   
    Top-level design, connecting IO from smallmult to the IOs on ZedBoard
                    
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

module top(SW0, SW1, SW2, SW3, SW4, SW5, SW6, SW7,
           LD0, LD1, LD2, LD3, LD4, LD5, LD6, LD7);

    input  SW0, SW1, SW2, SW3, SW4, SW5, SW6, SW7;
    output LD0, LD1, LD2, LD3, LD4, LD5, LD6, LD7;

    mult multInst(.in0({SW3, SW2, SW1, SW0}),
                 .in1({SW7, SW6, SW5, SW4}),
                 .out0({LD7, LD6, LD5, LD4, LD3, LD2, LD1, LD0}));

endmodule 


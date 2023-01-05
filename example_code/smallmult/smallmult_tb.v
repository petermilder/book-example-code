/*   
    A tiny testbench for combinational multiplier example
                    
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

module mult_tb();
    reg signed [3:0] m0, m1;
    wire signed [7:0] prod;

    mult dut(.in0(m0), .in1(m1), .out0(prod));

    initial begin
        #10;
        m0 = 2;
        m1 = 3;

        #10;
        m0 = -7;
        m1 = 4;

        // etc... add more here
        #10;
        $stop;      
    end
endmodule

   

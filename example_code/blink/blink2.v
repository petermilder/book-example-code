/*   
    Example Verilog module to increment a counter every 100M cycles. 

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

// top-level module
//    inputs:  1 push-button and 100 MHz GCLK
//    outputs: 8 LED signals
module top(GCLK, BTND, LD0, LD1, LD2, LD3, LD4, LD5, LD6, LD7);

    input GCLK, BTND;
    output LD0, LD1, LD2, LD3, LD4, LD5, LD6, LD7;

    blink2 instBlink2(.clk100(GCLK), .clr(BTND),
                      .led_vals({LD7, LD6, LD5, LD4, LD3, 
                                 LD2, LD1, LD0}));
endmodule

module blink2(clk100, clr, led_vals);
    input clk100, clr;
    output reg [7:0] led_vals;
    reg [26:0] clock_counter; 

    always @(posedge clk100) begin
        if (clr) begin
            clock_counter <= 0;
            led_vals <= 0;
        end
        else if (clock_counter == 100000000) begin
            clock_counter <= 0;
            led_vals <= led_vals+1;
        end
        else
            clock_counter <= clock_counter+1;
    end
endmodule 

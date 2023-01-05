/*
    Tiny testbench for floating point example.

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

`timescale 1ns / 1ps


module multtest();

    logic clk, reset;
    initial clk=0;
    always #5 clk = ~clk;

    logic [31:0] in0, in1, out0;
    logic valid_in, valid_out;

    multtop dut(clk, reset, valid_in, valid_out, in0, in1, out0);

    initial begin
        reset = 1;
        in0 = 0;
        in1 = 0;
        valid_in = 0;

        @(posedge clk);
        #1; 
        reset = 0;

        // Test 1: -2 * 17.5
        // Expected output: -35 == 0xc20c0000
        @(posedge clk);
        #1;
        in0 = 32'hc0000000; //  == -2
        in1 = 32'h418c0000; //  == 17.5     
        valid_in = 1'b1;


        // Test 2: -pi * e
        // Expect output: approx. -8.5397342227 == 0xc108a2c0
        @(posedge clk);
        #1;
        in0 = 32'hc0490fdb; // approx == -pi
        in1 = 32'h402df854; // approx == e      
        valid_in = 1'b1;

        @(posedge clk);
        #0;
        in0 = 0;
        in1 = 0;
        valid_in = 0;

        // You could do more tests here
        #1000;
        $finish;
    end

    always @(posedge clk) begin
        if (valid_out == 1)
            $display("Received output 0x%x", out0);
    end

endmodule

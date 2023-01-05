/*   
    A testbench for maxval.v. 

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

// This is a quick simulation testbench to check the maxval module only.
// This will randomly initialize a RAM with values, and put 32'hffffffff
// (the max unsigned 32 bit value) into the last location.

// To check correct functionality, simulate and use a waveform to observe
// correct behavior in the maxval module. Check that it correctly:
//    - reads all words in the memory
//    - checks each one and stores the largest at each step
//    - does not miss the last word in the sequence (addr = 2047)
//    - stores the largest back to address 0

module maxval_tb();

    logic clk, reset;
    logic  [31:0] ps_control;
    logic [31:0] pl_status;
    logic [12:0] bram_addr;
    logic  [31:0] bram_rddata;
    logic [31:0] bram_wrdata;
    logic [3:0]  bram_we;

    maxval dut(.clk(clk), .reset(reset), .ps_control(ps_control), .pl_status(pl_status),
                .bram_addr(bram_addr), .bram_rddata(bram_rddata), .bram_wrdata(bram_wrdata),
                .bram_we(bram_we));

    memory_sim ms(.clk(clk), .reset(reset), .bram_addr(bram_addr), 
                    .bram_rddata(bram_rddata), .bram_wrdata(bram_wrdata),
                    .bram_we(bram_we));

    initial clk=0;
    always #5 clk = ~clk;

    initial begin
        ps_control = 0;
        reset = 1;
        @(posedge clk);
        @(posedge clk);
        @(posedge clk);
        @(posedge clk);
        #1; reset = 0;

        @(posedge clk);
        @(posedge clk);
        #1; ps_control = 1;

        wait(pl_status[0] == 1'b1);

        @(posedge clk);
        #1; ps_control = 0;

        wait(pl_status[0] == 1'b0);

        #100;
        $stop;
    end
endmodule

module memory_sim(
    input         clk,
    input         reset,
    input        [12:0] bram_addr,
    output logic [31:0] bram_rddata,
    input        [31:0] bram_wrdata,
    input         [3:0] bram_we);

    logic [2047:0][31:0] mem;

    initial begin
        integer i;
        for (i=0; i<2047; i=i+1)
            mem[i] = $random;

        mem[2047] = 32'hffffffff;
    end // initial

    always @(posedge clk) begin
        bram_rddata <= mem[bram_addr[12:2]];
        if (bram_we == 4'hf)
            mem[bram_addr[12:2]] <= bram_wrdata;
        else if (bram_we != 0)
            $display("ERROR: Memory simulation model only implemented we = 0 and we=4'hf. Simulation will be incorrect.");              
    end
endmodule // memory_sim
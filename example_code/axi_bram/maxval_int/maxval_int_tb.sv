/*   
    A testbench for maxval_int.v. 

    From "Getting Started with the Xilinx Zynq FPGA and Vivado" 
    by Peter Milder (peter.milder@stonybrook.edu)

    Copyright (C) 2020 Peter Milder

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
// The testbench will simulate writing input values into the BRAM using
// the ps_bram_* ports. Then it will initialize the maxval operation and
// check the result.

// To check correct functionality, simulate and use a waveform to observe
// correct behavior in the maxval module. Check that it correctly:
//    - stores data into the BRAM
//    - reads all words in the memory
//    - checks each one and stores the largest at each step
//    - does not miss the last word in the sequence (addr = 2047)
//    - stores the largest back to address 0

module maxval_tb();

    logic clk, reset;
    logic [31:0] ps_control;
    logic [31:0] pl_status;
    logic [12:0] bram_addr;
    logic [31:0] bram_rddata;
    logic [31:0] bram_wrdata;
    logic [3:0]  bram_we;

    maxval_int dut(.clk(clk), .reset(reset), .ps_control(ps_control), .pl_status(pl_status),
               .ps_bram_clk(clk), .ps_bram_addr(bram_addr), .ps_bram_rddata(bram_rddata),
               .ps_bram_wrdata(bram_wrdata), .ps_bram_we(bram_we), .ps_bram_en(1'b1));


    initial clk=0;
    always #5 clk = ~clk;

    integer i;

    initial begin
        // initialize
        ps_control = 0;
        bram_addr = 0;
        bram_wrdata = 0;
        bram_we = 0;

        reset = 1;
        @(posedge clk);
        @(posedge clk);
        @(posedge clk);
        @(posedge clk);
        #1; reset = 0;

        // write test data into BRAM (simulating the function of the PS+AXIBRAMCTRL)
        for (i=0; i<2047*4; i=i+4) begin
            bram_addr   = i;
            bram_wrdata = $random;
            bram_we     = 4'hf;
            @(posedge clk); #1;
        end
        bram_addr   = 2047*4;
        bram_wrdata = 32'hffffffff;
        @(posedge clk); #1;

        bram_we     = 0;
        bram_wrdata = 32'hx;
        bram_addr   = 32'hx;

        // Now we can tell the maxval hardware to activate
        @(posedge clk);
        @(posedge clk);
        #1; ps_control = 1;

        // Wait until it is done
        wait(pl_status[0] == 1'b1);

        // De-assert our start signal
        @(posedge clk);
        #1; ps_control = 0;

        // Wait for the hardware to de-assert done
        wait(pl_status[0] == 1'b0);

        // Now, check that we can correctly read FFFFFFFF from memory address 0:
        @(posedge clk); #1;
        bram_addr = 0;

        @(posedge clk); #1;
        if (bram_rddata !== 32'hffffffff)
            $display("ERROR");
        else
            $display("Largest value found and read correctly from BRAM");


        #100;
        $stop;
    end
endmodule

/*   
    Testbench for myreg example design
            
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

`timescale 1ns / 1ps

// This points to the instance name of your PS
`define VIP tb.dut.design_1_i.processing_system7_0.inst

// This points to the base address of your "myreg" IP. If yours is at a different address, change this:
`define MYREG_BASE 32'h43c00000


// Testbench module
module tb();
        
    // Signals for our testbench's clock and reset signals
    reg clk, rstn;
    wire tmp_clk, tmp_rstn;
    assign tmp_clk=clk;
    assign tmp_rstn = rstn;
   
    // Create the clock signal
    initial clk=0;
    always #5 clk = ~clk;
    
    // Signals needed for VIP commands        
    reg resp;
    reg [31:0] read_data;
    
    // Instantiate your block design
    design_1_wrapper dut(
        .DDR_addr(),
        .DDR_ba(),
        .DDR_cas_n(),
        .DDR_ck_n(),
        .DDR_ck_p(),
        .DDR_cke(),
        .DDR_cs_n(),
        .DDR_dm(),
        .DDR_dq(),
        .DDR_dqs_n(),
        .DDR_dqs_p(),
        .DDR_odt(),
        .DDR_ras_n(),
        .DDR_reset_n(),
        .DDR_we_n(),
        .FIXED_IO_ddr_vrn(),
        .FIXED_IO_ddr_vrp(),
        .FIXED_IO_mio(),
        .FIXED_IO_ps_clk(tmp_clk),
        .FIXED_IO_ps_porb(tmp_rstn),
        .FIXED_IO_ps_srstb(tmp_rstn)
    );
        
        
    integer     i;
    initial begin        
        $display ("running the testbench!");
             
        // turn off debug messages. If you remove this, the simulation will print
        // information to the console for each VIP operation you perform
        `VIP.set_debug_level_info(0);
         
        // reset the entire system
        rstn = 1'b0;
        @(posedge clk);  
        @(posedge clk);       
        rstn = 1'b1;
         
        // wait a few cycles
        @(posedge clk);
        @(posedge clk);
        @(posedge clk);
        @(posedge clk);
        @(posedge clk);
                   
        //Reset the PL
        `VIP.fpga_soft_reset(32'h1);
        `VIP.fpga_soft_reset(32'h0);

        // simulate writing the input signals, just like in test software
        // note that we we increment the address in steps of four because
        // it is byte-addressed (and one word = four bytes)
        `VIP.write_data(`MYREG_BASE,   4, 32'h00f0, resp);
        `VIP.write_data(`MYREG_BASE+4, 4, 32'h000f, resp);

        for (i=0; i < 8; i=i+1) begin    
            `VIP.read_data(`MYREG_BASE + 4*i, 4, read_data, resp);
            $display ("reg[%d] = %d = 0x%x", i, read_data, read_data);
        end
    
        $display("done running testbench");

        #100;
        $stop;
    end

endmodule

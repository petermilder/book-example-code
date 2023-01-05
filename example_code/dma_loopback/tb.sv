/*   
    Testbench for the DMA loopback system.
            
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

// Note: this doesn't seem to work right on Vivado 2017.2. I tested it successfully on 2018.2 though.

`timescale 1ns / 1ps

// This points to the intance name of your PS.
`define VIP tb.dut.design_1_i.processing_system7_0.inst

// This points to the base address of the AXI DMA. If yours is at a different address, change this:
`define DMA_BASE 32'h40400000

// These are the memory addresses in the PS where the input and output data will reside
`define TX_ADDR 32'h00100000
`define RX_ADDR 32'h00104000

// How much data (in 32-bit integers) we will transfer
`define TX_LENGTH 32'd4088      
// Requirements on the size:
//   1. By default, the DMA controller's buffer length is 14 bits, meaning it can
//      work with DMAs of length < (2^14) bytes = 2^12 integers
//   2. The ZYNQ's cache lines are each 32 bytes (which holds 8 integers). To work properly, the
//      buffers you allocate for the DMA must be a multiple of 8 integers in length.
//
//   So, this code will work if you set TX_LENGTH to any integer <= 4088 that is a multiple of 8.
//   If your application would benefit from DMAs larger than this, you can simply increase the
//   width of the max buffer length register (which you can configure by double-clicking on
//   the DMA block in your block diagram.)



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
    reg    resp;
    reg [31:0] read_data;

    // Variables used in testbench
    integer    i, errors;   

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
                         .FIXED_IO_ps_srstb(tmp_rstn));
                
    // Test procedure:
    //   We will set up the DMA to read 4088 integers (== 16352 bytes) from PS address 
    //   0x00100000, and write data back to memory starting from 0x00104000. So, we will
    //   do the following steps:   
    //   1. Load test data into the PS memory. Let's choose to write 0x70000000 + i into address
    //      0x00010000 + 4*i, for i=0 to 4087.
    //   2. Clear the memory locations where we want to receive the data (addresses 
    //      0x00014000 + i*4, for i=0 to 4087. (This is not strictly necessary, but this way we
    //      are setting to known values so we can be sure the DMA overwrote them with new values.
    //   3. Set the DMA's S2MM address to 0x00104000 and MM2S address to 0x00100000.
    //   4. Set the length register of the S2MM to 4088*4 bytes; set the length register
    //      of the MM2S to the same value.
    //   5. Set the S2MM and MM2S start bits to 1.
    //   6. Wait until both the MM2S and S2MM status registers indicate the DMA is idle.
    //   7. Check that the data in memory starting from address 0x00104000 is correct.

    initial begin
                 
        // turn off debug messages. If you remove this, the simulation will print
        // information to the console for each VIP operation you perform
        `VIP.set_debug_level_info(0);

        // Stop simulation if there is an error
        `VIP.set_stop_on_error(1);  
    
        $display ("Starting the testbench");

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

        // Initialize the memory in the PS
        // For this test, we will place the TX buffer starting at address `TX_ADDR, and the
        // RX buffer `RX_ADDR. In your  real system, the PS will allocate buffers and use 
        // their locations. (See the example C code for more information.)
        for (i=0; i<`TX_LENGTH; i=i+1) begin       
            `VIP.write_mem(32'h70000000 + i, `TX_ADDR + 4*i, 4);
        end

        // Clear the buffer for receiving data
        for (i=0; i<`TX_LENGTH; i=i+1) begin       
            `VIP.write_mem(32'd0, `RX_ADDR + 4*i, 4);
        end
    
        // Tell the DMA the address values
        `VIP.write_data(`DMA_BASE + 32'h48, 4, `RX_ADDR, resp);
        `VIP.write_data(`DMA_BASE + 32'h18, 4, `TX_ADDR, resp);

        // Set the DMA's start bits
        `VIP.write_data(`DMA_BASE,          4, 32'd1, resp);
        `VIP.write_data(`DMA_BASE + 32'h30, 4, 32'd1, resp);
            
        // Set the DMA's length registers (in bytes so need `TX_LENGTH * 4)
        `VIP.write_data(`DMA_BASE + 32'h58, 4, `TX_LENGTH*4, resp);
        `VIP.write_data(`DMA_BASE + 32'h28, 4, `TX_LENGTH*4, resp);

        // Wait until the MM2S status register has bit 1 set to 1, meaning it is idle.
        // Check every five clock cycles, and timeout after 100k cycles
        `VIP.wait_reg_update(`DMA_BASE + 32'h04, 32'h00000002, 32'hfffffffd, 5, 100000, read_data);

        // Wait until S2MM status register has bit 1 set to 1, meaning it is idle
        `VIP.wait_reg_update(`DMA_BASE + 32'h34, 32'h00000002, 32'hfffffffd, 5, 100000, read_data);

        // Check the data
        errors = 0; 
        for (i=0; i<`TX_LENGTH; i=i+1) begin
            `VIP.read_mem(`RX_ADDR + i*4, 4, read_data);
            if (read_data != (32'h70000000 + i)) begin
                errors = errors+1;
                $display("Error on word %d: Expected 0x%x, received 0x%x", i, 32'h70000000 + 4*i, read_data);
            end
        end

        if (errors)
            $display("%d errors detected", errors);
        else
            $display("All data (%d integers) received successfully", `TX_LENGTH);
            
        #1000;  
        $stop;  
    end
endmodule



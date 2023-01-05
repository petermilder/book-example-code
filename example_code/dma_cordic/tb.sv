/*   
    A testbench to test the AXI CORDIC and AXI DMA module.
    
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

// Testbench for CORDIC loopback design. This testbench is based on the DMA loopback testbench;
// please see that (example_code/dma_loopback/tb.sv) for more details and explanation of the DMA

// The CORDIC is configured as follows:
//    Function:        Sin and Cos
//    Arch. Confi.:   Parallel
//    Pipelining Mode: Maximum
//    Phase Format:    Radians
//    Input Width:     32 bits
//    Output Width:    16 bits (this is 16 bits each for cos and sin)
//    Adv. Config:     [Defaults, including Coarse Rotation]
// AXI Stream Options:
//    Phase Channel:   Has TLAST
//    Flow Control:    Blocking
//    Output has TREADY: Selected
//    Output TLAST Behavior: Pass Phase TLAST

// The DMA is configured in "Simple mode"

// Data representation:
// The phase uses number format XXX.XX... (3 bits integer, 29 bits
//                                                  fraction)
//          This means it can hold a number between -4 and almost 4.
//          To represent number N compute: round(N * 2^29). This will give
//          a number of up to 32 bits, which can be transmitted to the CORDIC.
//          For example, to represent pi: round(pi * 2^29) = 1686629713.

// The output (cosine and sine) are 16 bits each, in format XX.XX... (2 bits
//          integer, 14 bits fraction). This allow it to hold a number between
//          -2 and almost 2. If it outputs a number N, to convert it to the real
//          number equivalent, multiply N * 2^(-14).
//          For example if, the system outputs 11585, this is equivalent to
//          11585 * 2^(-14) = 0.7070922852 (approx. sqrt(2)/2).


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
`define TX_LENGTH 32'd8


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
    integer    i;
    reg signed [15:0] c, s;
    real realAng, realCos, realSin; 
     
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
        `VIP.write_mem(32'h00000000,   `TX_ADDR + 0,  4);  // == 0 (computed as 0*2^29)
        `VIP.write_mem(32'h20000000,   `TX_ADDR + 4,  4);  // == 1
        `VIP.write_mem(32'he0000000,   `TX_ADDR + 8,  4);  // == -1
        `VIP.write_mem(32'd843314857,  `TX_ADDR + 12, 4);  // ==  pi/2 [computed as  (pi/2)*2^29]
        `VIP.write_mem(-32'd843314857, `TX_ADDR + 16, 4);  // == -pi/2 [computed as (-pi/2)*2^29]
        `VIP.write_mem(32'd421657428,  `TX_ADDR + 20, 4);  // ==  pi/4 [computed as  (pi/4)*2^29]
        `VIP.write_mem(-32'd421657428, `TX_ADDR + 24, 4);  // == -pi/4 [computed as (-pi/4)*2^29]
        `VIP.write_mem(32'd281104952,  `TX_ADDR + 28, 4);  // ==  pi/6 [computed as  (pi/6)*2^29]
    
        // Clear the buffer for receiving data
        for (i=0; i<`TX_LENGTH; i=i+1) begin       
            `VIP.write_mem(32'd0, `RX_ADDR + 4*i, 4);
        end
    
        // Tell the DMA the address values
        `VIP.write_data(`DMA_BASE + 32'h48, 4, `RX_ADDR, resp);
        `VIP.write_data(`DMA_BASE + 32'h18, 4, `TX_ADDR, resp);

        // Set the start DMA's start bits
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

        // Print the results.
        // In each loop iteration, let's read 4 bytes, then pull out the
        // 16 LSBs (which represent the cos) and the 16 MSBs (which represent
        // the sin).
        for (i=0; i<`TX_LENGTH; i=i+1) begin
            `VIP.read_mem(`RX_ADDR + i*4, 4, read_data);
            c = read_data[15:0];    // cosine
            s = read_data[31:16];   // sine

            // Let's get the angle used as input so we can print everything nicely
            `VIP.read_mem(`TX_ADDR + i*4, 4, read_data);

            // Convert everything to floating point so we can display it nicely.
            realCos = $itor(c) / $itor(1<<14);
            realSin = $itor(s) / $itor(1<<14);
            realAng = $itor($signed(read_data)) / $itor(1<<29);

            // Print the cosine, sine, and angle
            $display("cos(%f) = %f\t sin(%f) = %f", realAng, realCos, realAng, realSin);
        end
        
        #1000;  
        $stop;  
    end     
endmodule



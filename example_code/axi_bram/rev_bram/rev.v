/*   
    An example design with two BRAMs. It reads values from one, and writes them
    to the other in reverse order.

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

module rev(
    input         clk,
    input         reset,
    input  [31:0] ps_control,
    output [31:0] pl_status,
    input         ps_bram_clk,
    input  [13:0] ps_bram_addr,
    output [31:0] ps_bram_rddata,
    input  [31:0] ps_bram_wrdata,
    input  [3:0]  ps_bram_we,
    input         ps_bram_en);

    wire clr, inc, done;

    reg  [12:0] bram_0_addr;
    wire [31:0] bram_0_rddata;
    wire [31:0] bram_0_wrdata;
    wire [3:0]  bram_0_we;

    reg  [12:0] bram_1_addr;
    wire [31:0] bram_1_rddata;
    wire [31:0] bram_1_wrdata;
    wire [3:0]  bram_1_we;

    memSys  mem(.clk_a(ps_bram_clk), .addr_a(ps_bram_addr), .wrdata_a(ps_bram_wrdata), 
                 .rddata_a(ps_bram_rddata), .wr_a(ps_bram_we), .en_a(ps_bram_en),
                 .clk_b(clk), .addr_b(bram_0_addr), .wrdata_b(bram_0_wrdata), 
                 .rddata_b(bram_0_rddata), .wr_b(bram_0_we),
                 .clk_c(clk), .addr_c(bram_1_addr), .wrdata_c(bram_1_wrdata), 
                 .rddata_c(bram_1_rddata), .wr_c(bram_1_we));

    // State machine
    reg [1:0]  state, next_state;
    // state 0: wait for ps_control == 1 (PS start signal). Also read address 0 from BRAM 0
    // state 1: receive data last cycle's read request and write it to BRAM 1
    //          also request the next read from BRAM 0
    // state 2: done reading; perform last write to BRAM 1
    // state 3: write status = 1 (done); wait for acknowledgment from PS (ps_control == 0)

    // State register
    always @(posedge clk) begin
        if (reset)
          state <= 0;
        else
          state <= next_state;
    end

    // Next state logic
    always @* begin
        if (state == 0) begin
            if (ps_control[0] == 1)
                next_state = 1;
            else
                next_state = 0;  
        end
        else if (state == 1) begin
            if (bram_0_addr == 8188)
                next_state = 2;
            else
                next_state = 1;
        end
        else if (state == 2) begin
            next_state = 3;
        end
        else begin
            if (ps_control[0] == 0)
                next_state = 0;
            else
                next_state = 3;
        end
    end

    // BRAM 0 address register
    always @(posedge clk) begin
        if (state == 0)
            if (ps_control[0] == 1)
                bram_0_addr <= 4;
            else
                bram_0_addr <= 0;
        else if (state == 1)
            bram_0_addr <= bram_0_addr+4;
    end

    // BRAM 1 address 
    // Note the -4 is needed b/c we write 1 cycle after we read
    always @(*) begin
        if (state == 1)
            bram_1_addr = 8188-(bram_0_addr-4);
        else
            bram_1_addr = 0;
    end

    // We never need to write to BRAM 0:
    assign bram_0_wrdata = 0;
    assign bram_0_we     = 0;

    // BRAM 1 write data and enable:
    assign bram_1_wrdata = bram_0_rddata;
    assign bram_1_we = ((state == 1) || (state == 2)) ? 4'hf : 4'h0;

    // Status signal
    assign pl_status = (state == 3) ? 1 : 0;

endmodule

// This module contains two BRAMs (each of 2048 words by 4 bytes per word) 
// and three ports.
//
// Port A: allows the PS to read and write from both BRAMs. 
//     Addresses 0 through 8188 (0x0000 through 0x1FFC) will address to BRAM 0 at 
//               the same addresses
//     Addresses 8192 through 16380 (0x2000 through 0x3FFC) will address to BRAM 1 at
//               addresses 0 through 8188.
//
//     Therefore we can use addr_a[13] to determine whether port A should access
//     BRAM 0 or 1; in either case, the port will access addrress addr_a[12:0].
//
// Port B: allows the datapath to read and write BRAM 0
// Port C: allows the datapath to read and write BRAM 1
//                       
// Assumptions:
//    - The address inputs are byte-addressed (to match default behavior of AXI BRAM controller)
//    - However this memory module supports only full-word writes
module memSys(clk_a, addr_a, wrdata_a, rddata_a, wr_a, en_a, clk_b, addr_b, wrdata_b, rddata_b, wr_b, 
              clk_c, addr_c, wrdata_c, rddata_c, wr_c);

    parameter     DEPTH = 2048;       // for each of the two BRAMs in this system
    localparam    ADDRWIDTH = $clog2(DEPTH)+2;
    localparam    WIDTH = 32;

    // Port A
    input                    clk_a;
    input  [(ADDRWIDTH):0]   addr_a;    // Note: addr_a has 1 more bit than addr_b or addr_c
    input  [(WIDTH-1):0]     wrdata_a;
    output [(WIDTH-1):0]     rddata_a;
    input  [3:0]             wr_a;
    input                    en_a;

    // Port B
    input                    clk_b;
    input  [(ADDRWIDTH-1):0] addr_b;
    input  [(WIDTH-1):0]     wrdata_b;
    output [(WIDTH-1):0]     rddata_b;
    input  [3:0]             wr_b;


    // Port C
    input                    clk_c;
    input  [(ADDRWIDTH-1):0] addr_c;
    input  [(WIDTH-1):0]     wrdata_c;
    output [(WIDTH-1):0]     rddata_c;
    input  [3:0]             wr_c;


    // Logic to share port A between the two BRAMs
    reg    [(WIDTH-1):0]     rddata_a;
    wire   [(WIDTH-1):0]     bram_0_rd_data_a, bram_1_rd_data_a;
    reg    [3:0]             bram_0_wr_a, bram_1_wr_a;

    // Writing is easy: based on address, decide which BRAM write enable to possibly assert
    always @* begin
        if (addr_a[ADDRWIDTH] == 1'b0) begin
            bram_0_wr_a = wr_a;
            bram_1_wr_a = 0;
        end
        else begin
            bram_0_wr_a = 0;
            bram_1_wr_a = wr_a;
        end
    end

    // Reading is also relatively simple: we can use the address to determine which BRAM
    // to connect to the rddata_a read port. However there is one pitfall: because there
    // is a cycle of latency between when we request the read and when we get the data, 
    // we need to control this based on the address *one cycle ago*
    reg                      rd_src;
    always @(posedge clk_a)
        rd_src <= addr_a[ADDRWIDTH];

    always @* begin
        if (rd_src == 1'b0) begin
            rddata_a    = bram_0_rd_data_a;
        end
        else begin
            rddata_a = bram_1_rd_data_a;
        end
    end


    // Instantiate two BRAM modules
    bramMod #(DEPTH) BRAM_0(.clk_a(clk_a), .addr_a(addr_a[(ADDRWIDTH-1):0]), 
                            .wrdata_a(wrdata_a), .rddata_a(bram_0_rd_data_a),
                            .wr_a(bram_0_wr_a), .en_a(en_a), .clk_b(clk_b), .addr_b(addr_b), 
                            .wrdata_b(wrdata_b), .rddata_b(rddata_b), .wr_b(wr_b));

    bramMod #(DEPTH) BRAM_1(.clk_a(clk_a), .addr_a(addr_a[(ADDRWIDTH-1):0]), 
                            .wrdata_a(wrdata_a), .rddata_a(bram_1_rd_data_a),
                            .wr_a(bram_1_wr_a), .en_a(en_a), .clk_b(clk_c), .addr_b(addr_c), 
                            .wrdata_b(wrdata_c), .rddata_b(rddata_c), .wr_b(wr_c));

endmodule


// Assumptions:
//    - The address inputs are byte-addressed (to match default behavior of AXI BRAM controller)
//    - However this memory module supports only full-word writes
module bramMod(clk_a, addr_a, wrdata_a, rddata_a, wr_a, en_a, clk_b, addr_b, wrdata_b, rddata_b, wr_b);

    parameter     DEPTH = 2048;
    localparam    ADDRWIDTH = $clog2(DEPTH)+2;
    localparam    WIDTH = 32;

    input                    clk_a;
    input  [(ADDRWIDTH-1):0] addr_a;
    input  [(WIDTH-1):0]     wrdata_a;
    output [(WIDTH-1):0]     rddata_a;
    input  [3:0]             wr_a;
    input                    en_a;

    input                    clk_b;
    input  [(ADDRWIDTH-1):0] addr_b;
    input  [(WIDTH-1):0]     wrdata_b;
    output [(WIDTH-1):0]     rddata_b;
    input  [3:0]             wr_b;

    reg    [(WIDTH-1):0]     rddata_a;
    reg    [(WIDTH-1):0]     rddata_b;
    
    reg [(WIDTH-1):0] RAM[(DEPTH-1):0];

    always @(posedge clk_a) begin
        if (en_a) begin
            if (wr_a == 4'b1111)
                RAM[addr_a[(ADDRWIDTH-1):2]] <= wrdata_a;
            rddata_a <= RAM[addr_a[(ADDRWIDTH-1):2]];
        end
    end

    always @(posedge clk_b) begin
        if (wr_b == 4'b1111)
            RAM[addr_b[(ADDRWIDTH-1):2]] <= wrdata_b;
        rddata_b <= RAM[addr_b[(ADDRWIDTH-1):2]];
    end
endmodule


/*
 
module ctrlpath(
    input         clk, 
    input         reset, 
    output  [3:0] bram_we, 
    output        clr, 
    output        inc, 
    input         done,
    input  [31:0] ps_control,
    output [31:0] pl_status);
    
    // Current state register and next state signal
    reg [2:0]           state;
    reg [2:0]           next_state;

    // State machine function:
    // state 0: wait for ps_control == 1 (PS start signal)
    // state 1: activate datapath; stay until we are issuing last read request (addr=8188)
    // state 2: compare the last word in memory with largest value
    // state 3: write result to memory
    // state 4: write status = 1 (done); wait for acknowledgment from PS (ps_control == 0)
    
    // State register
    always @(posedge clk) begin
        if (reset)
          state <= 0;
        else
          state <= next_state;
    end
    
    // Next state logic
    always @* begin
        if (state == 0) begin
            if (ps_control[0] == 1)
                next_state = 1;
            else
                next_state = 0;  
        end
        
        else if (state == 1) begin
            if (done == 1)
                next_state = 2;
            else
                next_state = 1;
        end

        else if (state == 2) 
            next_state = 3;
        
        else if (state == 3)
            next_state = 4;


        else if (state == 4) begin
            if (ps_control[0] == 0)
                next_state = 0;
            else
                next_state = 4;
        end

        else 
            next_state = 0;
    end 


    // Assign output values
    assign bram_we = (state == 3) ? 4'hf : 4'h0;
    assign clr = (state == 0);
    assign inc = (state == 1);
    assign pl_status = (state == 4) ? 1 : 0;
        
endmodule
     
*/
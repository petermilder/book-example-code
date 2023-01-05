/*   
    An example design that reads values from a BRAM, finds the largest value,
    and stores the largest value into address 0. 

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

module maxval(
    input         clk,
    input         reset,
    input  [31:0] ps_control,
    output [31:0] pl_status,
    output [12:0] bram_addr,
    input  [31:0] bram_rddata,
    output [31:0] bram_wrdata,
    output [3:0]  bram_we);

    wire clr, inc, done;

    datapath dp(.clk(clk), .reset(reset), .bram_addr(bram_addr), .bram_rddata(bram_rddata), 
                .bram_wrdata(bram_wrdata), .clr(clr), .inc(inc), .done(done));
                     
    ctrlpath cp(.clk(clk), .reset(reset), .bram_we(bram_we), .clr(clr), .inc(inc), 
                .done(done), .ps_control(ps_control), .pl_status(pl_status));

endmodule


module datapath(
    input             clk,
    input             reset,
    output reg [12:0] bram_addr,
    input      [31:0] bram_rddata,
    output     [31:0] bram_wrdata,
    input             clr,
    input             inc,
    output            done);
    
    wire              load;
    reg        [31:0] largest;

    // Register to store largest value
    always @(posedge clk) begin
        if (clr)
            largest <= 0;
        else if (load)
            largest <= bram_rddata;
    end
    
    // If the value we read from BRAM is larger than the largest so far,
    // set load signal
    assign load = bram_rddata > largest;
    
    // Incrementer
    always @(posedge clk) begin
        if (clr)
            bram_addr <= 0;
        else if (inc) 
            bram_addr <= bram_addr+4;    
    end
    
    // Assert done signal when we are on the last word
    assign done = (bram_addr == 8188);    

    // Connect the "largest" seen value back to the BRAM's write port
    assign bram_wrdata = largest;
        
endmodule
 
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
     

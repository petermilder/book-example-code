/*   
    An example AXI-Stream multiplier 
                    
    From Section 8.5 of "Getting Started with the Xilinx Zynq 
    FPGA and Vivado" by Peter Milder (peter.milder@stonybrook.edu)

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


`timescale 1 ns / 1 ps

module streammult_v1_0 #(
        parameter integer C_S00_AXIS_TDATA_WIDTH = 32,
        parameter integer C_M00_AXIS_TDATA_WIDTH = 32,
        parameter integer C_M00_AXIS_START_COUNT = 32)
    (
        input wire                     s00_axis_aclk,
        input wire                     s00_axis_aresetn,
        output wire                    s00_axis_tready,
        input wire [C_S00_AXIS_TDATA_WIDTH-1 : 0]  s00_axis_tdata,
        input wire                     s00_axis_tlast,
        input wire                     s00_axis_tvalid, 
        input wire                     m00_axis_aclk,
        input wire                     m00_axis_aresetn,
        output wire                    m00_axis_tvalid,
        output wire [C_M00_AXIS_TDATA_WIDTH-1 : 0] m00_axis_tdata,
        output wire                    m00_axis_tlast,
        input wire                     m00_axis_tready);

    // State names
    localparam STATEIN = 0, STATECOMP = 1, STATEOUT = 2;
    
    // ---------------- datapath ------------------

    // Enable signals for registers
    wire en_ab, en_c;

    // Registers for a, b, and c.
    reg signed [15:0] a, b;
    reg signed [31:0] c;

    // Registers for tlast signals
    reg tlast_ab, tlast_c;

    // Registers
    always @(posedge s00_axis_aclk) begin
        if (s00_axis_aresetn == 0) begin
            a <= 0;
            b <= 0;
            tlast_ab <= 0;
            c <= 0;
            tlast_c <= 0;
        end        
        else begin
            if (en_ab) begin
                a <= s00_axis_tdata[31:16];
                b <= s00_axis_tdata[15:0];
                tlast_ab <= s00_axis_tlast;
            end

            if (en_c) begin
                c <= a*b;
                tlast_c <= tlast_ab;
            end
        end  
    end

    assign m00_axis_tdata = c;
    assign m00_axis_tlast = tlast_c;   
    
    // ---------------- FSM ---------------

    // State and next state
    reg [1:0] state, next_state;

    // State register
    always @(posedge s00_axis_aclk) begin
        if (s00_axis_aresetn == 0)
            state <= 0;
        else 
            state <= next_state;
    end

    // Combinational next state logic
    always @* begin
        if (state == STATEIN)
            if (s00_axis_tvalid)
                next_state = STATECOMP;
            else
                next_state = STATEIN;
       
        else if (state == STATECOMP)
            next_state = STATEOUT;
       
        else
            if (m00_axis_tready)
                next_state = STATEIN;
            else
                next_state = STATEOUT;
    end

    // Combinational FSM output logic
    assign s00_axis_tready = (state == STATEIN);
    assign en_ab           = (state == STATEIN);
    assign en_c            = (state == STATECOMP);
    assign m00_axis_tvalid = (state == STATEOUT);

endmodule

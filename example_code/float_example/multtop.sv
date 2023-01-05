/*
    An example design that instantiates a generated floating point multiplier.

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

module multtop(clk, reset, valid_in, valid_out, in0, in1, out0);
    input clk, reset, valid_in;
    output logic valid_out;
    input [31:0] in0, in1;
    output logic [31:0] out0;

    logic [31:0] in0_reg, in1_reg, out_tmp;
    logic valid_reg, valid_tmp; 


    always_ff @(posedge clk) begin
        if (reset == 1'b1) begin
            in0_reg <= 0;
            in1_reg <= 0;           
            valid_reg <= 0;
        end
        else begin
            in0_reg <= in0;
            in1_reg <= in1;
            valid_reg <= valid_in;
        end
    end

    mymult mult_inst (
        .aclk(clk),                       
        .s_axis_a_tvalid(valid_reg),      
        .s_axis_a_tdata(in0_reg),
        .s_axis_b_tvalid(valid_reg),
        .s_axis_b_tdata(in1_reg),
        .m_axis_result_tvalid(valid_tmp),
        .m_axis_result_tdata(out_tmp)
    );

    always_ff @(posedge clk) begin
        if (reset == 1'b1) begin
            out0 <= 0;
            valid_out = 0;
        end
        else begin
            out0 <= out_tmp;
            valid_out <= valid_tmp;
        end
    end
endmodule

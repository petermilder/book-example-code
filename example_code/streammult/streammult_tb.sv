/*   
    Testbench for simulating streamult.v or streammult2.v
                    
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


module streammult_tb();
    parameter NUMTESTS = 10000;   

    // The streammult_v1_0 module is our design under test (DUT).
    // There is one potential place for confusion here: the DUT takes input
    // on an AXI4-Stream slave port, and it produces its output on an AXI4-Stream
    // master port.

    // So, this testbench uses an AXI4-Stream *master* port to provide the DUT's
    // test data. Similarly, the testbench's AXI4-Stream *slave* port receives
    // the DUT's output data. So the place for confusion is that: this module's master
    // connects to the DUT's slave and vice-versa.

    // To help clarify this, this testbench uses signals named tb_m_* to represent signals
    // in the testbench's master (which will connect to the DUT's slave port).
    // Similarly, we will use signals named tb_s_* to represent signals in the
    // testbench's slave port (which connects to the DUT's master).

    reg clk, resetn;

    
    wire       tb_m_tready;
    reg [31:0] tb_m_tdata;
    reg        tb_m_tlast, tb_m_tvalid;
    
    reg         tb_s_tready;
    wire [31:0] tb_s_tdata;
    wire        tb_s_tlast, tb_s_tvalid;

    initial clk=0;
    always #5 clk = ~clk;

    streammult_v1_0 dut(
        .s00_axis_aclk(clk),
        .s00_axis_aresetn(resetn),
        .s00_axis_tready(tb_m_tready),
        .s00_axis_tdata(tb_m_tdata),
        .s00_axis_tlast(tb_m_tlast),
        .s00_axis_tvalid(tb_m_tvalid),
        .m00_axis_aclk(clk),
        .m00_axis_aresetn(resetn),
        .m00_axis_tvalid(tb_s_tvalid),
        .m00_axis_tdata(tb_s_tdata),
        .m00_axis_tlast(tb_s_tlast),
        .m00_axis_tready(tb_s_tready)
    );

    // randomly generate control
    reg s;
    reg [1:0] r; // a random 2-bit value we will use to ensure random behavior in testbench

    always begin
        @(posedge clk);      
        #1;    
        // when simulating, Vivado may flag the following line as an error, but you should
        // still be able to run the simulator:  
        s=std::randomize(r);
    end

    // Count the number of inputs loaded to the DUT
    reg [31:0] inputsDone;

    // Generate the tb_m_tvalid signal. If the random bit r[0]==1 and
    // we are not done testing, set tb_m_tvalid to 1. Else 0.
    always @* begin
        if ((inputsDone >= 0) && (inputsDone < NUMTESTS) && (r[0]==1'b1))
            tb_m_tvalid=1;      
        else
            tb_m_tvalid=0;      
    end

    // Generate input data.
    // Only provide data to design when tb_m_tvalid == 1.
    // For the test data, we are just incrementing as we go, so first we test 0*0, then 1*1, etc.
    // Obvioulsy this is not a very strong set of tests for this problem, especially because
    // we are not testing any negative inputs or outputs. A good exercise would be for youto improve 
    // this.
    always @* begin
        if (tb_m_tvalid == 1)
            tb_m_tdata = {inputsDone[15:0], inputsDone[15:0]};      
        else
            tb_m_tdata = 'x;     
    end

    // Provide a tlast signal, == 1 on the last input.
    always @* begin
        if ((tb_m_tvalid == 1) && (inputsDone == NUMTESTS-1))
            tb_m_tlast = 1;
        else
            tb_m_tlast = 0;      
    end
    
    
    // On each clock cycle, if the DUT slave port's valid and ready signals are both 1,
    // we increment the number of tests done.
    always @(posedge clk) begin
        if (tb_m_tvalid && tb_m_tready)
            inputsDone <= #1 inputsDone+1;   
    end

    // Count of number of outputsReceived from the DUT
    logic [31:0] outputsReceived;

    // Generate the tb_s_tready signal. If the random bit r[1]==1 and
    // we have more outputs to receive, set tb_s_tready to 1. Else 0.
    always @* begin
        if ((outputsReceived >= 0) && (outputsReceived < NUMTESTS) && (r[1]==1'b1))
            tb_s_tready = 1;      
        else
            tb_s_tready = 0;      
    end

    logic [31:0] errors;
 
    logic signed [15:0] signed_input0, signed_input1;
    logic signed [31:0] product;

    // On each clock posedge, if the DUT's master port's valid and ready signals are both 1,
    // then we display the output signal received and incrment the number of test outputs recv'd.
    // We also check that tlast==1 on the final word only.
    always @(posedge clk) begin
        if (tb_s_tready && tb_s_tvalid) begin

            // compute expected output
            signed_input0 = outputsReceived[15:0];
            signed_input1 = outputsReceived[15:0];
            product = signed_input0 * signed_input1;         
         
            // Check for correct product
            if (tb_s_tdata != product) begin
                $display($time,,"ERROR: Ouput number %d: Received output %d; expected value = %d", outputsReceived, tb_s_tdata, product);               
                errors = errors+1;
            end
         
            // If this is the last output, check that tlast is set.
            if (outputsReceived == NUMTESTS-1) begin
                if (tb_s_tlast != 1) begin
                    $display($time,,"ERROR: TLAST = %b; expected value = 1", tb_s_tlast);
                    errors = errors+1;
                end
            end

            // If this is not the last output, check that tlast is not set.
            else begin
                if (tb_s_tlast != 0)  begin
                    $display($time,,"ERROR: TLAST = %b; expected value = 0", tb_s_tlast);
                    errors = errors+1;
                end
            end   

            // Increment the number of outputs received.
            outputsReceived = outputsReceived + 1;  
        end
    end
    
    initial begin
        errors=0;
        
        inputsDone=0;
        outputsReceived=0;

        // Before first clock edge, initialize
        tb_s_tready = 0;
        tb_m_tvalid = 0;      

        // reset
        resetn = 0;
        @(posedge clk);
        #1;
        resetn = 1;

        // wait
        wait(outputsReceived==NUMTESTS);
        $display("Simulated %d outputs. Detected %d errors", NUMTESTS, errors);

        $finish;
    end
endmodule 


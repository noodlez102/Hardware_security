`timescale 1ns/1ps

module tb_counter_group();

    reg reset;
    reg [3:0] Cha0;
    reg [3:0] Cha1;

    reg [15:0] RO_out;

    wire Response;

    Counter_group dut(
    .reset(reset),
    .Cha0(Cha0),
    .Cha1(Cha1),
    .RO_out(RO_out),
    .Response(Response)
        );
    always #3 RO_out[0] = ~RO_out[0];
    always #5 RO_out[1] = ~RO_out[1];
    always #7 RO_out[2] = ~RO_out[2];
    always #11 RO_out[3] = ~RO_out[3];

    initial begin
        reset = 1;
        RO_out = 0;
        #20
        reset = 0;
        Cha0 = 0;
        Cha1 = 1;
        #200
        Cha0 = 2;
        Cha1 = 3;
        #200
        $stop;
    end
endmodule

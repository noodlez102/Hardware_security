`timescale 1ns/1ps

module tb_RO_basic();

  reg  Enable, clock;
  wire RO_out;

  RO_basic_sim dut (
    .Enable(Enable),
    .RO_out(RO_out)
  );

  always #5 clock=~clock;

  initial begin
    #10
    Enable= 1'b1;
    #100
    $stop;
  end
endmodule

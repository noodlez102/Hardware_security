module challenge_cycle #(
  parameter C_BITS = 4, // Challenge Bits
  parameter R_BITS = 4, // Response Bits
  parameter [2*4*C_BITS*R_BITS-1:0] DELAY = 4'd12 // Random Delay Values
) (
  input               reset,     // Active-high reset to set all registers to 0
  input               enable,    // Enable the PUF circuit
  input  [C_BITS-1:0] challenge, // The PUF challenge input
  output reg resp       // The PUF response output
);
        
wire top;
wire bottom;
    
assign top = enable;
assign bottom = enable;

genvar r;
generate
    for (r = 0; r < C_BITS; r = r + 1) begin : STAGES


        mux #(.DELAY(DELAY[4*(2*r+1)-1 -: 4])) mux_top (
        .a   (top),
        .b   (bottom),
        .sel (challenge[r]),
        .out (top)
        );

        mux #(.DELAY(DELAY[4*(2*r+2)-1 -: 4])) mux_bot (
        .a   (bottom),
        .b   (top),
        .sel (challenge[r]),
        .out (bottom)
        );
    end
endgenerate

always @(posedge bottom or posedge reset) begin
    if (reset)
      resp <= 1'b0;
    else
      resp <= top;
end

endmodule

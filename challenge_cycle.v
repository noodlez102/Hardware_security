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
        
wire [C_BITS:0] top;
wire [C_BITS:0] bottom;

genvar r;
generate
    for (r = 0; r < C_BITS; r = r + 1) begin : STAGES


        assign top[0] = enable;
        assign bottom[0] = enable;

        mux #(.DELAY(DELAY[4*(r+1)-1 -: 4])) mux_top (
        .a   (top[r]),
        .b   (bottom[r]),
        .sel (challenge[r]),
        .out (top[r+1])
        );

        mux #(.DELAY(DELAY[4*(r+2)-1 -: 4])) mux_bot (
        .a   (bottom[r]),
        .b   (top[r]),
        .sel (challenge[r]),
        .out (bottom[r+1])
        );
    end
endgenerate

always @(posedge bottom[C_BITS] or posedge reset) begin
    if (reset)
      resp <= 1'b0;
    else
      resp <= top[C_BITS];
end

endmodule

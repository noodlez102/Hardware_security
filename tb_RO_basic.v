`timescale 1ns/1ps

module tb_RO_basic;

  reg  Enable;
  wire RO_out;

  RO_basic_sim dut (
    .Enable(Enable),
    .RO_out(RO_out)
  );

  // Count rising edges on RO_out to verify oscillation
  integer toggle_count;
  initial toggle_count = 0;
  always @(posedge RO_out) toggle_count = toggle_count + 1;

  initial begin
    $dumpfile("tb_RO_basic.vcd");
    $dumpvars(0, tb_RO_basic);

    // ---------------------------------------------------
    // Test 1: Enable=0, RO should NOT oscillate
    // ---------------------------------------------------
    Enable = 0;
    #50;
    $display("Test 1 - Enable=0 : toggle_count=%0d (expect 0)", toggle_count);
    if (toggle_count == 0)
      $display("  PASS");
    else
      $display("  FAIL — RO oscillating when disabled");

    // ---------------------------------------------------
    // Test 2: Enable=1, RO should oscillate
    // Each gate has #1 delay, 5 gates total = period of 10ns
    // So in 100ns we expect ~10 toggles
    // ---------------------------------------------------
    toggle_count = 0;  // reset count for clean measurement
    Enable = 1;
    #100;
    $display("Test 2 - Enable=1 : toggle_count=%0d (expect ~10)", toggle_count);
    if (toggle_count > 0)
      $display("  PASS");
    else
      $display("  FAIL — RO not oscillating when enabled");

    // ---------------------------------------------------
    // Test 3: Enable=0 again, oscillation should stop
    // ---------------------------------------------------
    Enable = 0;
    #50;
    $display("Test 3 - Enable=0 again: RO_out=%b (expect stable)", RO_out);
    // Check no new toggles occurred after disable
    begin
      integer count_at_disable;
      count_at_disable = toggle_count;
      #50;
      if (toggle_count == count_at_disable)
        $display("  PASS — count stayed at %0d", toggle_count);
      else
        $display("  FAIL — count changed to %0d after disable", toggle_count);
    end

    $display("\nInternal wire states at end:");
    $display("  n[0]=%b n[1]=%b n[2]=%b n[3]=%b n[4]=%b",
             dut.n[0], dut.n[1], dut.n[2], dut.n[3], dut.n[4]);

    $stop;
  end

endmodule
```

---

### Expected output
```
Test 1 - Enable=0 : toggle_count=0 (expect 0)
  PASS
Test 2 - Enable=1 : toggle_count=10 (expect ~10)
  PASS
Test 3 - Enable=0 again: RO_out=0 (expect stable)
  PASS — count stayed at 10

Internal wire states at end:
  n[0]=0 n[1]=1 n[2]=0 n[3]=1 n[4]=1
```

The period calculation for your 5-gate chain with `#1` delays:
```

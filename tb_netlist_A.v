/** @module : tb_mm_uart
 *  @author : Secure, Trusted, and Assured Microelectronics (STAM) Center
 *  Copyright (c) 2022 (STAM/SCAI/ASU)
 */

module tb_netlist_A ();

localparam DATA_WIDTH         = 8;
localparam ADDR_WIDTH         = 8;
localparam UART_TXDATA_ADDR   = 8'd0;
localparam UART_RXDATA_ADDR   = 8'd4;

reg clock;
reg clock_baud;
reg reset;

// UART Rx/Tx
reg  uart_rx;
wire uart_tx;

// Memory Mapped Port
reg  readEnable;
reg  writeEnable;
reg  [DATA_WIDTH/8-1:0] writeByteEnable;
reg  [ADDR_WIDTH-1:0] address;
reg  [DATA_WIDTH-1:0] writeData;
wire [DATA_WIDTH-1:0] readData;
wire                  ready;

//Loopback tx --> rx
//assign uart_rx = uart_tx;

//define the log2 function
function integer log2;
  input integer num;
  integer i, result;
  begin
    for (i = 0; 2 ** i < num; i = i + 1)
      result = i + 1;
    log2 = result;
  end
endfunction

mm_uart DUT (
  .clock         (clock),
  .reset         (reset),
  .uart_rx       (uart_rx),
  .uart_tx       (uart_tx),
  // Memory Mapped Port
  .readEnable     (readEnable),
  .writeEnable    (writeEnable),
  .writeByteEnable(writeByteEnable),
  .address        (address),
  .writeData      (writeData),
  .readData       (readData),
  .ready           (ready)
);

//-----------------------------------------------------------//
//
// Tasks
//
//-----------------------------------------------------------//

// initialization task

task initialize;
begin
  $display("INITIALIZING...");
  clock = 1'b1;
  clock_baud =1'b1;
  uart_rx = 1'b1;
  reset = 1'b1;
  readEnable      = 1'b0;
  writeEnable     = 1'b0;
  writeByteEnable = 4'h0;
  address         = 32'h0;
  writeData       = 32'h0;

  repeat (3) @ (posedge clock);
  reset = 1'b0;
end
endtask

// register write task

task reg_write;
  input [31:0] addr;
  input [3:0]  wbe;
  input [31:0] wrdata;

  begin
    repeat (1) @ (posedge clock);
    $display(" REG WRITE -- ADDR=%08h at %tns" ,addr, $time);
    readEnable      = 1'h0;
    writeEnable     = 1'h1;
    writeByteEnable = wbe;
    writeData       = wrdata;
    address         = addr ;
  end
endtask

// register read task

task reg_read;
  input [31:0] addr;

  begin
    repeat (1) @ (posedge clock);
    $display(" REG READ --- ADDR=%08h at %tns" ,addr, $time);
    readEnable      = 1'h1;
    writeEnable     = 1'h0;
    writeByteEnable = 4'h0;
    address         = addr ;
    repeat (1) @ (posedge clock);
    #1
    $display("          --- DATA=%08h at %tns",readData, $time);
  end
endtask

task delay;
  input integer number;
  begin
    repeat (number) @ (posedge clock);
  end
endtask

task inactive;
  input integer number;
  begin
    readEnable      = 1'h0;
    writeEnable     = 1'h0;
    writeByteEnable = 4'h0;
    address         =32'h0 ;
    delay(number);
  end
endtask
//-----------------------------------------------------------//
//
// Simulation
//
//-----------------------------------------------------------//

//100MHz CLK
always #5 clock = ~clock;
always #50 clock_baud = ~clock_baud;

initial begin
  initialize;
  inactive(100);
  test_init;
  inactive(100);
  test_tx;
  inactive(100);
  test_rx;
  inactive(100);
  test_exit;
end

task test_init;
begin

  reg_read(UART_TXDATA_ADDR);
  reg_read(UART_RXDATA_ADDR);

end
endtask

task test_tx;
begin

  $display("TEST_TX...");
  //test byte enables on fifo
  reg_write (UART_TXDATA_ADDR , 4'h1, 32'h00FFEE01);
  reg_write (UART_TXDATA_ADDR , 4'h3, 32'h00FFEE02);
  reg_write (UART_TXDATA_ADDR , 4'h5, 32'h00FF0003);
  reg_write (UART_TXDATA_ADDR , 4'h7, 32'h00000004);
  reg_write (UART_TXDATA_ADDR , 4'h9, 32'h00000005);
  reg_write (UART_TXDATA_ADDR , 4'hB, 32'h00000006);
  reg_write (UART_TXDATA_ADDR , 4'hD, 32'h00000007);
  reg_write (UART_TXDATA_ADDR , 4'hF, 32'h00000008);
  inactive(1);


  //expected to be transmitting data , wait for FIFO empty
  fork : wait_tx_timeout
    begin
      #1_000_000; //#1ms
      $display ("timeout error waiting for tx data");
      $stop;
    end
    begin
      repeat(10000) @(posedge clock);
      disable wait_tx_timeout;
      // wait for last tx transaction
      repeat(11) @(posedge clock);
    end
  join
end
endtask

task reg_read_rx;
begin
  reg_read (UART_RXDATA_ADDR);
  $display ("          --- 'h%0h 'd%0d char %c" , readData, readData, readData);
  inactive(1);
end
endtask


task rx_data;
integer i;
input [7:0] char ;

begin
  $display ("UART RX transaction 'h%0h 'd%0d char %c",char,char,char);
//start bit
  repeat (1) @ (posedge clock_baud);
  uart_rx = 1'b0;
//data bits
//for (i= 7 ; i >=0 ; i=i-1) begin
  for (i= 0 ; i < 8 ; i=i+1) begin
    repeat (1) @ (posedge clock_baud);
    uart_rx = char[i];
  end
// stop bit
  repeat (1) @ (posedge clock_baud);
  uart_rx = 1'b1;
  repeat (5) @ (posedge clock_baud);
end
endtask


task test_rx;
begin
  uart_rx = 1'b1;

  inactive(1);

  // write to RX DATA
  // data should read 'testing!'

  rx_data('d84);
  rx_data('d69);
  rx_data('d83);
  rx_data('d84);
  rx_data('d73);
  rx_data('d78);
  rx_data('d71);
  rx_data('d33);

  delay(100);

  // we should see 10 entries in fifo
integer count;
count = 0;

// give time for RX logic to push into FIFO
delay(100);

fork : wait_rx_timeout
  begin
    #10_000;
    $display ("timeout error waiting for rx data");
    $stop;
  end
  begin
    // try to read 8 entries
    repeat (8) begin
      reg_read_rx;
      count = count + 1;
    end

    disable wait_rx_timeout;
  end
join

  // check we got exactly 8
  if (count !== 8) begin
    $display("ERROR :: expected 8 RX entries, got %0d", count);
    $stop;
  end else begin
    $display("found expected number of RX entries");
  end

endtask

task test_exit;
begin
  $display("\ntb_mm_uart --> Test Passed!\n\n");
  $stop;
end
endtask

endmodule

`timescale 1ns/1ps
// Verify pixel/active alignment while address changes EVERY video clock.
// Holding coordinates constant for many clocks cannot catch a one-cycle
// mismatch between the registered RAM output and selected pixel bit.
module tb_scan_stream;
 reg clk=0,reset=1,we=0,wh=0;
 reg [4:0] wr=0;
 reg [7:0] wc=0,wd=0,x=0,y=0;
 wire rh,active,pixel;
 wire [4:0] rr;
 wire [7:0] rc,rd;
 reg expect_pixel_d0=0, expect_pixel_d1=0;
 reg expect_active_d0=0, expect_active_d1=0;
 integer cycle=0;
 always #5 clk=~clk;
 cn_osd240_ram memory(.clk(clk),.reset(reset),.write_enable(we),
  .write_header(wh),.write_row(wr),.write_column(wc),.write_data(wd),
  .read_header(rh),.read_row(rr),.read_column(rc),.read_data(rd));
 cn_osd240_pixel scanner(.clk(clk),.reset(reset),.x(x),.y(y),
  .mode_224p(1'b0), .ram_data(rd),.ram_header(rh),.ram_row(rr),.ram_column(rc),
  .pixel(pixel),.active(active));
 task put(input bank,input [4:0] row,input [7:0] col,input [7:0] data);
 begin
  @(negedge clk);we=1;wh=bank;wr=row;wc=col;wd=data;
  @(negedge clk);we=0;
 end endtask
 initial begin
  put(1,0,0,8'h01); // header y=0 -> pixel=1
  put(1,0,1,8'h00); // header x=1 -> pixel=0
  put(0,0,0,8'h80); // body y=31 -> pixel=1
  put(0,0,1,8'h00); // body x=1 -> pixel=0
  @(negedge clk);reset=0;
  // Each input is new on consecutive pixel clock cycles.
  for(integer i=0;i<80;i=i+1) begin
    @(negedge clk);
    case(i%6)
     0: begin y=0; x=0; end
     1: begin y=0; x=1; end
     2: begin y=31; x=0; end
     3: begin y=31; x=1; end
     4: begin y=248; x=0; end
     5: begin y=255; x=0; end
    endcase
    // Expectations are computed from the known initialized bytes.
    expect_active_d0 = (y<248);
    expect_pixel_d0 = ((y==0 || y==31) && x==0);
    @(posedge clk); #1;
    // The pixel stage consumes the PREVIOUS registered memory byte
    // and PREVIOUS selector, so the output reflects sample i-1.
    if(i>=1 && (active!==expect_active_d1 ||
                (expect_active_d1 && pixel!==expect_pixel_d1))) begin
      $display("FAIL stream i=%0d x=%0d y=%0d got %b/%b expected %b/%b",
       i,x,y,active,pixel,expect_active_d1,expect_pixel_d1);
      $fatal(1);
    end
    expect_active_d1=expect_active_d0;
    expect_pixel_d1=expect_pixel_d0;
  end
  $display("PASS: 80 consecutive video samples maintain pixel/active alignment");
  $finish;
 end
endmodule

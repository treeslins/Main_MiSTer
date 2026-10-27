`timescale 1ns/1ps
module tb_224_256;
 reg clk=0,reset=1,mode=0;
 reg [7:0] y=0;
 wire hdr;wire [4:0] row;wire [7:0] col;wire pixel,active;
 always #5 clk=~clk;
 cn_osd240_pixel dut(.clk(clk),.reset(reset),.x(8'd0),.y(y),
  .mode_224p(mode),.ram_data(8'hff),.ram_header(hdr),
  .ram_row(row),.ram_column(col),.pixel(pixel),.active(active));
 task check(input m,input [7:0] pos,input valid,input expected_header);
 begin
  @(negedge clk);mode=m;y=pos;
  repeat(3) @(posedge clk);
  #1;
  if(active!==valid || hdr!==expected_header)
   $fatal(1,"mode=%b y=%0d active=%b expected=%b hdr=%b expected=%b",m,pos,active,valid,hdr,expected_header);
 end endtask
 initial begin
 @(negedge clk);reset=0;
 check(0,0,1,1);check(0,23,1,1);
 check(0,24,1,0);check(0,247,1,0);
 check(0,248,0,0);check(0,255,0,0);
 check(1,0,1,1);check(1,23,1,1);
 check(1,24,1,0);check(1,215,1,0);
 check(1,216,0,0);check(1,223,0,0);
 check(1,231,0,0);check(1,239,0,0);
 $display("PASS: 224p 12 rows and 256p 14 rows with 24px header and 8px margin");
 $finish;
 end
endmodule

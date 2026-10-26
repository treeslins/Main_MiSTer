`timescale 1ns/1ps
module tb_dualclock;
reg cs=0,cv=0,we=0,wh=0,rh=0;
reg [4:0] wr=0,rr=0;
reg [7:0] wc=0,wd=0,rc=0;
wire [7:0] value;
always #5 cs=~cs;
always #7 cv=~cv;
cn_osd240_dualclock_ram dut(
 .clk_sys(cs),.wr_en(we),.wr_header(wh),.wr_row(wr),.wr_col(wc),.wr_data(wd),
 .clk_video(cv),.rd_header(rh),.rd_row(rr),.rd_col(rc),.rd_data(value));
task put(input bank,input [4:0] row,input [7:0] col,input [7:0] data);
begin
 @(negedge cs);we=1;wh=bank;wr=row;wc=col;wd=data;
 @(negedge cs);we=0;
end endtask
task check(input bank,input [4:0] row,input [7:0] col,input [7:0] expected);
begin
 @(negedge cv);rh=bank;rr=row;rc=col;
 @(posedge cv);#1;
 if(value!==expected) $fatal(1,"dualclock read mismatch bank=%b row=%0d expected=%h got=%h",bank,row,expected,value);
end endtask
initial begin
 put(0,0,0,8'h11);
 put(0,25,255,8'h9a);
 put(1,0,0,8'h42);
 put(1,2,255,8'h81);
 check(0,0,0,8'h11);
 check(0,25,255,8'h9a);
 check(1,0,0,8'h42);
 check(1,2,255,8'h81);
 check(0,26,0,0);
 check(1,3,0,0);
 $display("PASS: independent SPI/video clocks, bank separation and bounds");
 $finish;
end
endmodule

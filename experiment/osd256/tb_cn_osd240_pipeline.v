`timescale 1ns/1ps
module tb_pipeline;
reg clk=0, reset=1,cv=0,dv=0;
reg [7:0] cmd=0, payload_byte=0,x=0,y=0;
wire ready,legacy,pixel,active;
cn_osd240_pipeline dut(.clk(clk),.reset(reset),.command(cmd),.command_valid(cv),
 .data_byte(payload_byte),.data_valid(dv),.pixel_x(x),.pixel_y(y),
 .extended_ready(ready),.legacy_command(legacy),.pixel(pixel),.active(active));
always #5 clk=~clk;

task issue(input [7:0] value);
begin
 @(negedge clk);cmd=value;cv=1;
 @(negedge clk);cv=0;
end endtask

task write_byte(input [7:0] value);
begin
 @(negedge clk); payload_byte=value;dv=1;
 @(negedge clk);dv=0;
end endtask

task check_pixel(input [7:0] pos_y, input expected_active, input expected_pixel);
begin
 @(negedge clk);x=0;y=pos_y;
 repeat(5) @(posedge clk);
 #1;
 if(active!==expected_active || (expected_active && pixel!==expected_pixel)) begin
   $display("FAIL y=%0d active=%b pixel=%b expected=%b/%b",
      pos_y,active,pixel,expected_active,expected_pixel);
   $fatal(1);
 end
end endtask

initial begin
 @(negedge clk);reset=0;
 // Before handshake extended payload must not write any experimental RAM.
 issue(8'hA0);write_byte(8'hFF);
 if(ready!==0) $fatal(1,"extension incorrectly on before handshake");
 issue(8'hE0);issue(8'h43);issue(8'h4E);issue(8'h32);issue(8'h34);
 if(ready!==1) $fatal(1,"extension handshake failed");
 // Body physical row 0 bit 0 -> 240p y=24
 issue(8'hA0);write_byte(8'h01);
 // Body physical row 25 bit 7 -> 240p y=231
 issue(8'hB9);write_byte(8'h80);
 // Header physical row 0 bit 0 -> 240p y=0
 issue(8'hC0);write_byte(8'h01);
 // Header physical row 2 bit 7 -> 240p y=23
 issue(8'hC2);write_byte(8'h80);
 check_pixel(0,1,1);
 check_pixel(1,1,0);
 check_pixel(23,1,1);
 check_pixel(24,1,1);
 check_pixel(25,1,0);
 check_pixel(231,1,1);
 check_pixel(232,0,0);
 check_pixel(239,0,0);
 $display("PASS: protocol->separate SRAM banks->240p scan boundaries");
 $finish;
end
endmodule

`timescale 1ns/1ps
module tb_vscan;
 reg clk=0, reset=1, start=0, step=0;
 wire [7:0] y;wire running;
 always #5 clk=~clk;
 cn_osd240_vscan dut(.clk_video(clk),.reset(reset),
 .frame_start(start),.line_step(step),.y(y),.running(running));
 task advance;
 begin @(negedge clk);step=1;@(negedge clk);step=0;end endtask
 initial begin
 repeat(2) @(negedge clk);reset=0;start=1;
 @(negedge clk);start=0;
 if(y!==0 || !running) $fatal(1,"bad start");
 repeat(240) advance();
 if(y!==240 || !running) $fatal(1,"scan truncated at %d",y);
 repeat(15) advance();
 if(y!==255 || !running) $fatal(1,"bad final address");
 advance();
 if(running) $fatal(1,"Y wrapped without new frame");
 start=1;step=1;@(negedge clk);start=0;step=0;
 if(y!==0 || !running) $fatal(1,"frame priority failed");
 $display("PASS: full 240-line scan beyond legacy 128px wrap");$finish;
 end
endmodule

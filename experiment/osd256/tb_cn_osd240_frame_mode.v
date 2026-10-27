`timescale 1ns/1ps
module tb_frame_mode;
 reg clk=0,reset=1,request=0,vs=0;
 wire actual;
 always #5 clk=~clk;
 cn_osd240_frame_mode dut(.clk_video(clk),.reset(reset),
 .requested_224p_async(request),.vs_in(vs),.mode_224p(actual));
 task tick(input integer n);
 begin repeat(n) @(negedge clk); end endtask
 initial begin
 tick(3); reset=0;
 request=1;
 tick(5);
 if(actual!==0) $fatal(1,"mode switched in middle of frame");
 vs=1;
 tick(2);
 if(actual!==1) $fatal(1,"failed to select 224p at rising VSYNC");
 request=0;
 tick(5);
 if(actual!==1) $fatal(1,"changed mode mid-frame");
 // Holding VS high is not a new frame event.
 tick(5);
 if(actual!==1) $fatal(1,"switched with VS held high");
 vs=0;tick(4);vs=1;tick(2);
 if(actual!==0) $fatal(1,"failed to select 240p at next frame");
 reset=1;tick(2);
 if(actual!==0) $fatal(1,"reset did not restore 240p default");
 $display("PASS: frame-atomic 224p/240p latch, held VSYNC and reset");
 $finish;
 end
endmodule

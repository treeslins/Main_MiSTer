`timescale 1ns/1ps
module tb_scan_pulses;
 reg clk=0,reset=1,ce=0,line_start=0,advance=0,frame_event=0;
 wire line_step,frame_start;
 integer i;
 always #5 clk=~clk;
 cn_osd240_scan_pulses dut(.clk_video(clk),.reset(reset),.ce_pix(ce),
   .line_start(line_start),.line_advance(advance),
   .frame_start_event(frame_event),.line_step(line_step),
   .frame_start(frame_start));
 task tick;
 begin @(posedge clk); #1; end
 endtask
 initial begin
   tick(); @(negedge clk); reset=0;
   ce=1; line_start=1; advance=1; tick();
   if(line_step!==1 || frame_start!==0) $fatal(1,"line event not emitted");
   @(negedge clk); ce=0; line_start=0; advance=0;
   for(i=0;i<5;i=i+1) begin
     tick();
     if(line_step!==0 || frame_start!==0)
       $fatal(1,"line pulse stretched across ce_pix gap");
   end
   @(negedge clk); ce=1; line_start=1; frame_event=1;
   tick();
   if(line_step!==0 || frame_start!==1) $fatal(1,"frame event not emitted");
   @(negedge clk); ce=0; line_start=0; frame_event=0;
   tick();
   if(line_step!==0 || frame_start!==0) $fatal(1,"frame pulse stretched");
   $display("PASS: upstream line/frame events produce single video-clock pulses");
   $finish;
 end
endmodule

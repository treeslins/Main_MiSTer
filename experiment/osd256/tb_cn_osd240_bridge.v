`timescale 1ns/1ps
module tb_bridge;
 reg clk=0,reset=1,cv=0,dv=0;
 reg [7:0] cmd=0,data=0;
 wire ready,we,header,legacy;
 wire [4:0] row;
 wire [7:0] col,wdata;
 cn_osd240_bridge dut(
 .clk(clk),.reset(reset),.command(cmd),.command_valid(cv),
 .data_byte(data),.data_valid(dv),.extended_ready(ready),
 .extended_write(we),.extended_header(header),.extended_row(row),
 .extended_column(col),.extended_data(wdata),.legacy_command(legacy));
 always #5 clk=~clk;
 task command(input [7:0] v);
 begin
 @(negedge clk);cmd=v;cv=1;
 @(negedge clk);cv=0;
 end endtask
 task payload(input [7:0] v);
 begin
 @(negedge clk);data=v;dv=1;
 #1;
 if(!we || wdata!==v) $fatal(1,"write not enabled for data %h",v);
 @(negedge clk);dv=0;
 end endtask
 initial begin
 @(negedge clk); reset=0;
 command(8'hA0);
 if(ready!==0) $fatal(1,"unexpected extended mode");
 command(8'hE0);command(8'h43);command(8'h10);
 if(ready!==0) $fatal(1,"invalid handshake");
 command(8'hE0);command(8'h43);command(8'h4E);
 command(8'h32);command(8'h34);
 if(ready!==1) $fatal(1,"handshake failed");
 command(8'hB9);
 if(header!==0 || row!==25 || col!==0) $fatal(1,"body bank wrong");
 payload(8'h80);
 if(col!==1) $fatal(1,"column not advancing");
 command(8'hC1);
 if(header!==1 || row!==1 || col!==0) $fatal(1,"header bank wrong");
 payload(8'hA1);
 if(col!==1) $fatal(1,"header column not advancing");
 // Simultaneous new command and stale payload must never write into RAM.
 @(negedge clk); cmd=8'hA1;cv=1;data=8'h5a;dv=1;
 #1; if(we!==0) $fatal(1,"command/data overlap permitted a stale write");
 @(negedge clk);cv=0;dv=0;
 command(8'hBF);
 if(we!==0) $fatal(1,"invalid row unexpectedly written");
 $display("PASS: opt-in 240p protocol rejects invalid handshake and dispatches banked writes");
 $finish;
 end
endmodule

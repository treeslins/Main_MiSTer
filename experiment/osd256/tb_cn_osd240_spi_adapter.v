`timescale 1ns/1ps
module tb_spi_adapter;
reg clk=0,reset=1,sel=0,strobe=0;
reg [15:0] din=0;
wire enabled,we,header,legacy,mode_224p;
wire [4:0] row;
wire [7:0] col,data;
cn_osd240_spi_adapter dut(.clk(clk),.reset(reset),.io_osd(sel),
 .io_strobe(strobe),.io_din(din),.enabled(enabled),.mode_224p(mode_224p),
 .ext_write(we),.ext_header(header),.ext_row(row),
 .ext_col(col),.ext_data(data),.legacy_passthrough(legacy));
always #5 clk=~clk;
task byte_in(input [7:0] v);
begin
 @(negedge clk);din={8'h0,v};strobe=1;
 @(negedge clk);strobe=0;
end endtask
task start_packet(input [7:0] command);
begin
 @(negedge clk);sel=1;
 byte_in(command);
end endtask
task end_packet;
begin
 @(negedge clk);sel=0;
 repeat(2) @(negedge clk);
end endtask
initial begin
 @(negedge clk);reset=0;
 start_packet(8'h20);byte_in(8'hff);end_packet();
 if(enabled!==0) $fatal(1,"legacy command enabled 240p");
 start_packet(8'hF0);byte_in(8'h43);byte_in(8'h4E);
 byte_in(8'h32);byte_in(8'h00);end_packet();
 if(enabled!==0) $fatal(1,"bad handshake accepted");
 start_packet(8'hF0);byte_in(8'h43);byte_in(8'h4E);
 byte_in(8'h32);byte_in(8'h34);end_packet();
 if(enabled!==1) $fatal(1,"valid handshake rejected");
 start_packet(8'hF4);byte_in(8'h01);end_packet();
 if(mode_224p!==1) $fatal(1,"224p mode select failed");
 start_packet(8'hF4);byte_in(8'h02);end_packet();
 if(mode_224p!==1) $fatal(1,"invalid mode changed selection");
 start_packet(8'hF4);byte_in(8'h00);end_packet();
 if(mode_224p!==0) $fatal(1,"240p mode select failed");
 start_packet(8'hF2);byte_in(25);
 byte_in(8'h80);
 #1;
 if(!we || header!==0 || row!==25 || col!==0 || data!==8'h80)
   $fatal(1,"body row 25 not written correctly");
 byte_in(8'h55);
 #1;
 if(!we || col!==1 || data!==8'h55) $fatal(1,"second byte mismatch");
 end_packet();
 start_packet(8'hF3);byte_in(1);byte_in(8'hAA);
 #1;
 if(!we || header!==1 || row!==1 || col!==0 || data!==8'hAA)
   $fatal(1,"header write mismatch");
 end_packet();
 start_packet(8'hF2);byte_in(26);byte_in(8'hEE);
 #1;
 if(we) $fatal(1,"out-of-range body write allowed");
 end_packet();
 start_packet(8'hF1);end_packet();
 if(enabled!==0) $fatal(1,"disable failed");
 start_packet(8'hF2);byte_in(0);byte_in(8'hff);
 #1;
 if(we) $fatal(1,"extension wrote while disabled");
 end_packet();
 $display("PASS: selected SPI transactions, handshake, row safety, disable");
 $finish;
end
endmodule

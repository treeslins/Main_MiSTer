`timescale 1ns/1ps
module tb_scan;
reg clk=0,reset=1;
reg [7:0] x=0,y=0,wd=0,wr=0,wc=0,rr=0,rc=0;
reg wh=0,we=0;
wire rh;wire [4:0] row;wire [7:0] col,byte_data;wire pixel,active;
always #5 clk=~clk;
cn_osd240_ram memory(.clk(clk),.reset(reset),.write_enable(we),
    .write_header(wh),.write_row(wr[4:0]),.write_column(wc),
    .write_data(wd),.read_header(rh),.read_row(row),.read_column(col),.read_data(byte_data));
cn_osd240_pixel scanner(.clk(clk),.reset(reset),.x(x),.y(y),.ram_data(byte_data),
    .ram_header(rh),.ram_row(row),.ram_column(col),.pixel(pixel),.active(active));
task put(input h,input [4:0] row_addr,input [7:0] data);
begin
    @(negedge clk);we=1;wh=h;wr={3'b0,row_addr};wc=0;wd=data;
    @(negedge clk);we=0;
end endtask
task sample(input [7:0] pos_y,input expected);
begin
    @(negedge clk);y=pos_y;x=0;
    repeat (4) @(posedge clk);
    #1;
    if(active!==expected) begin $display("invalid active y=%d got=%d expected=%d",pos_y,active,expected);$fatal(1);end
end endtask
initial begin
    @(negedge clk); reset=0;
    put(1,0,8'hff); // header row 0 (y0..7)
    put(1,2,8'hff); // header row 2 (y16..23)
    put(0,0,8'hff); // body first row (y24..31)
    put(0,25,8'hff); // body last row (y224..231)
    sample(0,1); if(pixel!==1) $fatal(1,"header top missing");
    sample(23,1); if(pixel!==1) $fatal(1,"header bottom missing");
    sample(24,1); if(pixel!==1) $fatal(1,"body top missing");
    sample(231,1); if(pixel!==1) $fatal(1,"body bottom missing");
    sample(232,0); if(pixel!==0) $display("NOTE: blank-area pixel is masked by active");
    sample(239,0);
    $display("PASS: 24px status + 208px body + 8px margin scan addressing");
    $finish;
end
endmodule

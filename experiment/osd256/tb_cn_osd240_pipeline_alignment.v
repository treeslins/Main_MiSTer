`timescale 1ns/1ps
module tb_pipeline_alignment;
 reg clk=0,reset=1,we=0,wh=0,mode=0;
 reg [4:0] wr=0;reg [7:0] wc=0,wd=0,x=0,y=0;
 wire rh;wire [4:0] rr;wire [7:0] rc,data;wire pixel,active;
 always #5 clk=~clk;
 cn_osd240_ram ram(.clk(clk),.reset(reset),.write_enable(we),
 .write_header(wh),.write_row(wr),.write_column(wc),.write_data(wd),
 .read_header(rh),.read_row(rr),.read_column(rc),.read_data(data));
 cn_osd240_pixel scan(.clk(clk),.reset(reset),.x(x),.y(y),
 .mode_224p(mode),.ram_data(data),.ram_header(rh),.ram_row(rr),
 .ram_column(rc),.pixel(pixel),.active(active));
 task write_byte(input h,input [4:0] row_i,input [7:0] col_i,input [7:0] val);
 begin @(negedge clk);we=1;wh=h;wr=row_i;wc=col_i;wd=val;
 @(negedge clk);we=0;end endtask
 // At clock N the RAM reads input N and scanner consumes RAM output N-1.
 // Registered scanner output N must equal input coordinate N-1.
 reg prior_valid=0,prior_bit=0;
 task point(input [7:0] px,input [7:0] py,input m,
            input expected_valid,input expected_pixel);
 begin
 @(negedge clk);x=px;y=py;mode=m;
 @(posedge clk);#1;
 if(active!==prior_valid || (prior_valid && pixel!==prior_bit))
   $fatal(1,"pipeline mismatch x=%0d y=%0d output valid=%b pixel=%b expected=%b/%b",
      px,py,active,pixel,prior_valid,prior_bit);
 prior_valid=expected_valid;prior_bit=expected_pixel;
 end endtask
 initial begin
 @(negedge clk);reset=0;
 write_byte(1,0,0,8'h01); // y=0 -> 1, y=1 -> 0
 write_byte(1,0,1,8'h02); // y=0 -> 0, y=1 -> 1
 write_byte(0,0,0,8'h01); // y=24 -> 1
 write_byte(0,0,1,8'h02); // y=25 -> 1
 point(0,0,0,1,1);
 point(1,0,0,1,0);
 point(1,1,0,1,1);
 point(0,1,0,1,0);
 point(0,24,0,1,1);
 point(1,25,0,1,1);
 point(0,232,0,0,0);
 point(0,216,1,0,0);
 point(0,24,1,1,1);
 point(0,232,0,0,0);
 // drain last issued pixel
 point(0,239,0,0,0);
 $display("PASS: moving coordinate pixel/data/valid alignment across RAM boundary");
 $finish;
 end
endmodule

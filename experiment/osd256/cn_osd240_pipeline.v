// SPDX-License-Identifier: GPL-3.0-or-later
// End-to-end 240p experimental datapath: command decoder -> banked RAM ->
// pixel scan. This module deliberately does NOT attach to MiSTer SPI wires.
// It is a verification harness for memory/protocol consistency.
module cn_osd240_pipeline(
 input wire clk,
 input wire reset,
 input wire [7:0] command,
 input wire command_valid,
 input wire [7:0] data_byte,
 input wire data_valid,
 input wire [7:0] pixel_x,
 input wire [7:0] pixel_y,
 output wire extended_ready,
 output wire legacy_command,
 output wire pixel,
 output wire active
);
 wire wr,wh,rh;
 wire [4:0] wr_row,rd_row;
 wire [7:0] wr_col,rd_col,wr_data,rd_data;
 cn_osd240_bridge decoder (
   .clk(clk), .reset(reset), .command(command), .command_valid(command_valid),
   .data_byte(data_byte), .data_valid(data_valid),
   .extended_ready(extended_ready), .extended_write(wr),
   .extended_header(wh), .extended_row(wr_row),
   .extended_column(wr_col), .extended_data(wr_data),
   .legacy_command(legacy_command));
 cn_osd240_ram memory (
   .clk(clk), .reset(reset), .write_enable(wr), .write_header(wh),
   .write_row(wr_row), .write_column(wr_col), .write_data(wr_data),
   .read_header(rh), .read_row(rd_row), .read_column(rd_col),
   .read_data(rd_data));
 cn_osd240_pixel reader (
   .clk(clk), .reset(reset), .x(pixel_x), .y(pixel_y),
   .ram_data(rd_data), .ram_header(rh), .ram_row(rd_row),
   .ram_column(rd_col), .pixel(pixel), .active(active));
endmodule

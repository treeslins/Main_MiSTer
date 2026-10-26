// SPDX-License-Identifier: GPL-3.0-or-later
// Dual-clock framebuffer for 240p experimental OSD.
// System/SPI writes use clk_sys, video reads use clk_video.
// One 256-byte-wide bitplane row = eight pixel scanlines.
module cn_osd240_dualclock_ram (
 input wire clk_sys,
 input wire wr_en,
 input wire wr_header,
 input wire [4:0] wr_row,
 input wire [7:0] wr_col,
 input wire [7:0] wr_data,
 input wire clk_video,
 input wire rd_header,
 input wire [4:0] rd_row,
 input wire [7:0] rd_col,
 output reg [7:0] rd_data
);
 localparam integer BODY_LINES=26;
 localparam integer HEADER_LINES=3;
 (* ramstyle="no_rw_check" *) reg [7:0] body [0:6655];
 (* ramstyle="no_rw_check" *) reg [7:0] header [0:767];
 wire [12:0] wa={wr_row,wr_col};
 wire [12:0] ra={rd_row,rd_col};
 always @(posedge clk_sys) begin
   if(wr_en)
     if(wr_header) begin
       if(wr_row<HEADER_LINES) header[wa]<=wr_data;
     end else if(wr_row<BODY_LINES) body[wa]<=wr_data;
 end
 always @(posedge clk_video) begin
   if(rd_header)
     rd_data <= (rd_row<HEADER_LINES) ? header[ra] : 8'h00;
   else
     rd_data <= (rd_row<BODY_LINES) ? body[ra] : 8'h00;
 end
endmodule

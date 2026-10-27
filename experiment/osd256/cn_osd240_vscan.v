// SPDX-License-Identifier: GPL-3.0-or-later
// Independent expanded-OSD scan Y counter. The legacy osd_vcnt has
// special header flags and a 128px wrap, so it is NOT a 240p coordinate.
// line_step and frame_start must be derived from the same video clock and
// upstream vertical divider. Frame start wins over line advance.
module cn_osd240_vscan (
 input wire clk_video, input wire reset,
 input wire frame_start, input wire line_step,
 output reg [7:0] y=0,
 output reg running=0
);
 always @(posedge clk_video) begin
   if (reset) begin y<=0;running<=0;end
   else if (frame_start) begin y<=0;running<=1;end
   else if (line_step && running) begin
     if(y != 8'hff) y<=y+8'd1;
     else running<=0;
   end
 end
endmodule

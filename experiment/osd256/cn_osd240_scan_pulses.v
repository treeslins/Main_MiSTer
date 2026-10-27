// SPDX-License-Identifier: GPL-3.0-or-later
// Convert upstream OSD line/frame events sampled at ce_pix into one-clock
// pulses for the expanded video-domain scan counter.
module cn_osd240_scan_pulses (
 input wire clk_video,
 input wire reset,
 input wire ce_pix,
 input wire line_start,
 input wire line_advance,
 input wire frame_start_event,
 output reg line_step=0,
 output reg frame_start=0
);
 always @(posedge clk_video) begin
   if(reset) begin
     line_step<=0;
     frame_start<=0;
   end else begin
     // Clear on every video clock, not only ce_pix. The scan counter consumes
     // the registered pulse on the following clock edge.
     line_step<=ce_pix && line_start && line_advance;
     frame_start<=ce_pix && line_start && frame_start_event;
   end
 end
endmodule

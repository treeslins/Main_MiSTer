// SPDX-License-Identifier: GPL-3.0-or-later
// Latch 224p/256p request on the selected incoming video frame edge.
// Explicit active-high rising-edge semantics; this is not automatic
// detection of VSYNC polarity. Integration must verify actual video polarity.
module cn_osd240_frame_mode(
 input wire clk_video,
 input wire reset,
 input wire requested_224p_async,
 input wire vs_in,
 output reg mode_224p=0
);
 (* ASYNC_REG="TRUE" *) reg mode_meta=0;
 (* ASYNC_REG="TRUE" *) reg mode_sync=0;
 reg vs_previous=0;
 always @(posedge clk_video) begin
   if(reset) begin
     mode_meta<=0;mode_sync<=0;
     mode_224p<=0;vs_previous<=0;
   end else begin
     mode_meta<=requested_224p_async;
     mode_sync<=mode_meta;
     vs_previous<=vs_in;
     if(!vs_previous && vs_in) mode_224p<=mode_sync;
   end
 end
endmodule

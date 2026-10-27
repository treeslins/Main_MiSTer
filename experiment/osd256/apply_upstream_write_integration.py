#!/usr/bin/env python3
"""Apply *opt-in* 240p write-bank hardware to an upstream Menu_MiSTer tree.
This is NOT the final 240p display implementation and MUST NOT be packaged
as a 240p menu.rbf. Called solely for syntax/elaboration integration checks.
"""
from pathlib import Path
import sys

root=Path(sys.argv[1])
p=root/"sys/osd.v"
text=p.read_text()
needle="reg        osd_enable;"
if text.count(needle)!=1:
    raise SystemExit("Upstream osd.v structure changed; refuse unsafe patch")
insertion="""// CN_OSD240_WRITE_EXPERIMENT (not yet complete video integration).
// This write-bank subsystem remains isolated from the legacy osd_buffer.
// Pixel-clock readout now samples the real legacy OSD scan counters.\n// Existing output mux still selects only legacy pixels.
wire cn240_enabled;
wire cn240_write, cn240_header;
wire [4:0] cn240_row;
wire [7:0] cn240_col, cn240_data;
wire [7:0] cn240_video_read;
wire cn240_video_header;
wire [4:0] cn240_video_row;
wire [7:0] cn240_video_col;
wire cn240_video_pixel, cn240_video_active;
// Explicit experimental video-height request, clocked by the SPI domain.
// It is not automatic video-mode detection. Default 240p on power-up.
wire cn240_mode_224p_sys;
(* ASYNC_REG="TRUE" *) reg cn240_mode_224p_meta=0;
(* ASYNC_REG="TRUE" *) reg cn240_mode_224p_sync=0;
reg cn240_mode_224p_video=0;
// Capture the requested mode only at the detected frame boundary.
// This does not yet validate legacy osd_vcnt scanning or video scaling.
reg cn240_vs_prev=0;
always @(posedge clk_video) begin
 cn240_mode_224p_meta <= cn240_mode_224p_sys;
 cn240_mode_224p_sync <= cn240_mode_224p_meta;
 cn240_vs_prev <= vs_in;
 if (!cn240_vs_prev && vs_in)
   cn240_mode_224p_video <= cn240_mode_224p_sync;
end
// Synchronize the static mode request into the video domain. This is NOT
// sufficient for synchronizing framebuffer writes or frame-atomic switching.
(* ASYNC_REG="TRUE" *) reg cn240_enabled_meta=0;
(* ASYNC_REG="TRUE" *) reg cn240_enabled_video=0;
always @(posedge clk_video) begin
 cn240_enabled_meta <= cn240_enabled;
 cn240_enabled_video <= cn240_enabled_meta;
end
wire cn240_preview_pixel = cn240_enabled_video &&
                            cn240_video_active && cn240_video_pixel;
// Demonstrate use of the genuine pixel clock and a registered read port.
// Display mux is NOT switched to this path until timing is validated.\n// The SPI adapter must survive chip-select deselection and OSD disable:\n// it resets its per-transaction parser internally on !io_osd.\n// A future upstream hardware reset should use a dedicated reset signal.
cn_osd240_pixel cn240_reader (
 .clk(clk_video), .reset(1'b0),
 .x(osd_hcnt[7:0]), .y(osd_vcnt[7:0]), .mode_224p(cn240_mode_224p_video), .ram_data(cn240_video_read),
 .ram_header(cn240_video_header), .ram_row(cn240_video_row),
 .ram_column(cn240_video_col), .pixel(cn240_video_pixel),
 .active(cn240_video_active)
);
cn_osd240_spi_adapter cn240_spi (
 .clk(clk_sys), .reset(1'b0),
 .io_osd(io_osd), .io_strobe(io_strobe), .io_din(io_din),
 .enabled(cn240_enabled), .mode_224p(cn240_mode_224p_sys),
 .ext_write(cn240_write),
 .ext_header(cn240_header), .ext_row(cn240_row),
 .ext_col(cn240_col), .ext_data(cn240_data),
 .legacy_passthrough()
);
cn_osd240_dualclock_ram cn240_bank (
 .clk_sys(clk_sys), .wr_en(cn240_write), .wr_header(cn240_header),
 .wr_row(cn240_row), .wr_col(cn240_col), .wr_data(cn240_data),
 .clk_video(clk_video), .rd_header(cn240_video_header),
 .rd_row(cn240_video_row), .rd_col(cn240_video_col),
 .rd_data(cn240_video_read)
);
"""
# Intentionally guard the original display implementation untouched.
text=text.replace(needle,insertion+"\n"+needle)
# Move existing OSD scan counters from the local clocked block to module scope
# so the independent experimental read path can observe their true values.
marker="\treg [21:0] osd_vcnt;"
marker2="\treg [21:0] osd_hcnt;"
if text.count(marker)!=1 or text.count(marker2)!=1:
    raise SystemExit("OSD scan-counter layout changed; refusing patch")
text=text.replace(marker,"",1).replace(marker2,"",1)
scope="reg        osd_enable;"
text=text.replace(scope,"reg [21:0] osd_vcnt;\nreg [21:0] osd_hcnt;\n"+scope,1)
# Compile-time gated first video-mux integration. It stays OFF in the
# ordinary upstream build; this only checks syntax of the opt-in branch.
# The actual 224p/240p CRT-safe coordinate and CDC design is still pending.
mux_anchor="\tordout1 <= {{osd_pixel, osd_pixel, OSD_COLOR[2], din[23:19]},"
assert text.count(mux_anchor)==1, "upstream video mixer changed"
text=text.replace(mux_anchor, """\t// The existing legacy output remains the default in all builds.
\t// ENABLE ONLY for waveform-level investigation; this does not yet
\t// implement safe scan scaling or a synchronized mode transition.
\t`ifdef CN_OSD240_PREVIEW_UNSAFE
\tordout1 <= {{cn240_preview_pixel, cn240_preview_pixel, OSD_COLOR[2], din[23:19]},
\t            {cn240_preview_pixel, cn240_preview_pixel, OSD_COLOR[1], din[15:11]},
\t            {cn240_preview_pixel, cn240_preview_pixel, OSD_COLOR[0], din[7:3]}};
\t`else
"""+mux_anchor,1)
end_anchor="\t            {osd_pixel, osd_pixel, OSD_COLOR[0], din[7:3]}}; //  7:0"
assert text.count(end_anchor)==1
text=text.replace(end_anchor,end_anchor+"\n\t`endif",1)
# Add the mode command as a separate selected SPI transaction (F4,
# one payload byte: 0 = 240p, 1 = 224p). The adapter exports its mode
# register; this wire assignment connects it to the two-stage synchronizer.
# The selected mode is still experimental and not read back by ARM.
p.write_text(text)
q=root/"sys/sys.qip"
q.write_text(q.read_text()+
 '\nset_global_assignment -name VERILOG_FILE [file join $::quartus(qip_path) cn_osd240_spi_adapter.v ]\n'+
 'set_global_assignment -name VERILOG_FILE [file join $::quartus(qip_path) cn_osd240_dualclock_ram.v ]\n'+
 'set_global_assignment -name VERILOG_FILE [file join $::quartus(qip_path) cn_osd240_pixel.v ]\n')
print("Applied scan-counter tap and dualclock RAM with persistent SPI mode; legacy video unchanged")

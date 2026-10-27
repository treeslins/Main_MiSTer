#!/usr/bin/env python3
"""Apply experimental 256p OSD read/write and output routing to Menu_MiSTer.
This is not hardware-validated and must not be represented as installable.
The resulting source is used for HDL elaboration and Quartus build attempts.
"""
from pathlib import Path
import sys

root=Path(sys.argv[1])
p=root/"sys/osd.v"
text=p.read_text()
needle="reg        osd_enable;"
if text.count(needle)!=1:
    raise SystemExit("Upstream osd.v structure changed; refuse unsafe patch")
insertion="""// CN_OSD240_INTEGRATED experimental video output path.
// This write-bank subsystem remains isolated from the legacy osd_buffer.
// Pixel-clock readout follows the real OSD scan timing and is confined to
// the 256x256 experiment window; outside it the legacy OSD remains selected.
wire cn240_enabled;
wire cn240_write, cn240_header;
wire [4:0] cn240_row;
wire [7:0] cn240_col, cn240_data;
wire [7:0] cn240_video_read;
wire cn240_video_header;
wire [4:0] cn240_video_row;
wire [7:0] cn240_video_col;
wire cn240_video_pixel, cn240_video_active;
wire cn240_line_step, cn240_frame_start;
// The official horizontal counter changes only while ce_pix is asserted.
// Keep the experiment's read-address coordinate stable across intermediate
// video clocks, and latch on ce_pix instead of using the combinational tap.
reg [21:0] cn240_scan_x_full=0;
wire [7:0] cn240_scan_x = cn240_scan_x_full[7:0];
// Sample the entire coordinate atomically, including its range bits.
always @(posedge clk_video) if(ce_pix) cn240_scan_x_full <= osd_hcnt;
wire [7:0] cn240_scan_y;
wire cn240_scan_running;
cn_osd240_vscan cn240_vscan (
 .clk_video(clk_video), .reset(1'b0),
 .frame_start(cn240_frame_start), .line_step(cn240_line_step),
 .y(cn240_scan_y), .running(cn240_scan_running)
);
cn_osd240_scan_pulses cn240_scan_events (
 .clk_video(clk_video), .reset(1'b0), .ce_pix(ce_pix),
 .line_start(de_in && !deD), .line_advance(osd_div == multiscan),
 .frame_start_event(v_osd_start == v_cnt),
 .line_step(cn240_line_step), .frame_start(cn240_frame_start)
);
// Explicit experimental video-height request, clocked by the SPI domain.
// It is not automatic video-mode detection. Default 256p on power-up.
wire cn240_mode_224p_requested;
wire cn240_mode_224p_video;
cn_osd240_frame_mode cn240_frame_latch (
 .clk_video(clk_video), .reset(1'b0),
 .requested_224p_async(cn240_mode_224p_requested), .vs_in(vs_in),
 .mode_224p(cn240_mode_224p_video)
);
// Synchronize the static mode request into the video domain. This is NOT
// sufficient for synchronizing framebuffer writes or frame-atomic switching.
(* ASYNC_REG="TRUE" *) reg cn240_enabled_meta=0;
(* ASYNC_REG="TRUE" *) reg cn240_enabled_video=0;
always @(posedge clk_video) begin
 cn240_enabled_meta <= cn240_enabled;
 cn240_enabled_video <= cn240_enabled_meta;
end
// Align the running gate to the registered RAM pixel and active outputs.
// Both control and pixel are sampled by the mux on the same video clock.
reg cn240_running_d1=0, cn240_running_d2=0;
// The official 22-bit OSD horizontal counter does not wrap at x=255.
// Mask the extended 256-column bank with the FULL counter to prevent
// repeating the same line on x=256,512,... due to low-byte truncation.
reg cn240_x_in_range_d1=0, cn240_x_in_range_d2=0;
always @(posedge clk_video) begin
 cn240_running_d1 <= cn240_scan_running;
 cn240_running_d2 <= cn240_running_d1;
 cn240_x_in_range_d1 <= (cn240_scan_x_full < 22'd256);
 cn240_x_in_range_d2 <= cn240_x_in_range_d1;
end
wire cn240_mux_pixel = (cn240_enabled_video && cn240_running_d2 &&
                        cn240_x_in_range_d2 && cn240_video_active) ?
                       cn240_video_pixel : osd_pixel;
// Use the genuine pixel clock and registered extension read port.
// The SPI adapter resets its per-transaction parser when !io_osd.
cn_osd240_pixel cn240_reader (
 .clk(clk_video), .reset(1'b0),
 .x(cn240_scan_x), .y(cn240_scan_y), .mode_224p(cn240_mode_224p_video), .ram_data(cn240_video_read),
 .ram_header(cn240_video_header), .ram_row(cn240_video_row),
 .ram_column(cn240_video_col), .pixel(cn240_video_pixel),
 .active(cn240_video_active)
);
cn_osd240_spi_adapter cn240_spi (
 .clk(clk_sys), .reset(1'b0),
 .io_osd(io_osd), .io_strobe(io_strobe), .io_din(io_din),
 .enabled(cn240_enabled), .mode_224p(cn240_mode_224p_requested),
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
# Keep the original OSD enable state and storage declarations in place.
# Move existing OSD scan counters from the local clocked block to module scope
# so the independent experimental read path can observe their true values.
marker="\treg [21:0] osd_vcnt;"
marker2="\treg [21:0] osd_hcnt;"
if text.count(marker)!=1 or text.count(marker2)!=1:
    raise SystemExit("OSD scan-counter layout changed; refusing patch")
text=text.replace(marker,"",1).replace(marker2,"",1)
v_counter="reg [21:0] v_cnt;"
if text.count(v_counter)!=1:
    raise SystemExit("OSD vertical counter layout changed; refusing patch")
text=text.replace(v_counter,"",1)
scope="reg        osd_enable;"
text=text.replace(scope,"reg [21:0] osd_vcnt;\nreg [21:0] osd_hcnt;\nreg [21:0] v_cnt;\nreg        deD;\nreg  [2:0] osd_div;\nreg  [2:0] multiscan;\nreg [21:0] v_osd_start;\nreg  [1:0] osd_en;\n"+scope,1)
# Promote scan state referenced by the independent event and output logic.
for declaration in ("\treg        deD;", "\treg  [2:0] osd_div;",
                    "\treg  [2:0] multiscan;", "\treg [21:0] v_osd_start;",
                    "\treg  [1:0] osd_en;"):
    if text.count(declaration)!=1:
        raise SystemExit(f"OSD scan-state layout changed; refusing patch: {declaration!r}")
    text=text.replace(declaration,"",1)
# Place extension references after their upstream clock/scan declarations to
# avoid implicit nets in both Icarus and Quartus Verilog elaboration.
pixel_enable_decl="(* direct_enable *) reg ce_pix;"
if text.count(pixel_enable_decl)!=1:
    raise SystemExit("Upstream pixel-enable declaration changed; refusing patch")
text=text.replace(pixel_enable_decl,pixel_enable_decl+"\n\n"+insertion,1)
# Confirm the official vertical scan event anchor before using it below.
edge_anchor="\t\tif(de_in && !deD) begin"
assert text.count(edge_anchor)==1, "upstream DE edge structure changed"

# Connect the extension to the output raster. It remains selected only after
# explicit extension opt-in and while the upstream OSD itself is enabled.
legacy_pixels="""\tordout1 <= {{osd_pixel, osd_pixel, OSD_COLOR[2], din[23:19]},// 23:16
\t            {osd_pixel, osd_pixel, OSD_COLOR[1], din[15:11]},// 15:8
\t            {osd_pixel, osd_pixel, OSD_COLOR[0], din[7:3]}}; //  7:0"""
assert text.count(legacy_pixels)==1, "upstream legacy color mapper changed"
extended_pixels="""\tordout1 <= {{cn240_mux_pixel, cn240_mux_pixel, OSD_COLOR[2], din[23:19]},// 23:16
\t            {cn240_mux_pixel, cn240_mux_pixel, OSD_COLOR[1], din[15:11]},// 15:8
\t            {cn240_mux_pixel, cn240_mux_pixel, OSD_COLOR[0], din[7:3]}}; //  7:0"""
text=text.replace(legacy_pixels,extended_pixels,1)
mux_gate="\tosd_mux <= ~osd_de[2];"
assert text.count(mux_gate)==1, "legacy OSD active mux changed"
text=text.replace(mux_gate, "\tosd_mux <= ~((cn240_enabled_video && osd_en[1]) ? (cn240_running_d2 && cn240_x_in_range_d2 && cn240_video_active) : osd_de[2]);",1)
# Add the mode command as a separate selected SPI transaction (F4,
# one payload byte: 0 = 256p, 1 = 224p). The adapter exports its mode
# register; this wire assignment connects it to the two-stage synchronizer.
# The selected mode is still experimental and not read back by ARM.
p.write_text(text)
q=root/"sys/sys.qip"
q.write_text(q.read_text()+
 '\nset_global_assignment -name VERILOG_FILE [file join $::quartus(qip_path) cn_osd240_spi_adapter.v ]\n'+
 'set_global_assignment -name VERILOG_FILE [file join $::quartus(qip_path) cn_osd240_dualclock_ram.v ]\n'+
 'set_global_assignment -name VERILOG_FILE [file join $::quartus(qip_path) cn_osd240_pixel.v ]\n'+
 'set_global_assignment -name VERILOG_FILE [file join $::quartus(qip_path) cn_osd240_frame_mode.v ]\n'+
 'set_global_assignment -name VERILOG_FILE [file join $::quartus(qip_path) cn_osd240_scan_pulses.v ]\n'+
 'set_global_assignment -name VERILOG_FILE [file join $::quartus(qip_path) cn_osd240_vscan.v ]\n')
print("Applied expanded video output path, one-cycle scan events, and isolated dualclock RAM")

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
// Pixel-clock access is now wired but scan coordinates are a fixed stub.
wire cn240_enabled;
wire cn240_write, cn240_header;
wire [4:0] cn240_row;
wire [7:0] cn240_col, cn240_data;
wire [7:0] cn240_video_read;
wire cn240_video_header;
wire [4:0] cn240_video_row;
wire [7:0] cn240_video_col;
wire cn240_video_pixel, cn240_video_active;
// Demonstrate use of the genuine pixel clock and a registered read port.
// Display mux is NOT switched to this path until timing is validated.
cn_osd240_pixel cn240_reader (
 .clk(clk_video), .reset(1'b0),
 .x(8'd0), .y(8'd0), .ram_data(cn240_video_read),
 .ram_header(cn240_video_header), .ram_row(cn240_video_row),
 .ram_column(cn240_video_col), .pixel(cn240_video_pixel),
 .active(cn240_video_active)
);
cn_osd240_spi_adapter cn240_spi (
 .clk(clk_sys), .reset(~io_osd && !osd_enable),
 .io_osd(io_osd), .io_strobe(io_strobe), .io_din(io_din),
 .enabled(cn240_enabled), .ext_write(cn240_write),
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
p.write_text(text)
q=root/"sys/sys.qip"
q.write_text(q.read_text()+
 '\nset_global_assignment -name VERILOG_FILE [file join $::quartus(qip_path) cn_osd240_spi_adapter.v ]\n'+
 'set_global_assignment -name VERILOG_FILE [file join $::quartus(qip_path) cn_osd240_dualclock_ram.v ]\n'+
 'set_global_assignment -name VERILOG_FILE [file join $::quartus(qip_path) cn_osd240_pixel.v ]\n')
print("Applied isolated SPI write-bank experiment; NO 240p video integration")

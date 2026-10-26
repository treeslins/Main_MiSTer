// SPDX-License-Identifier: GPL-3.0-or-later
// 240p CRT OSD experimental framebuffer: 13 x 16px menu entries
// with an independent 24px header. This is a separately testable component,
// NOT yet connected to Menu_MiSTer or the ARM SPI protocol.
module cn_osd240_ram (
    input wire clk,
    input wire reset,
    input wire write_enable,
    input wire write_header,
    input wire [4:0] write_row,
    input wire [7:0] write_column,
    input wire [7:0] write_data,
    input wire read_header,
    input wire [4:0] read_row,
    input wire [7:0] read_column,
    output reg [7:0] read_data
);
    localparam integer BODY_LINES = 26;
    localparam integer HEADER_LINES = 3;
    localparam integer BODY_BYTES = BODY_LINES * 256;
    localparam integer HEADER_BYTES = HEADER_LINES * 256;

    reg [7:0] body [0:BODY_BYTES-1];
    reg [7:0] header [0:HEADER_BYTES-1];
    wire valid_body_wr = write_row < BODY_LINES;
    wire valid_header_wr = write_row < HEADER_LINES;
    wire valid_body_rd = read_row < BODY_LINES;
    wire valid_header_rd = read_row < HEADER_LINES;

    wire [12:0] wr_addr = {write_row,write_column};
    wire [12:0] rd_addr = {read_row,read_column};

    always @(posedge clk) begin
        if (write_enable) begin
            if (write_header) begin
                if (valid_header_wr) header[wr_addr] <= write_data;
            end else if(valid_body_wr) begin
                body[wr_addr] <= write_data;
            end
        end
        if (reset) read_data <= 8'h00;
        else if (read_header)
            read_data <= valid_header_rd ? header[rd_addr] : 8'h00;
        else
            read_data <= valid_body_rd ? body[rd_addr] : 8'h00;
    end
endmodule

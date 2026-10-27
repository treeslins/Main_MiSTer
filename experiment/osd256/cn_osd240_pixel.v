// SPDX-License-Identifier: GPL-3.0-or-later
// Experimental 256p pixel-to-RAM address mapper. The RAM outputs the byte
// on the clock after receiving this address; cn_osd240_pixel emits the
// corresponding bit one clock later. Not connected to official sys/osd.v.
module cn_osd240_pixel (
    input wire clk,
    input wire reset,
    input wire [7:0] x,
    input wire [7:0] y,
    input wire mode_224p,
    input wire [7:0] ram_data,
    output wire ram_header,
    output wire [4:0] ram_row,
    output wire [7:0] ram_column,
    output reg pixel,
    output reg active
);
    wire is_header = (y < 8'd24);
    // 224p mode: 24px header + 192px body + 8px margin.
    // 256p mode: 24px header + 224px body + 8px margin.
    wire [7:0] body_end = mode_224p ? 8'd216 : 8'd248;
    wire is_body = (y >= 8'd24 && y < body_end);
    wire [7:0] body_y = y - 8'd24;
    wire [7:0] logical_y = is_header ? y : body_y;
    assign ram_header = is_header;
    assign ram_row = {2'b00,logical_y[7:3]};
    assign ram_column = x;

    // Read data is registered in SRAM on the first rising edge.
    // Use delayed pixel selector when SRAM data arrives on the next cycle.
    reg [2:0] pixel_bit_q;
    reg valid_q;
    // Data, row-bit selector and active are sampled in matching clock stages.
    always @(posedge clk) begin
        if (reset) begin
            pixel_bit_q <= 0;
            valid_q <= 0;
            pixel <= 0;
            active <= 0;
        end else begin
            pixel_bit_q <= logical_y[2:0];
            valid_q <= is_header || is_body;
            pixel <= valid_q ? ram_data[pixel_bit_q] : 1'b0;
            active <= valid_q;
        end
    end
endmodule

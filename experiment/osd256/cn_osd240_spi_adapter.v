// SPDX-License-Identifier: GPL-3.0-or-later
// SPI-transaction-facing, experimental 240p command adapter.
// Compatible with sys/osd.v's io_osd / io_strobe / io_din convention.
// Protocol is intentionally inert until an explicit one-transaction opt-in:
// command F0, payload 'C','N','2','4' (four bytes). F1 deactivates.
// F2 payload row[0..25], 256 data bytes: body; F3 row[0..2], 256 bytes: header.
// These F* commands are experimental and must be reserved in official RTL;
// they are NOT standard MiSTer commands. No capability response yet.
module cn_osd240_spi_adapter (
 input wire clk, input wire reset,
 input wire io_osd, input wire io_strobe, input wire [15:0] io_din,
 output reg enabled,
 output reg ext_write, output reg ext_header,
 output reg [4:0] ext_row,
 output reg [7:0] ext_col, output reg [7:0] ext_data,
 output wire legacy_passthrough
);
 reg old_strobe=0;
 reg old_selected=0;
 reg got_command=0;
 reg [7:0] command=0;
 reg [8:0] count=0;
 reg [2:0] magic=0;
 reg row_valid=0;
 wire rise=io_osd && io_strobe && !old_strobe;
 wire special=(command==8'hF0 || command==8'hF1 ||
               command==8'hF2 || command==8'hF3);
 assign legacy_passthrough = !enabled && !special;
 always @(posedge clk) begin
   old_strobe<=io_strobe;
   old_selected<=io_osd;
   ext_write<=0;
   if(reset) begin
     enabled<=0; got_command<=0; command<=0; count<=0;
     magic<=0; row_valid<=0;
     ext_header<=0;ext_row<=0;ext_col<=0;ext_data<=0;
   end else if(!io_osd) begin
     got_command<=0; command<=0;count<=0;magic<=0;row_valid<=0;
   end else if(rise) begin
     if(!got_command) begin
       got_command<=1;
       command<=io_din[7:0];
       count<=0;
       magic<=0;
       row_valid<=0;
       if(io_din[7:0]==8'hF1) enabled<=0;
     end else begin
       count<=count+9'd1;
       if(command==8'hF0 && !enabled) begin
         if((count==0 && io_din[7:0]==8'h43) ||
            (count==1 && magic==1 && io_din[7:0]==8'h4E) ||
            (count==2 && magic==2 && io_din[7:0]==8'h32) ||
            (count==3 && magic==3 && io_din[7:0]==8'h34))
           magic<=magic+3'd1;
         else magic<=7;
         if(count==3 && magic==3 && io_din[7:0]==8'h34)
           enabled<=1;
       end
       if(enabled && (command==8'hF2 || command==8'hF3)) begin
         if(count==0) begin
           ext_header<=command==8'hF3;
           ext_row<=io_din[4:0];
           row_valid<=((command==8'hF2 && io_din[7:0]<26) ||
                       (command==8'hF3 && io_din[7:0]<3));
         end else if(count<=256 && row_valid) begin
           ext_write<=1;
           ext_col<=count-9'd1;
           ext_data<=io_din[7:0];
         end
       end
     end
   end
 end
endmodule

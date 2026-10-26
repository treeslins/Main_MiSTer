// SPDX-License-Identifier: GPL-3.0-or-later
// Experimental *interface prototype*. This is not wired to official sys/osd.v.
// New 240p writes need an explicit capability unlock. Until then a separate
// legacy marker is used, and no extended write may take place.
module cn_osd240_bridge (
 input wire clk,
 input wire reset,
 input wire [7:0] command,
 input wire command_valid,
 input wire [7:0] data_byte,
 input wire data_valid,
 output reg extended_ready,
 output wire extended_write,
 output wire extended_header,
 output reg [4:0] extended_row,
 output reg [7:0] extended_column,
 output wire [7:0] extended_data,
 output wire legacy_command
);
 // EXPERIMENT ONLY: 0xE0,0x43,0x4E,0x32,0x34 in five successive command
 // strobes opts in. A non-handshake command cancels any partial handshake.
 // This is not yet an official MiSTer command protocol. Real osd.v takes
 // one command per selected SPI transaction: a multiple-command handshake
 // MUST NOT be connected directly without a transaction-level adapter.
 reg [2:0] handshake=0;
 reg armed_row=0;
 reg header=0;
 reg [7:0] column=0;
 wire is_body = command>=8'hA0 && command<=8'hB9;
 wire is_header = command>=8'hC0 && command<=8'hC2;
 assign extended_header=header;
 assign extended_data=data_byte;
 assign extended_write=extended_ready && armed_row && data_valid && !command_valid;
 assign legacy_command = command_valid && !extended_ready &&
   !(handshake!=0 || command==8'hE0);
 always @(posedge clk) begin
   if(reset) begin
     handshake<=0;
     extended_ready<=0;
     armed_row<=0;
     extended_row<=0;
     column<=0;
     header<=0;
   end else begin
     if(command_valid) begin
       armed_row<=0;
       if(!extended_ready) begin
         case(handshake)
           0: handshake<=(command==8'hE0)?1:0;
           1: handshake<=(command==8'h43)?2:0;
           2: handshake<=(command==8'h4E)?3:0;
           3: handshake<=(command==8'h32)?4:0;
           4: begin
             if(command==8'h34) extended_ready<=1;
             handshake<=0;
           end
           default: handshake<=0;
         endcase
       end else if(is_body || is_header) begin
         header<=is_header;
         extended_row<=is_header ? (command-8'hC0) : (command-8'hA0);
         column<=0;
         armed_row<=1;
       end else begin
         armed_row<=0;
       end
     end
     if(data_valid && armed_row && extended_ready) begin
       if(column==8'hFF) armed_row<=0;
       else column<=column+8'd1;
     end
   end
 end
 assign extended_column=column;
endmodule

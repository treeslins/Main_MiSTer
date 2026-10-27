// SPDX-License-Identifier: GPL-3.0-or-later
`timescale 1ns/1ps
module tb;
    reg clk=0, reset=0, we=0, wh=0, rh=0;
    reg [4:0] wr=0, rr=0;
    reg [7:0] wc=0, rc=0, wd=0;
    wire [7:0] value;
    cn_osd240_ram dut(clk,reset,we,wh,wr,wc,wd,rh,rr,rc,value);
    always #5 clk=~clk;

    task put(input reg bank, input [4:0] row, input [7:0] col, input [7:0] val);
    begin
        @(negedge clk); wh=bank; wr=row; wc=col; wd=val; we=1;
        @(negedge clk); we=0;
    end
    endtask

    task expect_byte(input reg bank, input [4:0] row, input [7:0] col, input [7:0] val);
    begin
        @(negedge clk); rh=bank; rr=row; rc=col;
        @(posedge clk); #1;
        if(value!==val) begin
            $display("FAIL bank=%d row=%d col=%d expected=%h got=%h",bank,row,col,val,value);
            $fatal(1);
        end
    end
    endtask

    initial begin
        @(negedge clk); reset=1;
        @(negedge clk); reset=0;
        // Boundary and typical body rows, including row 17 that was
        // previously incorrectly used as a status-bar storage address.
        put(0,0,0,8'hA0);
        put(0,15,255,8'hAF);
        put(0,16,0,8'hB0);
        put(0,17,0,8'hB1);
        put(0,18,0,8'hB2);
        put(0,25,255,8'hCF);
        put(0,27,255,8'hDF);
        // Status bank shares row numbers without aliasing body addresses.
        put(1,0,0,8'hD0);
        put(1,1,0,8'hD1);
        put(1,2,255,8'hD2);
        expect_byte(0,0,0,8'hA0);
        expect_byte(0,15,255,8'hAF);
        expect_byte(0,16,0,8'hB0);
        expect_byte(0,17,0,8'hB1);
        expect_byte(0,18,0,8'hB2);
        expect_byte(0,25,255,8'hCF);
        expect_byte(0,27,255,8'hDF);
        expect_byte(1,0,0,8'hD0);
        expect_byte(1,1,0,8'hD1);
        expect_byte(1,2,255,8'hD2);
        // Out-of-range access must be ignored.
        put(0,28,0,8'hEE);
        put(1,3,0,8'hEE);
        expect_byte(0,28,0,8'h00);
        expect_byte(1,3,0,8'h00);
        expect_byte(0,17,0,8'hB1);
        expect_byte(1,1,0,8'hD1);
        $display("PASS: 256p menu SRAM/header SRAM isolated, all boundary cases");
        $finish;
    end
endmodule

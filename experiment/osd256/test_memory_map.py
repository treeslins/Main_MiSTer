#!/usr/bin/env python3
"""Hardware-independent protocol prototype. NOT actual MiSTer SPI encoding.

Raises on overlapping logical rows/status regions or commands with unsafe
5-bit physical row aliasing. Does not synthesize FPGA logic.
"""
from dataclasses import dataclass

WIDTH=256
GLYPH_HEIGHT=16
LOGICAL_LINES=16
BODY_ROWS=LOGICAL_LINES*2
BODY_BYTES=WIDTH*BODY_ROWS
HEADER_ROWS=4  # include alignment space for 24px status header
HEADER_BYTES=HEADER_ROWS*WIDTH

@dataclass(frozen=True)
class BankAddress:
    bank: str
    row: int
    start: int
    stop: int

def body(line: int, half: int) -> BankAddress:
    assert 0 <= line < LOGICAL_LINES
    assert half in (0,1)
    row=line*2+half
    off=row*WIDTH
    return BankAddress('BODY',row,off,off+WIDTH)

def status(row: int) -> BankAddress:
    assert 0 <= row < 3
    off=BODY_BYTES + row*WIDTH
    return BankAddress('HEADER',row,off,off+WIDTH)

def main():
    all_ranges = [body(line,half) for line in range(LOGICAL_LINES) for half in (0,1)]
    all_ranges += [status(row) for row in range(3)]
    assert len(all_ranges)==35
    assert BODY_BYTES==8192 and HEADER_BYTES==1024
    assert max(r.stop for r in all_ranges)==BODY_BYTES + 3*WIDTH
    for index,a in enumerate(all_ranges):
        assert a.stop-a.start==WIDTH
        for b in all_ranges[index+1:]:
            assert a.stop<=b.start or b.stop<=a.start, (a,b)
    # Current 5-bit OSD command encodes only body rows.
    assert body(15,1).row==31
    assert max(status(x).start for x in range(3))>=8192
    print('PASS: 32 body rows and 3 separate header rows; no SRAM collisions.')
    print('NOT IMPLEMENTED: FPGA status bank command, read mux, handshake and Quartus build.')

if __name__=='__main__':
    main()

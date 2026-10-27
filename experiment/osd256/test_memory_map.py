#!/usr/bin/env python3
"""Offline address-design checks for CRT-safe OSD layouts. NOT FPGA firmware."""
from dataclasses import dataclass

WIDTH=256
GLYPH_HEIGHT=16
STATUS_HEIGHT=24
BOTTOM_MARGIN=8
STATUS_STORAGE_ROWS=4   # 1024 bytes, aligned for special menu status
TOTAL_ROWS_CRT_256=256
TOTAL_ROWS_CRT_224=224

@dataclass(frozen=True)
class BankAddress:
    bank: str
    row: int
    start: int
    stop: int

def geometry(screen_height: int):
    if screen_height not in (224,256):
        raise ValueError("supported design targets: 224p and 256p")
    available=screen_height-STATUS_HEIGHT-BOTTOM_MARGIN
    lines=available//GLYPH_HEIGHT
    return {
        "display_height":screen_height,
        "status_height":STATUS_HEIGHT,
        "logical_lines":lines,
        "body_height":lines*GLYPH_HEIGHT,
        "body_rows":lines*2,
        "margin":screen_height-STATUS_HEIGHT-lines*GLYPH_HEIGHT,
        "body_bytes":WIDTH*lines*2,
        "header_bytes":WIDTH*STATUS_STORAGE_ROWS,
    }

def verify(screen_height):
    g=geometry(screen_height)
    body_bytes=g["body_bytes"]
    records=[]
    for logical in range(g["logical_lines"]):
        for half in (0,1):
            row=logical*2+half
            begin=row*WIDTH
            records.append(BankAddress("BODY",row,begin,begin+WIDTH))
    for row in range(3):
        begin=body_bytes+row*WIDTH
        records.append(BankAddress("HEADER",row,begin,begin+WIDTH))
    for i,a in enumerate(records):
        for b in records[i+1:]:
            assert a.stop<=b.start or b.stop<=a.start,(a,b)
    assert g["status_height"]+g["body_height"]+g["margin"] == screen_height
    assert g["margin"] >= BOTTOM_MARGIN
    assert g["body_rows"] <= 30
    assert max(record.stop for record in records) <= body_bytes+g["header_bytes"]
    # Status uses its own logical bank; it cannot alias body rows 16..18.
    return g

def main():
    for height in (TOTAL_ROWS_CRT_224,TOTAL_ROWS_CRT_256):
        cfg=verify(height)
        print(f"PASS {height}p: {cfg['logical_lines']} rows of 16px, "
              f"{cfg['body_height']}px body + {STATUS_HEIGHT}px status + "
              f"{cfg['margin']}px blank = {height}px; "
              f"body SRAM {cfg['body_bytes']} bytes.")
    print("No FPGA bitstream or ARM protocol implementation has been built.")

if __name__=="__main__":
    main()

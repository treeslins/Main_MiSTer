"""Create a temporary native-host font object source; ARM builds use ld -b binary."""
from pathlib import Path
import sys
root=Path(__file__).resolve().parents[1]
data=(root/'localization/zh_font.bin').read_bytes()
Path(sys.argv[1]).write_text('extern "C" const unsigned char _binary_localization_zh_font_bin_start[] = {\n'+
    ',\n'.join(','.join(str(x) for x in data[i:i+64]) for i in range(0,len(data),64))+'\n};\n',encoding='utf-8')

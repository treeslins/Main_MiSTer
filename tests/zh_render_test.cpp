#include "zh_render.h"
#include <assert.h>
#include <stdio.h>
#include <vector>
int main(int argc, char **argv) {
    uint8_t a[234],b[234],c[234],d[234];
    assert(ZhRender(u8"中文",a,b,234)==32);
    ZhRender(u8"中文",c,d,234,true);
    for (unsigned i=0;i<234;++i) { assert((a[i]^c[i])==255); assert((b[i]^d[i])==255); }
    assert(ZhRender(u8"中",a,b,15)==0);
    assert(ZhRender(u8"中",a,b,16)==16);
    assert(ZhRender(u8"中A",a,b,234)==32); // A absent: visible fallback box
    assert(a[16]==255 && b[16]==255);
    ZhRender(u8"中文",a,b,234);
    ZhRender(u8"中文",c,d,234,false,false,3);
    for(unsigned i=0;i<29;++i) { assert(c[i]==a[i+3]); assert(d[i]==b[i+3]); }
    for(const char *input : {"\xe4", "\xe4\xb8", "\xc0\xaf", "\xed\xa0\x80", "\xf4\x90\x80\x80"}) {
        const char *p=input; assert(ZhDecode(p)==0xfffd);
    }
    struct { uint8_t pre; uint8_t data[15]; uint8_t post; } guard={42,{},43};
    ZhRender(u8"中文",guard.data,b,15); assert(guard.pre==42 && guard.post==43);
    if(argc>1) {
        const char *labels[]={u8"MiSTer 中文测试",u8"保存 设置 退出",u8"保存 设置 退出",u8"保存 设置 退出",u8"中文显示测试 中文显示测试 中文显示测试"};
        std::vector<unsigned char> pixels(256*128,0);
        for(unsigned row=0;row<5;++row) {
            ZhRender(labels[row],a,b,234,row==1,row==2,row==3?7:0);
            for(unsigned x=0;x<234;++x) for(unsigned y=0;y<16;++y)
                pixels[(row*24+y)*256+22+x]=(((y<8?a[x]:b[x])>>(y%8))&1)?255:0;
        }
        FILE *f=fopen(argv[1],"wb"); assert(f); fprintf(f,"P5\n256 128\n255\n");
        fwrite(pixels.data(),1,pixels.size(),f); fclose(f);
    }
    puts("UTF-8, clipping, inverse, scrolling, fallback and bounds: PASS");
}

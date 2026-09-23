#include "zh_render.h"
#include "zh_ui.h"
#include <algorithm>
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
    assert(ZhRender(u8"中\U0001f680",a,b,234)==32); // Unknown supplementary glyph: visible fallback box
    assert(a[16]==255 && b[16]==255);
    ZhRender(u8"中文",a,b,234);
    ZhRender(u8"中文",c,d,234,false,false,3);
    for(unsigned i=0;i<29;++i) { assert(c[i]==a[i+3]); assert(d[i]==b[i+3]); }
    for(const char *input : {"\xe4", "\xe4\xb8", "\xc0\xaf", "\xed\xa0\x80", "\xf4\x90\x80\x80"}) {
        const char *p=input; assert(ZhDecode(p)==0xfffd);
    }
    struct { uint8_t pre; uint8_t data[15]; uint8_t post; } guard={42,{},43};
    ZhRender(u8"中文",guard.data,b,15); assert(guard.pre==42 && guard.post==43);
    assert(ZhTranslate("   System Settings  ")==u8"系统设置");
    assert(ZhTranslate("Core Volume:        Off")==u8"核心音量: 关闭");
    assert(ZhTranslate("Video Mode: Auto")==u8"视频模式: 自动");
    assert(ZhTranslate("/media/fat/Save.rom")=="/media/fat/Save.rom");
    assert(ZhTranslate("Offset: 20")==u8"偏移: 20");
    assert(ZhTranslate("ControllerX")=="ControllerX");
    assert(ZhMixedText(u8"Save 中文.rom")==u8"Save 中文.rom");
    assert(ZhTextWidth(u8"中文AB")==48);
    auto wrapped=ZhWrap(u8"中文中文",32);
    assert(wrapped.size()==2 && wrapped[0]==u8"中文" && wrapped[1]==u8"中文");
    for(unsigned size=8;size<=18;++size) {
        ZhPage page;
        for(unsigned i=0;i<size-1;++i) page.set(i,"Menu",false,false);
        page.set(size-1,"            exit",false,false);
        for(unsigned selected=0;selected<size;++selected) {
            for(unsigned i=0;i<size;++i) page.set(i,i==size-1?"exit":"Menu",i==selected,false);
            auto rows=page.visible(size);
            assert(rows.size()<=8 && rows.back()==(int)size-1);
            assert(std::find(rows.begin(),rows.end(),selected)!=rows.end());
        }
        page.page(-1); assert(page.visible(size).front()<=(int)size-8);
    }
    ZhPage page;
    page.set(0,"Save",true,false,true);
    assert(page.rows[0].text=="Save");
    page.clear();
    page.set(0,"Help",false,false); page.set(4,"About",true,false); page.set(15,"exit",false,false);
    auto rows=page.visible(16); assert(rows[0]==0 && rows[1]==4 && rows[7]==15);
    uint8_t icons[256][8]={}; icons[0x16][0]=1;
    uint8_t menu_frame[16*256];
    ZhDrawPage(page,u8"系统设置",16,1,menu_frame,icons);
    assert(menu_frame[0]!=0); // Sidebar survives the 16-pixel menu layout.
    assert(menu_frame[14*256+40]!=0); // Pinned exit is present on the last two rows.
    assert(menu_frame[2*256+40]!=0); // Focused "About" is highlighted.
    auto mixed=ZhMixedText("\x16");
    assert(ZhRender(mixed.c_str(),a,b,8,false,false,0,icons)==8 && a[0]==3 && b[0]==0);
    auto toggle=ZhMixedText("\x0c A");
    ZhRender(toggle.c_str(),a,b,16,false,false,0,icons); assert(a[0]==255);
    if(argc>1) {
        ZhPage screen;
        const char *labels[]={"MiSTer v260922","Available space: 19gb","Storage: SD card","Switch to USB","Remap keyboard","Define joystick buttons","Scripts","Help","Reboot (hold cold reboot)"};
        for(unsigned i=0;i<9;++i) screen.set(i,labels[i],i==5,false);
        screen.set(15,"exit",false,false);
        uint8_t frame[16*256];
        ZhDrawPage(screen,u8"系统设置",16,1,frame,icons);
        std::vector<unsigned char> pixels(256*128,0);
        for(unsigned y=0;y<128;++y) for(unsigned x=0;x<256;++x)
            pixels[y*256+x]=(frame[(y/8)*256+x]>>(y%8))&1?255:0;
        FILE *f=fopen(argv[1],"wb"); assert(f); fprintf(f,"P5\n256 128\n255\n");
        fwrite(pixels.data(),1,pixels.size(),f); fclose(f);
    }
    puts("UTF-8, clipping, inverse, scrolling, fallback and bounds: PASS");
}

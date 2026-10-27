#include "zh_render.h"
#include "zh_ui.h"
#include <algorithm>
#include <assert.h>
#include <stdio.h>
#include <utility>
#include <vector>
int main(int argc, char **argv) {
    uint8_t a[234],b[234],c[234],d[234];
    assert(ZhRender(u8"中文",a,b,234)==24);
    ZhRender(u8"中文",c,d,234,true);
    for (unsigned i=0;i<234;++i) { assert((a[i]^c[i])==255); assert((b[i]^d[i])==255); }
    assert(ZhRender(u8"中",a,b,11)==0);
    assert(ZhRender(u8"中",a,b,12)==12);
    assert(ZhRender(u8"中\U0010ffff",a,b,234)==28); // Unknown glyph: visible fallback box
    assert(a[12]==255 && b[12]==255);
    ZhRender(u8"中文",a,b,234);
    ZhRender(u8"中文",c,d,234,false,false,3);
    for(unsigned i=0;i<21;++i) { assert(c[i]==a[i+3]); assert(d[i]==b[i+3]); }
    for(const char *input : {"\xe4", "\xe4\xb8", "\xc0\xaf", "\xed\xa0\x80", "\xf4\x90\x80\x80"}) {
        const char *p=input; assert(ZhDecode(p)==0xfffd);
    }
    struct { uint8_t pre; uint8_t data[15]; uint8_t post; } guard={42,{},43};
    ZhRender(u8"中文",guard.data,b,15); assert(guard.pre==42 && guard.post==43);
    assert(ZhTranslate("   System Settings  ")==u8"   系统设置");
    assert(ZhTranslate("Core Volume:        Off")==u8"核心音量: 关闭");
    assert(ZhTranslate("Video Mode: Auto")==u8"视频模式: 自动");
    assert(ZhTranslate("/media/fat/Save.rom")=="/media/fat/Save.rom");
    assert(ZhTranslate("Offset: 20")==u8"偏移: 20");
    assert(ZhTranslate("ControllerX")=="ControllerX");
    assert(ZhMixedText(u8"Save 中文.rom")==u8"Save 中文.rom");
    assert(ZhTextWidth(u8"中文AB")==36);
    auto wrapped=ZhWrap(u8"中文中文",24);
    assert(wrapped.size()==2 && wrapped[0]==u8"中文" && wrapped[1]==u8"中文");
    // English padding must not shift Chinese options; values share a right edge.
    ZhPage columns;
    columns.set(0,"   Core Volume:       Off",false,false);
    uint8_t column_frame[16*256];
    unsigned char column_icons[256][8]={};
    ZhDrawPage(columns,"",8,0,column_frame,column_icons);
    uint8_t value_top[24],value_bottom[24];
    assert(ZhRender(u8"关闭",value_top,value_bottom,24)==24);
    for(unsigned x=0;x<24;++x) {
        assert(column_frame[22+234-24+x]==value_top[x]);
        assert(column_frame[256+22+234-24+x]==value_bottom[x]);
    }
    // Collapsed blank rows still leave a visible section marker.
    ZhPage sections;
    sections.set(0,"First",false,false);
    for(unsigned i=2;i<=8;++i) sections.set(i,"Next",false,false);
    ZhDrawPage(sections,"",16,0,column_frame,column_icons);
    assert((column_frame[2*256+22+4]&1)!=0);
    // A compact status page must show its three controls and exit together.
    ZhPage status;
    for(unsigned i=0;i<4;++i) status.set(i,"Status",false,false);
    for(unsigned selected=4;selected<=6;++selected) {
        for(unsigned i=4;i<=6;++i) status.set(i,"Control",i==selected,false);
        status.set(15,"exit",false,false);
        auto shown=status.visible(16);
        assert(shown.size()==8);
        for(unsigned i=0;i<7;++i) assert(shown[i]==(int)i);
        assert(shown[7]==15);
    }
    // Selecting the pinned Exit keeps the end of a long menu above it.
    ZhPage exit_page;
    for(unsigned i=0;i<15;++i) exit_page.set(i,"Option",false,false);
    exit_page.set(15,"exit",true,false);
    auto exit_rows=exit_page.visible(16);
    assert(exit_rows.size()==8 && exit_rows[0]==8 && exit_rows[6]==14 && exit_rows[7]==15);
    // A short menu must not jump into its unused blank rows.
    ZhPage short_exit_page;
    for(unsigned i=0;i<4;++i) short_exit_page.set(i,"Option",false,false);
    short_exit_page.set(15,"exit",true,false);
    auto short_exit_rows=short_exit_page.visible(16);
    assert(short_exit_rows.size()==8 && short_exit_rows[0]==0 && short_exit_rows[3]==3 && short_exit_rows[7]==15);
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
    auto rows=page.visible(16); assert(rows.size()==8 && rows[0]==0 && rows[1]==1 && rows[4]==4 && rows[7]==15);
    // Preserve menu separator rows, native row indices and leading spaces.
    ZhPage spacing;
    spacing.set(0,"Settings",false,false);
    spacing.set(1,"",false,false); // deliberately blank separator
    spacing.set(2,"    Video Mode: Auto",false,false);
    spacing.set(4,"    Core Volume:        Off",true,false);
    spacing.set(7,"         exit",false,false);
    auto natural=spacing.visible(8);
    assert(natural.size()==8);
    for(int i=0;i<7;++i) assert(natural[(unsigned)i]==i);
    assert(natural[7]==7);
    assert(spacing.rows[2].text==u8"    视频模式: 自动");
    assert(spacing.rows[4].text==u8"    核心音量: 关闭");
    // On a taller native menu, scroll using logical row numbers and do not
    // drop blank separators or reorder options.
    ZhPage grouped;
    grouped.set(0,"Top",false,false);
    grouped.set(1,"",false,false);
    grouped.set(2,"Group A",false,false);
    grouped.set(3,"",false,false);
    grouped.set(4,"Group B",false,false);
    grouped.set(12,"Selected",true,false);
    grouped.set(15,"       exit",false,false);
    auto paged=grouped.visible(16);
    assert(paged.size()==8 && paged[7]==15);
    assert(paged[0]==6 && paged[6]==12 && grouped.first==6);
    assert(std::find(paged.begin(),paged.end(),3)==paged.end());

    uint8_t icons[256][8]={}; icons[0x16][0]=1;
    uint8_t menu_frame[16*256];
    ZhDrawPage(page,u8"系统设置",16,1,menu_frame,icons);
    assert(menu_frame[0]!=0); // Sidebar survives the 16-pixel menu layout.
    for(const auto &pair : {std::pair<const char*,const char*>{u8"系统设置",u8"系"},
                            {u8"核心列表",u8"核"}, {u8"其他选项",u8"其"}}) {
        ZhPage empty;
        ZhDrawPage(empty,pair.first,16,0,menu_frame,icons);
        uint8_t top_glyph[16],bottom_glyph[16];
        assert(ZhRender(pair.second,top_glyph,bottom_glyph,16)==12);
        for(unsigned x=0;x<12;++x) {
            assert(menu_frame[4*256+4+x]==(uint8_t)~top_glyph[x]);
            assert(menu_frame[5*256+4+x]==(uint8_t)~bottom_glyph[x]);
        }
    }
    ZhDrawPage(page,u8"系统设置",16,1,menu_frame,icons);
    // Pinning is a layout property, not a guarantee that column 40 contains ink.
    bool footer_ink=false;
    for(unsigned physical=14;physical<16;++physical)
        for(unsigned x=22;x<160;++x)
            if(menu_frame[physical*256+x]) footer_ink=true;
    assert(footer_ink); // Pinned exit is visible in the bottom two OSD rows.
    assert(menu_frame[8*256+40]!=0); // Focused "About" stays in logical row 4.
    auto mixed=ZhMixedText("\x16");
    assert(ZhRender(mixed.c_str(),a,b,8,false,false,0,icons)==8 && a[0]==1 && b[0]==0);
    icons[0x16][0]=0x7c; // Lower edge of the original inline submenu arrow.
    ZhRender(mixed.c_str(),a,b,8,false,false,0,icons);
    assert((((uint16_t)b[0]<<8)|a[0]) & (1u<<12));
    assert(!((((uint16_t)b[0]<<8)|a[0]) & (1u<<13)));
    for(unsigned char network_icon : {0x1c,0x1d}) {
        icons[network_icon][0]=0x7f;
        char icon_text[2]={(char)network_icon,0};
        auto unicode_icon=ZhMixedText(icon_text);
        assert(ZhRender(unicode_icon.c_str(),a,b,8,false,false,0,icons)==8);
        assert((((uint16_t)b[0]<<8)|a[0])==0x07f0);
    }
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

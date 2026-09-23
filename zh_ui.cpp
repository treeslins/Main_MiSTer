// SPDX-License-Identifier: GPL-3.0-or-later
#include "zh_ui.h"
#include "zh_catalog.h"
#include "zh_render.h"
#include <algorithm>
#include <cstring>

static bool word(unsigned char c) { return (c>='a' && c<='z') || (c>='A' && c<='Z') || (c>='0' && c<='9') || c=='_' || c=='.' || c=='/' || c=='\\'; }
static unsigned char lower(unsigned char c) { return c>='A' && c<='Z' ? c+32 : c; }

std::string ZhTranslate(const std::string &source)
{
    // Normalize presentation padding only; the caller's source is untouched.
    std::string text;
    for(unsigned char c : source) {
        if(c==' ' || c=='\t') { if(!text.empty() && text.back()!=' ') text+=' '; }
        else text+=c;
    }
    while(!text.empty() && text.back()==' ') text.pop_back();
    std::string output;
    for(size_t i=0; i<text.size();) {
        const ZhTerm *match=nullptr;
        if(i==0 || !word((unsigned char)text[i-1])) {
            for(const auto &term : zh_terms) {
                const size_t len=strlen(term.english);
                if(i+len>text.size() || (i+len<text.size() && word((unsigned char)text[i+len]))) continue;
                size_t j=0;
                while(j<len && lower((unsigned char)text[i+j])==(unsigned char)term.english[j]) ++j;
                if(j==len) { match=&term; break; }
            }
        }
        if(match) { output+=match->chinese; i+=strlen(match->english); }
        else output+=text[i++];
    }
    return ZhMixedText(output);
}

static void append_utf8(std::string &s, uint32_t cp)
{
    if(cp<0x80) s+=(char)cp;
    else if(cp<0x800) { s+=(char)(0xc0|(cp>>6)); s+=(char)(0x80|(cp&63)); }
    else if(cp<0x10000) { s+=(char)(0xe0|(cp>>12)); s+=(char)(0x80|((cp>>6)&63)); s+=(char)(0x80|(cp&63)); }
    else { s+=(char)(0xf0|(cp>>18)); s+=(char)(0x80|((cp>>12)&63)); s+=(char)(0x80|((cp>>6)&63)); s+=(char)(0x80|(cp&63)); }
}

std::string ZhMixedText(const std::string &source)
{
    std::string output;
    const char *p=source.c_str();
    while(*p) {
        unsigned char c=(unsigned char)*p;
        if((c<32 && c!='\n' && c!='\r' && c!='\t') || (c>=0x7f && c<0xa0)) {
            // Preserve the legacy OSD icons in a private Unicode range.
            // 0x0b and 0x0c are style toggles, interpreted by the renderer.
            append_utf8(output, 0xe000+c); ++p;
        } else {
            uint32_t cp=ZhDecode(p);
            append_utf8(output, cp);
        }
    }
    return output;
}

unsigned ZhTextWidth(const std::string &text)
{
    unsigned result=0;
    const char *p=text.c_str();
    while(*p) {
        uint32_t cp=ZhDecode(p);
        if(cp==0xe00b || cp==0xe00c || cp=='\n' || cp=='\r') continue;
        const ZhGlyph *g=ZhFindGlyph(cp);
        result+=(cp>=0xe000 && cp<=0xe0ff)?8:g?g->width:16;
    }
    return result;
}

std::vector<std::string> ZhWrap(const std::string &text, unsigned width)
{
    std::vector<std::string> lines;
    std::string line;
    unsigned used=0;
    const char *p=text.c_str();
    while(*p) {
        const char *start=p;
        uint32_t cp=ZhDecode(p);
        if(cp=='\r') continue;
        std::string glyph(start,p);
        unsigned w=ZhTextWidth(glyph);
        if(cp=='\n' || used+w>width) {
            lines.push_back(line); line.clear(); used=0;
            if(cp=='\n') continue;
        }
        if(w<=width) { line+=glyph; used+=w; }
    }
    if(!line.empty() || lines.empty()) lines.push_back(line);
    return lines;
}

void ZhPage::clear()
{
    for(auto &r : rows) r=ZhRow{};
    first=0; focus=-1; manual=false;
}

void ZhPage::set(unsigned row, const char *source, bool invert, bool stipple,
                 bool raw, int mininv, int maxinv, unsigned char arrow)
{
    if(row>=32) return;
    auto &r=rows[row];
    const std::string incoming=source?source:"";
    if(incoming!=r.source || raw!=r.raw) {
        r.source=incoming; r.raw=raw;
        r.text=raw?ZhMixedText(incoming):ZhTranslate(incoming);
        r.scroll=0;
    }
    r.invert=invert; r.stipple=stipple; r.mininv=mininv; r.maxinv=maxinv; r.arrow=arrow;
    if(invert && focus!=(int)row) { focus=(int)row; manual=false; }
}

std::vector<int> ZhPage::visible(unsigned logical_rows)
{
    std::vector<int> active;
    int footer=-1;
    for(unsigned i=0; i<std::min(logical_rows,32u); ++i) {
        const std::string &text=rows[i].text;
        if(text.find_first_not_of(" \t\r\n")==std::string::npos) continue;
        if(i==logical_rows-1 && !rows[i].raw &&
           (text==u8"退出" || text==u8"返回" || text==u8"空格退出" || text==u8"Ctrl+Esc 退出")) footer=(int)i;
        else active.push_back((int)i);
    }
    const int slots=footer>=0?7:8;
    auto selected=std::find(active.begin(),active.end(),focus);
    if(!manual && selected!=active.end()) {
        int at=(int)(selected-active.begin());
        if(at<first) first=at;
        if(at>=first+slots) first=at-slots+1;
    }
    first=std::max(0,std::min(first,std::max(0,(int)active.size()-slots)));
    std::vector<int> result;
    for(int i=first;i<(int)active.size() && (int)result.size()<slots;++i) result.push_back(active[i]);
    if(footer>=0) { while(result.size()<7) result.push_back(-1); result.push_back(footer); }
    return result;
}

void ZhDrawPage(ZhPage &page, const std::string &title, unsigned logical_rows, int arrows, uint8_t *frame, const unsigned char (*icons)[8])
{
	memset(frame,0,16*256);
	uint8_t title_top[128],title_bottom[128];
	ZhRender(title.c_str(),title_top,title_bottom,128,false,false,0,icons);
	unsigned title_width=std::min(128u,ZhTextWidth(title));
	unsigned title_start=(128-title_width)/2;
	// Upright sidebar band with the same 22-pixel footprint as the old OSD.
	for(unsigned y=0;y<128;++y) for(unsigned x=0;x<20;++x) {
		bool ink=false;
		if(x>=2 && x<18 && y>=title_start && y<title_start+title_width) {
			unsigned col=y-title_start, bit=17-x;
			ink=((bit<8?title_top[col]:title_bottom[col])>>(bit%8))&1;
		}
		if(!ink) frame[(y/8)*256+x]|=1u<<(y%8);
	}
	auto visible=page.visible(logical_rows);
	for(unsigned physical=0;physical<visible.size();++physical) {
		if(visible[physical]<0) continue;
		auto &row=page.rows[visible[physical]];
		uint8_t top[234],bottom[234];
		std::string text=row.text;
		if(visible[physical]==(int)logical_rows-1 && !row.raw) {
			if(arrows&1) text=u8"← "+text;
			if(arrows&2) text+=u8" →";
		}
		if(row.scroll) text+="        "+row.text;
		ZhRender(text.c_str(),top,bottom,234,false,row.stipple,row.scroll,icons);
		for(unsigned x=0;x<234;++x) {
			// Partial highlights are measured in old 8-pixel OSD cells.
			bool inv=row.invert && (int)((x+22)/8)>=row.mininv && (int)((x+22)/8)<row.maxinv;
			frame[(physical*2)*256+22+x]=top[x]^(inv?255:0);
			frame[(physical*2+1)*256+22+x]=bottom[x]^(inv?255:0);
		}
		// Preserve per-row up/down navigation markers supplied by the core menu.
		if(row.arrow) for(unsigned x=0;x<8;++x) {
			unsigned bits=0;
			for(unsigned y=0;y<8;++y) if(icons[row.arrow][x]&(1u<<y)) bits|=3u<<(2*y);
			frame[physical*2*256+6+x]=(uint8_t)bits^255;
			frame[(physical*2+1)*256+6+x]=(uint8_t)(bits>>8)^255;
		}
	}
	// Page markers occupy the sidebar, not any translated text or selection.
	unsigned total=0;
	for(unsigned i=0;i<(unsigned)logical_rows && i<32;++i) if(page.rows[i].text.find_first_not_of(" \t\r\n")!=std::string::npos) ++total;
	if(total>8) {
		for(unsigned x=0;x<8;++x) {
			if(page.first) frame[6+x]=icons[17][x]^255;
			if(page.first+8<(int)total) frame[15*256+6+x]=icons[16][x]^255;
		}
	}
}

// SPDX-License-Identifier: GPL-3.0-or-later
#include "zh_ui.h"
#include "zh_catalog.h"
#include "zh_render.h"
#include <algorithm>
#include <cstring>

static bool word(unsigned char c) { return (c>='a' && c<='z') || (c>='A' && c<='Z') || (c>='0' && c<='9') || c=='_' || c=='.' || c=='/' || c=='\\'; }
static unsigned char lower(unsigned char c) { return c>='A' && c<='Z' ? c+32 : c; }
static std::string trim_layout_space(const std::string &s)
{
    const size_t first=s.find_first_not_of(" \t");
    if(first==std::string::npos) return "";
    const size_t last=s.find_last_not_of(" \t");
    return s.substr(first,last-first+1);
}

std::string ZhTranslate(const std::string &source)
{
    // Keep the original OSD indentation. The legacy menu deliberately
    // uses leading spaces to align submenus and settings with their parents.
    // Only fold repeated spaces in the translatable part of the sentence,
    // preserving the previous phrase-matching behavior for padded values.
    size_t indent_end=0;
    while(indent_end<source.size() &&
          (source[indent_end]==' ' || source[indent_end]=='\t')) ++indent_end;
    std::string text;
    for(size_t index=indent_end; index<source.size(); ++index) {
        unsigned char c=(unsigned char)source[index];
        if(c==' ' || c=='\t') {
            if(!text.empty() && text.back()!=' ') text+=' ';
        } else text+=(char)c;
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
    // Restore the leading-space columns after translation, so translated
    // options retain the original menu's horizontal hierarchy.
    output.insert(0, indent_end, ' ');
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
    first=0; focus=-1; body_entries=0; manual=false;
}

void ZhPage::set(unsigned row, const char *source, bool invert, bool stipple,
                 bool raw, int mininv, int maxinv, unsigned char arrow,
                 unsigned char vertical_shift, bool use_background,
                 const char *right_label, unsigned prefix_bytes)
{
    if(row>=32) return;
    auto &r=rows[row];
    const std::string incoming=source?source:"";
    if(incoming!=r.source || raw!=r.raw || prefix_bytes!=r.prefix_bytes) {
        r.source=incoming; r.raw=raw;
        r.prefix_bytes=prefix_bytes;
        // Legacy ScrollText offsets are byte counts. Stop on a complete
        // UTF-8 character so an offset cannot split a Chinese glyph.
        const char *start=incoming.c_str(), *end=start;
        size_t prefix_end=0;
        while(*end && (size_t)(end-start)<prefix_bytes) {
            ZhDecode(end);
            if((size_t)(end-start)>prefix_bytes) break;
            prefix_end=(size_t)(end-start);
        }
        r.fixed_prefix=ZhMixedText(incoming.substr(0,prefix_end));
        r.prefix_width=ZhTextWidth(r.fixed_prefix);
        const std::string body=incoming.substr(prefix_end);
        r.text=raw?ZhMixedText(body):ZhTranslate(body);
        r.pixel_width=ZhTextWidth(raw?r.text:trim_layout_space(r.text));
        r.scroll=0;
    }
    const std::string suffix=right_label?right_label:"";
    if(suffix!=r.right_label) {
        r.right_label=suffix;
        r.right_width=ZhTextWidth(suffix);
        r.scroll=0;
    }
    r.invert=invert; r.stipple=stipple; r.mininv=mininv; r.maxinv=maxinv; r.arrow=arrow;
    r.vertical_shift=vertical_shift>7?7:vertical_shift;
    r.use_background=use_background;
    if(invert && focus!=(int)row) { focus=(int)row; manual=false; }
}

// Translation now keeps original leading indentation. Recognize the pinned
// exit row by its label, not by its padding columns.
static bool zh_footer_label(const ZhRow &row)
{
    if(row.raw) return false;
    size_t start=row.text.find_first_not_of(" \t");
    if(start==std::string::npos) return false;
    const std::string label=row.text.substr(start);
    return label==u8"退出" || label==u8"返回" ||
           label==u8"空格退出" || label==u8"Ctrl+Esc 退出";
}

std::vector<int> ZhPage::visible(unsigned logical_rows)
{
    const unsigned limit=std::min(logical_rows,32u);
    const int footer=(limit && zh_footer_label(rows[limit-1]))?(int)limit-1:-1;
    const int body_end=(footer>=0)?footer:(int)limit;
    const int slots=(footer>=0)?7:8;
    int last_used=0;
    for(int i=0;i<body_end;++i)
        if(!rows[i].source.empty() || !rows[i].text.empty() || rows[i].invert)
            last_used=i+1;

    std::vector<int> candidates;
    // Keep native row geometry when it fits. For a 16-row legacy menu,
    // 16px OSD rows leave only seven body slots: remove separators only when
    // required, so actual entries remain reachable by focus/page controls.
    if(last_used<=slots) {
        // Keep the current screen's blank slots, but do not create empty
        // pages from the unused tail of a legacy 16-row menu.
        for(int i=0;i<std::min(body_end,slots);++i) candidates.push_back(i);
    } else {
        for(int i=0;i<last_used;++i)
            if(rows[i].text.find_first_not_of(" \t\r\n")!=std::string::npos ||
               rows[i].invert || rows[i].arrow)
                candidates.push_back(i);
        // If fewer meaningful entries remain, keep one separator where room
        // allows, instead of filling the viewport with discarded blanks.
        if((int)candidates.size()<slots) {
            candidates.clear();
            for(int i=0;i<last_used;++i) candidates.push_back(i);
        }
    }

    body_entries=(int)candidates.size();
    const int max_first=std::max(0,body_entries-slots);
    if(!manual && focus>=0) {
        if(focus==footer) {
            // Title redraws reset the viewport. With Exit selected, keep
            // the final options visible above its pinned footer row.
            first=last_used>slots?max_first:0;
        } else {
            auto it=std::find(candidates.begin(),candidates.end(),focus);
            if(it!=candidates.end()) {
                int index=(int)(it-candidates.begin());
                if(index<first) first=index;
                if(index>=first+slots) first=index-slots+1;
            }
        }
    }
    first=std::max(0,std::min(first,max_first));
    std::vector<int> result;
    for(int i=first;i<(int)candidates.size() && (int)result.size()<slots;++i)
        result.push_back(candidates[i]);
    if(footer>=0) {
        while(result.size()<7) result.push_back(-1);
        result.push_back(footer);
    }
    return result;
}

void ZhDrawPage(ZhPage &page, const std::string &title, unsigned logical_rows, int arrows, uint8_t *frame, const unsigned char (*icons)[8], const char (*background)[256])
{
	memset(frame,0,16*256);
	const bool stacked_title=title==u8"其他选项" || title==u8"核心列表" || title==u8"系统设置";
	if(stacked_title) {
		// These four-character Chinese titles read top to bottom without
		// rotating the individual glyphs. Keep the 20px sidebar footprint.
		for(unsigned y=0;y<128;++y) for(unsigned x=0;x<20;++x)
			frame[(y/8)*256+x]|=1u<<(y%8);
		const char *p=title.c_str();
		for(unsigned row=0;row<4;++row) {
			const char *start=p;
			ZhDecode(p);
			std::string glyph(start,p);
			uint8_t top[16],bottom[16];
			const unsigned width=ZhRender(glyph.c_str(),top,bottom,16,false,false,0,icons);
			const unsigned left=(20-width)/2;
			for(unsigned x=0;x<width;++x) {
				const uint16_t bits=(uint16_t)top[x] | ((uint16_t)bottom[x]<<8);
				for(unsigned y=0;y<16;++y) if(bits&(1u<<y)) {
					const unsigned pixel_y=32+row*16+y;
					frame[(pixel_y/8)*256+left+x]&=~(1u<<(pixel_y%8));
				}
			}
		}
	} else {
		uint8_t title_top[128],title_bottom[128];
		ZhRender(title.c_str(),title_top,title_bottom,128,false,false,0,icons);
		unsigned title_width=std::min(128u,ZhTextWidth(title));
		unsigned title_start=(128-title_width)/2;
		for(unsigned y=0;y<128;++y) for(unsigned x=0;x<20;++x) {
			bool ink=false;
			if(x>=2 && x<18 && y>=title_start && y<title_start+title_width) {
				unsigned col=y-title_start, bit=17-x;
				ink=((bit<8?title_top[col]:title_bottom[col])>>(bit%8))&1;
			}
			if(!ink) frame[(y/8)*256+x]|=1u<<(y%8);
		}
	}
	auto visible=page.visible(logical_rows);
	for(unsigned physical=0;physical<visible.size();++physical) {
		if(visible[physical]<0) continue;
		auto &row=page.rows[visible[physical]];
		uint8_t top[234]={},bottom[234]={};
		// Legacy spaces align 8px English characters, not 12px Chinese glyphs.
		std::string text=row.raw?row.text:trim_layout_space(row.text);
		if(visible[physical]==(int)logical_rows-1 && !row.raw) {
			if(arrows&1) text=u8"← "+text;
			if(arrows&2) text+=u8" →";
		}
		if(row.scroll) text+="        "+(row.raw?row.text:trim_layout_space(row.text));
		const unsigned suffix_width=std::min(row.right_width,234u);
		const unsigned gap=suffix_width?8u:0u;
		const unsigned name_width=suffix_width+gap<234u?234u-suffix_width-gap:0u;
		const unsigned prefix_width=std::min(row.prefix_width,name_width);
		const unsigned text_width=name_width-prefix_width;
		if(prefix_width) ZhRender(row.fixed_prefix.c_str(),top,bottom,prefix_width,
		                         false,row.stipple,0,icons,row.vertical_shift);
		bool aligned_value=false;
		if(!row.raw && !suffix_width && !prefix_width && !row.scroll) {
			const size_t ascii_colon=text.find(':');
			const size_t wide_colon=text.find(u8"：");
			const size_t colon=std::min(ascii_colon,wide_colon);
			if(colon!=std::string::npos) {
				const size_t colon_end=colon+(colon==wide_colon?strlen(u8"："):1);
				const std::string label=trim_layout_space(text.substr(0,colon_end));
				const std::string value=trim_layout_space(text.substr(colon_end));
				const unsigned value_width=ZhTextWidth(value);
				if(!value.empty() && value_width+8u+ZhTextWidth(label)<=name_width) {
					ZhRender(label.c_str(),top,bottom,name_width,false,row.stipple,0,icons,row.vertical_shift);
					ZhRender(value.c_str(),top+name_width-value_width,bottom+name_width-value_width,
					         value_width,false,row.stipple,0,icons,row.vertical_shift);
					aligned_value=true;
				}
			}
		}
		if(!aligned_value) ZhRender(text.c_str(),top+prefix_width,bottom+prefix_width,text_width,
		                           false,row.stipple,row.scroll,icons,row.vertical_shift);
		if(suffix_width) ZhRender(row.right_label.c_str(),top+234-suffix_width,
		                           bottom+234-suffix_width,suffix_width,false,row.stipple,
		                           0,icons,row.vertical_shift);
		for(unsigned x=0;x<234;++x) {
			// Partial highlights are measured in old 8-pixel OSD cells.
			bool inv=row.invert && (int)((x+22)/8)>=row.mininv && (int)((x+22)/8)<row.maxinv;
			uint8_t upper=top[x]^(inv?255:0), lower=bottom[x]^(inv?255:0);
			// Legacy framebuffer is in 8-pixel rows. Chinese OSD occupies
			// two consecutive physical rows for every logical menu entry.
			if(row.use_background && background) {
				upper|=(uint8_t)background[physical*2][x];
				lower|=(uint8_t)background[physical*2+1][x];
			}
			frame[(physical*2)*256+22+x]=upper;
			frame[(physical*2+1)*256+22+x]=lower;
		}
		// Preserve per-row up/down navigation markers supplied by the core menu.
		if(row.arrow && icons) for(unsigned x=0;x<8;++x) {
			unsigned bits=0;
			for(unsigned y=0;y<8;++y) if(icons[row.arrow][x]&(1u<<y)) bits|=3u<<(2*y);
			frame[physical*2*256+6+x]=(uint8_t)bits^255;
			frame[(physical*2+1)*256+6+x]=(uint8_t)(bits>>8)^255;
		}
	}
	// Match pagination indicators to the same compacted list as visible().
	const unsigned upper=std::min(logical_rows,32u);
	const bool pinned=upper && zh_footer_label(page.rows[upper-1]);
	const int slots=pinned?7:8;
	const int entries=page.body_entries;
	if(icons && (page.first>0 || page.first+slots<entries)) {
		for(unsigned x=0;x<8;++x) {
			if(page.first>0) frame[6+x]=icons[17][x]^255;
			if(page.first+slots<entries) {
				// Center the page-down marker across the 16px exit row.
				const uint16_t bits=(uint16_t)icons[16][x]<<4;
				frame[14*256+6+x]=(uint8_t)bits^255;
				frame[15*256+6+x]=(uint8_t)(bits>>8)^255;
			}
		}
	}
}

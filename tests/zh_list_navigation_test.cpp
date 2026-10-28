#include "cn/list_navigation.h"
#include "cn/path_utils.h"
#include "osd.h"
#include <assert.h>
#include <stdio.h>
#include <string>

static bool chinese;
static int logical_rows;
bool OsdChinese() { return chinese; }
int OsdGetSize() { return logical_rows; }

int main()
{
    for (int size : {8, 16, 19, 32}) {
        logical_rows = size;
        chinese = false;
        assert(OsdGetVisibleRows() == size); // English/missing-font fallback.
        chinese = true;
        assert(OsdGetVisibleRows() == 8);
    }
    for (int count = 0; count <= 65; ++count) {
        for (int first = 0; first <= std::max(0, count - 8); ++first) {
            for (int selected = first; selected < std::min(count, first + 8); ++selected) {
                for (bool forward : {false, true}) {
                    int next_first = first, next_selected = selected;
                    OsdListPage(forward, count, 8, next_first, next_selected);
                    assert(next_selected == std::max(0, std::min(count - 1, selected + (forward ? 8 : -8))));
                    assert(next_first >= 0 && next_first <= std::max(0, count - 8));
                    assert(next_selected >= next_first && next_selected < next_first + 8);
                }
            }
        }
    }
    int first = 0, selected = 0;
    OsdListPage(true, 24, 8, first, selected);
    assert(first == 8 && selected == 8);
    OsdListPage(true, 24, 8, first, selected);
    assert(first == 16 && selected == 16);
    OsdListPage(false, 24, 8, first, selected);
    assert(first == 8 && selected == 8);
    OsdListPage(true, 0, 8, first, selected);
    assert(first == 0 && selected == 0);

    // A failed path join must preserve the old selection, never truncate it.
    char path[8] = "old";
    assert(TryJoinPath(path,"abc","def") && !strcmp(path,"abc/def"));
    assert(!TryJoinPath(path,"abc","defg") && !strcmp(path,"abc/def"));
    assert(TryJoinPath(path,"","rom") && !strcmp(path,"rom"));
    assert(TryJoinPath(path,"abc/","def") && !strcmp(path,"abc/def"));
    assert(TryJoinPath(path,"abc","") && !strcmp(path,"abc"));
    assert(TryJoinPath(path,path,"def") && !strcmp(path,"abc/def"));
    assert(!TryJoinPath(path,path,"g") && !strcmp(path,"abc/def"));
    char one[1] = {};
    assert(TryJoinPath(one,"","") && !one[0]);
    assert(!TryJoinPath(one,"","x") && !one[0]);

    char long_path[1024] = "unchanged";
    const std::string directory(924,'d');
    std::string filename;
    for (int i=0; i<85; ++i) filename += u8"中"; // Full 255-byte dirent name.
    assert(!TryJoinPath(long_path,directory.c_str(),filename.c_str()));
    assert(!strcmp(long_path,"unchanged"));
    assert(TryJoinPath(long_path,"Games",filename.c_str()));
    assert(std::string(long_path) == "Games/" + filename);
    char empty_capacity = 'x';
    assert(!TryJoinPath(&empty_capacity,0,"","") && empty_capacity=='x');
    // ZIP splitting needs one extra NUL after the path terminator.
    char zip_path[12] = {};
    zip_path[11] = 'x';
    assert(TryJoinPath(zip_path,sizeof(zip_path)-1,"abc","de.zip"));
    assert(strlen(zip_path)==10 && zip_path[11]=='x');
    assert(!TryJoinPath(zip_path,sizeof(zip_path)-1,"abc","def.zip"));
    assert(!strcmp(zip_path,"abc/de.zip") && zip_path[11]=='x');
    puts("Chinese list pages, English fallback and bounded path joins: PASS");
}

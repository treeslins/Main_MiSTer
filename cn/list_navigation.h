// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <algorithm>

// Move by a displayed page and keep the selection inside the viewport,
// including short lists and a partially filled final page.
inline void OsdListPage(bool forward, int count, int rows, int &first, int &selected)
{
    if (count <= 0 || rows <= 0) { first = selected = 0; return; }
    const int step = forward ? rows : -rows;
    selected = std::max(0, std::min(count - 1, selected + step));
    first = std::max(0, std::min(std::max(0, count - rows), first + step));
    if (selected < first) first = selected;
    if (selected >= first + rows) first = selected - rows + 1;
}

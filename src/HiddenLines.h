// This file is part of Search++ (a plugin for Notepad++),
// Copyright 2026 by Randy Fellmy <https://www.coises.com/>.

// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// at your option any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.

#include "Framework/UtilityFramework.h"
#include <map>

#pragma once

struct HiddenLines {

    bool first = false;  // first line is hidden
    bool last  = false;  // last line is hidden
    bool peek1 = false;  // peek checked for this document in primary view
    bool peek2 = false;  // peek checked for this document in secondary view

    inline static std::map<uintptr_t, HiddenLines> map;
    inline static HiddenLines* active  = 0;
    inline static bool         active2 = false;

    static HiddenLines* activate() {
        active2 = npp(NPPM_GETCURRENTVIEW, 0, 0);
        if (auto it = map.find(npp(NPPM_GETCURRENTBUFFERID, 0, 0)); it != map.end()) active = &(it->second);
        else active = 0;
        return active;
    }
    static HiddenLines& current() { return active ? *active : *(active = &map[npp(NPPM_GETCURRENTBUFFERID, 0, 0)]); }
    static bool peeking() { return !active ? false : active2 ? active->peek2 : active->peek1; }

    static void applyMarkers();
    static void erase(uintptr_t bufferId) { map.erase(bufferId); }
    static void peek();
    static void restore();
    static void restoreView();

};
// This file is part of Search++ (a plugin for Notepad++),
// Copyright 2026 by Randy Fellmy <https://www.coises.com/>.

// The source code contained in this file is independent of Notepad++ code.
// It is released under the MIT (Expat) license:
//
// Permission is hereby granted, free of charge, to any person obtaining a copy of this software and 
// associated documentation files (the "Software"), to deal in the Software without restriction, 
// including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, 
// and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, 
// subject to the following conditions:
// 
// The above copyright notice and this permission notice shall be included in all copies or substantial 
// portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT 
// LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
// IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, 
// WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE 
// SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.


#pragma once

#define NOMINMAX
#include <windows.h>

#include "RegularExpression.h"
#include "SearchableFile.h"

#include <locale>
#include <ppl.h>
#include <string>


constexpr unsigned int WM_APP_SEARCH_STARTED  = WM_APP + 1;
constexpr unsigned int WM_APP_SEARCH_COMPLETE = WM_APP + 2;  // wParam = 0 (normal completion), 1 (canceled) or 2 (failed)
                                                             //    for wParam = 2 (failed), lParam = error code from GetLastError()

inline const std::locale UserLocale("");


struct FileSpecification {
    std::wstring path;
    RegularExpression filter;
    uint64_t minSize = 1, maxSize = std::numeric_limits<uint64_t>::max();
    uint64_t minTime = 0, maxTime = std::numeric_limits<uint64_t>::max();
    enum { TimeNone, TimeCreation, TimeModification, TimeAccess } timePoint = TimeNone;
    bool recursive  = true;
    bool skipHidden = false;
    bool useFilter  = false;
};


// SearchInFiles common data

inline struct SearchInFilesCommonData {
    FileSpecification fileSpecification;
    RegularExpression rx;
    MatchResults      matchResults;
    std::string       findString;
    concurrency::cancellation_token_source cancel_all_source;
} sif;
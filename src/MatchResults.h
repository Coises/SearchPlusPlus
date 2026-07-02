// This file is part of Search++.
// Copyright 2026 by by Randy Fellmy <https://www.coises.com/>.

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

#include <cstdint>
#include <string>
#include <vector>

// A MatchResults structure contains the text of every line that contains any part of a match,
// and a corresponding vector that lists line number of each line and the matches which begin in that line.
// Lines in which no matches begin are included if a match that started in a previous line extends into them.
// Header lines (describing a file or a search) can be included.
// There must always be a one-to-one correspondence between the elements of index and the lines in text.

struct MatchResults {
    struct LineIndex {
        struct SingleMatch {
            intptr_t offset;                // offset into (the UTF-8 translation of) the line in which the match begins
            intptr_t length;                // length of (the UTF-8 translation of) the match
        }; 
        std::vector<SingleMatch> matches;   // list of all matches that begin in this line
        intptr_t lineNumber;                // line number in the original file (negative if this is a header line)
        intptr_t length;                    // length of this line (translated to UTF-8), including line ending
    };
    std::vector<LineIndex> index;           // one entry for each line containing text that is part of any match
    std::string            text;            // text of all lines containing matches, including line endings, translated to UTF-8
    intptr_t               offset = 0;      // offset into text where actual text begins; used to reserve space ahead of time
};

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

#include <string>

struct NumericFormat {
    int  leftPad         = 1;    // left pad to at least this many digits before the decimal separator
    int  minDec          = -1;   // minimum digits after decimal; -1 = do not show separator if no digits after
    int  maxDec          = 6;    // round to this number of digits after the decimal separator
    int  timeEnable      = 1;    // bit mask for enabled formats: 8 (4 segments) + 4 (3 segments) + 2 (2 segments) + 1 (1 segment)
    int  timeScalarUnit  = 3;    // time segment as which to interpert a scalar (no colons): 0 = days, 1 = hours, 2 = minutes, 3 = seconds
    int  timePartialRule = 3;    // interpretation of 2 and 3 segment times: 0 = d:h, d:h:m; 1 = h:m, d:h:m; 2 = h:m, h:m:s; 3 = m:s, h:m:s
    char decimal         = '.';  // decimal separator
    char thousands       = 0;    // thousands separator, or 0 for no thousands separator
    std::string format(double value) const;
};

double parseNumber(const std::wstring& text, wchar_t decimalSeparator = L'.');

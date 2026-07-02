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

#include <concepts>
#include "Framework/ScintillaCallEx.h"

class RegularExpression {
public:
    class Mono;
    class Poly;
private:
    Mono* mono = 0;
    Poly* poly = 0;
public:
    RegularExpression();
    RegularExpression(Scintilla::ScintillaCall& sciCall);
    RegularExpression(const RegularExpression& rx);
    ~RegularExpression();
    bool               can_search() const;
    std::string        find(const std::string& s, bool caseSensitive, bool dotAll, bool freeSpacing);
    std::string        format(const std::string& replacement) const;
    void               invalidate();
    intptr_t           length(int n = 0) const;
    size_t             mark_count() const;
    intptr_t           position(int n = 0) const;
    bool               search(std::string_view s, size_t from = 0);
    bool               search(std::string_view s, std::string& errmsg);
    bool               search(std::string_view s, size_t from, std::string& errmsg);
    bool               search(intptr_t from, intptr_t to, intptr_t start);
    bool               search(intptr_t from, intptr_t to, intptr_t start, std::string& errmsg);
    RegularExpression& setup(unsigned int codepage);
    RegularExpression& setup(Scintilla::ScintillaCall& sciCall);
    size_t             size() const;
    std::string        str(int n = 0) const;
    std::string        str(std::string_view n) const;
    std::wstring       wstr(int n = 0) const;
    std::wstring       wstr(std::string_view n) const;

    template <typename P, typename... Args> requires std::derived_from<P, Poly>
    RegularExpression& custom(Args&&... args) {
        invalidate();
        if (poly) delete poly;
        poly = new P(*mono, std::forward<Args>(args)...);
        return *this;
    }

};
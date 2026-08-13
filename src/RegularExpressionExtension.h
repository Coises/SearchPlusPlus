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

// This header exposes the interfaces needed to implement custom iterators for RegularExpression.
// A custom iterator must be implemented within a class derived from RegularExpression::Poly.
// The derived class must define all the pure virtual functions of Poly so it can be instantiated;
// however, it need not have working implementations of functions that are not used. The class
// can define the iterator as it chooses, subject to the requirements of Boost::Regex; the iterator
// is used in a call to the template boost::regex_search, which should be called in the Poly::search
// implementation.
//
// The class derived from Poly is used as a template parameter to RegularExpression::custom.
// The first argument to the constructor of the class is a pointer to a RegularExpression::Mono
// instance. Additional arguments to the constructor can be passed as arguments to custom.


#pragma once

#include "RegularExpression.h"
#include "Unicode/UnicodeRegexTraits.h"

class RegularExpression::Mono {
public:
    boost::basic_regex<char32_t, utf32_regex_traits> uFind;
    Scintilla::ScintillaCall* sci = 0;
    mutable intptr_t    end = 0;
    mutable intptr_t    gap = 0;
    mutable const char* pt1 = 0;
    mutable const char* pt2 = 0;
    unsigned int codepage = 0;
    bool regexValid = false;
    void ensureValid() {
        if (sci && pt1 == 0 && pt2 == 0) {
            sci->CharacterPointer();  // <=== Temporary test fix; should cause gap always to move to the end
            end = sci->Length();
            gap = sci->GapPosition();
            pt1 = gap > 0 ? reinterpret_cast<const char*>(sci->RangePointer(0, gap)) : 0;
            pt2 = gap < end ? reinterpret_cast<const char*>(sci->RangePointer(gap, end - gap)) - gap : 0;
        }
    }
};

class RegularExpression::Poly {
protected:
    Mono& mono;
public:
    Poly(Mono& mono) : mono(mono) {}
    virtual ~Poly() {}
    virtual std::string  format(const std::string& replacement)                                  const = 0;
    virtual intptr_t     length(int n = 0)                                                       const = 0;
    virtual intptr_t     position(int n = 0)                                                     const = 0;
    virtual bool         search(std::string_view s, size_t from, std::string* errmsg) = 0;
    virtual bool         search(intptr_t from, intptr_t to, intptr_t start, std::string* errmsg) = 0;
    virtual size_t       size()                                                                  const = 0;
    virtual std::string  str(int n = 0)                                                          const = 0;
    virtual std::string  str(std::string_view n)                                                 const = 0;
    virtual std::wstring wstr(int n = 0)                                                         const = 0;
    virtual std::wstring wstr(std::string_view n)                                                const = 0;
};

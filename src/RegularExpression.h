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
#include "Framework/ScintillaCallEx.h"


class RegularExpressionInterface {
    friend class RegularExpression;
protected:
    Scintilla::ScintillaCall& sci;
public:
    RegularExpressionInterface(Scintilla::ScintillaCall& sciCall) : sci(sciCall) {}
    virtual ~RegularExpressionInterface() {}
    virtual bool         can_search(                                                                       ) const = 0;
    virtual std::string  find      (const std::string& s, bool caseSensitive, bool dotAll, bool freeSpacing) = 0;
    virtual std::string  format    (const std::string& replacement                                         ) const = 0;
    virtual void         invalidate(                                                                       )       = 0;
    virtual intptr_t     length    (int n = 0                                                              ) const = 0;
    virtual size_t       mark_count(                                                                       ) const = 0;
    virtual intptr_t     position  (int n = 0                                                              ) const = 0;
    virtual bool         search    (std::string_view s, size_t from = 0                                    )       = 0;
    virtual bool         search    (intptr_t from, intptr_t to, intptr_t start                             )       = 0;
    virtual size_t       size      (                                                                       ) const = 0;
    virtual std::string  str       (int n = 0                                                              ) const = 0;
    virtual std::string  str       (std::string_view n                                                     ) const = 0;
    virtual std::wstring wstr      (int n = 0                                                              ) const = 0;
    virtual std::wstring wstr      (std::string_view n                                                     ) const = 0;
};

class RegularExpression {
    RegularExpressionInterface* rex = 0;
public:
    RegularExpression() {}
    RegularExpression(Scintilla::ScintillaCall& sciCall) { setup(sciCall); }
    ~RegularExpression() { if (rex) delete rex; }
    RegularExpression& setup(Scintilla::ScintillaCall& sciCall);
    bool         can_search(                                                                       ) const {return rex->can_search(                                     );}
    std::string  find      (const std::string& s, bool caseSensitive, bool dotAll, bool freeSpacing)       {return rex->find      (s, caseSensitive, dotAll, freeSpacing);}
    std::string  format    (const std::string& replacement                                         ) const {return rex->format    (replacement                          );}
    void         invalidate(                                                                       )       {       rex->invalidate(                                     );}
    intptr_t     length    (int n = 0                                                              ) const {return rex->length    (n                                    );}
    size_t       mark_count(                                                                       ) const {return rex->mark_count(                                     );}
    intptr_t     position  (int n = 0                                                              ) const {return rex->position  (n                                    );}
    bool         search    (std::string_view s, size_t from = 0                                    )       {return rex->search    (s, from                              );}
    bool         search    (intptr_t from, intptr_t to, intptr_t start                             )       {return rex->search    (from, to, start                      );}
    size_t       size      (                                                                       ) const {return rex->size      (                                     );}
    std::string  str       (int n = 0                                                              ) const {return rex->str       (n                                    );}
    std::string  str       (std::string_view n                                                     ) const {return rex->str       (n                                    );}
    std::wstring wstr      (int n = 0                                                              ) const {return rex->wstr      (n                                    );}
    std::wstring wstr      (std::string_view n                                                     ) const {return rex->wstr      (n                                    );}
};

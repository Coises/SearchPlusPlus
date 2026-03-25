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

// #include <algorithm>
#include "Framework/ScintillaCallEx.h"
#include "Framework/UtilityFrameworkMIT.h"
#include "Framework/UnicodeFormatTranslation.h"

#include "RegularExpression.h"
#include "Unicode\UnicodeRegexTraits.h"
#include <mbstring.h>


class RegularExpressionU : public RegularExpressionInterface {

public:

    class DocumentIterator {

        intptr_t    pos;
        intptr_t    end;
        intptr_t    gap;
        const char* pt1;
        const char* pt2;

        char at(intptr_t cp) const { return cp < gap ? pt1[cp] : pt2[cp]; }

        // Note: Scintilla shows an invalid byte graphic -- an "x" and two hexadecimal digits on an inverted background -- 
        // for each byte in a sequence that cannot be decoded.  Accordingly, instead of following the standard of counting
        // the longest valid prefix as a single error, this iterator regards each byte of an undecodable sequence as an error.
        // An invalid Unicode code point, 0xDC00 + the error byte (which must be between 0x80 and 0xFF), results
        // from dereferencing an iterator to an error byte. This is the same encoding as Python surrogateescape.

        // length(p) returns the length of the sequence indexed by p if it is a valid sequence, or 1 if it is not a valid sequence
        // p must not be less than 0 nor greater than or equal to end

        int length(intptr_t p) const {
            const unsigned char c1 = at(p);
            intptr_t n = utf8byte::implicit_length(c1);
            if (n < 2 || p + n > end) return 1;
            const unsigned char c2 = at(p + 1);
            if (!utf8byte::isTrail(c2)) return 1;
            if (n == 2) return 2;
            if (utf8byte::badPair(c1, c2)) return 1;
            if (!utf8byte::isTrail(at(p + 2))) return 1;
            if (n == 3) return 3;
            return utf8byte::isTrail(at(p + 3)) ? 4 : 1;
        }

        // fix_position advances the iterator position if it is on a continuation byte within a valid character
        // so that it points to the start of a valid character or to an error byte.
        // If this were not done, we could create an iterator that could never return to the same value
        // after being incremented and decremented, which can break the regular expression algorithm.

        void fix_position() {
            if (pos <= 0 || pos >= end || !utf8byte::isTrail(at(pos)) || utf8byte::isASCII(at(pos - 1))) return;
            int n = length(pos - 1);
            if (n > 1) pos += n - 1;
            else if (pos > 1) {
                n = length(pos - 2);
                if (n > 2) pos += n - 2;
                else if (pos > 2) {
                    n = length(pos - 3);
                    if (n > 3) pos += n - 3;
                }
            }
        }

    public:

        using iterator_category = std::bidirectional_iterator_tag;
        using value_type        = char32_t;
        using difference_type   = ptrdiff_t;
        using pointer           = char32_t*;
        using reference         = char32_t&;

        DocumentIterator() : pos(0), end(0), gap(0), pt1(0), pt2(0) {}
        DocumentIterator(RegularExpressionU*     reba, intptr_t pos) : pos(pos), end(reba->end), gap(reba->gap), pt1(reba->pt1), pt2(reba->pt2) { fix_position(); }
        DocumentIterator(const DocumentIterator& di  , intptr_t pos) : pos(pos), end(di.end   ), gap(di.gap   ), pt1(di.pt1   ), pt2(di.pt2   ) { fix_position(); }

        bool operator==(const DocumentIterator& other) const { return pos == other.pos; }
        bool operator!=(const DocumentIterator& other) const { return pos != other.pos; }

        intptr_t          position() const { return pos; }
        DocumentIterator& position(Scintilla::Position at) { pos = at; return *this; }

        DocumentIterator& operator++() {
            pos += length(pos);
            return *this;
        }

        DocumentIterator& operator--() {
            if      (!utf8byte::isTrail(at(pos - 1))) --pos;
            else if (pos < 2              ) --pos;
            else if (length(pos - 2) == 2 ) pos -= 2;
            else if (pos < 3              ) --pos;
            else if (length(pos - 3) == 3 ) pos -= 3;
            else if (pos < 4              ) --pos;
            else if (length(pos - 4) == 4 ) pos -= 4;
            else --pos;
            return *this;
        }

        char32_t operator*() const {
            unsigned char c1 = at(pos);
            if (utf8byte::isASCII(c1)) return c1;
            int n = length(pos);
            return n == 2 ? utf8byte::to32(c1, at(pos + 1))
                 : n == 3 ? utf8byte::to32(c1, at(pos + 1), at(pos + 2))
                 : n == 4 ? utf8byte::to32(c1, at(pos + 1), at(pos + 2), at(pos + 3))
                 : 0xDC00 + c1;  /* Invalid Unicode code point: encode error byte in the same way as Python surrogateescape */
        }

    };

private:

    friend class DocumentIterator;

    mutable intptr_t    end = 0;
    mutable intptr_t    gap = 0;
    mutable const char* pt1 = 0;
    mutable const char* pt2 = 0;

    boost::basic_regex<char32_t, utf32_regex_traits> uFind;
    boost::match_results<DocumentIterator>           uMatch;
    bool                                             regexValid = false;

    void ensureValid() const {
        if (pt1 == 0 && pt2 == 0) {
            end = sci.Length();
            gap = sci.GapPosition();
            pt1 = gap > 0 ? reinterpret_cast<const char*>(sci.RangePointer(0, gap)) : 0;
            pt2 = gap < end ? reinterpret_cast<const char*>(sci.RangePointer(gap, end - gap)) - gap : 0;
        }
    }

public:

    RegularExpressionU(Scintilla::ScintillaCall& sci) : RegularExpressionInterface(sci) {}

    bool can_search() const override { return regexValid; }

    std::string find(const std::string& s, bool caseSensitive, bool dotAll, bool freeSpacing) override {
        try {
            uFind.assign(utf8to32(s), ( (caseSensitive ? 0 : boost::regex_constants::icase)
                                      | (dotAll        ? boost::regex_constants::mod_s : boost::regex_constants::no_mod_s)
                                      | (freeSpacing   ? boost::regex_constants::mod_x : 0) ));
        }
        catch (const boost::regex_error& e) {
            regexValid = false;
            return e.what();
        }
        catch (...) {
            regexValid = false;
            return "Undetermined error processing this regular expression.";
        }
        regexValid = true;
        return "";
    }

    std::string format(const std::string& replacement) const override {
        return utf32to8(uMatch.format(utf8to32(replacement), boost::format_all));
    }

    void invalidate() override {
        end = gap = 0;
        pt1 = pt2 = 0;
    }

    intptr_t length(int n = 0) const override {
        return uMatch.empty() || n < 0 || n >= static_cast<int>(uMatch.size()) ? -1 : uMatch[n].second.position() - uMatch[n].first.position();
    }

    size_t mark_count() const override { return !regexValid ? 0 : uFind.mark_count(); }

    intptr_t position(int n = 0) const override { return uMatch.empty() || n < 0 || n >= static_cast<int>(uMatch.size()) ? -1 : uMatch[n].first.position(); }

    bool search(std::string_view s, size_t from = 0) override {
        if (!regexValid) return false;
        end = gap = s.length();
        pt1 = s.data();
        pt2 = 0;
        try {
            return boost::regex_search(DocumentIterator(this, from), DocumentIterator(this, s.length()), uMatch, uFind,
                                       boost::match_not_dot_newline, DocumentIterator(this, 0));
        }
        catch (const boost::regex_error& e) {
            regexValid = false;
            MessageBox(0, toWide(e.what(), 0).data(), L"Search++: Error in regular expression search", MB_ICONERROR);
        }
        catch (...) {
            regexValid = false;
            MessageBox(0, L"An undetermined error occurred while performing a regular expression search.",
                          L"Search++: Error in regular expression search", MB_ICONERROR);
        }
        return false;
    }

    bool search(intptr_t from, intptr_t to, intptr_t start) override {
        if (!regexValid) return false;
        ensureValid();
        try {
            return boost::regex_search(DocumentIterator(this, from), DocumentIterator(this, to), uMatch, uFind,
                                       boost::match_not_dot_newline, DocumentIterator(this, start));
        }
        catch (const boost::regex_error& e) {
            regexValid = false;
            MessageBox(0, toWide(e.what(), 0).data(), L"Search++: Error in regular expression search", MB_ICONERROR);
        }
        catch (...) {
            regexValid = false;
            MessageBox(0, L"An undetermined error occurred while performing a regular expression search.",
                          L"Search++: Error in regular expression search", MB_ICONERROR);
        }
        return false;
    }

    size_t size() const override { return uMatch.size(); }

    std::string str(int n) const override {
        ensureValid();
        if (uMatch.empty() || n < 0 || n >= static_cast<int>(uMatch.size()) || !uMatch[n].matched) return "";
        Scintilla::Position s1 = uMatch[n].first.position();
        Scintilla::Position s2 = uMatch[n].second.position();
        if (s2 <= gap) return std::string(pt1 + s1, pt1 + s2);
        if (s1 >= gap) return std::string(pt2 + s1, pt2 + s2);
        return std::string(pt1 + s1, pt1 + gap) + std::string(pt2 + gap, pt2 + s2);
    }

    std::string str(std::string_view n) const override {
        ensureValid();
        if (uMatch.empty() || n.empty()) return "";
        auto x = uMatch[n.data()];
        if (!x.matched) return "";
        Scintilla::Position s1 = uMatch[n.data()].first.position();
        Scintilla::Position s2 = uMatch[n.data()].second.position();
        if (s2 <= gap) return std::string(pt1 + s1, pt1 + s2);
        if (s1 >= gap) return std::string(pt2 + s1, pt2 + s2);
        return std::string(pt1 + s1, pt1 + gap) + std::string(pt2 + gap, pt2 + s2);
    }

    std::wstring wstr(int              n) const override { return utf8to16(str(n)); }
    std::wstring wstr(std::string_view n) const override { return utf8to16(str(n)); }

};


class RegularExpressionSBCS : public RegularExpressionInterface {

public:

    class DocumentIterator {

        intptr_t    pos;
        intptr_t    gap;
        const char* pt1;
        const char* pt2;

    public:

        using iterator_category = std::bidirectional_iterator_tag;
        using value_type        = char32_t;
        using difference_type   = ptrdiff_t;
        using pointer           = char32_t*;
        using reference         = char32_t&;

        DocumentIterator() : pos(0), gap(0), pt1(0), pt2(0) {}
        DocumentIterator(RegularExpressionSBCS* reba, intptr_t pos) : pos(pos), gap(reba->gap), pt1(reba->pt1), pt2(reba->pt2) {}
        DocumentIterator(const DocumentIterator&  di, intptr_t pos) : pos(pos), gap(di.gap   ), pt1(di.pt1   ), pt2(di.pt2   ) {}

        bool operator==(const DocumentIterator& other) const { return pos == other.pos; }
        bool operator!=(const DocumentIterator& other) const { return pos != other.pos; }

        intptr_t          position() const { return pos; }
        DocumentIterator& position(Scintilla::Position at) { pos = at; return *this; }

        DocumentIterator& operator++() { ++pos; return *this; }
        DocumentIterator& operator--() { --pos; return *this; }

        char32_t operator*() const {
            static const struct Map {
                char32_t value[256];
                Map() {
                    char sbc[1];
                    wchar_t wide[2];
                    for (int i = 0; i < 256; ++i) {
                        sbc[0] = static_cast<unsigned char>(i);
                        switch (MultiByteToWideChar(CP_ACP, MB_ERR_INVALID_CHARS, sbc, 1, wide, 2)) {
                        case 1:
                            value[i] = wide[0];
                            break;
                        case 2:
                            value[i] = (static_cast<char32_t>(wide[0] & 0x7FF) << 10 | (wide[1] & 0x03FF)) + 0x10000;
                            break;
                        default:
                            value[i] = 0xDC00 + static_cast<unsigned char>(sbc[0]);  /* Invalid: encode like Python surrogateescape */
                        }
                    }
                }
            } map;
            return map.value[static_cast<unsigned char>(pos < gap ? pt1[pos] : pt2[pos])];
        }

    };

private:

    friend class DocumentIterator;

    mutable intptr_t    end = 0;
    mutable intptr_t    gap = 0;
    mutable const char* pt1 = 0;
    mutable const char* pt2 = 0;

    boost::basic_regex<char32_t, utf32_regex_traits> uFind;
    boost::match_results<DocumentIterator>           uMatch;
    bool                                             regexValid = false;

    void ensureValid() const {
        if (pt1 == 0 && pt2 == 0) {
            end = sci.Length();
            gap = sci.GapPosition();
            pt1 = gap > 0 ? reinterpret_cast<const char*>(sci.RangePointer(0, gap)) : 0;
            pt2 = gap < end ? reinterpret_cast<const char*>(sci.RangePointer(gap, end - gap)) - gap : 0;
        }
    }

public:

    RegularExpressionSBCS(Scintilla::ScintillaCall& sci) : RegularExpressionInterface(sci) {}

    bool can_search() const override { return regexValid; }

    std::string find(const std::string& s, bool caseSensitive, bool dotAll, bool freeSpacing) override {
        try {
            uFind.assign(utf8to32(s), ( (caseSensitive ? 0 : boost::regex_constants::icase)
                                      | (dotAll        ? boost::regex_constants::mod_s : boost::regex_constants::no_mod_s)
                                      | (freeSpacing   ? boost::regex_constants::mod_x : 0) ));
        }
        catch (const boost::regex_error& e) {
            regexValid = false;
            return e.what();
        }
        catch (...) {
            regexValid = false;
            return "Undetermined error processing this regular expression.";
        }
        regexValid = true;
        return "";
    }

    std::string format(const std::string& replacement) const override {
        return fromWide(utf32to16(uMatch.format(utf8to32(replacement), boost::format_all)), 0);
    }

    void invalidate() override {
        end = gap = 0;
        pt1 = pt2 = 0;
    }

    intptr_t length(int n = 0) const override {
        return uMatch.empty() || n < 0 || n >= static_cast<int>(uMatch.size()) ? -1 : uMatch[n].second.position() - uMatch[n].first.position();
    }

    size_t mark_count() const override { return !regexValid ? 0 : uFind.mark_count(); }

    intptr_t position(int n = 0) const override { return uMatch.empty() || n < 0 || n >= static_cast<int>(uMatch.size()) ? -1 : uMatch[n].first.position(); }

    bool search(std::string_view s, size_t from = 0) override {
        if (!regexValid) return false;
        end = gap = s.length();
        pt1 = s.data();
        pt2 = 0;
        try {
            return boost::regex_search(DocumentIterator(this, from), DocumentIterator(this, s.length()), uMatch, uFind,
                                       boost::match_not_dot_newline, DocumentIterator(this, 0));
        }
        catch (const boost::regex_error& e) {
            regexValid = false;
            MessageBox(0, toWide(e.what(), 0).data(), L"Search++: Error in regular expression search", MB_ICONERROR);
        }
        catch (...) {
            regexValid = false;
            MessageBox(0, L"An undetermined error occurred while performing a regular expression search.",
                          L"Search++: Error in regular expression search", MB_ICONERROR);
        }
        return false;
    }

    bool search(intptr_t from, intptr_t to, intptr_t start) override {
        if (!regexValid) return false;
        ensureValid();
        try {
            return boost::regex_search(DocumentIterator(this, from), DocumentIterator(this, to), uMatch, uFind,
                                       boost::match_not_dot_newline, DocumentIterator(this, start));
        }
        catch (const boost::regex_error& e) {
            regexValid = false;
            MessageBox(0, toWide(e.what(), 0).data(), L"Search++: Error in regular expression search", MB_ICONERROR);
        }
        catch (...) {
            regexValid = false;
            MessageBox(0, L"An undetermined error occurred while performing a regular expression search.",
                          L"Search++: Error in regular expression search", MB_ICONERROR);
        }
        return false;
    }

    size_t size() const override { return uMatch.size(); }

    std::string str(int n) const override {
        ensureValid();
        if (uMatch.empty() || n < 0 || n >= static_cast<int>(uMatch.size()) || !uMatch[n].matched) return "";
        Scintilla::Position s1 = uMatch[n].first.position();
        Scintilla::Position s2 = uMatch[n].second.position();
        if (s2 <= gap) return std::string(pt1 + s1, pt1 + s2);
        if (s1 >= gap) return std::string(pt2 + s1, pt2 + s2);
        return std::string(pt1 + s1, pt1 + gap) + std::string(pt2 + gap, pt2 + s2);
    }

    std::string str(std::string_view n) const override {
        ensureValid();
        if (uMatch.empty() || n.empty()) return "";
        auto x = uMatch[n.data()];
        if (!x.matched) return "";
        Scintilla::Position s1 = uMatch[n.data()].first.position();
        Scintilla::Position s2 = uMatch[n.data()].second.position();
        if (s2 <= gap) return std::string(pt1 + s1, pt1 + s2);
        if (s1 >= gap) return std::string(pt2 + s1, pt2 + s2);
        return std::string(pt1 + s1, pt1 + gap) + std::string(pt2 + gap, pt2 + s2);
    }

    std::wstring wstr(int              n) const override { return toWide(str(n), 0); }
    std::wstring wstr(std::string_view n) const override { return toWide(str(n), 0); }

};


class RegularExpressionDBCS : public RegularExpressionInterface {

public:

    class DocumentIterator {

        mutable intptr_t    pos;
        mutable intptr_t    end;
        mutable intptr_t    gap;
        mutable const char* pt1;
        mutable const char* pt2;

        char at(intptr_t cp) const { return cp < gap ? pt1[cp] : pt2[cp]; }

        bool canBeLead(intptr_t p) const {
            static const struct Map {
                bool lead[256];
                Map() { for (int i = 0; i < 256; ++i) lead[i] = _ismbblead(i); }
            } map;
            return map.lead[static_cast<unsigned char>(at(p))];
        }

        bool canBeTrail(intptr_t p) const {
            static const struct Map {
                bool trail[256];
                Map() { for (int i = 0; i < 256; ++i) trail[i] = _ismbbtrail(i); }
            } map;
            return map.trail[static_cast<unsigned char>(at(p))];
        }

        // length(p) returns the length of the sequence indexed by p if it is a valid sequence, or 1 if it is not a valid sequence
        // p must not be less than 0 nor greater than or equal to end

        int length(intptr_t p) const {
            if (!canBeLead(p)) return 1;
            return p + 1 < end && canBeTrail(p + 1) ? 2 : 1;
        }

        // fix_position advances the iterator position if it is on a trailing byte within a valid character
        // so that it points to the start of a valid character or to an error byte.
        // If this were not done, we could create an iterator that could never return to the same value
        // after being incremented and decremented, which can break the regular expression algorithm.
        // To synchronize, go backward to find a byte that cannot be a lead byte. Following that point are
        // either valid pairs, or pairs of a lead byte and an invalid trail byte. If the string of bytes
        // preceding pos that could be lead bytes is even in length, pos is a valid character position;
        // if odd, either pos points to a trail byte or an error byte (invalid trail byte).

        void fix_position() {
            if (pos <= 0 || pos >= end || !canBeTrail(pos)) return;
            intptr_t p = pos;
            while (p > 0 && canBeLead(p - 1)) --p;
            if (((pos - p) & 1)) ++pos;
        }

    public:

        using iterator_category = std::bidirectional_iterator_tag;
        using value_type        = char32_t;
        using difference_type   = ptrdiff_t;
        using pointer           = char32_t*;
        using reference         = char32_t&;

        DocumentIterator() : pos(0), end(0), gap(0), pt1(0), pt2(0) {}
        DocumentIterator(RegularExpressionDBCS*  reba, intptr_t pos) : pos(pos), end(reba->end), gap(reba->gap), pt1(reba->pt1), pt2(reba->pt2) { fix_position(); }
        DocumentIterator(const DocumentIterator& di  , intptr_t pos) : pos(pos), end(di.end   ), gap(di.gap   ), pt1(di.pt1   ), pt2(di.pt2   ) { fix_position(); }

        bool operator==(const DocumentIterator& other) const { return pos == other.pos; }
        bool operator!=(const DocumentIterator& other) const { return pos != other.pos; }

        intptr_t          position() const { return pos; }
        DocumentIterator& position(Scintilla::Position at) { pos = at; return *this; }

        DocumentIterator& operator++() {
            pos += length(pos);
            return *this;
        }

        DocumentIterator& operator--() {  // logic for decrement is similar to fix_position above
            if (pos <= 1) pos = 0;
            else {
                --pos;
                if (canBeTrail(pos)) {
                    intptr_t p = pos;
                    while (p > 0 && canBeLead(p - 1)) --p;
                    if (((pos - p) & 1)) --pos;
                }
            }
            return *this;
        }

        char32_t operator*() const {
            const unsigned char c = at(pos);
            if (c < 0x80) return c;
            wchar_t wide[1];
            char dbcs[2];
            dbcs[0] = c;
            int n = length(pos);
            if (n == 2) dbcs[1] = at(pos + 1);
            if (MultiByteToWideChar(CP_ACP, MB_ERR_INVALID_CHARS, dbcs, n, wide, 1)) return wide[0];
            return 0xDC00 + c;  /* invalid sequence; encode error byte in the same way as Python surrogateescape */
        }

    };

private:

    friend class DocumentIterator;

    mutable intptr_t    end = 0;
    mutable intptr_t    gap = 0;
    mutable const char* pt1 = 0;
    mutable const char* pt2 = 0;

    boost::basic_regex<char32_t, utf32_regex_traits> uFind;
    boost::match_results<DocumentIterator>           uMatch;
    bool                                             regexValid = false;

    void ensureValid() const {
        if (pt1 == 0 && pt2 == 0) {
            end = sci.Length();
            gap = sci.GapPosition();
            pt1 = gap > 0 ? reinterpret_cast<const char*>(sci.RangePointer(0, gap)) : 0;
            pt2 = gap < end ? reinterpret_cast<const char*>(sci.RangePointer(gap, end - gap)) - gap : 0;
        }
    }

public:

    RegularExpressionDBCS(Scintilla::ScintillaCall& sci) : RegularExpressionInterface(sci) {}

    bool can_search() const override { return regexValid; }

    std::string find(const std::string& s, bool caseSensitive, bool dotAll, bool freeSpacing) override {
        try {
            uFind.assign(utf8to32(s), ( (caseSensitive ? 0 : boost::regex_constants::icase)
                                      | (dotAll        ? boost::regex_constants::mod_s : boost::regex_constants::no_mod_s)
                                      | (freeSpacing   ? boost::regex_constants::mod_x : 0) ));
        }
        catch (const boost::regex_error& e) {
            regexValid = false;
            return e.what();
        }
        catch (...) {
            regexValid = false;
            return "Undetermined error processing this regular expression.";
        }
        regexValid = true;
        return "";
    }

    std::string format(const std::string& replacement) const override {
        return fromWide(utf32to16(uMatch.format(utf8to32(replacement), boost::format_all)), 0);
    }

    void invalidate() override {
        end = gap = 0;
        pt1 = pt2 = 0;
    }

    intptr_t length(int n = 0) const override {
        return uMatch.empty() || n < 0 || n >= static_cast<int>(uMatch.size()) ? -1 : uMatch[n].second.position() - uMatch[n].first.position();
    }

    size_t mark_count() const override { return !regexValid ? 0 : uFind.mark_count(); }

    intptr_t position(int n = 0) const override { return uMatch.empty() || n < 0 || n >= static_cast<int>(uMatch.size()) ? -1 : uMatch[n].first.position(); }

    bool search(std::string_view s, size_t from = 0) override {
        if (!regexValid) return false;
        end = gap = s.length();
        pt1 = s.data();
        pt2 = 0;
        try {
            return boost::regex_search(DocumentIterator(this, from), DocumentIterator(this, s.length()), uMatch, uFind,
                                       boost::match_not_dot_newline, DocumentIterator(this, 0));
        }
        catch (const boost::regex_error& e) {
            regexValid = false;
            MessageBox(0, toWide(e.what(), 0).data(), L"Error in regular expression search", MB_ICONERROR);
        }
        catch (...) {
            regexValid = false;
            MessageBox(0, L"An undetermined error occurred while performing a regular expression search.",
                          L"Error in regular expression search", MB_ICONERROR);
        }
        return false;
    }

    bool search(intptr_t from, intptr_t to, intptr_t start) override {
        if (!regexValid) return false;
        ensureValid();
        try {
            return boost::regex_search(DocumentIterator(this, from), DocumentIterator(this, to), uMatch, uFind,
                                       boost::match_not_dot_newline, DocumentIterator(this, start));
        }
        catch (const boost::regex_error& e) {
            regexValid = false;
            MessageBox(0, toWide(e.what(), 0).data(), L"Error in regular expression search", MB_ICONERROR);
        }
        catch (...) {
            regexValid = false;
            MessageBox(0, L"An undetermined error occurred while performing a regular expression search.",
                          L"Error in regular expression search", MB_ICONERROR);
        }
        return false;
    }

    size_t size() const override { return uMatch.size(); }

    std::string str(int n) const override {
        ensureValid();
        if (uMatch.empty() || n < 0 || n >= static_cast<int>(uMatch.size()) || !uMatch[n].matched) return "";
        Scintilla::Position s1 = uMatch[n].first.position();
        Scintilla::Position s2 = uMatch[n].second.position();
        if (s2 <= gap) return std::string(pt1 + s1, pt1 + s2);
        if (s1 >= gap) return std::string(pt2 + s1, pt2 + s2);
        return std::string(pt1 + s1, pt1 + gap) + std::string(pt2 + gap, pt2 + s2);
    }

    std::string str(std::string_view n) const override {
        ensureValid();
        if (uMatch.empty() || n.empty()) return "";
        auto x = uMatch[n.data()];
        if (!x.matched) return "";
        Scintilla::Position s1 = uMatch[n.data()].first.position();
        Scintilla::Position s2 = uMatch[n.data()].second.position();
        if (s2 <= gap) return std::string(pt1 + s1, pt1 + s2);
        if (s1 >= gap) return std::string(pt2 + s1, pt2 + s2);
        return std::string(pt1 + s1, pt1 + gap) + std::string(pt2 + gap, pt2 + s2);
    }

    std::wstring wstr(int              n) const override { return toWide(str(n), 0); }
    std::wstring wstr(std::string_view n) const override { return toWide(str(n), 0); }

};


#include "RegularExpressionTS.h"

RegularExpression& RegularExpression::setup(Scintilla::ScintillaCall& sciCall) {
    if (rex) delete rex;
    switch (sciCall.CodePage()) {
    case 0:
        rex = new RegularExpressionSBCS(sciCall);
        break;
    case CP_UTF8:
        rex = new RegularExpressionU(sciCall);
        break;
    default:
        rex = new RegularExpressionDBCS(sciCall);
    }
    return *this;
}

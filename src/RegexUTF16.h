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

// This header defines implementations of two extensions to RegularExpression:
// RegularExpressionLE manages UTF-16LE files and RegularExpressionBE manages UTF-16BE files.

#pragma once

#include "Framework/UtilityFrameworkMIT.h"
#include "RegularExpressionExtension.h"


class RegularExpressionLE : public RegularExpression::Poly {

public:

    class DocumentIterator {

        intptr_t    pos;
        intptr_t    end;
        const char* pt1;

        char at(intptr_t cp) const { return pt1[cp]; }

        // length(p) returns the length of the sequence indexed by p if it is a valid sequence, or 2 if it is not a valid sequence.
        // p must not be odd, less than 0 nor greater than or equal to end.

        int length(intptr_t p) const {
            const unsigned char c2 = at(p + 1);
            if (c2 < 0xD8 || c2 > 0xDB || p + 3 >= end) return 2;
            const unsigned char c4 = at(p + 3);
            return c4 < 0xDC || c4 > 0xDF ? 2 : 4;
        }

        // fix_position advances the iterator position if it is on an odd character position or a trailing surrogate
        // within a valid surrogate pair so that it points to the start of a valid character (or end).
        // If this were not done, we could create an iterator that could never return to the same value
        // after being incremented and decremented, which can break the regular expression algorithm.

        void fix_position() {
            if (pos <= 0) pos = 0;
            else if (pos >= end - 1) pos = end;
            else {
                if (pos & 1) ++pos;
                const unsigned char c4 = at(pos + 1);
                if (c4 >= 0xDC && c4 <= 0xDF && pos >= 2) {
                    const unsigned char c2 = at(pos - 1);
                    if (c2 >= 0xD8 && c2 <= 0xDB) pos += 2;
                }
            }
        }

    public:

        using iterator_category = std::bidirectional_iterator_tag;
        using value_type = char32_t;
        using difference_type = ptrdiff_t;
        using pointer = char32_t*;
        using reference = char32_t&;

        DocumentIterator() : pos(0), end(0), pt1(0) {}
        DocumentIterator(const DocumentIterator& di, intptr_t pos) : pos(pos), end(di.end), pt1(di.pt1) { fix_position(); }
        DocumentIterator(const RegularExpression::Mono& mono, intptr_t pos) : pos(pos), end(mono.end), pt1(mono.pt1) { fix_position(); }

        bool operator==(const DocumentIterator& other) const { return pos == other.pos; }
        bool operator!=(const DocumentIterator& other) const { return pos != other.pos; }

        intptr_t          position() const { return pos; }
        DocumentIterator& position(intptr_t at) { pos = at; return *this; }

        DocumentIterator& operator++() {
            pos += length(pos);
            return *this;
        }

        DocumentIterator& operator--() {
            if (pos < 4) pos = 0;
            else {
                pos -= 2;
                unsigned char c4 = at(pos + 1);
                if (c4 >= 0xDC && c4 <= 0xDF) {
                    unsigned char c2 = at(pos - 1);
                    if (c2 >= 0xD8 && c2 <= 0xDB) pos -= 2;
                }
            }
            return *this;
        }

        char32_t operator*() const {
            unsigned char c1 = at(pos);
            unsigned char c2 = at(pos + 1);
            if (c2 < 0xD8 || c2 > 0xDB || pos + 3 >= end) return (static_cast<char32_t>(c2) << 8) | static_cast<char32_t>(c1);
            unsigned char c4 = at(pos + 3);
            if (c4 < 0xDC || c4 > 0xDF) return (static_cast<char32_t>(c2) << 8) | static_cast<char32_t>(c1);
            unsigned char c3 = at(pos + 2);
            return (((static_cast<char32_t>(c2) & 3) << 24)
                | (static_cast<char32_t>(c1) << 16)
                | ((static_cast<char32_t>(c4) & 3) << 8)
                | static_cast<char32_t>(c3)) + 0x10000;
        }

    };

private:

    friend class DocumentIterator;
    boost::match_results<DocumentIterator> uMatch;

public:

    RegularExpressionLE(RegularExpression::Mono& mono) : Poly(mono) {}

    std::string format(const std::string& replacement) const override {
        return utf32to8(uMatch.format(utf8to32(replacement), boost::format_all));
    }

    intptr_t length(int n = 0) const override {
        return uMatch.empty() || n < 0 || n >= static_cast<int>(uMatch.size())
            ? -1 : uMatch[n].second.position() - uMatch[n].first.position();
    }

    intptr_t position(int n = 0) const override {
        return uMatch.empty() || n < 0 || n >= static_cast<int>(uMatch.size()) ? -1 : uMatch[n].first.position();
    }

    bool search(std::string_view s, size_t from, std::string* errmsg) override {
        if (!mono.regexValid) return false;
        mono.end = mono.gap = s.length();
        mono.pt1 = s.data();
        mono.pt2 = 0;
        try {
            return boost::regex_search(DocumentIterator(mono, from), DocumentIterator(mono, s.length()), uMatch, mono.uFind,
                boost::match_not_dot_newline, DocumentIterator(mono, 0));
        }
        catch (const boost::regex_error& e) {
            mono.regexValid = false;
            if (errmsg) *errmsg = e.what();
            else MessageBox(0, toWide(e.what(), 0).data(), L"Search++: Error in regular expression search", MB_ICONERROR);
        }
        catch (...) {
            mono.regexValid = false;
            if (errmsg) *errmsg = "An undetermined error occurred while performing a regular expression search.";
            else MessageBox(0, L"An undetermined error occurred while performing a regular expression search.",
                L"Search++: Error in regular expression search", MB_ICONERROR);
        }
        return false;
    }

    bool search(intptr_t, intptr_t, intptr_t, std::string*) override { return false; }  // not implemented

    size_t size() const override { return uMatch.size(); }

    std::wstring wstr(int n) const override {
        if (uMatch.empty() || n < 0 || n >= static_cast<int>(uMatch.size()) || !uMatch[n].matched) return L"";
        intptr_t s1 = uMatch[n].first.position();
        intptr_t s2 = uMatch[n].second.position();
        std::wstring ws((s2 - s1) / 2, 0);
        memcpy(ws.data(), mono.pt1 + s1, s2 - s1);
        return ws;
    }

    std::wstring wstr(std::string_view n) const override {
        if (uMatch.empty() || n.empty()) return L"";
        auto x = uMatch[n.data()];
        if (!x.matched) return L"";
        intptr_t s1 = uMatch[n.data()].first.position();
        intptr_t s2 = uMatch[n.data()].second.position();
        std::wstring ws((s2 - s1) / 2, 0);
        memcpy(ws.data(), mono.pt1 + s1, s2 - s1);
        return ws;
    }

    std::string str(int              n) const override { return utf16to8(wstr(n)); }
    std::string str(std::string_view n) const override { return utf16to8(wstr(n)); }

};


class RegularExpressionBE : public RegularExpression::Poly {

public:

    class DocumentIterator {

        intptr_t    pos;
        intptr_t    end;
        const char* pt1;

        char at(intptr_t cp) const { return pt1[cp]; }

        // length(p) returns the length of the sequence indexed by p if it is a valid sequence, or 2 if it is not a valid sequence.
        // p must not be odd, less than 0 nor greater than or equal to end.

        int length(intptr_t p) const {
            const unsigned char c1 = at(p);
            if (c1 < 0xD8 || c1 > 0xDB || p + 3 >= end) return 2;
            const unsigned char c3 = at(p + 2);
            return c3 < 0xDC || c3 > 0xDF ? 2 : 4;
        }

        // fix_position advances the iterator position if it is on an odd character position or a trailing surrogate
        // within a valid surrogate pair so that it points to the start of a valid character (or end).
        // If this were not done, we could create an iterator that could never return to the same value
        // after being incremented and decremented, which can break the regular expression algorithm.

        void fix_position() {
            if (pos <= 0) pos = 0;
            else if (pos >= end - 1) pos = end;
            else {
                if (pos & 1) ++pos;
                const unsigned char c3 = at(pos);
                if (c3 >= 0xDC && c3 <= 0xDF && pos >= 2) {
                    const unsigned char c1 = at(pos - 2);
                    if (c1 >= 0xD8 && c1 <= 0xDB) pos += 2;
                }
            }
        }

    public:

        using iterator_category = std::bidirectional_iterator_tag;
        using value_type = char32_t;
        using difference_type = ptrdiff_t;
        using pointer = char32_t*;
        using reference = char32_t&;

        DocumentIterator() : pos(0), end(0), pt1(0) {}
        DocumentIterator(const DocumentIterator& di, intptr_t pos) : pos(pos), end(di.end), pt1(di.pt1) { fix_position(); }
        DocumentIterator(const RegularExpression::Mono& mono, intptr_t pos) : pos(pos), end(mono.end), pt1(mono.pt1) { fix_position(); }

        bool operator==(const DocumentIterator& other) const { return pos == other.pos; }
        bool operator!=(const DocumentIterator& other) const { return pos != other.pos; }

        intptr_t          position() const { return pos; }
        DocumentIterator& position(intptr_t at) { pos = at; return *this; }

        DocumentIterator& operator++() {
            pos += length(pos);
            return *this;
        }

        DocumentIterator& operator--() {
            if (pos < 4) pos = 0;
            else {
                pos -= 2;
                unsigned char c3 = at(pos);
                if (c3 >= 0xDC && c3 <= 0xDF) {
                    unsigned char c1 = at(pos - 2);
                    if (c1 >= 0xD8 && c1 <= 0xDB) pos -= 2;
                }
            }
            return *this;
        }

        char32_t operator*() const {
            unsigned char c1 = at(pos);
            unsigned char c2 = at(pos + 1);
            if (c1 < 0xD8 || c1 > 0xDB || pos + 3 >= end) return (static_cast<char32_t>(c1) << 8) | static_cast<char32_t>(c2);
            unsigned char c3 = at(pos + 2);
            if (c3 < 0xDC || c3 > 0xDF) return (static_cast<char32_t>(c1) << 8) | static_cast<char32_t>(c2);
            unsigned char c4 = at(pos + 3);
            return (((static_cast<char32_t>(c1) & 3) << 24)
                | (static_cast<char32_t>(c2) << 16)
                | ((static_cast<char32_t>(c3) & 3) << 8)
                | static_cast<char32_t>(c4)) + 0x10000;
        }

    };

private:

    friend class DocumentIterator;
    boost::match_results<DocumentIterator> uMatch;

public:

    RegularExpressionBE(RegularExpression::Mono& mono) : Poly(mono) {}

    std::string format(const std::string& replacement) const override {
        return utf32to8(uMatch.format(utf8to32(replacement), boost::format_all));
    }

    intptr_t length(int n = 0) const override {
        return uMatch.empty() || n < 0 || n >= static_cast<int>(uMatch.size())
            ? -1 : uMatch[n].second.position() - uMatch[n].first.position();
    }

    intptr_t position(int n = 0) const override {
        return uMatch.empty() || n < 0 || n >= static_cast<int>(uMatch.size()) ? -1 : uMatch[n].first.position();
    }

    bool search(std::string_view s, size_t from, std::string* errmsg) override {
        if (!mono.regexValid) return false;
        mono.end = mono.gap = s.length();
        mono.pt1 = s.data();
        mono.pt2 = 0;
        try {
            return boost::regex_search(DocumentIterator(mono, from), DocumentIterator(mono, s.length()), uMatch, mono.uFind,
                boost::match_not_dot_newline, DocumentIterator(mono, 0));
        }
        catch (const boost::regex_error& e) {
            mono.regexValid = false;
            if (errmsg) *errmsg = e.what();
            else MessageBox(0, toWide(e.what(), 0).data(), L"Search++: Error in regular expression search", MB_ICONERROR);
        }
        catch (...) {
            mono.regexValid = false;
            if (errmsg) *errmsg = "An undetermined error occurred while performing a regular expression search.";
            else MessageBox(0, L"An undetermined error occurred while performing a regular expression search.",
                L"Search++: Error in regular expression search", MB_ICONERROR);
        }
        return false;
    }

    bool search(intptr_t, intptr_t, intptr_t, std::string*) override { return false; }  // not implemented

    size_t size() const override { return uMatch.size(); }

    std::wstring wstr(int n) const override {
        if (uMatch.empty() || n < 0 || n >= static_cast<int>(uMatch.size()) || !uMatch[n].matched) return L"";
        intptr_t s1 = uMatch[n].first.position();
        intptr_t s2 = uMatch[n].second.position();
        std::wstring ws((s2 - s1) / 2, 0);
        memcpy(ws.data(), mono.pt1 + s1, s2 - s1);
        return ws;
    }

    std::wstring wstr(std::string_view n) const override {
        if (uMatch.empty() || n.empty()) return L"";
        auto x = uMatch[n.data()];
        if (!x.matched) return L"";
        intptr_t s1 = uMatch[n.data()].first.position();
        intptr_t s2 = uMatch[n.data()].second.position();
        std::wstring ws((s2 - s1) / 2, 0);
        memcpy(ws.data(), mono.pt1 + s1, s2 - s1);
        return ws;
    }

    std::string str(int             ) const override { return ""; }  // not implemented
    std::string str(std::string_view) const override { return ""; }  // not implemented

};

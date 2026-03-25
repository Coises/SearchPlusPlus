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

#include "..\Framework\UnicodeFormatTranslation.h"

#include <windows.h>
#include <map>
#include <set>
#include <string>
#include <unicode/uchar.h>
#include <unicode/coll.h>

#pragma warning( push )
#pragma warning( disable : 4244 )
#include <boost/regex.hpp>
#pragma warning( pop )

namespace {

    inline uint64_t catMask(uint32_t c) { return 0x100000000Ui64 << u_charType(c); }

    constexpr uint64_t CatMask_Cn = 1Ui64 << (32 + U_GENERAL_OTHER_TYPES);
    constexpr uint64_t CatMask_Cc = 1Ui64 << (32 + U_CONTROL_CHAR);
    constexpr uint64_t CatMask_Cf = 1Ui64 << (32 + U_FORMAT_CHAR);
    constexpr uint64_t CatMask_Co = 1Ui64 << (32 + U_PRIVATE_USE_CHAR);
    constexpr uint64_t CatMask_Cs = 1Ui64 << (32 + U_SURROGATE);
    constexpr uint64_t CatMask_Ll = 1Ui64 << (32 + U_LOWERCASE_LETTER);
    constexpr uint64_t CatMask_Lm = 1Ui64 << (32 + U_MODIFIER_LETTER);
    constexpr uint64_t CatMask_Lo = 1Ui64 << (32 + U_OTHER_LETTER);
    constexpr uint64_t CatMask_Lt = 1Ui64 << (32 + U_TITLECASE_LETTER);
    constexpr uint64_t CatMask_Lu = 1Ui64 << (32 + U_UPPERCASE_LETTER);
    constexpr uint64_t CatMask_Mc = 1Ui64 << (32 + U_COMBINING_SPACING_MARK);
    constexpr uint64_t CatMask_Me = 1Ui64 << (32 + U_ENCLOSING_MARK);
    constexpr uint64_t CatMask_Mn = 1Ui64 << (32 + U_NON_SPACING_MARK);
    constexpr uint64_t CatMask_Nd = 1Ui64 << (32 + U_DECIMAL_DIGIT_NUMBER);
    constexpr uint64_t CatMask_Nl = 1Ui64 << (32 + U_LETTER_NUMBER);
    constexpr uint64_t CatMask_No = 1Ui64 << (32 + U_OTHER_NUMBER);
    constexpr uint64_t CatMask_Pc = 1Ui64 << (32 + U_CONNECTOR_PUNCTUATION);
    constexpr uint64_t CatMask_Pd = 1Ui64 << (32 + U_DASH_PUNCTUATION);
    constexpr uint64_t CatMask_Pe = 1Ui64 << (32 + U_END_PUNCTUATION);
    constexpr uint64_t CatMask_Pf = 1Ui64 << (32 + U_FINAL_PUNCTUATION);
    constexpr uint64_t CatMask_Pi = 1Ui64 << (32 + U_INITIAL_PUNCTUATION);
    constexpr uint64_t CatMask_Po = 1Ui64 << (32 + U_OTHER_PUNCTUATION);
    constexpr uint64_t CatMask_Ps = 1Ui64 << (32 + U_START_PUNCTUATION);
    constexpr uint64_t CatMask_Sc = 1Ui64 << (32 + U_CURRENCY_SYMBOL);
    constexpr uint64_t CatMask_Sk = 1Ui64 << (32 + U_MODIFIER_SYMBOL);
    constexpr uint64_t CatMask_Sm = 1Ui64 << (32 + U_MATH_SYMBOL);
    constexpr uint64_t CatMask_So = 1Ui64 << (32 + U_OTHER_SYMBOL);
    constexpr uint64_t CatMask_Zl = 1Ui64 << (32 + U_LINE_SEPARATOR);
    constexpr uint64_t CatMask_Zp = 1Ui64 << (32 + U_PARAGRAPH_SEPARATOR);
    constexpr uint64_t CatMask_Zs = 1Ui64 << (32 + U_SPACE_SEPARATOR);

}


namespace boost {
    namespace BOOST_REGEX_DETAIL_NS {
        template<> inline bool is_combining<char32_t>(char32_t c) {
            return catMask(c) & (CatMask_Mc | CatMask_Me | CatMask_Mn);
        }
        template<> inline bool is_separator<char32_t>(char32_t c) {
            return c == 0x0A || c == 0x0D;
        }
        template<> inline char32_t global_lower<char32_t>(char32_t c) {
            return u_tolower(c);
        }
        template<> inline char32_t global_upper<char32_t>(char32_t c) {
            return u_toupper(c);
        }
    }
}


struct utf32_regex_traits {

    typedef char32_t                    char_type;
    typedef std::size_t                 size_type;
    typedef std::basic_string<char32_t> string_type;
    typedef std::wstring                locale_type;
    typedef uint64_t                    char_class_type;

    static constexpr char_class_type mask_upper    = 0x0001;  // upper case
    static constexpr char_class_type mask_lower    = 0x0002;  // lower case
    static constexpr char_class_type mask_digit    = 0x0004;  // decimal digits
    static constexpr char_class_type mask_punct    = 0x0008;  // punctuation characters
    static constexpr char_class_type mask_cntrl    = 0x0010;  // control characters
    static constexpr char_class_type mask_blank    = 0x0020;  // horizontal space
    static constexpr char_class_type mask_vertical = 0x0040;  // vertical space
    static constexpr char_class_type mask_xdigit   = 0x0080;  // hexadecimal digits
    static constexpr char_class_type mask_alpha    = 0x0100;  // any linguistic character
    static constexpr char_class_type mask_word     = 0x0200;  // word characters (alpha, number, marks and connectors)
    static constexpr char_class_type mask_graph    = 0x0400;  // any visible character
    static constexpr char_class_type mask_ascii    = 0x0800;  // code points < 128
    static constexpr char_class_type mask_unicode  = 0x1000;  // code points > 255

    static constexpr char_class_type mask_space = mask_blank | mask_vertical;
    static constexpr char_class_type mask_alnum = mask_alpha | mask_digit;
    static constexpr char_class_type mask_print = mask_graph | mask_space;

    static const char_class_type categoryMasks[];
    static const char_class_type asciiMasks[];
    static const std::map<std::string, char_class_type> classnames;
    static const std::map<std::string, char_type> character_names;
    static const std::set<string_type> digraphs;

    locale_type locale;

    std::unique_ptr<icu::Collator> collator_identical;
    std::unique_ptr<icu::Collator> collator_primary;

    utf32_regex_traits() {
        locale.resize(LOCALE_NAME_MAX_LENGTH);
        int n = GetUserDefaultLocaleName(locale.data(), LOCALE_NAME_MAX_LENGTH);
        locale.resize(n - 1);
        UErrorCode ec = U_ZERO_ERROR;
        collator_identical.reset(icu::Collator::createInstance(ec));
        collator_identical->setStrength(U_NAMESPACE_QUALIFIER Collator::IDENTICAL);
        ec = U_ZERO_ERROR;
        collator_primary.reset(icu::Collator::createInstance(ec));
        collator_primary->setStrength(U_NAMESPACE_QUALIFIER Collator::PRIMARY);
    }

    static size_type length(const char_type* p) { return std::char_traits<char_type>::length(p); }

    char_type translate(char_type c) const { return c; }

    char_type translate_nocase(char_type c) const { return u_foldCase(c, U_FOLD_CASE_DEFAULT); }

    string_type transform(const char_type* p1, const char_type* p2) const {
        if (p1 == p2) return string_type();
        icu::UnicodeString us = icu::UnicodeString::fromUTF32(reinterpret_cast<const UChar32*>(p1), static_cast<int32_t>(p2 - p1));
        std::int32_t n = collator_identical->getSortKey(us, 0, 0);
        std::basic_string<uint8_t> sk8(n, 0);
        collator_identical->getSortKey(us, sk8.data(), n);
        string_type st;
        for (auto& c : sk8) {
            if (!c) break;
            st += c;
        }
        return st;
    }

    string_type transform_primary(const char_type* p1, const char_type* p2) const {
        if (p1 == p2) return string_type();
        icu::UnicodeString us = icu::UnicodeString::fromUTF32(reinterpret_cast<const UChar32*>(p1), static_cast<int32_t>(p2 - p1));
        std::int32_t n = collator_primary->getSortKey(us, 0, 0);
        std::basic_string<uint8_t> sk8(n, 0);
        collator_primary->getSortKey(us, sk8.data(), n);
        string_type st;
        for (auto& c : sk8) {
            if (!c) break;
            st += c;
        }
        return st;
    }

    char_class_type lookup_classname(const char_type* p1, const char_type* p2) const {
        std::string name;
        for (const char_type* p = p1; p < p2; ++p) {
            if (*p > 127) return 0;
            name += static_cast<char>(std::tolower(static_cast<char>(*p)));
        }
        if (classnames.contains(name)) return classnames.at(name);
        return 0;
    }

    string_type lookup_collatename(const char_type* p1, const char_type* p2) const {
        if (p2 - p1 < 2) return string_type(p1, p2);
        std::string s;
        for (const char_type* p = p1; p < p2; ++p) {
            if (*p > 127) return string_type();
            s += static_cast<char>(std::tolower(static_cast<char>(*p)));
        }
        if (character_names.contains(s)) return string_type(1, character_names.at(s));
        if (p2 - p1 != 2) return string_type();
        string_type digraph(p1, p2);
        return digraphs.contains(digraph) ? digraph : string_type();
    }

    bool isctype(char_type c, char_class_type class_mask) const {
        if (c < 128) return class_mask & asciiMasks[c];
        if ((class_mask & mask_unicode) && c > 256) return true;
        char_class_type genmask = catMask(c);
        if (class_mask & genmask) return true;
        if ((class_mask & mask_blank              ) && (genmask == CatMask_Zs)) return true;
        if ((class_mask & mask_cntrl              ) && (genmask == CatMask_Cc)) return true;
        if ((class_mask & mask_graph              ) && u_hasBinaryProperty(c, UCHAR_POSIX_GRAPH)) return true;
        if ((class_mask & mask_vertical           ) && (c == 0x85 || c == 0x2028 || c == 0x2029)) return true;
        if ((class_mask & (mask_alpha | mask_word)) && u_isUAlphabetic(c)) return true;
        if ((class_mask & (mask_digit | mask_word)) && u_isdigit      (c)) return true;
        if ((class_mask & mask_lower              ) && u_isULowercase (c)) return true;
        if ((class_mask & mask_punct              ) && u_ispunct      (c)) return true;
        if ((class_mask & mask_upper              ) && u_isUUppercase (c)) return true;
        if ((class_mask & mask_xdigit             ) && u_isxdigit     (c)) return true;
        if ((class_mask & mask_word)) {
            if (genmask & (CatMask_Mc | CatMask_Me | CatMask_Mn | CatMask_Pc)) return true;
            if (u_hasBinaryProperty(c, UCHAR_JOIN_CONTROL)) return true;
        }
        return false;
    }

    int value(char_type c, int radix) const {
        int n = c <  '0' ? -1
              : c <= '9' ? c - '0'
              : c <  'A' ? -1
              : c <= 'F' ? c - ('A' - 10)
              : c <  'a' ? -1
              : c <= 'f' ? c - ('a' - 10)
              : -1;
        return n < radix ? n : -1;
    }

    locale_type imbue(locale_type l) {
        std::wstring last = locale;
        locale = l;
        return last;
    }

    locale_type getloc() const {
        return locale;
    }

};

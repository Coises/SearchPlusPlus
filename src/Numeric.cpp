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
 
// Note: These routines were copied with minimal changes from Columns++.
// That plugin supports some formats not used in Search++, so not all settings and code paths are relevant here.

#include <charconv>
#include <cmath>
#include "Numeric.h"
#include "Framework\UtilityFrameworkMIT.h"


static constexpr double scientificNotationThreshold = static_cast<long long>(1) << std::numeric_limits<double>::digits;

static constexpr double factors[4][4] = { 1.                  , 24.          , 60. * 24., 60. * 60. * 24,
                                          1. / 24.            , 1            , 60.      , 60. * 60.     ,
                                          1. / 24. / 60.      , 1. / 60.     , 1.       , 60.           ,
                                          1. / 24. / 60. / 60., 1 / 60. / 60., 1. / 60. , 1.            };


std::string NumericFormat::format(double value) const {

    if (!std::isfinite(value)) return "";

    int segments[4];  // unit (0=days, 1=hours, 2=minutes, 3=seconds) of rightmost segment in formats with 0 - 3 colons; -1 if format not enabled
    int minSeg;       // leftmost unit (0=days, 1=hours, 2=minutes, 3=seconds) in any enabled format
    int maxSeg;       // rightmost unit (0=days, 1=hours, 2=minutes, 3=seconds) in any enabled format

    if (timeEnable & 14) /* one or more multi-segment formats enabled */ {
        segments[0] = !(timeEnable & 1) ? -1 : timeScalarUnit;
        segments[1] = !(timeEnable & 2) ? -1 : timePartialRule == 0 ? 1 : timePartialRule == 3 ? 3 : 2;
        segments[2] = !(timeEnable & 4) ? -1 : timePartialRule < 2 ? 2 : 3;
        segments[3] = !(timeEnable & 8) ? -1 : 3;
        maxSeg = std::max(std::max(segments[0], segments[1]), std::max(segments[2], segments[3]));
        minSeg = std::min(std::min(segments[0] < 0 ? 3 : segments[0], segments[1] < 0 ? 3 : segments[1] - 1),
                          std::min(segments[2] < 0 ? 3 : segments[2] - 2, segments[3] < 0 ? 3 : 0));
    }

    else /* scalar */ {
        segments[0] = minSeg = maxSeg = timeScalarUnit;
        segments[1] = segments[2] = segments[3] = -1;
    }

    if (timeScalarUnit != maxSeg) value *= factors[timeScalarUnit][maxSeg];
    value = std::round(value * pow(10, maxDec)) / pow(10, maxDec);

    std::string s(31 + maxDec, 0);

    if (std::abs(value) > scientificNotationThreshold) {
        std::to_chars_result tcr = std::to_chars(s.data(), s.data() + s.length(), value, std::chars_format::scientific);
        if (tcr.ec != std::errc()) return "";
        s.resize(tcr.ptr - s.data());
        size_t d = s.find('.'); 
        if (d != std::string::npos) {
            if (decimal != '.') s[d] = decimal;
            if (thousands) {
                size_t e = s.find_first_of("eE");
                if (e != std::string::npos && e > d + 4) 
                    for (size_t q = e - (e - d - 2) % 3 - 1; q > d + 1; q -= 3) s = s.substr(0, q) + thousands + s.substr(q);
            }
        }
        return s;
    }

    int best;               // best (or only) enabled format
    long long segFinal[4];  // value of segment if it is the first segment rendered
    long long segValue[4];  // value of segment when other segments are rendered to the left
    
    if (minSeg == maxSeg) /* scalar */ best = 0;
    else /* one or more multi-segment formats enabled */ {
        double whole = std::trunc(std::abs(value));
        int needRight = whole == std::abs(value) ? minSeg : maxSeg;
        int wantLeft = maxSeg;
        segFinal[maxSeg] = segValue[maxSeg] = static_cast<long long>(whole);
        for (int i = maxSeg; i > minSeg; --i) {
            segFinal[i - 1] = segValue[i - 1] = segValue[i] / (i == 1 ? 24 : 60);
            segValue[i] = segValue[i] % (i == 1 ? 24 : 60);
            if (segValue[i] != 0 && needRight < i) needRight = i;
            if (segValue[i - 1] != 0) wantLeft = i - 1;
        }
        best = -1;
        for (int i = 0; i < 4; ++i) {
            if (segments[i] < needRight) continue;
            if (best < 0 || segments[best] - best > wantLeft && segments[i] - i < segments[best] - best) best = i;
        }
    }
    
    if (best == 0 && segments[best] == maxSeg) /* single segment which is also the smallest division enabled */ {
        std::to_chars_result tcr = std::to_chars(s.data(), s.data() + s.length(), value, std::chars_format::fixed, maxDec);
        if (tcr.ec != std::errc()) return "";
        s.resize(tcr.ptr - s.data());
    }

    else /* either multi-segment, or single segment which is not the smallest division enabled */ {
        char* p = s.data();
        char* q = p + s.length();
        if (value < 0) *p++ = '-';
        int fromSeg = segments[best] - best;
        std::to_chars_result tcr = std::to_chars(p, q, segFinal[segments[best] - best]);
        if (tcr.ec != std::errc()) return "";
        p = tcr.ptr;
        for (int i = fromSeg + 1; i <= segments[best]; ++i) {
            *p++ = ':';
            if (segValue[i] < 10) *p++ = '0';
            tcr = std::to_chars(p, q, segValue[i]);
            if (tcr.ec != std::errc()) return "";
            p = tcr.ptr;
        }
        if (segments[best] == maxSeg) {
            char onesDigit = *--p;
            tcr = std::to_chars(p, q, std::abs(value) - std::trunc(std::abs(value)), std::chars_format::fixed, maxDec);
            if (tcr.ec != std::errc()) return "";
            *p = onesDigit;
            p = tcr.ptr;
        }
        s.resize(p - s.data());
    }
    
    if (leftPad > 1) {
        size_t left = s.find_first_of(".:");
        if (left == std::string::npos) left = s.length();
        if (s[0] == '-') --left;
        if (static_cast<int>(left) < leftPad)
            s = s[0] == '-' ? '-' + std::string(leftPad - left, '0') + s.substr(1) : std::string(leftPad - left, '0') + s;
    }

    if (maxDec && segments[best] == maxSeg) {
        if (minDec < maxDec) {
            size_t dMin = minDec < 0 ? 0 : static_cast<size_t>(minDec);
            size_t dMax = static_cast<size_t>(maxDec);
            s = s.substr(0, std::max(s.find_last_not_of('0') + 1, s.length() - (dMax - dMin)));
            if (minDec < 0 && !std::isdigit(s.back())) s.pop_back();
        }
    }
    else if (minDec == 0) s += '.';

    if (decimal != '.') {
        size_t d = s.find('.');
        if (d != std::string::npos) s[d] = decimal;
    }

    if (thousands) {
        bool negative = false;
        if (s[0] == '-') {
            s = s.substr(1);
            negative = true;
        }
        size_t j = s.find_first_not_of("0123456789");
        if (j == std::string::npos) j = s.length();
        else {
            size_t k = s.find_last_not_of("0123456789");
            if (s[k] == decimal) {
                size_t p = s.length() - k - 1;
                if (p > 3) {
                    for (size_t q = s.length() - (p - 1) % 3 - 1; q > k + 1; q -= 3)
                        s = s.substr(0, q) + thousands + s.substr(q);
                }
            }
        }
        if (j > 3) for (ptrdiff_t q = j - 3; q > 0; q -= 3)
            s = s.substr(0, q) + thousands + s.substr(q);
        if (negative) s = '-' + s;
    }

    return s;

}


double parseNumber(const std::wstring& s, wchar_t decimalSeparator) {

    static const std::wstring currency =
        L"$\u00A2\u00A3\u00A4\u00A5\u058F\u060B\u07FE\u07FF\u09F2\u09F3\u09FB\u0AF1\u0BF9\u0E3F\u17DB"
        L"\u20A0\u20A1\u20A2\u20A3\u20A4\u20A5\u20A6\u20A7\u20A8\u20A9\u20AA\u20AB\u20AC\u20AD\u20AE\u20AF"
        L"\u20B0\u20B1\u20B2\u20B3\u20B4\u20B5\u20B6\u20B7\u20B8\u20B9\u20BA\u20BB\u20BC\u20BD\u20BE\u20BF"
        L"\uA838\uFDFC\uFE69\uFF04\uFFE0\uFFE1\uFFE5\uFFE6";
    static const std::wstring plus   = L"+\uFF0B";
    static const std::wstring minus  = L"-\u2012\u2013\u2212\uFE63\uFF0D";
    static const std::wstring spaces = L" \u00A0\u2000\u2001\u2002\u2003\u2004\u2005\u2006\u2007\u2008\u2009\u200A\u202F\u205F";
    static const std::wstring inside = L".,:'0123456789" + spaces;
    
    constexpr double nan = std::numeric_limits<double>::quiet_NaN();
    constexpr int timeScalarUnit  = 3; // time segment as which to interpert a scalar (no colons): 0 = days, 1 = hours, 2 = minutes, 3 = seconds
    constexpr int timePartialRule = 3; // interpretation of 2 and 3 segment times: 0 = d:h, d:h:m; 1 = h:m, d:h:m; 2 = h:m, h:m:s; 3 = m:s, h:m:s

    const wchar_t decimal = decimalSeparator;

    size_t left = s.find_first_of(L"0123456789");
    if (left == std::string::npos) return nan;
    size_t right = s.find_last_of(L"0123456789");

    size_t decimalPosition = (left > 0 && s[left - 1] == decimal) ? --left : s.find_first_of(decimal, left);
    if (decimalPosition == right + 1) ++right;
    else if (decimalPosition > right) decimalPosition = right + 1;
    else if (decimalPosition != s.find_last_of(decimal, right)) return nan;

    size_t colonPosition = s.find_last_of(L':', right);
    if (colonPosition < left) colonPosition = std::wstring::npos;
    if (colonPosition != std::wstring::npos) {
        if (colonPosition > decimalPosition) return nan;
        if (std::count(std::next(s.begin(), left), std::next(s.begin(), right), ':') > 3) return nan;
    }

    const std::wstring_view prefix = std::wstring_view(s).substr(0, left);
    const std::wstring_view number = std::wstring_view(s).substr(left, right - left + 1);
    const std::wstring_view suffix = std::wstring_view(s).substr(right + 1);

    if (number.find_first_not_of(inside) != std::wstring::npos) return nan;

    // prefix can be:
    // - empty or all space characters
    // - a sign and/or a currency symbol, with optional space characters before, after or between them
    // - any characters, followed by a space, followed by a sign and/or a currency symbol
    // - any mix of characters ending in a space

    // suffix can be:
    // - empty or all space characters
    // - a sign and/or a currency symbol, with optional space characters before, after or between them
    // - a sign and/or a currency symbol, followed by a space, followed by any characters
    // - any mix of characters that does not begin with a sign or with a currency symbol followed by a sign

    // a sign cannot appear in both the prefix and the suffix

    bool negative = false;
    bool foundSign = false;

    if (prefix.find_first_not_of(spaces) != std::wstring::npos) /* not empty or all space characters */ {
        const size_t lastArbitrary = prefix.find_last_not_of(spaces + currency + plus + minus);
        const size_t signAt = prefix.find_first_of(plus + minus);
        const size_t cncyAt = prefix.find_first_of(currency);
        if (lastArbitrary == std::wstring::npos
            && (signAt == std::wstring::npos || signAt == prefix.find_last_of(plus + minus))
            && (cncyAt == std::wstring::npos || cncyAt == prefix.find_last_of(currency))) {
            // at most one sign, at most one currency symbol, and the rest space characters
            if (signAt != std::wstring::npos) {
                foundSign = true;
                negative = minus.find_first_of(prefix[signAt]) != std::wstring::npos;
            }
        }
        else {
            const size_t lastSpace = prefix.find_last_of(spaces);
            if (lastSpace == std::wstring::npos) return nan;
            switch (prefix.length() - lastSpace) {
            case 1 /* space is last, OK */:
                break;
            case 2 /* one character, OK if sign or currency */:
                if (currency.find_first_of(prefix[lastSpace + 1]) != std::wstring::npos) break;
                foundSign = true;
                if (plus.find_first_of(prefix[lastSpace + 1]) != std::wstring::npos) break;
                negative = true;
                if (minus.find_first_of(prefix[lastSpace + 1]) != std::wstring::npos) break;
                return nan;
            case 3 /* two characters, OK if one is sign and one is currency */:
                if (plus.find_first_of(prefix[lastSpace + 2]) != std::wstring::npos)   foundSign = true;
                else if (minus.find_first_of(prefix[lastSpace + 2]) != std::wstring::npos) { foundSign = true; negative = true; }
                else if (currency.find_first_of(prefix[lastSpace + 2]) == std::wstring::npos) return nan;
                if (currency.find_first_of(prefix[lastSpace + 1]) != std::wstring::npos)
                    if (foundSign) break;
                    else return nan;
                foundSign = true;
                if (plus.find_first_of(prefix[lastSpace + 1]) != std::wstring::npos) break;
                negative = true;
                if (minus.find_first_of(prefix[lastSpace + 1]) != std::wstring::npos) break;
                return nan;
            default:
                return nan;
            }
        }
    }

    if (suffix.find_first_not_of(spaces) != std::wstring::npos) /* not empty or all space characters */ {
        const size_t firstArbitrary = suffix.find_first_not_of(spaces + currency + plus + minus);
        const size_t signAt = suffix.find_first_of(plus + minus);
        const size_t cncyAt = suffix.find_first_of(currency);
        if (firstArbitrary == std::wstring::npos
            && (signAt == std::wstring::npos || signAt == suffix.find_last_of(plus + minus))
            && (cncyAt == std::wstring::npos || cncyAt == suffix.find_last_of(currency))) {
            // at most one sign, at most one currency symbol, and the rest space characters
            if (signAt != std::wstring::npos) {
                if (foundSign) return nan;
                foundSign = true;
                if (minus.find_first_of(suffix[signAt]) != std::wstring::npos) negative = true;
            }
        }
        else if (firstArbitrary != 0) {
            // There will always be at least two characters here; any single character would have been handled already
            size_t firstSpace = suffix.find_first_of(spaces);
            if (firstSpace != 0) {
                if (cncyAt == 0) /* first character in suffix is a currency symbol -- only matters if followed by a sign */ {
                    bool isMinus = minus.find_first_of(suffix[1]);
                    bool isPlus = !isMinus && plus.find_first_of(suffix[1]);
                    if (isPlus || isMinus) {
                        if (foundSign || firstSpace != 2) return nan;
                        foundSign = true;
                        if (isMinus) negative = true;
                    }
                }
                else /* first character in suffix is a sign */ {
                    if (foundSign || !(firstSpace == 1 || (cncyAt == 1 && firstSpace == 2))) return nan;
                    foundSign = true;
                    negative = minus.find_first_of(suffix[0]) != std::wstring::npos;
                }
            }
        }
    }

    std::string n[4];
    int nLevel = 0;
    bool separatorOK = true;
    for (size_t i = 0; i < number.length(); ++i) {
        if (number[i] >= L'0' && number[i] <= L'9') {
            n[nLevel] += static_cast<char>(number[i]);
            separatorOK = true;
            continue;
        }
        else if (!separatorOK) return nan;
        separatorOK = false;
        if (number[i] == decimal) n[nLevel] += '.';
        else if (number[i] == L':') ++nLevel;
    }

    double v = std::stod(n[nLevel]);

    switch (nLevel) {
    case 1:
        v += (timePartialRule == 0 ? 24 : 60) * std::stod(n[0]);
        v *= factors[timePartialRule == 0 ? 1 : timePartialRule == 3 ? 3 : 2][timeScalarUnit];
        break;
    case 2:
        v += (timePartialRule < 2 ? 24 * 60 : 60 * 60) * std::stod(n[0]) + 60 * std::stod(n[1]);
        v *= factors[timePartialRule < 2 ? 2 : 3][timeScalarUnit];
        break;
    case 3:
        v += 24 * 60 * 60 * std::stod(n[0]) + 60 * 60 * std::stod(n[1]) + 60 * std::stod(n[2]);
        v *= factors[3][timeScalarUnit];
    }

    return negative ? -v : v;

}

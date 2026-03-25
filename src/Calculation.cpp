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

#include <regex>
#include "Calculation.h"
#include "Numeric.h"

#pragma warning (push)
#pragma warning (disable: 4702)
#pragma warning (disable: 4459)
#define exprtk_disable_rtl_io
#define exprtk_disable_rtl_io_file
#define exprtk_disable_rtl_vecops
#include "exprtk/exprtk.hpp"
#pragma warning (pop)


class Calculation_Context {

public:

    struct RegComma : public exprtk::ifunction <double> {
        RegularExpression** rx = 0;
        RegComma(RegularExpression** rx) : exprtk::ifunction<double>(1), rx(rx) {}
        double operator()(const double& capture) {
            return parseNumber((*rx)->wstr(static_cast<int>(capture)), L',');
        }
    };

    struct RegPeriod : public exprtk::ifunction <double> {
        RegularExpression** rx = 0;
        RegPeriod(RegularExpression** rx) : exprtk::ifunction<double>(1), rx(rx) {}
        double operator()(const double& capture) {
            return parseNumber((*rx)->wstr(static_cast<int>(capture)), L'.');
        }
    };

    class Formula {
    public:
        exprtk::expression<double> expression;
        NumericFormat nf;
        std::string format() { return nf.format(expression.value()); }

    };

    RegularExpression* rx;

    exprtk::symbol_table<double> symbol_table;
    exprtk::parser<double>       parser;
    std::vector<Formula>         formula;
    std::vector<std::string>     replacement;
    double rcMatch = 0;
    double rcLine = 0;
    RegComma  regc;
    RegPeriod regp;

    Calculation_Context() : regc(&rx), regp(&rx) {
        symbol_table.add_variable("match", rcMatch);
        symbol_table.add_variable("line" , rcLine);
        symbol_table.add_function("regc" , regc);
        symbol_table.add_function("reg"  , regp);
    }

    void clear() {
        formula.clear();
        replacement.clear();
        rcMatch = 0;
    }

    std::string compile(const std::string& text);

};


std::string Calculation_Context::compile(const std::string& text) {
    static const std::regex
        rxFormat("\\s*(\\d{1,2}[t]?|t)?(?:(\\.|,|,\\.|\\.,)(?:((\\d{1,2})?-)?(\\d{1,2}))?)?\\s*:(.*)", std::regex::optimize);
    auto& calc = formula.emplace_back();
    calc.expression.register_symbol_table(symbol_table);
    std::string s = text;
    std::smatch m;
    if (std::regex_match(s, m, rxFormat)) {
        if (m[1].matched) {
            if (std::isdigit(m[1].str()[0])) calc.nf.leftPad = std::stoi(m[1]);
            if (!(std::isdigit(m[1].str().back()))) calc.nf.timeEnable = 6;
            calc.nf.minDec = -1;
            calc.nf.maxDec = 0;
        }
        if (m[2].matched) {
            std::string td = m[2].str();
            if (td.length() == 2)  calc.nf.thousands = td.front();
            calc.nf.decimal = td.back();
            calc.nf.maxDec = 6;
            if (m[5].matched) {
                calc.nf.minDec = calc.nf.maxDec = std::stoi(m[5]);
                if (m[3].matched) calc.nf.minDec = m[4].matched ? std::stoi(m[4]) : -1;
            }
        }
        s = m[6];
    }
    if (!parser.compile(s, calc.expression)) {
        auto error = parser.get_error(0);
        return "Formula \"" + text + "\": " + error.diagnostic;
    }
    return "";
}


std::string Calculation::parse(const std::string& replaceText) {
    context->clear();
    std::string segment;
    int  depth = 0;
    bool insideQuotes = false;
    for (size_t i = 0; i < replaceText.length(); ++i) {
        if (insideQuotes) {
            if (replaceText[i] == L'\'') insideQuotes = false;
            segment += replaceText[i];
        }
        else if (depth) {
            switch (replaceText[i]) {
            case '\'':
                insideQuotes = true;
                break;
            case '(':
                ++depth;
                break;
            case ')':
                if (!--depth) {
                    std::string errmsg = context->compile(segment);
                    if (!errmsg.empty()) return errmsg;
                    segment.clear();
                }
                break;
            case '$':
            {
                std::string function = "reg";
                if (i + 1 < replaceText.length() && replaceText[i + 1] == '$') {
                    function = "regc";
                    ++i;
                }
                size_t stop = replaceText.find_first_not_of("0123456789", ++i);
                if (stop == std::string::npos) stop = replaceText.length();
                if (stop == i) {
                    if (i < replaceText.length() && replaceText[i] == '(') segment += function;
                    else segment += function + "(0)";
                }
                else segment += function + '(' + replaceText.substr(i, stop - i) + ')';
                i = stop - 1;
                continue;
            }
            default:;
            }
            segment += replaceText[i];
        }
        else {
            segment += replaceText[i];
            switch (replaceText[i]) {
            case '(':
                if (i + 2 < replaceText.length() && replaceText.substr(i, 3) == "(?=") {
                    i += 2;
                    depth = 1;
                    context->replacement.emplace_back(segment);
                    segment.clear();
                }
                break;
            case '\\':
                if (i < replaceText.length() - 1) segment += replaceText[++i];
                break;
            }
        }
    }
    context->replacement.emplace_back(segment);
    return "";
}


std::string Calculation::format(RegularExpression& regularExpression, Scintilla::ScintillaCall& sciCall) {
    if (context->replacement.size() != context->formula.size() + 1)
        return ">>> AN UNKNOWN ERROR OCCURRED PROCESSING THIS REPLACEMENT <<<";
    context->rx = &regularExpression;
    ++context->rcMatch;
    context->rcLine = static_cast<double>(sciCall.LineFromPosition(context->rx->position(0)) + 1);
    std::string regexReplace;
    for (size_t i = 0; i < context->formula.size(); ++i) {
        regexReplace += context->replacement[i];
        regexReplace += context->formula[i].format();
    }
    regexReplace += context->replacement.back();
    return context->rx->format(regexReplace);
}

Calculation::Calculation() {
    context = std::make_unique<Calculation_Context>();
}

Calculation::~Calculation() {}  // Must be defined where Calculation_Context is fully defined so std::unique_ptr can delete.


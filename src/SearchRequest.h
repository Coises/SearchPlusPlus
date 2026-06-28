// This file is part of Search++.
// Copyright 2026 by Randy Fellmy <https://www.coises.com/>.

// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// at your option any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.

#pragma once

#include "Search.h"
#include "ScintillaControl.h"


struct SearchRequest {

    SearchCommand command;

    SearchContext* context = 0;

    HWND sciText = 0;

    std::string find;
    std::string repl;

    std::vector<Scintilla::CharacterRangeFull> ranges;

    SearchRequest() {}

    SearchResult exec(SearchCommand cmd, SearchContext& context, std::string_view find, std::string_view repl, HWND sciText);

    SearchResult error(const std::wstring& message, const std::string& bubble = "")
    {
        context->clear(); return SearchResult(message, bubble);
    }
    SearchResult found(const std::wstring& message, const std::string& bubble = "")
    {
        context->setFound(); return SearchResult(SearchResult::Success, message, bubble);
    }
    SearchResult notFound(const std::wstring& message, const std::string& bubble = "")
    {
        context->setNotFound(); return SearchResult(SearchResult::Failure, message, bubble);
    }
    SearchResult replaced(const std::wstring& message, const std::string& bubble = "")
    {
        context->setReplaced(); return SearchResult(SearchResult::Success, message, bubble);
    }
    SearchResult endRepl(const std::wstring& message, const std::string& bubble = "")
    {
        context->setNotFound(); return SearchResult(SearchResult::Success, message, bubble);
    }

    void scrollIntoView(Scintilla::Position foundStart, Scintilla::Position foundEnd, bool select = true);

    SearchResult found(Scintilla::Position cpMin, Scintilla::Position cpMax, const std::wstring& message) {
        scrollIntoView(cpMin, cpMax, true);
        sci.CallTipCancel();
        if (cpMin == cpMax) {
            char c;
            if (zlmIndicator == 0 || cpMin == sci.Length()
                || (!sci.ViewEOL() && ((c = sci.CharacterAt(cpMin)) == '\r' || c == '\n'))) {
                sci.CallTipShow(cpMin, "^ zero length match");
            }
            else {
                sci.IndicSetStyle(zlmIndicator, Scintilla::IndicatorStyle::Point);
                sci.IndicSetFore(zlmIndicator, sci.ElementColour(Scintilla::Element::Caret));
                sci.SetIndicatorCurrent(zlmIndicator);
                sci.SetIndicatorValue(1);
                sci.IndicatorClearRange(0, sci.Length());
                sci.IndicatorFillRange(cpMin, 1);
            }
        }
        context->setFound();
        return found(message);
    }

    SearchResult replaced(Scintilla::Position cpMin, Scintilla::Position cpMax, const std::wstring& message) {
        scrollIntoView(cpMin, cpMax, true);
        return replaced(message);
    }

};

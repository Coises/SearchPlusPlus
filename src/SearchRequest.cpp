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

#include "CommonData.h"
#include <algorithm>

SearchResult searchPlain(SearchRequest& req);
SearchResult searchBoost(SearchRequest& req);
SearchResult searchICU(SearchRequest& req);


SearchResult SearchRequest::exec(SearchCommand cmd) {

    command = cmd;
    ranges.clear();
    
    plugin.bypassNotifications = true;
    plugin.getScintillaPointers(sciText);
    Scintilla::Position documentLength = sci.Length();
    sci.CallTipCancel();
    if (zlmIndicator != 0) {
        sci.SetIndicatorCurrent(zlmIndicator);
        sci.IndicatorClearRange(0, sci.Length());
    }

    if (command.scope == SearchCommand::Smart || command.scope == SearchCommand::Region
        || command.scope == SearchCommand::Selection || command.scope == SearchCommand::Whole) {

        if (documentLength == 0) {
            plugin.bypassNotifications = false;
            return SearchResult(L"Empty document; nothing to search.");
        }

        if (command.scope == SearchCommand::Smart) {
            if (data.autoSearchMarked) {
                if (sci.IndicatorValueAt(data.indicator, 0)) command.scope = SearchCommand::Region;
                else {
                    Scintilla::Position p = sci.IndicatorEnd(data.indicator, 0);
                    if (p != 0 && p != documentLength) command.scope = SearchCommand::Region;
                }
            }
            if (command.scope == SearchCommand::Smart && context->none()
                && command.direction != SearchCommand::Before && command.direction != SearchCommand::After) {
                if (data.autoSearchSelect && !sci.SelectionEmpty()) {
                    if (!data.autoSearchSelectLimit || sci.Selections() > 1) command.scope = SearchCommand::Selection;
                    else {
                        Scintilla::Position start = sci.SelectionStart();
                        Scintilla::Position end = sci.SelectionEnd();
                        Scintilla::Position startLine = sci.LineFromPosition(start);
                        Scintilla::Position endLine = sci.LineFromPosition(end);
                        if (endLine - startLine + 1 >= data.selLines && end - start >= data.selChars) {
                            if (end - start >= 4 * data.selChars ||
                                sci.CountCharacters(start, end) >= data.selChars) command.scope = SearchCommand::Selection;
                        }
                    }
                    if (data.selectionToMarks && command.scope == SearchCommand::Selection && command.direction != SearchCommand::All) {
                        sci.SetIndicatorCurrent(data.indicator);
                        sci.SetIndicatorValue(1);
                        int n = sci.Selections();
                        Scintilla::Position smallest = documentLength;
                        for (int i = 0; i < n; ++i) {
                            Scintilla::Position a = sci.SelectionNStart(i);
                            Scintilla::Position b = sci.SelectionNEnd(i);
                            if (b > a) sci.IndicatorFillRange(a, b - a);
                            if (a < smallest) smallest = a;
                        }
                        sci.SetAnchor(smallest);
                        sci.SetCurrentPos(smallest);
                        command.scope = SearchCommand::Region;
                    }
                }
            }
        }

        if (command.scope == SearchCommand::Region) {
            for (Scintilla::Position cpMin = 0;;) {
                Scintilla::Position cpMax = sci.IndicatorEnd(data.indicator, cpMin);
                if (cpMax == cpMin) break;
                if (sci.IndicatorValueAt(data.indicator, cpMin)) ranges.push_back(Scintilla::CharacterRangeFull{ cpMin, cpMax });
                if (cpMax == documentLength) break;
                cpMin = cpMax;
            }
        }
        
        if (context->none() && (command.scope == SearchCommand::Selection)) {
            if (!sci.SelectionEmpty()) {
                int n = sci.Selections();
                if (n == 1) {
                    Scintilla::Position cpMin = sci.SelectionStart();
                    Scintilla::Position cpMax = sci.SelectionEnd();
                    ranges.push_back(Scintilla::CharacterRangeFull{cpMin, cpMax});
                }
                else {
                    for (int i = 0; i < n; ++i) {
                        Scintilla::Position cpMin = sci.SelectionNStart(i);
                        Scintilla::Position cpMax = sci.SelectionNEnd(i);
                        if (cpMin != cpMax) ranges.push_back(Scintilla::CharacterRangeFull{cpMin, cpMax});
                    }
                    std::sort(ranges.begin(), ranges.end(),
                        [](const Scintilla::CharacterRangeFull& a, const Scintilla::CharacterRangeFull& b) { return a.cpMin < b.cpMin; });
                }
            }
        }
        
        if (ranges.empty()) {
            ranges.push_back(Scintilla::CharacterRangeFull{ 0, documentLength });
            command.scope = SearchCommand::Whole;
        }

    }

    if (command.verb == SearchCommand::Select) {
        if (data.clearSelections || command.scope == SearchCommand::Selection) sci.ClearSelections();
    }
    else if (command.verb == SearchCommand::Mark) {
        if (data.clearMarked || command.scope == SearchCommand::Region) sci.IndicatorClearRange(0, documentLength);
    }
    else if (command.verb == SearchCommand::Show) {
        if (data.hideBeforeShow) {
            sci.IndicatorClearRange(0, sci.Length());
            sci.HideLines(0, sci.LineCount() - 1);
        }
        else if (sci.AllLinesVisible()) sci.HideLines(0, sci.LineCount() - 1);
    }

    SearchResult result;
    switch (data.searchEngine) {
    case SearchEngine::Plain:
        result = searchPlain(*this);
        break;
    case SearchEngine::Boost:
        result = searchBoost(*this);
        break;
    case SearchEngine::ICU:
        result = searchICU(*this);
        break;
    default:
        result = SearchResult(L"Search engine not implemented.");
    }

    if (!result.error()) {
        plugin.getScintillaPointers(sciFind);
        Scintilla::Position length = sci.Length();
        if (length) data.historyFind += utf8to16(sci.GetText(length));
        if (command.verb == SearchCommand::Verb::Replace || command.verb == SearchCommand::Verb::FindRepl) {
            plugin.getScintillaPointers(sciRepl);
            length = sci.Length();
            if (length) data.historyRepl += utf8to16(sci.GetText(length));
        }
        if (result.success() && data.focusStepwise
            && (command.direction == SearchCommand::Forward || command.direction == SearchCommand::Backward)) 
            SetFocus(plugin.currentScintilla());
    }

    plugin.bypassNotifications = false;
    return result;

}

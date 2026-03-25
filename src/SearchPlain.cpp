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
#include "Framework/UnicodeFormatTranslation.h"


namespace {

struct ProgressInfoPlain : ProgressInfo {
    std::string find;
    std::string repl;
    Scintilla::Position offset = 0;
    ProgressInfoPlain(SearchRequest& req) : ProgressInfo(req) {}
};


SearchResult singleFind(SearchRequest& req, bool postReplace = false) {

    bool backward    = req.command.direction == SearchCommand::Backward;
    bool inSelection = !postReplace && req.command.scope == SearchCommand::Selection;
    bool wrap        = req.context->notFound();

    plugin.getScintillaPointers(req.sciFind);
    Scintilla::Position findLength = sci.Length();
    if (findLength < 1) return req.error(L"Nothing to find.");
    std::string find = sci.GetText(findLength);
    
    plugin.getScintillaPointers(req.sciText);
    if (sci.CodePage() != CP_UTF8) find = fromWide(utf8to16(find));
    Scintilla::FindOption searchFlags = Scintilla::FindOption::None;
    if (data.wholeWord) searchFlags |= Scintilla::FindOption::WholeWord;
    if (data.matchCase) searchFlags |= Scintilla::FindOption::MatchCase;
    sci.SetSearchFlags(searchFlags);

    Scintilla::Position documentLength = sci.Length();
    Scintilla::Position searchFrom;
    size_t rangeFrom, rangeTo;
    if (backward) {
        rangeFrom  = req.ranges.size() - 1;
        rangeTo    = 0;
        searchFrom = inSelection ? req.ranges.back().cpMax : std::min(sci.Anchor(), sci.CurrentPos());
        if (wrap) searchFrom = documentLength;
    }
    else {
        rangeFrom = 0;
        rangeTo   = req.ranges.size() - 1;
        searchFrom = inSelection ? req.ranges.front().cpMin : std::max(sci.Anchor(), sci.CurrentPos());
        if (wrap) searchFrom = 0;
    }

    for (size_t range = rangeFrom; range <= rangeTo; backward ? --range : ++range) {
        auto r = req.ranges[range];
        if (backward) {
            if (searchFrom <= r.cpMin) continue;
            sci.SetTargetRange(std::min(searchFrom, r.cpMax), r.cpMin);
        }
        else {
            if (searchFrom >= r.cpMax) continue;
            sci.SetTargetRange(std::max(searchFrom, r.cpMin), r.cpMax);

        }
        Scintilla::Position found = sci.SearchInTarget(find);
        if (found >= 0)
            return req.found(found, sci.TargetEnd(), !postReplace  ? L"Match found."
                                                   : backward      ? L"Match replaced; previous match found."
                                                                   : L"Match replaced; next match found.");
        if (found < -1)
            return req.error(L"A Scintilla search error occurred (SEARCHINTARGET returned " + std::to_wstring(found) + L").");
    }

    if (postReplace) return req.endRepl(L"Match replaced; no more matches found.");
    if (backward ? searchFrom >= req.ranges.back().cpMax : searchFrom <= req.ranges.front().cpMin) return req.notFound(
            req.command.scope == SearchCommand::Selection ? L"No matches found in selection."
          : req.command.scope == SearchCommand::Region    ? L"No matches found in marked text."
                                                          : L"No matches found in entire document.");
    else return req.notFound(
            req.command.scope == SearchCommand::Selection ? L"No matches found in selection."
          : req.command.scope == SearchCommand::Region    ? L"No matches found. (Find again to search all marked text.)"
                                                          : L"No matches found. (Find again to search entire document.)");

}


SearchResult singleReplace(SearchRequest& req) {

    if (!req.context->found()) return singleFind(req);
    
    req.context->clear();

    plugin.getScintillaPointers(req.sciRepl);
    Scintilla::Position replLength = sci.Length();
    std::string repl = replLength ? sci.GetText(replLength) : "";
    
    plugin.getScintillaPointers(req.sciText);
    if (sci.CodePage() != CP_UTF8) repl = fromWide(utf8to16(repl));

    sci.TargetFromSelection();
    sci.ReplaceTarget(repl);

    Scintilla::Position start = sci.TargetStart();
    Scintilla::Position end   = sci.TargetEnd();
    if (req.command.scope == SearchCommand::Scope::Region && start != end) sci.IndicatorFillRange(start, end - start);
    if (req.command.verb == SearchCommand::FindRepl) return req.replaced(start, end, L"Match replaced.");
    sci.SetSel(start, end);
    return singleFind(req, true);

}


bool progressiveSearch(ProgressInfo& pi) {

    auto& count    = pi.count;
    auto& position = pi.position;
    auto& rangeEnd = pi.rangeEnd;
    auto& req      = pi.req;

    ProgressInfoPlain& pip = static_cast<ProgressInfoPlain&>(pi);
    auto& find = pip.find;
    auto& offset = pip.offset;

    auto r = req.ranges[pi.rangeIndex];
    Scintilla::Position scanMax = std::min(rangeEnd, r.cpMax);
    if (position >= scanMax) return false;
    sci.SetTargetRange(position + offset, scanMax + offset);
    Scintilla::Position found = sci.SearchInTarget(find);

    if (found >= 0) {
        Scintilla::Position foundEnd = sci.TargetEnd();
        Scintilla::Position length = foundEnd - found;
        position = foundEnd - offset;
        ++count;
        if (req.command.verb != SearchCommand::Count)
            pip.hitSet->add(found, foundEnd);
        switch (req.command.verb) {
        case SearchCommand::Select:
            if (count == 1) scrollIntoView(position, found);
                       else sci.AddSelection(position, found);
            break;
        case SearchCommand::Show:
            sci.ShowLines(sci.LineFromPosition(found), sci.LineFromPosition(position));
            [[fallthrough]];
        case SearchCommand::Mark:
            sci.SetIndicatorCurrent(data.indicator);
            sci.SetIndicatorValue(1);
            sci.IndicatorFillRange(found, length);
            break;
        case SearchCommand::Replace:
            sci.ReplaceTarget(pip.repl);
            if (req.command.scope == SearchCommand::Scope::Region && !pip.repl.empty()) {
                sci.SetIndicatorCurrent(data.indicator);
                sci.SetIndicatorValue(1);
                sci.IndicatorFillRange(found, pip.repl.length());
            }
            pip.offset += pip.repl.length() - pip.find.length();
            break;
        default:;
        }
    }
    else if (found < -1) {
        pi.result = SearchResult(L"A Scintilla search error occurred (SEARCHINTARGET returned " + std::to_wstring(found) + L").");
        return false;
    }

    if (found == -1 || position >= r.cpMax) {
        if (++pi.rangeIndex >= req.ranges.size()) return false;
        position = req.ranges[pi.rangeIndex].cpMin;
    }

    return position < rangeEnd;

}


SearchResult multipleSearch(SearchRequest& req) {
    plugin.getScintillaPointers(req.sciFind);
    Scintilla::Position findLength = sci.Length();
    if (findLength < 1) return req.error(L"Nothing to find.");
    ProgressInfoPlain pip(req);
    pip.find = sci.GetText(findLength);
    if (req.command.verb == SearchCommand::Replace) {
        plugin.getScintillaPointers(req.sciRepl);
        Scintilla::Position replLength = sci.Length();
        pip.repl = replLength ? sci.GetText(replLength) : "";
    }
    plugin.getScintillaPointers(req.sciText);
    if (auto cp = sci.CodePage() != CP_UTF8) {
        pip.find = fromWide(utf8to16(pip.find), cp);
        if (req.command.verb == SearchCommand::Replace) pip.repl = fromWide(utf8to16(pip.repl), cp);
    }
    Scintilla::FindOption searchFlags = Scintilla::FindOption::None;
    if (data.wholeWord) searchFlags |= Scintilla::FindOption::WholeWord;
    if (data.matchCase) searchFlags |= Scintilla::FindOption::MatchCase;
    sci.SetSearchFlags(searchFlags);
    sci.SetIndicatorCurrent(data.indicator);
    sci.SetIndicatorValue(1);
    pip.hitSet = std::make_unique<ProgressInfo::HitSet>();
    pip.hitSet->searchString = "(Text): " + pip.find;
    pip.exec(progressiveSearch);
    return pip.result;
}

}


SearchResult searchPlain(SearchRequest& req) {
    switch (req.command.verb) {
    case SearchCommand::Find:
        switch (req.command.direction) {
        case SearchCommand::Forward:
        case SearchCommand::Backward:
            return singleFind(req);
        case SearchCommand::All:
        case SearchCommand::Before:
        case SearchCommand::After:
            return multipleSearch(req);
        default:
            return SearchResult(L"Command not implemented.");
        }
    case SearchCommand::Count:
    case SearchCommand::Mark:
    case SearchCommand::Select:
    case SearchCommand::Show:
        return multipleSearch(req);
    case SearchCommand::Replace:
    case SearchCommand::FindRepl:
        switch (req.command.direction) {
        case SearchCommand::Forward:
        case SearchCommand::Backward:
            return singleReplace(req);
        case SearchCommand::All:
        case SearchCommand::Before:
        case SearchCommand::After:
            return multipleSearch(req);
        default:
            return SearchResult(L"Command not implemented.");
        }
    default:
        return SearchResult(L"Command not implemented.");
    }
}
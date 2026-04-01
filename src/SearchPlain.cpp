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


struct ProgressOpenDocumentsPlain : ProgressInfoPlain {
    std::string originalFind;
    std::string originalRepl;
    Scintilla::FindOption searchFlags = Scintilla::FindOption::None;
    ProgressOpenDocumentsPlain(SearchRequest& req) : ProgressInfoPlain(req) {}
};


SearchResult singleFind(SearchRequest& req, bool postReplace = false) {

    bool backward    = req.command.extent == SearchCommand::Backward;
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
    Scintilla::Position searchFrom =
        backward ? (inSelection ? req.ranges.back() .cpMax : wrap ? documentLength : std::min(sci.Anchor(), sci.CurrentPos()))
                 : (inSelection ? req.ranges.front().cpMin : wrap ? 0              : std::max(sci.Anchor(), sci.CurrentPos()));

    for (size_t range = 0; range < req.ranges.size(); ++range) {
        auto r = req.ranges[backward ? req.ranges.size() - 1 - range : range];
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
    if (req.command.scope == SearchCommand::Scope::Region && start != end) {
        sci.SetIndicatorCurrent(data.indicator);
        sci.SetIndicatorValue(1);
        sci.IndicatorFillRange(start, end - start);
    }
    if (req.command.verb == SearchCommand::ReplStop) return req.replaced(start, end, L"Match replaced.");
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
        case SearchCommand::ReplaceAll:
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
    if (req.command.verb == SearchCommand::ReplaceAll) {
        plugin.getScintillaPointers(req.sciRepl);
        Scintilla::Position replLength = sci.Length();
        pip.repl = replLength ? sci.GetText(replLength) : "";
    }
    plugin.getScintillaPointers(req.sciText);
    Scintilla::FindOption searchFlags = Scintilla::FindOption::None;
    if (data.wholeWord) searchFlags |= Scintilla::FindOption::WholeWord;
    if (data.matchCase) searchFlags |= Scintilla::FindOption::MatchCase;
    sci.SetSearchFlags(searchFlags);
    sci.SetIndicatorCurrent(data.indicator);
    sci.SetIndicatorValue(1);
    pip.hitSet = std::make_unique<ProgressInfo::HitSet>();
    pip.hitSet->searchString = "(Text): " + pip.find;
    if (auto cp = sci.CodePage() != CP_UTF8) {
        pip.find = fromWide(utf8to16(pip.find), cp);
        if (req.command.verb == SearchCommand::ReplaceAll) pip.repl = fromWide(utf8to16(pip.repl), cp);
    }
    pip.exec(progressiveSearch);
    return pip.result;
}


void openDocumentsPrepare(ProgressInfo& pi) {
    ProgressOpenDocumentsPlain& pip = static_cast<ProgressOpenDocumentsPlain&>(pi);
    pip.offset = 0;
    if (auto cp = sci.CodePage() != CP_UTF8) {
        pip.find = fromWide(utf8to16(pip.originalFind), cp);
        pip.repl = fromWide(utf8to16(pip.originalRepl), cp);
    }
    else {
        pip.find = pip.originalFind;
        pip.repl = pip.originalRepl;
    }
    sci.SetSearchFlags(pip.searchFlags);
}


SearchResult openDocumentsSearch(SearchRequest& req) {
    plugin.getScintillaPointers(req.sciFind);
    Scintilla::Position findLength = sci.Length();
    if (findLength < 1) return req.error(L"Nothing to find.");
    ProgressOpenDocumentsPlain pip(req);
    pip.originalFind = sci.GetText(findLength);
    if (req.command.verb == SearchCommand::ReplaceAll) {
        plugin.getScintillaPointers(req.sciRepl);
        Scintilla::Position replLength = sci.Length();
        pip.originalRepl = replLength ? sci.GetText(replLength) : "";
    }
    if (data.wholeWord) pip.searchFlags |= Scintilla::FindOption::WholeWord;
    if (data.matchCase) pip.searchFlags |= Scintilla::FindOption::MatchCase;
    pip.hitSet = std::make_unique<ProgressInfo::HitSet>();
    pip.hitSet->searchString = "(Text): " + pip.originalFind;
    pip.openDocuments(progressiveSearch, openDocumentsPrepare);
    return pip.result;
}

}


SearchResult searchPlain(SearchRequest& req) {
    switch (req.command.verb) {
    case SearchCommand::Find:
        switch (req.command.extent) {
        case SearchCommand::Forward:
        case SearchCommand::Backward:
            return singleFind(req);
        default:
            return SearchResult(L"Command not implemented.");
        }
    case SearchCommand::Replace:
    case SearchCommand::ReplStop:
        switch (req.command.extent) {
        case SearchCommand::Forward:
        case SearchCommand::Backward:
            return singleReplace(req);
        default:
            return SearchResult(L"Command not implemented.");
        }
    case SearchCommand::Count:
    case SearchCommand::FindAll:
    case SearchCommand::Mark:
    case SearchCommand::Select:
    case SearchCommand::Show:
    case SearchCommand::ReplaceAll:
        switch (req.command.extent) {
        case SearchCommand::All:
        case SearchCommand::Before:
        case SearchCommand::After:
            return multipleSearch(req);
        case SearchCommand::Open:
            return openDocumentsSearch(req);
        default:
            return SearchResult(L"Command not implemented.");
        }
    default:
        return SearchResult(L"Command not implemented.");
    }
}
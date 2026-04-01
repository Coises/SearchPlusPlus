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
#include "RegularExpression.h"


namespace {

struct ProgressInfoBoost : ProgressInfo {
    RegularExpression rx;
    Scintilla::Position offset1 = 0;
    Scintilla::Position offset2 = 0;
    ProgressInfoBoost(SearchRequest& req) : ProgressInfo(req) {}
};


struct ProgressOpenDocumentsBoost : ProgressInfoBoost {
    std::string originalFind;
    std::string originalRepl;
    ProgressOpenDocumentsBoost(SearchRequest& req) : ProgressInfoBoost(req) {}
};


SearchResult singleFind(SearchRequest& req, bool postReplace = false) {

    bool inSelection = !postReplace && req.command.scope == SearchCommand::Selection;
    bool wrap        = req.context->notFound();

    plugin.getScintillaPointers(req.sciFind);
    Scintilla::Position findLength = sci.Length();
    if (findLength < 1) return req.error(L"Nothing to find.");
    std::string find = sci.GetText(sci.Length());

    plugin.getScintillaPointers(req.sciText);
    RegularExpression& rx = req.context->rx;
    rx.setup(sci);
    std::string rxMessage = rx.find(find, data.matchCase, data.dotAll, data.freeSpacing);
    if (!rxMessage.empty()) return req.error(L"Invalid regular expression.", rxMessage);

    Scintilla::Position searchFrom = inSelection ? req.ranges.front().cpMin : std::max(sci.Anchor(), sci.CurrentPos());
    if (wrap) searchFrom = 0;

    for (size_t range = 0; range < req.ranges.size(); ++range) {
        auto& r = req.ranges[range];
        if (searchFrom >= r.cpMax) continue;
        if (rx.search(std::max(searchFrom, r.cpMin), r.cpMax, r.cpMin)) {
            Scintilla::Position found  = rx.position();
            Scintilla::Position length = rx.length(0);
            if (length == 0 && req.context->nullAt(found)) {
                if (found == r.cpMax) continue;
                if (!rx.search(found + 1, r.cpMax, r.cpMin)) continue;
                found  = rx.position();
                length = rx.length(0);
            }
            return req.found(found, found + length, postReplace ? L"Match replaced; next match found." : L"Match found.");
        }
    }

    if (postReplace) return req.endRepl(L"Match replaced; no more matches found.");
    if (searchFrom <= req.ranges.front().cpMin) return req.notFound(
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

    plugin.getScintillaPointers(req.sciRepl);
    Scintilla::Position replLength = sci.Length();
    std::string repl = replLength ? sci.GetText(replLength) : "";

    plugin.getScintillaPointers(req.sciText);
    sci.TargetFromSelection();
    Scintilla::Position foundStart = sci.TargetStart();
    Scintilla::Position foundEnd = sci.TargetEnd();
    if (!req.context->calcIsValid) {
        std::string errmsg = req.context->calc.parse(repl);
        if (!errmsg.empty()) return SearchResult(L"Error in replacement formula.", "", errmsg);
        req.context->calcIsValid = true;
    }
    req.context->rx.invalidate();
    sci.ReplaceTarget(req.context->calc.format(req.context->rx, sci));
    req.context->rx.invalidate();

    Scintilla::Position start = sci.TargetStart();
    Scintilla::Position end = sci.TargetEnd();
    if (req.command.scope == SearchCommand::Scope::Region && start != end) sci.IndicatorFillRange(start, end - start);
    if (req.command.verb == SearchCommand::ReplStop) return req.replaced(start, end, L"Match replaced.");
    sci.SetSel(start, end);

    Scintilla::Position offset = end - start - (foundEnd - foundStart);
    if (offset) for (size_t range = 0; range < req.ranges.size(); ++range) {
        auto& r = req.ranges[range];
        if (r.cpMin >= foundEnd) r.cpMin += offset;
        if (r.cpMax >= foundEnd) r.cpMax += offset;
    }
    return singleFind(req, true);

}


bool progressiveSearch(ProgressInfo& pi) {

    auto& count    = pi.count;
    auto& position = pi.position;
    auto& rangeEnd = pi.rangeEnd;
    auto& req      = pi.req;

    ProgressInfoBoost& pib = static_cast<ProgressInfoBoost&>(pi);

    auto r = req.ranges[pi.rangeIndex];
    Scintilla::Position scanMax = std::min(rangeEnd, r.cpMax);
    if (position > scanMax) return false;
    bool rxSuccess = pib.rx.search(std::max(position + pib.offset2, r.cpMin + pib.offset1), r.cpMax + pib.offset2, r.cpMin + pib.offset1);

    if (rxSuccess){
        Scintilla::Position found  = pib.rx.position();
        Scintilla::Position length = pib.rx.length(0);
        position = found + length - pib.offset2;
        if (position > scanMax) return false;
        ++count;
        if (req.command.verb != SearchCommand::Count)
            pib.hitSet->add(found, found + length);
        switch (req.command.verb) {
        case SearchCommand::Select:
            if (count == 1) scrollIntoView(position, found);
                       else sci.AddSelection(position, found);
            break;
        case SearchCommand::Show:
            sci.ShowLines(sci.LineFromPosition(found), sci.LineFromPosition(position));
            [[fallthrough]];
        case SearchCommand::Mark:
            if (length) {
                sci.SetIndicatorCurrent(data.indicator);
                sci.SetIndicatorValue(1);
                sci.IndicatorFillRange(found, length);
            }
            break;
        case SearchCommand::ReplaceAll:
        {
            std::string repl = pib.req.context->calc.format(pib.rx, sci);
            sci.SetTarget(Scintilla::Span(found, found + length));
            sci.ReplaceTarget(repl);
            if (req.command.scope == SearchCommand::Scope::Region && !repl.empty()) {
                sci.SetIndicatorCurrent(data.indicator);
                sci.SetIndicatorValue(1);
                sci.IndicatorFillRange(found, repl.length());
            }
            pib.offset2 += repl.length() - length;
            break;
        }
        }
        if (!length) ++position;
        pib.rx.invalidate();
    }

    if (!rxSuccess || position >= r.cpMax) {
        if (++pi.rangeIndex >= req.ranges.size()) return false;
        pib.offset1 = pib.offset2;
        position = req.ranges[pi.rangeIndex].cpMin;
    }

    return position < rangeEnd;

}


SearchResult multipleSearch(SearchRequest& req) {
    plugin.getScintillaPointers(req.sciFind);
    Scintilla::Position findLength = sci.Length();
    if (findLength < 1) return req.error(L"Nothing to find.");
    std::string find = sci.GetText(findLength);
    ProgressInfoBoost pib(req);
    if (req.command.verb == SearchCommand::ReplaceAll && !req.context->calcIsValid) {
        plugin.getScintillaPointers(req.sciRepl);
        Scintilla::Position replLength = sci.Length();
        std::string repl = replLength ? sci.GetText(replLength) : "";
        std::string errmsg = req.context->calc.parse(repl);
        if (!errmsg.empty()) return SearchResult(L"Error in replacement formula.", "", errmsg);
        req.context->calcIsValid = true;
    }
    plugin.getScintillaPointers(req.sciText);
    pib.rx.setup(sci);
    std::string rxMessage = pib.rx.find(find, data.matchCase, data.dotAll, data.freeSpacing);
    if (!rxMessage.empty()) return SearchResult(L"Invalid regular expression.", rxMessage);
    sci.SetIndicatorCurrent(data.indicator);
    sci.SetIndicatorValue(1);
    pib.hitSet = std::make_unique<ProgressInfo::HitSet>();
    pib.hitSet->searchString = "(Regex): " + find;
    pib.exec(progressiveSearch);
    if (pib.result.success() && req.command.verb == SearchCommand::ReplaceAll) req.context->calcIsValid = false;
    return pib.result;
}


void openDocumentsPrepare(ProgressInfo& pi) {
    ProgressOpenDocumentsBoost& pib = static_cast<ProgressOpenDocumentsBoost&>(pi);
    pib.offset1 = pib.offset2 = 0;
    pib.rx.setup(sci);
    pib.rx.find(pib.originalFind, data.matchCase, data.dotAll, data.freeSpacing);
    pib.req.context->calc.parse(pib.originalRepl);
    pib.req.context->calcIsValid = true;
}


SearchResult openDocumentsSearch(SearchRequest& req) {
    plugin.getScintillaPointers(req.sciFind);
    Scintilla::Position findLength = sci.Length();
    if (findLength < 1) return req.error(L"Nothing to find.");
    ProgressOpenDocumentsBoost pib(req);
    pib.originalFind = sci.GetText(findLength);
    if (req.command.verb == SearchCommand::ReplaceAll && !req.context->calcIsValid) {
        plugin.getScintillaPointers(req.sciRepl);
        Scintilla::Position replLength = sci.Length();
        pib.originalRepl = replLength ? sci.GetText(replLength) : "";
        std::string errmsg = req.context->calc.parse(pib.originalRepl);
        if (!errmsg.empty()) return SearchResult(L"Error in replacement formula.", "", errmsg);
        req.context->calcIsValid = true;
    }
    plugin.getScintillaPointers(req.sciText);
    pib.rx.setup(sci);
    std::string rxMessage = pib.rx.find(pib.originalFind, data.matchCase, data.dotAll, data.freeSpacing);
    if (!rxMessage.empty()) return SearchResult(L"Invalid regular expression.", rxMessage);
    pib.hitSet = std::make_unique<ProgressInfo::HitSet>();
    pib.hitSet->searchString = "(Regex): " + pib.originalFind;
    pib.openDocuments(progressiveSearch, openDocumentsPrepare);
    if (pib.result.success() && req.command.verb == SearchCommand::ReplaceAll) req.context->calcIsValid = false;
    return pib.result;
}

}


SearchResult searchBoost(SearchRequest& req) {
    switch (req.command.verb) {
    case SearchCommand::Find:
        switch (req.command.extent) {
        case SearchCommand::Forward:
            return singleFind(req);
        default:
            return SearchResult(L"Command not implemented.");
        }
    case SearchCommand::Replace:
    case SearchCommand::ReplStop:
        switch (req.command.extent) {
        case SearchCommand::Forward:
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
        case SearchCommand::View:
        case SearchCommand::Open:
            return openDocumentsSearch(req);
        default:
            return SearchResult(L"Command not implemented.");
        }
    default:
        return SearchResult(L"Command not implemented.");
    }
}

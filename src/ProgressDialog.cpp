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
#include "resource.h"
#include <set>

void showHitlist(ProgressInfo& pi);

namespace {
    struct OpenDocument {
        UINT_PTR bufferID;
        int index;
        int view;
        OpenDocument(UINT_PTR bufferID, int index, int view) : bufferID(bufferID), index(index), view(view) {}
    };
}

class ProgressiveDocumentsList : public std::vector<OpenDocument> {
public:
    intptr_t fileHits   = 0;
    intptr_t priorCount = 0;
};


namespace {

    std::locale userLocale("");

    INT_PTR CALLBACK progressDialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {

        ProgressInfo* pip = 0;
        if (uMsg == WM_INITDIALOG) {
            SetWindowLongPtr(hwndDlg, DWLP_USER, lParam);
            pip = reinterpret_cast<ProgressInfo*>(lParam);
        }
        else pip = reinterpret_cast<ProgressInfo*>(GetWindowLongPtr(hwndDlg, DWLP_USER));
        if (!pip) return TRUE;
        ProgressInfo& pi = *pip;

        switch (uMsg) {

        case WM_DESTROY:
            EnableWindow(data.searchDialog, TRUE);
            SetForegroundWindow(data.searchDialog);
            return TRUE;

        case WM_INITDIALOG:
        {
            config_rect::show(hwndDlg);  // centers dialog on owner client area, without saving position
            SendDlgItemMessage(hwndDlg, IDC_SEARCH_PROGRESS_BAR, PBM_SETRANGE32, 0, 4096);
            SendDlgItemMessage(hwndDlg, IDC_SEARCH_PROGRESS_BAR, PBM_SETPOS, 0, 0);
            EnableWindow(data.searchDialog, FALSE);
            return TRUE;
        }

        case WM_WINDOWPOSCHANGED:
            if (!pi.timerStarted && reinterpret_cast<WINDOWPOS*>(lParam)->flags & SWP_SHOWWINDOW) {
                pi.timerStarted = true;
                PostMessage(hwndDlg, WM_TIMER, 0, 0);
            }
            return FALSE;

        case WM_COMMAND:
            switch (LOWORD(wParam)) {
            case IDCANCEL:
                KillTimer(hwndDlg, 1);
                EndDialog(hwndDlg, 1);
                return TRUE;
            }
            break;

        case WM_TIMER:
        {
            SetTimer(hwndDlg, 1, 0, 0);
            auto before = GetTickCount64();
            for (;;) {
                if (!pi.task(pi)) {
                    if (++pi.documentIndex >= pi.documentCount) break;
                    pi.nextDocument();
                    continue;
                }
                if (GetTickCount64() - before > 250) {
                    if (pi.documentCount < 2) {
                        SendDlgItemMessage(hwndDlg, IDC_SEARCH_PROGRESS_BAR, PBM_SETPOS,
                            static_cast<LPARAM>((pi.position << 12) / (pi.rangeEnd - pi.rangeStart)), 0);
                        SetDlgItemText(hwndDlg, IDC_SEARCH_PROGRESS_MESSAGE,
                            std::format(userLocale, L"{:s}: {:\u2002>10Ld}", pi.message, pi.count).data());
                    }
                    else {
                        SendDlgItemMessage(hwndDlg, IDC_SEARCH_PROGRESS_BAR, PBM_SETPOS,
                            static_cast<LPARAM>( ((pi.documentIndex * (pi.rangeEnd - pi.rangeStart) + pi.position) << 12)
                                               / ((pi.rangeEnd - pi.rangeStart) * pi.documentCount)), 0);
                        SetDlgItemText(hwndDlg, IDC_SEARCH_PROGRESS_MESSAGE,
                            std::format(userLocale, L"{:s}: {:\u2002>10Ld} in {:d}/{:d} documents",
                                pi.message, pi.count, pi.documentIndex + 1, pi.documentCount).data());
                    }
                    return TRUE;
                }
            }
            KillTimer(hwndDlg, 1);
            EndDialog(hwndDlg, 0);
            return TRUE;
        }

        case WM_NOTIFY:
            switch (((LPNMHDR)lParam)->code) {
            case NM_CLICK:
                KillTimer(hwndDlg, 1);
                EndDialog(hwndDlg, 1);
                return TRUE;
            }
            break;

        }

        return FALSE;

    }

}


void ProgressInfo::preClear() {
    if (!needPreClear) return;
    switch (req.command.verb) {
    case SearchCommand::Select:
        if (data.clearSelections || req.command.scope == SearchCommand::Selection) sci.ClearSelections();
        else if (sci.SelectionIsRectangle()) sci.ChangeSelectionMode(Scintilla::SelectionMode::Stream);
        break;
    case SearchCommand::Mark:
        if (data.clearMarked || req.command.scope == SearchCommand::Region) {
            sci.SetIndicatorCurrent(data.indicator);
            sci.IndicatorClearRange(0, sci.Length());
            if (data.markAlsoBookmarks) sci.MarkerDeleteAll(data.bookMarker);
        }
        break;
    case SearchCommand::Show:
        if (data.hideBeforeShow) {
            sci.SetIndicatorCurrent(data.indicator);
            sci.IndicatorClearRange(0, sci.Length());
            if (data.markAlsoBookmarks) sci.MarkerDeleteAll(data.bookMarker);
            sci.HideLines(0, sci.LineCount() - 1);
        }
        else if (sci.AllLinesVisible()) sci.HideLines(0, sci.LineCount() - 1);
        break;
    }
    needPreClear = false;
}


SearchResult ProgressInfo::exec(bool (*worker)(ProgressInfo&)) {

    task = worker;
    plugin.getScintillaPointers(req.sciText);

    rangeStart = req.ranges.front().cpMin;
    rangeEnd   = req.ranges.back ().cpMax;
    if (req.command.extent == SearchCommand::After) {
        rangeStart = req.command.verb == SearchCommand::ReplaceAll && req.context->found() ? sci.SelectionStart() : sci.SelectionEnd();
        if (req.ranges.back().cpMax <= rangeStart) return SearchResult(L"Nothing to search after current position or selection.");
        for (; req.ranges[rangeIndex].cpMax <= rangeStart; ++rangeIndex);
        rangeStart = std::max(rangeStart, req.ranges[rangeIndex].cpMin);
    }
    else if (req.command.extent == SearchCommand::Before) {
        rangeEnd = std::min(rangeEnd, req.command.verb == SearchCommand::ReplaceAll && req.context->found()
                                    ? sci.SelectionEnd() : sci.SelectionStart());
        if (req.ranges.front().cpMin >= rangeEnd) return SearchResult(L"Nothing to search before current position or selection.");
    }

    position = rangeStart;
    result   = SearchResult(SearchResult::Success, L"");
    message  = req.command.verb == SearchCommand::ReplaceAll ? L"Matches replaced"
             : req.command.verb == SearchCommand::Select     ? L"Matches selected"
             : req.command.verb == SearchCommand::Show       ? L"Matches shown"
             : req.command.verb == SearchCommand::Mark       ? L"Matches marked"
                                                             : L"Matches found";

    if (req.command.verb == SearchCommand::ReplaceAll) sci.BeginUndoAction();

    unsigned long long tickBefore, tickAfter;
    Scintilla::Position posBefore = rangeStart;
    tickBefore = GetTickCount64();
    double tickLimit = 2;
    while (task(*this)) {
        tickAfter = GetTickCount64();
        if (tickAfter - tickBefore < 20 || position == posBefore) continue;
        double projected =
            static_cast<double>(rangeEnd - position) * static_cast<double>(tickAfter - tickBefore)
            / (1000 * static_cast<double>(position - posBefore));
        if (projected > tickLimit) {
            DialogBoxParam(plugin.dllInstance, MAKEINTRESOURCE(IDD_SEARCH_PROGRESS), plugin.nppData._nppHandle,
                progressDialogProc, reinterpret_cast<LPARAM>(this));
            break;
        }
        posBefore = position;
        tickBefore = tickAfter;
    }

    if (req.command.verb == SearchCommand::ReplaceAll) sci.EndUndoAction();

    if (!result.error()) {
        std::wstring verb = req.command.verb == SearchCommand::ReplaceAll ? L"Replaced "
                          : req.command.verb == SearchCommand::Select     ? L"Selected "
                          : req.command.verb == SearchCommand::Show       ? L"Showing "
                          : req.command.verb == SearchCommand::Mark       ? L"Marked "
                                                                          : L"Found ";

        std::wstring suffix = req.command.scope == SearchCommand::Scope::Region    ? L" in marked text"
                            : req.command.scope == SearchCommand::Scope::Selection ? L" in selection"
                                                                                   : L"";
        if (countEmpty > 0) {
            if (req.command.verb == SearchCommand::Mark)
                verb = L"Found ";
            if (req.command.verb == SearchCommand::Mark || req.command.verb == SearchCommand::Show)
                suffix = std::format(userLocale, L" ({:Ld} marked, {:Ld} null)", count - countEmpty, countEmpty) + suffix;
        }

        if (req.command.extent == SearchCommand::All) suffix += L".";
        else {
            suffix += (req.command.extent == SearchCommand::Before ? L" before " : L" after ");
            suffix += (sci.SelectionEmpty() ? L"current position." : L"selection.");
        }
        result = !count     ? SearchResult(SearchResult::Failure, L"No matches found" + suffix)
               : count == 1 ? SearchResult(SearchResult::Success, verb + L"1 match" + suffix)
                            : SearchResult(SearchResult::Success, verb + std::format(userLocale, L"{:Ld} matches", count) + suffix);
        if (req.command.verb == SearchCommand::FindAll && count > 0) showHitlist(*this);
    }
    return result;

}


void ProgressInfo::nextDocument() {
    if (count > pdl->priorCount) {
        ++pdl->fileHits;
        pdl->priorCount = count;
    }
    if (result.error()) return;
    if (documentIndex > 0 && req.command.verb == SearchCommand::ReplaceAll) sci.EndUndoAction();
    npp(NPPM_ACTIVATEDOC, (*pdl)[documentIndex].view, (*pdl)[documentIndex].index);
    plugin.getScintillaPointers();
    Scintilla::Position length = sci.Length();
    req.ranges.clear();
    if (req.command.scope == SearchCommand::Region) {
        for (Scintilla::Position cpMin = 0;;) {
            Scintilla::Position cpMax = sci.IndicatorEnd(data.indicator, cpMin);
            if (cpMax == cpMin) break;
            if (sci.IndicatorValueAt(data.indicator, cpMin)) req.ranges.push_back(Scintilla::CharacterRangeFull{ cpMin, cpMax });
            if (cpMax == length) break;
            cpMin = cpMax;
        }
    }
    else req.ranges.emplace_back(Scintilla::CharacterRangeFull(0, length));
    position = rangeIndex = rangeStart = 0;
    rangeEnd = length;
    needPreClear = true; // This only affects Mark (in Open Documents or in this View): clearing is not contingent on finding a match
    preClear();          // in a particular document; for consistency, any clearing is applied to all documents in the extent.
    prep(*this);
    if (req.command.verb == SearchCommand::ReplaceAll) sci.BeginUndoAction();
}


SearchResult ProgressInfo::openDocuments(bool (*worker)(ProgressInfo&), void (*prepare)(ProgressInfo&)) {

    ProgressiveDocumentsList documents;
    task = worker;
    prep = prepare;
    pdl  = &documents;

    result = SearchResult(SearchResult::Success, L"");
    message = req.command.verb == SearchCommand::ReplaceAll ? L"Matches replaced"
            : req.command.verb == SearchCommand::Mark       ? L"Matches marked"
                                                            : L"Matches found";

    int originalDocIndex0 = static_cast<int>(npp(NPPM_GETCURRENTDOCINDEX, 0, 0));
    int originalDocIndex1 = static_cast<int>(npp(NPPM_GETCURRENTDOCINDEX, 0, 1));
    int originalView      = static_cast<int>(npp(NPPM_GETCURRENTVIEW, 0, 0));

    if (req.command.extent == SearchCommand::Open) {
        std::set<UINT_PTR> alreadyHaveBuffer;
        int documentCount0 = originalDocIndex0 < 0 ? 0 : static_cast<int>(npp(NPPM_GETNBOPENFILES, 0, 1));
        int documentCount1 = originalDocIndex1 < 0 ? 0 : static_cast<int>(npp(NPPM_GETNBOPENFILES, 0, 2));
        for (int pos = 0; pos < documentCount0; ++pos) {
            UINT_PTR buffer = npp(NPPM_GETBUFFERIDFROMPOS, pos, 0);
            documents.emplace_back(buffer, pos, 0);
            alreadyHaveBuffer.insert(buffer);
        }
        for (int pos = 0; pos < documentCount1; ++pos) {
            UINT_PTR buffer = npp(NPPM_GETBUFFERIDFROMPOS, pos, 1);
            if (alreadyHaveBuffer.contains(buffer)) continue;
            documents.emplace_back(buffer, pos, 1);
        }
    }
    else {
        int documentsInView = static_cast<int>(npp(NPPM_GETNBOPENFILES, 0, originalView + 1));
        for (int pos = 0; pos < documentsInView; ++pos)
            documents.emplace_back(npp(NPPM_GETBUFFERIDFROMPOS, pos, originalView), pos, originalView);
    }

    documentCount = documents.size();
    nextDocument();

    unsigned long long tickBefore, tickAfter;
    Scintilla::Position posBefore = rangeStart;
    tickBefore = GetTickCount64();
    double tickLimit = 2;
    for (;;) {
        if (!task(*this)) {
            if (++documentIndex >= documentCount) break;
            nextDocument();
            continue;
        }
        tickAfter = GetTickCount64();
        if (tickAfter - tickBefore < 20 || position == posBefore) continue;
        double projected =
            static_cast<double>(rangeEnd - position) * static_cast<double>(tickAfter - tickBefore)
            / (1000 * static_cast<double>(position - posBefore));
        if (projected > tickLimit) {
            DialogBoxParam(plugin.dllInstance, MAKEINTRESOURCE(IDD_SEARCH_PROGRESS), plugin.nppData._nppHandle,
                progressDialogProc, reinterpret_cast<LPARAM>(this));
            break;
        }
        posBefore = position;
        tickBefore = tickAfter;
    }

    if (req.command.verb == SearchCommand::ReplaceAll) sci.EndUndoAction();

    if (originalView == 0) {
        if (req.command.extent == SearchCommand::Open && originalDocIndex1 >= 0) npp(NPPM_ACTIVATEDOC, 1, originalDocIndex1);
        npp(NPPM_ACTIVATEDOC, 0, originalDocIndex0);
    }
    else {
        if (req.command.extent == SearchCommand::Open && originalDocIndex0 >= 0) npp(NPPM_ACTIVATEDOC, 0, originalDocIndex0);
        npp(NPPM_ACTIVATEDOC, 1, originalDocIndex1);
    }

    if (!result.error()) {
        if (count > pdl->priorCount) ++pdl->fileHits;
        std::wstring verb = req.command.verb == SearchCommand::ReplaceAll ? L"Replaced "
                          : req.command.verb == SearchCommand::Mark       ? L"Marked "
                                                                          : L"Found ";

        std::wstring suffix =
            documentCount == 1 ? (req.command.extent == SearchCommand::Open ? L" in 1 open document." : L" in 1 document in this view.")
            : (count > 1 ? std::format(userLocale, L" in {:Ld} of {:Ld}", pdl->fileHits, documentCount)
                         : std::format(userLocale, L" in {:Ld}", documentCount))
            + (req.command.extent == SearchCommand::Open ? L" open documents." : L" documents in this view.");

        if (req.command.verb == SearchCommand::Mark && countEmpty > 0) {
            verb = L"Found ";
            suffix = std::format(userLocale, L" ({:Ld} marked, {:Ld} null)", count - countEmpty, countEmpty) + suffix;
        }

        result = !count     ? SearchResult(SearchResult::Failure, L"No matches found" + suffix)
               : count == 1 ? SearchResult(SearchResult::Success, verb + L"1 match" + suffix)
                            : SearchResult(SearchResult::Success, verb + std::format(userLocale, L"{:Ld} matches", count) + suffix);
        if (req.command.verb == SearchCommand::FindAll && count > 0) showHitlist(*this);
    }
    return result;

}


// The two-parameter version of ProgressInfo::HitSet::add (bufferID and codepage omitted, set to 0) must be called
// with the document in the active buffer and Scintilla pointers (plugin.getScintillaPointers()) already set.

void ProgressInfo::HitSet::add(Scintilla::Position cpMin, Scintilla::Position cpMax, UINT_PTR bufferID, UINT codepage) {
    UINT_PTR resolvedBufferID = bufferID == 0 ? npp(NPPM_GETCURRENTBUFFERID, 0, 0) : bufferID;
    if (hitBlocks.empty() || hitBlocks.back().bufferID != resolvedBufferID) {
        hitBlocks.emplace_back();
        hitBlocks.back().bufferID = resolvedBufferID;
        hitBlocks.back().codepage = bufferID == 0 ? sci.CodePage() : codepage;
        hitBlocks.back().documentPath = utf16to8(getFilePath(resolvedBufferID));
    }
    Scintilla::Line firstLine = sci.LineFromPosition(cpMin);
    HitBlock& hb = hitBlocks.back();
    if (hb.hitLines.empty() || hb.hitLines.back().line != firstLine) {
        hb.hitLines.emplace_back();
        hb.hitLines.back().line = firstLine;
        hb.hitLines.back().position = sci.PositionFromLine(firstLine);
        hb.hitLines.back().text = sci.GetLine(firstLine);
    }
    hb.hitLines.back().hits.push_back({ cpMin, cpMax });
    Scintilla::Line lastLine = sci.LineFromPosition(cpMax);
    for (Scintilla::Line line = firstLine + 1; line <= lastLine; ++line) {
        hb.hitLines.emplace_back();
        hb.hitLines.back().line = line;
        hb.hitLines.back().position = sci.PositionFromLine(line);
        hb.hitLines.back().text = sci.GetLine(line);
    }
}

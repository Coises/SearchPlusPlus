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

void showHitlist(ProgressInfo& pi);


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
            while (pi.task(pi)) {
                if (GetTickCount64() - before > 250) {
                    SendDlgItemMessage(hwndDlg, IDC_SEARCH_PROGRESS_BAR, PBM_SETPOS,
                        static_cast<LPARAM>((pi.position << 12) / (pi.rangeEnd - pi.rangeStart)), 0);
                    SetDlgItemText(hwndDlg, IDC_SEARCH_PROGRESS_MESSAGE,
                        std::format(userLocale, L"{:s}: {:\u2002>10Ld}", pi.message, pi.count).data());
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


SearchResult ProgressInfo::exec(bool (*worker)(ProgressInfo&)) {

    task = worker;
    plugin.getScintillaPointers(req.sciText);

    rangeStart = req.ranges.front().cpMin;
    rangeEnd   = req.ranges.back ().cpMax;
    if (req.command.direction == SearchCommand::After) {
        rangeStart = req.command.verb == SearchCommand::Replace && req.context->found() ? sci.SelectionStart() : sci.SelectionEnd();
        if (req.ranges.back().cpMax <= rangeStart) return SearchResult(L"Nothing to search after current position or selection.");
        for (; req.ranges[rangeIndex].cpMax <= rangeStart; ++rangeIndex);
        rangeStart = std::max(rangeStart, req.ranges[rangeIndex].cpMin);
    }
    else if (req.command.direction == SearchCommand::Before) {
        rangeEnd = std::min(rangeEnd, req.command.verb == SearchCommand::Replace && req.context->found()
                                    ? sci.SelectionEnd() : sci.SelectionStart());
        if (req.ranges.front().cpMin >= rangeEnd) return SearchResult(L"Nothing to search before current position or selection.");
    }

    position = rangeStart;
    result   = SearchResult(SearchResult::Success, L"");
    message  = req.command.verb == SearchCommand::Replace ? L"Matches replaced"
             : req.command.verb == SearchCommand::Select  ? L"Matches selected"
             : req.command.verb == SearchCommand::Show    ? L"Matches shown"
             : req.command.verb == SearchCommand::Mark    ? L"Matches marked"
                                                          : L"Matches found";

    if (req.command.verb != SearchCommand::Count) sci.BeginUndoAction();

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

    if (req.command.verb != SearchCommand::Count) sci.EndUndoAction();

    if (!result.error()) {
        std::wstring verb = req.command.verb == SearchCommand::Replace ? L"Replaced "
                          : req.command.verb == SearchCommand::Select  ? L"Selected "
                          : req.command.verb == SearchCommand::Show    ? L"Shown "
                          : req.command.verb == SearchCommand::Mark    ? L"Marked "
                                                                       : L"Found ";

        std::wstring suffix = req.command.scope == SearchCommand::Scope::Region    ? L" in marked region"
                            : req.command.scope == SearchCommand::Scope::Selection ? L" in selection"
                                                                                   : L"";
        if (req.command.direction == SearchCommand::All) suffix += L".";
        else {
            suffix += (req.command.direction == SearchCommand::Before ? L" before " : L" after ");
            suffix += (sci.SelectionEmpty() ? L"current position." : L"selection.");
        }
        result = !count     ? SearchResult(SearchResult::Failure, L"No matches found" + suffix)
               : count == 1 ? SearchResult(SearchResult::Success, verb + L"1 match" + suffix)
                            : SearchResult(SearchResult::Success, verb + std::to_wstring(count) + L" matches" + suffix);
        if (req.command.verb == SearchCommand::Find && count > 0) showHitlist(*this);
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

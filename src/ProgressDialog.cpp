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
#include "ProgressInfo.h"
#include <format>
#include <set>

void showHitlist(MatchResults& matchResults);

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

    constexpr char maximalSearchCounts[] = " 00,000,000,000,000,000,000 matches in 00,000,000,000,000,000,000 files ";

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

    if (req.command.verb == SearchCommand::FindAll) reserveSearchHeader();

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

        if (req.command.verb == SearchCommand::FindAll && count > 0) {
            appendDocumentMatches();
            updateSearchHeader(1);
            showHitlist(matchResults);
        }
    }
    return result;

}


void ProgressInfo::nextDocument() {
    if (count > pdl->priorCount) {
        ++pdl->fileHits;
        pdl->priorCount = count;
        if (req.command.verb == SearchCommand::FindAll) appendDocumentMatches();
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

    if (req.command.verb == SearchCommand::FindAll) reserveSearchHeader();

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

    if (count > pdl->priorCount) ++pdl->fileHits;

    if (req.command.verb == SearchCommand::ReplaceAll) sci.EndUndoAction();
    else if (req.command.verb == SearchCommand::FindAll && count > 0) {
        appendDocumentMatches();
        updateSearchHeader(pdl->fileHits);
    }

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
        
        if (req.command.verb == SearchCommand::FindAll && count > 0) showHitlist(matchResults);

    }

    return result;

}


void ProgressInfo::reserveSearchHeader() {
    matchResults.text = maximalSearchCounts;
    matchResults.text += data.searchEngine == SearchEngine::Boost ? "(Regex): "
        : data.searchEngine == SearchEngine::ICU ? "(ICU Regex): "
        : "(Text): ";
    for (size_t i = 0; i < req.find.length(); ++i) {
        switch (req.find[i]) {
        case '\t':
            matchResults.text += reinterpret_cast<const char*>(u8"\u2B72");
            break;
        case '\n':
            matchResults.text += reinterpret_cast<const char*>(u8"\u240A");
            break;
        case '\r':
            if (i + 1 < req.find.length() && req.find[i + 1] == '\n') {
                matchResults.text += reinterpret_cast<const char*>(u8"\u21A9");
                ++i;
            }
            else matchResults.text += reinterpret_cast<const char*>(u8"\u240D");
            break;
        default:
            matchResults.text += req.find[i];
        }
    }
    matchResults.text += "\r\n";
    matchResults.index.emplace_back();
    matchResults.index.back().lineNumber = -2;
    matchResults.index.back().length = matchResults.text.length();
}


void ProgressInfo::updateSearchHeader(size_t documentsMatched) {
    // Using wide character formatting because it honors the userLocale more reliably
    std::string searchCounts = utf16to8(std::format(userLocale, L" {:Ld} match{:s} in {:Ld} file{:s} ",
        count, count == 1 ? L"" : L"es", documentsMatched, documentsMatched == 1 ? L"" : L"s"));
    matchResults.offset = sizeof(maximalSearchCounts) - 1 - searchCounts.length();
    std::copy(searchCounts.begin(), searchCounts.end(), matchResults.text.begin() + matchResults.offset);
    matchResults.index[0].length -= matchResults.offset;
}


void ProgressInfo::DocumentMatches::add(Scintilla::Position cpMin, Scintilla::Position cpMax) {
    Scintilla::Line firstLine = sci.LineFromPosition(cpMin);
    Scintilla::Line lastLine = cpMax == cpMin ? firstLine : sci.LineFromPosition(cpMax - 1);
    if (index.empty() || firstLine != index.back().lineNumber) {
        auto& idx = index.emplace_back();
        idx.lineNumber = firstLine;
        idx.position = sci.PositionFromLine(firstLine);
        idx.length = sci.LineLength(firstLine);
    }
    index.back().matches.emplace_back(cpMin - index.back().position, cpMax - cpMin);
    for (Scintilla::Line line = firstLine + 1; line <= lastLine; ++line) {
        auto& idx = index.emplace_back();
        idx.lineNumber = line;
        idx.position = sci.PositionFromLine(line);
        idx.length = sci.LineLength(line);
    }
}


void ProgressInfo::appendDocumentMatches() {

    if (documentMatches.index.empty()) return;

    const int codepage = sci.CodePage();
    const std::wstring documentPath = getFilePath();

    size_t countMatches = 0;
    size_t totalTextLength = 0;
    intptr_t maximumLineLength = 0;
    for (const auto& idx : documentMatches.index) {
        countMatches += idx.matches.size();
        totalTextLength += idx.length;
        if (maximumLineLength < idx.length) maximumLineLength = idx.length;
    }

    // Using wide character formatting because it honors the userLocale more reliably

    const std::string fileHeader = utf16to8(std::format(userLocale, L"   {:Ld} match{:s} in {:Ld} line{:s}: ",
        countMatches, countMatches == 1 ? L"" : L"es",
        documentMatches.index.size(), documentMatches.index.size() == 1 ? L"" : L"s")
        + documentPath) + "\r\n";

    // Calculate / estimate the required additional size to reserve for matchResults.text.
    // This should be exact for UTF-8 files; for legacy codepages, it's better to guess a little high,
    // but allocation will still work if the guess is too small, it will just be slower.

    size_t textAllocationEstimate = fileHeader.size() + 2;  // +2 because the last line might not include a line ending
    textAllocationEstimate = codepage == CP_UTF8 ? totalTextLength : (3 * totalTextLength) / 2;

    matchResults.index.reserve(matchResults.index.size() + documentMatches.index.size() + 1);
    matchResults.text.reserve(matchResults.text.size() + textAllocationEstimate);

    matchResults.index.emplace_back();
    matchResults.index.back().lineNumber = -1;
    matchResults.index.back().length = fileHeader.length();
    matchResults.text += fileHeader;

    if (codepage == CP_UTF8) {
        for (auto& idx : documentMatches.index) {
            auto& mri = matchResults.index.emplace_back();
            mri.matches = std::move(idx.matches);
            mri.length = idx.length;
            mri.lineNumber = idx.lineNumber;
            size_t pos = matchResults.text.length();
            matchResults.text.resize(pos + idx.length);
            Scintilla::TextRangeFull trf = { idx.position, idx.position + idx.length, matchResults.text.data() + pos };
            sci.GetTextRangeFull(&trf);
        }
    }

    else {

        struct Buffers {
            Scintilla::TextRangeFull trf;
            std::string  legacy;
            std::wstring wide;
            std::string  unicode;
            int codepage = 0;
            void sciToUni(intptr_t cpMin, intptr_t cpMax) {
                trf.chrg.cpMin = cpMin;
                trf.chrg.cpMax = cpMax;
                trf.lpstrText  = legacy.data();
                sci.GetTextRangeFull(&trf);
                int wlen = MultiByteToWideChar(codepage, 0, legacy.data(), static_cast<int>(cpMax - cpMin),
                                               wide.data(), static_cast<int>(wide.length()));
                unicode = utf16to8(std::wstring_view(wide.data(), wlen));
            }
        } buffers;

        buffers.legacy .resize(maximumLineLength);
        buffers.wide   .resize(maximumLineLength);
        buffers.unicode.reserve(maximumLineLength * 3);
        buffers.codepage = codepage;

        MatchResults::LineIndex::SingleMatch* multiLineMatch = 0;
        intptr_t multiLineMatchEnd = 0;

        for (size_t i = 0; i < documentMatches.index.size(); ++i) {

            const auto& idx = documentMatches.index[i];
            auto& mri = matchResults.index.emplace_back();
            mri.lineNumber = idx.lineNumber;
            mri.length = 0;
            intptr_t pos = idx.position;

            if (multiLineMatch && multiLineMatchEnd <= idx.position + idx.length) {
                buffers.sciToUni(pos, multiLineMatchEnd);
                matchResults.text += buffers.unicode;
                mri.length += buffers.unicode.length();
                multiLineMatch->length += buffers.unicode.length();
                pos = multiLineMatchEnd;
                multiLineMatch = 0;
            }

            if (idx.matches.empty()) {
                if (pos < idx.position + idx.length) {
                    buffers.sciToUni(pos, idx.position + idx.length);
                    matchResults.text += buffers.unicode;
                    mri.length = buffers.unicode.length();
                    if (multiLineMatch) multiLineMatch->length += buffers.unicode.length();
                }
            }

            else {
                mri.matches.resize(idx.matches.size());
                mri.length = 0;
                for (size_t j = 0; j < idx.matches.size(); ++j) {
                    auto& match = idx.matches[j];
                    if (pos < match.offset + idx.position) /* unmatched text before this match */ {
                        buffers.sciToUni(pos, idx.position + match.offset);
                        matchResults.text += buffers.unicode;
                        mri.length += buffers.unicode.length();
                    }
                    pos = idx.position + std::min(idx.length, match.offset + match.length);
                    buffers.sciToUni(idx.position + match.offset, pos);
                    mri.matches[j].offset = mri.length;
                    mri.matches[j].length = buffers.unicode.length();
                    matchResults.text += buffers.unicode;
                    mri.length += buffers.unicode.length();
                }

                if (pos < idx.position + idx.length) /* unmatched text after the last match */ {
                    buffers.sciToUni(pos, idx.position + idx.length);
                    matchResults.text += buffers.unicode;
                    mri.length += buffers.unicode.length();
                }
                else if (pos < idx.position + idx.matches.back().offset + idx.matches.back().length) {
                    multiLineMatch = &mri.matches.back();
                    multiLineMatchEnd = idx.position + idx.matches.back().offset + idx.matches.back().length;
                }
            }
        }
    }

    if (matchResults.index.back().length == 0 || (matchResults.text.back() != '\n' && matchResults.text.back() != '\r')) {
        matchResults.text += "\r\n";
        matchResults.index.back().length += 2;
    }

    documentMatches.index.clear();

}
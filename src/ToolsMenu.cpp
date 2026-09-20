// This file is part of Search++ (a plugin for Notepad++),
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

void showSettingsDialog();
void clearHitlist();
void closeSearchInFilesDialog();
bool hitlistEmpty();
HWND hitlistHwnd();
void hideHitlist();
void showHitlist();
void showSearchDialog();
void showSearchInFilesDialog();
void syncReplaceButton();


namespace ToolsCommand {

    constexpr unsigned char Focus_Find_Or_Repl = 'o';
    constexpr unsigned char SearchInFiles      = 'g';
    constexpr unsigned char Hitlist_Show       = 'h';
    constexpr unsigned char BookmarkWhenMark   = 'b';
    constexpr unsigned char JumpReplace        = 'j';
    constexpr unsigned char SaveSearch         = 's';
    constexpr unsigned char ShowLines          = 'q';
    constexpr unsigned char ShowLinesShift     = 'Q';
    constexpr unsigned char ShowShown          = 'W';
    constexpr unsigned char ExpandVisible      = 'p';
    constexpr unsigned char ExpandVisibleShift = 'P';
    constexpr unsigned char HideAll            = 'D';
    constexpr unsigned char SelToMark          = 'm';
    constexpr unsigned char SelToMarkShift     = 'M';
    constexpr unsigned char MarkShown          = 'k';
    constexpr unsigned char MarkShownShift     = 'K';
    constexpr unsigned char MarkToSel          = 'S';
    constexpr unsigned char RemoveMarksFromSel = 'X';
    constexpr unsigned char InvertMarked       = 'I';
    constexpr unsigned char CopyMarked         = 'C';
    constexpr unsigned char CopyMarkedDialog   = 'Y';
    constexpr unsigned char CopyMarkedMultiple = 'T';
    constexpr unsigned char ClearMarks         = 'R';
    constexpr unsigned char ClearMarksMultiple = 'V';
    constexpr unsigned char ShowAllClear       = 'A';
    constexpr unsigned char ClearHitlist       = 1  ;
    constexpr unsigned char Settings           = 'E';
    constexpr unsigned char SearchDialog_Close = 'O';

    // Following are not on the Tools menu, but use this mechanism to implement dialog-wide shortcuts

    constexpr unsigned char SearchInFiles_Close = 'G';
    constexpr unsigned char Hitlist_Hide        = 'H';
    constexpr unsigned char Document_Focus      = 'n';
    constexpr unsigned char All_Windows_Close   = 'N';

};


namespace {

    const std::map<const unsigned char, const wchar_t*> Tools_Text {
        { ToolsCommand::Focus_Find_Or_Repl, L"Search &Open Documents..."                },
        { ToolsCommand::SearchInFiles     , L"Search in &Files..."                      },
        { ToolsCommand::Hitlist_Show      , L"Searc&h Results..."                       },
        { ToolsCommand::BookmarkWhenMark  , L"&Bookmark lines when marking text"        },
        { ToolsCommand::JumpReplace       , L"&Jump to next match after replace"        },
        { ToolsCommand::SaveSearch        , L"&Save search..."                          },
        { ToolsCommand::ShowLines         , L"Show "                                    },
        { ToolsCommand::ShowShown         , L"Sho&w only lines with shown text"         },
        { ToolsCommand::ExpandVisible     , L"Ex&pand visible"                          },
        { ToolsCommand::HideAll           , L"Hi&de all lines"                          },
        { ToolsCommand::SelToMark         , L"&Mark selected text"                      },
        { ToolsCommand::MarkShown         , L"Mar&k shown text"                         },
        { ToolsCommand::MarkToSel         , L"Se&lect marked text"                      },
        { ToolsCommand::RemoveMarksFromSel, L"Remove marks from selected te&xt"         },
        { ToolsCommand::InvertMarked      , L"&Invert marked text"                      },
        { ToolsCommand::CopyMarked        , L"&Copy marked text "                       },
        { ToolsCommand::CopyMarkedDialog  , L"Cop&y marked text..."                     },
        { ToolsCommand::CopyMarkedMultiple, L"Copy marked &text as multiple selections" },
        { ToolsCommand::ClearMarks        , L"&Remove marks "                           },
        { ToolsCommand::ClearMarksMultiple, L"Remo&ve marks from multiple documents..." },
        { ToolsCommand::ShowAllClear      , L"Clear shown (show &all and clear style)"  },
        { ToolsCommand::ClearHitlist      , L"Clear search res&ults list"               },
        { ToolsCommand::Settings          , L"S&ettings..."                             },
        { ToolsCommand::SearchDialog_Close, L"Cl&ose"                                   }
    };
    
    void AddToolItem(HMENU menu, unsigned char command, bool accelerator, const std::wstring& tag = L"") {
        if (!Tools_Text.contains(command)) return;
        std::wstring item = Tools_Text.at(command) + tag;
        if (accelerator && isalpha(command)) {
            item += (isupper(command) ? L"\tCtrl+Shift+" : L"\tCtrl+");
            item += static_cast<wchar_t>(toupper(command));
        }
        AppendMenu(menu, MF_STRING, command, item.data());
    }

    struct ToolsState {
        bool anyHidden   = false;
        bool anyHits     = false;
        bool anyMarked   = false;
        bool anySelected = false;
        bool anyShown    = false;
        bool anyShownNn  = false;
        bool anyVisible  = false;
        bool hitVisible  = false;
        bool selVisible  = false;
        bool shift       = false;
        void get() {
            anyHidden = !sci.AllLinesVisible();
            anyHits = !hitlistEmpty();
            if (sci.IndicatorValueAt(data.markIndicator, 0)) anyMarked = true;
            else {
                Scintilla::Position p = sci.IndicatorEnd(data.markIndicator, 0);
                anyMarked = p != 0 && p != sci.Length();
            }
            anySelected = !sci.SelectionEmpty();
            if (sci.IndicatorValueAt(data.showIndicator, 0)) anyShownNn = true;
            else {
                Scintilla::Position p = sci.IndicatorEnd(data.showIndicator, 0);
                anyShownNn = p != 0 && p != sci.Length();
            }
            if (anyShownNn) anyShown = true;
            else if (!zlmIndicator) anyShown = false;
            else {
                if (sci.IndicatorValueAt(zlmIndicator + 1, 0)) anyShown = true;
                else {
                    Scintilla::Position p = sci.IndicatorEnd(zlmIndicator + 1, 0);
                    anyShown = p != 0 && p != sci.Length();
                }
            }
            anyVisible = sci.LineVisible(0) || sci.VisibleFromDocLine(sci.LineCount() - 1);
            {
                HWND hh = hitlistHwnd();
                hitVisible = hh && IsWindowVisible(hh);
            }
            if (!anyHidden) selVisible = true;
            else if (!anyVisible) selVisible = false;
            else {
                selVisible = true;
                int selections = sci.Selections();
                for (int n = 0; n < selections; ++n) {
                    Scintilla::Position cpMin = sci.SelectionNStart(n);
                    Scintilla::Position cpMax = sci.SelectionNEnd(n);
                    Scintilla::Line lnMin = sci.LineFromPosition(cpMin);
                    if (!sci.LineVisible(lnMin)) {
                        selVisible = false;
                        break;
                    }
                    if (cpMax == cpMin) continue;
                    Scintilla::Line lnMax = sci.LineFromPosition(cpMax - 1);
                    if (lnMax == lnMin) continue;
                    if (!sci.LineVisible(lnMax)) {
                        selVisible = false;
                        break;
                    }
                    Scintilla::Line lnGap = lnMax - lnMin;
                    if (lnGap == 1) continue;
                    if (sci.VisibleFromDocLine(lnMax) - sci.VisibleFromDocLine(lnMin) < lnGap) {
                        selVisible = false;
                        break;
                    }
                }
            }
        }
    };


    // Dialog procedure for Tools | Copy Marked Text...

    INT_PTR CALLBACK copyMarkedDialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM) {
    
        switch (uMsg) {
    
        case WM_DESTROY:
            return TRUE;
    
        case WM_INITDIALOG:
        {
            config_rect::show(hwndDlg);  // centers dialog on owner client area, without saving position
            switch (data.copyMarkedSeparator) {
            case CopyMarkedSeparator::None  : CheckRadioButton(hwndDlg, IDC_COPYMARKED_NONE, IDC_COPYMARKED_CUSTOM, IDC_COPYMARKED_NONE  ); break;
            case CopyMarkedSeparator::Blank : CheckRadioButton(hwndDlg, IDC_COPYMARKED_NONE, IDC_COPYMARKED_CUSTOM, IDC_COPYMARKED_BLANK ); break;
            case CopyMarkedSeparator::Tab   : CheckRadioButton(hwndDlg, IDC_COPYMARKED_NONE, IDC_COPYMARKED_CUSTOM, IDC_COPYMARKED_TAB   ); break;
            case CopyMarkedSeparator::Custom: CheckRadioButton(hwndDlg, IDC_COPYMARKED_NONE, IDC_COPYMARKED_CUSTOM, IDC_COPYMARKED_CUSTOM); break;
            default                         : CheckRadioButton(hwndDlg, IDC_COPYMARKED_NONE, IDC_COPYMARKED_CUSTOM, IDC_COPYMARKED_LINE  );
            }
            std::wstring wText = utf8to16(data.copyMarkedSeparatorText.get());
            std::wstring showText;
            for (const wchar_t& wc : wText) switch (wc) {
            case L'\t': showText += L"\\t";  break;
            case L'\n': showText += L"\\n";  break;
            case L'\r': showText += L"\\r";  break;
            case L'\\': showText += L"\\\\"; break;
            default   : showText += wc;
            }
            HWND hText = GetDlgItem(hwndDlg, IDC_COPYMARKED_TEXT);
            SetWindowText(hText, showText.data());
            EnableWindow(hText, data.copyMarkedSeparator == CopyMarkedSeparator::Custom);
            if (npp(NPPM_ISDARKMODEENABLED, 0, 0)) npp(NPPM_DARKMODESUBCLASSANDTHEME, NPP::NppDarkMode::dmfInit, hwndDlg);
            return TRUE;
        }
    
        case WM_COMMAND:
    
            switch (LOWORD(wParam)) {
    
            case IDCANCEL:
                EndDialog(hwndDlg, 1);
                return TRUE;
    
            case IDOK:
            {
                data.copyMarkedSeparator = 
                      IsDlgButtonChecked(hwndDlg, IDC_COPYMARKED_NONE  ) == BST_CHECKED ? CopyMarkedSeparator::None
                    : IsDlgButtonChecked(hwndDlg, IDC_COPYMARKED_BLANK ) == BST_CHECKED ? CopyMarkedSeparator::Blank
                    : IsDlgButtonChecked(hwndDlg, IDC_COPYMARKED_TAB   ) == BST_CHECKED ? CopyMarkedSeparator::Tab
                    : IsDlgButtonChecked(hwndDlg, IDC_COPYMARKED_CUSTOM) == BST_CHECKED ? CopyMarkedSeparator::Custom
                                                                                        : CopyMarkedSeparator::Line;
                std::wstring showText = GetDlgItemString(hwndDlg, IDC_COPYMARKED_TEXT);
                std::wstring wText;
                for (size_t i = 0; i < showText.length(); ++i) {
                    if (showText[i] == L'\\' && i + 1 < showText.length()) switch (showText[i + 1]) {
                    case L't' : wText += L'\t'; ++i; continue;
                    case L'n' : wText += L'\n'; ++i; continue;
                    case L'r' : wText += L'\r'; ++i; continue;
                    case L'\\': wText += L'\\'; ++i; continue;
                    }
                    wText += showText[i];
                }
                data.copyMarkedSeparatorText = utf16to8(wText);
                EndDialog(hwndDlg, 0);
                return TRUE;
            }
    
            case IDC_COPYMARKED_NONE:
            case IDC_COPYMARKED_BLANK:
            case IDC_COPYMARKED_TAB:
            case IDC_COPYMARKED_LINE:
            case IDC_COPYMARKED_CUSTOM:
                EnableWindow(GetDlgItem(hwndDlg, IDC_COPYMARKED_TEXT), 
                             IsDlgButtonChecked(hwndDlg, IDC_COPYMARKED_CUSTOM) == BST_CHECKED ? TRUE : FALSE);
                return TRUE;
            }
    
        }
    
        return FALSE;
    }
    
    
    // Dialog procedure for Remove Marks from multiple documents
    
    INT_PTR CALLBACK removeMarksDialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM) {
        switch (uMsg) {
        case WM_DESTROY:
            return TRUE;
        case WM_INITDIALOG:
            config_rect::show(hwndDlg);
            if (npp(NPPM_ISDARKMODEENABLED, 0, 0)) npp(NPPM_DARKMODESUBCLASSANDTHEME, NPP::NppDarkMode::dmfInit, hwndDlg);
            return TRUE;
        case WM_COMMAND:
            switch (LOWORD(wParam)) {
            case IDCANCEL:
                EndDialog(hwndDlg, 1);
                return TRUE;
            case IDC_REMOVEMARKS_VIEW:
                EndDialog(hwndDlg, IDC_REMOVEMARKS_VIEW);
                return TRUE;
            case IDC_REMOVEMARKS_OPEN:
                EndDialog(hwndDlg, IDC_REMOVEMARKS_OPEN);
                return TRUE;
            }
        }
        return FALSE;
    }


    struct ShowPosition {
    
        Scintilla::ScintillaCall& sci;
        Scintilla::Line startDoc;
        Scintilla::Line startVis;
        Scintilla::Line firstVis;
        Scintilla::Line firstDoc;
        Scintilla::Line firstSub;
    
        ShowPosition(Scintilla::ScintillaCall& sci)
            : sci(sci)
            , startDoc(sci.LineFromPosition(sci.SelectionStart()))
            , startVis(sci.VisibleFromDocLine(startDoc))
            , firstVis(sci.FirstVisibleLine())
            , firstDoc(sci.DocLineFromVisible(firstVis))
            , firstSub(firstVis - sci.VisibleFromDocLine(firstDoc))
        {}
    
        void scroll() /* If beginning of selection was on screen, keep it in place; otherwise keep first visible line in place */ {
            if (startVis >= firstVis && startVis < firstVis + sci.LinesOnScreen())
                sci.SetFirstVisibleLine(sci.VisibleFromDocLine(startDoc) - (startVis - firstVis));
            else sci.ScrollVertical(firstDoc, firstSub);
        }
    
    };


    bool processToolsCommandWithState(unsigned char command, ToolsState& ts) {
    
        switch (command) {
    
        case ToolsCommand::SearchInFiles:
        case ToolsCommand::SearchInFiles_Close:
            if (ts.shift) closeSearchInFilesDialog();
                     else showSearchInFilesDialog();
            break;
    
        case ToolsCommand::BookmarkWhenMark:
            data.markAlsoBookmarks = !data.markAlsoBookmarks;
            break;
    
        case ToolsCommand::JumpReplace:
        {
            SearchCommand repl = SearchCommand(data.buttonReplace);
            repl.verb = repl.verb == SearchCommand::Replace ? SearchCommand::ReplStop : SearchCommand::Replace;
            data.buttonReplace = repl;
            syncReplaceButton();
            break;
        }
    
        case ToolsCommand::ShowAllClear:
            sci.SetIndicatorCurrent(data.showIndicator);
            sci.IndicatorClearRange(0, sci.Length());
            if (zlmIndicator) {
                sci.SetIndicatorCurrent(zlmIndicator + 1);
                sci.IndicatorClearRange(0, sci.Length());
            }
        [[fallthrough]];
    
        case ToolsCommand::ShowLines:
        case ToolsCommand::ShowLinesShift:
            if (command == ToolsCommand::ShowAllClear || ts.shift || ts.selVisible) {
                ShowPosition sp(sci);
                sci.ShowLines(0, sci.LineCount() - 1);
                sp.scroll();
            }
            else {
                int n = sci.Selections();
                for (int i = 0; i < n; ++i) {
                    Scintilla::Position a = sci.SelectionNStart(i);
                    Scintilla::Position b = sci.SelectionNEnd(i);
                    if (b > a) --b;
                    sci.ShowLines(sci.LineFromPosition(a), sci.LineFromPosition(b));
                }
            }
            break;
    
        case ToolsCommand::ShowShown:
        {
            ShowPosition sp(sci);
            sci.HideLines(0, sci.LineCount() - 1);
            Scintilla::Position documentLength = sci.Length();
            for (Scintilla::Position cpMin = 0;;) {
                Scintilla::Position cpMax = sci.IndicatorEnd(data.showIndicator, cpMin);
                if (cpMax <= cpMin) cpMax = documentLength;
                if (sci.IndicatorValueAt(data.showIndicator, cpMin)) {
                    Scintilla::Position b = std::max(cpMin, cpMax - 1);
                    sci.ShowLines(sci.LineFromPosition(cpMin), sci.LineFromPosition(b));
                }
                if (cpMax == documentLength) break;
                cpMin = cpMax;
            }
            if (zlmIndicator) for (Scintilla::Position cpMin = 0;;) {
                Scintilla::Position cpMax = sci.IndicatorEnd(zlmIndicator + 1, cpMin);
                if (cpMax <= cpMin) cpMax = documentLength;
                if (sci.IndicatorValueAt(zlmIndicator + 1, cpMin)) {
                    Scintilla::Position b = std::max(cpMin, cpMax - 1);
                    sci.ShowLines(sci.LineFromPosition(cpMin), sci.LineFromPosition(b));
                }
                if (cpMax == documentLength) break;
                cpMin = cpMax;
            }
            sp.scroll();
            break;
        }
    
        case ToolsCommand::ExpandVisible:
        {
            Scintilla::Line lineCount = sci.LineCount();
            if (sci.AllLinesVisible() || (sci.VisibleFromDocLine(sci.LineCount() - 1) == 0 && !sci.LineVisible(0))) break;
            ShowPosition sp(sci);
            for (Scintilla::Line line = 0; line < lineCount; ++line) {
                if (!sci.LineVisible(line)) {
                    if (line == 0) line = sci.DocLineFromVisible(0);
                    else {
                        sci.ShowLines(line, line);
                        line = sci.DocLineFromVisible(sci.VisibleFromDocLine(line) + sci.WrapCount(line));
                    }
                    if (line > 0 && line < lineCount) sci.ShowLines(line - 1, line - 1);
                }
            }
            sp.scroll();
            break;
        }
    
        case ToolsCommand::HideAll:
            sci.HideLines(0, sci.LineCount() - 1);
            break;
    
        case ToolsCommand::SelToMark:
        case ToolsCommand::SelToMarkShift:
        {
            sci.SetIndicatorCurrent(data.markIndicator);
            if (ts.shift) {
                sci.IndicatorClearRange(0, sci.Length());
                if (data.markAlsoBookmarks) sci.MarkerDeleteAll(data.bookMarker);
            }
            sci.SetIndicatorValue(1);
            int n = sci.Selections();
            for (int i = 0; i < n; ++i) {
                Scintilla::Position a = sci.SelectionNStart(i);
                Scintilla::Position b = sci.SelectionNEnd(i);
                if (b > a) {
                    sci.IndicatorFillRange(a, b - a);
                    if (data.markAlsoBookmarks) {
                        Scintilla::Line line = sci.LineFromPosition(a);
                        if (!(sci.MarkerGet(line) & (1 << data.bookMarker))) sci.MarkerAdd(line, data.bookMarker);
                    }
                }
            }
            break;
        }
    
        case ToolsCommand::MarkShown:
        case ToolsCommand::MarkShownShift:
        {
            sci.SetIndicatorCurrent(data.markIndicator);
            if (ts.shift) {
                sci.IndicatorClearRange(0, sci.Length());
                if (data.markAlsoBookmarks) sci.MarkerDeleteAll(data.bookMarker);
            }
            sci.SetIndicatorValue(1);
            Scintilla::Position documentLength = sci.Length();
            for (Scintilla::Position cpMin = 0;;) {
                Scintilla::Position cpMax = sci.IndicatorEnd(data.showIndicator, cpMin);
                if (cpMax <= cpMin) cpMax = documentLength;
                if (sci.IndicatorValueAt(data.showIndicator, cpMin)) {
                    sci.IndicatorFillRange(cpMin, cpMax - cpMin);
                    if (data.markAlsoBookmarks) {
                        Scintilla::Line line = sci.LineFromPosition(cpMin);
                        if (!(sci.MarkerGet(line) & (1 << data.bookMarker))) sci.MarkerAdd(line, data.bookMarker);
                    }
                }
                if (cpMax == documentLength) break;
                cpMin = cpMax;
            }
            break;
        }
    
        case ToolsCommand::MarkToSel:
        {
            bool first = true;
            Scintilla::Position documentLength = sci.Length();
            for (Scintilla::Position cpMin = 0;;) {
                Scintilla::Position cpMax = sci.IndicatorEnd(data.markIndicator, cpMin);
                if (cpMax == cpMin) break;
                if (sci.IndicatorValueAt(data.markIndicator, cpMin)) {
                    if (first) {
                        sci.ClearSelections();
                        sci.SetSelection(cpMax, cpMin);
                        first = false;
                    }
                    else sci.AddSelection(cpMax, cpMin);
                }
                if (cpMax == documentLength) break;
                cpMin = cpMax;
            }
            break;
        }
    
        case ToolsCommand::RemoveMarksFromSel:
        {
            sci.SetIndicatorCurrent(data.markIndicator);
            int n = sci.Selections();
            for (int i = 0; i < n; ++i) {
                Scintilla::Position a = sci.SelectionNStart(i);
                Scintilla::Position b = sci.SelectionNEnd(i);
                if (b > a) sci.IndicatorClearRange(a, b - a);
            }
            break;
        }
    
        case ToolsCommand::InvertMarked:
        {
            sci.SetIndicatorCurrent(data.markIndicator);
            sci.SetIndicatorValue(1);
            Scintilla::Position documentLength = sci.Length();
            if (data.markAlsoBookmarks) sci.MarkerDeleteAll(data.bookMarker);
            for (Scintilla::Position cpMin = 0;;) {
                Scintilla::Position cpMax = sci.IndicatorEnd(data.markIndicator, cpMin);
                if (cpMax <= cpMin) cpMax = documentLength;
                if (sci.IndicatorValueAt(data.markIndicator, cpMin)) sci.IndicatorClearRange(cpMin, cpMax - cpMin);
                else {
                    sci.IndicatorFillRange(cpMin, cpMax - cpMin);
                    if (data.markAlsoBookmarks) {
                        Scintilla::Line line = sci.LineFromPosition(cpMin);
                        if (!(sci.MarkerGet(line) & (1 << data.bookMarker))) sci.MarkerAdd(line, data.bookMarker);
                    }
                }
                if (cpMax == documentLength) break;
                cpMin = cpMax;
            }
            break;
        }
    
        case ToolsCommand::CopyMarkedDialog:
        {
            HWND focus = GetFocus();
            INT_PTR cancel = DialogBox(plugin.dllInstance, MAKEINTRESOURCE(IDD_TOOLS_COPYMARKED), data.searchDialog, copyMarkedDialogProc);
            SetFocus(focus);
            if (cancel) break;
        }
        [[fallthrough]];
    
        case ToolsCommand::CopyMarked:
        {
            plugin.getScintillaPointers();
            std::string text;
            bool first = true;
            Scintilla::Position documentLength = sci.Length();
            std::string sep;
            switch (data.copyMarkedSeparator.get()) {
            case CopyMarkedSeparator::None: sep = ""; break;
            case CopyMarkedSeparator::Blank: sep = " "; break;
            case CopyMarkedSeparator::Tab: sep = "\t"; break;
            case CopyMarkedSeparator::Custom: sep = data.copyMarkedSeparatorText; break;
            default:
            {
                Scintilla::EndOfLine eolm = sci.EOLMode();
                sep = eolm == Scintilla::EndOfLine::CrLf ? "\r\n" : eolm == Scintilla::EndOfLine::Cr ? "\r" : "\n";
            }
            }
            for (Scintilla::Position cpMin = 0;;) {
                Scintilla::Position cpMax = sci.IndicatorEnd(data.markIndicator, cpMin);
                if (cpMax == cpMin) break;
                if (sci.IndicatorValueAt(data.markIndicator, cpMin)) {
                    if (first) first = false;
                    else text += sep;
                    text += sci.StringOfRange(Scintilla::Span(cpMin, cpMax));
                }
                if (cpMax == documentLength) break;
                cpMin = cpMax;
            }
            sci.CopyText(text.length(), text.data());
            break;
        }
    
        case ToolsCommand::CopyMarkedMultiple:
        {
            std::string text;
            Scintilla::Position documentLength = sci.Length();
            Scintilla::EndOfLine eolm = sci.EOLMode();
            std::string sep = eolm == Scintilla::EndOfLine::CrLf ? "\r\n" : eolm == Scintilla::EndOfLine::Cr ? "\r" : "\n";
            int count = 0;
            for (Scintilla::Position cpMin = 0;;) {
                Scintilla::Position cpMax = sci.IndicatorEnd(data.markIndicator, cpMin);
                if (cpMax == cpMin) break;
                if (sci.IndicatorValueAt(data.markIndicator, cpMin)) {
                    if (++count > 1) text += sep;
                    text += sci.StringOfRange(Scintilla::Span(cpMin, cpMax));
                }
                if (cpMax == documentLength) break;
                cpMin = cpMax;
            }
            if (count < 1) break;
            if (count == 1) sci.CopyText(text.length(), text.data());
            else {
                static CLIPFORMAT ClipFormatColumn = static_cast<CLIPFORMAT>(RegisterClipboardFormat(L"MSDEVColumnSelect"));
                UINT codepage = sci.CodePage();
                std::wstring cliptext = codepage == CP_UTF8 ? utf8to16(text) : toWide(text, codepage);
                if (!OpenClipboard(data.searchDialog)) break;
                EmptyClipboard();
                HGLOBAL hg = GlobalAlloc(GMEM_MOVEABLE | GMEM_ZEROINIT, (1 + cliptext.length()) * 2);
                if (!hg) {
                    CloseClipboard();
                    break;
                }
                void* pg = ::GlobalLock(hg);
                if (!pg) {
                    GlobalFree(hg);
                    CloseClipboard();
                    break;
                }
                memcpy(pg, cliptext.data(), (1 + cliptext.length()) * 2);
                GlobalUnlock(hg);
                SetClipboardData(CF_UNICODETEXT, hg);
                SetClipboardData(ClipFormatColumn, {});
                CloseClipboard();
            }
            break;
        }
    
        case ToolsCommand::ClearMarks:
            sci.SetIndicatorCurrent(data.markIndicator);
            sci.IndicatorClearRange(0, sci.Length());
            if (data.markAlsoBookmarks) sci.MarkerDeleteAll(data.bookMarker);
            break;
    
        case ToolsCommand::ClearMarksMultiple:
        {
            HWND focus = GetFocus();
            INT_PTR action = DialogBox(plugin.dllInstance, MAKEINTRESOURCE(IDD_REMOVEMARKS), data.searchDialog, removeMarksDialogProc);
            if (action == IDC_REMOVEMARKS_OPEN || action == IDC_REMOVEMARKS_VIEW) {
                int originalView = static_cast<int>(npp(NPPM_GETCURRENTVIEW, 0, 0));
                for (int view = action == IDC_REMOVEMARKS_VIEW ? originalView : 1 - originalView; ; view = originalView) {
                    int originalDocIndex = static_cast<int>(npp(NPPM_GETCURRENTDOCINDEX, 0, view));
                    if (originalDocIndex >= 0) {
                        int documentCount = static_cast<int>(npp(NPPM_GETNBOPENFILES, 0, view + 1));
                        for (int pos = 0; pos < documentCount; ++pos) {
                            npp(NPPM_ACTIVATEDOC, view, pos);
                            plugin.getScintillaPointers();
                            sci.SetIndicatorCurrent(data.markIndicator);
                            sci.IndicatorClearRange(0, sci.Length());
                            if (data.markAlsoBookmarks) sci.MarkerDeleteAll(data.bookMarker);
                        }
                        npp(NPPM_ACTIVATEDOC, view, originalDocIndex);
                    }
                    if (view == originalView) break;
                }
            }
            SetFocus(focus);
            break;
        }
    
        case ToolsCommand::ClearHitlist:
            clearHitlist();
            break;
    
        case ToolsCommand::Settings:
        {
            HWND focus = GetFocus();
            showSettingsDialog();
            SetFocus(IsWindowVisible(focus) ? focus : plugin.currentScintilla());
            break;
        }
    
        case ToolsCommand::Hitlist_Show:
        case ToolsCommand::Hitlist_Hide:
            if (ts.shift) hideHitlist();
                     else showHitlist();
            break;
    
        case ToolsCommand::Document_Focus:
            SetFocus(plugin.currentScintilla());
            break;
    
        case ToolsCommand::All_Windows_Close:
            SetFocus(plugin.currentScintilla());
            if (data.searchDialog == data.dockingDialog) npp(NPPM_DMMHIDE, 0, data.searchDialog);
            else if (data.searchDialog) ShowWindow(data.searchDialog, SW_HIDE);
            hideHitlist();
            closeSearchInFilesDialog();
            break;
    
        case ToolsCommand::Focus_Find_Or_Repl:
            if (!ts.shift) {
                HWND fw = GetFocus();
                if (data.searchDialog && (fw == data.searchDialog || IsChild(data.searchDialog, fw)))
                    SetFocus(fw == data.find.handle ? data.repl.handle : data.find.handle);
                else {
                    showSearchDialog();
                    SetFocus(data.find.handle);
                }
                break;
            }
        [[fallthrough]];

        case ToolsCommand::SearchDialog_Close:
            if (data.searchDialog) {
                if (GetActiveWindow() == data.searchDialog) SetFocus(plugin.currentScintilla());
                if (data.searchDialog == data.dockingDialog) npp(NPPM_DMMHIDE, 0, data.searchDialog);
                else ShowWindow(data.searchDialog, SW_HIDE);
            }
            break;
    
        default:
            return false;
        }
    
        return true;
    
    }

}


bool processToolsCommand(unsigned char command) {
    ToolsState ts;
    plugin.getScintillaPointers();
    ts.get();
    ts.shift = isupper(command);
    return processToolsCommandWithState(command, ts);
}


void showToolsMenu(HWND button) {

    ToolsState ts;
    plugin.getScintillaPointers();
    ts.get();

    HMENU pum = CreatePopupMenu();
    if (!button) AddToolItem(pum, ToolsCommand::Focus_Find_Or_Repl, 0);
    AddToolItem(pum, ToolsCommand::SearchInFiles, button);
    AddToolItem(pum, ToolsCommand::Hitlist_Show , button);
    AppendMenu(pum, MF_SEPARATOR, 0, 0);
    AddToolItem(pum, ToolsCommand::BookmarkWhenMark, button);
    AddToolItem(pum, ToolsCommand::JumpReplace     , button);
    AppendMenu(pum, MF_SEPARATOR, 0, 0);
    if (ts.selVisible) AddToolItem(pum, ToolsCommand::ShowLines, button, L"all li&nes");
                  else AddToolItem(pum, ToolsCommand::ShowLines, button, L"selected li&nes (Shift: all)");
    AddToolItem(pum, ToolsCommand::ShowShown    , button);
    AddToolItem(pum, ToolsCommand::ExpandVisible, button);
    AddToolItem(pum, ToolsCommand::HideAll      , button);
    AppendMenu(pum, MF_SEPARATOR, 0, 0);
    if (ts.anySelected && ts.anyMarked) AddToolItem(pum, ToolsCommand::SelToMark, button, L" (Shift: clear first)");
                                   else AddToolItem(pum, ToolsCommand::SelToMark, button);
    if (ts.anyShownNn  && ts.anyMarked) AddToolItem(pum, ToolsCommand::MarkShown, button, L" (Shift: clear first)");
                                   else AddToolItem(pum, ToolsCommand::MarkShown, button);
    AddToolItem(pum, ToolsCommand::MarkToSel         , button);
    AddToolItem(pum, ToolsCommand::RemoveMarksFromSel, button);
    AddToolItem(pum, ToolsCommand::InvertMarked      , button);
    AppendMenu(pum, MF_SEPARATOR, 0, 0);
    AddToolItem(pum, ToolsCommand::CopyMarked, button,
          data.copyMarkedSeparator == CopyMarkedSeparator::None   ? L"with no separators"
        : data.copyMarkedSeparator == CopyMarkedSeparator::Blank  ? L"separated by blanks"
        : data.copyMarkedSeparator == CopyMarkedSeparator::Tab    ? L"separated by tabs"
        : data.copyMarkedSeparator == CopyMarkedSeparator::Line   ? L"separated by line breaks"
                                                                  : L"separated by custom string"
        );
    AddToolItem(pum, ToolsCommand::CopyMarkedDialog  , button);
    AddToolItem(pum, ToolsCommand::CopyMarkedMultiple, button);
    AppendMenu(pum, MF_SEPARATOR, 0, 0);
    AddToolItem(pum, ToolsCommand::ClearMarks, button,
        data.markAlsoBookmarks ? L"and bookmarks from active document"
                               : L"from active document");
    AddToolItem(pum, ToolsCommand::ClearMarksMultiple, button);
    AppendMenu(pum, MF_SEPARATOR, 0, 0);
    AddToolItem(pum, ToolsCommand::ShowAllClear, button);
    AddToolItem(pum, ToolsCommand::ClearHitlist, button);
    AppendMenu(pum, MF_SEPARATOR, 0, 0);
    AddToolItem(pum, ToolsCommand::Settings, button);
    if (button) AddToolItem(pum, ToolsCommand::SearchDialog_Close, button);

    EnableMenuItem(pum, ToolsCommand::Hitlist_Show      , ts.hitVisible || ts.anyHits    ? MF_ENABLED : MF_GRAYED);
    EnableMenuItem(pum, ToolsCommand::ShowLines         , ts.anyHidden                   ? MF_ENABLED : MF_GRAYED);
    EnableMenuItem(pum, ToolsCommand::ShowShown         , ts.anyShown                    ? MF_ENABLED : MF_GRAYED);
    EnableMenuItem(pum, ToolsCommand::ExpandVisible     , ts.anyHidden && ts.anyVisible  ? MF_ENABLED : MF_GRAYED);
    EnableMenuItem(pum, ToolsCommand::HideAll           , ts.anyVisible                  ? MF_ENABLED : MF_GRAYED);
    EnableMenuItem(pum, ToolsCommand::SelToMark         , ts.anySelected                 ? MF_ENABLED : MF_GRAYED);
    EnableMenuItem(pum, ToolsCommand::MarkShown         , ts.anyShownNn                  ? MF_ENABLED : MF_GRAYED);
    EnableMenuItem(pum, ToolsCommand::MarkToSel         , ts.anyMarked                   ? MF_ENABLED : MF_GRAYED);
    EnableMenuItem(pum, ToolsCommand::RemoveMarksFromSel, ts.anySelected && ts.anyMarked ? MF_ENABLED : MF_GRAYED);
    EnableMenuItem(pum, ToolsCommand::CopyMarked        , ts.anyMarked                   ? MF_ENABLED : MF_GRAYED);
    EnableMenuItem(pum, ToolsCommand::CopyMarkedDialog  , ts.anyMarked                   ? MF_ENABLED : MF_GRAYED);
    EnableMenuItem(pum, ToolsCommand::CopyMarkedMultiple, ts.anyMarked                   ? MF_ENABLED : MF_GRAYED);
    EnableMenuItem(pum, ToolsCommand::ShowAllClear      , ts.anyHidden || ts.anyShown    ? MF_ENABLED : MF_GRAYED);
    EnableMenuItem(pum, ToolsCommand::ClearHitlist      , ts.anyHits                     ? MF_ENABLED : MF_GRAYED);
    EnableMenuItem(pum, ToolsCommand::ClearMarks,
        ts.anyMarked || (data.markAlsoBookmarks && sci.MarkerNext(0, 1 << data.bookMarker) >= 0) ? MF_ENABLED : MF_GRAYED);
    MENUITEMINFO mii;
    mii.cbSize = sizeof mii;
    mii.fMask = MIIM_STATE;
    mii.fState = data.markAlsoBookmarks ? MFS_CHECKED : 0;
    SetMenuItemInfo(pum, ToolsCommand::BookmarkWhenMark, FALSE, &mii);
    mii.fState = SearchCommand(data.buttonReplace).verb == SearchCommand::Replace ? MFS_CHECKED : 0;
    SetMenuItemInfo(pum, ToolsCommand::JumpReplace, FALSE, &mii);
    int choice;
    if (button) {
        TPMPARAMS tpmp;
        tpmp.cbSize = sizeof tpmp;
        GetWindowRect(button, &tpmp.rcExclude);
        choice = TrackPopupMenuEx(pum, TPM_LEFTALIGN | TPM_TOPALIGN | TPM_NONOTIFY | TPM_RETURNCMD | TPM_VERTICAL,
                                  tpmp.rcExclude.left, tpmp.rcExclude.bottom, GetParent(button), &tpmp);
    }
    else {
        Scintilla::Position caret = sci.CurrentPos();
        POINT pt;
        RECT scRect;
        GetClientRect(plugin.currentScintilla(), &scRect);
        pt.x = sci.PointXFromPosition(caret);
        pt.y = sci.PointYFromPosition(caret);
        if (pt.x < 0 || pt.y < 0 || pt.x >= scRect.right || pt.y >= scRect.bottom) GetCursorPos(&pt);
        else {
            pt.y += sci.TextHeight(sci.LineFromPosition(caret));
            ClientToScreen(plugin.currentScintilla(), &pt);
        }
        choice = TrackPopupMenuEx(pum, TPM_LEFTALIGN | TPM_TOPALIGN | TPM_NONOTIFY | TPM_RETURNCMD | TPM_VERTICAL,
                                  pt.x, pt.y, plugin.nppData._nppHandle, 0);
    }
    ts.shift = GetAsyncKeyState(VK_SHIFT) < 0;
    DestroyMenu(pum);
    processToolsCommandWithState(static_cast<unsigned char>(choice), ts);

}


void showToolsMenu() { showToolsMenu(0); }
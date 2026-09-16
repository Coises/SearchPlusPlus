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
// void hideHitlist();
// void showHitlist();
void showSearchInFilesDialog();
void syncReplaceButton();


namespace ToolsCommand {

    constexpr unsigned char SearchInFiles      = 'g';
    constexpr unsigned char BookmarkWhenMark   = 'b';
    constexpr unsigned char JumpReplace        = 'j';
    constexpr unsigned char ShowAll            = 'q';
    constexpr unsigned char ShowAllClear       = 'Q';
    constexpr unsigned char ShowSelected       = 'W';
    constexpr unsigned char ShowHighlighted    = 'P';
    constexpr unsigned char ShowSurrounding    = 'p';
    constexpr unsigned char ClearHighlights    = 'k';
    constexpr unsigned char HideAll            = 'K';
    constexpr unsigned char MarkHighlighted    = 'J';
    constexpr unsigned char SelToMark          = 'm';
    constexpr unsigned char MarkToSel          = 'M';
    constexpr unsigned char AddMarksToSel      = 'V';
    constexpr unsigned char RemoveMarksFromSel = 'X';
    constexpr unsigned char InvertMarked       = 'I';
    constexpr unsigned char CopyMarked         = 'C';
    constexpr unsigned char CopyMarkedDialog   = 'Y';
    constexpr unsigned char CopyMarkedMultiple = 'T';
    constexpr unsigned char ClearMarks         = 'R';
    constexpr unsigned char ClearMarksMultiple =   1;
    constexpr unsigned char ClearHitlist       =   2;
    constexpr unsigned char Settings           = 'E';

    // Following are not on the Tools menu, but use this mechanism to implement dialog-wide shortcuts

    constexpr unsigned char SearchInFiles_Close = 'G';

};


namespace {

    const std::map<const unsigned char, std::pair<const wchar_t*, const wchar_t*>> Tools_Text {
        { ToolsCommand::SearchInFiles     , { L"Search in &Files..."                     , L"Ctrl+G"       } },
        { ToolsCommand::BookmarkWhenMark  , { L"&Bookmark lines when marking text"       , L"Ctrl+B"       } },
        { ToolsCommand::JumpReplace       , { L"&Jump to next match after Replace"       , L"Ctrl+J"       } },
        { ToolsCommand::ShowAll           , { L"Show &All Lines"                         , L"Ctrl+Q"       } },
        { ToolsCommand::ShowAllClear      , { L"Sh&ow All Lines and Clear Highlights"    , L"Ctrl+Shift+Q" } },
        { ToolsCommand::ShowSelected      , { L"Sho&w Selected Lines"                    , L"Ctrl+Shift+W" } },
        { ToolsCommand::ShowHighlighted   , { L"Show Highlighted Li&nes"                 , L"Ctrl+Shift+P" } },
        { ToolsCommand::ShowSurrounding   , { L"Show S&urrounding"                       , L"Ctrl+P"       } },
        { ToolsCommand::ClearHighlights   , { L"Clear Hi&ghlights"                       , L"Ctrl+K"       } },
        { ToolsCommand::HideAll           , { L"&Hide All Lines"                         , L"Ctrl+Shift+K" } },
        { ToolsCommand::MarkHighlighted   , { L"Add Mar&ks to Highlighted Text"          , L"Ctrl+Shift+J" } },
        { ToolsCommand::SelToMark         , { L"&Mark Selected Text"                     , L"Ctrl+M"       } },
        { ToolsCommand::MarkToSel         , { L"&Select Marked Text"                     , L"Ctrl+Shift+M" } },
        { ToolsCommand::AddMarksToSel     , { L"A&dd Marks to Selected Text"             , L"Ctrl+Shift+V" } },
        { ToolsCommand::RemoveMarksFromSel, { L"Remove Marks from Selected Te&xt"        , L"Ctrl+Shift+X" } },
        { ToolsCommand::InvertMarked      , { L"&Invert Marked Text"                     , L"Ctrl+Shift+I" } },
        { ToolsCommand::CopyMarked        , { L"&Copy Marked Text "                      , L"Ctrl+Shift+C" } },
        { ToolsCommand::CopyMarkedDialog  , { L"Cop&y Marked Text..."                    , L"Ctrl+Shift+Y" } },
        { ToolsCommand::CopyMarkedMultiple, { L"Copy Marked &Text as multiple selections", L"Ctrl+Shift+T" } },
        { ToolsCommand::ClearMarks        , { L"&Remove marks "                          , L"Ctrl+Shift+R" } },
        { ToolsCommand::ClearMarksMultiple, { L"Remove marks from multi&ple documents...", L""             } },
        { ToolsCommand::ClearHitlist      , { L"C&lear search results list"              , L""             } },
        { ToolsCommand::Settings          , { L"S&ettings..."                            , L"Ctrl+Shift+E" } }
    };
    
    void AddToolItem(HMENU menu, unsigned char command, bool accelerator, const std::wstring& tag = L"") {
        if (!Tools_Text.contains(command)) return;
        std::wstring item = Tools_Text.at(command).first + tag;
        if (accelerator) item += L'\t' + std::wstring(Tools_Text.at(command).second);
        AppendMenu(menu, MF_STRING, command, item.data());
    }


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

}


bool processToolsCommand(unsigned char command) {

    switch (command) {

    case ToolsCommand::SearchInFiles:
        showSearchInFilesDialog();
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

    case ToolsCommand::ShowAll:
    case ToolsCommand::ShowAllClear:
    {
        plugin.getScintillaPointers();
        if (command == ToolsCommand::ShowAllClear) {
            sci.SetIndicatorCurrent(data.showIndicator);
            sci.IndicatorClearRange(0, sci.Length());
            if (zlmIndicator) {
                sci.SetIndicatorCurrent(zlmIndicator + 1);
                sci.IndicatorClearRange(0, sci.Length());
            }
        }
        ShowPosition sp(sci);
        sci.ShowLines(0, sci.LineCount() - 1);
        sp.scroll();
        break;
    }

    case ToolsCommand::ShowSelected:
    {
        plugin.getScintillaPointers();
        int n = sci.Selections();
        for (int i = 0; i < n; ++i) {
            Scintilla::Position a = sci.SelectionNStart(i);
            Scintilla::Position b = sci.SelectionNEnd(i);
            if (b > a) --b;
            sci.ShowLines(sci.LineFromPosition(a), sci.LineFromPosition(b));
        }
        break;
    }

    case ToolsCommand::ShowHighlighted:
    {
        plugin.getScintillaPointers();
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
        sp.scroll();
        break;
    }

    case ToolsCommand::ShowSurrounding:
    {
        plugin.getScintillaPointers();
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

    case ToolsCommand::ClearHighlights:
        plugin.getScintillaPointers();
        sci.SetIndicatorCurrent(data.showIndicator);
        sci.IndicatorClearRange(0, sci.Length());
        if (zlmIndicator) {
            sci.SetIndicatorCurrent(zlmIndicator + 1);
            sci.IndicatorClearRange(0, sci.Length());
        }
        break;

    case ToolsCommand::HideAll:
        plugin.getScintillaPointers();
        sci.HideLines(0, sci.LineCount() - 1);
        break;

    case ToolsCommand::MarkHighlighted:
    {
        plugin.getScintillaPointers();
        sci.SetIndicatorCurrent(data.markIndicator);
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

    case ToolsCommand::SelToMark:
    {
        plugin.getScintillaPointers();
        sci.SetIndicatorCurrent(data.markIndicator);
        sci.IndicatorClearRange(0, sci.Length());
        sci.SetIndicatorValue(1);
        if (data.markAlsoBookmarks) sci.MarkerDeleteAll(data.bookMarker);
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

    case ToolsCommand::MarkToSel:
    {
        plugin.getScintillaPointers();
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

    case ToolsCommand::AddMarksToSel:
    {
        plugin.getScintillaPointers();
        sci.SetIndicatorCurrent(data.markIndicator);
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

    case ToolsCommand::RemoveMarksFromSel:
    {
        plugin.getScintillaPointers();
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
        plugin.getScintillaPointers();
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
        plugin.getScintillaPointers();
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
        plugin.getScintillaPointers();
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
        SetFocus(focus);
        break;
    }

    case ToolsCommand::SearchInFiles_Close:
        closeSearchInFilesDialog();
        break;

    default:
        return false;
    }

    return true;

}


void showToolsMenu(HWND button) {

    HMENU pum = CreatePopupMenu();
    AddToolItem(pum, ToolsCommand::SearchInFiles, button);
    AppendMenu(pum, MF_SEPARATOR, 0, 0);
    AddToolItem(pum, ToolsCommand::BookmarkWhenMark, button);
    AddToolItem(pum, ToolsCommand::JumpReplace     , button);
    AppendMenu(pum, MF_SEPARATOR, 0, 0);
    AddToolItem(pum, ToolsCommand::ShowAll        , button);
    AddToolItem(pum, ToolsCommand::ShowAllClear   , button);
    AddToolItem(pum, ToolsCommand::ShowSelected   , button);
    AddToolItem(pum, ToolsCommand::ShowHighlighted, button);
    AddToolItem(pum, ToolsCommand::ShowSurrounding, button);
    AppendMenu(pum, MF_SEPARATOR, 0, 0);
    AddToolItem(pum, ToolsCommand::ClearHighlights, button);
    AddToolItem(pum, ToolsCommand::HideAll        , button);
    AddToolItem(pum, ToolsCommand::MarkHighlighted, button);
    AppendMenu(pum, MF_SEPARATOR, 0, 0);
    AddToolItem(pum, ToolsCommand::SelToMark         , button);
    AddToolItem(pum, ToolsCommand::MarkToSel         , button);
    AddToolItem(pum, ToolsCommand::AddMarksToSel     , button);
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
    AddToolItem(pum, ToolsCommand::ClearHitlist, button);
    AppendMenu(pum, MF_SEPARATOR, 0, 0);
    AddToolItem(pum, ToolsCommand::Settings, button);
    plugin.getScintillaPointers();
    bool hasMarks = false;
    if (sci.IndicatorValueAt(data.markIndicator, 0)) hasMarks = true;
    else {
        Scintilla::Position p = sci.IndicatorEnd(data.markIndicator, 0);
        if (p != 0 && p != sci.Length()) hasMarks = true;
    }
    bool hasHighs = false;
    if (sci.IndicatorValueAt(data.showIndicator, 0)) hasHighs = true;
    else {
        Scintilla::Position p = sci.IndicatorEnd(data.showIndicator, 0);
        if (p != 0 && p != sci.Length()) hasHighs = true;
    }
    bool allHidden = sci.VisibleFromDocLine(sci.LineCount() - 1) == 0 && !sci.LineVisible(0);
    EnableMenuItem(pum, ToolsCommand::ShowAll           , sci.AllLinesVisible()                         ? MF_GRAYED : MF_ENABLED);
    EnableMenuItem(pum, ToolsCommand::ShowAllClear      , sci.AllLinesVisible() && !hasHighs            ? MF_GRAYED : MF_ENABLED);
    EnableMenuItem(pum, ToolsCommand::ShowSelected      , sci.AllLinesVisible() || sci.SelectionEmpty() ? MF_GRAYED : MF_ENABLED);
    EnableMenuItem(pum, ToolsCommand::ShowHighlighted   , sci.AllLinesVisible() || !hasHighs            ? MF_GRAYED : MF_ENABLED);
    EnableMenuItem(pum, ToolsCommand::ShowSurrounding   , sci.AllLinesVisible() || allHidden            ? MF_GRAYED : MF_ENABLED);
    EnableMenuItem(pum, ToolsCommand::ClearHighlights   , !hasHighs                                     ? MF_GRAYED : MF_ENABLED);
    EnableMenuItem(pum, ToolsCommand::HideAll           , allHidden                                     ? MF_GRAYED : MF_ENABLED);
    EnableMenuItem(pum, ToolsCommand::MarkHighlighted   , !hasHighs           	                        ? MF_GRAYED : MF_ENABLED);
    EnableMenuItem(pum, ToolsCommand::SelToMark         , sci.SelectionEmpty()                          ? MF_GRAYED : MF_ENABLED);
    EnableMenuItem(pum, ToolsCommand::MarkToSel         , !hasMarks                                     ? MF_GRAYED : MF_ENABLED);
    EnableMenuItem(pum, ToolsCommand::AddMarksToSel     , sci.SelectionEmpty()                          ? MF_GRAYED : MF_ENABLED);
    EnableMenuItem(pum, ToolsCommand::RemoveMarksFromSel, sci.SelectionEmpty() || !hasMarks             ? MF_GRAYED : MF_ENABLED);
    EnableMenuItem(pum, ToolsCommand::CopyMarked        , !hasMarks                                     ? MF_GRAYED : MF_ENABLED);
    EnableMenuItem(pum, ToolsCommand::CopyMarkedDialog  , !hasMarks                                     ? MF_GRAYED : MF_ENABLED);
    EnableMenuItem(pum, ToolsCommand::CopyMarkedMultiple, !hasMarks                                     ? MF_GRAYED : MF_ENABLED);
    EnableMenuItem(pum, ToolsCommand::ClearHitlist      , hitlistEmpty()                                ? MF_GRAYED : MF_ENABLED);
    EnableMenuItem(pum, ToolsCommand::ClearMarks,
        hasMarks || (data.markAlsoBookmarks && sci.MarkerNext(0, 1 << data.bookMarker) >= 0) ? MF_ENABLED : MF_GRAYED);
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
    DestroyMenu(pum);
    processToolsCommand(static_cast<unsigned char>(choice));

}


void showToolsMenu() { showToolsMenu(0); }
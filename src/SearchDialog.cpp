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
#include "Shlwapi.h"
#include <windowsx.h>
#include "Host/dockingResource.h"

void showSettingsDialog();
void clearHitlist();
bool hitlistEmpty();
void hideHitlist();
void showHitlist();


namespace {

// Command button and menu text
        
const std::map<unsigned int, std::pair<const wchar_t*, const wchar_t*>> Command_Text {

    { SearchCommand(SearchCommand::Find, SearchCommand::Forward ), {L"&Find"         , L"&Find🡪"} },
    { SearchCommand(SearchCommand::Find, SearchCommand::Backward), {L"Find &Backward", L"&Find🡨"} },

    { SearchCommand(SearchCommand::Find, SearchCommand::Forward , SearchCommand::Selection), {L"Find in &Selection"              , L"▣ &Find🡪"} },
    { SearchCommand(SearchCommand::Find, SearchCommand::Forward , SearchCommand::Region   ), {L"Find in &Marked Text"            , L"▤ &Find🡪"} },
    { SearchCommand(SearchCommand::Find, SearchCommand::Forward , SearchCommand::Whole    ), {L"Find in &Whole Document"         , L"▯ &Find🡪"} },
    { SearchCommand(SearchCommand::Find, SearchCommand::Backward, SearchCommand::Selection), {L"Find Backward in S&election"     , L"▣ &Find🡨"} },
    { SearchCommand(SearchCommand::Find, SearchCommand::Backward, SearchCommand::Region   ), {L"Find Backward in Marked &Text"   , L"▤ &Find🡨"} },
    { SearchCommand(SearchCommand::Find, SearchCommand::Backward, SearchCommand::Whole    ), {L"Find Backward in Whole &Document", L"▯ &Find🡨"} },


    { SearchCommand(SearchCommand::Count, SearchCommand::All), {L"&Count", L"&Count↕"} },

    { SearchCommand(SearchCommand::Count, SearchCommand::All   , SearchCommand::Selection), {L"Count in &Selection"                    , L"▣ &Count↕"} },
    { SearchCommand(SearchCommand::Count, SearchCommand::Open                            ), {L"Count in &Open Documents"               , L"&Count 🗐"} },
    { SearchCommand(SearchCommand::Count, SearchCommand::View                            ), {L"Count in Documents in this &View"       , L"&Count 🗟"} },
    { SearchCommand(SearchCommand::Count, SearchCommand::All   , SearchCommand::Region   ), {L"Count in &Marked Text"                  , L"▤ &Count↕"} },
    { SearchCommand(SearchCommand::Count, SearchCommand::Before, SearchCommand::Region   ), {L"Count Before in Mar&ked Text"           , L"▤ &Count🡩"} },
    { SearchCommand(SearchCommand::Count, SearchCommand::After , SearchCommand::Region   ), {L"Count After in Marked &Text"            , L"▤ &Count🡫"} },
    { SearchCommand(SearchCommand::Count, SearchCommand::Open  , SearchCommand::Region   ), {L"Count in Marked Text in O&pen Documents", L"▤ &Count 🗐"} },
    { SearchCommand(SearchCommand::Count, SearchCommand::View  , SearchCommand::Region   ), {L"Count in Marked Text in this Vie&w"     , L"▤ &Count 🗟"} },
    { SearchCommand(SearchCommand::Count, SearchCommand::All   , SearchCommand::Whole    ), {L"Count in &Whole Document"               , L"▯ &Count↕"} },
    { SearchCommand(SearchCommand::Count, SearchCommand::Before, SearchCommand::Whole    ), {L"Count Before in W&hole Document"        , L"▯ &Count🡩"} },
    { SearchCommand(SearchCommand::Count, SearchCommand::After , SearchCommand::Whole    ), {L"Count After in Whole &Document"         , L"▯ &Count🡫"} },


    { SearchCommand(SearchCommand::FindAll, SearchCommand::All), {L"&Find All", L"Find &All↕"} },
    { SearchCommand(SearchCommand::Select , SearchCommand::All), {L"&Select"  , L"&Select↕"  } },
    { SearchCommand(SearchCommand::Mark   , SearchCommand::All), {L"&Mark"    , L"M&ark↕"    } },
    { SearchCommand(SearchCommand::Show   , SearchCommand::All), {L"S&how"    , L"&Show↕"    } },

    { SearchCommand(SearchCommand::FindAll, SearchCommand::Open), {L"Find All in &Open Documents"        , L"Find &All 🗐"} },
    { SearchCommand(SearchCommand::FindAll, SearchCommand::View), {L"Find All in Documents in this &View", L"Find &All 🗟"} },

    { SearchCommand(SearchCommand::FindAll, SearchCommand::All   , SearchCommand::Selection), {L"Find All in &Selection"                    , L"▣ Find &All↕"} },
    { SearchCommand(SearchCommand::FindAll, SearchCommand::All   , SearchCommand::Region   ), {L"Find All in &Marked Text"                  , L"▤ Find &All↕"} },
    { SearchCommand(SearchCommand::FindAll, SearchCommand::Before, SearchCommand::Region   ), {L"Find All Before in Mar&ked Text"           , L"▤ Find &All🡩"} },
    { SearchCommand(SearchCommand::FindAll, SearchCommand::After , SearchCommand::Region   ), {L"Find All After in Marked &Text"            , L"▤ Find &All🡫"} },
    { SearchCommand(SearchCommand::FindAll, SearchCommand::Open  , SearchCommand::Region   ), {L"Find All in Marked Text in &Open Documents", L"▤ Find &All 🗐"} },
    { SearchCommand(SearchCommand::FindAll, SearchCommand::View  , SearchCommand::Region   ), {L"Find All in Marked Text in this &View"     , L"▤ Find &All 🗟"} },
    { SearchCommand(SearchCommand::FindAll, SearchCommand::All   , SearchCommand::Whole    ), {L"Find All in &Whole Document"               , L"▯ Find &All↕" } },
    { SearchCommand(SearchCommand::FindAll, SearchCommand::Before, SearchCommand::Whole    ), {L"Find All Before in W&hole Document"        , L"▯ Find &All🡩"} },
    { SearchCommand(SearchCommand::FindAll, SearchCommand::After , SearchCommand::Whole    ), {L"Find All After in Whole &Document"         , L"▯ Find &All🡫"} },

    { SearchCommand(SearchCommand::Select, SearchCommand::All   , SearchCommand::Selection), {L"Select in &Selection"            , L"▣ &Select↕" } },
    { SearchCommand(SearchCommand::Select, SearchCommand::All   , SearchCommand::Region   ), {L"Select in &Marked Text"          , L"▤ &Select↕" } },
    { SearchCommand(SearchCommand::Select, SearchCommand::Before, SearchCommand::Region   ), {L"Select Before in Mar&ked Text"   , L"▤ &Select🡩"} },
    { SearchCommand(SearchCommand::Select, SearchCommand::After , SearchCommand::Region   ), {L"Select After in Marked &Text"    , L"▤ &Select🡫"} },
    { SearchCommand(SearchCommand::Select, SearchCommand::All   , SearchCommand::Whole    ), {L"Select in &Whole Document"       , L"▯ &Select↕" } },
    { SearchCommand(SearchCommand::Select, SearchCommand::Before, SearchCommand::Whole    ), {L"Select Before in W&hole Document", L"▯ &Select🡩"} },
    { SearchCommand(SearchCommand::Select, SearchCommand::After , SearchCommand::Whole    ), {L"Select After in Whole &Document" , L"▯ &Select🡫"} },

    { SearchCommand(SearchCommand::Mark, SearchCommand::Open                            ), {L"Mark in &Open Documents"               , L"M&ark 🗐"} },
    { SearchCommand(SearchCommand::Mark, SearchCommand::View                            ), {L"Mark in Documents in this &View"       , L"M&ark 🗟"} },
    { SearchCommand(SearchCommand::Mark, SearchCommand::All   , SearchCommand::Selection), {L"Mark in &Selection"                    , L"▣ M&ark↕" } },
    { SearchCommand(SearchCommand::Mark, SearchCommand::All   , SearchCommand::Region   ), {L"Mark in &Marked Text"                  , L"▤ M&ark↕" } },
    { SearchCommand(SearchCommand::Mark, SearchCommand::Before, SearchCommand::Region   ), {L"Mark Before in Mar&ked Text"           , L"▤ M&ark🡩"} },
    { SearchCommand(SearchCommand::Mark, SearchCommand::After , SearchCommand::Region   ), {L"Mark After in Marked &Text"            , L"▤ M&ark🡫"} },
    { SearchCommand(SearchCommand::Mark, SearchCommand::Open  , SearchCommand::Region   ), {L"Mark in Marked Text in O&pen Documents", L"▤ M&ark 🗐"} },
    { SearchCommand(SearchCommand::Mark, SearchCommand::View  , SearchCommand::Region   ), {L"Mark in Marked Text in this V&iew"     , L"▤ M&ark 🗟"} },
    { SearchCommand(SearchCommand::Mark, SearchCommand::All   , SearchCommand::Whole    ), {L"Mark in &Whole Document"               , L"▯ M&ark↕" } },
    { SearchCommand(SearchCommand::Mark, SearchCommand::Before, SearchCommand::Whole    ), {L"Mark Before in W&hole Document"        , L"▯ M&ark🡩"} },
    { SearchCommand(SearchCommand::Mark, SearchCommand::After , SearchCommand::Whole    ), {L"Mark After in Whole &Document"         , L"▯ M&ark🡫"} },

    { SearchCommand(SearchCommand::Show, SearchCommand::All   , SearchCommand::Selection), {L"Show in &Selection"              , L"▣ &Show↕"} },
    { SearchCommand(SearchCommand::Show, SearchCommand::All   , SearchCommand::Region   ), {L"Show in &Marked Text"            , L"▤ &Show↕"} },
    { SearchCommand(SearchCommand::Show, SearchCommand::Before, SearchCommand::Region   ), {L"Show Before in Mar&ked Text"     , L"▤ &Show🡩"} },
    { SearchCommand(SearchCommand::Show, SearchCommand::After , SearchCommand::Region   ), {L"Show After in Marked &Text"      , L"▤ &Show🡫"} },
    { SearchCommand(SearchCommand::Show, SearchCommand::All   , SearchCommand::Whole    ), {L"Show in &Whole Document"         , L"▯ &Show↕"} },
    { SearchCommand(SearchCommand::Show, SearchCommand::Before, SearchCommand::Whole    ), {L"Show Before in W&hole Document"  , L"▯ &Show🡩"} },
    { SearchCommand(SearchCommand::Show, SearchCommand::After , SearchCommand::Whole    ), {L"Show After in Whole &Document"   , L"▯ &Show🡫"} },


    { SearchCommand(SearchCommand::Replace , SearchCommand::Forward ), {L"&Replace"         , L"&Replace🡪"} },
    { SearchCommand(SearchCommand::Replace , SearchCommand::Backward), {L"Replace &Backward", L"&Replace🡨"} },

    { SearchCommand(SearchCommand::Replace, SearchCommand::Forward , SearchCommand::Selection), {L"Replace in &Selection"              , L"▣ &Replace🡪"} },
    { SearchCommand(SearchCommand::Replace, SearchCommand::Forward , SearchCommand::Region   ), {L"Replace in &Marked Text"            , L"▤ &Replace🡪"} },
    { SearchCommand(SearchCommand::Replace, SearchCommand::Forward , SearchCommand::Whole    ), {L"Replace in &Whole Document"         , L"▯ &Replace🡪"} },
    { SearchCommand(SearchCommand::Replace, SearchCommand::Backward, SearchCommand::Selection), {L"Replace Backward in S&election"     , L"▣ &Replace🡨"} },
    { SearchCommand(SearchCommand::Replace, SearchCommand::Backward, SearchCommand::Region   ), {L"Replace Backward in Marked &Text"   , L"▤ &Replace🡨"} },
    { SearchCommand(SearchCommand::Replace, SearchCommand::Backward, SearchCommand::Whole    ), {L"Replace Backward in Whole &Document", L"▯ &Replace🡨"} },

    { SearchCommand(SearchCommand::ReplStop, SearchCommand::Forward ), {L"&Replace"         , L"&Replace🡪❚"} },
    { SearchCommand(SearchCommand::ReplStop, SearchCommand::Backward), {L"Replace &Backward", L"&Replace❚🡨"} },

    { SearchCommand(SearchCommand::ReplStop, SearchCommand::Forward , SearchCommand::Selection), {L"Replace in &Selection"              , L"▣ &Replace🡪❚"} },
    { SearchCommand(SearchCommand::ReplStop, SearchCommand::Forward , SearchCommand::Region   ), {L"Replace in &Marked Text"            , L"▤ &Replace🡪❚"} },
    { SearchCommand(SearchCommand::ReplStop, SearchCommand::Forward , SearchCommand::Whole    ), {L"Replace in &Whole Document"         , L"▯ &Replace🡪❚"} },
    { SearchCommand(SearchCommand::ReplStop, SearchCommand::Backward, SearchCommand::Selection), {L"Replace Backward in S&election"     , L"▣ &Replace❚🡨"} },
    { SearchCommand(SearchCommand::ReplStop, SearchCommand::Backward, SearchCommand::Region   ), {L"Replace Backward in Marked &Text"   , L"▤ &Replace❚🡨"} },
    { SearchCommand(SearchCommand::ReplStop, SearchCommand::Backward, SearchCommand::Whole    ), {L"Replace Backward in Whole &Document", L"▯ &Replace❚🡨"} },


    { SearchCommand(SearchCommand::ReplaceAll, SearchCommand::All   ), {L"&Replace All"       , L"R&eplace All↕"} },

    { SearchCommand(SearchCommand::ReplaceAll, SearchCommand::All, SearchCommand::Selection), {L"Replace All in &Selection"             , L"▣ R&eplace All↕"} },
    { SearchCommand(SearchCommand::ReplaceAll, SearchCommand::Open                         ), {L"Replace All in &Open Documents"        , L"R&eplace All 🗐"} },
    { SearchCommand(SearchCommand::ReplaceAll, SearchCommand::View                         ), {L"Replace All in Documents in this &View", L"R&eplace All 🗟"} },

    { SearchCommand(SearchCommand::ReplaceAll, SearchCommand::All   , SearchCommand::Region   ), {L"Replace All in &Marked Text"                  , L"▤ R&eplace All↕" } },
    { SearchCommand(SearchCommand::ReplaceAll, SearchCommand::Before, SearchCommand::Region   ), {L"Replace All Before in Mar&ked Text"           , L"▤ R&eplace All🡩"} },
    { SearchCommand(SearchCommand::ReplaceAll, SearchCommand::After , SearchCommand::Region   ), {L"Replace All After in Marked &Text"            , L"▤ R&eplace All🡫"} },
    { SearchCommand(SearchCommand::ReplaceAll, SearchCommand::Open  , SearchCommand::Region   ), {L"Replace All in Marked Text in O&pen Documents", L"▤ R&eplace All 🗐"} },
    { SearchCommand(SearchCommand::ReplaceAll, SearchCommand::View  , SearchCommand::Region   ), {L"Replace All in Marked Text in this V&iew"     , L"▤ R&eplace All 🗟"} },
    { SearchCommand(SearchCommand::ReplaceAll, SearchCommand::All   , SearchCommand::Whole    ), {L"Replace All in &Whole Document"               , L"▯ R&eplace All↕" } },
    { SearchCommand(SearchCommand::ReplaceAll, SearchCommand::Before, SearchCommand::Whole    ), {L"Replace All Before in W&hole Document"        , L"▯ R&eplace All🡩"} },
    { SearchCommand(SearchCommand::ReplaceAll, SearchCommand::After , SearchCommand::Whole    ), {L"Replace All After in Whole &Document"         , L"▯ R&eplace All🡫"} },

};

// The contains check protects against a nasty crash if the configuration file contains bad data for the buttons.

const wchar_t* Command_Button(unsigned int command) {
    return (Command_Text.contains(command)) ? Command_Text.at(command).second: L"???";
}

const wchar_t* Command_Menu(unsigned int command) {
    return (Command_Text.contains(command)) ? Command_Text.at(command).first: L"???";
}


config_rect placement("Search dialog placement");


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


// Tools button and shortcuts processing

namespace ToolsCommand {
    constexpr unsigned char BookmarkWhenMark   = 'b';
    constexpr unsigned char JumpReplace        = 'j';
    constexpr unsigned char HideAll            = 'Q';
    constexpr unsigned char ShowAll            = 'q';
    constexpr unsigned char SelToMark          = 'm';
    constexpr unsigned char MarkToSel          = 'S';
    constexpr unsigned char CopyMarked         = 'C';
    constexpr unsigned char CopyMarkedDialog   = 'Y';
    constexpr unsigned char CopyMarkedMultiple = 'M';
    constexpr unsigned char ClearMarks         = 'R';
    constexpr unsigned char ClearMarksMultiple =   1;
    constexpr unsigned char ClearHitlist       =   2;
    constexpr unsigned char Settings           = 'E';
};

bool processToolsCommand(unsigned char command) {

    switch (command) {

    case ToolsCommand::BookmarkWhenMark:
        data.markAlsoBookmarks = !data.markAlsoBookmarks;
        break;

    case ToolsCommand::JumpReplace:
    {
        SearchCommand repl = SearchCommand(data.buttonReplace);
        repl.verb = repl.verb == SearchCommand::Replace ? SearchCommand::ReplStop : SearchCommand::Replace;
        data.buttonReplace = repl;
        if (data.dockingDialog) SetDlgItemText(data.dockingDialog, IDC_SEARCH_REPLACE, Command_Button(repl));
        if (data.regularDialog) SetDlgItemText(data.regularDialog, IDC_SEARCH_REPLACE, Command_Button(repl));
        break;
    }

    case ToolsCommand::HideAll:
        plugin.getScintillaPointers();
        sci.HideLines(0, sci.LineCount() - 1);
        break;

    case ToolsCommand::ShowAll:
        plugin.getScintillaPointers();
        sci.ShowLines(0, sci.LineCount() - 1);
        sci.SetXCaretPolicy(Scintilla::CaretPolicy::Even, 0);
        sci.SetYCaretPolicy(Scintilla::CaretPolicy::Slop | Scintilla::CaretPolicy::Strict, static_cast<int>(sci.LinesOnScreen() / 3));
        sci.ScrollRange(sci.Anchor(), sci.CurrentPos());
        break;

    case ToolsCommand::SelToMark:
    {
        plugin.getScintillaPointers();
        sci.SetIndicatorCurrent(data.indicator);
        sci.SetIndicatorValue(1);
        int n = sci.Selections();
        for (int i = 0; i < n; ++i) {
            Scintilla::Position a = sci.SelectionNStart(i);
            Scintilla::Position b = sci.SelectionNEnd(i);
            if (b > a) sci.IndicatorFillRange(a, b - a);
        }
        break;
    }

    case ToolsCommand::MarkToSel:
    {
        plugin.getScintillaPointers();
        bool first = true;
        Scintilla::Position documentLength = sci.Length();
        for (Scintilla::Position cpMin = 0;;) {
            Scintilla::Position cpMax = sci.IndicatorEnd(data.indicator, cpMin);
            if (cpMax == cpMin) break;
            if (sci.IndicatorValueAt(data.indicator, cpMin)) {
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
            Scintilla::Position cpMax = sci.IndicatorEnd(data.indicator, cpMin);
            if (cpMax == cpMin) break;
            if (sci.IndicatorValueAt(data.indicator, cpMin)) {
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
            Scintilla::Position cpMax = sci.IndicatorEnd(data.indicator, cpMin);
            if (cpMax == cpMin) break;
            if (sci.IndicatorValueAt(data.indicator, cpMin)) {
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
        sci.SetIndicatorCurrent(data.indicator);
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
                        sci.SetIndicatorCurrent(data.indicator);
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
        showSettingsDialog();
        break;

    default:
        return false;
    }

    return true;

}


// Subclass procedure for Scintilla controls: implement keyboard shortcuts not built in to Scintilla

LRESULT __stdcall subclassScintilla(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR, DWORD_PTR) {
    switch (uMsg) {
    case WM_GETDLGCODE:
        return DefSubclassProc(hWnd, uMsg, wParam, lParam) & ~DLGC_HASSETSEL;
    case WM_KEYDOWN:
        if ((lParam & KF_REPEAT) || !(GetKeyState(VK_CONTROL) & 0x8000) || wParam < L'A' || wParam > L'Z') break;
        if (!processToolsCommand(static_cast<unsigned char>(
            GetKeyState(VK_SHIFT) & 0x8000 ? std::toupper(static_cast<unsigned char>(wParam))
                                           : std::tolower(static_cast<unsigned char>(wParam)))
        )) switch (wParam) {
        case 'E':
        {
            if (GetKeyState(VK_SHIFT) & 0x8000) break;
            HWND hFindBox = GetDlgItem(data.searchDialog, IDC_SEARCH_FINDBOX);
            HWND hReplBox = GetDlgItem(data.searchDialog, IDC_SEARCH_REPLBOX);
            plugin.getScintillaPointers(hReplBox);
            std::string replText = sci.GetText(sci.Length());
            plugin.getScintillaPointers(hFindBox);
            std::string findText = sci.GetText(sci.Length());
            sci.TargetWholeDocument();
            sci.ReplaceTarget(replText);
            plugin.getScintillaPointers(hReplBox);
            sci.TargetWholeDocument();
            sci.ReplaceTarget(findText);
            break;
        }
        case 'F':
        {
            if (GetKeyState(VK_SHIFT) & 0x8000) break;
            HWND hFindBox = GetDlgItem(data.searchDialog, IDC_SEARCH_FINDBOX);
            HWND hReplBox = GetDlgItem(data.searchDialog, IDC_SEARCH_REPLBOX);
            plugin.getScintillaPointers(hFindBox);
            std::string text = sci.GetText(sci.Length());
            plugin.getScintillaPointers(hReplBox);
            if (hWnd == hReplBox) sci.TargetFromSelection();
                             else sci.TargetWholeDocument();
            sci.ReplaceTarget(text);
            break;
        }
        case 'H':
            if (GetKeyState(VK_SHIFT) & 0x8000) hideHitlist();
            else                                showHitlist();
            break;
        case 'I':
        {
            if (GetKeyState(VK_SHIFT) & 0x8000) break;
            plugin.getScintillaPointers();
            std::string text = sci.GetSelText();
            plugin.getScintillaPointers(hWnd);
            sci.TargetFromSelection();
            sci.ReplaceTarget(text);
            break;
        }
        case 'N':
            SetFocus(plugin.currentScintilla());
            if (GetKeyState(VK_SHIFT) & 0x8000) {
                if (data.searchDialog == data.dockingDialog) npp(NPPM_DMMHIDE, 0, data.searchDialog);
                                                        else ShowWindow(data.searchDialog, SW_HIDE);
                hideHitlist();
            }
            break;
        case 'O':
            if (GetKeyState(VK_SHIFT) & 0x8000) {
                SetFocus(plugin.currentScintilla());
                if (data.searchDialog == data.dockingDialog) npp(NPPM_DMMHIDE, 0, data.searchDialog);
                                                        else ShowWindow(data.searchDialog, SW_HIDE);
            }
            else {
                HWND hFindBox = GetDlgItem(data.searchDialog, IDC_SEARCH_FINDBOX);
                SetFocus(hWnd == hFindBox ? GetDlgItem(data.searchDialog, IDC_SEARCH_REPLBOX) : hFindBox);
            }
            break;
        case 'R':
        {
            if (GetKeyState(VK_SHIFT) & 0x8000) break;
            HWND hFindBox = GetDlgItem(data.searchDialog, IDC_SEARCH_FINDBOX);
            HWND hReplBox = GetDlgItem(data.searchDialog, IDC_SEARCH_REPLBOX);
            plugin.getScintillaPointers(hReplBox);
            std::string text = sci.GetText(sci.Length());
            plugin.getScintillaPointers(hFindBox);
            if (hWnd == hFindBox) sci.TargetFromSelection();
                             else sci.TargetWholeDocument();
            sci.ReplaceTarget(text);
            break;
        }
        case 'W':
        {
            if (GetKeyState(VK_SHIFT) & 0x8000) break;
            HWND hFindBox = GetDlgItem(data.searchDialog, IDC_SEARCH_FINDBOX);
            auto& setting = hWnd == hFindBox ? data.wrapFind : data.wrapRepl;
            plugin.getScintillaPointers(hWnd);
            Scintilla::Wrap current = sci.WrapMode();
            if      (current == Scintilla::Wrap::None) sci.SetWrapMode(setting = Scintilla::Wrap::Char);
            else if (current == Scintilla::Wrap::Char) sci.SetWrapMode(setting = Scintilla::Wrap::Word);
            else                                       sci.SetWrapMode(setting = Scintilla::Wrap::None);
            break;
        }
        }
    }
    return DefSubclassProc(hWnd, uMsg, wParam, lParam);
}


// Subclass procedure for non-Scintilla controls: implement dialog-wide keyboard shortcuts

LRESULT __stdcall subclassOther(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR, DWORD_PTR) {
    switch (uMsg) {
    case WM_KEYDOWN:
        if ((lParam & KF_REPEAT) || !(GetKeyState(VK_CONTROL) & 0x8000) || wParam < L'A' || wParam > L'Z') break;
        if (!processToolsCommand(static_cast<unsigned char>(
            GetKeyState(VK_SHIFT) & 0x8000 ? std::toupper(static_cast<unsigned char>(wParam))
            : std::tolower(static_cast<unsigned char>(wParam)))
        )) switch (wParam) {
        case 'H':
            if (GetKeyState(VK_SHIFT) & 0x8000) hideHitlist();
            else                                showHitlist();
            break;
        case 'N':
            SetFocus(plugin.currentScintilla());
            if (GetKeyState(VK_SHIFT) & 0x8000) {
                if (data.searchDialog == data.dockingDialog) npp(NPPM_DMMHIDE, 0, data.searchDialog);
                                                        else ShowWindow(data.searchDialog, SW_HIDE);
                hideHitlist();
            }
            break;
        case 'O':
            if (GetKeyState(VK_SHIFT) & 0x8000) {
                SetFocus(plugin.currentScintilla());
                if (data.searchDialog == data.dockingDialog) npp(NPPM_DMMHIDE, 0, data.searchDialog);
                else ShowWindow(data.searchDialog, SW_HIDE);
            }
            else SetFocus(GetDlgItem(data.searchDialog, IDC_SEARCH_FINDBOX));
            break;
        }
    }
    return DefSubclassProc(hWnd, uMsg, wParam, lParam);
}


void configureSearchBox(HWND sciBox) {

    plugin.getScintillaPointers();
    Scintilla::ColourAlpha caret          = sci.ElementColour(Scintilla::Element::Caret);
    Scintilla::ColourAlpha caretLineBack  = sci.ElementColour(Scintilla::Element::CaretLineBack);
    Scintilla::ColourAlpha selectionBack  = sci.ElementColour(Scintilla::Element::SelectionBack);
    Scintilla::ColourAlpha whiteSpace     = sci.ElementColour(Scintilla::Element::WhiteSpace);
    std::string            defaultFont    = sci.StyleGetFont(STYLE_DEFAULT);
    int                    defaultSize    = sci.StyleGetSize(STYLE_DEFAULT);
    Scintilla::Colour      defaultFore    = sci.StyleGetFore(STYLE_DEFAULT);
    Scintilla::Colour      defaultBack    = sci.StyleGetBack(STYLE_DEFAULT);

    plugin.getScintillaPointers(sciBox);

    sci.SetElementColour(Scintilla::Element::Caret                , caret        );
    sci.SetElementColour(Scintilla::Element::CaretLineBack        , caretLineBack);
    sci.SetElementColour(Scintilla::Element::SelectionBack        , selectionBack);
    sci.SetElementColour(Scintilla::Element::SelectionInactiveBack, selectionBack);
    sci.SetElementColour(Scintilla::Element::WhiteSpace           , whiteSpace   );
    sci.StyleSetFont(STYLE_DEFAULT, defaultFont.data());
    sci.StyleSetSize(STYLE_DEFAULT, defaultSize);
    sci.StyleSetFore(STYLE_DEFAULT, defaultFore);
    sci.StyleSetBack(STYLE_DEFAULT, defaultBack);
    sci.StyleClearAll();

    sci.SetModEventMask(Scintilla::ModificationFlags::DeleteText | Scintilla::ModificationFlags::InsertText);
    sci.SetMargins(0);
    sci.SetWrapMode(Scintilla::Wrap::Char);
    sci.SetTabWidth(1);
    sci.SetUseTabs(true);
    sci.SetViewWS(Scintilla::WhiteSpace::VisibleAlways);
    sci.SetViewEOL(true);
    sci.SetWhitespaceSize(2);
    sci.UsePopUp(Scintilla::PopUp::Never);
    sci.SetMultipleSelection(false);
    sci.SetVirtualSpaceOptions(Scintilla::VirtualSpace::None);
    sci.SetAdditionalSelectionTyping(true);
    sci.SetRepresentation("\n"  , reinterpret_cast<const char*>(u8"\U0001F807"));
    sci.SetRepresentation("\r"  , reinterpret_cast<const char*>(u8"\U0001F804"));
    sci.SetRepresentation("\r\n", reinterpret_cast<const char*>(u8"\u21A9"));
    sci.SetRepresentationAppearance("\n"  , Scintilla::RepresentationAppearance::Plain);
    sci.SetRepresentationAppearance("\r"  , Scintilla::RepresentationAppearance::Plain);
    sci.SetRepresentationAppearance("\r\n", Scintilla::RepresentationAppearance::Plain);
    sci.SetRepresentationColour("\n"  , whiteSpace);
    sci.SetRepresentationColour("\r"  , whiteSpace);
    sci.SetRepresentationColour("\r\n", whiteSpace);
    sci.ClearCmdKey('B' + (SCMOD_CTRL << 16));
    sci.ClearCmdKey('E' + (SCMOD_CTRL << 16));
    sci.ClearCmdKey('F' + (SCMOD_CTRL << 16));
    sci.ClearCmdKey('H' + (SCMOD_CTRL << 16));
    sci.ClearCmdKey('I' + (SCMOD_CTRL << 16));
    sci.ClearCmdKey('J' + (SCMOD_CTRL << 16));
    sci.ClearCmdKey('M' + (SCMOD_CTRL << 16));
    sci.ClearCmdKey('N' + (SCMOD_CTRL << 16));
    sci.ClearCmdKey('O' + (SCMOD_CTRL << 16));
    sci.ClearCmdKey('Q' + (SCMOD_CTRL << 16));
    sci.ClearCmdKey('R' + (SCMOD_CTRL << 16));
    sci.ClearCmdKey('W' + (SCMOD_CTRL << 16));
    sci.ClearCmdKey('C' + ((SCMOD_CTRL + SCMOD_SHIFT) << 16));
    sci.ClearCmdKey('E' + ((SCMOD_CTRL + SCMOD_SHIFT) << 16));
    sci.ClearCmdKey('H' + ((SCMOD_CTRL + SCMOD_SHIFT) << 16));
    sci.ClearCmdKey('M' + ((SCMOD_CTRL + SCMOD_SHIFT) << 16));
    sci.ClearCmdKey('N' + ((SCMOD_CTRL + SCMOD_SHIFT) << 16));
    sci.ClearCmdKey('O' + ((SCMOD_CTRL + SCMOD_SHIFT) << 16));
    sci.ClearCmdKey('Q' + ((SCMOD_CTRL + SCMOD_SHIFT) << 16));
    sci.ClearCmdKey('R' + ((SCMOD_CTRL + SCMOD_SHIFT) << 16));
    sci.ClearCmdKey('S' + ((SCMOD_CTRL + SCMOD_SHIFT) << 16));
    sci.ClearCmdKey('Y' + ((SCMOD_CTRL + SCMOD_SHIFT) << 16));
}


HWND setupSearchBox(HWND hwndDlg, int box) {
    HWND customBox = GetDlgItem(hwndDlg, box);
    RECT rectBox;
    GetWindowRect(customBox, &rectBox);
    MapWindowPoints(0, hwndDlg, reinterpret_cast<LPPOINT>(&rectBox), 2);
    DestroyWindow(customBox);
    HWND sciBox = reinterpret_cast<HWND>(npp(NPPM_CREATESCINTILLAHANDLE, 0, hwndDlg));
    SetWindowPos(sciBox, 0, rectBox.left, rectBox.top, rectBox.right - rectBox.left, rectBox.bottom - rectBox.top,
                 SWP_FRAMECHANGED | SWP_SHOWWINDOW);
    SetWindowLong(sciBox, GWL_ID, box);
    SetWindowLong(sciBox, GWL_STYLE, GetWindowLong(sciBox, GWL_STYLE) | WS_BORDER | WS_TABSTOP);
    configureSearchBox(sciBox);
    SetWindowSubclass(sciBox, subclassScintilla, 0, 0);
    return sciBox;
}


// Reference information for control locations, used in WM_SIZE

RECT hPlain, hBoost, hICU, hTools, hFindbox, hReplbox, hFind, hCount, hFindAll, hReplace, hReplaceAll,
     hMatchCase, hDotAll, hWholeWord, hFreeSpacing, hUnicodeWord, hMessage, hClient,
     vPlain, vBoost, vICU, vTools, vFindbox, vReplbox, vFind, vCount, vFindAll, vReplace, vReplaceAll,
     vMatchCase, vDotAll, vWholeWord, vFreeSpacing, vUnicodeWord, vMessage, vClient,
     wPlain, wBoost, wICU, wTools, wFindbox, wReplbox, wFind, wCount, wFindAll, wReplace, wReplaceAll,
     wMatchCase, wDotAll, wWholeWord, wFreeSpacing, wUnicodeWord, wMessage, wClient;

void GetControlReferencesForLayout(HWND hwndDlg) {
    RECT window, client;
    GetWindowRect(hwndDlg, &window);
    GetClientRect(hwndDlg, &client);
    hPlain       = {  7,   7,  44,  19};
    hBoost       = { 51,   7,  88,  19};
    hICU         = { 95,   7, 132,  19};
    hTools       = {139,   7, 176,  19};
    hFindbox     = {  7,  25, 176,  56};
    hReplbox     = {186,  25, 355,  56};
    hFind        = {  7,  62,  60,  76};
    hCount       = { 65,  62, 118,  76};
    hFindAll     = {123,  62, 176,  76};
    hReplace     = {197,  62, 266,  76};
    hReplaceAll  = {275,  62, 344,  76};
    hMatchCase   = { 25,  84,  77,  94};
    hDotAll      = { 82,  84, 174,  94};
    hWholeWord   = { 82,  84, 174,  94};
    hFreeSpacing = {179,  84, 236,  94};
    hUnicodeWord = {241,  84, 338,  94};
    hMessage     = {  7, 104, 356, 112};
    hClient      = {  0,   0, 363, 116};
    vPlain       = {  7,   7,  44,  19};
    vBoost       = { 51,   7,  88,  19};
    vICU         = { 95,   7, 132,  19};
    vTools       = {139,   7, 176,  19};
    vFindbox     = {  7,  25, 176,  56};
    vFind        = {  7,  62,  60,  76};
    vCount       = { 65,  62, 118,  76};
    vFindAll     = {123,  62, 176,  76};
    vMatchCase   = {  7,  81,  59,  91};
    vDotAll      = { 69,  81, 161,  91};
    vWholeWord   = { 69,  81, 161,  91};
    vFreeSpacing = {  7,  94,  64, 104};
    vUnicodeWord = { 69,  94, 166, 104};
    vReplbox     = {  7, 110, 176, 141};
    vReplace     = { 18, 147,  87, 161};
    vReplaceAll  = { 96, 147, 165, 161};
    vMessage     = {  7, 167, 176, 175};
    vClient      = {  0,   0, 183, 181};
    wPlain       = {  7,  7,  44, 19};
    wBoost       = {  7, 24,  44, 36};
    wICU         = {  7, 41,  44, 53};
    wTools       = {  7, 58,  44, 70};
    wFindbox     = { 52,  7, 365, 38};
    wReplbox     = {374,  7, 687, 38};
    wFind        = { 52, 43, 105, 57};
    wCount       = {110, 43, 163, 57};
    wFindAll     = {168, 43, 221, 57};
    wReplace     = {544, 43, 613, 57};
    wReplaceAll  = {618, 43, 687, 57};
    wMatchCase   = {226, 45, 278, 55};
    wDotAll      = {283, 45, 375, 55};
    wWholeWord   = {283, 45, 375, 55};
    wFreeSpacing = {380, 45, 437, 55};
    wUnicodeWord = {442, 45, 539, 55};
    wMessage     = { 51, 62, 688, 70};
    wClient      = {  0,  0, 695, 72};
    MapDialogRect(hwndDlg, &hPlain);
    MapDialogRect(hwndDlg, &hBoost);
    MapDialogRect(hwndDlg, &hICU);
    MapDialogRect(hwndDlg, &hTools);
    MapDialogRect(hwndDlg, &hFindbox);
    MapDialogRect(hwndDlg, &hReplbox);
    MapDialogRect(hwndDlg, &hFind);
    MapDialogRect(hwndDlg, &hCount);
    MapDialogRect(hwndDlg, &hFindAll);
    MapDialogRect(hwndDlg, &hReplace);
    MapDialogRect(hwndDlg, &hReplaceAll);
    MapDialogRect(hwndDlg, &hMatchCase);
    MapDialogRect(hwndDlg, &hDotAll);
    MapDialogRect(hwndDlg, &hWholeWord);
    MapDialogRect(hwndDlg, &hFreeSpacing);
    MapDialogRect(hwndDlg, &hUnicodeWord);
    MapDialogRect(hwndDlg, &hMessage);
    MapDialogRect(hwndDlg, &hClient);
    MapDialogRect(hwndDlg, &vPlain);
    MapDialogRect(hwndDlg, &vBoost);
    MapDialogRect(hwndDlg, &vICU);
    MapDialogRect(hwndDlg, &vTools);
    MapDialogRect(hwndDlg, &vFindbox);
    MapDialogRect(hwndDlg, &vReplbox);
    MapDialogRect(hwndDlg, &vFind);
    MapDialogRect(hwndDlg, &vCount);
    MapDialogRect(hwndDlg, &vFindAll);
    MapDialogRect(hwndDlg, &vReplace);
    MapDialogRect(hwndDlg, &vReplaceAll);
    MapDialogRect(hwndDlg, &vMatchCase);
    MapDialogRect(hwndDlg, &vDotAll);
    MapDialogRect(hwndDlg, &vWholeWord);
    MapDialogRect(hwndDlg, &vFreeSpacing);
    MapDialogRect(hwndDlg, &vUnicodeWord);
    MapDialogRect(hwndDlg, &vMessage);
    MapDialogRect(hwndDlg, &vClient);
    MapDialogRect(hwndDlg, &wPlain);
    MapDialogRect(hwndDlg, &wBoost);
    MapDialogRect(hwndDlg, &wICU);
    MapDialogRect(hwndDlg, &wTools);
    MapDialogRect(hwndDlg, &wFindbox);
    MapDialogRect(hwndDlg, &wReplbox);
    MapDialogRect(hwndDlg, &wFind);
    MapDialogRect(hwndDlg, &wCount);
    MapDialogRect(hwndDlg, &wFindAll);
    MapDialogRect(hwndDlg, &wReplace);
    MapDialogRect(hwndDlg, &wReplaceAll);
    MapDialogRect(hwndDlg, &wMatchCase);
    MapDialogRect(hwndDlg, &wDotAll);
    MapDialogRect(hwndDlg, &wWholeWord);
    MapDialogRect(hwndDlg, &wFreeSpacing);
    MapDialogRect(hwndDlg, &wUnicodeWord);
    MapDialogRect(hwndDlg, &wMessage);
    MapDialogRect(hwndDlg, &wClient);
}

void adjustControlPos(HWND hwnd, RECT& rect, int dx, int dy, double xStretch, double yStretch, double xMove, double yMove) {
    SetWindowPos(hwnd, 0, std::lround(rect.left + xMove * dx),
        std::lround(rect.top + yMove * dy),
        std::lround(rect.right  - rect.left + xStretch * dx),
        std::lround(rect.bottom - rect.top  + yStretch * dy),
        SWP_NOACTIVATE | SWP_NOOWNERZORDER | SWP_NOZORDER | SWP_FRAMECHANGED);
    InvalidateRect(hwnd, 0, TRUE);
}

void layoutDialog(HWND hwndDlg) {
    RECT dcr;  // dialog client rectangle
    GetClientRect(hwndDlg, &dcr);
    bool layoutVertical = data.dialogLayout == DialogLayout::Horizontal ? false
        : data.dialogLayout == DialogLayout::Vertical ? true
        : 4 * dcr.bottom > 3 * dcr.right || dcr.right < hClient.right;
    bool layoutWide = !layoutVertical && data.dialogLayout != DialogLayout::Horizontal && dcr.right - dcr.left >= wClient.right;
    if (layoutVertical) {
        int dx = dcr.right - vClient.right;
        int dy = dcr.bottom - vClient.bottom;
        if (dx < 0 || dy < 0) {
            RECT dwr;
            GetWindowRect(hwndDlg, &dwr);
            long newWidth  = dwr.right - dwr.left;
            long newHeight = dwr.bottom - dwr.top;
            if (dx < 0) { newWidth  -= dx; dx = 0; }
            if (dy < 0) { newHeight -= dy; dy = 0; }
            SetWindowPos(hwndDlg, 0, 0, 0, newWidth, newHeight,
                SWP_NOACTIVATE | SWP_NOOWNERZORDER | SWP_NOZORDER | SWP_FRAMECHANGED | SWP_NOMOVE);
        }
        adjustControlPos(GetDlgItem(hwndDlg, IDC_SEARCH_PLAIN      ), vPlain      , dx, dy, 0, 0  , 0  , 0  );
        adjustControlPos(GetDlgItem(hwndDlg, IDC_SEARCH_BOOST      ), vBoost      , dx, dy, 0, 0  , 0  , 0  );
        adjustControlPos(GetDlgItem(hwndDlg, IDC_SEARCH_ICU        ), vICU        , dx, dy, 0, 0  , 0  , 0  );
        adjustControlPos(GetDlgItem(hwndDlg, IDC_SEARCH_TOOLS      ), vTools      , dx, dy, 0, 0  , 1  , 0  );
        adjustControlPos(GetDlgItem(hwndDlg, IDC_SEARCH_FINDBOX    ), vFindbox    , dx, dy, 1, 0.5, 0  , 0  );
        adjustControlPos(GetDlgItem(hwndDlg, IDC_SEARCH_REPLBOX    ), vReplbox    , dx, dy, 1, 0.5, 0  , 0.5);
        adjustControlPos(GetDlgItem(hwndDlg, IDC_SEARCH_FIND       ), vFind       , dx, dy, 0, 0  , 0.5, 0.5);
        adjustControlPos(GetDlgItem(hwndDlg, IDC_SEARCH_COUNT      ), vCount      , dx, dy, 0, 0  , 0.5, 0.5);
        adjustControlPos(GetDlgItem(hwndDlg, IDC_SEARCH_FINDALL    ), vFindAll    , dx, dy, 0, 0  , 0.5, 0.5);
        adjustControlPos(GetDlgItem(hwndDlg, IDC_SEARCH_REPLACE    ), vReplace    , dx, dy, 0, 0  , 0.5, 1  );
        adjustControlPos(GetDlgItem(hwndDlg, IDC_SEARCH_REPLACEALL ), vReplaceAll , dx, dy, 0, 0  , 0.5, 1  );
        adjustControlPos(GetDlgItem(hwndDlg, IDC_SEARCH_MATCHCASE  ), vMatchCase  , dx, dy, 0, 0  , 0.5, 0.5);
        adjustControlPos(GetDlgItem(hwndDlg, IDC_SEARCH_DOTALL     ), vDotAll     , dx, dy, 0, 0  , 0.5, 0.5);
        adjustControlPos(GetDlgItem(hwndDlg, IDC_SEARCH_WHOLEWORD  ), vWholeWord  , dx, dy, 0, 0  , 0.5, 0.5);
        adjustControlPos(GetDlgItem(hwndDlg, IDC_SEARCH_FREESPACING), vFreeSpacing, dx, dy, 0, 0  , 0.5, 0.5);
        adjustControlPos(GetDlgItem(hwndDlg, IDC_SEARCH_UNICODEWORD), vUnicodeWord, dx, dy, 0, 0  , 0.5, 0.5);
        adjustControlPos(GetDlgItem(hwndDlg, IDC_SEARCH_MESSAGE    ), vMessage    , dx, dy, 1, 0  , 0  , 1  );
    }
    else if (layoutWide) {
        int dx = dcr.right - wClient.right;
        int dy = dcr.bottom - wClient.bottom;
        if (dx < 0 || dy < 0) {
            RECT dwr;
            GetWindowRect(hwndDlg, &dwr);
            long newWidth  = dwr.right - dwr.left;
            long newHeight = dwr.bottom - dwr.top;
            if (dx < 0) { newWidth  -= dx; dx = 0; }
            if (dy < 0) { newHeight -= dy; dy = 0; }
            SetWindowPos(hwndDlg, 0, 0, 0, newWidth, newHeight,
                SWP_NOACTIVATE | SWP_NOOWNERZORDER | SWP_NOZORDER | SWP_FRAMECHANGED | SWP_NOMOVE);
        }
        adjustControlPos(GetDlgItem(hwndDlg, IDC_SEARCH_PLAIN      ), wPlain      , dx, dy, 0  , 0, 0   , 0);
        adjustControlPos(GetDlgItem(hwndDlg, IDC_SEARCH_BOOST      ), wBoost      , dx, dy, 0  , 0, 0   , 0);
        adjustControlPos(GetDlgItem(hwndDlg, IDC_SEARCH_ICU        ), wICU        , dx, dy, 0  , 0, 0   , 0);
        adjustControlPos(GetDlgItem(hwndDlg, IDC_SEARCH_TOOLS      ), wTools      , dx, dy, 0  , 0, 0   , 0);
        adjustControlPos(GetDlgItem(hwndDlg, IDC_SEARCH_FINDBOX    ), wFindbox    , dx, dy, 0.5, 1, 0   , 0);
        adjustControlPos(GetDlgItem(hwndDlg, IDC_SEARCH_REPLBOX    ), wReplbox    , dx, dy, 0.5, 1, 0.5 , 0);
        adjustControlPos(GetDlgItem(hwndDlg, IDC_SEARCH_FIND       ), wFind       , dx, dy, 0  , 0, 0.25, 1);
        adjustControlPos(GetDlgItem(hwndDlg, IDC_SEARCH_COUNT      ), wCount      , dx, dy, 0  , 0, 0.25, 1);
        adjustControlPos(GetDlgItem(hwndDlg, IDC_SEARCH_FINDALL    ), wFindAll    , dx, dy, 0  , 0, 0.25, 1);
        adjustControlPos(GetDlgItem(hwndDlg, IDC_SEARCH_REPLACE    ), wReplace    , dx, dy, 0  , 0, 0.75, 1);
        adjustControlPos(GetDlgItem(hwndDlg, IDC_SEARCH_REPLACEALL ), wReplaceAll , dx, dy, 0  , 0, 0.75, 1);
        adjustControlPos(GetDlgItem(hwndDlg, IDC_SEARCH_MATCHCASE  ), wMatchCase  , dx, dy, 0  , 0, 0.5 , 1);
        adjustControlPos(GetDlgItem(hwndDlg, IDC_SEARCH_DOTALL     ), wDotAll     , dx, dy, 0  , 0, 0.5 , 1);
        adjustControlPos(GetDlgItem(hwndDlg, IDC_SEARCH_WHOLEWORD  ), wWholeWord  , dx, dy, 0  , 0, 0.5 , 1);
        adjustControlPos(GetDlgItem(hwndDlg, IDC_SEARCH_FREESPACING), wFreeSpacing, dx, dy, 0  , 0, 0.5 , 1);
        adjustControlPos(GetDlgItem(hwndDlg, IDC_SEARCH_UNICODEWORD), wUnicodeWord, dx, dy, 0  , 0, 0.5 , 1);
        adjustControlPos(GetDlgItem(hwndDlg, IDC_SEARCH_MESSAGE    ), wMessage    , dx, dy, 1  , 0, 0   , 1);
    }
    else {
        int dx = dcr.right - hClient.right;
        int dy = dcr.bottom - hClient.bottom;
        if (dx < 0 || dy < 0) {
            RECT dwr;
            GetWindowRect(hwndDlg, &dwr);
            long newWidth = dwr.right - dwr.left;
            long newHeight = dwr.bottom - dwr.top;
            if (dx < 0) { newWidth  -= dx; dx = 0; }
            if (dy < 0) { newHeight -= dy; dy = 0; }
            SetWindowPos(hwndDlg, 0, 0, 0, newWidth, newHeight,
                SWP_NOACTIVATE | SWP_NOOWNERZORDER | SWP_NOZORDER | SWP_FRAMECHANGED | SWP_NOMOVE);
        }
        adjustControlPos(GetDlgItem(hwndDlg, IDC_SEARCH_PLAIN      ), hPlain      , dx, dy, 0  , 0, 0   , 0);
        adjustControlPos(GetDlgItem(hwndDlg, IDC_SEARCH_BOOST      ), hBoost      , dx, dy, 0  , 0, 0   , 0);
        adjustControlPos(GetDlgItem(hwndDlg, IDC_SEARCH_ICU        ), hICU        , dx, dy, 0  , 0, 0   , 0);
        adjustControlPos(GetDlgItem(hwndDlg, IDC_SEARCH_TOOLS      ), hTools      , dx, dy, 0  , 0, 0.5 , 0);
        adjustControlPos(GetDlgItem(hwndDlg, IDC_SEARCH_FINDBOX    ), hFindbox    , dx, dy, 0.5, 1, 0   , 0);
        adjustControlPos(GetDlgItem(hwndDlg, IDC_SEARCH_REPLBOX    ), hReplbox    , dx, dy, 0.5, 1, 0.5 , 0);
        adjustControlPos(GetDlgItem(hwndDlg, IDC_SEARCH_FIND       ), hFind       , dx, dy, 0  , 0, 0.25, 1);
        adjustControlPos(GetDlgItem(hwndDlg, IDC_SEARCH_COUNT      ), hCount      , dx, dy, 0  , 0, 0.25, 1);
        adjustControlPos(GetDlgItem(hwndDlg, IDC_SEARCH_FINDALL    ), hFindAll    , dx, dy, 0  , 0, 0.25, 1);
        adjustControlPos(GetDlgItem(hwndDlg, IDC_SEARCH_REPLACE    ), hReplace    , dx, dy, 0  , 0, 0.75, 1);
        adjustControlPos(GetDlgItem(hwndDlg, IDC_SEARCH_REPLACEALL ), hReplaceAll , dx, dy, 0  , 0, 0.75, 1);
        adjustControlPos(GetDlgItem(hwndDlg, IDC_SEARCH_MATCHCASE  ), hMatchCase  , dx, dy, 0  , 0, 0.5 , 1);
        adjustControlPos(GetDlgItem(hwndDlg, IDC_SEARCH_DOTALL     ), hDotAll     , dx, dy, 0  , 0, 0.5 , 1);
        adjustControlPos(GetDlgItem(hwndDlg, IDC_SEARCH_WHOLEWORD  ), hWholeWord  , dx, dy, 0  , 0, 0.5 , 1);
        adjustControlPos(GetDlgItem(hwndDlg, IDC_SEARCH_FREESPACING), hFreeSpacing, dx, dy, 0  , 0, 0.5 , 1);
        adjustControlPos(GetDlgItem(hwndDlg, IDC_SEARCH_UNICODEWORD), hUnicodeWord, dx, dy, 0  , 0, 0.5 , 1);
        adjustControlPos(GetDlgItem(hwndDlg, IDC_SEARCH_MESSAGE    ), hMessage    , dx, dy, 1  , 0, 0   , 1);
    }
    InvalidateRect(data.searchDialog, 0, TRUE);
}


BOOL addButtonItem(HMENU menu, SearchCommand cmd) {
    return AppendMenu(menu, MF_STRING, cmd, Command_Menu(cmd));
}

void showBubble(HWND scintilla, std::string bubble) {
    constexpr size_t target_width = 32;
    for (size_t p = 0; p + target_width < bubble.length();) {
        size_t q = bubble.substr(p, target_width).find_last_of(' ');
        if (q == std::string::npos) {
            q = bubble.find_first_of(' ', p + target_width);
            if (q == std::string::npos) break;
        }
        else q += p;
        bubble[q] = '\n';
        p = q + 1;
        while (p < bubble.length() && bubble[p] == ' ') bubble.erase(p, 1);
    }
    plugin.getScintillaPointers(scintilla);
    sci.CallTipShow(sci.Length(), bubble.data());
}

void showMessage(HWND hwndDlg, const SearchResult& result) {
    SetDlgItemText(hwndDlg, IDC_SEARCH_MESSAGE, result.message.data());
    if (!result.findMessage.empty()) showBubble(GetDlgItem(hwndDlg, IDC_SEARCH_FINDBOX), result.findMessage);
    if (!result.replMessage.empty()) showBubble(GetDlgItem(hwndDlg, IDC_SEARCH_REPLBOX), result.replMessage);
}


void synchronizeDialogItems(HWND hwndDlg) {

    plugin.getScintillaPointers(GetDlgItem(hwndDlg, IDC_SEARCH_FINDBOX));
    sci.SetWrapMode(data.wrapFind);
    sci.SetZoom(data.zoomFind);
    sci.TargetWholeDocument();
    sci.ReplaceTarget(data.findBoxLast.get());

    plugin.getScintillaPointers(GetDlgItem(hwndDlg, IDC_SEARCH_REPLBOX));
    sci.SetWrapMode(data.wrapRepl);
    sci.SetZoom(data.zoomRepl);
    sci.TargetWholeDocument();
    sci.ReplaceTarget(data.replBoxLast.get());

    switch (data.searchEngine) {
    case SearchEngine::Boost   : CheckRadioButton(hwndDlg, IDC_SEARCH_PLAIN, IDC_SEARCH_ICU, IDC_SEARCH_BOOST); break;
    case SearchEngine::ICU     : CheckRadioButton(hwndDlg, IDC_SEARCH_PLAIN, IDC_SEARCH_ICU, IDC_SEARCH_ICU  ); break;
    default:                     CheckRadioButton(hwndDlg, IDC_SEARCH_PLAIN, IDC_SEARCH_ICU, IDC_SEARCH_PLAIN);
    }

    data.dotAll      .put(hwndDlg, IDC_SEARCH_DOTALL     );
    data.freeSpacing .put(hwndDlg, IDC_SEARCH_FREESPACING);
    data.matchCase   .put(hwndDlg, IDC_SEARCH_MATCHCASE  );
    data.uniWordBound.put(hwndDlg, IDC_SEARCH_UNICODEWORD);
    data.wholeWord   .put(hwndDlg, IDC_SEARCH_WHOLEWORD  );

    SetDlgItemText(hwndDlg, IDC_SEARCH_FIND      , Command_Button(data.buttonFind      ));
    SetDlgItemText(hwndDlg, IDC_SEARCH_COUNT     , Command_Button(data.buttonCount     ));
    SetDlgItemText(hwndDlg, IDC_SEARCH_FINDALL   , Command_Button(data.buttonFindAll   ));
    SetDlgItemText(hwndDlg, IDC_SEARCH_REPLACE   , Command_Button(data.buttonReplace   ));
    SetDlgItemText(hwndDlg, IDC_SEARCH_REPLACEALL, Command_Button(data.buttonReplaceAll));

    if (data.searchEngine == SearchEngine::Plain) {
        ShowWindow(GetDlgItem(hwndDlg, IDC_SEARCH_DOTALL      ), SW_HIDE);
        ShowWindow(GetDlgItem(hwndDlg, IDC_SEARCH_FREESPACING ), SW_HIDE);
        ShowWindow(GetDlgItem(hwndDlg, IDC_SEARCH_UNICODEWORD ), SW_HIDE);
        ShowWindow(GetDlgItem(hwndDlg, IDC_SEARCH_WHOLEWORD   ), SW_SHOW);
    }
    else {
        ShowWindow(GetDlgItem(hwndDlg, IDC_SEARCH_DOTALL     ), SW_SHOW);
        ShowWindow(GetDlgItem(hwndDlg, IDC_SEARCH_FREESPACING), SW_SHOW);
        ShowWindow(GetDlgItem(hwndDlg, IDC_SEARCH_UNICODEWORD), SW_SHOW);
        ShowWindow(GetDlgItem(hwndDlg, IDC_SEARCH_WHOLEWORD  ), SW_HIDE);
        EnableWindow(GetDlgItem(hwndDlg, IDC_SEARCH_UNICODEWORD), data.searchEngine == SearchEngine::ICU ? TRUE : FALSE);
    }

}


INT_PTR CALLBACK searchDialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {

    switch (uMsg) {

    case WM_DESTROY:
        RemoveWindowSubclass(GetDlgItem(hwndDlg, IDC_SEARCH_FINDBOX    ), subclassScintilla, 0);
        RemoveWindowSubclass(GetDlgItem(hwndDlg, IDC_SEARCH_REPLBOX    ), subclassScintilla, 0);
        RemoveWindowSubclass(GetDlgItem(hwndDlg, IDC_SEARCH_PLAIN      ), subclassOther    , 0);
        RemoveWindowSubclass(GetDlgItem(hwndDlg, IDC_SEARCH_BOOST      ), subclassOther    , 0);
        RemoveWindowSubclass(GetDlgItem(hwndDlg, IDC_SEARCH_ICU        ), subclassOther    , 0);
        RemoveWindowSubclass(GetDlgItem(hwndDlg, IDC_SEARCH_TOOLS      ), subclassOther    , 0);
        RemoveWindowSubclass(GetDlgItem(hwndDlg, IDC_SEARCH_DOTALL     ), subclassOther    , 0);
        RemoveWindowSubclass(GetDlgItem(hwndDlg, IDC_SEARCH_FREESPACING), subclassOther    , 0);
        RemoveWindowSubclass(GetDlgItem(hwndDlg, IDC_SEARCH_MATCHCASE  ), subclassOther    , 0);
        RemoveWindowSubclass(GetDlgItem(hwndDlg, IDC_SEARCH_UNICODEWORD), subclassOther    , 0);
        RemoveWindowSubclass(GetDlgItem(hwndDlg, IDC_SEARCH_WHOLEWORD  ), subclassOther    , 0);
        RemoveWindowSubclass(GetDlgItem(hwndDlg, IDC_SEARCH_FIND       ), subclassOther    , 0);
        RemoveWindowSubclass(GetDlgItem(hwndDlg, IDC_SEARCH_COUNT      ), subclassOther    , 0);
        RemoveWindowSubclass(GetDlgItem(hwndDlg, IDC_SEARCH_FINDALL    ), subclassOther    , 0);
        RemoveWindowSubclass(GetDlgItem(hwndDlg, IDC_SEARCH_REPLACE    ), subclassOther    , 0);
        RemoveWindowSubclass(GetDlgItem(hwndDlg, IDC_SEARCH_REPLACEALL ), subclassOther    , 0);
        return TRUE;

    case WM_INITDIALOG:
    {
        setupSearchBox(hwndDlg, IDC_SEARCH_REPLBOX);
        setupSearchBox(hwndDlg, IDC_SEARCH_FINDBOX);
        GetControlReferencesForLayout(hwndDlg);
        synchronizeDialogItems(hwndDlg);
        
        SetWindowSubclass(GetDlgItem(hwndDlg, IDC_SEARCH_PLAIN      ), subclassOther, 0, 0);
        SetWindowSubclass(GetDlgItem(hwndDlg, IDC_SEARCH_BOOST      ), subclassOther, 0, 0);
        SetWindowSubclass(GetDlgItem(hwndDlg, IDC_SEARCH_ICU        ), subclassOther, 0, 0);
        SetWindowSubclass(GetDlgItem(hwndDlg, IDC_SEARCH_TOOLS      ), subclassOther, 0, 0);
        SetWindowSubclass(GetDlgItem(hwndDlg, IDC_SEARCH_DOTALL     ), subclassOther, 0, 0);
        SetWindowSubclass(GetDlgItem(hwndDlg, IDC_SEARCH_WHOLEWORD  ), subclassOther, 0, 0);
        SetWindowSubclass(GetDlgItem(hwndDlg, IDC_SEARCH_FREESPACING), subclassOther, 0, 0);
        SetWindowSubclass(GetDlgItem(hwndDlg, IDC_SEARCH_MATCHCASE  ), subclassOther, 0, 0);
        SetWindowSubclass(GetDlgItem(hwndDlg, IDC_SEARCH_UNICODEWORD), subclassOther, 0, 0);
        SetWindowSubclass(GetDlgItem(hwndDlg, IDC_SEARCH_FIND       ), subclassOther, 0, 0);
        SetWindowSubclass(GetDlgItem(hwndDlg, IDC_SEARCH_COUNT      ), subclassOther, 0, 0);
        SetWindowSubclass(GetDlgItem(hwndDlg, IDC_SEARCH_FINDALL    ), subclassOther, 0, 0);
        SetWindowSubclass(GetDlgItem(hwndDlg, IDC_SEARCH_REPLACE    ), subclassOther, 0, 0);
        SetWindowSubclass(GetDlgItem(hwndDlg, IDC_SEARCH_REPLACEALL ), subclassOther, 0, 0);

        layoutDialog(hwndDlg);

        HWND hwndTip = CreateWindowEx(0, TOOLTIPS_CLASS, 0, WS_POPUP | TTS_ALWAYSTIP | TTS_BALLOON, 0, 0, 0, 0,
                                      hwndDlg, 0, plugin.dllInstance, 0);
        TOOLINFO toolInfo = {};
        toolInfo.cbSize = sizeof(toolInfo);
        toolInfo.uFlags = TTF_IDISHWND | TTF_SUBCLASS;
        toolInfo.hwnd = hwndDlg;
        toolInfo.uId = reinterpret_cast<UINT_PTR>(GetDlgItem(hwndDlg, IDC_SEARCH_MESSAGE));
        toolInfo.lpszText = LPSTR_TEXTCALLBACK;
        SendMessage(hwndTip, TTM_ADDTOOL, 0, reinterpret_cast<LPARAM>(&toolInfo));
        SendMessage(hwndTip, TTM_SETDELAYTIME, TTDT_AUTOPOP, 32767);

        npp(NPPM_MODELESSDIALOG, MODELESSDIALOGADD, hwndDlg);
        if (data.dialogLayout != DialogLayout::Docking) {
            placement.put(hwndDlg);
            npp(NPPM_DARKMODESUBCLASSANDTHEME, NPP::NppDarkMode::dmfInit, hwndDlg);
        }

        SetFocus(GetDlgItem(hwndDlg, IDC_SEARCH_FINDBOX));
        return FALSE;

    }

    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case IDCANCEL:
            plugin.getScintillaPointers(GetDlgItem(hwndDlg, IDC_SEARCH_FINDBOX));
            sci.TargetWholeDocument();
            data.findBoxLast = sci.TargetText();
            plugin.getScintillaPointers(GetDlgItem(hwndDlg, IDC_SEARCH_REPLBOX));
            sci.TargetWholeDocument();
            data.replBoxLast = sci.TargetText();
            if (hwndDlg == data.dockingDialog) npp(NPPM_DMMHIDE, 0, hwndDlg);
                                          else ShowWindow(hwndDlg, SW_HIDE);
            return TRUE;
        case IDC_SEARCH_FIND:
        {
            SearchCommand command = data.buttonFind.get();
            if ((GetKeyState(VK_SHIFT) & 0x8000) && data.searchEngine == SearchEngine::Plain)
                command.extent = command.extent == SearchCommand::Forward ? SearchCommand::Backward : SearchCommand::Forward;
            auto result = SearchRequest::exec(command, data.context,
                GetDlgItem(hwndDlg, IDC_SEARCH_FINDBOX), GetDlgItem(hwndDlg, IDC_SEARCH_REPLBOX), plugin.currentScintilla());
            showMessage(hwndDlg, result);
            return TRUE;
        }
        case IDC_SEARCH_COUNT:
        {
            auto result = SearchRequest::exec(data.buttonCount.value, data.context,
                GetDlgItem(hwndDlg, IDC_SEARCH_FINDBOX), GetDlgItem(hwndDlg, IDC_SEARCH_REPLBOX), plugin.currentScintilla());
            showMessage(hwndDlg, result);
            return TRUE;
        }
        case IDC_SEARCH_FINDALL:
        {
            auto result = SearchRequest::exec(data.buttonFindAll.value, data.context,
                GetDlgItem(hwndDlg, IDC_SEARCH_FINDBOX), GetDlgItem(hwndDlg, IDC_SEARCH_REPLBOX), plugin.currentScintilla());
            showMessage(hwndDlg, result);
            return TRUE;
        }
        case IDC_SEARCH_REPLACE:
        {
            SearchCommand command = data.buttonReplace.get();
            if ((GetKeyState(VK_SHIFT) & 0x8000) && data.searchEngine == SearchEngine::Plain)
                command.extent = command.extent == SearchCommand::Forward ? SearchCommand::Backward : SearchCommand::Forward;
            auto result = SearchRequest::exec(command, data.context,
                GetDlgItem(hwndDlg, IDC_SEARCH_FINDBOX), GetDlgItem(hwndDlg, IDC_SEARCH_REPLBOX), plugin.currentScintilla());
            showMessage(hwndDlg, result);
            return TRUE;
        }
        case IDC_SEARCH_REPLACEALL:
        {
            auto result = SearchRequest::exec(data.buttonReplaceAll.value, data.context,
                GetDlgItem(hwndDlg, IDC_SEARCH_FINDBOX), GetDlgItem(hwndDlg, IDC_SEARCH_REPLBOX), plugin.currentScintilla());
            showMessage(hwndDlg, result);
            return TRUE;
        }
        case IDC_SEARCH_DOTALL     : data.dotAll      .get(hwndDlg, IDC_SEARCH_DOTALL     ); data.context.clear(); return TRUE;
        case IDC_SEARCH_FREESPACING: data.freeSpacing .get(hwndDlg, IDC_SEARCH_FREESPACING); data.context.clear(); return TRUE;
        case IDC_SEARCH_MATCHCASE  : data.matchCase   .get(hwndDlg, IDC_SEARCH_MATCHCASE  ); data.context.clear(); return TRUE;
        case IDC_SEARCH_UNICODEWORD: data.uniWordBound.get(hwndDlg, IDC_SEARCH_UNICODEWORD); data.context.clear(); return TRUE;
        case IDC_SEARCH_WHOLEWORD  : data.wholeWord   .get(hwndDlg, IDC_SEARCH_WHOLEWORD  ); data.context.clear(); return TRUE;
        case IDC_SEARCH_PLAIN:
        case IDC_SEARCH_BOOST:
        case IDC_SEARCH_ICU:
            data.searchEngine = IsDlgButtonChecked(hwndDlg, IDC_SEARCH_BOOST   ) == BST_CHECKED ? SearchEngine::Boost
                              : IsDlgButtonChecked(hwndDlg, IDC_SEARCH_ICU     ) == BST_CHECKED ? SearchEngine::ICU
                                                                                                : SearchEngine::Plain;
            data.context.clear();
            if (data.searchEngine == SearchEngine::Plain) {
                ShowWindow(GetDlgItem(hwndDlg, IDC_SEARCH_DOTALL     ), SW_HIDE);
                ShowWindow(GetDlgItem(hwndDlg, IDC_SEARCH_FREESPACING), SW_HIDE);
                ShowWindow(GetDlgItem(hwndDlg, IDC_SEARCH_UNICODEWORD), SW_HIDE);
                ShowWindow(GetDlgItem(hwndDlg, IDC_SEARCH_WHOLEWORD  ), SW_SHOW);
            }
            else {
                ShowWindow(GetDlgItem(hwndDlg, IDC_SEARCH_DOTALL     ), SW_SHOW);
                ShowWindow(GetDlgItem(hwndDlg, IDC_SEARCH_FREESPACING), SW_SHOW);
                ShowWindow(GetDlgItem(hwndDlg, IDC_SEARCH_UNICODEWORD), SW_SHOW);
                ShowWindow(GetDlgItem(hwndDlg, IDC_SEARCH_WHOLEWORD  ), SW_HIDE);
                EnableWindow(GetDlgItem(hwndDlg, IDC_SEARCH_UNICODEWORD), data.searchEngine == SearchEngine::ICU ? TRUE : FALSE);
                SearchCommand sc(data.buttonFind.value);
                if (sc.extent == SearchCommand::Backward) {
                    sc.extent = SearchCommand::Forward;
                    data.buttonFind = sc;
                    SetDlgItemText(hwndDlg, IDC_SEARCH_FIND, Command_Button(data.buttonFind));
                }
                sc = data.buttonReplace.value;
                if (sc.extent == SearchCommand::Backward) {
                    sc.extent = SearchCommand::Forward;
                    data.buttonReplace = sc;
                    SetDlgItemText(hwndDlg, IDC_SEARCH_REPLACE, Command_Button(data.buttonReplace));
                }
            }
            return TRUE;
        case IDC_SEARCH_TOOLS:
        {
            HMENU pum = CreatePopupMenu();
            AppendMenu(pum, MF_STRING, ToolsCommand::BookmarkWhenMark, L"&Bookmark lines when marking text\tCtrl+B");
            AppendMenu(pum, MF_STRING, ToolsCommand::JumpReplace     , L"&Jump to next match after Replace\tCtrl+J");
            AppendMenu(pum, MF_SEPARATOR, 0, 0);
            AppendMenu(pum, MF_STRING, ToolsCommand::HideAll, L"&Hide All Lines\tCtrl+Shift+Q");
            AppendMenu(pum, MF_STRING, ToolsCommand::ShowAll, L"Show &All Lines\tCtrl+Q");
            AppendMenu(pum, MF_SEPARATOR, 0, 0);
            AppendMenu(pum, MF_STRING, ToolsCommand::SelToMark, L"Add Selection to &Marked Text\tCtrl+M");
            AppendMenu(pum, MF_STRING, ToolsCommand::MarkToSel, L"&Select Marked Text\tCtrl+Shift+S");
            AppendMenu(pum, MF_SEPARATOR, 0, 0);
            AppendMenu(pum, MF_STRING, ToolsCommand::CopyMarked,
                  data.copyMarkedSeparator == CopyMarkedSeparator::None   ? L"&Copy Marked Text with no separators\tCtrl+Shift+C"
                : data.copyMarkedSeparator == CopyMarkedSeparator::Blank  ? L"&Copy Marked Text separated by blanks\tCtrl+Shift+C"
                : data.copyMarkedSeparator == CopyMarkedSeparator::Tab    ? L"&Copy Marked Text separated by tabs\tCtrl+Shift+C"
                : data.copyMarkedSeparator == CopyMarkedSeparator::Line   ? L"&Copy Marked Text separated by line breaks\tCtrl+Shift+C"
                                                                          : L"&Copy Marked Text separated by custom string\tCtrl+Shift+C"
                );
            AppendMenu(pum, MF_STRING, ToolsCommand::CopyMarkedDialog, L"Cop&y Marked Text...\tCtrl+Shift+Y");
            AppendMenu(pum, MF_STRING, ToolsCommand::CopyMarkedMultiple, L"Copy Marked &Text as multiple selections\tCtrl+Shift+M");
            AppendMenu(pum, MF_SEPARATOR, 0, 0);
            AppendMenu(pum, MF_STRING, ToolsCommand::ClearMarks, data.markAlsoBookmarks
                ? L"&Remove marks and bookmarks from active document\tCtrl+Shift+R"
                : L"&Remove marks from active document\tCtrl+Shift+R");
            AppendMenu(pum, MF_STRING, ToolsCommand::ClearMarksMultiple, L"Remove marks from multiple &documents...");
            AppendMenu(pum, MF_SEPARATOR, 0, 0);
            AppendMenu(pum, MF_STRING, ToolsCommand::ClearHitlist, L"C&lear search results list");
            AppendMenu(pum, MF_SEPARATOR, 0, 0);
            AppendMenu(pum, MF_STRING, ToolsCommand::Settings, L"S&ettings...\tCtrl+Shift+E");
            plugin.getScintillaPointers();
            bool hasMarks = false;
            if (sci.IndicatorValueAt(data.indicator, 0)) hasMarks = true;
            else {
                Scintilla::Position p = sci.IndicatorEnd(data.indicator, 0);
                if (p != 0 && p != sci.Length()) hasMarks = true;
            }
            EnableMenuItem(pum, ToolsCommand::ShowAll           , sci.AllLinesVisible() ? MF_GRAYED : MF_ENABLED);
            EnableMenuItem(pum, ToolsCommand::SelToMark         , sci.SelectionEmpty()  ? MF_GRAYED : MF_ENABLED);
            EnableMenuItem(pum, ToolsCommand::MarkToSel         , !hasMarks             ? MF_GRAYED : MF_ENABLED);
            EnableMenuItem(pum, ToolsCommand::CopyMarked        , !hasMarks             ? MF_GRAYED : MF_ENABLED);
            EnableMenuItem(pum, ToolsCommand::CopyMarkedDialog  , !hasMarks             ? MF_GRAYED : MF_ENABLED);
            EnableMenuItem(pum, ToolsCommand::CopyMarkedMultiple, !hasMarks             ? MF_GRAYED : MF_ENABLED);
            EnableMenuItem(pum, ToolsCommand::ClearHitlist      , hitlistEmpty()        ? MF_GRAYED : MF_ENABLED);
            EnableMenuItem(pum, ToolsCommand::HideAll,
                sci.VisibleFromDocLine(sci.LineCount() - 1) == 0 && !sci.LineVisible(0) ? MF_GRAYED : MF_ENABLED);
            EnableMenuItem(pum, ToolsCommand::ClearMarks,
                hasMarks || (data.markAlsoBookmarks && sci.MarkerNext(0, 1 << data.bookMarker) >= 0) ? MF_ENABLED : MF_GRAYED);
            MENUITEMINFO mii;
            mii.cbSize = sizeof mii;
            mii.fMask = MIIM_STATE;
            mii.fState = data.markAlsoBookmarks ? MFS_CHECKED : 0;
            SetMenuItemInfo(pum, ToolsCommand::BookmarkWhenMark, FALSE, &mii);
            mii.fState = SearchCommand(data.buttonReplace).verb == SearchCommand::Replace ? MFS_CHECKED : 0;
            SetMenuItemInfo(pum, ToolsCommand::JumpReplace, FALSE, &mii);
            TPMPARAMS tpmp;
            tpmp.cbSize = sizeof tpmp;
            GetWindowRect(reinterpret_cast<HWND>(lParam), &tpmp.rcExclude);
            int choice = TrackPopupMenuEx(pum, TPM_LEFTALIGN | TPM_TOPALIGN | TPM_NONOTIFY | TPM_RETURNCMD | TPM_VERTICAL,
                                          tpmp.rcExclude.left, tpmp.rcExclude.bottom, hwndDlg, &tpmp);
            DestroyMenu(pum);
            processToolsCommand(static_cast<unsigned char>(choice));
            return TRUE;
        }
        }

        return FALSE;

    case WM_SHOWWINDOW:
        if (!wParam && !lParam && data.dialogLayout != DialogLayout::Docking && data.autoClearMarks) {
            plugin.getScintillaPointers();
            sci.SetIndicatorCurrent(data.indicator);
            sci.IndicatorClearRange(0, sci.Length());
            if (data.markAlsoBookmarks) sci.MarkerDeleteAll(data.bookMarker);
        }
        return FALSE;

    case WM_CONTEXTMENU:
    {
        POINT screenLocation = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
        HWND target = reinterpret_cast<HWND>(wParam);
        HWND hFindBox = GetDlgItem(hwndDlg, IDC_SEARCH_FINDBOX);
        HWND hReplBox = GetDlgItem(hwndDlg, IDC_SEARCH_REPLBOX);
        if (target != hFindBox && target != hReplBox) return FALSE;
        plugin.getScintillaPointers();
        bool documentHasSelection = !sci.SelectionEmpty();
        plugin.getScintillaPointers(target);
        if (screenLocation.x == -1 && screenLocation.y == -1) /* invoked from keyboard, not mouse */ {
            Scintilla::Position caret = sci.CurrentPos();
            screenLocation.x = sci.PointXFromPosition(caret);
            screenLocation.y = sci.PointYFromPosition(caret);
            MapWindowPoints(target, 0, &screenLocation, 1);
        }
        bool hasSelection = !sci.SelectionEmpty();
        int zoom = sci.Zoom();
        std::wstring zoomText = (zoom > 0 ? L"&Zoom (+" : L"&Zoom (") + std::to_wstring(zoom) + L")";
        HMENU menu = GetSubMenu(LoadMenu(plugin.dllInstance, MAKEINTRESOURCE(IDR_SEARCH_CONTEXT)), 0);
        MENUITEMINFO mii;
        mii.cbSize     = sizeof mii;
        mii.fMask      = MIIM_STRING;
        mii.dwTypeData = zoomText.data();
        SetMenuItemInfo(menu, 10, TRUE, &mii);
        mii.fMask = MIIM_FTYPE | MIIM_STATE;
        mii.fType = MFT_RADIOCHECK;
        mii.fState = sci.WrapMode() == Scintilla::Wrap::None ? MFS_CHECKED : 0;
        SetMenuItemInfo(menu, ID_SCMSCI_WRAPNONE, FALSE, &mii);
        mii.fState = sci.WrapMode() == Scintilla::Wrap::Char ? MFS_CHECKED : 0;
        SetMenuItemInfo(menu, ID_SCMSCI_WRAPCHAR, FALSE, &mii);
        mii.fState = sci.WrapMode() == Scintilla::Wrap::Word ? MFS_CHECKED : 0;
        SetMenuItemInfo(menu, ID_SCMSCI_WRAPWORD, FALSE, &mii);
        EnableMenuItem(menu, ID_SCMSCI_UNDO           , sci.CanUndo()        ? MF_ENABLED : MF_GRAYED);
        EnableMenuItem(menu, ID_SCMSCI_REDO           , sci.CanRedo()        ? MF_ENABLED : MF_GRAYED);
        EnableMenuItem(menu, ID_SCMSCI_CUT            , hasSelection         ? MF_ENABLED : MF_GRAYED);
        EnableMenuItem(menu, ID_SCMSCI_COPY           , hasSelection         ? MF_ENABLED : MF_GRAYED);
        EnableMenuItem(menu, ID_SCMSCI_PASTE          , sci.CanPaste()       ? MF_ENABLED : MF_GRAYED);
        EnableMenuItem(menu, ID_SCMSCI_DELETE         , hasSelection         ? MF_ENABLED : MF_GRAYED);
        EnableMenuItem(menu, ID_SCMSCI_INSERTSELECTION, documentHasSelection ? MF_ENABLED : MF_GRAYED);
        EnableMenuItem(menu, ID_SCMSCI_ZOOMIN         , zoom < 60            ? MF_ENABLED : MF_GRAYED);
        EnableMenuItem(menu, ID_SCMSCI_ZOOMOUT        , zoom > -10           ? MF_ENABLED : MF_GRAYED);
        EnableMenuItem(menu, ID_SCMSCI_ZOOMDEFAULT    , zoom != 0            ? MF_ENABLED : MF_GRAYED);
        std::vector<std::wstring> historyList;
        const auto& history = target == hFindBox ? data.historyFind.history : data.historyRepl.history;
        if (!history.empty() && !(history.size() == 1 && history.back().empty())) {
            AppendMenu(menu, MF_SEPARATOR, 0, 0);
            int menuCounter = 5000;
            for (const std::wstring& item : history) {
                if (item.empty()) {
                    historyList.push_back(L"");
                    ++menuCounter;
                    continue;
                }
                constexpr size_t limit_width = 60;
                constexpr size_t trail_width = 12;
                std::wstring text;
                for (size_t i = 0; i < item.length(); ++i) {
                    switch (item[i]) {
                    case L'\t':
                        text += L"\u2B72";
                        break;
                    case L'\n':
                        text += L"\u240A";
                        break;
                    case L'\r':
                        if (i + 1 < item.length() && item[i + 1] == L'\n') {
                            text += L"\u21A9";
                            ++i;
                        }
                        else text += L"\u240D";
                        break;
                    case L'&':
                        text += L"&&";
                        break;
                    default:
                        text += item[i];
                    }
                }
                if (text.back() == L' ') /* trailing invisible blanks might be confusing */ {
                    size_t p = text.find_last_not_of(L' ');
                    if (p == std::wstring::npos) p = 0;
                    for (; p < text.length(); ++p) text[p] = L'·';
                }
                if (text.length() > limit_width)
                    text = text.substr(0, limit_width - trail_width - 1) + L'…' + text.substr(text.length() - trail_width);
                historyList.push_back(text);
                AppendMenu(menu, MF_STRING, ++menuCounter, text.data());
            }
        }
        int result = TrackPopupMenu(menu, TPM_NONOTIFY | TPM_RETURNCMD,
                                    screenLocation.x, screenLocation.y, 0, target, 0);
        DestroyMenu(menu);
        switch (result) {
        case ID_SCMSCI_UNDO       : sci.Undo     (); break;
        case ID_SCMSCI_REDO       : sci.Redo     (); break;
        case ID_SCMSCI_CUT        : sci.Cut      (); break;
        case ID_SCMSCI_COPY       : sci.Copy     (); break;
        case ID_SCMSCI_PASTE      : sci.Paste    (); break;
        case ID_SCMSCI_DELETE     : sci.Clear    (); break;
        case ID_SCMSCI_SELECTALL  : sci.SelectAll(); break;
        case ID_SCMSCI_ZOOMIN     : sci.ZoomIn   (); break;
        case ID_SCMSCI_ZOOMOUT    : sci.ZoomOut  (); break;
        case ID_SCMSCI_ZOOMDEFAULT: sci.SetZoom (0); break;
        case ID_SCMSCI_WRAPNONE   : sci.SetWrapMode((target == hFindBox ? data.wrapFind : data.wrapRepl) = Scintilla::Wrap::None); break;
        case ID_SCMSCI_WRAPCHAR   : sci.SetWrapMode((target == hFindBox ? data.wrapFind : data.wrapRepl) = Scintilla::Wrap::Char); break;
        case ID_SCMSCI_WRAPWORD   : sci.SetWrapMode((target == hFindBox ? data.wrapFind : data.wrapRepl) = Scintilla::Wrap::Word); break;

        case ID_SCMSCI_INSERTSELECTION:
        {
            plugin.getScintillaPointers();
            std::string text = sci.GetSelText();
            plugin.getScintillaPointers(target);
            sci.TargetFromSelection();
            sci.ReplaceTarget(text);
            break;
        }
        case ID_SCMSCI_COPYTOREPLACE:
        {
            plugin.getScintillaPointers(hFindBox);
            std::string text = sci.GetText(sci.Length());
            plugin.getScintillaPointers(hReplBox);
            if (target == hReplBox) sci.TargetFromSelection();
                               else sci.TargetWholeDocument();
            sci.ReplaceTarget(text);
            break;
        }
        case ID_SCMSCI_COPYTOFIND:
        {
            plugin.getScintillaPointers(hReplBox);
            std::string text = sci.GetText(sci.Length());
            plugin.getScintillaPointers(hFindBox);
            if (target == hFindBox) sci.TargetFromSelection();
                               else sci.TargetWholeDocument();
            sci.ReplaceTarget(text);
            break;
        }
        case ID_SCMSCI_EXCHANGE:
        {
            plugin.getScintillaPointers(hReplBox);
            std::string replText = sci.GetText(sci.Length());
            plugin.getScintillaPointers(hFindBox);
            std::string findText = sci.GetText(sci.Length());
            sci.TargetWholeDocument();
            sci.ReplaceTarget(replText);
            plugin.getScintillaPointers(hReplBox);
            sci.TargetWholeDocument();
            sci.ReplaceTarget(findText);
            break;
        }
        default:
            if (result > 5000 && result <= 5000 + static_cast<int>(history.size())) {
                std::string item = utf16to8(history[result - 5001]).data();
                sci.TargetWholeDocument();
                sci.ReplaceTarget(item);
            }
        }
        return TRUE;
    }

    case WM_NOTIFY:

        if (reinterpret_cast<NMHDR*>(lParam)->hwndFrom == plugin.nppData._nppHandle) {
            switch (reinterpret_cast<NMHDR*>(lParam)->code & 0xFFFF) {
            case DMN_DOCK:
                data.dockingDialogIsDocked = true;
                break;
            case DMN_FLOAT:
                data.dockingDialogIsDocked = false;
                break;
            }
            break;
        }

        switch (reinterpret_cast<NMHDR*>(lParam)->code) {

        case BCN_DROPDOWN:
        {
            const NMBCDROPDOWN& bd = *reinterpret_cast<NMBCDROPDOWN*>(lParam);
            TPMPARAMS tpmp;
            tpmp.cbSize = sizeof tpmp;
            tpmp.rcExclude = bd.rcButton;
            ClientToScreen(bd.hdr.hwndFrom, reinterpret_cast<POINT*>(&tpmp.rcExclude.left));
            ClientToScreen(bd.hdr.hwndFrom, reinterpret_cast<POINT*>(&tpmp.rcExclude.right));
            HMENU pum = 0;
            config<unsigned int>* searchButton;
            switch (bd.hdr.idFrom) {
            case IDC_SEARCH_FIND:
            {
                searchButton = &data.buttonFind;
                pum = CreatePopupMenu();
                addButtonItem(pum, SearchCommand::Find);
                if (data.searchEngine == SearchEngine::Plain)
                    addButtonItem(pum, SearchCommand(SearchCommand::Find, SearchCommand::Backward));
                AppendMenu(pum, MF_SEPARATOR, 0, 0);
                addButtonItem(pum, SearchCommand(SearchCommand::Find, SearchCommand::Forward, SearchCommand::Selection));
                addButtonItem(pum, SearchCommand(SearchCommand::Find, SearchCommand::Forward, SearchCommand::Region   ));
                addButtonItem(pum, SearchCommand(SearchCommand::Find, SearchCommand::Forward, SearchCommand::Whole    ));
                if (data.searchEngine == SearchEngine::Plain) {
                    AppendMenu(pum, MF_SEPARATOR, 0, 0);
                    addButtonItem(pum, SearchCommand(SearchCommand::Find, SearchCommand::Backward, SearchCommand::Selection));
                    addButtonItem(pum, SearchCommand(SearchCommand::Find, SearchCommand::Backward, SearchCommand::Region   ));
                    addButtonItem(pum, SearchCommand(SearchCommand::Find, SearchCommand::Backward, SearchCommand::Whole    ));
                }
                break;
            }
            case IDC_SEARCH_COUNT:
            {
                searchButton = &data.buttonCount;
                pum = CreatePopupMenu();
                addButtonItem(pum, SearchCommand(SearchCommand::Count, SearchCommand::All   ));
                AppendMenu(pum, MF_SEPARATOR, 0, 0);
                addButtonItem(pum, SearchCommand(SearchCommand::Count, SearchCommand::All, SearchCommand::Selection));
                addButtonItem(pum, SearchCommand(SearchCommand::Count, SearchCommand::Open));
                addButtonItem(pum, SearchCommand(SearchCommand::Count, SearchCommand::View));
                AppendMenu(pum, MF_SEPARATOR, 0, 0);
                addButtonItem(pum, SearchCommand(SearchCommand::Count, SearchCommand::All   , SearchCommand::Region));
                addButtonItem(pum, SearchCommand(SearchCommand::Count, SearchCommand::Before, SearchCommand::Region));
                addButtonItem(pum, SearchCommand(SearchCommand::Count, SearchCommand::After , SearchCommand::Region));
                addButtonItem(pum, SearchCommand(SearchCommand::Count, SearchCommand::Open  , SearchCommand::Region));
                addButtonItem(pum, SearchCommand(SearchCommand::Count, SearchCommand::View  , SearchCommand::Region));
                AppendMenu(pum, MF_SEPARATOR, 0, 0);
                addButtonItem(pum, SearchCommand(SearchCommand::Count, SearchCommand::All   , SearchCommand::Whole));
                addButtonItem(pum, SearchCommand(SearchCommand::Count, SearchCommand::Before, SearchCommand::Whole));
                addButtonItem(pum, SearchCommand(SearchCommand::Count, SearchCommand::After , SearchCommand::Whole));
                break;
            }
            case IDC_SEARCH_FINDALL:
            {
                searchButton = &data.buttonFindAll;
                pum = CreatePopupMenu();

                HMENU subFind = CreatePopupMenu();
                addButtonItem(subFind, SearchCommand(SearchCommand::FindAll, SearchCommand::All, SearchCommand::Selection));
                AppendMenu(subFind, MF_SEPARATOR, 0, 0);
                addButtonItem(subFind, SearchCommand(SearchCommand::FindAll, SearchCommand::All   , SearchCommand::Region));
                addButtonItem(subFind, SearchCommand(SearchCommand::FindAll, SearchCommand::Before, SearchCommand::Region));
                addButtonItem(subFind, SearchCommand(SearchCommand::FindAll, SearchCommand::After , SearchCommand::Region));
                addButtonItem(subFind, SearchCommand(SearchCommand::FindAll, SearchCommand::Open  , SearchCommand::Region));
                addButtonItem(subFind, SearchCommand(SearchCommand::FindAll, SearchCommand::View  , SearchCommand::Region));
                AppendMenu(subFind, MF_SEPARATOR, 0, 0);
                addButtonItem(subFind, SearchCommand(SearchCommand::FindAll, SearchCommand::All   , SearchCommand::Whole));
                addButtonItem(subFind, SearchCommand(SearchCommand::FindAll, SearchCommand::Before, SearchCommand::Whole));
                addButtonItem(subFind, SearchCommand(SearchCommand::FindAll, SearchCommand::After , SearchCommand::Whole));

                HMENU subMark = CreatePopupMenu();
                addButtonItem(subMark, SearchCommand(SearchCommand::Mark, SearchCommand::All, SearchCommand::Selection));
                addButtonItem(subMark, SearchCommand(SearchCommand::Mark, SearchCommand::Open));
                addButtonItem(subMark, SearchCommand(SearchCommand::Mark, SearchCommand::View));
                AppendMenu(subMark, MF_SEPARATOR, 0, 0);
                addButtonItem(subMark, SearchCommand(SearchCommand::Mark, SearchCommand::All   , SearchCommand::Region));
                addButtonItem(subMark, SearchCommand(SearchCommand::Mark, SearchCommand::Before, SearchCommand::Region));
                addButtonItem(subMark, SearchCommand(SearchCommand::Mark, SearchCommand::After , SearchCommand::Region));
                addButtonItem(subMark, SearchCommand(SearchCommand::Mark, SearchCommand::Open  , SearchCommand::Region));
                addButtonItem(subMark, SearchCommand(SearchCommand::Mark, SearchCommand::View  , SearchCommand::Region));
                AppendMenu(subMark, MF_SEPARATOR, 0, 0);
                addButtonItem(subMark, SearchCommand(SearchCommand::Mark, SearchCommand::All   , SearchCommand::Whole));
                addButtonItem(subMark, SearchCommand(SearchCommand::Mark, SearchCommand::Before, SearchCommand::Whole));
                addButtonItem(subMark, SearchCommand(SearchCommand::Mark, SearchCommand::After , SearchCommand::Whole));

                HMENU subSlct = CreatePopupMenu();
                addButtonItem(subSlct, SearchCommand(SearchCommand::Select, SearchCommand::All, SearchCommand::Selection));
                AppendMenu(subSlct, MF_SEPARATOR, 0, 0);
                addButtonItem(subSlct, SearchCommand(SearchCommand::Select, SearchCommand::All   , SearchCommand::Region));
                addButtonItem(subSlct, SearchCommand(SearchCommand::Select, SearchCommand::Before, SearchCommand::Region));
                addButtonItem(subSlct, SearchCommand(SearchCommand::Select, SearchCommand::After , SearchCommand::Region));
                AppendMenu(subSlct, MF_SEPARATOR, 0, 0);
                addButtonItem(subSlct, SearchCommand(SearchCommand::Select, SearchCommand::All   , SearchCommand::Whole));
                addButtonItem(subSlct, SearchCommand(SearchCommand::Select, SearchCommand::Before, SearchCommand::Whole));
                addButtonItem(subSlct, SearchCommand(SearchCommand::Select, SearchCommand::After , SearchCommand::Whole));

                HMENU subShow = CreatePopupMenu();
                addButtonItem(subShow, SearchCommand(SearchCommand::Show, SearchCommand::All, SearchCommand::Selection));
                AppendMenu(subShow, MF_SEPARATOR, 0, 0);
                addButtonItem(subShow, SearchCommand(SearchCommand::Show, SearchCommand::All   , SearchCommand::Region));
                addButtonItem(subShow, SearchCommand(SearchCommand::Show, SearchCommand::Before, SearchCommand::Region));
                addButtonItem(subShow, SearchCommand(SearchCommand::Show, SearchCommand::After , SearchCommand::Region));
                AppendMenu(subShow, MF_SEPARATOR, 0, 0);
                addButtonItem(subShow, SearchCommand(SearchCommand::Show, SearchCommand::All   , SearchCommand::Whole));
                addButtonItem(subShow, SearchCommand(SearchCommand::Show, SearchCommand::Before, SearchCommand::Whole));
                addButtonItem(subShow, SearchCommand(SearchCommand::Show, SearchCommand::After , SearchCommand::Whole));

                addButtonItem(pum, SearchCommand(SearchCommand::FindAll, SearchCommand::All));
                addButtonItem(pum, SearchCommand(SearchCommand::Select , SearchCommand::All));
                addButtonItem(pum, SearchCommand(SearchCommand::Mark   , SearchCommand::All));
                addButtonItem(pum, SearchCommand(SearchCommand::Show   , SearchCommand::All));
                AppendMenu(pum, MF_SEPARATOR, 0, 0);
                addButtonItem(pum, SearchCommand(SearchCommand::FindAll, SearchCommand::Open));
                addButtonItem(pum, SearchCommand(SearchCommand::FindAll, SearchCommand::View));
                AppendMenu(pum, MF_SEPARATOR, 0, 0);
                AppendMenu(pum, MF_POPUP | MF_STRING, reinterpret_cast<UINT_PTR>(subFind), L"Find &All");
                AppendMenu(pum, MF_POPUP | MF_STRING, reinterpret_cast<UINT_PTR>(subSlct), L"Selec&t");
                AppendMenu(pum, MF_POPUP | MF_STRING, reinterpret_cast<UINT_PTR>(subMark), L"Mar&k");
                AppendMenu(pum, MF_POPUP | MF_STRING, reinterpret_cast<UINT_PTR>(subShow), L"Sho&w");
                break;
            }
            case IDC_SEARCH_REPLACE:
            {
                searchButton = &data.buttonReplace;
                HMENU thenFind = CreatePopupMenu();
                addButtonItem(thenFind, SearchCommand::Replace );
                if (data.searchEngine == SearchEngine::Plain) {
                    addButtonItem(thenFind, SearchCommand(SearchCommand::Replace , SearchCommand::Backward));
                }
                AppendMenu(thenFind, MF_SEPARATOR, 0, 0);
                addButtonItem(thenFind, SearchCommand(SearchCommand::Replace, SearchCommand::Forward, SearchCommand::Selection));
                addButtonItem(thenFind, SearchCommand(SearchCommand::Replace, SearchCommand::Forward, SearchCommand::Region   ));
                addButtonItem(thenFind, SearchCommand(SearchCommand::Replace, SearchCommand::Forward, SearchCommand::Whole    ));
                if (data.searchEngine == SearchEngine::Plain) {
                    AppendMenu(thenFind, MF_SEPARATOR, 0, 0);
                    addButtonItem(thenFind, SearchCommand(SearchCommand::Replace, SearchCommand::Backward, SearchCommand::Selection));
                    addButtonItem(thenFind, SearchCommand(SearchCommand::Replace, SearchCommand::Backward, SearchCommand::Region   ));
                    addButtonItem(thenFind, SearchCommand(SearchCommand::Replace, SearchCommand::Backward, SearchCommand::Whole    ));
                }
                HMENU thenStop = CreatePopupMenu();
                addButtonItem(thenStop, SearchCommand::ReplStop);
                if (data.searchEngine == SearchEngine::Plain) {
                    addButtonItem(thenStop, SearchCommand(SearchCommand::ReplStop, SearchCommand::Backward));
                }
                AppendMenu(thenStop, MF_SEPARATOR, 0, 0);
                addButtonItem(thenStop, SearchCommand(SearchCommand::ReplStop, SearchCommand::Forward, SearchCommand::Selection));
                addButtonItem(thenStop, SearchCommand(SearchCommand::ReplStop, SearchCommand::Forward, SearchCommand::Region   ));
                addButtonItem(thenStop, SearchCommand(SearchCommand::ReplStop, SearchCommand::Forward, SearchCommand::Whole    ));
                if (data.searchEngine == SearchEngine::Plain) {
                    AppendMenu(thenStop, MF_SEPARATOR, 0, 0);
                    addButtonItem(thenStop, SearchCommand(SearchCommand::ReplStop, SearchCommand::Backward, SearchCommand::Selection));
                    addButtonItem(thenStop, SearchCommand(SearchCommand::ReplStop, SearchCommand::Backward, SearchCommand::Region   ));
                    addButtonItem(thenStop, SearchCommand(SearchCommand::ReplStop, SearchCommand::Backward, SearchCommand::Whole    ));
                }
                if (SearchCommand(data.buttonReplace) == SearchCommand::Replace) {
                    pum = thenFind;
                    AppendMenu(pum, MF_SEPARATOR, 0, 0);
                    AppendMenu(pum, MF_POPUP | MF_STRING, reinterpret_cast<UINT_PTR>(thenStop), L"Do not &jump to next match");
                }
                else {
                    pum = thenStop;
                    AppendMenu(pum, MF_SEPARATOR, 0, 0);
                    AppendMenu(pum, MF_POPUP | MF_STRING, reinterpret_cast<UINT_PTR>(thenFind), L"&Jump to next match");
                }
                break;
            }
            case IDC_SEARCH_REPLACEALL:
            {
                searchButton = &data.buttonReplaceAll;
                pum = CreatePopupMenu();
                addButtonItem(pum, SearchCommand(SearchCommand::ReplaceAll, SearchCommand::All   ));
                AppendMenu(pum, MF_SEPARATOR, 0, 0);
                addButtonItem(pum, SearchCommand(SearchCommand::ReplaceAll, SearchCommand::All, SearchCommand::Selection));
                addButtonItem(pum, SearchCommand(SearchCommand::ReplaceAll, SearchCommand::Open));
                addButtonItem(pum, SearchCommand(SearchCommand::ReplaceAll, SearchCommand::View));
                AppendMenu(pum, MF_SEPARATOR, 0, 0);
                addButtonItem(pum, SearchCommand(SearchCommand::ReplaceAll, SearchCommand::All   , SearchCommand::Region));
                addButtonItem(pum, SearchCommand(SearchCommand::ReplaceAll, SearchCommand::Before, SearchCommand::Region));
                addButtonItem(pum, SearchCommand(SearchCommand::ReplaceAll, SearchCommand::After , SearchCommand::Region));
                addButtonItem(pum, SearchCommand(SearchCommand::ReplaceAll, SearchCommand::Open  , SearchCommand::Region));
                addButtonItem(pum, SearchCommand(SearchCommand::ReplaceAll, SearchCommand::View  , SearchCommand::Region));
                AppendMenu(pum, MF_SEPARATOR, 0, 0);
                addButtonItem(pum, SearchCommand(SearchCommand::ReplaceAll, SearchCommand::All   , SearchCommand::Whole));
                addButtonItem(pum, SearchCommand(SearchCommand::ReplaceAll, SearchCommand::Before, SearchCommand::Whole));
                addButtonItem(pum, SearchCommand(SearchCommand::ReplaceAll, SearchCommand::After , SearchCommand::Whole));
                break;
            }
            default:
                return FALSE;
            }
            int choice = TrackPopupMenuEx(pum, TPM_LEFTALIGN | TPM_TOPALIGN | TPM_NONOTIFY | TPM_RETURNCMD | TPM_VERTICAL,
                                          tpmp.rcExclude.left, tpmp.rcExclude.bottom, hwndDlg, &tpmp);
            bool shifted = GetAsyncKeyState(VK_SHIFT) < 0;
            DestroyMenu(pum);
            if (choice) {
                if (shifted) {
                    *searchButton = choice;
                    SetDlgItemText(hwndDlg, static_cast<int>(bd.hdr.idFrom), Command_Button(choice));
                }
                auto result = SearchRequest::exec(choice, data.context,
                    GetDlgItem(hwndDlg, IDC_SEARCH_FINDBOX), GetDlgItem(hwndDlg, IDC_SEARCH_REPLBOX), plugin.currentScintilla());
                showMessage(hwndDlg, result);
            }
            return TRUE;
        }

        case TTN_GETDISPINFO:
        {
            static std::wstring tipText;
            NMTTDISPINFO& di = *reinterpret_cast<LPNMTTDISPINFO>(lParam);
            di.hinst = 0;
            SIZE textSize;
            RECT messageRect;
            HWND hMsg = GetDlgItem(hwndDlg, IDC_SEARCH_MESSAGE);
            GetClientRect(hMsg, &messageRect);
            tipText = GetWindowString(hMsg);
            HDC hdc = GetDC(hMsg);
            HFONT hFont = reinterpret_cast<HFONT>(SendMessage(hMsg, WM_GETFONT, 0, 0));
            HFONT hOldFont = (HFONT)SelectObject(hdc, hFont);
            GetTextExtentPoint32(hdc, tipText.data(), static_cast<int>(tipText.length()), &textSize);
            SelectObject(hdc, hOldFont);
            ReleaseDC(hMsg, hdc);
            di.lpszText = textSize.cx > (messageRect.right - messageRect.left) ? tipText.data() : 0;
            break;
        }

        case SCN_MODIFIED:
        {
            const Scintilla::NotificationData& scn = *reinterpret_cast<Scintilla::NotificationData*>(lParam);
            if (scn.nmhdr.idFrom == IDC_SEARCH_FINDBOX) data.context.clear();
            if (scn.nmhdr.idFrom == IDC_SEARCH_REPLBOX) data.context.calcIsValid = false;
            return TRUE;
        }

        case SCN_ZOOM:
        {
            const Scintilla::NotificationData& scn = *reinterpret_cast<Scintilla::NotificationData*>(lParam);
            plugin.getScintillaPointers(reinterpret_cast<HWND>(scn.nmhdr.hwndFrom));
            (scn.nmhdr.idFrom == IDC_SEARCH_FINDBOX ? data.zoomFind : data.zoomRepl) = sci.Zoom();
            return TRUE;
        }
        
        }
        return FALSE;

    case WM_GETMINMAXINFO:
    {
        MINMAXINFO& mmi = *reinterpret_cast<MINMAXINFO*>(lParam);
        RECT window, client;
        GetWindowRect(hwndDlg, &window);
        GetClientRect(hwndDlg, &client);
        switch (data.dialogLayout) {
        case DialogLayout::Horizontal:
            mmi.ptMinTrackSize.x = hClient.right;
            mmi.ptMinTrackSize.y = hClient.bottom;
            break;
        case DialogLayout::Vertical:
            mmi.ptMinTrackSize.x = vClient.right;
            mmi.ptMinTrackSize.y = vClient.bottom;
            break;
        default:
            mmi.ptMinTrackSize.x = client.bottom < hClient.bottom ? wClient.right
                                 : client.bottom < vClient.bottom ? hClient.right
                                                                  : vClient.right;
            mmi.ptMinTrackSize.y = client.right  < hClient.right  ? vClient.bottom
                                 : client.right  < wClient.right  ? hClient.bottom
                                                                  : wClient.bottom;
        }
        mmi.ptMinTrackSize.x += window.right  - window.left - client.right;
        mmi.ptMinTrackSize.y += window.bottom - window.top  - client.bottom;
        return FALSE;
    }

    case WM_SIZE:
        layoutDialog(hwndDlg);
        return FALSE;

    }

    return FALSE;
}

}


void colorSearch() {
    if (data.searchDialog) {
        constexpr ULONG dmfSetThemeDirectly = 0x00000010UL;
        HWND findBox = GetDlgItem(data.searchDialog, IDC_SEARCH_FINDBOX);
        HWND replBox = GetDlgItem(data.searchDialog, IDC_SEARCH_REPLBOX);
        npp(NPPM_DARKMODESUBCLASSANDTHEME, dmfSetThemeDirectly, findBox);
        npp(NPPM_DARKMODESUBCLASSANDTHEME, dmfSetThemeDirectly, replBox);
        configureSearchBox(findBox);
        configureSearchBox(replBox);
    }
}


void changeDialogLayout() {
    if (data.dialogLayout == DialogLayout::Docking) {
        if (data.searchDialog) SendMessage(data.searchDialog, WM_COMMAND, IDCANCEL, 0);
        data.searchDialog = data.dockingDialog;
        if (data.searchDialog) synchronizeDialogItems(data.searchDialog);
    }
    else if (data.searchDialog == data.dockingDialog) {
        if (data.searchDialog) SendMessage(data.searchDialog, WM_COMMAND, IDCANCEL, 0);
        data.searchDialog = data.regularDialog;
        if (data.searchDialog) synchronizeDialogItems(data.searchDialog);
    }
    if (!data.searchDialog) return;
    if (data.dialogLayout != DialogLayout::Docking) {
        RECT window, client;
        GetWindowRect(data.searchDialog, &window);
        GetClientRect(data.searchDialog, &client);
        int minWidth, minHeight;
        switch (data.dialogLayout) {
        case DialogLayout::Horizontal:
            minWidth  = hClient.right;
            minHeight = hClient.bottom;
            break;
        case DialogLayout::Vertical:
            minWidth  = vClient.right;
            minHeight = vClient.bottom;
            break;
        default:
            minWidth  = vClient.right;
            minHeight = hClient.bottom;
        }
        if (client.right - client.left < minWidth && client.bottom - client.top < minHeight) {
            minWidth  += window.right - window.left - client.right;
            minHeight += window.bottom - window.top - client.bottom;
            int width  = window.right  - window.left;
            int height = window.bottom - window.top;
            SetWindowPos(data.searchDialog, 0, 0, 0, std::max(width, minWidth), std::max(height, minHeight),
                SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOZORDER);
        }
    }
    layoutDialog(data.searchDialog);
}

void showSearchDialog() {
    bool alreadyVisible = data.searchDialog && IsWindowVisible(data.searchDialog);
    if (data.searchDialog) {
        if (data.dialogLayout == DialogLayout::Docking) npp(NPPM_DMMSHOW, 0, data.searchDialog);
        else {
            ShowWindow(data.searchDialog, SW_NORMAL);
            SetActiveWindow(data.searchDialog);
        }
    }
    else {
        data.historyFind.get();
        data.historyRepl.get();
        if (data.dialogLayout == DialogLayout::Docking) {
            data.searchDialog = CreateDialog(plugin.dllInstance, MAKEINTRESOURCE(IDD_DOCKINGSEARCH), plugin.nppData._nppHandle, searchDialogProc);
            NPP::tTbData dock;
            dock.hClient       = data.searchDialog;
            dock.pszName       = L"Search++";             // title bar text (caption in dialog is replaced)
            dock.dlgID         = 0;                       // zero-based position in menu to recall dialog at next startup
            dock.uMask         = DWS_DF_CONT_RIGHT;       // first time display will be docked at the right
            dock.pszModuleName = L"Search++.dll";         // plugin module name
            npp(NPPM_DMMREGASDCKDLG, 0, &dock);
            data.dockingDialog = data.searchDialog;
        }
        else {
            data.searchDialog = CreateDialog(plugin.dllInstance, MAKEINTRESOURCE(IDD_SEARCH), plugin.nppData._nppHandle, searchDialogProc);
            ShowWindow(data.searchDialog, SW_NORMAL);
            data.regularDialog = data.searchDialog;
        }
    }
    if (data.fillSearch && (data.fillVisible || !alreadyVisible)) {
        plugin.getScintillaPointers();
        if (sci.Selections() == 1) {
            std::string text;
            if (sci.SelectionEmpty()) {
                if (data.fillNothing) {
                    Scintilla::Position p = sci.CurrentPos();
                    sci.MultipleSelectAddNext();
                    sci.TargetFromSelection();
                    text = sci.TargetAsUTF8();
                    sci.SetAnchor(p);
                    sci.SetCurrentPos(p);
                }
            }
            else if (!data.fillSearchLimit) {
                sci.TargetFromSelection();
                text = sci.TargetAsUTF8();
            }
            else {
                Scintilla::Position start = sci.SelectionStart();
                Scintilla::Position end = sci.SelectionEnd();
                Scintilla::Position startLine = sci.LineFromPosition(start);
                Scintilla::Position endLine = sci.LineFromPosition(end);
                if (endLine - startLine < data.fillLines && end - start <= 4 * data.fillChars) {
                    if (end - start <= data.fillChars || sci.CountCharacters(start, end) <= data.selChars) {
                        sci.TargetFromSelection();
                        text = sci.TargetAsUTF8();
                    }
                }
            }
            if (!text.empty()) {
                plugin.getScintillaPointers(GetDlgItem(data.searchDialog, IDC_SEARCH_FINDBOX));
                sci.TargetWholeDocument();
                sci.ReplaceTarget(text);
                sci.SelectAll();
            }
        }
    }
}

void destroySearchDialogs() {
    if (data.searchDialog) {
        plugin.getScintillaPointers(GetDlgItem(data.searchDialog, IDC_SEARCH_FINDBOX));
        sci.TargetWholeDocument();
        data.findBoxLast = sci.TargetText();
        plugin.getScintillaPointers(GetDlgItem(data.searchDialog, IDC_SEARCH_REPLBOX));
        sci.TargetWholeDocument();
        data.replBoxLast = sci.TargetText();
        if (data.dockingDialog) {
            npp(NPPM_MODELESSDIALOG, MODELESSDIALOGREMOVE, data.dockingDialog);
            data.dockingDialog = 0;
        }
        if (data.regularDialog) {
            placement.get(data.regularDialog);
            npp(NPPM_MODELESSDIALOG, MODELESSDIALOGREMOVE, data.regularDialog);
            DestroyWindow(data.regularDialog);
            data.regularDialog = 0;
        }
        data.searchDialog = 0;
    }
}
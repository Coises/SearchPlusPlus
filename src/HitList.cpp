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
#include "MatchResults.h"
#include "ScintillaControl.h"
#include "ProgressInfo.h"

#include "resource.h"
#include "Shlwapi.h"
#include <windowsx.h>

void closeSearchInFilesDialog();
void scrollIntoView(HWND scintilla, HWND avoid, Scintilla::Position foundStart, Scintilla::Position foundEnd, bool select);
void showSearchDialog();
void showSearchInFilesDialog();


namespace {

std::vector<MatchResults::LineIndex> cumulativeLineIndex;

std::locale userLocale("");

HWND hitlist = 0;  // Window handle to the hit list dialog

ScintillaControl sciHits = { "results list", true, 0 };

constexpr int SCIHITS = 101;  // Dialog ID of the Scintilla control within the hit list dialog

constexpr int Style_Search   = 1;
constexpr int Style_Document = 2;
constexpr int Style_Found    = 3;

constexpr int Marker_Search   = 1;
constexpr int Marker_Document = 2;

constexpr int Indicator_Found     = 8;
constexpr int Indicator_NullMatch = 9;

constexpr Scintilla::FoldLevel Level_Search   = static_cast<Scintilla::FoldLevel>(static_cast<int>(Scintilla::FoldLevel::HeaderFlag)
                                              | (static_cast<int>(Scintilla::FoldLevel::Base) - 20));
constexpr Scintilla::FoldLevel Level_Document = static_cast<Scintilla::FoldLevel>(static_cast<int>(Scintilla::FoldLevel::HeaderFlag)
                                              | (static_cast<int>(Scintilla::FoldLevel::Base) - 10));

intptr_t maxMarginNumber = 999;                   // Maximum number that can be displayed in margin; always a power of 10 minus 1

POINT lastLButtonDown;                            // Last place the left mouse button went down -- used in double-click processing

bool caretLineIsBackground = false;               // Set when caret line indicator is highlight background; used for header lines


// Handling for double-clicks and the Enter key

bool processDoubleClickOrEnterKey(Scintilla::Position cpMin, Scintilla::Position cpMax, bool switchFocus) {
    const Scintilla::Line lnMin = sciHits.LineFromPosition(cpMin);
    if (lnMin >= static_cast<Scintilla::Line>(cumulativeLineIndex.size())) return false;
    if (Scintilla::LevelIsHeader(sciHits.FoldLevel(lnMin))) /* a search or document header: toggle fold */ {
        sciHits.ToggleFold(lnMin);
        return true;
    }
    const Scintilla::Line parentLine = sciHits.FoldParent(lnMin);
    const std::string parentLineText = sciHits.GetLine(parentLine);
    const size_t fn = parentLineText.find(": ");
    if (fn == std::string::npos || fn >= parentLineText.length() - 4) return false;
    const Scintilla::Line lineNumber = cumulativeLineIndex[lnMin].lineNumber;
    const Scintilla::Position cpLine = sciHits.PositionFromLine(lnMin);
    if (!npp(NPPM_DOOPEN, 0, utf8to16(parentLineText.substr(fn + 2, parentLineText.length() - fn - 4)).data())) return false;
    plugin.getScintillaPointers();
    const UINT codepage = sci.CodePage();
    Scintilla::Position start;
    Scintilla::Position end;
    if (codepage == CP_UTF8) {
        intptr_t offset = sci.PositionFromLine(lineNumber) - cpLine;
        start = cpMin + offset;
        end   = cpMax + offset;
    }
    else {
        start = cpMin == cpLine ? 0 : fromWide(utf8to16(sciHits.StringOfRange(Scintilla::Span(cpLine, cpMin))), codepage).length();
        Scintilla::Position length = cpMax == cpMin ? 0
            : fromWide(utf8to16(sciHits.StringOfRange(Scintilla::Span(cpMin, cpMax))), codepage).length();
        start += sci.PositionFromLine(lineNumber);
        end = start + length;
    }
    data.context.clear();

//
// Following does not always work; occasionally results in an odd crash. Reason not yet determined.
// 
//    HWND curSci = plugin.currentScintilla();
//    UpdateWindow(curSci);             // These lines are needed to ensure that Scintilla calculates layout
//    sci.EnsureVisible(lineNumber);    // at least up to the line containg the start of the match
//    sci.WrapCount(lineNumber);        // so that scrolling will be accurate.
//    scrollIntoView(curSci, data.searchDialog, start, end, true);
//
// Following adapted from Searching::displaySectionCentered in FindReplaceDlg.cpp in Notepad++:

    {
        sci.EnsureVisible(sci.LineFromPosition(start));
        sci.EnsureVisible(sci.LineFromPosition(end));

        // Jump-scroll to center, if current position is out of view
        sci.SetVisiblePolicy(static_cast<Scintilla::VisiblePolicy>(CARET_JUMPS | CARET_EVEN), 0);
        sci.EnsureVisibleEnforcePolicy(sci.LineFromPosition(end));
        sci.GotoPos(end);
        sci.SetVisiblePolicy(static_cast<Scintilla::VisiblePolicy>(CARET_EVEN), 0);
        sci.EnsureVisibleEnforcePolicy(sci.LineFromPosition(end));

        // Adjust so that we see the entire match; primarily horizontally
        sci.ScrollRange(start, end);

        // make sure won't start/end the selection in the middle of a multibyte character,
        // or in between a CR/LF pair for Windows files
        // (needed for stale search-results where user has made doc edits after the search)
        if (start > 0)
        {
            start = sci.PositionBefore(start);
            start = sci.PositionAfter (start);
        }
        if (end > 0)
        {
            end = sci.PositionBefore(end);
            end = sci.PositionAfter (end);
        }

        // Move cursor to end of result and select result
        sci.GotoPos(end);
        sci.SetAnchor(start);

        // Update Scintilla's knowledge about what column the caret is in, so that if user
        // does up/down arrow as first navigation after the search result is selected,
        // the caret doesn't jump to an unexpected column
        sci.ChooseCaretX();
    }

    if (!switchFocus) {
        if (start == end) {
            char c;
            if (zlmIndicator == 0 || start == sci.Length()
                || (!sci.ViewEOL() && ((c = sci.CharacterAt(start)) == '\r' || c == '\n'))) {
                sci.CallTipShow(start, "^ zero length match");
            }
            else {
                sci.IndicSetStyle(zlmIndicator, Scintilla::IndicatorStyle::Point);
                sci.IndicSetFore(zlmIndicator, sci.ElementColour(Scintilla::Element::Caret));
                sci.SetIndicatorCurrent(zlmIndicator);
                sci.SetIndicatorValue(1);
                sci.IndicatorClearRange(0, sci.Length());
                sci.IndicatorFillRange(start, 1);
            }
        }
        npp(NPPM_DMMSHOW, 0, hitlist);
    }
    return true;
}

bool processDoubleClick(POINT click, bool switchFocus = true) {
    Scintilla::Position position1 = sciHits.CharPositionFromPoint(click.x, click.y);
    Scintilla::Position position2 = sciHits.PositionFromPoint(click.x, click.y);
    Scintilla::Position cpMin, cpMax;
    if (sciHits.IndicatorValueAt(Indicator_Found, position1)) {
        cpMin = sciHits.IndicatorStart(Indicator_Found, position1);
        cpMax = sciHits.IndicatorEnd(Indicator_Found, position1);
    }
    else cpMin = cpMax = position2;
    sciHits.SetSel(cpMin, cpMax);
    return processDoubleClickOrEnterKey(cpMin, cpMax, switchFocus);
}

bool processEnterKey(bool switchFocus = true) {
    return processDoubleClickOrEnterKey(sciHits.SelectionStart(), sciHits.SelectionEnd(), switchFocus);
}


// Navigation in search results

void nextMatch() {

    Scintilla::Position length   = sciHits.Length();
    Scintilla::Position oldStart = sciHits.SelectionStart();
    Scintilla::Position oldEnd   = sciHits.SelectionEnd();

    if (oldEnd == length) {
        sciHits.SetSel(length, length);
        return;
    }

    if (oldStart != oldEnd && sciHits.IndicatorValueAt(Indicator_NullMatch, oldEnd)) {
        sciHits.SetSel(oldEnd, oldEnd);
        return;
    }

    Scintilla::Position nextNull = oldEnd + 1;
    if (nextNull >= length) nextNull = length;
    else if (!sciHits.IndicatorValueAt(Indicator_NullMatch, nextNull)) {
        nextNull = sciHits.IndicatorEnd(Indicator_NullMatch, nextNull);
        if (nextNull == 0) nextNull = length;
    }

    Scintilla::Position nextEnd = sciHits.IndicatorEnd(Indicator_Found, oldEnd);
    Scintilla::Position nextStart;
    if (nextEnd == 0) nextStart = nextEnd = length;
    else if (sciHits.IndicatorValueAt(Indicator_Found, nextEnd - 1)) nextStart = sciHits.IndicatorStart(Indicator_Found, nextEnd - 1);
    else {
        nextStart = nextEnd;
        nextEnd = sciHits.IndicatorEnd(Indicator_Found, nextStart);
    }

    if (nextNull <= nextStart) sciHits.SetSel(nextNull, nextNull);
    else {
        sciHits.SetSel(nextStart, nextEnd);
        sciHits.ScrollRange(nextEnd, nextStart);
    }

}

void prevMatch() {

    Scintilla::Position oldStart = sciHits.SelectionStart();
    Scintilla::Position oldEnd   = sciHits.SelectionEnd();

    if (oldStart == 0) {
        sciHits.SetSel(0, 0);
        return;
    }

    if (oldStart != oldEnd && sciHits.IndicatorValueAt(Indicator_NullMatch, oldStart)) {
        sciHits.SetSel(oldStart, oldStart);
        return;
    }

    Scintilla::Position prevNull = oldEnd - 1;
    if (prevNull <= 0) prevNull = 0;
    else if (!sciHits.IndicatorValueAt(Indicator_NullMatch, prevNull)) {
        prevNull = sciHits.IndicatorStart(Indicator_NullMatch, prevNull);
        if (prevNull > 0) --prevNull;
    }

    Scintilla::Position prevStart = sciHits.IndicatorStart(Indicator_Found, oldStart - 1);
    Scintilla::Position prevEnd;
    if (sciHits.IndicatorValueAt(Indicator_Found, prevStart)) prevEnd = sciHits.IndicatorEnd(Indicator_Found, prevStart);
    else if (prevStart == 0) prevStart = prevEnd = 0;
    else {
        prevEnd = prevStart;
        prevStart = sciHits.IndicatorStart(Indicator_Found, prevEnd - 1);
    }

    if (prevNull >= prevStart) sciHits.SetSel(prevNull, prevNull);
    else {
        sciHits.SetSel(prevStart, prevEnd);
        sciHits.ScrollRange(prevEnd, prevStart);
    }

}

void nextSearch() {
    Scintilla::Line line = sciHits.LineFromPosition(sciHits.CurrentPos());
    Scintilla::Line last = sciHits.LastChild(line, Level_Search);
    if (last + 1 < sciHits.LineCount()) {
        sciHits.EnsureVisible(line + 1);
        sciHits.GotoLine(last + 1);
        sciHits.ScrollVertical(last + 1, 0);
    }
}

void prevSearch() {
    Scintilla::Line line = sciHits.LineFromPosition(sciHits.CurrentPos());
    if (line == 0) return;
    while (--line > 0 && sciHits.FoldLevel(line) != Level_Search);
    sciHits.EnsureVisible(line);
    sciHits.GotoLine(line);
    sciHits.ScrollVertical(line, 0);
}

void nextDocument() {
    Scintilla::Line line = sciHits.LineFromPosition(sciHits.CurrentPos());
    Scintilla::Line last = sciHits.LastChild(line, Level_Document);
    if (last + 1 < sciHits.LineCount()) {
        sciHits.EnsureVisible(line + 1);
        sciHits.GotoLine(last + 1);
        sciHits.ScrollVertical(last + 1, 0);
    }
}

void prevDocument() {
    Scintilla::Line line = sciHits.LineFromPosition(sciHits.CurrentPos());
    if (line == 0) return;
    while (--line > 0 && sciHits.FoldLevel(line) == Scintilla::FoldLevel::Base);
    sciHits.EnsureVisible(line);
    sciHits.GotoLine(line);
    sciHits.ScrollVertical(line, 0);
}

void clearAll() {
    sciHits.SetReadOnly(false);
    sciHits.ClearAll();
    sciHits.SetReadOnly(true);
    cumulativeLineIndex.clear();
}

void clearBelow() {
    Scintilla::Line line = sciHits.LineFromPosition(sciHits.CurrentPos());
    Scintilla::Line last = sciHits.LastChild(line, Level_Search);
    if (last + 1 < sciHits.LineCount()) {
        sciHits.SetReadOnly(false);
        Scintilla::Position removeBelow = sciHits.PositionFromLine(last + 1);
        sciHits.DeleteRange(removeBelow, sciHits.Length() - removeBelow);
        sciHits.MarkerDelete(last + 1, -1);
        sciHits.SetReadOnly(true);
        cumulativeLineIndex.resize(sciHits.LineCount() - 1);
    }
}


void configureSciHits() {

    plugin.getScintillaPointers();
    sciHits.configure(sci);

    constexpr Scintilla::Colour      searchFore = 0x000000;
    constexpr Scintilla::Colour      searchBack = 0x10C0D4;
    constexpr Scintilla::Colour      documentFore = 0xC0FFFF;
    constexpr Scintilla::Colour      documentBack = 0xC0A040;
    const     Scintilla::ColourAlpha caret = sciHits.ElementColour(Scintilla::Element::Caret);
    const     Scintilla::Colour      lineNumberFore = sciHits.StyleGetFore(STYLE_LINENUMBER);
    const     Scintilla::Colour      lineNumberBack = sciHits.StyleGetBack(STYLE_LINENUMBER);

    sciHits.SetMargins(1);
    sciHits.SetMarginTypeN(0, Scintilla::MarginType::RText);
    sciHits.SetMarginSensitiveN(0, true);
    sciHits.SetMarginWidthN(0, sciHits.TextWidth(STYLE_DEFAULT, (' ' + std::to_string(maxMarginNumber) + ' ').data()));

    sciHits.MarkerDefine(Marker_Search, Scintilla::MarkerSymbol::Background);
    sciHits.MarkerDefine(Marker_Document, Scintilla::MarkerSymbol::Background);
    sciHits.MarkerSetBack(Marker_Search, searchBack);
    sciHits.MarkerSetBack(Marker_Document, documentBack);

    sciHits.IndicSetStyle(Indicator_Found, Scintilla::IndicatorStyle::RoundBox);
    sciHits.IndicSetFore(Indicator_Found, caret);
    sciHits.IndicSetUnder(Indicator_Found, true);
    sciHits.IndicSetAlpha(Indicator_Found, Scintilla::Alpha(40));
    sciHits.IndicSetOutlineAlpha(Indicator_Found, Scintilla::Alpha(0));

    sciHits.IndicSetStyle(Indicator_NullMatch, Scintilla::IndicatorStyle::Point);
    sciHits.IndicSetFore(Indicator_NullMatch, caret);

    sciHits.StyleSetFore(STYLE_LINENUMBER, lineNumberFore);
    sciHits.StyleSetBack(STYLE_LINENUMBER, lineNumberBack);

    sciHits.StyleSetFore(Style_Found, caret);
    sciHits.StyleSetBold(Style_Found, true);
    sciHits.StyleSetFore(Style_Search, searchFore);
    sciHits.StyleSetBack(Style_Search, searchBack);
    sciHits.StyleSetBold(Style_Search, true);
    sciHits.StyleSetFore(Style_Document, documentFore);
    sciHits.StyleSetBack(Style_Document, documentBack);

    sciHits.SetRepresentationColour("\n", Scintilla::ColourAlpha(0));
    sciHits.SetRepresentationColour("\r", Scintilla::ColourAlpha(0));
    sciHits.SetRepresentationColour("\r\n", Scintilla::ColourAlpha(0));
    sciHits.SetCursor(Scintilla::CursorShape::Arrow);
    sciHits.SetReadOnly(true);

    caretLineIsBackground = sciHits.CaretLineVisible() && !sciHits.CaretLineFrame();
 
 }


HWND setupScintilla() {

    RECT r;
    GetClientRect(hitlist, &r);
    HWND sh = reinterpret_cast<HWND>(npp(NPPM_CREATESCINTILLAHANDLE, 0, hitlist));
    SetWindowPos(sh, 0, 0, 0, r.right, r.bottom, SWP_FRAMECHANGED | SWP_SHOWWINDOW);
    SetWindowLong(sh, GWL_ID, SCIHITS);
    SetWindowLong(sh, GWL_STYLE, GetWindowLong(sh, GWL_STYLE) | WS_BORDER);
    sciHits.attach(sh);
    configureSciHits();

    sciHits.subclass(
        [](ScintillaControl&, char key) -> bool /* Cntl keys */ {
            switch (key) {
            case 'd':
                nextDocument();
                return true;
            case 'D':
                prevDocument();
                return true;
            case 'g':
                showSearchInFilesDialog();
                return true;
            case 'G':
                closeSearchInFilesDialog();
                return true;
            case 'h':
                showSearchDialog();
                return true;
            case 'H':
                npp(NPPM_DMMHIDE, 0, hitlist);
                SetFocus(plugin.currentScintilla());
                return true;
            case 'n':
                SetFocus(plugin.currentScintilla());
                return true;
            case 'N':
                SetFocus(plugin.currentScintilla());
                if (data.searchDialog == data.dockingDialog) npp(NPPM_DMMHIDE, 0, data.searchDialog);
                                                        else ShowWindow(data.searchDialog, SW_HIDE);
                npp(NPPM_DMMHIDE, 0, hitlist);
                if (data.searchInFilesDialog) SendMessage(data.searchInFilesDialog, WM_COMMAND, IDCANCEL, 0);
                return true;
            case 'o':
                showSearchDialog();
                SetFocus(GetDlgItem(data.searchDialog, IDC_SEARCH_FINDBOX));
                return true;
            case 'O':
                if (data.searchDialog == data.dockingDialog) npp(NPPM_DMMHIDE, 0, data.searchDialog);
                else ShowWindow(data.searchDialog, SW_HIDE);
                return true;
            case 's':
                nextSearch();
                return true;
            case 'S':
                prevSearch();
                return true;
            }
            return false;
        },
        [](ScintillaControl&, bool shift, bool cntl) -> bool /* Tab key */ {
            if (cntl) return false;
            if (shift) prevMatch();
                  else nextMatch();
            return true;
        },
        [](ScintillaControl&, bool shift, bool cntl) -> bool /* Enter key */ {
            if (cntl) return false;
            processEnterKey(shift);
            return true;
        }
    );

    return sh;

}


INT_PTR CALLBACK hitlistDialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {

    switch (uMsg) {

    case WM_INITDIALOG:
    {
        hitlist = hwndDlg;
        setupScintilla();
        npp(NPPM_MODELESSDIALOG, MODELESSDIALOGADD, hwndDlg);
        return TRUE;
    }

    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case IDCANCEL:
            npp(NPPM_DMMHIDE, 0, hwndDlg);
            return TRUE;
        }
        return FALSE;

    case WM_CONTEXTMENU:
    {
        POINT screenLocation = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
        if (screenLocation.x == -1 && screenLocation.y == -1) /* invoked from keyboard, not mouse */ {
            Scintilla::Position caret = sciHits.CurrentPos();
            screenLocation.x = sciHits.PointXFromPosition(caret);
            screenLocation.y = sciHits.PointYFromPosition(caret);
            MapWindowPoints(sciHits.handle, 0, &screenLocation, 1);
        }
        bool hasSelection = !sciHits.SelectionEmpty();
        int zoom = sciHits.Zoom();
        std::wstring zoomText = (zoom > 0 ? L"&Zoom (+" : L"&Zoom (") + std::to_wstring(zoom) + L")";
        HMENU menu = GetSubMenu(LoadMenu(plugin.dllInstance, MAKEINTRESOURCE(IDR_SEARCH_CONTEXT)), 1);
        MENUITEMINFO mii;
        mii.cbSize     = sizeof mii;
        mii.fMask      = MIIM_STRING;
        mii.dwTypeData = zoomText.data();
        SetMenuItemInfo(menu, 14, TRUE, &mii);
        EnableMenuItem(menu, ID_SCMSCI_COPY       , hasSelection   ? MF_ENABLED : MF_GRAYED);
        EnableMenuItem(menu, ID_SCMSCI_ZOOMIN     , zoom < 60      ? MF_ENABLED : MF_GRAYED);
        EnableMenuItem(menu, ID_SCMSCI_ZOOMOUT    , zoom > -10     ? MF_ENABLED : MF_GRAYED);
        EnableMenuItem(menu, ID_SCMSCI_ZOOMDEFAULT, zoom != 0      ? MF_ENABLED : MF_GRAYED);
        mii.fMask = MIIM_FTYPE | MIIM_STATE;
        mii.fType = MFT_RADIOCHECK;
        mii.fState = sciHits.WrapMode() == Scintilla::Wrap::None ? MFS_CHECKED : 0;
        SetMenuItemInfo(menu, ID_SCMSCI_WRAPNONE, FALSE, &mii);
        mii.fState = sciHits.WrapMode() == Scintilla::Wrap::Char ? MFS_CHECKED : 0;
        SetMenuItemInfo(menu, ID_SCMSCI_WRAPCHAR, FALSE, &mii);
        mii.fState = sciHits.WrapMode() == Scintilla::Wrap::Word ? MFS_CHECKED : 0;
        SetMenuItemInfo(menu, ID_SCMSCI_WRAPWORD, FALSE, &mii);
        int result = TrackPopupMenu(menu, TPM_NONOTIFY | TPM_RETURNCMD,
                                    screenLocation.x, screenLocation.y, 0, sciHits.handle, 0);
        DestroyMenu(menu);
        switch (result) {
        case ID_SCMSCI_NEXTMATCH  : nextMatch        (); break;
        case ID_SCMSCI_PREVMATCH  : prevMatch        (); break;
        case ID_SCMSCI_NEXTDOC    : nextDocument     (); break;
        case ID_SCMSCI_PREVDOC    : prevDocument     (); break;
        case ID_SCMSCI_NEXTSEARCH : nextSearch       (); break;
        case ID_SCMSCI_PREVSEARCH : prevSearch       (); break;
        case ID_SCMSCI_CLEARALL   : clearAll         (); break;
        case ID_SCMSCI_CLEARBELOW : clearBelow       (); break;
        case ID_SCMSCI_COPY       : sciHits.Copy     (); break;
        case ID_SCMSCI_SELECTALL  : sciHits.SelectAll(); break;
        case ID_SCMSCI_ZOOMIN     : sciHits.ZoomIn   (); break;
        case ID_SCMSCI_ZOOMOUT    : sciHits.ZoomOut  (); break;
        case ID_SCMSCI_ZOOMDEFAULT: sciHits.SetZoom (0); break;
        case ID_SCMSCI_WRAPNONE   : sciHits.SetWrapMode(sciHits.configWrap = Scintilla::Wrap::None); break;
        case ID_SCMSCI_WRAPCHAR   : sciHits.SetWrapMode(sciHits.configWrap = Scintilla::Wrap::Char); break;
        case ID_SCMSCI_WRAPWORD   : sciHits.SetWrapMode(sciHits.configWrap = Scintilla::Wrap::Word); break;
        }
        return TRUE;
    }

    case WM_NOTIFY:
        switch (reinterpret_cast<NMHDR*>(lParam)->code) {
        case SCN_UPDATEUI:
        {
            const Scintilla::NotificationData& scn = *reinterpret_cast<Scintilla::NotificationData*>(lParam);
            if (Scintilla::FlagSet(scn.updated, Scintilla::Update::Selection)) {
                if (caretLineIsBackground) {
                    Scintilla::Line caretLine = sciHits.LineFromPosition(sciHits.CurrentPos());
                    sciHits.SetCaretLineFrame(Scintilla::LevelIsHeader(sciHits.FoldLevel(caretLine)) ? 3 : 0);
                }
            }
            return TRUE;
        }
        case SCN_ZOOM:
        {
            sciHits.sync();
            return TRUE;
        }
        case SCN_DOUBLECLICK:
            processDoubleClick(sciHits.lastLButtonDown);
            return TRUE;
        }
        return FALSE;

    case WM_SIZE:
        RECT r;
        GetClientRect(hitlist, &r);
        SetWindowPos(sciHits.handle, 0, 0, 0, r.right, r.bottom, SWP_FRAMECHANGED);
        return FALSE;
    }

    return FALSE;
}

}


void clearHitlist() { if (hitlist) clearAll(); }
bool hitlistEmpty() { if (!hitlist) return true; return sciHits.Length() == 0; }
void hideHitlist () { if (hitlist) npp(NPPM_DMMHIDE, 0, hitlist); }
void showHitlist () { if (hitlist) npp(NPPM_DMMSHOW, 0, hitlist); }

void colorHitlist() {
    if (hitlist) {
        constexpr ULONG dmfSetThemeDirectly = 0x00000010UL;
        npp(NPPM_DARKMODESUBCLASSANDTHEME, dmfSetThemeDirectly, sciHits.handle);
        configureSciHits();
    }
}


void showHitlist(std::string_view path) {
    showHitlist();
    if (sciHits.LineCount() < 3) return;
    Scintilla::Line headerLine = 1;
    for (;;) {
        std::string headerText = sciHits.GetLine(headerLine);
        if (headerText.length() > path.length() + 2
            && headerText.substr(headerText.length() - path.length() - 2, path.length()) == path) {
            sciHits.FoldLine(headerLine, Scintilla::FoldAction::Expand);
            sciHits.SetSel(-1, sciHits.PositionFromLine(headerLine + 1));
            sciHits.SetFirstVisibleLine(sciHits.VisibleFromDocLine(headerLine));
            return;
        }
        headerLine = sciHits.LastChild(headerLine, Level_Document) + 1;
        if (sciHits.FoldLevel(headerLine) != Level_Document) return;
    }
}


void showHitlist(MatchResults& matchResults) {

    HWND focused = data.focusResults ? 0 : GetFocus();

    if (hitlist) npp(NPPM_DMMSHOW, 0, hitlist);
    else {
        hitlist = CreateDialog(plugin.dllInstance, MAKEINTRESOURCE(IDD_HITLIST), plugin.nppData._nppHandle, hitlistDialogProc);
        NPP::tTbData dock;
        dock.hClient       = hitlist;
        dock.pszName       = L"Search++ Results";       // title bar text (caption in dialog is replaced)
        dock.dlgID         = -1;                        // zero-based position in menu to recall dialog at next startup
        dock.uMask         = DWS_DF_CONT_BOTTOM;        // first time display will be docked at the right
        dock.pszModuleName = L"Search++.dll";           // plugin module name
        npp(NPPM_DMMREGASDCKDLG, 0, &dock);
    }

    if (matchResults.index.empty()) {
        ShowWindow(hitlist, SW_NORMAL);
        return;
    }

    intptr_t newLines = matchResults.index.size();

    sciHits.SetReadOnly(false);
    sciHits.SetTargetRange(0, 0);
    sciHits.ReplaceTarget(std::string_view(matchResults.text.data() + matchResults.offset,
                                           matchResults.text.length() - matchResults.offset));
    matchResults.text.clear();
    cumulativeLineIndex.insert(cumulativeLineIndex.begin(), std::make_move_iterator(matchResults.index.begin())
                                                          , std::make_move_iterator(matchResults.index.end()));

    intptr_t newMargin = maxMarginNumber;

    sciHits.StartStyling  (0, 0);
    sciHits.SetStyling    (cumulativeLineIndex[0].length, Style_Search);
    sciHits.MarkerAdd     (0, Marker_Search);
    sciHits.MarginSetStyle(0, Style_Search);
    sciHits.SetFoldLevel  (0, Level_Search);

    bool indicatorSwap = false;
    intptr_t position = cumulativeLineIndex[0].length;

    for (intptr_t line = 1; line < newLines; ++line) {
        const MatchResults::LineIndex& mld = cumulativeLineIndex[line];
        if (mld.lineNumber < 0) /* file header line */ {
            sciHits.StartStyling  (position, 0);
            sciHits.SetStyling    (mld.length, Style_Document);
            sciHits.MarkerAdd     (line, Marker_Document);
            sciHits.MarginSetStyle(line, Style_Document);
            sciHits.SetFoldLevel  (line, Level_Document);
        }
        else {
            intptr_t lineNumber = mld.lineNumber + 1;
            sciHits.MarginSetText (line, (std::to_string(lineNumber) + ' ').data());
            sciHits.MarginSetStyle(line, STYLE_LINENUMBER);
            sciHits.SetFoldLevel  (line, Scintilla::FoldLevel::Base);
            if (newMargin < lineNumber) newMargin = lineNumber;
            for (const auto& hit : mld.matches) {
                Scintilla::Position hitStart;
                hitStart = hit.offset + position;
                if (hit.length == 0) {
                    sciHits.SetIndicatorCurrent(Indicator_NullMatch);
                    sciHits.SetIndicatorValue  (1);
                    sciHits.IndicatorFillRange (hitStart, 1);
                }
                else {
                    sciHits.StartStyling       (hitStart, 0);
                    sciHits.SetStyling         (hit.length, Style_Found);
                    sciHits.SetIndicatorCurrent(Indicator_Found);
                    sciHits.SetIndicatorValue  ((indicatorSwap = !indicatorSwap) ? 2 : 1);
                    sciHits.IndicatorFillRange (hitStart, hit.length);
                }
            }
        }
        position += mld.length;
    }

    if (newMargin > maxMarginNumber) {
        ++maxMarginNumber;
        while (maxMarginNumber <= newMargin) maxMarginNumber *= 10;
        --maxMarginNumber;
        sciHits.SetMarginWidthN(0, sciHits.TextWidth(STYLE_DEFAULT, (' ' + std::to_string(maxMarginNumber) + ' ').data()));
    }

    sciHits.SetFirstVisibleLine(0);
    sciHits.SetSel(-1, sciHits.PositionFromLine(2));
    sciHits.SetReadOnly(true);
    if (focused) SetFocus(focused);

}

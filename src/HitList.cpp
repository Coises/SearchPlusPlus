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
#include "SciControl.h"

#include "resource.h"
#include "Shlwapi.h"
#include <windowsx.h>

void showSearchDialog();
void showSearchInFilesDialog();


namespace {

std::vector<MatchResults::LineIndex> cumulativeLineIndex;

std::locale userLocale("");

HWND hitlist = 0;  // Window handle to the hit list dialog

SciControl sciHits = { "results list", true, 0 };

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
    scrollIntoView(start, end);
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
        [](SciControl&, char key) -> bool /* Cntl keys */ {
            switch (key) {
            case 'd':
                nextDocument();
                return true;
            case 'D':
                prevDocument();
                return true;
            case 'F':
                showSearchInFilesDialog();
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
        [](SciControl&, bool shift, bool cntl) -> bool /* Tab key */ {
            if (cntl) return false;
            if (shift) prevMatch();
                  else nextMatch();
            return true;
        },
        [](SciControl&, bool shift, bool cntl) -> bool /* Enter key */ {
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
    sciHits.ReplaceTarget(matchResults.text);
    matchResults.text.clear();
    cumulativeLineIndex.insert(cumulativeLineIndex.begin(), std::make_move_iterator(matchResults.index.begin())
                                                          , std::make_move_iterator(matchResults.index.end()));

    intptr_t newMargin = maxMarginNumber;

    sciHits.StartStyling  (0, 0);
    sciHits.SetStyling    (cumulativeLineIndex[0].length, Style_Search);
    sciHits.MarkerAdd     (0, Marker_Search);
    sciHits.MarginSetText (0, "====");
    sciHits.MarginSetStyle(0, Style_Search);
    sciHits.SetFoldLevel  (0, Level_Search);

    intptr_t position = cumulativeLineIndex[0].length;
    for (intptr_t line = 1; line < newLines; ++line) {
        const MatchResults::LineIndex& mld = cumulativeLineIndex[line];
        if (mld.lineNumber < 0) /* file header line */ {
            sciHits.StartStyling  (position, 0);
            sciHits.SetStyling    (mld.length, Style_Document);
            sciHits.MarkerAdd     (line, Marker_Document);
            sciHits.MarginSetText (line, "--");
            sciHits.MarginSetStyle(line, Style_Document);
            sciHits.SetFoldLevel  (line, Level_Document);
        }
        else {
            intptr_t lineNumber = mld.lineNumber + 1;
            sciHits.MarginSetText (line, (std::to_string(lineNumber) + ' ').data());
            sciHits.MarginSetStyle(line, STYLE_LINENUMBER);
            sciHits.SetFoldLevel  (line, Scintilla::FoldLevel::Base);
            if (newMargin < lineNumber) newMargin = lineNumber;
            bool indicatorSwap = false;
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


void showHitlist(ProgressInfo& pi) {

    if (!pi.count) return showHitlist();

    MatchResults matchResults;

    size_t files   = pi.hitSet->hitBlocks.size();
    size_t matches = pi.count;
    size_t indices = 1;
    size_t textlen = 0;  // estimate of the final textlen -- better to overestimate than underestimate!

    for (const auto& hb : pi.hitSet->hitBlocks) {
        indices += 1 + hb.hitLines.size();
        textlen += 62 + hb.documentPath.length();
        for (const auto& hl : hb.hitLines) textlen += hl.text.length();
    }

    std::string singleLineFindText = std::format(userLocale, " {:Ld} match{:s} in {:Ld} file{:s}: ",
                                                 matches, matches == 1 ? "" : "es", files, files == 1 ? "" : "s");
    for (size_t i = 0; i < pi.hitSet->searchString.length(); ++i) {
        switch (pi.hitSet->searchString[i]) {
        case '\t':
            singleLineFindText += reinterpret_cast<const char*>(u8"\u2B72");
            break;
        case '\n':
            singleLineFindText += reinterpret_cast<const char*>(u8"\u240A");
            break;
        case '\r':
            if (i + 1 < pi.hitSet->searchString.length() && pi.hitSet->searchString[i + 1] == '\n') {
                singleLineFindText += reinterpret_cast<const char*>(u8"\u21A9");
                ++i;
            }
            else singleLineFindText += reinterpret_cast<const char*>(u8"\u240D");
            break;
        default:
            singleLineFindText += pi.hitSet->searchString[i];
        }
    }
    singleLineFindText += "\r\n";

    matchResults.index.reserve(indices);
    matchResults.text.reserve(singleLineFindText.length() + textlen + 2);
    matchResults.index.emplace_back();
    matchResults.index.back().lineNumber = -2;
    matchResults.index.back().length = singleLineFindText.length();
    matchResults.text = singleLineFindText;

    for (const auto& hb : pi.hitSet->hitBlocks) {
        size_t linesMatched = hb.hitLines.size();
        size_t fileMatches = hb.count();
        std::string fileHeader = std::format(userLocale, "-- {:Ld} match{:s} in {:Ld} line{:s}: ",
                                                         fileMatches, fileMatches == 1 ? "" : "es",
                                                         linesMatched, linesMatched == 1 ? "" : "s")
                               + hb.documentPath + "\r\n";
        matchResults.index.emplace_back();
        matchResults.index.back().lineNumber = -1;
        matchResults.index.back().length = fileHeader.length();
        matchResults.text += fileHeader;
        if (hb.codepage == CP_UTF8) for (const auto& hl : hb.hitLines) {
            matchResults.index.emplace_back();
            matchResults.index.back().length = hl.text.length();
            matchResults.index.back().lineNumber = hl.line;
            matchResults.text += hl.text;
            for (const auto& hit : hl.hits)
                matchResults.index.back().matches.emplace_back(hit.cpMin - hl.position, hit.cpMax - hit.cpMin);
        }
        else for (size_t hitLineIndex = 0; hitLineIndex < hb.hitLines.size(); ++hitLineIndex) {  // <== can be made more efficient!
            const auto& hl = hb.hitLines[hitLineIndex];
            matchResults.index.emplace_back();
            matchResults.index.back().lineNumber = hl.line;
            for (const auto& hit : hl.hits) {
                matchResults.index.back().matches.emplace_back();
                auto& sm = matchResults.index.back().matches.back();
                sm.offset = (hit.cpMin == hl.position ? 0
                          : utf16to8(toWide(hl.text.substr(0, hit.cpMin - hl.position), hb.codepage)).length());
                if (hit.cpMax == hit.cpMin) sm.length = 0;
                else if (static_cast<size_t>(hit.cpMax - hl.position) <= hl.text.length())
                    sm.length = utf16to8(toWide(hl.text.substr(hit.cpMin - hl.position, hit.cpMax - hit.cpMin),
                                                hb.codepage)).length();
                else /* hit spans multiple lines */ {
                    sm.length = utf16to8(toWide(hl.text.substr(hit.cpMin - hl.position), hb.codepage)).length();
                    size_t remainingLength = hit.cpMax - hl.position - hl.text.length();
                    for (size_t hli = hitLineIndex + 1; remainingLength > 0; ++hli) {
                        const auto& extLine = hb.hitLines[hli];
                        if (remainingLength <= extLine.text.length()) {
                            sm.length += utf16to8(toWide(extLine.text.substr(0, remainingLength), hb.codepage)).length();
                            break;
                        }
                        sm.length += utf16to8(toWide(extLine.text, hb.codepage)).length();
                        remainingLength -= extLine.text.length();
                    }
                }
            }
            const std::string utf8text = utf16to8(toWide(hl.text, hb.codepage));
            matchResults.index.back().length = utf8text.length();
            matchResults.text += utf8text;
        }
        if (matchResults.text.back() != '\n' && matchResults.text.back() != '\r') {
            matchResults.text += "\r\n";
            matchResults.index.back().length += 2;
        }
    }

    showHitlist(matchResults);

}
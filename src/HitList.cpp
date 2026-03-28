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


namespace {

HWND hitlist = 0;  // Window handle to the hit list dialog
HWND sciHits = 0;  // Window handle to the Scintilla control containing the hit list

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

std::vector<std::unique_ptr<ProgressInfo::HitSet>> hitSets;

struct FoundLine {
    uintptr_t       buffer;
    Scintilla::Line line;
};
std::vector<FoundLine> foundLines = { {0, -1} };  // First entry represents no location

intptr_t maxMarginNumber = 999;                   // Maximum number that can be displayed in margin; always a power of 10 minus 1

POINT lastLButtonDown;                            // Last place the left mouse button went down -- used in double-click processing


// Handling for double-clicks and the Enter key

bool processDoubleClickOrEnterKey(Scintilla::Position cpMin, Scintilla::Position cpMax, bool switchFocus) {
    Scintilla::Line lnMin = sci.LineFromPosition(cpMin);
    if (Scintilla::LevelIsHeader(sci.FoldLevel(lnMin))) /* a search or document header: toggle fold */ {
        sci.ToggleFold(lnMin);
        return true;
    }
    int lineIndex = sci.LineState(lnMin);
    if (lineIndex < 1 || lineIndex >= static_cast<int>(foundLines.size()))  // Not a found line (e.g., empty line at end of hit list).
        return false;
    FoundLine foundLine = foundLines[lineIndex];   // Entry in table to translate hit list lines to document lines
    if (foundLine.buffer == 0) return false;       // Unexpected, but if it happens, the line isn't a found line.
    Scintilla::Position cpLine = sci.PositionFromLine(lnMin);
    int32_t docpos = static_cast<int32_t>(npp(NPPM_GETPOSFROMBUFFERID, foundLine.buffer, npp(NPPM_GETCURRENTVIEW, 0, 0)));
    if (docpos == -1) /* Document is no longer open. (Should we try to re-open it?) */ return false;
    npp(NPPM_ACTIVATEDOC, docpos >> 30, docpos & 0x3FFFFFFF);
    plugin.getScintillaPointers();
    UINT codepage = sci.CodePage();
    Scintilla::Position start;
    Scintilla::Position end;
    if (codepage == CP_UTF8) {
        Scintilla::Position offset = sci.PositionFromLine(foundLine.line) - cpLine;
        start = cpMin + offset;
        end   = cpMax + offset;
    }
    else {
        plugin.getScintillaPointers(sciHits);
        start = cpMin == cpLine ? 0 : fromWide(utf8to16(sci.StringOfRange(Scintilla::Span(cpLine, cpMin))), codepage).length();
        Scintilla::Position length = cpMax == cpMin ? 0
            : fromWide(utf8to16(sci.StringOfRange(Scintilla::Span(cpMin, cpMax ))), codepage).length();
        plugin.getScintillaPointers();
        start += sci.PositionFromLine(foundLine.line);
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
    plugin.getScintillaPointers(sciHits);
    Scintilla::Position position1 = sci.CharPositionFromPoint(click.x, click.y);
    Scintilla::Position position2 = sci.PositionFromPoint(click.x, click.y);
    Scintilla::Position cpMin, cpMax;
    if (sci.IndicatorValueAt(Indicator_Found, position1)) {
        cpMin = sci.IndicatorStart(Indicator_Found, position1);
        cpMax = sci.IndicatorEnd(Indicator_Found, position1);
    }
    else cpMin = cpMax = position2;
    sci.SetSel(cpMin, cpMax);
    return processDoubleClickOrEnterKey(cpMin, cpMax, switchFocus);
}

bool processEnterKey(bool switchFocus = true) {
    plugin.getScintillaPointers(sciHits);
    return processDoubleClickOrEnterKey(sci.SelectionStart(), sci.SelectionEnd(), switchFocus);
}


// Navigation in search results

void nextMatch() {

    plugin.getScintillaPointers(sciHits);
    Scintilla::Position length   = sci.Length();
    Scintilla::Position oldStart = sci.SelectionStart();
    Scintilla::Position oldEnd   = sci.SelectionEnd();

    if (oldEnd == length) {
        sci.SetSel(length, length);
        return;
    }

    if (oldStart != oldEnd && sci.IndicatorValueAt(Indicator_NullMatch, oldEnd)) {
        sci.SetSel(oldEnd, oldEnd);
        return;
    }

    Scintilla::Position nextNull = oldEnd + 1;
    if (nextNull >= length) nextNull = length;
    else if (!sci.IndicatorValueAt(Indicator_NullMatch, nextNull)) {
        nextNull = sci.IndicatorEnd(Indicator_NullMatch, nextNull);
        if (nextNull == 0) nextNull = length;
    }

    Scintilla::Position nextEnd = sci.IndicatorEnd(Indicator_Found, oldEnd);
    Scintilla::Position nextStart;
    if (nextEnd == 0) nextStart = nextEnd = length;
    else if (sci.IndicatorValueAt(Indicator_Found, nextEnd - 1)) nextStart = sci.IndicatorStart(Indicator_Found, nextEnd - 1);
    else {
        nextStart = nextEnd;
        nextEnd = sci.IndicatorEnd(Indicator_Found, nextStart);
    }

    if (nextNull <= nextStart) sci.SetSel(nextNull, nextNull);
    else {
        sci.SetSel(nextStart, nextEnd);
        sci.ScrollRange(nextEnd, nextStart);
    }

}

void prevMatch() {

    plugin.getScintillaPointers(sciHits);
    Scintilla::Position oldStart = sci.SelectionStart();
    Scintilla::Position oldEnd   = sci.SelectionEnd();

    if (oldStart == 0) {
        sci.SetSel(0, 0);
        return;
    }

    if (oldStart != oldEnd && sci.IndicatorValueAt(Indicator_NullMatch, oldStart)) {
        sci.SetSel(oldStart, oldStart);
        return;
    }

    Scintilla::Position prevNull = oldEnd - 1;
    if (prevNull <= 0) prevNull = 0;
    else if (!sci.IndicatorValueAt(Indicator_NullMatch, prevNull)) {
        prevNull = sci.IndicatorStart(Indicator_NullMatch, prevNull);
        if (prevNull > 0) --prevNull;
    }

    Scintilla::Position prevStart = sci.IndicatorStart(Indicator_Found, oldStart - 1);
    Scintilla::Position prevEnd;
    if (sci.IndicatorValueAt(Indicator_Found, prevStart)) prevEnd = sci.IndicatorEnd(Indicator_Found, prevStart);
    else if (prevStart == 0) prevStart = prevEnd = 0;
    else {
        prevEnd = prevStart;
        prevStart = sci.IndicatorStart(Indicator_Found, prevEnd - 1);
    }

    if (prevNull >= prevStart) sci.SetSel(prevNull, prevNull);
    else {
        sci.SetSel(prevStart, prevEnd);
        sci.ScrollRange(prevEnd, prevStart);
    }

}

void nextSearch() {
    plugin.getScintillaPointers(sciHits);
    Scintilla::Line line = sci.LineFromPosition(sci.CurrentPos());
    Scintilla::Line last = sci.LastChild(line, Level_Search);
    if (last + 1 < sci.LineCount()) {
        sci.EnsureVisible(line + 1);
        sci.GotoLine(last + 1);
        sci.ScrollVertical(last + 1, 0);
    }
}

void prevSearch() {
    plugin.getScintillaPointers(sciHits);
    Scintilla::Line line = sci.LineFromPosition(sci.CurrentPos());
    if (line == 0) return;
    while (--line > 0 && sci.FoldLevel(line) != Level_Search);
    sci.EnsureVisible(line);
    sci.GotoLine(line);
    sci.ScrollVertical(line, 0);
}

void nextDocument() {
    plugin.getScintillaPointers(sciHits);
    Scintilla::Line line = sci.LineFromPosition(sci.CurrentPos());
    Scintilla::Line last = sci.LastChild(line, Level_Document);
    if (last + 1 < sci.LineCount()) {
        sci.EnsureVisible(line + 1);
        sci.GotoLine(last + 1);
        sci.ScrollVertical(last + 1, 0);
    }
}

void prevDocument() {
    plugin.getScintillaPointers(sciHits);
    Scintilla::Line line = sci.LineFromPosition(sci.CurrentPos());
    if (line == 0) return;
    while (--line > 0 && sci.FoldLevel(line) == Scintilla::FoldLevel::Base);
    sci.EnsureVisible(line);
    sci.GotoLine(line);
    sci.ScrollVertical(line, 0);
}

void clearAll() {
    plugin.getScintillaPointers(sciHits);
    sci.SetReadOnly(false);
    sci.ClearAll();
    hitSets.clear();
    foundLines.clear();
    foundLines.push_back({ 0, -1 });  // First entry represents no location
    sci.SetReadOnly(true);
}

void clearBelow() {
    plugin.getScintillaPointers(sciHits);
    Scintilla::Line line = sci.LineFromPosition(sci.CurrentPos());
    Scintilla::Line last = sci.LastChild(line, Level_Search);
    if (last + 1 < sci.LineCount()) {
        sci.SetReadOnly(false);
        Scintilla::Position removeBelow = sci.PositionFromLine(last + 1);
        sci.DeleteRange(removeBelow, sci.Length() - removeBelow);
        sci.MarkerDelete(last + 1, -1);
        sci.SetReadOnly(true);
    }
}



// Subclass procedure for Scintilla control

LRESULT __stdcall subclassScintilla(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR, DWORD_PTR) {
    switch (uMsg) {
    case WM_GETDLGCODE:
        return DefSubclassProc(hWnd, uMsg, wParam, lParam) & ~DLGC_HASSETSEL;
    case WM_LBUTTONDOWN:
    {
        lastLButtonDown.x = GET_X_LPARAM(lParam);
        lastLButtonDown.y = GET_Y_LPARAM(lParam);
        break;
    }
    case WM_KEYDOWN:
        if ((lParam & KF_REPEAT) || !((GetKeyState(VK_CONTROL) & 0x8000) || (wParam == VK_TAB || wParam == VK_RETURN))) break;
        switch (wParam) {
        case 'D':
            if (GetKeyState(VK_SHIFT) & 0x8000) prevDocument();
            else nextDocument();
            return 0;
        case 'N':
            SetFocus(plugin.currentScintilla());
            if (GetKeyState(VK_SHIFT) & 0x8000) npp(NPPM_DMMHIDE, 0, hitlist);
            return 0;
        case 'O':
        {
            if (GetKeyState(VK_SHIFT) & 0x8000) break;
            if (!data.searchDialog) break;
            SetFocus(GetDlgItem(data.searchDialog, IDC_SEARCH_FINDBOX));
            return 0;
        }
        case 'S':
            if (GetKeyState(VK_SHIFT) & 0x8000) prevSearch();
            else nextSearch();
            return 0;
        case 'W':
        {
            if (GetKeyState(VK_SHIFT) & 0x8000) break;
            plugin.getScintillaPointers(hWnd);
            Scintilla::Wrap current = sci.WrapMode();
            if      (current == Scintilla::Wrap::None) sci.SetWrapMode(data.wrapHits = Scintilla::Wrap::Char);
            else if (current == Scintilla::Wrap::Char) sci.SetWrapMode(data.wrapHits = Scintilla::Wrap::Word);
            else                                       sci.SetWrapMode(data.wrapHits = Scintilla::Wrap::None);
            return 0;
        }
        case VK_TAB:
            if (GetKeyState(VK_SHIFT) & 0x8000) prevMatch();
            else nextMatch();
            return 0;
        case VK_RETURN:
            if (GetKeyState(VK_CONTROL) & 0x8000) break;
            processEnterKey(GetKeyState(VK_SHIFT) & 0x8000);
            return 0;
        }
    }
    return DefSubclassProc(hWnd, uMsg, wParam, lParam);
}


HWND setupScintilla() {

    plugin.getScintillaPointers();
    Scintilla::ColourAlpha whiteSpace     = sci.ElementColour(Scintilla::Element::WhiteSpace);
//    Scintilla::ColourAlpha caretLine      = sci.ElementColour(Scintilla::Element::CaretLineBack);
    Scintilla::Colour      lineNumberFore = sci.StyleGetFore(STYLE_LINENUMBER);
    Scintilla::Colour      lineNumberBack = sci.StyleGetBack(STYLE_LINENUMBER);
    Scintilla::Colour      searchFore     = 0x000000;
    Scintilla::Colour      searchBack     = 0x10C0D4;
    Scintilla::Colour      documentFore   = 0xC0FFFF;
    Scintilla::Colour      documentBack   = 0xC0A040;
    Scintilla::Colour      foundFore      = 0xFF0000;
    Scintilla::Colour      foundBack      = 0xC08080;

    RECT r;
    GetClientRect(hitlist, &r);
    sciHits = reinterpret_cast<HWND>(npp(NPPM_CREATESCINTILLAHANDLE, 0, hitlist));
    SetWindowPos(sciHits, 0, 0, 0, r.right, r.bottom, SWP_FRAMECHANGED | SWP_SHOWWINDOW);
    SetWindowLong(sciHits, GWL_ID, SCIHITS);
    SetWindowLong(sciHits, GWL_STYLE, GetWindowLong(sciHits, GWL_STYLE) | WS_BORDER);
    plugin.getScintillaPointers(sciHits);

    sci.SetModEventMask(Scintilla::ModificationFlags::None);
    sci.SetWrapMode(data.wrapHits);
    sci.SetZoom(data.zoomHits);
    sci.SetTabWidth(1);
    sci.SetViewWS(Scintilla::WhiteSpace::VisibleAlways);
    sci.SetViewEOL(true);
    sci.SetWhitespaceSize(2);
    sci.SetFoldFlags(Scintilla::FoldFlag::LineAfterContracted);
    sci.SetElementColour(Scintilla::Element::FoldLine, Scintilla::ColourAlpha(0));
    sci.UsePopUp(Scintilla::PopUp::Never);
    sci.SetUndoCollection(0);

    sci.SetElementColour(Scintilla::Element::SelectionInactiveBack, sci.ElementColour(Scintilla::Element::SelectionBack));
    sci.SetElementColour(Scintilla::Element::WhiteSpace, whiteSpace);
    sci.SetRepresentation("\n", reinterpret_cast<const char*>(u8"\u240A"));
    sci.SetRepresentation("\r", reinterpret_cast<const char*>(u8"\u240D"));
    sci.SetRepresentation("\r\n", reinterpret_cast<const char*>(u8"\u21A9"));
    sci.SetRepresentationAppearance("\r\n", Scintilla::RepresentationAppearance::Plain);
    sci.SetRepresentationAppearance("\n", Scintilla::RepresentationAppearance::Plain);
    sci.SetRepresentationAppearance("\r", Scintilla::RepresentationAppearance::Plain);
    sci.SetRepresentationColour("\n"  , Scintilla::ColourAlpha(0));
    sci.SetRepresentationColour("\r"  , Scintilla::ColourAlpha(0));
    sci.SetRepresentationColour("\r\n", Scintilla::ColourAlpha(0));

    sci.SetCursor(Scintilla::CursorShape::Arrow);
    sci.SetReadOnly(true);

    sci.ClearCmdKey(SCK_TAB);
    sci.ClearCmdKey(SCK_TAB + (SCMOD_SHIFT << 16));
    sci.ClearCmdKey(SCK_RETURN);
    sci.ClearCmdKey('D' + (SCMOD_CTRL << 16));
    sci.ClearCmdKey('N' + (SCMOD_CTRL << 16));
    sci.ClearCmdKey('O' + (SCMOD_CTRL << 16));
    sci.ClearCmdKey('S' + (SCMOD_CTRL << 16));
    sci.ClearCmdKey('W' + (SCMOD_CTRL << 16));
    sci.ClearCmdKey('D' + ((SCMOD_CTRL + SCMOD_SHIFT) << 16));
    sci.ClearCmdKey('N' + ((SCMOD_CTRL + SCMOD_SHIFT) << 16));
    sci.ClearCmdKey('S' + ((SCMOD_CTRL + SCMOD_SHIFT) << 16));

    sci.SetMargins(1);
    sci.SetMarginTypeN(0, Scintilla::MarginType::RText);
    sci.SetMarginSensitiveN(0, true);
    sci.SetMarginWidthN(0, sci.TextWidth(STYLE_DEFAULT, (' ' + std::to_string(maxMarginNumber) + ' ').data()));

    sci.MarkerDefine(Marker_Search, Scintilla::MarkerSymbol::Background);
    sci.MarkerDefine(Marker_Document, Scintilla::MarkerSymbol::Background);
    sci.MarkerSetBack(Marker_Search, searchBack);
    sci.MarkerSetBack(Marker_Document, documentBack);

    sci.IndicSetStyle(Indicator_Found, Scintilla::IndicatorStyle::RoundBox);
    sci.IndicSetFore (Indicator_Found, foundBack);
    sci.IndicSetUnder(Indicator_Found, true);
    sci.IndicSetAlpha(Indicator_Found, Scintilla::Alpha(64));
    sci.IndicSetOutlineAlpha(Indicator_Found, Scintilla::Alpha(0));

    sci.IndicSetStyle(Indicator_NullMatch, Scintilla::IndicatorStyle::Point);
    sci.IndicSetFore(Indicator_NullMatch, foundFore);

    sci.StyleClearAll();

    sci.StyleSetFore(STYLE_LINENUMBER, lineNumberFore);
    sci.StyleSetBack(STYLE_LINENUMBER, lineNumberBack);

    sci.StyleSetFore(Style_Found, foundFore);
    sci.StyleSetBold(Style_Found, true);
    sci.StyleSetFore(Style_Search, searchFore);
    sci.StyleSetBack(Style_Search, searchBack);
    sci.StyleSetBold(Style_Search, true);
    sci.StyleSetFore(Style_Document, documentFore);
    sci.StyleSetBack(Style_Document, documentBack);

    SetWindowSubclass(sciHits, subclassScintilla, 0, 0);
    return sciHits;
}


INT_PTR CALLBACK hitlistDialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {

    switch (uMsg) {

    case WM_DESTROY:
        RemoveWindowSubclass(sciHits, subclassScintilla, 0);
        return TRUE;

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
        HWND target = reinterpret_cast<HWND>(wParam);
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
        mii.fState = sci.WrapMode() == Scintilla::Wrap::None ? MFS_CHECKED : 0;
        SetMenuItemInfo(menu, ID_SCMSCI_WRAPNONE, FALSE, &mii);
        mii.fState = sci.WrapMode() == Scintilla::Wrap::Char ? MFS_CHECKED : 0;
        SetMenuItemInfo(menu, ID_SCMSCI_WRAPCHAR, FALSE, &mii);
        mii.fState = sci.WrapMode() == Scintilla::Wrap::Word ? MFS_CHECKED : 0;
        SetMenuItemInfo(menu, ID_SCMSCI_WRAPWORD, FALSE, &mii);
        int result = TrackPopupMenu(menu, TPM_NONOTIFY | TPM_RETURNCMD,
                                    screenLocation.x, screenLocation.y, 0, target, 0);
        DestroyMenu(menu);
        switch (result) {
        case ID_SCMSCI_NEXTMATCH  : nextMatch    (); break;
        case ID_SCMSCI_PREVMATCH  : prevMatch    (); break;
        case ID_SCMSCI_NEXTDOC    : nextDocument (); break;
        case ID_SCMSCI_PREVDOC    : prevDocument (); break;
        case ID_SCMSCI_NEXTSEARCH : nextSearch   (); break;
        case ID_SCMSCI_PREVSEARCH : prevSearch   (); break;
        case ID_SCMSCI_CLEARALL   : clearAll     (); break;
        case ID_SCMSCI_CLEARBELOW : clearBelow   (); break;
        case ID_SCMSCI_COPY       : sci.Copy     (); break;
        case ID_SCMSCI_SELECTALL  : sci.SelectAll(); break;
        case ID_SCMSCI_ZOOMIN     : sci.ZoomIn   (); break;
        case ID_SCMSCI_ZOOMOUT    : sci.ZoomOut  (); break;
        case ID_SCMSCI_ZOOMDEFAULT: sci.SetZoom (0); break;
        case ID_SCMSCI_WRAPNONE   : sci.SetWrapMode(data.wrapHits = Scintilla::Wrap::None); break;
        case ID_SCMSCI_WRAPCHAR   : sci.SetWrapMode(data.wrapHits = Scintilla::Wrap::Char); break;
        case ID_SCMSCI_WRAPWORD   : sci.SetWrapMode(data.wrapHits = Scintilla::Wrap::Word); break;
        }
        return TRUE;
    }

    case WM_NOTIFY:
        switch (reinterpret_cast<NMHDR*>(lParam)->code) {
        case SCN_MARGINCLICK:
            break;
        case SCN_ZOOM:
        {
            const Scintilla::NotificationData& scn = *reinterpret_cast<Scintilla::NotificationData*>(lParam);
            plugin.getScintillaPointers(reinterpret_cast<HWND>(scn.nmhdr.hwndFrom));
            data.zoomHits = sci.Zoom();
            return TRUE;
        }
        case SCN_DOUBLECLICK:
            processDoubleClick(lastLButtonDown);
            return TRUE;
        }
        return FALSE;

    case WM_SIZE:
        RECT r;
        GetClientRect(hitlist, &r);
        SetWindowPos(sciHits, 0, 0, 0, r.right, r.bottom, SWP_FRAMECHANGED);
        return FALSE;
    }

    return FALSE;
}

}


void clearHitlist() { if (hitlist) clearAll(); }
bool hitlistEmpty() { if (!hitlist) return true; plugin.getScintillaPointers(sciHits); return sci.Length() == 0; }
void showHitlist () { if (hitlist) npp(NPPM_DMMSHOW, 0, hitlist); }

void showHitlist(ProgressInfo& pi) {

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

    hitSets.push_back(std::move(pi.hitSet));

    intptr_t newMargin = maxMarginNumber;

    std::string rawFindText = hitSets.back()->searchString;
    std::string singleLineFindText;
    for (size_t i = 0; i < rawFindText.length(); ++i) {
        switch (rawFindText[i]) {
        case '\t':
            singleLineFindText += reinterpret_cast<const char*>(u8"\u2B72");
            break;
        case '\n':
            singleLineFindText += reinterpret_cast<const char*>(u8"\u240A");
            break;
        case '\r':
            if (i + 1 < rawFindText.length() && rawFindText[i + 1] == '\n') {
                singleLineFindText += reinterpret_cast<const char*>(u8"\u21A9");
                ++i;
            }
            else singleLineFindText += reinterpret_cast<const char*>(u8"\u240D");
            break;
        default:
            singleLineFindText += rawFindText[i];
        }
    }

    plugin.getScintillaPointers(sciHits);

    sci.SetReadOnly(false);
    sci.SetSel(0, 0);

    size_t documentCount = hitSets.back()->hitBlocks.size();
    std::string search = " " +std::to_string(pi.count) + (pi.count == 1 ? " match in " : " matches in ")
                       + std::to_string(documentCount) + (documentCount == 1 ? " document " : " documents ")
                       + singleLineFindText + "\r\n";
    sci.AddText(search.length(), search.data());
    sci.StartStyling(0, 0);
    sci.SetStyling(search.length(), Style_Search);
    sci.MarkerAdd(0, Marker_Search);
    sci.MarginSetText(0, "====");
    sci.MarginSetStyle(0, Style_Search);
    sci.SetFoldLevel(0, Level_Search);

    for (const auto& hitBlock : hitSets.back()->hitBlocks) {

        Scintilla::Position position = sci.CurrentPos();
        Scintilla::Line line = sci.LineFromPosition(position);
        intptr_t matchesInDocument = static_cast<intptr_t>(hitBlock.count());
        intptr_t linesMatched = static_cast<intptr_t>(hitBlock.hitLines.size());
        std::string documentLine = "-- " + std::to_string(matchesInDocument)
                                 + (matchesInDocument == 1 ? " match in " : " matches in ")
                                 + std::to_string(linesMatched)
                                 + (linesMatched == 1 ? " line: " : " lines: ")
                                 + hitBlock.documentPath + "\r\n";
        sci.AddText(documentLine.length(), documentLine.data());
        sci.StartStyling(position, 0);
        sci.SetStyling(documentLine.length(), Style_Document);
        sci.MarkerAdd(line, Marker_Document);
        sci.MarginSetText(line, "--");
        sci.MarginSetStyle(line, Style_Document);
        sci.SetFoldLevel(line, Level_Document);

        // Since some matches might cross lines, we add all the lines first,
        // then go back through again and style the matches.
        // Document line to position (dlp) will map document lines to hit list positions
        // so we know where to find matches when we do our second pass.

        std::map<Scintilla::Line, Scintilla::Position> dlp;

        for (const auto& hitLine : hitBlock.hitLines) {
            Scintilla::Position pos = sci.CurrentPos();
            Scintilla::Line ln = sci.LineFromPosition(pos);
            dlp[hitLine.line] = pos;
            std::string hitLineText = hitBlock.codepage == CP_UTF8 ? hitLine.text : utf16to8(toWide(hitLine.text, hitBlock.codepage));
            if (hitLineText.back() != '\n' && hitLineText.back() != '\r') hitLineText += "\r\n";  // can happen for the last line in a document
            sci.AddText(hitLineText.length(), hitLineText.data());
            Scintilla::Line displayLine = hitLine.line + 1;
            sci.MarginSetText(ln, (std::to_string(displayLine) + ' ').data());
            sci.MarginSetStyle(ln, STYLE_LINENUMBER);
            sci.SetLineState(ln, static_cast<int>(foundLines.size()));
            newMargin = std::max(newMargin, displayLine);
            sci.SetFoldLevel(ln, Scintilla::FoldLevel::Base);
            foundLines.push_back({ hitBlock.bufferID, hitLine.line });
        }

        bool indicatorSwap = false;
        for (size_t hitLineIndex = 0; hitLineIndex < hitBlock.hitLines.size(); ++hitLineIndex) {
            const auto& hitLine = hitBlock.hitLines[hitLineIndex];
            for (const auto& hit : hitLine.hits) {
                Scintilla::Position hitStart;
                Scintilla::Position hitLength;
                if (hitBlock.codepage == CP_UTF8) {
                    hitStart  = hit.cpMin - hitLine.position + dlp[hitLine.line];
                    hitLength = hit.cpMax - hit.cpMin;
                }
                else /* hit text and positions are in an ANSI code page, but the hit list is always UTF-8 */ {
                    hitStart = dlp[hitLine.line] + (hit.cpMin == hitLine.position ? 0
                        : utf16to8(toWide(hitLine.text.substr(0, hit.cpMin - hitLine.position), hitBlock.codepage)).length());
                    if (hit.cpMax == hit.cpMin) hitLength = 0;
                    else if (static_cast<size_t>(hit.cpMax - hitLine.position) <= hitLine.text.length())
                        hitLength = utf16to8(toWide(hitLine.text.substr(hit.cpMin - hitLine.position, hit.cpMax - hit.cpMin),
                                                    hitBlock.codepage)).length();
                    else /* hit spans multiple lines */ {
                        hitLength = utf16to8(toWide(hitLine.text.substr(hit.cpMin - hitLine.position), hitBlock.codepage)).length();
                        size_t remainingLength = hit.cpMax - hitLine.position - hitLine.text.length();
                        for (size_t hli = hitLineIndex + 1; remainingLength > 0; ++hli) {
                            const auto& extLine = hitBlock.hitLines[hli];
                            if (remainingLength <= extLine.text.length()) {
                                hitLength += utf16to8(toWide(extLine.text.substr(0, remainingLength), hitBlock.codepage)).length();
                                break;
                            }
                            hitLength += utf16to8(toWide(extLine.text, hitBlock.codepage)).length();
                            remainingLength -= extLine.text.length();
                        }
                    }
                }
                if (hitLength == 0) {
                    sci.SetIndicatorCurrent(Indicator_NullMatch);
                    sci.SetIndicatorValue(1);
                    sci.IndicatorFillRange(hitStart, 1);
                }
                else {
                    sci.StartStyling(hitStart, 0);
                    sci.SetStyling(hitLength, Style_Found);
                    sci.SetIndicatorCurrent(Indicator_Found);
                    sci.SetIndicatorValue((indicatorSwap = !indicatorSwap) ? 2 : 1);
                    sci.IndicatorFillRange(hitStart, hitLength);
                }
            }
        }
    }

    if (newMargin > maxMarginNumber) {
        ++maxMarginNumber;
        while (maxMarginNumber <= newMargin) maxMarginNumber *= 10;
        --maxMarginNumber;
        sci.SetMarginWidthN(0, sci.TextWidth(STYLE_DEFAULT, (' ' + std::to_string(maxMarginNumber) + ' ').data()));
    }

    sci.SetSel(-1, sci.PositionFromLine(2));
    sci.SetReadOnly(true);

    if (focused) SetFocus(focused);

}
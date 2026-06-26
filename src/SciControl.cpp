// This file is part of Search++.
// Copyright 2026 by by Randy Fellmy <https://www.coises.com/>.

// The source code contained in this file is independent of Notepad++ code.
// It is released under the MIT (Expat) license:
//
// Permission is hereby granted, free of charge, to any person obtaining a copy of this software and 
// associated documentation files (the "Software"), to deal in the Software without restriction, 
// including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, 
// and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, 
// subject to the following conditions:
// 
// The above copyright notice and this permission notice shall be included in all copies or substantial 
// portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT 
// LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
// IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, 
// WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE 
// SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

// SciControl centralizes setting up and maintaining configuration information for Scintilla controls

#include "SciControl.h"
#include "Host/Scintilla.h"
#include <windowsx.h>

namespace {

LRESULT __stdcall subclassScintilla(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR, DWORD_PTR sbp) {

    SciControl& sciCtrl = *reinterpret_cast<SciControl*>(sbp);

    switch (uMsg) {

    case WM_NCDESTROY:
        RemoveWindowSubclass(hWnd, subclassScintilla, 0);
        break;

    case WM_GETDLGCODE:
        if (!sciCtrl.acceptTab && wParam == VK_TAB)
            return DefSubclassProc(hWnd, uMsg, wParam, lParam) & ~(DLGC_HASSETSEL | DLGC_WANTTAB | DLGC_WANTMESSAGE);
        return DefSubclassProc(hWnd, uMsg, wParam, lParam) & ~DLGC_HASSETSEL;

    case WM_LBUTTONDOWN:
        sciCtrl.lastLButtonDown.x = GET_X_LPARAM(lParam);
        sciCtrl.lastLButtonDown.y = GET_Y_LPARAM(lParam);
        break;
    
    case WM_KEYDOWN:
    {

        if (lParam & KF_REPEAT) break;

        if (wParam == VK_RETURN && sciCtrl.enterKey) {
            if (sciCtrl.enterKey(sciCtrl, GetKeyState(VK_SHIFT) & 0x8000, GetKeyState(VK_CONTROL) & 0x8000)) return 0;
            break;
        }

        if (wParam == VK_TAB && sciCtrl.tabKey) {
            if (sciCtrl.tabKey(sciCtrl, GetKeyState(VK_SHIFT) & 0x8000, GetKeyState(VK_CONTROL) & 0x8000)) return 0;
            break;
        }

        if (!(GetKeyState(VK_CONTROL) & 0x8000) || wParam < L'A' || wParam > L'Z') break;

        // Passing the WM_KEYDOWN to Scintilla first, with ClearCmdKey having been performed for each combination,
        // avoids unwanted insertion of a control character when executing commands that open a dialog.

        LRESULT return_value = DefSubclassProc(hWnd, uMsg, wParam, lParam);

        if (sciCtrl.controlKey(sciCtrl, static_cast<unsigned char>(
            GetKeyState(VK_SHIFT) & 0x8000 ? std::toupper(static_cast<unsigned char>(wParam))
                                           : std::tolower(static_cast<unsigned char>(wParam)))
        )) return return_value;

        switch (wParam) {
        case 'W':
            if (GetKeyState(VK_SHIFT) & 0x8000) break;
            switch (sciCtrl.WrapMode()) {
            case Scintilla::Wrap::None: sciCtrl.SetWrapMode(Scintilla::Wrap::Char); sciCtrl.configWrap = Scintilla::Wrap::Char; break;
            case Scintilla::Wrap::Char: sciCtrl.SetWrapMode(Scintilla::Wrap::Word); sciCtrl.configWrap = Scintilla::Wrap::Word; break;
            default:                    sciCtrl.SetWrapMode(Scintilla::Wrap::None); sciCtrl.configWrap = Scintilla::Wrap::None;
            }
            break;
        }

        return return_value;

    }
    }

    return DefSubclassProc(hWnd, uMsg, wParam, lParam);

}

}


void SciControl::attach(HWND scintillaHandle) {
    handle = scintillaHandle;
    Scintilla::FunctionDirect d = reinterpret_cast<Scintilla::FunctionDirect>
        (SendMessage(handle, static_cast<UINT>(Scintilla::Message::GetDirectStatusFunction), 0, 0));
    intptr_t p = SendMessage(handle, static_cast<UINT>(Scintilla::Message::GetDirectPointer), 0, 0);
    SetFnPtr(d, p);
    SetStatus(Scintilla::Status::Ok);  // C-interface code can ignore an error status, causing exception in C++ interface
}

void SciControl::configure(Scintilla::ScintillaCall& reference) {
    Configuration c;
    c.caret            = reference.ElementColour(Scintilla::Element::Caret);
    c.caretLineBack    = reference.ElementColour(Scintilla::Element::CaretLineBack);
    c.selectionBack    = reference.ElementColour(Scintilla::Element::SelectionBack);
    c.whiteSpace       = reference.ElementColour(Scintilla::Element::WhiteSpace);
    c.defaultSize      = reference.StyleGetSizeFractional(STYLE_DEFAULT);
    c.defaultFont      = reference.StyleGetFont(STYLE_DEFAULT);
    c.defaultFore      = reference.StyleGetFore(STYLE_DEFAULT);
    c.defaultBack      = reference.StyleGetBack(STYLE_DEFAULT);
    c.lineNumberFore   = reference.StyleGetFore(STYLE_LINENUMBER);
    c.lineNumberBack   = reference.StyleGetBack(STYLE_LINENUMBER);
    c.caretStyle       = reference.CaretStyle();
    c.caretWidth       = reference.CaretWidth();
    c.caretPeriod      = reference.CaretPeriod();
    c.caretLineFrame   = reference.CaretLineFrame();
    c.caretLineVisible = reference.CaretLineVisible();
    configure(c);
    }


void SciControl::configure(const Configuration& c) {

    SetElementColour(Scintilla::Element::Caret                , c.caret        );
    SetElementColour(Scintilla::Element::CaretLineBack        , c.caretLineBack);
    SetElementColour(Scintilla::Element::SelectionBack        , c.selectionBack);
    SetElementColour(Scintilla::Element::SelectionInactiveBack, c.selectionBack);
    SetElementColour(Scintilla::Element::WhiteSpace           , c.whiteSpace   );
    StyleSetSizeFractional(STYLE_DEFAULT, c.defaultSize       );
    StyleSetFont(STYLE_DEFAULT, c.defaultFont.data());
    StyleSetFore(STYLE_DEFAULT, c.defaultFore       );
    StyleSetBack(STYLE_DEFAULT, c.defaultBack       );
    StyleClearAll();

    SetCaretStyle      (c.caretStyle      );
    SetCaretWidth      (c.caretWidth      );
    SetCaretPeriod     (c.caretPeriod     );
    SetCaretLineFrame  (c.caretLineFrame  );
    SetCaretLineVisible(c.caretLineVisible);

    StyleSetFore(STYLE_LINENUMBER, c.lineNumberFore);
    StyleSetBack(STYLE_LINENUMBER, c.lineNumberBack);

    SetModEventMask(Scintilla::ModificationFlags::DeleteText | Scintilla::ModificationFlags::InsertText);
    SetMargins(0);
    SetWrapMode(Scintilla::Wrap::Char);
    SetTabWidth(1);
    SetUseTabs(true);
    SetViewWS(Scintilla::WhiteSpace::VisibleAlways);
    SetViewEOL(true);
    SetWhitespaceSize(2);
    UsePopUp(Scintilla::PopUp::Never);
    SetMultipleSelection(false);
    SetVirtualSpaceOptions(Scintilla::VirtualSpace::None);
    SetAdditionalSelectionTyping(true);
    SetRepresentation("\n", reinterpret_cast<const char*>(u8"\U0001F807"));
    SetRepresentation("\r", reinterpret_cast<const char*>(u8"\U0001F804"));
    SetRepresentation("\r\n", reinterpret_cast<const char*>(u8"\u21A9"));
    SetRepresentationAppearance("\n", Scintilla::RepresentationAppearance::Plain);
    SetRepresentationAppearance("\r", Scintilla::RepresentationAppearance::Plain);
    SetRepresentationAppearance("\r\n", Scintilla::RepresentationAppearance::Plain);
    SetRepresentationColour("\n"  , c.whiteSpace);
    SetRepresentationColour("\r"  , c.whiteSpace);
    SetRepresentationColour("\r\n", c.whiteSpace);

    ClearCmdKey('B' + (SCMOD_CTRL << 16));
    ClearCmdKey('E' + (SCMOD_CTRL << 16));
    ClearCmdKey('F' + (SCMOD_CTRL << 16));
    ClearCmdKey('G' + (SCMOD_CTRL << 16));
    ClearCmdKey('H' + (SCMOD_CTRL << 16));
    ClearCmdKey('I' + (SCMOD_CTRL << 16));
    ClearCmdKey('J' + (SCMOD_CTRL << 16));
    ClearCmdKey('L' + (SCMOD_CTRL << 16));
    ClearCmdKey('M' + (SCMOD_CTRL << 16));
    ClearCmdKey('N' + (SCMOD_CTRL << 16));
    ClearCmdKey('O' + (SCMOD_CTRL << 16));
    ClearCmdKey('P' + (SCMOD_CTRL << 16));
    ClearCmdKey('Q' + (SCMOD_CTRL << 16));
    ClearCmdKey('R' + (SCMOD_CTRL << 16));
    ClearCmdKey('S' + (SCMOD_CTRL << 16));
    ClearCmdKey('W' + (SCMOD_CTRL << 16));
    ClearCmdKey('A' + ((SCMOD_CTRL + SCMOD_SHIFT) << 16));
    ClearCmdKey('B' + ((SCMOD_CTRL + SCMOD_SHIFT) << 16));
    ClearCmdKey('C' + ((SCMOD_CTRL + SCMOD_SHIFT) << 16));
    ClearCmdKey('D' + ((SCMOD_CTRL + SCMOD_SHIFT) << 16));
    ClearCmdKey('E' + ((SCMOD_CTRL + SCMOD_SHIFT) << 16));
    ClearCmdKey('F' + ((SCMOD_CTRL + SCMOD_SHIFT) << 16));
    ClearCmdKey('G' + ((SCMOD_CTRL + SCMOD_SHIFT) << 16));
    ClearCmdKey('H' + ((SCMOD_CTRL + SCMOD_SHIFT) << 16));
    ClearCmdKey('I' + ((SCMOD_CTRL + SCMOD_SHIFT) << 16));
    ClearCmdKey('J' + ((SCMOD_CTRL + SCMOD_SHIFT) << 16));
    ClearCmdKey('K' + ((SCMOD_CTRL + SCMOD_SHIFT) << 16));
    ClearCmdKey('M' + ((SCMOD_CTRL + SCMOD_SHIFT) << 16));
    ClearCmdKey('N' + ((SCMOD_CTRL + SCMOD_SHIFT) << 16));
    ClearCmdKey('O' + ((SCMOD_CTRL + SCMOD_SHIFT) << 16));
    ClearCmdKey('P' + ((SCMOD_CTRL + SCMOD_SHIFT) << 16));
    ClearCmdKey('Q' + ((SCMOD_CTRL + SCMOD_SHIFT) << 16));
    ClearCmdKey('R' + ((SCMOD_CTRL + SCMOD_SHIFT) << 16));
    ClearCmdKey('S' + ((SCMOD_CTRL + SCMOD_SHIFT) << 16));
    ClearCmdKey('V' + ((SCMOD_CTRL + SCMOD_SHIFT) << 16));
    ClearCmdKey('W' + ((SCMOD_CTRL + SCMOD_SHIFT) << 16));
    ClearCmdKey('X' + ((SCMOD_CTRL + SCMOD_SHIFT) << 16));
    ClearCmdKey('Y' + ((SCMOD_CTRL + SCMOD_SHIFT) << 16));
    ClearCmdKey('Z' + ((SCMOD_CTRL + SCMOD_SHIFT) << 16));

    if (!acceptTab) ClearCmdKey(SCK_TAB);
    if (depth) {
        TargetWholeDocument();
        ReplaceTarget(text.get());
    }
    SetWrapMode(configWrap);
    SetZoom(configZoom);

}


void SciControl::push() {
    if (!depth) return;
    TargetWholeDocument();
    std::string t = TargetText();
    std::vector<std::string>& h = history.get();
    if (h.empty()) h.push_back(t);
    else if (t != h[0]) {
        if (h[0].empty()) h[0] = t;
        else h.insert(h.begin(), t);
        if (h.size() > 1) {
            for (auto it = h.begin() + 1; it != h.end();) {
                if (t == *it) it = h.erase(it);
                else ++it;
            }
        }
    }
    if (depth && static_cast<int>(h.size()) > depth) h.resize(depth);
    history.put();
}


void SciControl::subclass(
    bool (*cKey)(SciControl& sciCtrl, char key),
    bool (*tKey)(SciControl& sciCtrl, bool shift, bool control),
    bool (*eKey)(SciControl& sciCtrl, bool shift, bool control)
) {
    controlKey = cKey;
    tabKey     = tKey;
    enterKey   = eKey;
    SetWindowSubclass(handle, subclassScintilla, 0, reinterpret_cast<DWORD_PTR>(this));
}

void SciControl::sync() {
    if (depth) {
        TargetWholeDocument();
        text = TargetText();
    }
    configWrap = WrapMode();
    configZoom = Zoom();
}
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

// ScintillaControl centralizes setting up and maintaining configuration information for Scintilla controls

#include "ScintillaControl.h"
#include "Host/Scintilla.h"
#include <windowsx.h>

namespace {

LRESULT __stdcall subclassScintilla(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR sbp) {

    ScintillaControl& sciCtrl = *reinterpret_cast<ScintillaControl*>(sbp);

    switch (uMsg) {

    case WM_NCDESTROY:
        RemoveWindowSubclass(hWnd, subclassScintilla, uIdSubclass);
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

        if (!(GetKeyState(VK_CONTROL) & 0x8000) || (GetKeyState(VK_MENU) & 0x8000)) break;

        if (!((wParam >= L'A' && wParam <= L'Z') || (wParam >= L'0' && wParam <= L'9'))) break;

        // Passing the WM_KEYDOWN to Scintilla first, with ClearCmdKey having been performed for each combination,
        // avoids unwanted insertion of a control character when executing commands that open a dialog.

        LRESULT return_value = DefSubclassProc(hWnd, uMsg, wParam, lParam);

        if (sciCtrl.controlKey(sciCtrl, static_cast<char>(
            GetKeyState(VK_SHIFT) & 0x8000 ? (wParam >= 'A' ? wParam : wParam - 16)
                                           : (wParam >= 'A' ? wParam + 32 : wParam))
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


ScintillaControl::Configuration& ScintillaControl::Configuration::get(Scintilla::ScintillaCall& sciCall) {
    caret            = sciCall.ElementColour(Scintilla::Element::Caret);
    caretLineBack    = sciCall.ElementColour(Scintilla::Element::CaretLineBack);
    selectionBack    = sciCall.ElementColour(Scintilla::Element::SelectionBack);
    whiteSpace       = sciCall.ElementColour(Scintilla::Element::WhiteSpace);
    defaultSize      = sciCall.StyleGetSizeFractional(STYLE_DEFAULT);
    defaultFont      = sciCall.StyleGetFont(STYLE_DEFAULT);
    defaultFore      = sciCall.StyleGetFore(STYLE_DEFAULT);
    defaultBack      = sciCall.StyleGetBack(STYLE_DEFAULT);
    caretStyle       = sciCall.CaretStyle();
    caretWidth       = sciCall.CaretWidth();
    caretPeriod      = sciCall.CaretPeriod();
    caretLineFrame   = sciCall.CaretLineFrame();
    caretLineVisible = sciCall.CaretLineVisible();
    lineNumberFore   = sciCall.StyleGetFore(STYLE_LINENUMBER);
    lineNumberBack   = sciCall.StyleGetBack(STYLE_LINENUMBER);
    return *this;
}

ScintillaControl::Configuration& ScintillaControl::Configuration::put(Scintilla::ScintillaCall& sciCall) {
    sciCall.SetElementColour(Scintilla::Element::Caret                , caret        );
    sciCall.SetElementColour(Scintilla::Element::CaretLineBack        , caretLineBack);
    sciCall.SetElementColour(Scintilla::Element::SelectionBack        , selectionBack);
    sciCall.SetElementColour(Scintilla::Element::SelectionInactiveBack, selectionBack);
    sciCall.SetElementColour(Scintilla::Element::WhiteSpace           , whiteSpace   );
    sciCall.StyleSetSizeFractional(STYLE_DEFAULT, defaultSize);
    sciCall.StyleSetFont(STYLE_DEFAULT, defaultFont.data());
    sciCall.StyleSetFore(STYLE_DEFAULT, defaultFore       );
    sciCall.StyleSetBack(STYLE_DEFAULT, defaultBack       );
    sciCall.StyleClearAll();
    sciCall.SetCaretStyle      (caretStyle      );
    sciCall.SetCaretWidth      (caretWidth      );
    sciCall.SetCaretPeriod     (caretPeriod     );
    sciCall.SetCaretLineFrame  (caretLineFrame  );
    sciCall.SetCaretLineVisible(caretLineVisible);
    sciCall.StyleSetFore(STYLE_LINENUMBER, lineNumberFore);
    sciCall.StyleSetBack(STYLE_LINENUMBER, lineNumberBack);
    sciCall.SetRepresentationColour("\n", whiteSpace);
    sciCall.SetRepresentationColour("\r", whiteSpace);
    sciCall.SetRepresentationColour("\r\n", whiteSpace);
    return *this;
}

void ScintillaControl::Configuration::common(Scintilla::ScintillaCall& sciCall) {
    sciCall.SetModEventMask(Scintilla::ModificationFlags::DeleteText | Scintilla::ModificationFlags::InsertText);
    sciCall.SetMargins(0);
    sciCall.SetWrapMode(Scintilla::Wrap::Char);
    sciCall.SetTabWidth(1);
    sciCall.SetUseTabs(true);
    sciCall.SetViewWS(Scintilla::WhiteSpace::VisibleAlways);
    sciCall.SetViewEOL(true);
    sciCall.SetWhitespaceSize(2);
    sciCall.UsePopUp(Scintilla::PopUp::Never);
    sciCall.SetMultipleSelection(false);
    sciCall.SetVirtualSpaceOptions(Scintilla::VirtualSpace::None);
    sciCall.SetAdditionalSelectionTyping(true);
    sciCall.SetRepresentation("\n", reinterpret_cast<const char*>(u8"\U0001F807"));
    sciCall.SetRepresentation("\r", reinterpret_cast<const char*>(u8"\U0001F804"));
    sciCall.SetRepresentation("\r\n", reinterpret_cast<const char*>(u8"\u21A9"));
    sciCall.SetRepresentationAppearance("\n", Scintilla::RepresentationAppearance::Plain);
    sciCall.SetRepresentationAppearance("\r", Scintilla::RepresentationAppearance::Plain);
    sciCall.SetRepresentationAppearance("\r\n", Scintilla::RepresentationAppearance::Plain);
    sciCall.ClearCmdKey('B' + (SCMOD_CTRL << 16));
    sciCall.ClearCmdKey('E' + (SCMOD_CTRL << 16));
    sciCall.ClearCmdKey('F' + (SCMOD_CTRL << 16));
    sciCall.ClearCmdKey('G' + (SCMOD_CTRL << 16));
    sciCall.ClearCmdKey('H' + (SCMOD_CTRL << 16));
    sciCall.ClearCmdKey('I' + (SCMOD_CTRL << 16));
    sciCall.ClearCmdKey('J' + (SCMOD_CTRL << 16));
    sciCall.ClearCmdKey('K' + (SCMOD_CTRL << 16));
    sciCall.ClearCmdKey('M' + (SCMOD_CTRL << 16));
    sciCall.ClearCmdKey('N' + (SCMOD_CTRL << 16));
    sciCall.ClearCmdKey('O' + (SCMOD_CTRL << 16));
    sciCall.ClearCmdKey('P' + (SCMOD_CTRL << 16));
    sciCall.ClearCmdKey('Q' + (SCMOD_CTRL << 16));
    sciCall.ClearCmdKey('R' + (SCMOD_CTRL << 16));
    sciCall.ClearCmdKey('S' + (SCMOD_CTRL << 16));
    sciCall.ClearCmdKey('W' + (SCMOD_CTRL << 16));
    sciCall.ClearCmdKey('A' + ((SCMOD_CTRL + SCMOD_SHIFT) << 16));
    sciCall.ClearCmdKey('B' + ((SCMOD_CTRL + SCMOD_SHIFT) << 16));
    sciCall.ClearCmdKey('C' + ((SCMOD_CTRL + SCMOD_SHIFT) << 16));
    sciCall.ClearCmdKey('D' + ((SCMOD_CTRL + SCMOD_SHIFT) << 16));
    sciCall.ClearCmdKey('E' + ((SCMOD_CTRL + SCMOD_SHIFT) << 16));
    sciCall.ClearCmdKey('F' + ((SCMOD_CTRL + SCMOD_SHIFT) << 16));
    sciCall.ClearCmdKey('G' + ((SCMOD_CTRL + SCMOD_SHIFT) << 16));
    sciCall.ClearCmdKey('H' + ((SCMOD_CTRL + SCMOD_SHIFT) << 16));
    sciCall.ClearCmdKey('I' + ((SCMOD_CTRL + SCMOD_SHIFT) << 16));
    sciCall.ClearCmdKey('J' + ((SCMOD_CTRL + SCMOD_SHIFT) << 16));
    sciCall.ClearCmdKey('K' + ((SCMOD_CTRL + SCMOD_SHIFT) << 16));
    sciCall.ClearCmdKey('M' + ((SCMOD_CTRL + SCMOD_SHIFT) << 16));
    sciCall.ClearCmdKey('N' + ((SCMOD_CTRL + SCMOD_SHIFT) << 16));
    sciCall.ClearCmdKey('O' + ((SCMOD_CTRL + SCMOD_SHIFT) << 16));
    sciCall.ClearCmdKey('P' + ((SCMOD_CTRL + SCMOD_SHIFT) << 16));
    sciCall.ClearCmdKey('Q' + ((SCMOD_CTRL + SCMOD_SHIFT) << 16));
    sciCall.ClearCmdKey('R' + ((SCMOD_CTRL + SCMOD_SHIFT) << 16));
    sciCall.ClearCmdKey('S' + ((SCMOD_CTRL + SCMOD_SHIFT) << 16));
    sciCall.ClearCmdKey('V' + ((SCMOD_CTRL + SCMOD_SHIFT) << 16));
    sciCall.ClearCmdKey('W' + ((SCMOD_CTRL + SCMOD_SHIFT) << 16));
    sciCall.ClearCmdKey('X' + ((SCMOD_CTRL + SCMOD_SHIFT) << 16));
    sciCall.ClearCmdKey('Y' + ((SCMOD_CTRL + SCMOD_SHIFT) << 16));
    sciCall.ClearCmdKey('Z' + ((SCMOD_CTRL + SCMOD_SHIFT) << 16));
}


ScintillaControl& ScintillaControl::attach(HWND scintillaHandle) {
    handle = scintillaHandle;
    Scintilla::FunctionDirect d = reinterpret_cast<Scintilla::FunctionDirect>
        (SendMessage(handle, static_cast<UINT>(Scintilla::Message::GetDirectStatusFunction), 0, 0));
    intptr_t p = SendMessage(handle, static_cast<UINT>(Scintilla::Message::GetDirectPointer), 0, 0);
    SetFnPtr(d, p);
    SetStatus(Scintilla::Status::Ok);  // C-interface code can ignore an error status, causing exception in C++ interface
    return *this;
}

ScintillaControl& ScintillaControl::configure(Scintilla::ScintillaCall& reference) {
    Configuration c;
    c.get(reference);
    configure(c);
    return *this;
    }

ScintillaControl& ScintillaControl::configure(Configuration& c) {
    c.put(*this);
    Configuration::common(*this);
    load();
    return *this;
}

ScintillaControl& ScintillaControl::load() {
    if (depth) {
        TargetWholeDocument();
        ReplaceTarget(configText.get());
    }
    SetWrapMode(configWrap);
    SetZoom(configZoom);
    return *this;
}

ScintillaControl& ScintillaControl::push() {
    if (!depth) return *this;
    std::string t = GetText(TextLength());
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
    return *this;
}

ScintillaControl& ScintillaControl::subclass(
    bool (*cKey)(ScintillaControl& sciCtrl, char key),
    bool (*tKey)(ScintillaControl& sciCtrl, bool shift, bool control),
    bool (*eKey)(ScintillaControl& sciCtrl, bool shift, bool control)
) {
    controlKey = cKey;
    tabKey     = tKey;
    enterKey   = eKey;
    SetWindowSubclass(handle, subclassScintilla, 0, reinterpret_cast<DWORD_PTR>(this));
    return *this;
}

ScintillaControl& ScintillaControl::sync() {
    if (depth) configText = GetText(TextLength());
    configWrap = WrapMode();
    configZoom = Zoom();
    return *this;
}
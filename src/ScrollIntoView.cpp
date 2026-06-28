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

#define NOMINMAX
#include <Windows.h>
#include "Framework/ScintillaCallEx.h"

namespace {

Scintilla::ScintillaCall sci;

RECT            scintillaRect;
RECT            dialogRect = { 0, 0, 0, 0 };
Scintilla::Line firstVisibleLine;
int             pxOffset;
int             pxBlank;

struct ScrollNeeded {
    Scintilla::Line firstVisible;
    int xOffset;
    bool vertical;
    bool horizontal;
    int clearVertical;
    int clearHorizontal;
    bool betterThan(const ScrollNeeded& sn) {
        if (!vertical && !horizontal && (sn.vertical || sn.horizontal)) return true;
        if ((vertical || horizontal) && !sn.vertical && !sn.horizontal) return false;
        if (clearVertical > sn.clearVertical && clearHorizontal > sn.clearHorizontal) return true;
        if (clearVertical >= 0 && clearHorizontal >= 0 && (clearVertical < 0 || clearHorizontal < 0)) return true;
        if ((clearVertical >= 0 || clearHorizontal >= 0) && (clearVertical < 0 && clearHorizontal < 0)) return true;
        if (!vertical && sn.vertical) return true;
        if (!horizontal && sn.horizontal) return true;
        return false;
    }
};

struct VisibleArea {
    RECT rect;
    Scintilla::Line lineFirst, lineLast, depth;
    int width;
    VisibleArea(int left, int top, int right, int bottom) {
        rect = { left, top, right, bottom };
        int lineHeight = sci.TextHeight(0);
        lineFirst = (top - scintillaRect.top + lineHeight - 1) / lineHeight;
        lineLast = (bottom - scintillaRect.top) / lineHeight - 1;
        depth = lineLast - lineFirst + 1;
        width = right - left;
    }
};

struct ScrollTarget {
    Scintilla::Position foundStart;
    Scintilla::Position foundEnd;
    Scintilla::Line foundLine;
    Scintilla::Line foundLast;
    Scintilla::Line foundVisibleLine;
    Scintilla::Line foundDepth;
    Scintilla::Line subLineOffset;
    int pxStart;
    int pxEnd;
    int pxLine;
    bool wrap;
    ScrollTarget(Scintilla::Position foundStart, Scintilla::Position foundEnd);
    ScrollNeeded find(const VisibleArea& va) const;
};

ScrollTarget::ScrollTarget(Scintilla::Position foundStart, Scintilla::Position foundEnd) : foundStart(foundStart), foundEnd(foundEnd) {
    foundLine = sci.LineFromPosition(foundStart);
    foundLast = sci.LineFromPosition(foundEnd);
    sci.ShowLines(foundLine, foundLast);
    foundVisibleLine = sci.VisibleFromDocLine(foundLine);
    wrap = sci.WrapMode() != Scintilla::Wrap::None;
    if (wrap) {
        int yFirst = sci.PointYFromPosition(foundStart);
        int yLast = sci.PointYFromPosition(foundEnd);
        int yLine = sci.PointYFromPosition(sci.PositionFromLine(foundLine));
        int pxHeight = sci.TextHeight(0);
        foundDepth = (yLast - yFirst + pxHeight / 2) / pxHeight + 1;
        subLineOffset = (yFirst - yLine + pxHeight / 2) / pxHeight;
    }
    else {
        foundDepth = foundLast - foundLine + 1;
        subLineOffset = 0;
    }
    pxStart = sci.PointXFromPosition(foundStart);
    pxEnd = sci.PointXFromPosition(foundEnd);
    pxLine = sci.PointXFromPosition(sci.PositionFromLine(foundLine));
}

ScrollNeeded ScrollTarget::find(const VisibleArea& va) const {
    ScrollNeeded sn;
    sn.clearVertical = static_cast<int>(va.depth - foundDepth);
    if (sn.clearVertical >= 0) {
        Scintilla::Line firstAreaLine = firstVisibleLine + va.lineFirst;
        if (foundVisibleLine >= firstAreaLine && foundVisibleLine + foundDepth <= firstAreaLine + va.depth) {
            sn.vertical = false;
            sn.firstVisible = firstVisibleLine;
        }
        else {
            sn.vertical = true;
            Scintilla::Line marginVertical = std::min(va.depth / 4, (va.depth - foundDepth) / 2);
            sn.firstVisible = std::max(Scintilla::Line(0), foundVisibleLine - marginVertical + subLineOffset - va.lineFirst);
        }
    }
    else {
        sn.firstVisible = std::max(Scintilla::Line(0), foundVisibleLine + subLineOffset - va.lineFirst);
    }
    if (wrap) {
        sn.horizontal = false;
        sn.clearHorizontal = 0;
        sn.xOffset = 0;
        return sn;
    }
    sn.clearHorizontal = va.width - pxEnd + pxStart;
    if (pxStart >= va.rect.left && pxEnd <= va.rect.right) {
        sn.horizontal = false;
        sn.xOffset = pxOffset;
        return sn;
    }
    sn.horizontal = true;
    if (pxStart + pxOffset >= va.rect.left && pxEnd + pxOffset <= va.rect.right) {
        sn.xOffset = 0;
        return sn;
    }
    if (sn.clearHorizontal <= 0) {
        sn.xOffset = pxEnd + pxOffset - va.rect.right;
        return sn;
    }
    sn.xOffset = pxStart - pxLine - std::min(sn.clearHorizontal / 2, 5 * pxBlank);
    return sn;
}

}


void scrollIntoView(HWND scintilla, HWND avoid, Scintilla::Position foundStart, Scintilla::Position foundEnd, bool select) {

    sci.SetFnPtr(
        reinterpret_cast<Scintilla::FunctionDirect>
            (SendMessage(scintilla, static_cast<UINT>(Scintilla::Message::GetDirectStatusFunction), 0, 0)),
        SendMessage(scintilla, static_cast<UINT>(Scintilla::Message::GetDirectPointer), 0, 0));
    sci.SetStatus(Scintilla::Status::Ok);  // C-interface code can ignore an error status, causing exception in C++ interface

    GetClientRect(scintilla, &scintillaRect);
    int marginCount = sci.Margins();
    for (int i = 0; i < marginCount; ++i) scintillaRect.left += sci.MarginWidthN(i);
    scintillaRect.left += sci.MarginLeft();
    scintillaRect.right -= sci.MarginRight();
    firstVisibleLine = sci.FirstVisibleLine();
    pxOffset = sci.XOffset();
    pxBlank = sci.TextWidth(32, " ");

    if (avoid) GetWindowRect(avoid, &dialogRect);
    MapWindowPoints(0, scintilla, reinterpret_cast<POINT*>(&dialogRect), 2);

    ScrollTarget st(foundStart, foundEnd);
    ScrollNeeded sn;

    if (dialogRect.left >= scintillaRect.right || dialogRect.right <= scintillaRect.left
        || dialogRect.top >= scintillaRect.bottom || dialogRect.bottom <= scintillaRect.top) {
        sn = st.find(VisibleArea(scintillaRect.left, scintillaRect.top, scintillaRect.right, scintillaRect.bottom));
    }
    else {
        VisibleArea vaTop   (scintillaRect.left, scintillaRect.top, scintillaRect.right, dialogRect   .top);
        VisibleArea vaLeft  (scintillaRect.left, scintillaRect.top, dialogRect.left    , scintillaRect.bottom);
        VisibleArea vaBottom(scintillaRect.left, dialogRect.bottom, scintillaRect.right, scintillaRect.bottom);
        VisibleArea vaRight (dialogRect.right  , scintillaRect.top, scintillaRect.right, scintillaRect.bottom);
        ScrollNeeded snTop    = st.find(vaTop   );
        ScrollNeeded snLeft   = st.find(vaLeft  );
        ScrollNeeded snBottom = st.find(vaBottom);
        ScrollNeeded snRight  = st.find(vaRight );
        sn = snTop;
        if (snLeft  .betterThan(sn)) sn = snLeft;
        if (snBottom.betterThan(sn)) sn = snBottom;
        if (snRight.betterThan(sn)) sn = snRight;
    }

    if (sn.vertical) sci.SetFirstVisibleLine(sn.firstVisible);
    if (sn.horizontal) sci.SetXOffset(sn.xOffset);
    if (select) {
        sci.SetSel(foundStart, foundEnd);
        sci.ChooseCaretX();
    }

}

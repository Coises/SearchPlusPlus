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

namespace {

RECT            scintillaRect;
RECT            dialogRect;
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


void scrollIntoView(Scintilla::Position foundStart, Scintilla::Position foundEnd, bool select) {

    HWND currentScintilla = plugin.currentScintilla();
    plugin.getScintillaPointers(currentScintilla);
    GetClientRect(currentScintilla, &scintillaRect);
    int marginCount = sci.Margins();
    for (int i = 0; i < marginCount; ++i) scintillaRect.left += sci.MarginWidthN(i);
    scintillaRect.left += sci.MarginLeft();
    scintillaRect.right -= sci.MarginRight();
    firstVisibleLine = sci.FirstVisibleLine();
    pxOffset = sci.XOffset();
    pxBlank = sci.TextWidth(STYLE_DEFAULT, " ");

    GetWindowRect(data.searchDialog, &dialogRect);
    MapWindowPoints(0, currentScintilla, reinterpret_cast<POINT*>(&dialogRect), 2);

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

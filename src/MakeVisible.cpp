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

#include "Framework/UtilityFramework.h"
#include "HiddenLines.h"
#include "NppHideLinesConstants.h"

// *** All routines in this file must be called with sci already set to access the appropriate Scintilla control. ***
//
// makeLinesVisible shows lines from lnMin to lnMax,
// and adjusts folding and hidden line markers to be consistent.
// 
// makeCharsVisible shows lines containing characters from cpMix to cpMax,
// or the line containing the position cpMin if cpMin and cpMax are equal,
// and adjusts folding and hidden line markers to be consistent.
//
// hideAllLines makes all lines hidden, removes existing hidden line markers
// and puts a single pair of hidden line markers on the first and last lines.
//
// showAllLines expands any folded lines, makes all lines visible
// and removes all hidden line markers.


void makeLinesVisible(Scintilla::Line lnMin, Scintilla::Line lnMax) {

    if (sci.AllLinesVisible()) return;
    if (lnMax < lnMin) std::swap(lnMin, lnMax);

    Scintilla::Line lineCount = sci.LineCount();
    Scintilla::Line originalLastVisible = sci.VisibleFromDocLine(lineCount - 1);
    bool anyHidden = false;
    for (Scintilla::Line line = lnMin; line <= lnMax; ++line) if (!sci.LineVisible(line)) {
        anyHidden = true;
        sci.EnsureVisible(line);
    }
    if (!anyHidden) return;
    bool anyExpanded = sci.VisibleFromDocLine(lineCount - 1) != originalLastVisible;

    sci.ShowLines(lnMin, lnMax);
    for (Scintilla::Line line = lnMin; line <= lnMax; ++line) {
        sci.MarkerDelete(line, HideBegin);
        sci.MarkerDelete(line, HideEnd  );
    }

    Scintilla::Line beforeLine = sci.MarkerPrevious(lnMin, HideMaskBegin | HideMaskEnd);
    Scintilla::Line afterLine  = sci.MarkerNext(lnMax, HideMaskBegin | HideMaskEnd);
    int beforeMark = beforeLine < 0 ? 0 : sci.MarkerGet(beforeLine);
    int afterMark  = afterLine  < 0 ? 0 : sci.MarkerGet(afterLine);
    if (beforeMark & HideMaskBegin) {
        if (beforeLine == lnMin - 1) sci.MarkerDelete(beforeLine, HideBegin);
        else sci.MarkerAdd(lnMin, HideEnd);
    }
    if (afterMark & HideMaskEnd) {
        if (afterLine == lnMax + 1) sci.MarkerDelete(afterLine, HideEnd);
        else sci.MarkerAdd(lnMax, HideBegin);
    }

    HiddenLines& hidden = HiddenLines::current();
    if (lnMin == 0) hidden.first = false;
    if (lnMax == lineCount - 1) hidden.last = false;
    if (anyExpanded) HiddenLines::applyMarkers();  // Expanding folds can cause hidden blocks to be incorrectly shown
    
}


void makeCharsVisible(Scintilla::Position cpMin, Scintilla::Position cpMax) {
    if (sci.AllLinesVisible()) return;
    if (cpMax < cpMin) std::swap(cpMin, cpMax);
    Scintilla::Line lnMin = sci.LineFromPosition(cpMin);
    Scintilla::Line lnMax = cpMax - cpMin < 2 ? lnMin : sci.LineFromPosition(cpMax - 1);
    makeLinesVisible(lnMin, lnMax);
}


void hideAllLines() {
    Scintilla::Line lastLine = sci.LineCount() - 1;
    sci.HideLines(0, lastLine);
    sci.MarkerDeleteAll(HideBegin);
    sci.MarkerDeleteAll(HideEnd);
    sci.MarkerAdd(0, HideBegin);
    sci.MarkerAdd(lastLine, HideEnd);
    HiddenLines& hidden = HiddenLines::current();
    hidden.first = hidden.last = true;
    (HiddenLines::active2 ? hidden.peek2 : hidden.peek1) = false;

}


void showAllLines() {
    sci.FoldAll(Scintilla::FoldAction::Expand);
    sci.ShowLines(0, sci.LineCount() - 1);
    sci.MarkerDeleteAll(HideBegin);
    sci.MarkerDeleteAll(HideEnd);
    if (HiddenLines::active) {
        HiddenLines& hidden = *HiddenLines::active;
        hidden.first = hidden.last = false;
        (HiddenLines::active2 ? hidden.peek2 : hidden.peek1) = false;
    }
}
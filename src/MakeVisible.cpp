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

    if (!anyExpanded) return;

    // Expanding folds can cause lines to be shown that have nothing to do with this show; just check everything

    Scintilla::Line line1 = sci.MarkerNext(0, HideMaskBegin | HideMaskEnd);
    if (line1 < 0) return;
    bool firstMark = true;
    int mark1 = sci.MarkerGet(line1);
    for (;;) {
        Scintilla::Line line2 = line1 + 1 >= lineCount ? -1 : sci.MarkerNext(line1 + 1, HideMaskBegin | HideMaskEnd);
        int mark2 = line2 == -1 ? 0 : sci.MarkerGet(line2);
        if (mark1 & HideMaskBegin) {
            if (mark2 & HideMaskEnd) {
                if (line2 - line1 > 1) sci.HideLines(line1 + 1, line2 - 1);
                if (mark2 & HideMaskBegin) {
                    line1 = line2;
                    mark1 = mark2;
                }
                else {
                    line1 = line2 + 1 >= lineCount ? -1 : sci.MarkerNext(line2 + 1, HideMaskBegin | HideMaskEnd);
                    if (line1 < 0) break;
                    mark1 = sci.MarkerGet(line1);
                }
            }
            else if (line2 < 0) /* Orphan hide begin followed by end of document; extend to the end */ {
                if (line1 < lineCount - 1) {
                    sci.HideLines(line1 + 1, lineCount - 1);
                    sci.MarkerAdd(lineCount - 1, HideEnd);
                }
                else sci.MarkerDelete(lineCount - 1, HideBegin);
                break;
            }
            else /* orphan hide begin followed by another hide begin; remove it */ {
                sci.MarkerDelete(line1, HideBegin);
                line1 = line2;
                mark1 = mark2;
            }
        }
        else /*orphan hide end */ {
            if (line1 == 0) sci.MarkerDelete(0, HideEnd);
            else if (firstMark) /* Orphan hide end follows document start */ {
                sci.HideLines(0, line1 - 1);
                sci.MarkerAdd(0, HideBegin);
            }
            else sci.MarkerDelete(0, HideEnd);
            line1 = line2;
            mark1 = mark2;
        }
        firstMark = false;
    }
    
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
}


void showAllLines() {
    sci.FoldAll(Scintilla::FoldAction::Expand);
    sci.ShowLines(0, sci.LineCount() - 1);
    sci.MarkerDeleteAll(HideBegin);
    sci.MarkerDeleteAll(HideEnd);
}
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

#include "HiddenLines.h"
#include "NppHideLinesConstants.h"


namespace {

    void getScintillaPointers() {
        plugin.getScintillaPointers(
            HiddenLines::active2 ? plugin.nppData._scintillaSecondHandle : plugin.nppData._scintillaMainHandle);
    }

}


void HiddenLines::applyMarkers() {

    Scintilla::Line line1 = sci.MarkerNext(0, HideMaskBegin | HideMaskEnd);
    if (line1 < 0) return;

    Scintilla::Line lineCount = sci.LineCount();
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
            else sci.MarkerDelete(line1, HideEnd);
            if (line1 + 1 >= lineCount) break;
            line1 = line2;
            mark1 = mark2;
        }

        firstMark = false;

    }

}


bool HiddenLines::indicated() {
    if (active && (active->first || active->last)) return true;
    getScintillaPointers();
    if (sci.MarkerNext(0, HideMaskBegin | HideMaskEnd) >= 0) return true;
    return sci.ContractedFoldNext(0) >= 0;
}


void HiddenLines::peek() {
    current();
    (active2 ? active->peek2 : active->peek1) = true;
    getScintillaPointers();
    Scintilla::Line lastLine = sci.LineCount() - 1;
    sci.ShowLines(0, lastLine);
}


void HiddenLines::restore() {
    if (!active) return;
    (active2 ? active->peek2 : active->peek1) = false;
    getScintillaPointers();
	Scintilla::Line lastLine = sci.LineCount() - 1;
	sci.ShowLines(0, lastLine);
    for (Scintilla::Line line = sci.ContractedFoldNext(0); line >= 0; line = sci.ContractedFoldNext(line + 1))
        sci.FoldLine(line, Scintilla::FoldAction::Contract);
    applyMarkers();
    if (active->first) sci.HideLines(0, 0);
    if (active->last) sci.HideLines(lastLine, lastLine);
}


void HiddenLines::restoreView() {
    if (!active) return;
    if (peeking()) {
	    getScintillaPointers();
	    Scintilla::Line lastLine = sci.LineCount() - 1;
        sci.ShowLines(0, lastLine);
    }
    else if (active->first || active->last) {
	    getScintillaPointers();
        if (active->first) sci.HideLines(0, 0);
        if (active->last) {
	        Scintilla::Line lastLine = sci.LineCount() - 1;
            sci.HideLines(lastLine, lastLine);
        }
    }
}

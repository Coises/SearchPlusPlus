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

void colorHitlist();
void colorSearch();

void scnFocusIn(const Scintilla::NotificationData* scnp) {
    if (zlmIndicator != 0) {
        plugin.getScintillaPointers(scnp);
        sci.SetIndicatorCurrent(zlmIndicator);
        sci.IndicatorClearRange(0, sci.Length());
    }
}

void scnModified(const Scintilla::NotificationData* scnp) {
    if (Scintilla::FlagSet(scnp->modificationType, Scintilla::ModificationFlags::InsertText | Scintilla::ModificationFlags::DeleteText))
        data.context.clear(false);
}

void scnUpdateUI(const Scintilla::NotificationData* scnp) {
    if (data.context.none()) return;
    if (!Scintilla::FlagSet(scnp->updated, Scintilla::Update::Selection)) return;
    // scnUpdateUI is called when something might have changed, not only when it actually has changed, so we must test.
    plugin.getScintillaPointers(scnp);
    data.context.checkSelection();
}

void bufferActivated() {
    data.context.clear();
}

void darkModeChanged() {
    colorHitlist();
    colorSearch();
}

void modifyAll(const NMHDR*) {
    data.context.clear();
}

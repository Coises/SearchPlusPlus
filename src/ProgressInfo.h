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

#include "Framework/PluginFramework.h"
#include "Framework/UnicodeFormatTranslation.h"
#include "Search.h"

class ProgressiveDocumentsList;

struct ProgressInfo {

    // A HitLine contains a list of every hit that begins in that line.
    // The text of the line includes the line ending.
    // A HitLine can include zero hits if it is included because a hit
    // on an earlier line extends into it.

    struct HitLine {
        std::vector<Scintilla::CharacterRangeFull> hits;
        std::string text;
        Scintilla::Line line;
        Scintilla::Position position;
        size_t count() const { return hits.size(); }
    };

    // A HitBlock contains all the hits from a single search in a single file.

    struct HitBlock {
        std::vector<HitLine> hitLines;
        std::string documentPath;
        UINT_PTR    bufferID;
        UINT        codepage;
        size_t count() const { size_t n = 0; for (const auto& x : hitLines) n += x.count(); return n; }
    };

    // A HitSet contains all the HitBlocks from a single search.
    // Ownership of a HitSet is passed to the HitList routines
    // after a search is completed.

    struct HitSet {
        std::vector<HitBlock> hitBlocks;
        std::string searchString;
        void add(Scintilla::Position cpMin, Scintilla::Position cpMax) { add(cpMin, cpMax, 0, 0); }
        void add(Scintilla::Position cpMin, Scintilla::Position cpMax, UINT_PTR bufferID, UINT codepage);
        size_t count() const { size_t n = 0; for (const auto& x : hitBlocks) n += x.count(); return n; }
    };

    std::unique_ptr<HitSet> hitSet;

    ProgressiveDocumentsList* pdl = 0;

    SearchRequest& req;
    SearchResult   result;
    std::wstring   message;

    Scintilla::Position position      = 0;
    Scintilla::Position rangeStart    = 0;
    Scintilla::Position rangeEnd      = 0;
    size_t              rangeIndex    = 0;
    size_t              documentIndex = 0;
    size_t              documentCount = 0;
    intptr_t            count         = 0;
    bool                timerStarted  = false;

    bool (*task)(ProgressInfo&) = 0;
    void (*prep)(ProgressInfo&) = 0;

    ProgressInfo(SearchRequest& req) : req(req) {}

    SearchResult exec(bool (*worker)(ProgressInfo&));
    SearchResult openDocuments(bool (*worker)(ProgressInfo&), void (*prepare)(ProgressInfo&));
    void nextDocument();

};

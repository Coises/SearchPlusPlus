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

#pragma once

#include "Framework/PluginFramework.h"
#include "Framework/UnicodeFormatTranslation.h"
#include "SearchRequest.h"
#include "MatchResults.h"

class ProgressiveDocumentsList;

struct ProgressInfo {

    // matchResults contains a cumulative list of all matches found in a search.
    // It is consumed by ShowHitList(MatchResults).
    // 
    // documentMatches contains information on hits in the current document needed
    // to append to matchResults after searching is finished for that document.
    // appendDocumentMatches processes it.

    MatchResults matchResults;

    struct DocumentMatches {
        struct LineIndex {
            std::vector<MatchResults::LineIndex::SingleMatch> matches;   // list of all matches that begin in this line
            intptr_t lineNumber;
            intptr_t position;
            intptr_t length;
        };
        std::vector<LineIndex> index;
        void add(Scintilla::Position cpMin, Scintilla::Position cpMax);
    } documentMatches;
    
    ProgressiveDocumentsList* pdl = 0;

    SearchRequest& req;
    SearchResult   result;
    std::wstring   message;

    Scintilla::Position position         = 0;
    Scintilla::Position rangeStart       = 0;
    Scintilla::Position rangeEnd         = 0;
    size_t              rangeIndex       = 0;
    size_t              documentIndex    = 0;
    size_t              documentCount    = 0;
    intptr_t            count            = 0;
    intptr_t            countEmpty       = 0;          // Used for Mark commands, since empty matches cannot be marked
    bool                timerStarted     = false;
    bool                needPreClear     = true;

    bool (*task)(ProgressInfo&) = 0;
    void (*prep)(ProgressInfo&) = 0;

    ProgressInfo(SearchRequest& req) : req(req) {}

    SearchResult exec(bool (*worker)(ProgressInfo&));
    SearchResult openDocuments(bool (*worker)(ProgressInfo&), void (*prepare)(ProgressInfo&));

    void appendDocumentMatches();
    void nextDocument();
    void preClear();
    void reserveSearchHeader();
    void updateSearchHeader(size_t documentsMatched);

};

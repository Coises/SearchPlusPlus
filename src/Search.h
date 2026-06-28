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

#include "Framework/UtilityFramework.h"
#include "RegularExpression.h"
#include "Calculation.h"

extern int zlmIndicator;   // Scintilla indicator used to show zero length matches


struct SearchCommand {
    enum Verb   : uint8_t { None, Find, Replace, ReplStop, Count, FindAll, Mark, Select, Show, ReplaceAll } verb;
    enum Extent : uint8_t { Forward, Backward, All, Before, After, View, Open } extent;
    enum Scope  : uint8_t { Smart, Region, Selection, Whole } scope;
    uint8_t unused = 0;
    constexpr SearchCommand() : verb(None), extent(Forward), scope(Smart) {}
    constexpr SearchCommand(Verb verb, Extent extent = Forward, Scope scope = Smart) : verb(verb), extent(extent), scope(scope) {}
    constexpr SearchCommand(unsigned int c)
        : verb(static_cast<Verb>(c & 15)), extent(static_cast<Extent>((c >> 8) & 15)), scope(static_cast<Scope>((c >> 16) & 15)) {
    }
    constexpr operator unsigned int() { return verb | (extent << 8) | (scope << 16); }
};


class SearchContext {

    Scintilla::CharacterRangeFull lastSelection;
    enum Status { None, NotFound, Found, Replaced } lastStatus = None;

public:

    RegularExpression   rx;
    Calculation         calc;
    bool                calcIsValid  = false;

    void   clear      (bool clearCalc = true)       { lastStatus = None; if (clearCalc) calcIsValid = false; }
    void   setFound   (                     )       { lastSelection = { sci.SelectionStart(), sci.SelectionEnd() }; lastStatus = Found;    }
    void   setNotFound(                     )       { lastSelection = { sci.SelectionStart(), sci.SelectionEnd() }; lastStatus = NotFound; }
    void   setReplaced(                     )       { lastSelection = { sci.SelectionStart(), sci.SelectionEnd() }; lastStatus = Replaced; }
    bool   none       (                     ) const { return lastStatus == None;     }
    bool   notFound   (                     ) const { return lastStatus == NotFound; }
    bool   found      (                     ) const { return lastStatus == Found;    }
    bool   replaced   (                     ) const { return lastStatus == Replaced; }
    bool   nullAt     (Scintilla::Position p) const { return lastStatus == Found && p == lastSelection.cpMin && p == lastSelection.cpMax; }
    Status status     (                     ) const { return lastStatus; }

    void checkSelection() {
        Scintilla::Position cpMin = sci.SelectionStart();
        Scintilla::Position cpMax = sci.SelectionEnd();
        if (lastSelection.cpMin != cpMin || lastSelection.cpMax != cpMax) {
            lastStatus = None;
            sci.CallTipCancel();
            if (zlmIndicator != 0) {
                sci.SetIndicatorCurrent(zlmIndicator);
                sci.IndicatorClearRange(0, sci.Length());
            }
        }
    }

    Scintilla::Position start() const { return lastStatus == None || lastStatus == NotFound ? -1 : lastSelection.cpMin; }
    Scintilla::Position end  () const { return lastStatus == None || lastStatus == NotFound ? -1 : lastSelection.cpMax; }

};


struct SearchResult {

    std::wstring message;
    std::string  findMessage;
    std::string  replMessage;
    enum Status { Success, Failure, Error } status = Error;

    SearchResult() {}
    SearchResult(const std::wstring& message, const std::string& findMessage = "", const std::string& replMessage = "")
        : status(Error), message(message), findMessage(findMessage), replMessage(replMessage) {}
    SearchResult(Status status, const std::wstring& message, const std::string& findMessage = "", const std::string& replMessage = "")
        : status(status), message(message), findMessage(findMessage), replMessage(replMessage) {}

    bool success() const { return status == Success; }
    bool failure() const { return status == Failure; }
    bool error  () const { return status == Error;   }

};

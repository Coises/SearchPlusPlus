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

#include "unicode/errorcode.h"
#include "unicode/regex.h"
#include "unicode/ustring.h"


namespace {

struct ProgressInfoICU : ProgressInfo {
    std::unique_ptr<icu::RegexMatcher> icuMatcher;
    std::string repl;
    Scintilla::Position offset1 = 0;
    Scintilla::Position offset2 = 0;
    ProgressInfoICU(SearchRequest& req) : ProgressInfo(req) {}
};

       
std::wstring rxError(UErrorCode ec, const std::wstring genericErrorText = L"An unknown error has occurred.") {
    static const std::map<UErrorCode, std::wstring> rxErrorList {
        {U_REGEX_RULE_SYNTAX                 , L"Syntax error in regexp pattern."                                },
        {U_REGEX_INVALID_STATE               , L"RegexMatcher in invalid state for requested operation."         },
        {U_REGEX_BAD_ESCAPE_SEQUENCE         , L"Unrecognized backslash escape sequence in pattern"              },
        {U_REGEX_PROPERTY_SYNTAX             , L"Incorrect Unicode property"                                     },
        {U_REGEX_UNIMPLEMENTED               , L"Use of regexp feature that is not yet implemented."             },
        {U_REGEX_MISMATCHED_PAREN            , L"Incorrectly nested parentheses in regexp pattern."              },
        {U_REGEX_NUMBER_TOO_BIG              , L"Decimal number is too large."                                   },
        {U_REGEX_BAD_INTERVAL                , L"Error in {min,max} interval"                                    },
        {U_REGEX_MAX_LT_MIN                  , L"In {min,max}, max is less than min."                            },
        {U_REGEX_INVALID_BACK_REF            , L"Back-reference to a non-existent capture group."                },
        {U_REGEX_INVALID_FLAG                , L"Invalid value for match mode flags."                            },
        {U_REGEX_LOOK_BEHIND_LIMIT           , L"Look-Behind pattern matches must have a bounded maximum length."},
        {U_REGEX_SET_CONTAINS_STRING         , L"Regexps cannot have UnicodeSets containing strings."            },
        {U_REGEX_OCTAL_TOO_BIG               , L"Octal character constants must be <= 0377."                     },
        {U_REGEX_MISSING_CLOSE_BRACKET       , L"Missing closing bracket on a bracket expression."               },
        {U_REGEX_INVALID_RANGE               , L"In a character range [x-y], x is greater than y."               },
        {U_REGEX_STACK_OVERFLOW              , L"Regular expression backtrack stack overflow."                   },
        {U_REGEX_TIME_OUT                    , L"Maximum allowed match time exceeded"                            },
        {U_REGEX_STOPPED_BY_CALLER           , L"Matching operation aborted by user callback fn."                },
        {U_REGEX_PATTERN_TOO_BIG             , L"Pattern exceeds limits on size or complexity."                  },
        {U_REGEX_INVALID_CAPTURE_GROUP_NAME  , L"Invalid capture group name."                                    }
    };
    return rxErrorList.contains(ec) ? rxErrorList.at(ec) : genericErrorText;
}


class UTextObject : public UText {
public:
    UTextObject()                                          { *static_cast<UText*>(this) = UTEXT_INITIALIZER; }
    ~UTextObject()                                         { utext_close(this); }
    int64_t    getNativeIndex(                     ) const { return utext_getNativeIndex(this); }
    UChar32    next32From    (int64_t start        )       { return utext_next32From(this, start); }
    UErrorCode openUTF8      (char* cs, int64_t len)       { UErrorCode ec; utext_openUTF8(this, cs, len, &ec); return ec; }
    UErrorCode openUChars    (UChar* cs, int64_t len)      { UErrorCode ec; utext_openUChars(this, cs, len, &ec); return ec; }
};


SearchResult singleFind(SearchRequest& req, bool postReplace = false) {

    bool inSelection = !postReplace && req.command.scope == SearchCommand::Selection;
    bool wrap        = req.context->notFound();

    plugin.getScintillaPointers(req.sciFind);
    std::string find = sci.GetText(sci.Length());
    uint32_t flags = UREGEX_MULTILINE;
    if ( data.dotAll      ) flags |= UREGEX_DOTALL;
    if ( data.freeSpacing ) flags |= UREGEX_COMMENTS;
    if (!data.matchCase   ) flags |= UREGEX_CASE_INSENSITIVE;
    if ( data.uniWordBound) flags |= UREGEX_UWORD;
    icu::ErrorCode status;
    icu::RegexMatcher matcher(find.data(), flags, status);
    if (status.isFailure()) return req.error(L"Invalid regular expression.", utf16to8(rxError(status.get(), L"")));

    plugin.getScintillaPointers(req.sciText);
    Scintilla::Position searchFrom = wrap ? 0 : inSelection ? req.ranges.front().cpMin : sci.SelectionEnd();

    for (size_t range = 0; range < req.ranges.size(); ++range) {
        auto& r = req.ranges[range];
        if (searchFrom >= r.cpMax) continue;
        UTextObject body;
        body.openUTF8(reinterpret_cast<char*>(sci.RangePointer(r.cpMin, r.cpMax)), r.cpMax - r.cpMin);
        matcher.reset(&body);
        matcher.reset(std::max(searchFrom, r.cpMin) - r.cpMin, status);
        if (status.isFailure()) return req.error(rxError(status.get(), L"Failed to set search position."));
        bool found = matcher.find(status);
        if (status.isFailure()) return req.error(rxError(status.get(), L"Search failed."));
        if (found) {
            Scintilla::Position matchStart = r.cpMin + static_cast<Scintilla::Position>(matcher.start64(status));
            Scintilla::Position matchEnd   = r.cpMin + static_cast<Scintilla::Position>(matcher.end64(status));
            if (matchStart == matchEnd && req.context->nullAt(matchStart)) {
                if (matchStart == r.cpMax) continue;
                matcher.reset(matchStart - r.cpMin + 1, status);
                if (status.isFailure()) return req.error(rxError(status.get(), L"Failed to set search position."));
                found = matcher.find(status);
                if (status.isFailure()) return req.error(rxError(status.get(), L"Search failed."));
                if (!found) continue;
                matchStart = r.cpMin + static_cast<Scintilla::Position>(matcher.start64(status));
                matchEnd   = r.cpMin + static_cast<Scintilla::Position>(matcher.end64(status));
            }
            return req.found(matchStart, matchEnd, L"Match found.");
        }
    }

    if (postReplace) return req.endRepl(L"Match replaced; no more matches found.");
    if (searchFrom <= req.ranges.front().cpMin) return req.notFound(
            req.command.scope == SearchCommand::Selection ? L"No matches found in selection."
          : req.command.scope == SearchCommand::Region    ? L"No matches found in marked text."
                                                          : L"No matches found in entire document.");
    else return req.notFound(
            req.command.scope == SearchCommand::Selection ? L"No matches found in selection."
          : req.command.scope == SearchCommand::Region    ? L"No matches found. (Find again to search all marked text.)"
                                                          : L"No matches found. (Find again to search entire document.)");

}


bool progressiveSearch(ProgressInfo& pi) {

    auto& count    = pi.count;
    auto& position = pi.position;
    auto& rangeEnd = pi.rangeEnd;
    auto& req      = pi.req;

    ProgressInfoICU& pii = static_cast<ProgressInfoICU&>(pi);
    auto& matcher = *pii.icuMatcher;

    if (pi.rangeIndex >= req.ranges.size()) return false;
    Scintilla::CharacterRangeFull r = req.ranges[pi.rangeIndex];
    Scintilla::Position scanMax = std::min(rangeEnd, r.cpMax);
    if (position > scanMax) return false;

    icu::ErrorCode status;
    UTextObject body;
    body.openUTF8(reinterpret_cast<char*>(sci.RangePointer(r.cpMin, r.cpMax)), r.cpMax - r.cpMin);
    matcher.reset(&body);
    matcher.reset(position + pii.offset2 - r.cpMin, status);
    if (status.isFailure()) return false;
    bool found = matcher.find(status);
    if (status.isFailure()) return false;
    if (found) {
        Scintilla::Position matchStart = r.cpMin + static_cast<Scintilla::Position>(matcher.start64(status));
        Scintilla::Position matchEnd   = r.cpMin + static_cast<Scintilla::Position>(matcher.end64(status));
        position = matchEnd - pii.offset2;
        if (position > scanMax) return false;
        ++count;
        pii.preClear();
        switch (req.command.verb) {
        case SearchCommand::FindAll:
            pii.hitSet->add(matchStart, matchEnd);
            break;
        case SearchCommand::Select:
            if (count == 1 && sci.Selections() == 1 && sci.SelectionEmpty()) scrollIntoView(matchStart, matchEnd);
            else sci.AddSelection(matchEnd, matchStart);
            break;
        case SearchCommand::Show:
            sci.ShowLines(sci.LineFromPosition(matchStart), sci.LineFromPosition(matchEnd));
            [[fallthrough]];
        case SearchCommand::Mark:
            if (matchStart != matchEnd) {
                sci.SetIndicatorCurrent(data.indicator);
                sci.SetIndicatorValue(1);
                sci.IndicatorFillRange(matchStart, matchEnd - matchStart);
            }
            if (data.markAlsoBookmarks) {
                Scintilla::Line line = sci.LineFromPosition(matchStart);
                if (!(sci.MarkerGet(line) & (1 << data.bookMarker))) sci.MarkerAdd(line, data.bookMarker);
            }
            break;
        }
        if (matchStart == matchEnd) ++position;
    }

    if (!found || position >= r.cpMax) {
        if (++pi.rangeIndex >= req.ranges.size()) return false;
        pii.offset1 = pii.offset2;
        position = req.ranges[pi.rangeIndex].cpMin;
    }

    return position < rangeEnd;

}


SearchResult multipleSearch(SearchRequest& req) {
    plugin.getScintillaPointers(req.sciFind);
    Scintilla::Position findLength = sci.Length();
    if (findLength < 1) return req.error(L"Nothing to find.");
    ProgressInfoICU pii(req);
    std::string find = sci.GetText(sci.Length());
    uint32_t flags = UREGEX_MULTILINE;
    if (data.dotAll) flags |= UREGEX_DOTALL;
    if (data.freeSpacing) flags |= UREGEX_COMMENTS;
    if (!data.matchCase) flags |= UREGEX_CASE_INSENSITIVE;
    if (data.uniWordBound) flags |= UREGEX_UWORD;
    icu::ErrorCode status;
    pii.icuMatcher = std::make_unique<icu::RegexMatcher>(find.data(), flags, status);
    if (status.isFailure()) return req.error(L"Invalid regular expression.", utf16to8(rxError(status.get(), L"")));
    if (req.command.verb == SearchCommand::ReplaceAll) {
        plugin.getScintillaPointers(req.sciRepl);
        Scintilla::Position replLength = sci.Length();
        pii.repl = replLength ? sci.GetText(replLength) : "";
    }
    plugin.getScintillaPointers(req.sciText);
    sci.SetIndicatorCurrent(data.indicator);
    sci.SetIndicatorValue(1);
    pii.hitSet = std::make_unique<ProgressInfo::HitSet>();
    pii.hitSet->searchString = "(ICU Regex): " + find;
    pii.exec(progressiveSearch);
    return pii.result;
}

}


SearchResult searchICU(SearchRequest& req) {

    plugin.getScintillaPointers();
    if (sci.CodePage() != CP_UTF8) return SearchResult(L"The ICU engine cannot search ANSI documents.");

    switch (req.command.verb) {
    case SearchCommand::Find:
        switch (req.command.extent) {
        case SearchCommand::Forward:
            return singleFind(req);
        default:
            return SearchResult(L"Command not implemented.");
        }
    case SearchCommand::Count:
    case SearchCommand::FindAll:
    case SearchCommand::Mark:
    case SearchCommand::Select:
    case SearchCommand::Show:
        switch (req.command.extent) {
        case SearchCommand::All:
        case SearchCommand::Before:
        case SearchCommand::After:
            return multipleSearch(req);
        default:
            return SearchResult(L"Command not implemented.");
        }
    default:
        return SearchResult(L"Command not implemented.");
    }

}
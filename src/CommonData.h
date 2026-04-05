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
#include "Framework/ConfigFramework.h"
#include "Framework/UtilityFramework.h"
#include "ProgressInfo.h"
#include "Search.h"


// Define enumerations for use with config, and tell the JSON package how represent them in the configuration file

enum class DialogLayout {Docking, Horizontal, Vertical, Adaptive};
NLOHMANN_JSON_SERIALIZE_ENUM(DialogLayout, {
    {DialogLayout::Docking   , "docking"   },
    {DialogLayout::Horizontal, "horizontal"},
    {DialogLayout::Vertical  , "vertical"  },
    {DialogLayout::Adaptive  , "adaptive"  }
})

enum class SearchEngine {Plain, Boost, ICU};
NLOHMANN_JSON_SERIALIZE_ENUM(SearchEngine, {
    {SearchEngine::Plain, "plain" },
    {SearchEngine::Boost, "Boost" },
    {SearchEngine::ICU  , "ICU"   }
})

enum class CopyMarkedSeparator {None, Blank, Tab, Line, Custom};
NLOHMANN_JSON_SERIALIZE_ENUM(CopyMarkedSeparator, {
    {CopyMarkedSeparator::None  , "none"   },
    {CopyMarkedSeparator::Blank , "blank"  },
    {CopyMarkedSeparator::Tab   , "tab"    },
    {CopyMarkedSeparator::Line  , "line"   },
    {CopyMarkedSeparator::Custom, "custom" }
})

NLOHMANN_JSON_SERIALIZE_ENUM(Scintilla::Wrap, {
    {Scintilla::Wrap::None      , "none"},
    {Scintilla::Wrap::Word      , "word"},
    {Scintilla::Wrap::Char      , "char"},
    {Scintilla::Wrap::WhiteSpace, "space"}
})


// Common data structure

inline struct CommonData {

    HWND searchDialog  = 0;
    HWND dockingDialog = 0;
    HWND regularDialog = 0;
    bool dockingDialogIsDocked = false;

    SearchContext context;

    // Data to be saved in the configuration file

    config<DialogLayout> dialogLayout = { "dialog format", DialogLayout::Docking };

    config<bool> fillSearch            = { "fill from selection"           , true  }; // IDC_SETTINGS_FILL
    config<bool> fillSearchLimit       = { "limit fill from selection"     , true  }; // IDC_SETTINGS_FILL_LIMIT
    config<int > fillChars             = { "maximum characters to fill"    , 79    }; // IDC_SETTINGS_FILLCHARS_EDIT/SPIN
    config<int > fillLines             = { "maximum lines to fill"         , 2     }; // IDC_SETTINGS_FILLLINES_EDIT/SPIN
    config<bool> fillNothing           = { "fill from word"                , true  }; // IDC_SETTINGS_FILL_NOTHING
    config<bool> autoSearchSelect      = { "search selection"              , true  }; // IDC_SETTINGS_AUTOSEARCH_SELECTION
    config<bool> autoSearchSelectLimit = { "limit search selection"        , true  }; // IDC_SETTINGS_AUTOSEARCH_SELECTION_LIMIT
    config<int > selChars              = { "minimum characters to select"  , 80    }; // IDC_SETTINGS_SELCHARS_EDIT/SPIN
    config<int > selLines              = { "minimum lines to select"       , 3     }; // IDC_SETTINGS_SELLINES_EDIT/SPIN
    config<bool> selectionToMarks      = { "convert selection to marks"    , false }; // IDC_SETTINGS_TOMARKS
    config<int > indicator             = { "mark indicator"                , 31    }; // IDC_SETTINGS_MARKSTYLE
    config<bool> autoSearchMarked      = { "search marked"                 , true  }; // IDC_SETTINGS_AUTOSEARCH_MARKS
    config<bool> autoClearMarks        = { "automatic clear marks"         , false }; // IDC_SETTINGS_AUTOCLEAR_MARKS
    config<bool> focusStepwise         = { "focus document after step"     , false }; // IDC_SETTINGS_FOCUS_STEPWISE
    config<bool> focusSelect           = { "focus document after select"   , true  }; // IDC_SETTINGS_FOCUS_SELECT
    config<bool> focusShow             = { "focus document after show"     , true  }; // IDC_SETTINGS_FOCUS_SHOW
    config<bool> focusResults          = { "focus search results"          , true  }; // IDC_SETTINGS_FOCUS_RESULTS
    config<bool> clearSelections       = { "clear selections before select", true  }; // IDC_SETTINGS_CLEARSELECTIONS
    config<bool> clearMarked           = { "unmark before mark"            , false }; // IDC_SETTINGS_CLEARMARKED
    config<bool> hideBeforeShow        = { "hide before show"              , false }; // IDC_SETTINGS_HIDEBEFORESHOW

    config<CopyMarkedSeparator> copyMarkedSeparator     = { "copy marked separator"     , CopyMarkedSeparator::Line };
    config<std::string>         copyMarkedSeparatorText = { "copy marked separator text", ""                        };

    config_history historyFind = { "find history"   , {}, 12, config_history::Blank };
    config_history historyRepl = { "replace history", {}, 12, config_history::Blank };

    config<std::string> findBoxLast = { "find box last content", "" };
    config<std::string> replBoxLast = { "replace box last content", "" };

    config<bool           > dotAll       = { "period matches all"     , false                 };
    config<bool           > freeSpacing  = { "free spacing"           , false                 };
    config<bool           > matchCase    = { "match case"             , false                 };
    config<bool           > wholeWord    = { "match whole word only"  , false                 };
    config<bool           > uniWordBound = { "unicode word boundaries", true                  };

    config<Scintilla::Wrap> wrapFind     = { "wrap find"              , Scintilla::Wrap::Char };
    config<Scintilla::Wrap> wrapRepl     = { "wrap replace"           , Scintilla::Wrap::Char };
    config<Scintilla::Wrap> wrapHits     = { "wrap search results"    , Scintilla::Wrap::Char };
    config<int            > zoomFind     = { "zoom find"              , 0                     };
    config<int            > zoomRepl     = { "zoom replace"           , 0                     };
    config<int            > zoomHits     = { "zoom search results"    , 0                     };

    config<SearchEngine> searchEngine     = { "search engine"     , SearchEngine::Plain };
    config<unsigned int> buttonFind       = { "Find command"      , SearchCommand(SearchCommand::Find                          ) };
    config<unsigned int> buttonCount      = { "Count command"     , SearchCommand(SearchCommand::Count     , SearchCommand::All) };
    config<unsigned int> buttonFindAll    = { "FindAll command"   , SearchCommand(SearchCommand::FindAll   , SearchCommand::All) };
    config<unsigned int> buttonReplace    = { "Replace command"   , SearchCommand(SearchCommand::Replace                       ) };  
    config<unsigned int> buttonReplaceAll = { "ReplaceAll command", SearchCommand(SearchCommand::ReplaceAll, SearchCommand::All) };  

} data;

// This file is part of Search++.
// Copyright 2026 by by Randy Fellmy <https://www.coises.com/>.

// The source code contained in this file is independent of Notepad++ code.
// It is released under the MIT (Expat) license:
//
// Permission is hereby granted, free of charge, to any person obtaining a copy of this software and 
// associated documentation files (the "Software"), to deal in the Software without restriction, 
// including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, 
// and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, 
// subject to the following conditions:
// 
// The above copyright notice and this permission notice shall be included in all copies or substantial 
// portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT 
// LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
// IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, 
// WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE 
// SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

// Data for Search in Files functionality saved in the configuration file

#pragma once
#include "Framework/UnicodeFormatTranslation.h"
#include "ConfigEnums.h"
#include "SciControl.h"

struct SciConfig {
    config_history          history;
    config<std::string>     text;
    config<Scintilla::Wrap> wrap;
    config<int>             zoom;
    bool                    useTab;
    SciConfig(const char* name, bool useTab)
        : history (std::string(name) + " history"  , {}, 12, config_history::Blank)
        , text    (std::string(name) + " text"     , "")
        , wrap    (std::string(name) + " wrap"     , Scintilla::Wrap::Char)
        , zoom    (std::string(name) + " zoom"     , 0)
        , useTab  (useTab)
    {}
    void push() { if (!text.get().empty()) history += utf8to16(text.get()); }
};


inline struct UserConfigurationData {

    SciControl folderCntl = { "sif folder" , false };  // IDC_SIF_FOLDER
    SciControl filterCntl = { "sif filter" , false };  // IDC_SIF_FILTER
    SciControl findCntl   = { "sif find"   , true  };  // IDC_SIF_FIND
    SciControl replCntl   = { "sif replace", true  };  // IDC_SIF_REPL

    config_rect mainWindowPosition = { "sif main window position" };

    config<bool        > subfolders  = { "sif subfolders"       , true                   };  // IDC_SIF_SUBFOLDERS
    config<bool        > hidden      = { "sif hidden"           , false                  };  // IDC_SIF_HIDDEN
    config<bool        > sizeFilter  = { "sif filter by size"   , false                  };  // IDC_SIF_SIZE
    config<int         > sizeMin     = { "sif minimum size"     , 0                      };  // IDC_SIF_SIZE_MIN_EDIT/SPIN
    config<FileSizeUnit> sizeMinUnit = { "sif minimum size unit", FileSizeUnit::Bytes    };  // IDC_SIF_SIZE_MIN_TYPE
    config<int         > sizeMax     = { "sif maximum size"     , 100                    };  // IDC_SIF_SIZE_MAX_EDIT/SPIN
    config<FileSizeUnit> sizeMaxUnit = { "sif maximum size unit", FileSizeUnit::MiB      };  // IDC_SIF_SIZE_MAX_TYPE
    config<bool        > dateFilter  = { "sif filter by date"   , false                  };  // IDC_SIF_DATE
    config<FileDateType> dateType    = { "sif file date type"   , FileDateType::Modified };  // IDC_SIF_DATE_TYPE
    config<uint64_t    > dateMin     = { "sif minimum date"     , 0                      };  // IDC_SIF_DATE_MIN
    config<uint64_t    > dateMax     = { "sif maximum date"     , 428266464000000000     };  // IDC_SIF_DATE_MAX

    config<bool> regex        = { "sif regular expression"     , false };  // IDC_SIF_REGEX
    config<bool> dotAll       = { "sif period matches all"     , false };  // IDC_SIF_DOTALL     
    config<bool> freeSpacing  = { "sif free spacing"           , false };  // IDC_SIF_FREESPACING
    config<bool> matchCase    = { "sif match case"             , false };  // IDC_SIF_MATCHCASE  
    config<bool> wholeWord    = { "sif match whole word only"  , false };  // IDC_SIF_WHOLEWORD  

    config<FilterEngine> filterEngine = { "sif filter search engine", FilterEngine::Disabled };

} ucd;

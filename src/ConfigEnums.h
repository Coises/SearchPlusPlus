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

#pragma once

#include "Framework/ConfigFramework.h"
#include "Host/ScintillaTypes.h"

// Define enumerations for use with config, and tell the JSON package how represent them in the configuration file

enum class DialogLayout { Docking, Horizontal, Vertical, Adaptive };
NLOHMANN_JSON_SERIALIZE_ENUM(DialogLayout, {
    {DialogLayout::Docking   , "docking"   },
    {DialogLayout::Horizontal, "horizontal"},
    {DialogLayout::Vertical  , "vertical"  },
    {DialogLayout::Adaptive  , "adaptive"  }
    })

enum class SearchEngine { Plain, Boost, ICU };
NLOHMANN_JSON_SERIALIZE_ENUM(SearchEngine, {
    {SearchEngine::Plain, "plain" },
    {SearchEngine::Boost, "Boost" },
    {SearchEngine::ICU  , "ICU"   }
    })

enum class CopyMarkedSeparator { None, Blank, Tab, Line, Custom };
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

enum class FilterEngine { Disabled, Extension, Exclude, Boost };
NLOHMANN_JSON_SERIALIZE_ENUM(FilterEngine, {
    {FilterEngine::Disabled , "disabled" },
    {FilterEngine::Extension, "extension"},
    {FilterEngine::Exclude,   "exclude"  },
    {FilterEngine::Boost    , "Boost"    }
    })

enum class FileSizeUnit { Bytes, KiB, MiB, GiB };
NLOHMANN_JSON_SERIALIZE_ENUM(FileSizeUnit, {
    {FileSizeUnit::Bytes, "bytes"},
    {FileSizeUnit::KiB  , "KiB"  },
    {FileSizeUnit::MiB  , "MiB"  },
    {FileSizeUnit::GiB  , "GiB"  }
    })

enum class FileDateType { Created, Modified, Accessed };
NLOHMANN_JSON_SERIALIZE_ENUM(FileDateType, {
    {FileDateType::Created , "Created" },
    {FileDateType::Modified, "Modified"},
    {FileDateType::Accessed, "Accessed"}
    })

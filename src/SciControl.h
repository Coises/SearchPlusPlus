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

// SciControl centralizes setting up and accessing Scintilla controls
// and accessing and maintaining their user configuration state.

#pragma once

#include "Framework/ScintillaCallEx.h"
#include "Framework/ConfigFramework.h"


struct SciControl : Scintilla::ScintillaCall {

    struct Configuration {
        Scintilla::ColourAlpha caret            = Scintilla::ColourAlpha(0xFF000000);
        Scintilla::ColourAlpha caretLineBack    = Scintilla::ColourAlpha(0xFFFFFFFF);
        Scintilla::ColourAlpha selectionBack    = Scintilla::ColourAlpha(0xFFCCCCCC);
        Scintilla::ColourAlpha whiteSpace       = Scintilla::ColourAlpha(0xFF6AB5FF);
        std::string            defaultFont      = "Consolas";
        int                    defaultSize      = 1150;
        Scintilla::Colour      defaultFore      = Scintilla::Colour(0x000000);
        Scintilla::Colour      defaultBack      = Scintilla::Colour(0xFFFFFF);
        Scintilla::Colour      lineNumberFore   = Scintilla::Colour(0x808080);
        Scintilla::Colour      lineNumberBack   = Scintilla::Colour(0xE0E0E0);
        Scintilla::CaretStyle  caretStyle       = Scintilla::CaretStyle::Line;
        int                    caretWidth       = 1;
        int                    caretPeriod      = 500;
        int                    caretLineFrame   = 0;
        bool                   caretLineVisible = false;
    };

    config<std::vector<std::string>> history;           // history of text in the control when push() is called
    config<std::string>     text;                       // content of the control; only up-to-date after a history() or sync() call
    config<Scintilla::Wrap> configWrap;                 // wrap status; only up-to-date after a sync() call
    config<int>             configZoom;                 // zoom amount; only up-to-date after a sync() call
    int                     depth;                      // maximum history depth; 0 = do not use history or text
    bool                    acceptTab;                  // true if the use can type a tab into the control
    HWND                    handle;                     // window handle of the Scintilla control, set by attach(HWND)
    POINT                   lastLButtonDown{ -1, -1 };  // Last left mouse button down; needed for some double-click processing

    bool (*controlKey)(SciControl& sciCtrl, char key) = 0;                 // routine to intercept Ctrl+letter keys; return true if consumed
    bool (*enterKey  )(SciControl& sciCtrl, bool shift, bool control) = 0; // routine to intercept the Enter key; return true if consumed
    bool (*tabKey    )(SciControl& sciCtrl, bool shift, bool control) = 0; // routine to intercept the Tab key; return true if consumed

    // name is a prefix used when storing the settings in the json configuration file
    // acceptTab is true if the use can type a tab into the control, false if tab should navigate out of the control
    // historyDepth is the maximum number of entries to be kept in the history

    SciControl(const char* name, bool acceptTab = false, int depth = 12)
        : history   (std::string(name) + " history", {})
        , text      (std::string(name) + " text"   , "")
        , configWrap(std::string(name) + " wrap"   , Scintilla::Wrap::Char)
        , configZoom(std::string(name) + " zoom"   , 0)
        , acceptTab(acceptTab), depth(depth), handle(0)
    {}

    void attach(HWND scintillaHandle);                    // Connects this structure to a Scintilla control
    void configure(const Configuration& configuration);   // Sets up styles, colors, etc. based on a configuration
    void configure(Scintilla::ScintillaCall& reference);  // Sets up styles and colors based on another Scintilla control
    void push();                                          // Update text to match the control and add text to the history
    void sync();                                          // Update text, configWrap and configZoom to match the control

    void subclass(                                         // Apply subclass:
        bool (*controlKey)(SciControl& sciCtrl, char key                ) = 0,  // intercept Ctrl+letter keys; return true if consumed
        bool (*tabKey    )(SciControl& sciCtrl, bool shift, bool control) = 0,  // intercept the Tab key; return true if consumed
        bool (*enterKey  )(SciControl& sciCtrl, bool shift, bool control) = 0   // intercept the Enter key; return true if consumed
    );

};

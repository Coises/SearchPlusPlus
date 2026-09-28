// This file is part of Search++ (a plugin for Notepad++),
// Copyright 2026 by Randy Fellmy <https://www.coises.com/>.

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

#define NOMINMAX
#include <windows.h>

class MonitoredTrackPopupMenu {
public:
    int  choice = 0;
    bool right  = false;
    bool shift  = false;
    int show(HMENU hMenu, UINT uFlags, int x, int y, HWND hwnd, LPTPMPARAMS lptpm) {
        choice = 0;
        right  = false;
        shift  = false;
        active = this;
        HHOOK hook = SetWindowsHookEx(WH_MSGFILTER, MessageProc, 0, GetCurrentThreadId());
        choice = TrackPopupMenuEx(hMenu, uFlags, x, y, hwnd, lptpm);
        UnhookWindowsHookEx(hook);
        active = 0;
        return choice;
    }
    MonitoredTrackPopupMenu() {}
    MonitoredTrackPopupMenu(HMENU hMenu, UINT uFlags, int x, int y, HWND hwnd, LPTPMPARAMS lptpm)
        { show(hMenu, uFlags, x, y, hwnd, lptpm); }
private:
    static MonitoredTrackPopupMenu* active;
    static LRESULT CALLBACK MessageProc(int code, WPARAM wParam, LPARAM lParam) {
        if (code == MSGF_MENU && active) {
            const MSG& msg = *reinterpret_cast<MSG*>(lParam);
            if      (msg.message == WM_LBUTTONDOWN || msg.message == WM_LBUTTONUP) active->right = false;
            else if (msg.message == WM_RBUTTONDOWN || msg.message == WM_RBUTTONUP) active->right = true;
            active->shift = GetAsyncKeyState(VK_SHIFT) < 0;
        }
        return CallNextHookEx(0, code, wParam, lParam);
    }
};

inline MonitoredTrackPopupMenu* MonitoredTrackPopupMenu::active = 0;

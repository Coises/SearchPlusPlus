// This file is part of Search++ (a plugin for Notepad++),
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
using namespace NPP;

// Assigned Scintilla indicator used to show zero length matches, or zero.

int zlmIndicator = 0;

// Routines that load and save the configuration file

void loadConfiguration();
void saveConfiguration();

// Routines that process Scintilla notifications

void scnFocusIn (const Scintilla::NotificationData*);
void scnModified(const Scintilla::NotificationData*);
void scnUpdateUI(const Scintilla::NotificationData*);

// Routines that process Notepad++ notifications

void bufferActivated();
void darkModeChanged();
void modifyAll(const NMHDR*);

// Routines that process menu commands

void menuCommandFind();
void menuCommandReplace();
void showAboutDialog();
void showSearchDialog();
void showSearchInFilesDialog();
void showSettingsDialog();
void showToolsMenu();

// Other routines needed for initialization or cleanup

void destroySearchDialogs();


namespace {

    LRESULT CALLBACK fixHiddenFirstLast(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR) {
        constexpr int SymbolMargin  = 1;
        constexpr int HideBegin     = 19;
        constexpr int HideEnd       = 18;
        constexpr int HideBeginMask = 1 << HideBegin;
        constexpr int HideEndMask   = 1 << HideEnd  ;
        switch (uMsg) {
        case WM_DESTROY:
            RemoveWindowSubclass(hWnd, fixHiddenFirstLast, uIdSubclass);
            break;
        case WM_NOTIFY:
        {
            auto& nmhdr = *reinterpret_cast<NMHDR*&>(lParam);
            if (nmhdr.code != SCN_MARGINCLICK) break;
            if (nmhdr.hwndFrom != plugin.nppData._scintillaMainHandle && nmhdr.hwndFrom != plugin.nppData._scintillaSecondHandle) break;
            auto& scn = *reinterpret_cast<Scintilla::NotificationData*>(lParam);
            if (scn.margin != SymbolMargin) break;
            if (scn.modifiers != Scintilla::KeyMod::Norm) break;
            plugin.getScintillaPointers(nmhdr.hwndFrom);
            Scintilla::Line line = sci.LineFromPosition(scn.position);
            int mask = sci.MarkerGet(line);
            if (mask & HideBeginMask) {
                Scintilla::Line lineCount = sci.LineCount();
                if (line >= lineCount - 1) break;
                Scintilla::Line term = sci.MarkerNext(line + 1, HideEndMask);
                if (term == lineCount - 1) sci.ShowLines(term, term);
                else if (term == -1) {
                    sci.MarkerAdd(lineCount - 1, HideEnd);
                    sci.ShowLines(lineCount - 1, lineCount - 1);
                }
            }
            else if (mask & HideEndMask)  {
                if (line <= 0) break;
                Scintilla::Line start = sci.MarkerPrevious(line - 1, HideBeginMask);
                if (start == 0) sci.ShowLines(0, 0);
                else if (start == -1) {
                    sci.MarkerAdd(0, HideBegin);
                    sci.ShowLines(0, 0);
                }
            }
            break;
        }
        }
        return DefSubclassProc(hWnd, uMsg, wParam, lParam);
    }
}


// Define menu commands:
//     text to appear on menu (ignored for a menu separator line)
//     address of a void, zero-argument function that processes the command (0 for a menu separator line)
//         recommended: use plugin.cmd, per examples, to wrap the function, setting Scintilla pointers and bypassing notifications
//     ignored on call; on return, Notepad++ will fill this in with the menu command ID it assigns
//     whether to show a checkmark beside this item on initial display of the menu
//     0 or default shortcut key (menu accelerator) specified as the address of an NPP::ShortcutKey structure

FuncItem menuDefinition[] = {
    { L"&Search..."         , []() {plugin.cmd(showSearchDialog       );}, 0, false, 0},
    { L"Search &in Files...", []() {plugin.cmd(showSearchInFilesDialog);}, 0, false, 0},
    { 0                     , 0                                          , 0, false, 0},
    { L"&Find"              , []() {plugin.cmd(menuCommandFind        );}, 0, false, 0},
    { L"&Replace"           , []() {plugin.cmd(menuCommandReplace     );}, 0, false, 0},
    { L"&Tools menu"        , []() {plugin.cmd(showToolsMenu          );}, 0, false, 0},
    { 0                     , 0                                          , 0, false, 0},
    { L"S&ettings..."       , []() {plugin.cmd(showSettingsDialog     );}, 0, false, 0},
    { L"&Help/About..."     , []() {plugin.cmd(showAboutDialog        );}, 0, false, 0}
};


// Tell Notepad++ the plugin name

extern "C" __declspec(dllexport) const wchar_t* getName() {
    return L"Search++";
}


// Tell Notepad++ about the plugin menu

extern "C" __declspec(dllexport) FuncItem * getFuncsArray(int *n) {
    loadConfiguration();
    *n = sizeof(menuDefinition) / sizeof(FuncItem);
    return reinterpret_cast<FuncItem*>(&menuDefinition);
}


// Notification processing: each notification desired must be sent to a function that will handle it.

extern "C" __declspec(dllexport) void beNotified(SCNotification *np) {

    if (plugin.bypassNotifications) return;
    plugin.bypassNotifications = true;
    auto*& nmhdr = reinterpret_cast<NMHDR*&>(np);

    // Notepad++ notifications

    if (nmhdr->hwndFrom == plugin.nppData._nppHandle) {
      
        switch (nmhdr->code) {

        case NPPN_BEFORESHUTDOWN:
            plugin.startupOrShutdown = true;
            break;

        case NPPN_BUFFERACTIVATED:
            if (!plugin.startupOrShutdown && !plugin.fileIsOpening) {
                bufferActivated();
            }
            break;

        case NPPN_CANCELSHUTDOWN:
            plugin.startupOrShutdown = false;
            break;

        case NPPN_DARKMODECHANGED:
            darkModeChanged();
            break;

        case NPPN_FILEBEFOREOPEN:
            plugin.fileIsOpening = true;
            break;

        case NPPN_FILEOPENED:
            plugin.fileIsOpening = false;
            break;

        case NPPN_GLOBALMODIFIED:
            modifyAll(nmhdr);
            break;

        case NPPN_READY:
            npp(NPPM_ALLOCATEINDICATOR, 2, &zlmIndicator);
            npp(NPPM_ADDSCNMODIFIEDFLAGS, 0, SC_MOD_INSERTTEXT | SC_MOD_DELETETEXT);                  
            data.bookMarker = static_cast<int>(npp(NPPM_GETBOOKMARKID, 0, 0));
            SetWindowSubclass(plugin.nppData._nppHandle, fixHiddenFirstLast, 1, 0);
            plugin.startupOrShutdown = false;
            bufferActivated();
            break;

        case NPPN_SHUTDOWN:
            destroySearchDialogs();
            saveConfiguration();
            break;

        }

    }

    // Scintilla notifications

    else if (nmhdr->hwndFrom == plugin.nppData._scintillaMainHandle || nmhdr->hwndFrom == plugin.nppData._scintillaSecondHandle) {

        auto*& scnp = reinterpret_cast<Scintilla::NotificationData*&>(np);
        switch (scnp->nmhdr.code) {

        case Scintilla::Notification::FocusIn:
            scnFocusIn(scnp);
            break;

        case Scintilla::Notification::Modified:
            scnModified(scnp);
            break;

        case Scintilla::Notification::UpdateUI:
            scnUpdateUI(scnp);
            break;

        default:;
        }

    }

    plugin.bypassNotifications = false;

}


// This is rarely used, but a few Notepad++ commands call this routine as part of their processing

extern "C" __declspec(dllexport) LRESULT messageProc(UINT, WPARAM, LPARAM) {return TRUE;}

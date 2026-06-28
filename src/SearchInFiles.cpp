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


#include "SearchInFiles.h"
#include "Framework/UtilityFramework.h"
#include "Framework/FileDialogBase.h"
#include <algorithm>
#include <format>
#include "shlwapi.h"
#include "windowsx.h"
#include "Host/Scintilla.h"
#include "Framework/ScintillaCallEx.h"
#include "SearchInFilesConfiguration.h"
#include "CommonData.h"


using namespace std::string_literals;

void closeSearchInFilesDialog();
void dispatchSearchTasks(HWND inform);
void hideHitlist();
void loadConfiguration();
void saveConfiguration();
void showHitlist();
void showHitlist(MatchResults& matchResults);
void showHitlist(std::string_view path);
void showSearchDialog();


namespace {

DialogStretch mainWindowStretch;

enum class ProcessingStatus { None, Scanning, Searching, Finished, Canceled };

ProcessingStatus processingStatus = ProcessingStatus::None;

enum class QueueColumn { Path, Matches, Size, Status, Progress };

HWND                queueListView = 0;
std::vector<size_t> queueSortedIndices;
QueueColumn         queueSortColumn = QueueColumn::Status;
bool                queueSortAscending = true;

constexpr wchar_t statusFormatSearching[]
    = L"Searching {:Ld} files; Active: {:Ld}; Waiting: {:Ld}; Finished: {:Ld}; Canceled: {:Ld}; Errors: {:Ld}";


struct {
    COLORREF background = 0;
    COLORREF softerBackground = 0;   // ctrl background color
    COLORREF hotBackground = 0;
    COLORREF pureBackground = 0;   // dlg background color
    COLORREF errorBackground = 0;
    COLORREF text = 0;
    COLORREF darkerText = 0;
    COLORREF disabledText = 0;
    COLORREF linkText = 0;
    COLORREF edge = 0;
    COLORREF hotEdge = 0;
    COLORREF disabledEdge = 0;
} darkModeColors;

bool isDarkMode = false;


void ApplyQueueSorting(HWND lv) {

    size_t n = SearchableFile::queue.size();
    if (queueSortedIndices.size() != n) {
        queueSortedIndices.resize(n);
        for (size_t i = 0; i < n; ++i) queueSortedIndices[i] = i;
    }

    std::vector<size_t> selections;
    for (int i = -1; (i = ListView_GetNextItem(lv, i, LVNI_SELECTED)) != -1;) {
        if (i >= queueSortedIndices.size() || queueSortedIndices[i] >= SearchableFile::queue.size()) continue;
        selections.push_back(queueSortedIndices[i]);
    }
    int focusedIndex = ListView_GetNextItem(lv, -1, LVNI_FOCUSED);
    focusedIndex = focusedIndex >= 0 && focusedIndex < queueSortedIndices.size()
        ? static_cast<int>(queueSortedIndices[focusedIndex]) : -1;

    switch (queueSortColumn) {

    case QueueColumn::Path:
        std::stable_sort(queueSortedIndices.begin(), queueSortedIndices.end(), [](size_t a, size_t b) {
            const auto& itemA = SearchableFile::queue[queueSortAscending ? a : b];
            const auto& itemB = SearchableFile::queue[queueSortAscending ? b : a];
            return StrCmpLogicalW(itemA.filePath.data(), itemB.filePath.data()) < 0;
            });
        break;

    case QueueColumn::Matches:
    {
        std::vector<size_t> freeze(SearchableFile::queue.size());
        for (size_t i = 0; i < freeze.size(); ++i) freeze[i] = SearchableFile::queue[i].matches_found;
        std::stable_sort(queueSortedIndices.begin(), queueSortedIndices.end(), [freeze](size_t a, size_t b) {
            return queueSortAscending ? freeze[a] < freeze[b] : freeze[b] < freeze[a];
            });
        break;
    }

    case QueueColumn::Size:
        std::stable_sort(queueSortedIndices.begin(), queueSortedIndices.end(), [](size_t a, size_t b) {
            const auto& itemA = SearchableFile::queue[queueSortAscending ? a : b];
            const auto& itemB = SearchableFile::queue[queueSortAscending ? b : a];
            return itemA.size < itemB.size;
        });
        break;

    case QueueColumn::Status: 
    {
        std::vector<int> freeze(SearchableFile::queue.size());
        for (size_t i = 0; i < freeze.size(); ++i) {
            switch (SearchableFile::queue[i].status) {
            case SearchableFile::Status::Reading:
            case SearchableFile::Status::Examining:
            case SearchableFile::Status::Searching: freeze[i] = 1; break;
            case SearchableFile::Status::Waiting:   freeze[i] = 2; break;
            case SearchableFile::Status::Error:     freeze[i] = 3; break;
            case SearchableFile::Status::Finished:  freeze[i] = 4; break;
            case SearchableFile::Status::Canceled:  freeze[i] = 5; break;
            default:                                freeze[i] = 6;
            }
        }
        std::stable_sort(queueSortedIndices.begin(), queueSortedIndices.end(), [freeze](size_t a, size_t b) {
            return queueSortAscending ? freeze[a] < freeze[b] : freeze[b] < freeze[a];
            });
        break;
    }


    case QueueColumn::Progress:
    {
        std::vector<double> freeze(SearchableFile::queue.size());
        for (size_t i = 0; i < freeze.size(); ++i) {
            freeze[i] = SearchableFile::queue[i].size
                ? static_cast<double>(SearchableFile::queue[i].bytes_processed) / SearchableFile::queue[i].size
                : SearchableFile::queue[i].status == SearchableFile::Status::Finished ? 1.0 : 0.0;
        }
        std::stable_sort(queueSortedIndices.begin(), queueSortedIndices.end(), [freeze](size_t a, size_t b) {
            return queueSortAscending ? freeze[a] < freeze[b] : freeze[b] < freeze[a];
            });
        break;
    }

    }

    if (focusedIndex >= 0 || !selections.empty()) {
        ListView_SetItemState(lv, -1, 0, LVIS_SELECTED | LVIS_FOCUSED);
        for (size_t i = 0; i < queueSortedIndices.size(); ++i) {
            size_t q = queueSortedIndices[i];
            bool focus = q == focusedIndex;
            bool selected = std::ranges::find(selections, q) != selections.end();
            ListView_SetItemState(lv, static_cast<int>(i), (focus ? LVIS_FOCUSED : 0) | (selected ? LVIS_SELECTED : 0),
                LVIS_SELECTED | LVIS_FOCUSED);
        }
    }

}


std::wstring BrowseForFolder(HWND hParent) {
    OpenDialogBase fod;
    fod.SetOptions(fod.GetOptions() | FOS_FORCEFILESYSTEM | FOS_PICKFOLDERS);
    fod.SetTitle(L"Select a folder in which to search");
    return fod.Show(hParent) ? fod.GetResultPath() : L"";
}


// Subclass procedure for non-Scintilla controls: implement dialog-wide keyboard shortcuts

LRESULT __stdcall subclassOther(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR) {
    switch (uMsg) {
    case WM_NCDESTROY:
        RemoveWindowSubclass(hWnd, subclassOther, uIdSubclass);
        break;
    case WM_KEYDOWN:
        if ((lParam & KF_REPEAT) || !(GetKeyState(VK_CONTROL) & 0x8000) || wParam < L'A' || wParam > L'Z') break;
        switch (wParam) {
        case 'H':
            if (GetKeyState(VK_SHIFT) & 0x8000) hideHitlist();
            else                                showHitlist();
            break;
        case 'N':
            SetFocus(plugin.currentScintilla());
            if (GetKeyState(VK_SHIFT) & 0x8000) {
                if (data.searchDialog) {
                    if (data.searchDialog == data.dockingDialog) npp(NPPM_DMMHIDE, 0, data.searchDialog);
                    else ShowWindow(data.searchDialog, SW_HIDE);
                }
                hideHitlist();
                closeSearchInFilesDialog();
            }
            break;
        case 'O':
            if (GetKeyState(VK_SHIFT) & 0x8000) {
                if (!data.searchDialog) break;
                if (data.searchDialog == data.dockingDialog) npp(NPPM_DMMHIDE, 0, data.searchDialog);
                else ShowWindow(data.searchDialog, SW_HIDE);
            }
            else showSearchDialog();
            break;
        }
    }
    return DefSubclassProc(hWnd, uMsg, wParam, lParam);
}


bool processScintillaShortcut(ScintillaControl& sc, char key) {
    switch (key) {
    case 'e':
    {
        std::string findText = ucd.findCntl.text();
        std::string replText = ucd.replCntl.text();
        ucd.findCntl.TargetWholeDocument();
        ucd.findCntl.ReplaceTarget(replText);
        ucd.replCntl.TargetWholeDocument();
        ucd.replCntl.ReplaceTarget(findText);
        return true;
    }
    case 'f':
    {
        if (!data.searchDialog) break;
        sc.TargetFromSelection();
        sc.ReplaceTarget(data.find.text());
        sc.SetSel(-1, sc.TargetEnd());
        return true;
    }
    case 'H':
        hideHitlist();
        return true;
    case 'h':
        showHitlist();
        return true;
    case 'i':
        plugin.getScintillaPointers();
        sc.TargetFromSelection();
        sc.ReplaceTarget(sci.GetSelText());
        sc.SetSel(-1, sc.TargetEnd());
        return true;
    case 'n':
        SetFocus(plugin.currentScintilla());
        return true;
    case 'N':
        SetFocus(plugin.currentScintilla());
        if (data.searchDialog) {
            if (data.searchDialog == data.dockingDialog) npp(NPPM_DMMHIDE, 0, data.searchDialog);
                                                    else ShowWindow(data.searchDialog, SW_HIDE);
        }
        hideHitlist();
        closeSearchInFilesDialog();
        return true;
    case 'o':
        showSearchDialog();
        return true;
    case 'O':
        if (!data.searchDialog) break;
        if (data.searchDialog == data.dockingDialog) npp(NPPM_DMMHIDE, 0, data.searchDialog);
                                                else ShowWindow(data.searchDialog, SW_HIDE);
        return true;
    case 'r':
    {
        if (!data.searchDialog) break;
        sc.TargetFromSelection();
        sc.ReplaceTarget(data.repl.text());
        sc.SetSel(-1, sc.TargetEnd());
        return true;
    }
    }
    return false;
}


HWND setupScintillaBox(HWND hwndDlg, int box, ScintillaControl& sciCtrl) {
    HWND customBox = GetDlgItem(hwndDlg, box);
    HWND hPrev = GetWindow(customBox, GW_HWNDPREV);
    RECT rectBox;
    GetWindowRect(customBox, &rectBox);
    MapWindowPoints(0, hwndDlg, reinterpret_cast<LPPOINT>(&rectBox), 2);
    DestroyWindow(customBox);
    HWND sciBox = reinterpret_cast<HWND>(npp(NPPM_CREATESCINTILLAHANDLE, 0, hwndDlg));
    SetWindowPos(sciBox, hPrev, rectBox.left, rectBox.top, rectBox.right - rectBox.left, rectBox.bottom - rectBox.top, SWP_SHOWWINDOW);
    SetWindowLong(sciBox, GWL_ID, box);
    SetWindowLong(sciBox, GWL_STYLE, GetWindowLong(sciBox, GWL_STYLE) | WS_BORDER | WS_TABSTOP);
    SetWindowPos(sciBox, 0, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);
    sciCtrl.attach(sciBox);
    plugin.getScintillaPointers();
    sciCtrl.configure(sci);
    sciCtrl.subclass(processScintillaShortcut);
    return sciBox;
}


void enableDisableDependentControls(HWND hwndDlg) {

    if (SendDlgItemMessage(hwndDlg, IDC_SIF_FILTER_NONE, BM_GETCHECK, 0, 0) == BST_CHECKED) {
        EnableWindow(ucd.filterCntl.handle, FALSE);
        ucd.filterCntl.StyleSetFore(0            , isDarkMode ? darkModeColors.disabledText : Scintilla::Colour(0x777777));
        ucd.filterCntl.StyleSetBack(0            , isDarkMode ? darkModeColors.background   : Scintilla::Colour(0xCCCCCC));
        ucd.filterCntl.StyleSetBack(STYLE_DEFAULT, isDarkMode ? darkModeColors.background   : Scintilla::Colour(0xCCCCCC));
    }
    else {
        EnableWindow(ucd.filterCntl.handle, TRUE);
        ucd.filterCntl.StyleSetFore(0            , sci.StyleGetFore(1));
        ucd.filterCntl.StyleSetBack(0            , sci.StyleGetBack(1));
        ucd.filterCntl.StyleSetBack(STYLE_DEFAULT, sci.StyleGetBack(1));
    }

    bool sizeFilter = SendDlgItemMessage(hwndDlg, IDC_SIF_SIZE , BM_GETCHECK, 0, 0) == BST_CHECKED;
    bool dateFilter = SendDlgItemMessage(hwndDlg, IDC_SIF_DATE , BM_GETCHECK, 0, 0) == BST_CHECKED;
    bool regex      = SendDlgItemMessage(hwndDlg, IDC_SIF_REGEX, BM_GETCHECK, 0, 0) == BST_CHECKED;

    EnableWindow(GetDlgItem(hwndDlg, IDC_SIF_SIZE_MIN_EDIT), sizeFilter ? TRUE : FALSE);
    EnableWindow(GetDlgItem(hwndDlg, IDC_SIF_SIZE_MIN_SPIN), sizeFilter ? TRUE : FALSE);
    EnableWindow(GetDlgItem(hwndDlg, IDC_SIF_SIZE_MIN_TYPE), sizeFilter ? TRUE : FALSE);
    EnableWindow(GetDlgItem(hwndDlg, IDC_SIF_SIZE_MAX_EDIT), sizeFilter ? TRUE : FALSE);
    EnableWindow(GetDlgItem(hwndDlg, IDC_SIF_SIZE_MAX_SPIN), sizeFilter ? TRUE : FALSE);
    EnableWindow(GetDlgItem(hwndDlg, IDC_SIF_SIZE_MAX_TYPE), sizeFilter ? TRUE : FALSE);

    EnableWindow(GetDlgItem(hwndDlg, IDC_SIF_DATE_TYPE), dateFilter ? TRUE : FALSE);
    EnableWindow(GetDlgItem(hwndDlg, IDC_SIF_DATE_MIN ), dateFilter ? TRUE : FALSE);
    EnableWindow(GetDlgItem(hwndDlg, IDC_SIF_DATE_MAX ), dateFilter ? TRUE : FALSE);

    EnableWindow(GetDlgItem(hwndDlg, IDC_SIF_DOTALL     ), regex ? TRUE  : FALSE);
    EnableWindow(GetDlgItem(hwndDlg, IDC_SIF_FREESPACING), regex ? TRUE  : FALSE);
    EnableWindow(GetDlgItem(hwndDlg, IDC_SIF_WHOLEWORD  ), regex ? FALSE : TRUE );

}


void updateConfigFromControls(HWND hwndDlg) {

    ucd.folderCntl.sync();
    ucd.filterCntl.sync();
    ucd.findCntl.sync();
    ucd.replCntl.sync();

    switch (SendDlgItemMessage(hwndDlg, IDC_SIF_SIZE_MIN_TYPE, CB_GETCURSEL, 0, 0)) {
    case  1: ucd.sizeMinUnit = FileSizeUnit::KiB; break;
    case  2: ucd.sizeMinUnit = FileSizeUnit::MiB; break;
    case  3: ucd.sizeMinUnit = FileSizeUnit::GiB; break;
    default: ucd.sizeMinUnit = FileSizeUnit::Bytes;
    }

    switch (SendDlgItemMessage(hwndDlg, IDC_SIF_SIZE_MAX_TYPE, CB_GETCURSEL, 0, 0)) {
    case  1: ucd.sizeMaxUnit = FileSizeUnit::KiB; break;
    case  2: ucd.sizeMaxUnit = FileSizeUnit::MiB; break;
    case  3: ucd.sizeMaxUnit = FileSizeUnit::GiB; break;
    default: ucd.sizeMaxUnit = FileSizeUnit::Bytes;
    }

    switch (SendDlgItemMessage(hwndDlg, IDC_SIF_DATE_TYPE, CB_GETCURSEL, 0, 0)) {
    case  0: ucd.dateType = FileDateType::Created ; break;
    case  2: ucd.dateType = FileDateType::Accessed; break;
    default: ucd.dateType = FileDateType::Modified;
    }

    ucd.subfolders.get(hwndDlg, IDC_SIF_SUBFOLDERS   );
    ucd.hidden    .get(hwndDlg, IDC_SIF_HIDDEN       );
    ucd.sizeFilter.get(hwndDlg, IDC_SIF_SIZE         );
    ucd.dateFilter.get(hwndDlg, IDC_SIF_DATE         );
    ucd.sizeMin   .get(hwndDlg, IDC_SIF_SIZE_MIN_SPIN);
    ucd.sizeMax   .get(hwndDlg, IDC_SIF_SIZE_MAX_SPIN);

    SYSTEMTIME lst;
    SYSTEMTIME ust;
    FILETIME ft;
    ULARGE_INTEGER ulit;
    if (DateTime_GetSystemtime(GetDlgItem(hwndDlg, IDC_SIF_DATE_MIN), &lst) == GDT_VALID) {
        lst.wHour = lst.wMinute = lst.wSecond = lst.wMilliseconds = 0;
        if (TzSpecificLocalTimeToSystemTimeEx(0, &lst, &ust)) {
            if (SystemTimeToFileTime(&ust, &ft)) {
                ulit.LowPart  = ft.dwLowDateTime;
                ulit.HighPart = ft.dwHighDateTime;
                ucd.dateMin   = ulit.QuadPart;
            }
        }
    }
    if (DateTime_GetSystemtime(GetDlgItem(hwndDlg, IDC_SIF_DATE_MAX), &lst) == GDT_VALID) {
        lst.wHour = 23; lst.wMinute = lst.wSecond = 59; lst.wMilliseconds = 999;
        if (TzSpecificLocalTimeToSystemTimeEx(0, &lst, &ust)) {
            if (SystemTimeToFileTime(&ust, &ft)) {
                ulit.LowPart  = ft.dwLowDateTime;
                ulit.HighPart = ft.dwHighDateTime;
                ucd.dateMax   = ulit.QuadPart;
            }
        }
    }

    ucd.regex      .get(hwndDlg, IDC_SIF_REGEX      );
    ucd.dotAll     .get(hwndDlg, IDC_SIF_DOTALL     );
    ucd.freeSpacing.get(hwndDlg, IDC_SIF_FREESPACING);
    ucd.matchCase  .get(hwndDlg, IDC_SIF_MATCHCASE  );
    ucd.wholeWord  .get(hwndDlg, IDC_SIF_WHOLEWORD  );

    ucd.filterEngine
        = SendDlgItemMessage(hwndDlg, IDC_SIF_FILTER_EXTENSION, BM_GETCHECK, 0, 0) == BST_CHECKED ? FilterEngine::Extension
        : SendDlgItemMessage(hwndDlg, IDC_SIF_FILTER_EXCLUDE  , BM_GETCHECK, 0, 0) == BST_CHECKED ? FilterEngine::Exclude
        : SendDlgItemMessage(hwndDlg, IDC_SIF_FILTER_REGEX    , BM_GETCHECK, 0, 0) == BST_CHECKED ? FilterEngine::Boost
                                                                                                  : FilterEngine::Disabled;
}


void showScintillaTip(HWND scintilla, std::string bubble) {
    constexpr size_t target_width = 32;
    for (size_t p = 0; p + target_width < bubble.length();) {
        size_t q = bubble.substr(p, target_width).find_last_of(' ');
        if (q == std::string::npos) {
            q = bubble.find_first_of(' ', p + target_width);
            if (q == std::string::npos) break;
        }
        else q += p;
        bubble[q] = '\n';
        p = q + 1;
        while (p < bubble.length() && bubble[p] == ' ') bubble.erase(p, 1);
    }
    plugin.getScintillaPointers(scintilla);
    sci.CallTipShow(sci.Length(), bubble.data());
}


INT_PTR CALLBACK mainDialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {

    switch (uMsg) {

    case WM_DESTROY:
        KillTimer(hwndDlg, 1);
        sif.cancel_all_source.cancel();
        npp(NPPM_MODELESSDIALOG, MODELESSDIALOGREMOVE, hwndDlg);
        data.searchInFilesDialog = 0;
        return FALSE;

    case WM_INITDIALOG:
    {

        setupScintillaBox(hwndDlg, IDC_SIF_FOLDER , ucd.folderCntl);
        setupScintillaBox(hwndDlg, IDC_SIF_FILTER , ucd.filterCntl);
        setupScintillaBox(hwndDlg, IDC_SIF_FINDBOX, ucd.findCntl  );
        setupScintillaBox(hwndDlg, IDC_SIF_REPLBOX, ucd.replCntl  );

        mainWindowStretch.setup(hwndDlg);
        ucd.mainWindowPosition.put(hwndDlg);

        SendDlgItemMessage(hwndDlg, IDC_SIF_SIZE_MIN_SPIN, UDM_SETRANGE32, 0, 999999);
        SendDlgItemMessage(hwndDlg, IDC_SIF_SIZE_MAX_SPIN, UDM_SETRANGE32, 0, 999999);

        SYSTEMTIME dateControlLimits[2];
        ZeroMemory(&dateControlLimits[0], sizeof(SYSTEMTIME));
        dateControlLimits[0].wYear = 1970; dateControlLimits[0].wMonth = 1; dateControlLimits[0].wDay = 1;
        GetLocalTime(&dateControlLimits[1]);
        DateTime_SetRange(GetDlgItem(hwndDlg, IDC_SIF_DATE_MIN), GDTR_MIN | GDTR_MAX, dateControlLimits);
        DateTime_SetRange(GetDlgItem(hwndDlg, IDC_SIF_DATE_MAX), GDTR_MIN | GDTR_MAX, dateControlLimits);

        HWND drop = GetDlgItem(hwndDlg, IDC_SIF_SIZE_MIN_TYPE);
        SendMessage(drop, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"Bytes"));
        SendMessage(drop, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"KiB"  ));
        SendMessage(drop, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"MiB"  ));
        SendMessage(drop, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"GiB"  ));
        switch (ucd.sizeMinUnit.get()) {
        case FileSizeUnit::KiB: SendMessage(drop, CB_SETCURSEL, 1, 0); break;
        case FileSizeUnit::MiB: SendMessage(drop, CB_SETCURSEL, 2, 0); break;
        case FileSizeUnit::GiB: SendMessage(drop, CB_SETCURSEL, 3, 0); break;
        default               : SendMessage(drop, CB_SETCURSEL, 0, 0);
        }

        drop = GetDlgItem(hwndDlg, IDC_SIF_SIZE_MAX_TYPE);
        SendMessage(drop, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"Bytes"));
        SendMessage(drop, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"KiB"  ));
        SendMessage(drop, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"MiB"  ));
        SendMessage(drop, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"GiB"  ));
        switch (ucd.sizeMaxUnit.get()) {
        case FileSizeUnit::KiB: SendMessage(drop, CB_SETCURSEL, 1, 0); break;
        case FileSizeUnit::MiB: SendMessage(drop, CB_SETCURSEL, 2, 0); break;
        case FileSizeUnit::GiB: SendMessage(drop, CB_SETCURSEL, 3, 0); break;
        default               : SendMessage(drop, CB_SETCURSEL, 0, 0);
        }

        drop = GetDlgItem(hwndDlg, IDC_SIF_DATE_TYPE);
        SendMessage(drop, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"Created" ));
        SendMessage(drop, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"Modified"));
        SendMessage(drop, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"Accessed"));
        switch (ucd.dateType.get()) {
        case FileDateType::Created : SendMessage(drop, CB_SETCURSEL, 0, 0); break;
        case FileDateType::Accessed: SendMessage(drop, CB_SETCURSEL, 2, 0); break;
        default                    : SendMessage(drop, CB_SETCURSEL, 1, 0);
        }

        ucd.subfolders.put(hwndDlg, IDC_SIF_SUBFOLDERS   );
        ucd.hidden    .put(hwndDlg, IDC_SIF_HIDDEN       );
        ucd.sizeFilter.put(hwndDlg, IDC_SIF_SIZE         );
        ucd.dateFilter.put(hwndDlg, IDC_SIF_DATE         );
        ucd.sizeMin   .put(hwndDlg, IDC_SIF_SIZE_MIN_SPIN);
        ucd.sizeMax   .put(hwndDlg, IDC_SIF_SIZE_MAX_SPIN);

        SYSTEMTIME lst;
        SYSTEMTIME ust;
        FILETIME ft;
        ULARGE_INTEGER ulit;
        ulit.QuadPart     = ucd.dateMin;
        ft.dwLowDateTime  = ulit.LowPart;
        ft.dwHighDateTime = ulit.HighPart;
        if (FileTimeToSystemTime(&ft, &ust)) {
            if (SystemTimeToTzSpecificLocalTimeEx(0, &ust, &lst)) {
                DateTime_SetSystemtime(GetDlgItem(hwndDlg, IDC_SIF_DATE_MIN), GDT_VALID, &lst);
            }
        }
        ulit.QuadPart     = ucd.dateMax;
        ft.dwLowDateTime  = ulit.LowPart;
        ft.dwHighDateTime = ulit.HighPart;
        if (FileTimeToSystemTime(&ft, &ust)) {
            if (SystemTimeToTzSpecificLocalTimeEx(0, &ust, &lst)) {
                DateTime_SetSystemtime(GetDlgItem(hwndDlg, IDC_SIF_DATE_MAX), GDT_VALID, &lst);
            }
        }

        ucd.regex      .put(hwndDlg, IDC_SIF_REGEX      );
        ucd.dotAll     .put(hwndDlg, IDC_SIF_DOTALL     );
        ucd.freeSpacing.put(hwndDlg, IDC_SIF_FREESPACING);
        ucd.matchCase  .put(hwndDlg, IDC_SIF_MATCHCASE  );
        ucd.wholeWord  .put(hwndDlg, IDC_SIF_WHOLEWORD  );

        switch (ucd.filterEngine) {
        case FilterEngine::Extension: CheckRadioButton(hwndDlg, IDC_SIF_FILTER_NONE, IDC_SIF_FILTER_REGEX, IDC_SIF_FILTER_EXTENSION); break;
        case FilterEngine::Exclude  : CheckRadioButton(hwndDlg, IDC_SIF_FILTER_NONE, IDC_SIF_FILTER_REGEX, IDC_SIF_FILTER_EXCLUDE  ); break;
        case FilterEngine::Boost    : CheckRadioButton(hwndDlg, IDC_SIF_FILTER_NONE, IDC_SIF_FILTER_REGEX, IDC_SIF_FILTER_REGEX    ); break;
        default                     : CheckRadioButton(hwndDlg, IDC_SIF_FILTER_NONE, IDC_SIF_FILTER_REGEX, IDC_SIF_FILTER_NONE     );
        }

        enableDisableDependentControls(hwndDlg);

        HWND qlv = GetDlgItem(hwndDlg, IDC_SIF_LIST);
        ListView_SetExtendedListViewStyle(qlv, LVS_EX_LABELTIP | LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER);
        LVCOLUMNW col;
        col.mask = LVCF_TEXT | LVCF_FMT;
        col.fmt = LVCFMT_LEFT ; col.pszText = const_cast<wchar_t*>(L"File Path"        ); ListView_InsertColumn(qlv, 0, &col);
        col.fmt = LVCFMT_RIGHT; col.pszText = const_cast<wchar_t*>(L"Matches"          ); ListView_InsertColumn(qlv, 1, &col);
        col.fmt = LVCFMT_RIGHT; col.pszText = const_cast<wchar_t*>(L"8888.8 WiB"       ); ListView_InsertColumn(qlv, 2, &col);
        col.fmt = LVCFMT_LEFT ; col.pszText = const_cast<wchar_t*>(L"Error: BufferingX"); ListView_InsertColumn(qlv, 3, &col);
        col.fmt = LVCFMT_RIGHT; col.pszText = const_cast<wchar_t*>(L"Progress"         ); ListView_InsertColumn(qlv, 4, &col);
        const int normalOrder[5] = { 0, 1, 2, 3, 4 };
        const int sizingOrder[5] = { 1, 2, 3, 4, 0 };
        ListView_SetColumnOrderArray(qlv, 5, sizingOrder);
        ListView_SetColumnWidth(qlv, 1, LVSCW_AUTOSIZE_USEHEADER);
        ListView_SetColumnWidth(qlv, 2, LVSCW_AUTOSIZE_USEHEADER);
        ListView_SetColumnWidth(qlv, 3, LVSCW_AUTOSIZE_USEHEADER);
        ListView_SetColumnWidth(qlv, 4, LVSCW_AUTOSIZE_USEHEADER);
        ListView_SetColumnWidth(qlv, 0, LVSCW_AUTOSIZE_USEHEADER);
        ListView_SetColumnOrderArray(qlv, 5, normalOrder);
        col.fmt = LVCFMT_RIGHT; col.pszText = const_cast<wchar_t*>(L"File Size"); ListView_SetColumn(qlv, 2, &col);
        col.fmt = LVCFMT_LEFT ; col.pszText = const_cast<wchar_t*>(L"Status"   ); ListView_SetColumn(qlv, 3, &col);

        SetWindowSubclass(GetDlgItem(hwndDlg, IDC_SIF_BROWSE          ), subclassOther, 0, 0);
        SetWindowSubclass(GetDlgItem(hwndDlg, IDC_SIF_SUBFOLDERS      ), subclassOther, 0, 0);
        SetWindowSubclass(GetDlgItem(hwndDlg, IDC_SIF_HIDDEN          ), subclassOther, 0, 0);
        SetWindowSubclass(GetDlgItem(hwndDlg, IDC_SIF_FILTER_NONE     ), subclassOther, 0, 0);
        SetWindowSubclass(GetDlgItem(hwndDlg, IDC_SIF_FILTER_EXTENSION), subclassOther, 0, 0);
        SetWindowSubclass(GetDlgItem(hwndDlg, IDC_SIF_FILTER_EXCLUDE  ), subclassOther, 0, 0);
        SetWindowSubclass(GetDlgItem(hwndDlg, IDC_SIF_FILTER_REGEX    ), subclassOther, 0, 0);
        SetWindowSubclass(GetDlgItem(hwndDlg, IDC_SIF_FILTER_LABEL    ), subclassOther, 0, 0);
        SetWindowSubclass(GetDlgItem(hwndDlg, IDC_SIF_SIZE            ), subclassOther, 0, 0);
        SetWindowSubclass(GetDlgItem(hwndDlg, IDC_SIF_SIZE_MIN_EDIT   ), subclassOther, 0, 0);
        SetWindowSubclass(GetDlgItem(hwndDlg, IDC_SIF_SIZE_MIN_SPIN   ), subclassOther, 0, 0);
        SetWindowSubclass(GetDlgItem(hwndDlg, IDC_SIF_SIZE_MIN_TYPE   ), subclassOther, 0, 0);
        SetWindowSubclass(GetDlgItem(hwndDlg, IDC_SIF_SIZE_MAX_EDIT   ), subclassOther, 0, 0);
        SetWindowSubclass(GetDlgItem(hwndDlg, IDC_SIF_SIZE_MAX_SPIN   ), subclassOther, 0, 0);
        SetWindowSubclass(GetDlgItem(hwndDlg, IDC_SIF_SIZE_MAX_TYPE   ), subclassOther, 0, 0);
        SetWindowSubclass(GetDlgItem(hwndDlg, IDC_SIF_DATE            ), subclassOther, 0, 0);
        SetWindowSubclass(GetDlgItem(hwndDlg, IDC_SIF_DATE_TYPE       ), subclassOther, 0, 0);
        SetWindowSubclass(GetDlgItem(hwndDlg, IDC_SIF_DATE_MIN        ), subclassOther, 0, 0);
        SetWindowSubclass(GetDlgItem(hwndDlg, IDC_SIF_DATE_MAX        ), subclassOther, 0, 0);
        SetWindowSubclass(GetDlgItem(hwndDlg, IDC_SIF_REGEX           ), subclassOther, 0, 0);
        SetWindowSubclass(GetDlgItem(hwndDlg, IDC_SIF_MATCHCASE       ), subclassOther, 0, 0);
        SetWindowSubclass(GetDlgItem(hwndDlg, IDC_SIF_WHOLEWORD       ), subclassOther, 0, 0);
        SetWindowSubclass(GetDlgItem(hwndDlg, IDC_SIF_DOTALL          ), subclassOther, 0, 0);
        SetWindowSubclass(GetDlgItem(hwndDlg, IDC_SIF_FREESPACING     ), subclassOther, 0, 0);
        SetWindowSubclass(GetDlgItem(hwndDlg, IDC_SIF_FIND            ), subclassOther, 0, 0);
        SetWindowSubclass(GetDlgItem(hwndDlg, IDC_SIF_CLOSECANCEL     ), subclassOther, 0, 0);
        SetWindowSubclass(GetDlgItem(hwndDlg, IDC_SIF_REPLACE         ), subclassOther, 0, 0);
        SetWindowSubclass(GetDlgItem(hwndDlg, IDC_SIF_MESSAGE         ), subclassOther, 0, 0);
        SetWindowSubclass(GetDlgItem(hwndDlg, IDC_SIF_LIST            ), subclassOther, 0, 0);

        npp(NPPM_MODELESSDIALOG, MODELESSDIALOGADD, hwndDlg);
        npp(NPPM_DARKMODESUBCLASSANDTHEME, NPP::NppDarkMode::dmfInit, hwndDlg);
        isDarkMode = npp(NPPM_ISDARKMODEENABLED, 0, 0);
        if (isDarkMode) npp(NPPM_GETDARKMODECOLORS, sizeof darkModeColors, &darkModeColors);

        SetTimer(hwndDlg, 1, 40, NULL);  // The ListView is refreshed on a timer while searching.

        SetFocus(GetDlgItem(hwndDlg, IDC_SIF_FINDBOX));

        return FALSE;

    }

    case WM_TIMER:
        switch (processingStatus) {
        case ProcessingStatus::Searching:
        {
            size_t active   = 0;
            size_t waiting  = 0;
            size_t finished = 0;
            size_t canceled = 0;
            size_t errors   = 0;
            for (const auto& sf : SearchableFile::queue) {
                switch (sf.status) {
                case SearchableFile::Status::Waiting : ++waiting ; break;
                case SearchableFile::Status::Finished: ++finished; break;
                case SearchableFile::Status::Canceled: ++canceled; break;
                case SearchableFile::Status::Error   : ++errors  ; break;
                default: ++active;
                }
            }
            SetWindowText(GetDlgItem(hwndDlg, IDC_SIF_MESSAGE),
                std::format(UserLocale, statusFormatSearching,
                    SearchableFile::queue.size(), active, waiting, finished, canceled, errors).data());
        }
        [[fallthrough]];
        case ProcessingStatus::Canceled:
        case ProcessingStatus::Finished:
            ApplyQueueSorting(GetDlgItem(hwndDlg, IDC_SIF_LIST));
            InvalidateRect(GetDlgItem(hwndDlg, IDC_SIF_LIST), NULL, FALSE);
        }
        return TRUE;

    case WM_APP_UPDATE_COUNT: {
        if (lParam == 0) {
            SetWindowTextW(GetDlgItem(hwndDlg, IDC_SIF_MESSAGE), std::format(L"Scanning directory; found {:L} files.", wParam).data());
        }
        else {
            ListView_SetItemCountEx(GetDlgItem(hwndDlg, IDC_SIF_LIST), SearchableFile::queue.size(), 0);
            processingStatus = ProcessingStatus::Searching;
            HWND qlv = GetDlgItem(hwndDlg, IDC_SIF_LIST);
            int normalOrder[5] = { 0, 1, 2, 3, 4 };
            const int sizingOrder[5] = { 1, 2, 3, 4, 0 };
            if (ListView_GetColumnOrderArray(qlv, 5, normalOrder)) {
                ListView_SetColumnOrderArray(qlv, 5, sizingOrder);
                ListView_SetColumnWidth(qlv, 0, LVSCW_AUTOSIZE_USEHEADER);
                ListView_SetColumnOrderArray(qlv, 5, normalOrder);
            }
        }
        return TRUE;
    }

    case WM_APP_SEARCH_CANCELED:
    case WM_APP_SEARCH_COMPLETE:
    {
        processingStatus = uMsg == WM_APP_SEARCH_CANCELED ? ProcessingStatus::Canceled : ProcessingStatus::Finished;
        size_t matches  = 0;
        size_t files    = 0;
        size_t canceled = 0;
        size_t errors   = 0;
        for (const auto& sf : SearchableFile::queue) {
            switch (sf.status) {
            case SearchableFile::Status::Canceled: ++canceled; break;
            case SearchableFile::Status::Error: ++errors; break;
            default:
                if (sf.matches_found) {
                    matches += sf.matches_found;
                    ++files;
                }
            }
        }
        SetWindowText(GetDlgItem(hwndDlg, IDC_SIF_MESSAGE),
            canceled || errors
            ? std::format(UserLocale, L"Finished; found {:Ld} match{:s} in {:Ld} of {:Ld} files; canceled: {:Ld}, errors: {:Ld}.",
                matches, matches == 1 ? L"" : L"es", files, SearchableFile::queue.size(), canceled, errors).data()
            : std::format(UserLocale, L"Finished; found {:Ld} match{:s} in {:Ld} of {:Ld} files.",
                matches, matches == 1 ? L"" : L"es", files, SearchableFile::queue.size()).data()
        );
        EnableWindow(GetDlgItem(hwndDlg, IDC_SIF_FIND), TRUE);
        EnableWindow(GetDlgItem(hwndDlg, IDC_SIF_REPLACE), TRUE);
        SetDlgItemText(hwndDlg, IDC_SIF_CLOSECANCEL, L"&Close");
        if (uMsg == WM_APP_SEARCH_COMPLETE && !sif.matchResults.text.empty()) showHitlist(sif.matchResults);
        return TRUE;
    }

    case WM_COMMAND:

        switch (LOWORD(wParam)) {

        case IDCANCEL:
            updateConfigFromControls(hwndDlg);
            ucd.mainWindowPosition.get(hwndDlg);
            DestroyWindow(hwndDlg);
            return TRUE;

        case IDC_SIF_BROWSE:
        {
            std::wstring folder = BrowseForFolder(hwndDlg);
            if (!folder.empty()) {
                plugin.getScintillaPointers(GetDlgItem(hwndDlg, IDC_SIF_FOLDER));
                sci.TargetWholeDocument();
                sci.ReplaceTarget(utf16to8(folder));
            }
            return TRUE;
        }

        case IDC_SIF_FIND:
        {

            static const std::string badPathChars      = "\x00\x01\x02\x03\x04\x05\x06\x07\x08\x09\x0A\x0B\x0C\x0D\x0E\x0F"s
                                                         "\x10\x11\x12\x13\x14\x15\x16\x17\x18\x19\x1A\x1B\x1C\x1D\x1E\x1F"s
                                                         "<>\"/|?*\x7F"s;
            static const std::string badExtensionChars = "\x00\x01\x02\x03\x04\x05\x06\x07\x08\x09\x0A\x0B\x0C\x0D\x0E\x0F"s
                                                         "\x10\x11\x12\x13\x14\x15\x16\x17\x18\x19\x1A\x1B\x1C\x1D\x1E\x1F"s
                                                         "<>:\"/|?*\\\x7F"s;

            updateConfigFromControls(hwndDlg);

            {
                std::string folder = ucd.folderCntl.text();
                std::erase(folder, '\n');
                std::erase(folder, '\r');
                if (folder.length() >= 2 && folder.substr(folder.length() - 2) == "\\*") folder = folder.substr(0, folder.length() - 2);
                else if (!folder.empty() && folder.back() == '\\') folder.pop_back();
                if (folder.empty()) {
                    showScintillaTip(GetDlgItem(hwndDlg, IDC_SIF_FOLDER), "Enter a path, or browse for a folder in which to search.");
                    return TRUE;
                }
                if (folder.find_first_of(badPathChars) != std::wstring::npos) {
                    showScintillaTip(GetDlgItem(hwndDlg, IDC_SIF_FOLDER),
                        "Folder path contains invalid characters (control characters or '<>\"/|?*').");
                    return TRUE;
                }
                sif.fileSpecification.path = utf8to16(folder);
            }

            if (ucd.filterEngine == FilterEngine::Disabled || ucd.filterCntl.text().empty())
                sif.fileSpecification.useFilter = false;
            else {
                const std::string& filter = ucd.filterCntl.text();
                if (ucd.filterEngine == FilterEngine::Extension || ucd.filterEngine == FilterEngine::Exclude) {
                    std::regex extensionDelimiters("[\\s,;\\|]+");
                    std::sregex_token_iterator exti(filter.begin(), filter.end(), extensionDelimiters, -1);
                    std::vector<std::string> extensions(exti, std::sregex_token_iterator());
                    std::string extrx;
                    for (auto& s : extensions) {
                        if (s.empty()) continue;
                        if (s.substr(0, 2) == "*.") s = s.substr(2);
                        else if (s.substr(0, 1) == ".") s = s.substr(1);
                        if (s.empty() || s.find_first_of(badExtensionChars) != std::wstring::npos) {
                            showScintillaTip(GetDlgItem(hwndDlg, IDC_SIF_FILTER),
                                ucd.filterEngine == FilterEngine::Exclude
                                ? "Enter a list of file extensions to exclude (separate using spaces, commas or semicolons)."
                                : "Enter a list of file extensions to process (separate using spaces, commas or semicolons).");
                            return FALSE;
                        }
                        if (!extrx.empty()) extrx += '|';
                        extrx += "\\Q" + s + "\\E";
                    }
                    extrx = ucd.filterEngine == FilterEngine::Exclude
                        ? "^.*\\.(" + extrx + ")$(*COMMIT)(*FAIL)|(*ACCEPT)"
                        : "\\.(" + extrx + ")$";
                    sif.fileSpecification.filter.find(extrx, false, false, false);
                }
                else {
                    std::string rxerr = sif.fileSpecification.filter.find(filter, false, false, false);
                    if (!rxerr.empty()) {
                        showScintillaTip(GetDlgItem(hwndDlg, IDC_SIF_FILTER), rxerr);
                        return FALSE;
                    }
                    sif.fileSpecification.filter.find(filter, false, false, false);
                }
                sif.fileSpecification.filter.setup(CP_UTF8);
                sif.fileSpecification.useFilter = true;
            }

            if (ucd.sizeFilter) {
                switch (ucd.sizeMinUnit) {
                case FileSizeUnit::KiB: sif.fileSpecification.minSize = ucd.sizeMin * 0x400ULL     ; break;
                case FileSizeUnit::MiB: sif.fileSpecification.minSize = ucd.sizeMin * 0x100000ULL  ; break;
                case FileSizeUnit::GiB: sif.fileSpecification.minSize = ucd.sizeMin * 0x40000000ULL; break;
                default               : sif.fileSpecification.minSize = ucd.sizeMin;
                }
                if (ucd.sizeMax == 0) sif.fileSpecification.maxSize = std::numeric_limits<uint64_t>::max();
                else switch (ucd.sizeMaxUnit) {
                case FileSizeUnit::KiB: sif.fileSpecification.maxSize = ucd.sizeMax * 0x400ULL     ; break;
                case FileSizeUnit::MiB: sif.fileSpecification.maxSize = ucd.sizeMax * 0x100000ULL  ; break;
                case FileSizeUnit::GiB: sif.fileSpecification.maxSize = ucd.sizeMax * 0x40000000ULL; break;
                default               : sif.fileSpecification.maxSize = ucd.sizeMax;
                }
            }
            else {
                sif.fileSpecification.minSize = 0;
                sif.fileSpecification.maxSize = std::numeric_limits<uint64_t>::max();
            }

            if (ucd.dateFilter) {
                sif.fileSpecification.timePoint =
                    ucd.dateType == FileDateType::Accessed ? FileSpecification::TimeAccess
                  : ucd.dateType == FileDateType::Created  ? FileSpecification::TimeCreation
                                                           : FileSpecification::TimeModification;
                sif.fileSpecification.minTime = ucd.dateMin;
                sif.fileSpecification.maxTime = ucd.dateMax;
            }
            else sif.fileSpecification.timePoint = FileSpecification::TimeNone;

            sif.fileSpecification.skipHidden = !ucd.hidden;
            sif.fileSpecification.recursive  = ucd.subfolders;

            if (ucd.findCntl.text().empty()) {
                showScintillaTip(GetDlgItem(hwndDlg, IDC_SIF_FINDBOX), "Enter a search string.");
                return TRUE;
            }
            sif.findString = ucd.findCntl.text();

            if (!ucd.regex) {
                std::string s;
                for (size_t i = 0;;) {
                    size_t j = sif.findString.find("\\E", i);
                    if (j == std::string::npos) {
                        if (i < sif.findString.length()) s += "\\Q" + sif.findString.substr(i) + "\\E";
                        break;
                    }
                    if (i < j) s += "\\Q" + sif.findString.substr(i, j) + "\\E\\\\E";
                    i = j + 2;
                }
                if (ucd.wholeWord) s = "\\b" + s + "\\b";
                sif.rx.find(s, ucd.matchCase, false, false);
            }
            else {
                std::string rxerr = sif.rx.find(sif.findString, ucd.matchCase, ucd.dotAll, ucd.freeSpacing);
                if (!rxerr.empty()) {
                    showScintillaTip(GetDlgItem(hwndDlg, IDC_SIF_FINDBOX), rxerr);
                    return FALSE;
                }
            }

            processingStatus = ProcessingStatus::Scanning;
            EnableWindow(GetDlgItem(hwndDlg, IDC_SIF_FIND   ), FALSE);
            EnableWindow(GetDlgItem(hwndDlg, IDC_SIF_REPLACE), FALSE);
            SetDlgItemText(hwndDlg, IDC_SIF_CLOSECANCEL, L"&Cancel All");
            ListView_SetItemCountEx(GetDlgItem(hwndDlg, IDC_SIF_LIST), 0, 0);
            dispatchSearchTasks(hwndDlg);
            ucd.folderCntl.push();
            if (ucd.filterEngine != FilterEngine::Disabled) ucd.filterCntl.push();
            ucd.findCntl.push();
            return TRUE;
        }

        case IDC_SIF_CLOSECANCEL:
            if (GetDlgItemString(hwndDlg, IDC_SIF_CLOSECANCEL) == L"&Cancel All") sif.cancel_all_source.cancel();
            else {
                updateConfigFromControls(hwndDlg);
                ucd.mainWindowPosition.get(hwndDlg);
                DestroyWindow(hwndDlg);
            }
            return TRUE;

        case IDC_SIF_REPLACE:
        {
            return TRUE;
        }

        case IDC_SIF_FILTER_NONE:
        case IDC_SIF_FILTER_EXTENSION:
        case IDC_SIF_FILTER_EXCLUDE:
        case IDC_SIF_FILTER_REGEX:
        case IDC_SIF_SIZE:
        case IDC_SIF_DATE:
        case IDC_SIF_REGEX:
            enableDisableDependentControls(hwndDlg);
            return TRUE;

        }

        return FALSE;

    case WM_SIZE:
    {
        mainWindowStretch
            .adjust(IDC_SIF_BROWSE          , 0  , 0, 0   , 0)
            .adjust(IDC_SIF_FOLDER          , 0.5, 0, 0   , 0)
            .adjust(IDC_SIF_FILTER_NONE     , 0  , 0, 0.5 , 0)
            .adjust(IDC_SIF_FILTER_EXTENSION, 0  , 0, 0.5 , 0)
            .adjust(IDC_SIF_FILTER_EXCLUDE  , 0  , 0, 0.5 , 0)
            .adjust(IDC_SIF_FILTER_REGEX    , 0  , 0, 0.5 , 0)
            .adjust(IDC_SIF_FILTER_LABEL    , 0  , 0, 0.5 , 0)
            .adjust(IDC_SIF_FILTER          , 0.5, 0, 0.5 , 0)
            .adjust(IDC_SIF_REGEX           , 0  , 0, 0.5 , 0)
            .adjust(IDC_SIF_MATCHCASE       , 0  , 0, 0.5 , 0)
            .adjust(IDC_SIF_WHOLEWORD       , 0  , 0, 0.5 , 0)
            .adjust(IDC_SIF_DOTALL          , 0  , 0, 0.5 , 0)
            .adjust(IDC_SIF_FREESPACING     , 0  , 0, 0.5 , 0)
            .adjust(IDC_SIF_FINDBOX         , 0.5, 0, 0   , 0)
            .adjust(IDC_SIF_REPLBOX         , 0.5, 0, 0.5 , 0)
            .adjust(IDC_SIF_FIND            , 0  , 0, 0.25, 0)
            .adjust(IDC_SIF_CLOSECANCEL     , 0  , 0, 0.5 , 0)
            .adjust(IDC_SIF_REPLACE         , 0  , 0, 0.75, 0)
            .adjust(IDC_SIF_MESSAGE         , 1  , 0, 0   , 0)
            .adjust(IDC_SIF_LIST            , 1  , 1, 0   , 0);
        HWND qlv = GetDlgItem(hwndDlg, IDC_SIF_LIST);
        int normalOrder[5] = { 0, 1, 2, 3, 4 };
        const int sizingOrder[5] = { 1, 2, 3, 4, 0 };
        if (ListView_GetColumnOrderArray(qlv, 5, normalOrder)) {  // This will fail, harmlessly, before the columns have been added.
            ListView_SetColumnOrderArray(qlv, 5, sizingOrder);
            ListView_SetColumnWidth(qlv, 0, LVSCW_AUTOSIZE_USEHEADER);
            ListView_SetColumnOrderArray(qlv, 5, normalOrder);
        }
        return TRUE;
    }

    case WM_GETMINMAXINFO:
    {
        MINMAXINFO& mmi = *reinterpret_cast<MINMAXINFO*>(lParam);
        mmi.ptMinTrackSize.x = mainWindowStretch.originalWidth();
        mmi.ptMinTrackSize.y = mainWindowStretch.originalHeight();
        return TRUE;
    }

    case WM_NOTIFY:
    {

        const NMHDR& nmhdr = *reinterpret_cast<NMHDR*>(lParam);

        if (nmhdr.code == LVN_COLUMNCLICK) /* Change sort for file list */ {
            const NMLISTVIEW& nmlv = *reinterpret_cast<NMLISTVIEW*>(lParam);
            QueueColumn selected = static_cast<QueueColumn>(nmlv.iSubItem);
            if (queueSortColumn == selected) queueSortAscending = !queueSortAscending;
            else { queueSortColumn = selected; queueSortAscending = true; }
            return TRUE;
        }

        if (nmhdr.code == LVN_GETDISPINFO) /* Provide data for one entry in file list */ {
            NMLVDISPINFO& lvdi = *reinterpret_cast<NMLVDISPINFO*>(lParam);
            if (!(lvdi.item.mask & LVIF_TEXT)) return FALSE;
            size_t idx = lvdi.item.iItem;
            if (idx >= queueSortedIndices.size() || queueSortedIndices[idx] >= SearchableFile::queue.size()) return FALSE;
            const SearchableFile& sf = SearchableFile::queue[queueSortedIndices[idx]];
            switch (static_cast<QueueColumn>(lvdi.item.iSubItem)) {
            case QueueColumn::Path:
                lvdi.item.pszText = const_cast<wchar_t*>(sf.filePath.data()) + sif.fileSpecification.path.length() + 1;
                break;
            case QueueColumn::Matches:
                wcsncpy_s(lvdi.item.pszText, lvdi.item.cchTextMax,
                    std::format(UserLocale, L"{:Ld}", sf.matches_found).data(),
                    lvdi.item.cchTextMax - 1);
                break;
            case QueueColumn::Size:
                wcsncpy_s(lvdi.item.pszText, lvdi.item.cchTextMax,
                    sf.size < 1024LL                   ? std::format(UserLocale, L"{:Ld} B", sf.size).data()
                  : sf.size < 1024LL * 1024LL          ? std::format(UserLocale, L"{:.1Lf} KiB", sf.size / 1024.0).data()
                  : sf.size < 1024LL * 1024LL * 1024LL ? std::format(UserLocale, L"{:.1Lf} MiB", sf.size / (1024.0 * 1024.0)).data()
                                                       : std::format(UserLocale, L"{:.1Lf} GiB", sf.size / (1024.0 * 1024.0 * 1024.0)).data(),
                    lvdi.item.cchTextMax - 1);
                break;
            case QueueColumn::Status:
            {
                const wchar_t* p;
                switch (sf.status) {
                case SearchableFile::Status::Canceled : p = L"Canceled" ; break;
                case SearchableFile::Status::Examining: p = L"Examining"; break;
                case SearchableFile::Status::Finished : p = L"Finished" ; break;
                case SearchableFile::Status::Reading  : p = L"Reading"  ; break;
                case SearchableFile::Status::Searching: p = L"Searching"; break;
                case SearchableFile::Status::Waiting  : p = L"Waiting"  ; break;
                case SearchableFile::Status::Error:
                {
                    switch (sf.error) {
                    case SearchableFile::ErrorType::NotDisk  : p = L"Error: Not Disk" ; break;
                    case SearchableFile::ErrorType::Creating : p = L"Error: Creating" ; break;
                    case SearchableFile::ErrorType::Buffering: p = L"Error: Buffering"; break;
                    case SearchableFile::ErrorType::Reading  : p = L"Error: Reading"  ; break;
                    case SearchableFile::ErrorType::Searching: p = L"Error: Searching"; break;
                    default: p = L"Error";
                    }
                    break;
                }
                default: p = L"Unknown";
                }
                lvdi.item.pszText = const_cast<wchar_t*>(p);
                break;
            }
            case QueueColumn::Progress:
                if (sf.size > 0) {
                    wcsncpy_s(lvdi.item.pszText, lvdi.item.cchTextMax,
                        std::format(UserLocale, L"{:Ld}%", (sf.bytes_processed * 100 + (sf.size / 2)) / sf.size).data(),
                        lvdi.item.cchTextMax - 1);
                }
                else if (sf.status == SearchableFile::Status::Finished) lvdi.item.pszText = const_cast<wchar_t*>(L"100%");
                else                                                    lvdi.item.pszText = const_cast<wchar_t*>(L"0%");
                break;
            }
            return TRUE;
        }

        return FALSE;
    }

    case WM_CONTEXTMENU:
    {
        const HWND contextControl = reinterpret_cast<HWND>(wParam);
        const int contextID = GetDlgCtrlID(contextControl);
        POINT screenLocation = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
        switch (contextID) {

        case IDC_SIF_LIST:
        {
            std::vector<size_t> selections;
            for (int i = -1; (i = ListView_GetNextItem(contextControl, i, LVNI_SELECTED)) != -1;) {
                if (i >= queueSortedIndices.size() || queueSortedIndices[i] >= SearchableFile::queue.size()) continue;
                selections.push_back(queueSortedIndices[i]);
            }
            if (selections.empty()) /* unexpected; ignore it */ return FALSE;
            if (screenLocation.x == -1 && screenLocation.y == -1) /* keyboard invocation */ {
                RECT rect;
                int focusedIndex = ListView_GetNextItem(contextControl, -1, LVNI_FOCUSED);
                if (focusedIndex != -1) {
                    ListView_GetItemRect(contextControl, focusedIndex, &rect, LVIR_BOUNDS);
                    screenLocation.x = rect.left + 15;
                    screenLocation.y = rect.bottom;
                    MapWindowPoints(contextControl, 0, &screenLocation, 1);
                }
                else /* selections exist but none are focused */ {
                    GetWindowRect(contextControl, &rect);
                    screenLocation.x = rect.left + 15;
                    screenLocation.y = rect.top + 15;
                }
            }
            HMENU menu = CreatePopupMenu();
            AppendMenu(menu, MF_STRING, 1, L"&Cancel search");
            AppendMenu(menu, MF_STRING, 2, L"&Open file");
            AppendMenu(menu, MF_STRING, 3, L"&Show file in explorer");
            AppendMenu(menu, MF_STRING, 4, L"&Go to match results");
            bool can_cancel = false;
            bool can_view   = false;
            for (size_t i : selections) {
                const SearchableFile& sf = SearchableFile::queue[i];
                if (sf.status != SearchableFile::Status::Canceled
                 && sf.status != SearchableFile::Status::Error
                 && sf.status != SearchableFile::Status::Finished
                 && sf.status != SearchableFile::Status::None    ) can_cancel = true;
                if (sf.matches_found > 0) can_view = true;
            }
            if (processingStatus != ProcessingStatus::Canceled && processingStatus != ProcessingStatus::Finished
                || selections.size() != 1) can_view = false;
            EnableMenuItem(menu, 1, can_cancel             ? MF_ENABLED : MF_GRAYED);
            EnableMenuItem(menu, 3, selections.size() == 1 ? MF_ENABLED : MF_GRAYED);
            EnableMenuItem(menu, 4, can_view               ? MF_ENABLED : MF_GRAYED);
            int result = TrackPopupMenu(menu, TPM_NONOTIFY | TPM_RETURNCMD,
                screenLocation.x, screenLocation.y, 0, contextControl, 0);
            switch (result) {
            case 1:
                for (size_t i : selections) {
                    const SearchableFile& sf = SearchableFile::queue[i];
                    if (sf.status != SearchableFile::Status::Canceled
                     && sf.status != SearchableFile::Status::Error
                     && sf.status != SearchableFile::Status::Finished
                     && sf.status != SearchableFile::Status::None   ) sf.cancel_source.cancel();
                }
                break;
            case 2:
                for (size_t i : selections) {
                    const SearchableFile& sf = SearchableFile::queue[i];
                    if (!npp(NPPM_DOOPEN, 0, sf.filePath.data())) {
                        TaskDialog(hwndDlg, 0, L"Search++", L"Notepad++ could not open this file:",
                            SearchableFile::queue[i].filePath.data(), TDCBF_OK_BUTTON, TD_ERROR_ICON, 0);
                    }
                }
                break;
            case 3:
            {
                std::wstring path = SearchableFile::queue[selections[0]].filePath;
                if (path.rfind(L"\\\\?\\UNC\\", 0) == 0) path = L"\\\\" + path.substr(8);
                else if (path.rfind(L"\\\\?\\", 0) == 0) path = path.substr(4);
                PIDLIST_ABSOLUTE pidlAbsolute = ILCreateFromPath(path.data());
                if (pidlAbsolute) {
                    HRESULT hr = SHOpenFolderAndSelectItems(pidlAbsolute, 0, nullptr, 0);
                    if (FAILED(hr)) TaskDialog(hwndDlg, 0, L"Search Files", L"Unable to show this file in Explorer:",
                        SearchableFile::queue[selections[0]].filePath.data(), TDCBF_OK_BUTTON, TD_ERROR_ICON, 0);
                    ILFree(pidlAbsolute);
                }
                else TaskDialog(hwndDlg, 0, L"Search Files", L"Unable to show this file in Explorer:",
                    SearchableFile::queue[selections[0]].filePath.data(), TDCBF_OK_BUTTON, TD_ERROR_ICON, 0);
                break;
            }
            case 4:
                if (!sif.matchResults.text.empty()) showHitlist(sif.matchResults);
                showHitlist(utf16to8(SearchableFile::queue[selections[0]].filePath));
                break;
            }
            DestroyMenu(menu);
            return TRUE;
        }

        case IDC_SIF_FOLDER:
        case IDC_SIF_FILTER:
        case IDC_SIF_FINDBOX:
        case IDC_SIF_REPLBOX:
        {
            ScintillaControl& sc = contextID == IDC_SIF_FOLDER  ? ucd.folderCntl
                                 : contextID == IDC_SIF_FILTER  ? ucd.filterCntl
                                 : contextID == IDC_SIF_FINDBOX ? ucd.findCntl
                                                                : ucd.replCntl;
            if (screenLocation.x == -1 && screenLocation.y == -1) /* invoked from keyboard, not mouse */ {
                Scintilla::Position caret = sc.CurrentPos();
                screenLocation.x = sc.PointXFromPosition(caret);
                screenLocation.y = sc.PointYFromPosition(caret);
                MapWindowPoints(contextControl, 0, &screenLocation, 1);
            }
            plugin.getScintillaPointers();
            bool documentHasSelection = !sci.SelectionEmpty();
            bool hasSelection = !sc.SelectionEmpty();
            int zoom = sc.Zoom();
            std::wstring zoomText = (zoom > 0 ? L"&Zoom (+" : L"&Zoom (") + std::to_wstring(zoom) + L")";
            HMENU menu = GetSubMenu(LoadMenu(plugin.dllInstance, MAKEINTRESOURCE(IDR_SEARCH_CONTEXT)), 0);
            MENUITEMINFO mii;
            mii.cbSize     = sizeof mii;
            mii.fMask      = MIIM_STRING;
            mii.dwTypeData = zoomText.data();
            SetMenuItemInfo(menu, 10, TRUE, &mii);
            mii.fMask = MIIM_FTYPE | MIIM_STATE;
            mii.fType = MFT_RADIOCHECK;
            mii.fState = sc.WrapMode() == Scintilla::Wrap::None ? MFS_CHECKED : 0;
            SetMenuItemInfo(menu, ID_SCMSCI_WRAPNONE, FALSE, &mii);
            mii.fState = sc.WrapMode() == Scintilla::Wrap::Char ? MFS_CHECKED : 0;
            SetMenuItemInfo(menu, ID_SCMSCI_WRAPCHAR, FALSE, &mii);
            mii.fState = sc.WrapMode() == Scintilla::Wrap::Word ? MFS_CHECKED : 0;
            SetMenuItemInfo(menu, ID_SCMSCI_WRAPWORD, FALSE, &mii);
            EnableMenuItem(menu, ID_SCMSCI_UNDO           , sc.CanUndo()         ? MF_ENABLED : MF_GRAYED);
            EnableMenuItem(menu, ID_SCMSCI_REDO           , sc.CanRedo()         ? MF_ENABLED : MF_GRAYED);
            EnableMenuItem(menu, ID_SCMSCI_CUT            , hasSelection         ? MF_ENABLED : MF_GRAYED);
            EnableMenuItem(menu, ID_SCMSCI_COPY           , hasSelection         ? MF_ENABLED : MF_GRAYED);
            EnableMenuItem(menu, ID_SCMSCI_PASTE          , sc.CanPaste()        ? MF_ENABLED : MF_GRAYED);
            EnableMenuItem(menu, ID_SCMSCI_DELETE         , hasSelection         ? MF_ENABLED : MF_GRAYED);
            EnableMenuItem(menu, ID_SCMSCI_ZOOMIN         , zoom < 60            ? MF_ENABLED : MF_GRAYED);
            EnableMenuItem(menu, ID_SCMSCI_ZOOMOUT        , zoom > -10           ? MF_ENABLED : MF_GRAYED);
            EnableMenuItem(menu, ID_SCMSCI_ZOOMDEFAULT    , zoom != 0            ? MF_ENABLED : MF_GRAYED);
            EnableMenuItem(menu, ID_SCMSCI_INSERTSELECTION, documentHasSelection ? MF_ENABLED : MF_GRAYED);
            if (contextID == IDC_SIF_FOLDER || contextID == IDC_SIF_FILTER)
                EnableMenuItem(menu, ID_SCMSCI_EXCHANGE, MF_GRAYED);
            std::vector<std::wstring> historyList;
            const auto& history = sc.history.get();
            if (!history.empty() && !(history.size() == 1 && history.back().empty())) {
                AppendMenu(menu, MF_SEPARATOR, 0, 0);
                int menuCounter = 5000;
                for (const std::string& hist : history) {
                    std::wstring item = utf8to16(hist);
                    if (item.empty()) {
                        historyList.push_back(L"");
                        ++menuCounter;
                        continue;
                    }
                    constexpr size_t limit_width = 60;
                    constexpr size_t trail_width = 12;
                    std::wstring text;
                    for (size_t i = 0; i < item.length(); ++i) {
                        switch (item[i]) {
                        case L'\t':
                            text += L"\u2B72";
                            break;
                        case L'\n':
                            text += L"\u240A";
                            break;
                        case L'\r':
                            if (i + 1 < item.length() && item[i + 1] == L'\n') {
                                text += L"\u21A9";
                                ++i;
                            }
                            else text += L"\u240D";
                            break;
                        case L'&':
                            text += L"&&";
                            break;
                        default:
                            text += item[i];
                        }
                    }
                    if (text.back() == L' ') /* trailing invisible blanks might be confusing */ {
                        size_t p = text.find_last_not_of(L' ');
                        if (p == std::wstring::npos) p = 0;
                        for (; p < text.length(); ++p) text[p] = L'·';
                    }
                    if (text.length() > limit_width)
                        text = text.substr(0, limit_width - trail_width - 1) + L'…' + text.substr(text.length() - trail_width);
                    historyList.push_back(text);
                    AppendMenu(menu, MF_STRING, ++menuCounter, text.data());
                }
            }
            int result = TrackPopupMenu(menu, TPM_NONOTIFY | TPM_RETURNCMD,
                                        screenLocation.x, screenLocation.y, 0, contextControl, 0);
            DestroyMenu(menu);
            switch (result) {
            case ID_SCMSCI_UNDO           : sc.Undo     (); break;
            case ID_SCMSCI_REDO           : sc.Redo     (); break;
            case ID_SCMSCI_CUT            : sc.Cut      (); break;
            case ID_SCMSCI_COPY           : sc.Copy     (); break;
            case ID_SCMSCI_PASTE          : sc.Paste    (); break;
            case ID_SCMSCI_DELETE         : sc.Clear    (); break;
            case ID_SCMSCI_SELECTALL      : sc.SelectAll(); break;
            case ID_SCMSCI_ZOOMIN         : sc.ZoomIn   (); break;
            case ID_SCMSCI_ZOOMOUT        : sc.ZoomOut  (); break;
            case ID_SCMSCI_ZOOMDEFAULT    : sc.SetZoom (0); break;
            case ID_SCMSCI_WRAPNONE       : sc.SetWrapMode(Scintilla::Wrap::None); break;
            case ID_SCMSCI_WRAPCHAR       : sc.SetWrapMode(Scintilla::Wrap::Char); break;
            case ID_SCMSCI_WRAPWORD       : sc.SetWrapMode(Scintilla::Wrap::Word); break;
            case ID_SCMSCI_INSERTSELECTION: processScintillaShortcut(sc, 'i'); break;
            case ID_SCMSCI_COPYFIND       : processScintillaShortcut(sc, 'f'); break;
            case ID_SCMSCI_COPYREPLACE    : processScintillaShortcut(sc, 'r'); break;
            case ID_SCMSCI_EXCHANGE       : processScintillaShortcut(sc, 'e'); break;
            default:
                if (result > 5000 && result <= 5000 + static_cast<int>(history.size())) {
                    sc.TargetWholeDocument();
                    sc.ReplaceTarget(history[result - 5001]);
                }
            }
            return TRUE;
        }

        }
        return FALSE;
    }

    }

    return FALSE;

}

}


void closeSearchInFilesDialog() {
    if (data.searchInFilesDialog) SendMessage(data.searchInFilesDialog, WM_COMMAND, IDCANCEL, 0);
}

void colorSif() {
    if (!data.searchInFilesDialog) return;
    constexpr ULONG dmfSetThemeDirectly = 0x00000010UL;
    npp(NPPM_DARKMODESUBCLASSANDTHEME, dmfSetThemeDirectly, ucd.folderCntl.handle);
    npp(NPPM_DARKMODESUBCLASSANDTHEME, dmfSetThemeDirectly, ucd.filterCntl.handle);
    npp(NPPM_DARKMODESUBCLASSANDTHEME, dmfSetThemeDirectly, ucd.findCntl  .handle);
    npp(NPPM_DARKMODESUBCLASSANDTHEME, dmfSetThemeDirectly, ucd.replCntl  .handle);
    plugin.getScintillaPointers();
    ScintillaControl::Configuration cfg(sci);
    cfg.put(ucd.folderCntl);
    cfg.put(ucd.filterCntl);
    cfg.put(ucd.findCntl  );
    cfg.put(ucd.replCntl  );
    isDarkMode = npp(NPPM_ISDARKMODEENABLED, 0, 0);
    if (isDarkMode) npp(NPPM_GETDARKMODECOLORS, sizeof darkModeColors, &darkModeColors);
    enableDisableDependentControls(data.searchInFilesDialog);
}

void showSearchInFilesDialog() {
    if (data.searchInFilesDialog) SetForegroundWindow(data.searchInFilesDialog);
    else {
        data.searchInFilesDialog =
            CreateDialog(plugin.dllInstance, MAKEINTRESOURCE(IDD_SIF), plugin.nppData._nppHandle, mainDialogProc);
        ShowWindow(data.searchInFilesDialog, SW_NORMAL);
    }
}
// This file is part of Search++.
// Copyright 2026 by by Randy Fellmy <https://www.coises.com/>.

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
#include "resource.h"
#include "Shlwapi.h"

void changeDialogLayout();

namespace {

INT_PTR CALLBACK settingsDialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {

    switch (uMsg) {

    case WM_DESTROY:
        return TRUE;

    case WM_INITDIALOG:
    {
        config_rect::show(hwndDlg);  // centers dialog on owner client area, without saving position
        switch (data.dialogLayout) {
        case DialogLayout::Horizontal: CheckRadioButton(hwndDlg, IDC_SETTINGS_DOCKING, IDC_SETTINGS_ADAPTIVE, IDC_SETTINGS_HORIZONTAL); break;
        case DialogLayout::Vertical  : CheckRadioButton(hwndDlg, IDC_SETTINGS_DOCKING, IDC_SETTINGS_ADAPTIVE, IDC_SETTINGS_VERTICAL  ); break;
        case DialogLayout::Adaptive  : CheckRadioButton(hwndDlg, IDC_SETTINGS_DOCKING, IDC_SETTINGS_ADAPTIVE, IDC_SETTINGS_ADAPTIVE  ); break;
        default                      : CheckRadioButton(hwndDlg, IDC_SETTINGS_DOCKING, IDC_SETTINGS_ADAPTIVE, IDC_SETTINGS_DOCKING   );
        }

        SendDlgItemMessage(hwndDlg, IDC_SETTINGS_FILLCHARS_SPIN, UDM_SETRANGE32, 1, 9999);
        SendDlgItemMessage(hwndDlg, IDC_SETTINGS_FILLLINES_SPIN, UDM_SETRANGE32, 1, 9999);
        SendDlgItemMessage(hwndDlg, IDC_SETTINGS_SELCHARS_SPIN , UDM_SETRANGE32, 1, 9999);
        SendDlgItemMessage(hwndDlg, IDC_SETTINGS_SELLINES_SPIN , UDM_SETRANGE32, 1, 9999);

        data.fillSearch           .put(hwndDlg, IDC_SETTINGS_FILL                      );
        data.fillSearchLimit      .put(hwndDlg, IDC_SETTINGS_FILL_LIMIT                );
        data.fillChars            .put(hwndDlg, IDC_SETTINGS_FILLCHARS_SPIN            );
        data.fillLines            .put(hwndDlg, IDC_SETTINGS_FILLLINES_SPIN            );
        data.fillNothing          .put(hwndDlg, IDC_SETTINGS_FILL_NOTHING              );
        data.autoSearchSelect     .put(hwndDlg, IDC_SETTINGS_AUTOSEARCH_SELECTION      );
        data.autoSearchSelectLimit.put(hwndDlg, IDC_SETTINGS_AUTOSEARCH_SELECTION_LIMIT);
        data.selChars             .put(hwndDlg, IDC_SETTINGS_SELCHARS_SPIN             );
        data.selLines             .put(hwndDlg, IDC_SETTINGS_SELLINES_SPIN             );
        data.selectionToMarks     .put(hwndDlg, IDC_SETTINGS_TOMARKS                   );
        data.indicator            .put(hwndDlg, IDC_SETTINGS_MARKSTYLE                 );
        data.autoSearchMarked     .put(hwndDlg, IDC_SETTINGS_AUTOSEARCH_MARKS          );
        data.autoClearMarks       .put(hwndDlg, IDC_SETTINGS_AUTOCLEAR_MARKS           );
        data.focusStepwise        .put(hwndDlg, IDC_SETTINGS_FOCUS_STEPWISE            );
        data.focusSelect          .put(hwndDlg, IDC_SETTINGS_FOCUS_SELECT              );
        data.focusShow            .put(hwndDlg, IDC_SETTINGS_FOCUS_SHOW                );
        data.focusResults         .put(hwndDlg, IDC_SETTINGS_FOCUS_RESULTS             );
        data.clearSelections      .put(hwndDlg, IDC_SETTINGS_CLEARSELECTIONS           );
        data.clearMarked          .put(hwndDlg, IDC_SETTINGS_CLEARMARKED               );
        data.hideBeforeShow       .put(hwndDlg, IDC_SETTINGS_HIDEBEFORESHOW            );

        EnableWindow(GetDlgItem(hwndDlg, IDC_SETTINGS_FILL_LIMIT                ), data.fillSearch                                     ? TRUE : FALSE);
        EnableWindow(GetDlgItem(hwndDlg, IDC_SETTINGS_FILLCHARS_EDIT            ), data.fillSearch && data.fillSearchLimit             ? TRUE : FALSE);
        EnableWindow(GetDlgItem(hwndDlg, IDC_SETTINGS_FILLCHARS_SPIN            ), data.fillSearch && data.fillSearchLimit             ? TRUE : FALSE);
        EnableWindow(GetDlgItem(hwndDlg, IDC_SETTINGS_FILLLINES_EDIT            ), data.fillSearch && data.fillSearchLimit             ? TRUE : FALSE);
        EnableWindow(GetDlgItem(hwndDlg, IDC_SETTINGS_FILLLINES_SPIN            ), data.fillSearch && data.fillSearchLimit             ? TRUE : FALSE);
        EnableWindow(GetDlgItem(hwndDlg, IDC_SETTINGS_FILL_NOTHING              ), data.fillSearch                                     ? TRUE : FALSE);
        EnableWindow(GetDlgItem(hwndDlg, IDC_SETTINGS_AUTOSEARCH_SELECTION_LIMIT), data.autoSearchSelect                               ? TRUE : FALSE);
        EnableWindow(GetDlgItem(hwndDlg, IDC_SETTINGS_SELCHARS_EDIT             ), data.autoSearchSelect && data.autoSearchSelectLimit ? TRUE : FALSE);
        EnableWindow(GetDlgItem(hwndDlg, IDC_SETTINGS_SELCHARS_SPIN             ), data.autoSearchSelect && data.autoSearchSelectLimit ? TRUE : FALSE);
        EnableWindow(GetDlgItem(hwndDlg, IDC_SETTINGS_SELLINES_EDIT             ), data.autoSearchSelect && data.autoSearchSelectLimit ? TRUE : FALSE);
        EnableWindow(GetDlgItem(hwndDlg, IDC_SETTINGS_SELLINES_SPIN             ), data.autoSearchSelect && data.autoSearchSelectLimit ? TRUE : FALSE);
        EnableWindow(GetDlgItem(hwndDlg, IDC_SETTINGS_TOMARKS                   ), data.autoSearchSelect                               ? TRUE : FALSE);

        SendDlgItemMessage(hwndDlg, IDC_SETTINGS_MARKSTYLE, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"Find Mark Style"));
        SendDlgItemMessage(hwndDlg, IDC_SETTINGS_MARKSTYLE, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"Mark Style 1"));
        SendDlgItemMessage(hwndDlg, IDC_SETTINGS_MARKSTYLE, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"Mark Style 2"));
        SendDlgItemMessage(hwndDlg, IDC_SETTINGS_MARKSTYLE, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"Mark Style 3"));
        SendDlgItemMessage(hwndDlg, IDC_SETTINGS_MARKSTYLE, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"Mark Style 4"));
        SendDlgItemMessage(hwndDlg, IDC_SETTINGS_MARKSTYLE, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"Mark Style 5"));
        switch (data.indicator) {
        case 25: SendDlgItemMessage(hwndDlg, IDC_SETTINGS_MARKSTYLE, CB_SETCURSEL, 1, 0); break;
        case 24: SendDlgItemMessage(hwndDlg, IDC_SETTINGS_MARKSTYLE, CB_SETCURSEL, 2, 0); break;
        case 23: SendDlgItemMessage(hwndDlg, IDC_SETTINGS_MARKSTYLE, CB_SETCURSEL, 3, 0); break;
        case 22: SendDlgItemMessage(hwndDlg, IDC_SETTINGS_MARKSTYLE, CB_SETCURSEL, 4, 0); break;
        case 21: SendDlgItemMessage(hwndDlg, IDC_SETTINGS_MARKSTYLE, CB_SETCURSEL, 5, 0); break;
        default: SendDlgItemMessage(hwndDlg, IDC_SETTINGS_MARKSTYLE, CB_SETCURSEL, 0, 0); break;
        }

        npp(NPPM_DARKMODESUBCLASSANDTHEME, NPP::NppDarkMode::dmfInit, hwndDlg);  // Include to support dark mode
        return TRUE;
    }

    case WM_COMMAND:

        switch (LOWORD(wParam)) {

        case IDCANCEL:
            EndDialog(hwndDlg, 1);
            return TRUE;

        case IDOK:
        {

            int spinner;
            if (!data.fillChars.peek(spinner, hwndDlg, IDC_SETTINGS_FILLCHARS_SPIN)) {
                ShowBalloonTip(hwndDlg, IDC_SETTINGS_FILLCHARS_SPIN, L"Must be a number between 1 and 9999.");
                return TRUE;
            }
            if (!data.fillChars.peek(spinner, hwndDlg, IDC_SETTINGS_FILLLINES_SPIN)) {
                ShowBalloonTip(hwndDlg, IDC_SETTINGS_FILLLINES_SPIN, L"Must be a number between 1 and 9999.");
                return TRUE;
            }
            if (!data.fillChars.peek(spinner, hwndDlg, IDC_SETTINGS_SELCHARS_SPIN)) {
                ShowBalloonTip(hwndDlg, IDC_SETTINGS_SELCHARS_SPIN, L"Must be a number between 1 and 9999.");
                return TRUE;
            }
            if (!data.fillChars.peek(spinner, hwndDlg, IDC_SETTINGS_SELLINES_SPIN)) {
                ShowBalloonTip(hwndDlg, IDC_SETTINGS_SELLINES_SPIN, L"Must be a number between 1 and 9999.");
                return TRUE;
            }

            data.fillSearch           .get(hwndDlg, IDC_SETTINGS_FILL                      );
            data.fillSearchLimit      .get(hwndDlg, IDC_SETTINGS_FILL_LIMIT                );
            data.fillChars            .get(hwndDlg, IDC_SETTINGS_FILLCHARS_SPIN            );
            data.fillLines            .get(hwndDlg, IDC_SETTINGS_FILLLINES_SPIN            );
            data.fillNothing          .get(hwndDlg, IDC_SETTINGS_FILL_NOTHING              );
            data.autoSearchSelect     .get(hwndDlg, IDC_SETTINGS_AUTOSEARCH_SELECTION      );
            data.autoSearchSelectLimit.get(hwndDlg, IDC_SETTINGS_AUTOSEARCH_SELECTION_LIMIT);
            data.selChars             .get(hwndDlg, IDC_SETTINGS_SELCHARS_SPIN             );
            data.selLines             .get(hwndDlg, IDC_SETTINGS_SELLINES_SPIN             );
            data.selectionToMarks     .get(hwndDlg, IDC_SETTINGS_TOMARKS                   );
            data.indicator            .get(hwndDlg, IDC_SETTINGS_MARKSTYLE                 );
            data.autoSearchMarked     .get(hwndDlg, IDC_SETTINGS_AUTOSEARCH_MARKS          );
            data.autoClearMarks       .get(hwndDlg, IDC_SETTINGS_AUTOCLEAR_MARKS           );
            data.focusStepwise        .get(hwndDlg, IDC_SETTINGS_FOCUS_STEPWISE            );
            data.focusSelect          .get(hwndDlg, IDC_SETTINGS_FOCUS_SELECT              );
            data.focusShow            .get(hwndDlg, IDC_SETTINGS_FOCUS_SHOW                );
            data.focusResults         .get(hwndDlg, IDC_SETTINGS_FOCUS_RESULTS             );
            data.clearSelections      .get(hwndDlg, IDC_SETTINGS_CLEARSELECTIONS           );
            data.clearMarked          .get(hwndDlg, IDC_SETTINGS_CLEARMARKED               );
            data.hideBeforeShow       .get(hwndDlg, IDC_SETTINGS_HIDEBEFORESHOW            );

            switch (SendDlgItemMessage(hwndDlg, IDC_SETTINGS_MARKSTYLE, CB_GETCURSEL, 0, 0)) {
            case 1 : data.indicator = 25; break;
            case 2 : data.indicator = 24; break;
            case 3 : data.indicator = 23; break;
            case 4 : data.indicator = 22; break;
            case 5 : data.indicator = 21; break;
            default: data.indicator = 31; break;
            }

            DialogLayout dl = IsDlgButtonChecked(hwndDlg, IDC_SETTINGS_HORIZONTAL) == BST_CHECKED ? DialogLayout::Horizontal
                            : IsDlgButtonChecked(hwndDlg, IDC_SETTINGS_VERTICAL  ) == BST_CHECKED ? DialogLayout::Vertical  
                            : IsDlgButtonChecked(hwndDlg, IDC_SETTINGS_ADAPTIVE  ) == BST_CHECKED ? DialogLayout::Adaptive  
                                                                                                  : DialogLayout::Docking;
            if (dl != data.dialogLayout) {
                data.dialogLayout = dl;
                changeDialogLayout();
            }
            EndDialog(hwndDlg, 0);
            return TRUE;

        }

        case IDC_SETTINGS_FILL:
        case IDC_SETTINGS_FILL_LIMIT:
        {
            bool fill      = IsDlgButtonChecked(hwndDlg, IDC_SETTINGS_FILL      ) == BST_CHECKED;
            bool fillLimit = IsDlgButtonChecked(hwndDlg, IDC_SETTINGS_FILL_LIMIT) == BST_CHECKED;
            EnableWindow(GetDlgItem(hwndDlg, IDC_SETTINGS_FILL_LIMIT    ), fill              ? TRUE : FALSE);
            EnableWindow(GetDlgItem(hwndDlg, IDC_SETTINGS_FILLCHARS_EDIT), fill && fillLimit ? TRUE : FALSE);
            EnableWindow(GetDlgItem(hwndDlg, IDC_SETTINGS_FILLCHARS_SPIN), fill && fillLimit ? TRUE : FALSE);
            EnableWindow(GetDlgItem(hwndDlg, IDC_SETTINGS_FILLLINES_EDIT), fill && fillLimit ? TRUE : FALSE);
            EnableWindow(GetDlgItem(hwndDlg, IDC_SETTINGS_FILLLINES_SPIN), fill && fillLimit ? TRUE : FALSE);
            EnableWindow(GetDlgItem(hwndDlg, IDC_SETTINGS_FILL_NOTHING  ), fill              ? TRUE : FALSE);
            return TRUE;
        }
        case IDC_SETTINGS_AUTOSEARCH_SELECTION:
        case IDC_SETTINGS_AUTOSEARCH_SELECTION_LIMIT:
        {
            bool sel      = IsDlgButtonChecked(hwndDlg, IDC_SETTINGS_AUTOSEARCH_SELECTION      ) == BST_CHECKED;
            bool selLimit = IsDlgButtonChecked(hwndDlg, IDC_SETTINGS_AUTOSEARCH_SELECTION_LIMIT) == BST_CHECKED;
            EnableWindow(GetDlgItem(hwndDlg, IDC_SETTINGS_AUTOSEARCH_SELECTION_LIMIT), sel             ? TRUE : FALSE);
            EnableWindow(GetDlgItem(hwndDlg, IDC_SETTINGS_SELCHARS_EDIT             ), sel && selLimit ? TRUE : FALSE);
            EnableWindow(GetDlgItem(hwndDlg, IDC_SETTINGS_SELCHARS_SPIN             ), sel && selLimit ? TRUE : FALSE);
            EnableWindow(GetDlgItem(hwndDlg, IDC_SETTINGS_SELLINES_EDIT             ), sel && selLimit ? TRUE : FALSE);
            EnableWindow(GetDlgItem(hwndDlg, IDC_SETTINGS_SELLINES_SPIN             ), sel && selLimit ? TRUE : FALSE);
            EnableWindow(GetDlgItem(hwndDlg, IDC_SETTINGS_TOMARKS                   ), sel             ? TRUE : FALSE);
            return TRUE;
        }

        }
        return FALSE;

    case WM_DRAWITEM:
        if (wParam == IDC_SETTINGS_MARKSTYLE) {
            const DRAWITEMSTRUCT& dis = *reinterpret_cast<DRAWITEMSTRUCT*>(lParam);
            int indicator;
            switch (dis.itemID) {
            case  1: indicator = 25; break;
            case  2: indicator = 24; break;
            case  3: indicator = 23; break;
            case  4: indicator = 22; break;
            case  5: indicator = 21; break;
            default: indicator = 31; break;
            }
            RECT rect = dis.rcItem;
            int margin = (rect.bottom - rect.top) / 8;
            int side = rect.bottom - rect.top - 2 * margin;
            RECT square = { rect.left + margin,  rect.top + margin, rect.left + margin + side, rect.bottom - margin };
            plugin.getScintillaPointers();
            Scintilla::Colour pageColor = sci.StyleGetBack(0);
            Scintilla::Colour indicatorColor = sci.IndicGetFore(indicator);
            int indicatorAlpha = static_cast<int>(sci.IndicGetAlpha(indicator));
            unsigned int ir = indicatorColor & 255;
            unsigned int ig = (indicatorColor >> 8) & 255;
            unsigned int ib = (indicatorColor >> 16) & 255;
            unsigned int pr = pageColor & 255;
            unsigned int pg = (pageColor >> 8) & 255;
            unsigned int pb = (pageColor >> 16) & 255;
            ir = (indicatorAlpha * ir + (255 - indicatorAlpha) * pr) / 255;
            ig = (indicatorAlpha * ig + (255 - indicatorAlpha) * pg) / 255;
            ib = (indicatorAlpha * ib + (255 - indicatorAlpha) * pb) / 255;
            unsigned int iColor = ir + (ig << 8) + (ib << 16);
            HBRUSH brush = CreateSolidBrush(iColor);
            FillRect(dis.hDC, &square, brush);
            DeleteObject(brush);
            rect.left += side + 3 * margin;
            DrawText(dis.hDC, reinterpret_cast<LPCTSTR>(dis.itemData), -1, &rect, DT_LEFT | DT_SINGLELINE | DT_VCENTER);
            return TRUE;
        }

    }

    return FALSE;
}

}


void showSettingsDialog() {
    DialogBox(plugin.dllInstance, MAKEINTRESOURCE(IDD_SETTINGS), plugin.nppData._nppHandle, settingsDialogProc);
}

// This file is part of "NppCppMSVS: Visual Studio Project Template for a Notepad++ C++ Plugin"
// Copyright 2025 by Randall Joseph Fellmy <software@coises.com>, <http://www.coises.com/software/>

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

// This header defines two structures, OpenDialogBase and SaveDialogBase, which can be used to customize
// and show a Windows Common Item Dialog to open or save a file.
//
// As of this writing, you can find Microsoft documentation for the Common Item (File) Dialogs here:
// https://learn.microsoft.com/en-us/windows/win32/shell/common-file-dialog
// The OpenDialogBase and SaveDialogBase classes manage many of the details of using these dialogs.
//
// This is a preliminary version. It has not yet been tested in a broad variety of contexts, and
// it does not yet directly implement all reasonable features of the Common Item Dialog interface.
// Details will probably change in the future; if possible, compatibility will be maintained.


#pragma once
#define NOMINMAX
#include <windows.h>      // For common windows data types and function headers
#define STRICT_TYPED_ITEMIDS
#include "Shlwapi.h"
#include <shobjidl.h>     // for IFileDialogEvents and IFileDialogControlEvents
#include <string>
#include <vector>

#pragma comment(linker, "\"/manifestdependency:type='Win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")


class FileDialogBase : public IFileDialogEvents, public IFileDialogControlEvents {

protected:

    IFileDialog*          _dialog         = 0;
    IFileDialogCustomize* _customize      = 0;
    DWORD                 _eventHandlerId = 0;
    ULONG                 _refCount       = 0;
    HRESULT               _lastResult     = 0;

    void ThrowOnFail(HRESULT hr, std::string text = "a com operation") /* Throw on failure */ {
        _lastResult = hr;
        if (hr >= 0) return;
        throw std::runtime_error("FileDialogBase: " + text + " failed with error code " + std::to_string(hr) + ".");
    }

    bool FalseOnFail(HRESULT hr) /* Return false on failure */ {
        _lastResult = hr;
        return !_lastResult;
    }

public: 

    #pragma warning( push )
    #pragma warning( disable : 4100 ) // Disable "unused formal parameter" warning
    #pragma warning( disable : 4838 ) // Disable a warning related to QITAB

    // IUnknown virtual functions

    ULONG STDMETHODCALLTYPE AddRef () { return ++_refCount;  }
    ULONG STDMETHODCALLTYPE Release() { return _refCount ? --_refCount : 0; }

    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppv) {
        static const QITAB qit[]
            = { QITABENT(FileDialogBase, IFileDialogEvents), QITABENT(FileDialogBase, IFileDialogControlEvents), { 0 } };
        return QISearch(this, qit, riid, ppv);
    }

    // IFileDialogEvents virtual functions

    STDMETHODIMP OnFileOk         (IFileDialog* pfd)                                                          { return E_NOTIMPL; }
    STDMETHODIMP OnFolderChanging (IFileDialog* pfd, IShellItem* psiFolder)                                   { return E_NOTIMPL; }
    STDMETHODIMP OnFolderChange   (IFileDialog* pfd)                                                          { return E_NOTIMPL; }
    STDMETHODIMP OnSelectionChange(IFileDialog* pfd)                                                          { return E_NOTIMPL; }
    STDMETHODIMP OnShareViolation (IFileDialog* pfd, IShellItem* psi, FDE_SHAREVIOLATION_RESPONSE* pResponse) { return E_NOTIMPL; }
    STDMETHODIMP OnTypeChange     (IFileDialog* pfd)                                                          { return E_NOTIMPL; }
    STDMETHODIMP OnOverwrite      (IFileDialog* pfd, IShellItem* psi, FDE_OVERWRITE_RESPONSE* pResponse)      { return E_NOTIMPL; }

    // IFileDialogControlEvents virtual functions

    STDMETHODIMP OnItemSelected      (IFileDialogCustomize* pfdc, DWORD dwIDCtl, DWORD dwIDItem) { return E_NOTIMPL; }
    STDMETHODIMP OnButtonClicked     (IFileDialogCustomize* pfdc, DWORD dwIDCtl)                 { Close(dwIDCtl); return 0; }
    STDMETHODIMP OnCheckButtonToggled(IFileDialogCustomize* pfdc, DWORD dwIDCtl, BOOL bChecked)  { return E_NOTIMPL; }
    STDMETHODIMP OnControlActivating (IFileDialogCustomize* pfdc, DWORD dwIDCtl)                 { return E_NOTIMPL; }

    #pragma warning( pop )

    FileDialogBase(bool isSave) {
        if (isSave) {
            IFileSaveDialog* dlg;
            ThrowOnFail(CoCreateInstance(CLSID_FileSaveDialog, 0, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&dlg)),
                        "CoCreateInstance(FileSaveDialog)");
            _dialog = dlg;
        }
        else {
            IFileOpenDialog* dlg;
            ThrowOnFail(CoCreateInstance(CLSID_FileOpenDialog, 0, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&dlg)),
                        "CoCreateInstance(FileOpenDialog)");
            _dialog = dlg;
        }
        IFileDialogCustomize* cust;
        ThrowOnFail(_dialog->QueryInterface(IID_PPV_ARGS(&cust)), "Query Interface(IFileDialogCustomize)");
        _customize = cust;
        ThrowOnFail(_dialog->Advise(this, &_eventHandlerId), "Advise");
    }

    virtual ~FileDialogBase() {
        if (_customize) _customize->Release();
        if (_eventHandlerId) _dialog->Unadvise(_eventHandlerId);
        if (_dialog) _dialog->Release();
    }

    // Direct access to the customize interface and the error code of the last call

    IFileDialogCustomize* customize() const { return _customize; }
    HRESULT lastResult() const { return _lastResult; }

    // Delegate common IFileDialog functions

    bool Close(HRESULT code) { return FalseOnFail(_dialog->Close(code)); }

    IShellItem* GetCurrentSelection() {
        IShellItem* isi;
        _lastResult = _dialog->GetCurrentSelection(&isi);
        if (_lastResult) return 0;
        return isi;
    }

    FILEOPENDIALOGOPTIONS GetOptions() {
        FILEOPENDIALOGOPTIONS opt = 0;
        _lastResult = _dialog->GetOptions(&opt);
        return opt;
    }

    IShellItem* GetResult() {
        IShellItem* psi;
        _lastResult = _dialog->GetResult(&psi);
        return _lastResult ? 0 : psi;
    }

    std::wstring GetResultPath() {
        IShellItem* psiResult;
        _lastResult = _dialog->GetResult(&psiResult);
        if (_lastResult) return L"";
        wchar_t* filePath = 0;
        _lastResult = psiResult->GetDisplayName(SIGDN_FILESYSPATH, &filePath);
        if (_lastResult) {
            psiResult->Release();
            return L"";
        }
        std::wstring result(filePath);
        CoTaskMemFree(filePath);
        psiResult->Release();
        return result;
    }

    template<typename T>
    T* QueryInterface() {
        T* p;
        _lastResult = _dialog->QueryInterface(IID_PPV_ARGS(&p));
        if (_lastResult) return 0;
        return p;
    }

    bool SetDefaultExtension(const std::wstring& extension) { return FalseOnFail(_dialog->SetDefaultExtension(extension.data())); }

    bool SetFileName(const std::wstring& name) {
        size_t split = name.rfind(L"\\");
        if (split == std::wstring::npos) return FalseOnFail(_dialog->SetFileName(name.data()));
        IShellItem* ishi;
        _lastResult = SHCreateItemFromParsingName(name.substr(0, split).data(), 0, IID_PPV_ARGS(&ishi));
        if (_lastResult) return false;
        _lastResult = _dialog->SetFolder(ishi);
        ishi->Release();
        if (_lastResult) return false;
        return FalseOnFail(_dialog->SetFileName(name.substr(split + 1).data()));
    }

    bool SetFileNameLabel(const std::wstring& label) { return FalseOnFail(_dialog->SetFileNameLabel(label.data())); }

    bool SetFileTypeIndex(int n) { return FalseOnFail(_dialog->SetFileTypeIndex(n)); }

    bool SetFileTypes(const std::wstring& types) {
        _lastResult = 1;  // Not a com code; set if parsing the string fails and SetFileTypes is never called
        std::vector<std::wstring> fsName, fsSpec;
        int count = 0;
        for (size_t p = 0;;) {
            size_t q = types.find(L"|", p);
            if (q == std::string::npos || p == q) return false;
            fsName.push_back(types.substr(p, q - p));
            if (++q == types.length()) return false;
            size_t r = types.find(L"|", q);
            if (q == r) return false;
            fsSpec.push_back(types.substr(q, r - q));
            ++count;
            if (r == std::wstring::npos) break;
            p = r + 1;
        }
        std::vector<COMDLG_FILTERSPEC> ft(count);
        for (int i = 0; i < count; ++i) {
            ft[i].pszName = fsName[i].data();
            ft[i].pszSpec = fsSpec[i].data();
        }
        return FalseOnFail(_dialog->SetFileTypes(static_cast<UINT>(count), &ft[0]));
    }

    bool SetOkButtonLabel(const std::wstring& label) { return FalseOnFail(_dialog->SetOkButtonLabel(label.data())); }

    bool SetOptions(FILEOPENDIALOGOPTIONS opt) { return FalseOnFail(_dialog->SetOptions(opt)); }

    bool SetTitle(const std::wstring& title) { return FalseOnFail(_dialog->SetTitle(title.data())); }
        
    bool Show(HWND owner = 0) {
        _lastResult = _dialog->Show(owner);
        if ((_lastResult & 0xFFFF) == ERROR_CANCELLED || _lastResult > 0) return false;
        ThrowOnFail(_lastResult, "Show");
        return true;
    }

    // Delegate common IFileDialogCustomize functions

    bool AddCheckButton(DWORD id, const std::wstring& label, bool checked) {
        return FalseOnFail(_customize->AddCheckButton(id, label.data(), checked));
    }

    bool AddControlItem(DWORD controlId, DWORD itemId, const std::wstring& label) {
        return FalseOnFail(_customize->AddControlItem(controlId, itemId, label.data()));
    }

    bool AddPushButton(DWORD id, const std::wstring& label) {
        return FalseOnFail(_customize->AddPushButton(id, label.data()));
    }

    bool AddText(DWORD id, const std::wstring& label) {
        return FalseOnFail(_customize->AddText(id, label.data()));
    }

    bool EnableOpenDropDown(DWORD id) {
        return FalseOnFail(_customize->EnableOpenDropDown(id));
    }

    bool GetCheckButtonState(DWORD id) {
        BOOL checked;
        return FalseOnFail(_customize->GetCheckButtonState(id, &checked)) ? checked : false;
    }

    DWORD GetSelectedControlItem(DWORD id) {
        DWORD result;
        return FalseOnFail(_customize->GetSelectedControlItem(id, &result)) ? result : 0;
    }

    bool MakeProminent(DWORD id) {
        return FalseOnFail(_customize->MakeProminent(id));
    }

    bool SetCheckButtonState(DWORD id, bool checked) {
        return FalseOnFail(_customize->SetCheckButtonState(id, checked));
    }

};


struct OpenDialogBase : FileDialogBase {

    OpenDialogBase() : FileDialogBase(false) {}
    ~OpenDialogBase() {}
    IFileOpenDialog* dialog() const { return dynamic_cast<IFileOpenDialog*>(_dialog); }
    IFileOpenDialog* operator->() const { return dialog(); }

    IShellItemArray* GetResults() {
        IShellItemArray* isia;
        _lastResult = dynamic_cast<IFileOpenDialog*>(_dialog)->GetResults(&isia);
        return _lastResult ? 0 : isia;
    }

    IShellItemArray* GetSelectedItems() {
        IShellItemArray* isia;
        _lastResult = dynamic_cast<IFileOpenDialog*>(_dialog)->GetSelectedItems(&isia);
        return _lastResult ? 0 : isia;
    }

};


struct SaveDialogBase : FileDialogBase {
    SaveDialogBase() : FileDialogBase(true) {}
    ~SaveDialogBase() {}
    IFileSaveDialog* dialog() const { return dynamic_cast<IFileSaveDialog*>(_dialog); }
    IFileSaveDialog* operator->() const { return dialog(); }
};

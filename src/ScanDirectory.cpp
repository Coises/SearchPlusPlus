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

#include "SearchInFiles.h"

void scanDirectory(const std::wstring& targetDir, concurrency::cancellation_token token, HWND inform) {

    std::wstring pattern = targetDir + L"\\*";
    WIN32_FIND_DATAW fd;
    HANDLE hFind = FindFirstFileEx(pattern.c_str(), FindExInfoBasic, &fd, FindExSearchNameMatch, 0, FIND_FIRST_EX_LARGE_FETCH);
    if (hFind == INVALID_HANDLE_VALUE) return;

    do {

        if (token.is_canceled()) break;
        std::wstring name = fd.cFileName;
        if (name == L"." || name == L"..") continue;

        if (sif.fileSpecification.skipHidden && (fd.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN)) continue;

        std::wstring fullPath = targetDir + L"\\" + name;
        if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            if (sif.fileSpecification.recursive) scanDirectory(fullPath, token, inform);
            continue;
        }

        ULARGE_INTEGER uli;
        uli.LowPart  = fd.nFileSizeLow;
        uli.HighPart = fd.nFileSizeHigh;
        uint64_t size = uli.QuadPart;
        if (size < sif.fileSpecification.minSize || size > sif.fileSpecification.maxSize) continue;
        if (sif.fileSpecification.timePoint != FileSpecification::TimeNone) {
            switch (sif.fileSpecification.timePoint) {
            case FileSpecification::TimeAccess:
                uli.LowPart  = fd.ftLastAccessTime.dwLowDateTime;
                uli.HighPart = fd.ftLastAccessTime.dwHighDateTime;
                break;
            case FileSpecification::TimeCreation:
                uli.LowPart  = fd.ftCreationTime.dwLowDateTime;
                uli.HighPart = fd.ftCreationTime.dwHighDateTime;
                break;
            case FileSpecification::TimeModification:
                uli.LowPart  = fd.ftLastWriteTime.dwLowDateTime;
                uli.HighPart = fd.ftLastWriteTime.dwHighDateTime;
                break;
            }
            if (uli.QuadPart < sif.fileSpecification.minTime || uli.QuadPart > sif.fileSpecification.maxTime) continue;
        }
        if (sif.fileSpecification.useFilter) {
            std::string relativeName = utf16to8(fullPath.substr(sif.fileSpecification.path.length() + 1));
            if (!sif.fileSpecification.filter.search(relativeName)) continue;
        }
        SearchableFile& sf = SearchableFile::queue.emplace_back();
        sf.cancel_source = Concurrency::cancellation_token_source::create_linked_source(token);
        sf.status        = SearchableFile::Status::Waiting;
        sf.size          = size;
        sf.filePath      = fullPath;
        if (SearchableFile::queue.size() % 200 == 0) {
            PostMessage(inform, WM_APP_UPDATE_COUNT, SearchableFile::queue.size(), 0);
        }

    } while (FindNextFileW(hFind, &fd));

    FindClose(hFind);

}

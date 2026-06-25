// This file is part of Coises' SearchFiles,
// Copyright 2026 by by Randy Fellmy <https://www.coises.com/>.

// The source code contained in this file is released under the MIT (Expat) license:
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

#include "Framework/UtilityFrameworkMIT.h"
#include "SearchInFiles.h"


std::vector<SearchableFile> SearchableFile::queue;

void SearchableFile::release() {
    if (mapping) {
        if (data) UnmapViewOfFile(data);
        CloseHandle(mapping);
        mapping = 0;
    }
    if (file != INVALID_HANDLE_VALUE) {
        CloseHandle(file);
        file = INVALID_HANDLE_VALUE;
    }
    buffer.reset();
    data = 0;
    text = std::string_view{};
}

bool SearchableFile::is_canceled() {
    if (!cancel_token.is_canceled()) return false;
    release();
    status = Status::Canceled;
    return true;
}


bool SearchableFile::read(char* smallBuffer, size_t smallBufferSize) {

    if (cancel_token.is_canceled()) return false;
    status = Status::Reading;

    // If the file size is zero, there is nothing to read

    if (!size) {
        encoding = Encoding::ASCII;
        codepage = CP_UTF8;
        return true;
    }

    // Attempt to open the file with basic read privileges

    file = CreateFile(filePath.data(), GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, 0);

    if (file == INVALID_HANDLE_VALUE) {
        errcode = GetLastError();
        error   = ErrorType::Creating;
        status  = Status::Error;
        release();
        return false;
    }

    if (is_canceled()) return false;

    // Be sure it is a disk file, as anything else will not work.

    DWORD fileType = GetFileType(file);
    if (fileType != FILE_TYPE_DISK) {
        if (fileType == FILE_TYPE_UNKNOWN) errcode = GetLastError();
        error  = ErrorType::NotDisk;
        status = Status::Error;
        release();
        return false;
    }

    if (is_canceled()) return false;


    // Files no larger than the buffer supplied to this routine are read directly into that buffer.

    if (size <= smallBufferSize) {
        DWORD bytesRead = 0;
        if (ReadFile(file, smallBuffer, static_cast<DWORD>(size), &bytesRead, 0)) data = smallBuffer;
        else {
            errcode = GetLastError();
            error   = ErrorType::Reading;
            status  = Status::Error;
            release();
            return false;
        }
        CloseHandle(file);
        file = INVALID_HANDLE_VALUE;
    }

    // Attempt to map larger files if they are local files; otherwise, skip mapping and just read the file.

    else {
        constexpr ULONG REMOTE_PROTOCOL_FLAG_LOOPBACK = 1;
        FILE_REMOTE_PROTOCOL_INFO frpi = { 0 };
        frpi.StructureVersion = 2;
        frpi.StructureSize = sizeof frpi;
        if (!GetFileInformationByHandleEx(file, FileRemoteProtocolInfo, &frpi, sizeof frpi)
         || !frpi.Protocol || (frpi.Flags & REMOTE_PROTOCOL_FLAG_LOOPBACK) != 0) {
            mapping = CreateFileMapping(file, 0, PAGE_READONLY, 0, 0, 0);
            if (mapping) {
                data = reinterpret_cast<char*>(MapViewOfFile(mapping, FILE_MAP_READ, 0, 0, 0));
                if (data) {
                    CloseHandle(file);
                    file = INVALID_HANDLE_VALUE;
                }
                else {
                    CloseHandle(mapping);
                    mapping = 0;
                }
            }
        }
        if (!data) {
            // If the file was not successfully mapped, attempt to allocate a buffer and read the entire file into it.
            try {
                buffer = std::make_unique<char[]>(size);
            }
            catch (...) {
                error  = ErrorType::Buffering;
                status = Status::Error;
                release();
                return false;
            }
            DWORD bytesRead = 0;
            if (ReadFile(file, buffer.get(), static_cast<DWORD>(size), &bytesRead, 0)) data = buffer.get();
            else {
                errcode = GetLastError();
                error   = ErrorType::Reading;
                status  = Status::Error;
                release();
                return false;
            }
            CloseHandle(file);
            file = INVALID_HANDLE_VALUE;
        }
    }

    if (is_canceled()) return false;

    status = Status::Examining;

    // Determine encoding and codepage

    if (size >= 3 && data[0] == '\xEF' && data[1] == '\xBB' && data[2] == '\xBF') {
        text = std::string_view(data + 3, size - 3);
        encoding = Encoding::UTF8BOM;
        codepage = CP_UTF8;
    }
    else if (size >= 2 && data[0] == '\xFF' && data[1] == '\xFE') {
        text = std::string_view(data + 2, size - 2);
        encoding = Encoding::UTF16LE;
        codepage = 1200;
    }
    else if (size >= 2 && data[0] == '\xFE' && data[1] == '\xFF') {
        text = std::string_view(data + 2, size - 2);
        encoding = Encoding::UTF16BE;
        codepage = 1201;
    }
    else {
        bool allASCII = true;
        const char* const stop = data + size;
        for (const char* p = data;;) {
            if (p >= stop) {
                encoding = allASCII ? Encoding::ASCII : Encoding::UTF8;
                codepage = CP_UTF8;
                break;
            }
            size_t n = utf8byte::implicit_length(*p);
            p += n;
            if (n == 1) continue;
            allASCII = false;
            if (p <= stop) switch (n) {
            case 1: continue;
            case 2: if (utf8byte::isTrail(*(p - 1))) continue; break;
            case 3: if (utf8byte::valid_trail(*(p - 3), *(p - 2), *(p - 1))) continue; break;
            case 4: if (utf8byte::valid_trail(*(p - 4), *(p - 3), *(p - 2), *(p - 1))) continue; break;
            }
            CPINFO cpi;
            codepage = GetACP();
            if (codepage == CP_UTF8) GetLocaleInfoEx(LOCALE_NAME_SYSTEM_DEFAULT, LOCALE_IDEFAULTANSICODEPAGE | LOCALE_RETURN_NUMBER,
                reinterpret_cast<LPTSTR>(&codepage), 2);
            GetCPInfo(codepage, &cpi);
            encoding = cpi.MaxCharSize == 1 ? Encoding::SingleByte : Encoding::DoubleByte;
            break;
        }
        text = std::string_view(data, size);
    }

    return true;

}


void SearchableFile::search() {

    char smallBuffer[4096];

    cancel_token = cancel_source.get_token();

    if (!read(smallBuffer, sizeof smallBuffer)) {
        if (status != Status::Canceled && status != Status::Error && status != Status::Finished)
            status = cancel_token.is_canceled() ? Status::Canceled : Status::Error;
        release();
        return;
    }
    if (is_canceled()) return;

    status = Status::Searching;

    RegularExpression rx(sif.rx);
    rx.setup(codepage);

    if (encoding == Encoding::UTF16LE || encoding == Encoding::UTF16BE) {
        const size_t text_len  = text.length();
        const size_t wtext_len = text.length() / 2;
        const std::wstring_view wtext(reinterpret_cast<const wchar_t*>(text.data()), wtext_len);
        const wchar_t        LF   = encoding == Encoding::UTF16LE ? L'\n'   : L'\x0a00'      ;
        const wchar_t        CR   = encoding == Encoding::UTF16LE ? L'\r'   : L'\x0d00'      ;
        const wchar_t* const CRLF = encoding == Encoding::UTF16LE ? L"\r\n" : L"\x0d00\x0a00";
        intptr_t line_idx = -1;
        size_t   line_pos = 0;
        size_t   next_pos = 0;
        for (size_t index = 0;;) {
            if (!rx.search(text, index, message)) break;
            ++matches_found;
            const size_t match_pos = rx.position();
            bytes_processed = index = match_pos + rx.length();
            if (match_pos >= next_pos) {
                while (match_pos >= next_pos) {
                    ++line_idx;
                    if (!(line_idx & 0xff)) if (is_canceled()) return;
                    line_pos = next_pos;
                    size_t next_wpos = wtext.find_first_of(CRLF, line_pos / 2);
                    if (next_wpos == std::wstring::npos) {
                        next_pos = text_len;
                        if (match_pos == text_len) break;
                    }
                    else {
                        if (wtext[next_wpos++] == CR && next_wpos < text_len / 2 && wtext[next_wpos] == LF) ++next_wpos;
                        next_pos = 2 * next_wpos;
                    }
                }
                results.index.emplace_back();
                results.index.back().lineNumber = line_idx;
                if (encoding == Encoding::UTF16LE) {
                    std::string lineText = utf16to8(wtext.substr(line_pos / 2, (next_pos - line_pos) / 2));
                    results.index.back().length = lineText.length();
                    results.text += lineText;
                }
                else {
                    // something
                }
            }
            if (encoding == Encoding::UTF16LE) {
                size_t p = match_pos == line_pos ? 0 : utf16to8(wtext.substr(line_pos / 2, (match_pos - line_pos) / 2)).length();
                size_t q = index == match_pos ? 0 : utf16to8(wtext.substr(match_pos / 2, (index - match_pos) / 2)).length();
                results.index.back().matches.emplace_back(p, q);
            }
            else {
                // something
            }
            if (index == match_pos) ++index;
            if (is_canceled()) return;
        }
    }

    else {
        const size_t text_len = text.length();
        intptr_t line_idx = -1;
        size_t   line_pos = 0;
        size_t   next_pos = 0;
        for (size_t index = 0;;) {
            if (!rx.search(text, index, message)) break;
            ++matches_found;
            const size_t match_pos = rx.position();
            bytes_processed = index = match_pos + rx.length();
            if (match_pos >= next_pos) {
                while (match_pos >= next_pos) {
                    ++line_idx;
                    if (!(line_idx & 0xff)) if (is_canceled()) return;
                    line_pos = next_pos;
                    next_pos = text.find_first_of("\r\n", line_pos);
                    if (next_pos == std::string::npos) {
                        next_pos = text_len;
                        if (match_pos == text_len) break;
                    }
                    else if (text[next_pos++] == '\r' && next_pos < text_len && text[next_pos] == '\n') ++next_pos;
                }
                results.index.emplace_back();
                results.index.back().lineNumber = line_idx;
                if (codepage == CP_UTF8) {
                    results.index.back().length = next_pos - line_pos;
                    results.text += text.substr(line_pos, next_pos - line_pos);
                }
                else {
                    std::string lineText = utf16to8(toWide(text.substr(line_pos, next_pos - line_pos), codepage));
                    results.index.back().length = lineText.length();
                    results.text += lineText;
                }
            }
            if (codepage == CP_UTF8) results.index.back().matches.emplace_back(match_pos - line_pos, index - match_pos);
            else {
                size_t p = match_pos == line_pos ? 0 : utf16to8(toWide(text.substr(line_pos, match_pos - line_pos), codepage)).length();
                size_t q = index == match_pos ? 0 : utf16to8(toWide(text.substr(match_pos, index - match_pos), codepage)).length();
                results.index.back().matches.emplace_back(p, q);
            }
            if (index == match_pos) ++index;
            if (is_canceled()) return;
        }
    }

    if (matches_found && results.text.back() != '\r' && results.text.back() != '\n') {
        results.index.back().length += 2;
        results.text += "\r\n";
    }
    release();
    if (message.empty()) {
        status = Status::Finished;
        bytes_processed = size;
    }
    else {
        status = Status::Error;
        error = ErrorType::Searching;
    }
    return;

}

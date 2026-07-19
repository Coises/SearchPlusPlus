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

    // Notepad++ can't open absolute file paths longer than MAX_PATH, but there's no reason
    // not to handle them here. If they aren't handled, the resulting error message is confusing,
    // indicating that the file can't be found rather than that the path is too long.

    std::wstring longFilePath = filePath.length() < MAX_PATH || filePath.substr(0, 4) == L"\\\\?\\" ? filePath
        : filePath.substr(0, 2) == L"\\\\" ? L"\\\\?\\UNC\\" + filePath.substr(2)
        : L"\\\\?\\" + filePath;

    file = CreateFile(longFilePath.data(), GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, 0);

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
        text = std::string_view(data + 2, (size - 2) & ~static_cast<size_t>(1));
        encoding = Encoding::UTF16LE;
        codepage = 1200;
    }
    else if (size >= 2 && data[0] == '\xFE' && data[1] == '\xFF') {
        text = std::string_view(data + 2, (size - 2) & ~static_cast<size_t>(1));
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

namespace {

    size_t utf16to8Length(const std::wstring_view w, InvalidUnicode errs = InvalidUnicode::Substitute) {
        size_t s = 0;
        for (size_t i = 0; i < w.length(); ++i) {
            wchar_t c = w[i];
            if (c < 0x80) s += 1;
            else if (c < 0x800) s += 2;
            else if (c < 0xD800 || c > 0xDFFF) s += 3;
            else {
                if (i + 1 < w.length() && c < 0xDC00 && w[i + 1] >= 0xDC00 && w[i + 1] <= 0xDFFF) {
                    s += 4;
                    ++i;
                }
                else if (errs == InvalidUnicode::Preserve_8 && (c >= 0xDC80 && c <= 0xDCFF)) s += 1;
                else s += 3;
            }
        }
        return s;
    }

    size_t utf16BEto8Length(const std::string_view be, InvalidUnicode errs = InvalidUnicode::Substitute) {
        size_t s = 0;
        for (size_t i = 0; i < be.length(); i += 2) {
            wchar_t c = (static_cast<unsigned short>(be[i]) << 8) | static_cast<unsigned char>(be[i + 1]);
            if (c < 0x80) s += 1;
            else if (c < 0x800) s += 2;
            else if (c < 0xD800 || c > 0xDFFF) s += 3;
            else {
                if (i + 3 < be.length() && c < 0xDC00 && static_cast<unsigned char>(be[i + 2]) >= 0xDC
                                                      && static_cast<unsigned char>(be[i + 2]) <= 0xDF) {
                    s += 4;
                    i += 2;
                }
                else if (errs == InvalidUnicode::Preserve_8 && (c >= 0xDC80 && c <= 0xDCFF)) s += 1;
                else s += 3;
            }
        }
        return s;
    }

    inline std::string utf16BEto8(const std::string_view be, InvalidUnicode errs = InvalidUnicode::Substitute) {
        std::string s;
        for (size_t i = 0; i < be.length(); i += 2) {
            wchar_t c = (static_cast<unsigned short>(be[i]) << 8) | static_cast<unsigned char>(be[i + 1]);
            if (c < 0x80) s += static_cast<char>(c);
            else if (c < 0x800) {
                s += static_cast<char>((c >> 6) | 0xC0);
                s += static_cast<char>((c & 0x3F) | 0x80);
            }
            else if (c < 0xD800 || c > 0xDFFF) {
                s += static_cast<char>((c >> 12) | 0xE0);
                s += static_cast<char>(((c >> 6) & 0x3F) | 0x80);
                s += static_cast<char>((c & 0x3F) | 0x80);
            }
            else {
                if (i + 3 < be.length() && c < 0xDC00 && static_cast<unsigned char>(be[i + 2]) >= 0xDC
                                                      && static_cast<unsigned char>(be[i + 2]) <= 0xDF) {
                    char32_t u = ((static_cast<char32_t>(c & 0x3FF) << 10)
                               | ((static_cast<char32_t>(be[i + 2]) << 8) & 0x0300)
                               | (static_cast<char32_t>(be[i + 3]) & 0xFF)) + 0x10000;
                    s += static_cast<char>((u >> 18) | 0xF0);
                    s += static_cast<char>(((u >> 12) & 0x3F) | 0x80);
                    s += static_cast<char>(((u >> 6) & 0x3F) | 0x80);
                    s += static_cast<char>((u & 0x3F) | 0x80);
                    i += 2;
                }
                else if (errs == InvalidUnicode::Preserve_8 && (c >= 0xDC80 && c <= 0xDCFF)) s += static_cast<char>(0xFF & c);
                else if (errs == InvalidUnicode::Preserve_16) {
                    s += static_cast<char>(0xED);
                    s += static_cast<char>(((c >> 6) & 0x3F) | 0x80);
                    s += static_cast<char>((c & 0x3F) | 0x80);
                }
                else s += "\xEF\xBF\xBD";
            }
        }
        return s;
    }

}


template<typename CurrentLine>
bool SearchableFile::searchByLines(RegularExpression& rx) {

    struct CurrentMatch {
        SearchableFile& sf;
        RegularExpression& rx;
        size_t pos, len;
        bool valid = false;
        CurrentMatch(SearchableFile& sf, RegularExpression& rx) : sf(sf), rx(rx), pos(0), len(0) {}
        bool begin(size_t start = 0) {
            if (!rx.search(sf.text, start, sf.message)) {
                valid = false;
                return false;
            }
            pos = rx.position();
            len = rx.length();
            ++sf.matches_found;
            valid = true;
            return true;
        }
        bool next() {
            if (pos == sf.text.length()) return valid = false;
            return begin(len ? pos + len : pos + 1);
        }
    };

    CurrentMatch match(*this, rx);

    if (match.begin()) {
        CurrentLine line(*this);
        do {
            if (match.pos >= line.end) continue;                 // the current match starts in a later line
            if (match.pos < line.pos) {                          // the current match extends from an earlier line onto this line:
                line.push();                                     //     keep a copy of this line
                if (match.pos + match.len > line.end) continue;  //     the current match extends beyond this line
                if (!match.next()) break;                        //     get the next match; escape if there are no more matches
                if (is_canceled()) return true;
                if (match.pos >= line.end) continue;             //     the next (now current) match starts in a later line
            }
            for (;;) {                                           // until there are no more matches that start on this line:
                line.push(match.pos, match.len);                 //     save the current match in this lines' index
                if (match.pos + match.len > line.end) break;     //     the current match extends beyond this line
                if (!match.next()) break;                        //     get the next match; escape if there are no more matches
                if (is_canceled()) return true;
                if (match.pos >= line.end) break;                //     the next (now current) match starts in a later line
            }
            if (!match.valid) break;
        } while (line.advance());
        if (match.valid) line.push(match.pos, match.len);        // This will happen when there is a null match at end of file
    }
    return false;

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

    RegularExpression rx(sif.rx);
    status = Status::Searching;

    if (encoding == Encoding::UTF16LE) {

        struct CurrentLine {
            SearchableFile& sf;
            size_t idx, pos, end;
            CurrentLine(SearchableFile& sf) : sf(sf), idx(0), pos(0), end(endFromPos(sf.text, 0)) {}
            size_t endFromPos(std::string_view s, size_t p) {
                const uint16_t* const w = reinterpret_cast<const uint16_t* const>(s.data());
                const size_t len = s.length();
                size_t wlen = len / 2;
                for (size_t i = p / 2; i < wlen; ++i) {
                    if (w[i] == 0x000A) return (i + 1) * 2;
                    if (w[i] == 0x000D) return (i + 1 < wlen && w[i + 1] == 0x000A) ? (i + 2) * 2 : (i + 1) * 2;
                }
                return len;
            }
            bool advance() {
                if (pos == end) return false;
                if (end == sf.text.length()) {
                    if (sf.text[end - 1] != 0 || (sf.text[end - 2] != '\r' && sf.text[end - 2] != '\n')) return false;
                    pos = end;
                    ++idx;
                    return true;
                }
                pos = end;
                ++idx;
                end = endFromPos(sf.text, pos);
                sf.bytes_processed = pos;
                return true;
            }
            void push() {
                if (sf.results.index.empty()) {  // guess at some initial allocations
                    sf.results.text.reserve(std::min(sf.text.length(), std::max(7 * (end - pos), static_cast<size_t>(4096))));
                    sf.results.index.reserve(16);
                }
                auto& mri = sf.results.index.emplace_back();
                mri.lineNumber = idx;
                std::string lineText =
                    utf16to8(std::wstring_view(reinterpret_cast<const wchar_t*>(sf.text.data() + pos), (end - pos) / 2));
                mri.length = lineText.length();
                sf.results.text += lineText;
            }
            void push(size_t matchPos, size_t matchLen) {
                if (sf.results.index.empty() || sf.results.index.back().lineNumber != static_cast<intptr_t>(idx)) push();
                const std::wstring_view w(reinterpret_cast<const wchar_t*>(sf.text.data()), sf.text.length() / 2);
                size_t p = matchPos == pos ? 0 : utf16to8Length(w.substr(pos / 2, (matchPos - pos) / 2));
                size_t q = matchLen == 0 ? 0 : utf16to8Length(w.substr(matchPos / 2, matchLen / 2));
                sf.results.index.back().matches.emplace_back(p, q);
            }

        };

        rx.custom<RegularExpressionLE>();
        if (searchByLines<CurrentLine>(rx)) return;

    }

    else if (encoding == Encoding::UTF16BE) {

        struct CurrentLine {
            SearchableFile& sf;
            size_t idx, pos, end;
            CurrentLine(SearchableFile& sf) : sf(sf), idx(0), pos(0), end(endFromPos(sf.text, 0)) {}
            size_t endFromPos(std::string_view s, size_t p) {
                const uint16_t* const w = reinterpret_cast<const uint16_t* const>(s.data());
                const size_t len = s.length();
                size_t wlen = len / 2;
                for (size_t i = p / 2; i < wlen; ++i) {
                    if (w[i] == 0x0A00) return (i + 1) * 2;
                    if (w[i] == 0x0D00) return (i + 1 < wlen && w[i + 1] == 0x0A00) ? (i + 2) * 2 : (i + 1) * 2;
                }
                return len;
            }
            bool advance() {
                if (pos == end) return false;
                if (end == sf.text.length()) {
                    if (sf.text[end - 2] != 0 || (sf.text[end - 1] != '\r' && sf.text[end - 1] != '\n')) return false;
                    pos = end;
                    ++idx;
                    return true;
                }
                pos = end;
                ++idx;
                end = endFromPos(sf.text, pos);
                sf.bytes_processed = pos;
                return true;
            }
            void push() {
                if (sf.results.index.empty()) {  // guess at some initial allocations
                    sf.results.text.reserve(std::min(sf.text.length(), std::max(7 * (end - pos), static_cast<size_t>(4096))));
                    sf.results.index.reserve(16);
                }
                auto& mri = sf.results.index.emplace_back();
                mri.lineNumber = idx;
                std::string lineText = utf16BEto8(sf.text.substr(pos, end - pos));
                mri.length = lineText.length();
                sf.results.text += lineText;
            }
            void push(size_t matchPos, size_t matchLen) {
                if (sf.results.index.empty() || sf.results.index.back().lineNumber != static_cast<intptr_t>(idx)) push();
                size_t p = matchPos == pos ? 0 : utf16BEto8Length(sf.text.substr(pos, matchPos - pos));
                size_t q = matchLen == 0 ? 0 : utf16BEto8Length(sf.text.substr(matchPos, matchLen));
                sf.results.index.back().matches.emplace_back(p, q);
            }

        };

        rx.custom<RegularExpressionBE>();
        if (searchByLines<CurrentLine>(rx)) return;

    }

    else {

        struct CurrentLine {
            SearchableFile& sf;
            size_t idx, pos, end;
            CurrentLine(SearchableFile& sf) : sf(sf), idx(0), pos(0), end(endFromPos(sf.text, 0)) {}
            size_t endFromPos(std::string_view s, size_t p) {
                size_t e = s.find_first_of("\r\n", p);
                if (e == std::string::npos) return s.length();
                if (s[e++] == '\r' && e < s.length() && s[e] == '\n') ++e;
                return e;
            }
            bool advance() {
                if (pos == end) return false;
                if (end == sf.text.length()) {
                    if (sf.text[end - 1] != '\r' && sf.text[end - 1] != '\n') return false;
                    pos = end;
                    ++idx;
                    return true;
                }
                pos = end;
                ++idx;
                end = endFromPos(sf.text, pos);
                sf.bytes_processed = pos;
                return true;
            }
            void push() {
                if (sf.results.index.empty()) {  // guess at some initial allocations
                    sf.results.text.reserve(std::min(sf.text.length(), std::max(10 * (end - pos), static_cast<size_t>(4096))));
                    sf.results.index.reserve(16);
                }
                auto& mri = sf.results.index.emplace_back();
                mri.lineNumber = idx;
                if (sf.codepage == CP_UTF8) sf.results.text += sf.text.substr(pos, mri.length = end - pos);
                else {
                    std::string lineText = utf16to8(toWide(sf.text.substr(pos, end - pos), sf.codepage));
                    mri.length = lineText.length();
                    sf.results.text += lineText;
                }
            }
            void push(size_t matchPos, size_t matchLen) {
                if (sf.results.index.empty() || sf.results.index.back().lineNumber != static_cast<intptr_t>(idx)) push();
                if (sf.codepage == CP_UTF8) sf.results.index.back().matches.emplace_back(matchPos - pos, matchLen);
                else {
                    size_t p = matchPos == pos ? 0 : utf16to8Length(toWide(sf.text.substr(pos, matchPos - pos), sf.codepage));
                    size_t q = matchLen == 0 ? 0 : utf16to8Length(toWide(sf.text.substr(matchPos, matchLen), sf.codepage));
                    sf.results.index.back().matches.emplace_back(p, q);
                }
            }

        };

        rx.setup(codepage);
        if (searchByLines<CurrentLine>(rx)) return;

    }

    if (matches_found && (results.index.back().length == 0 || (results.text.back() != '\r' && results.text.back() != '\n'))) {
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

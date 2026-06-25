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

#define NOMINMAX
#include <windows.h>
#include <ppl.h>
#include <vector>
#include <string>
#include <utility>
#include "MatchResults.h"

#pragma warning(push)
#pragma warning(disable : 4324)           // Suppress padding warning -- padding allows cache alignment, desired for multiple tasks

class alignas(64) SearchableFile {

public:

    static std::vector<SearchableFile> queue;

    static void Search(SearchableFile& sf) { sf.search(); }

    enum class Status : uint64_t { None, Error, Canceled, Waiting, Reading, Examining, Searching, Finished };

    Status   status          = Status::None;
    uint64_t matches_found   = 0;
    uint64_t bytes_processed = 0;
    uint64_t size            = 0;

    concurrency::cancellation_token_source cancel_source;
    std::wstring filePath;
    std::string message;

    MatchResults results;

    DWORD errcode  = 0;
    UINT  codepage = 0;
    enum class ErrorType { None, NotDisk, Creating, Buffering, Reading, Searching               } error    = ErrorType::None;
    enum class Encoding  { None, ASCII, UTF8, UTF8BOM, UTF16LE, UTF16BE, SingleByte, DoubleByte } encoding = Encoding::None;

    ~SearchableFile() { release(); }

    void search();

private:

    std::unique_ptr<char[]> buffer;

    char*  data           = 0;
    HANDLE file           = INVALID_HANDLE_VALUE;
    HANDLE mapping        = 0;

    std::string_view text;

    concurrency::cancellation_token cancel_token = concurrency::cancellation_token::none();

    // Note that is_canceled() has side effects: if it returns true, it first does a release() and sets status = Status::Canceled.

    bool is_canceled();
    bool read(char* smallBuffer, size_t smallBufferSize);
    void release();

public:

    // these are needed to support std::vector<SearchableFile>

    SearchableFile() = default;

    SearchableFile(SearchableFile&& other) noexcept         
        : cancel_source(std::move(other.cancel_source))
        , status(other.status)
        , matches_found(other.matches_found)
        , bytes_processed(other.bytes_processed)
        , size(other.size)
        , filePath(std::move(other.filePath))
        , errcode(other.errcode)
        , codepage(other.codepage)
        , error(other.error)
        , encoding(other.encoding)
        , buffer(std::move(other.buffer))
        , text(std::move(other.text))
        , cancel_token(std::move(other.cancel_token))
        , data(std::exchange(other.data, nullptr))
        , file(std::exchange(other.file, INVALID_HANDLE_VALUE))
        , mapping(std::exchange(other.mapping, nullptr))
        , results(std::move(other.results))
    {
    }

    SearchableFile& operator=(SearchableFile&& other) noexcept {
        if (this != &other) {
            cancel_source = std::move(other.cancel_source);
            status = other.status;
            matches_found = other.matches_found;
            bytes_processed = other.bytes_processed;
            size = other.size;
            filePath = std::move(other.filePath);
            errcode = other.errcode;
            codepage = other.codepage;
            error = other.error;
            encoding = other.encoding;
            buffer = std::move(other.buffer);
            text = std::move(other.text);
            cancel_token = std::move(other.cancel_token);
            data = std::exchange(other.data, nullptr);
            file = std::exchange(other.file, INVALID_HANDLE_VALUE);
            mapping = std::exchange(other.mapping, nullptr);
            results = std::move(other.results);
        }
        return *this;
    }

};

#pragma warning(pop)

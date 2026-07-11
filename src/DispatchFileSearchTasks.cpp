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

#include "SearchInFiles.h"
#include "SearchInFilesConfiguration.h"
#include <Shlwapi.h>
#include <format>

void scanDirectory(const std::wstring& targetDir, concurrency::cancellation_token token, HWND inform);

void dispatchSearchTasks(HWND inform) {

    SearchableFile::queue.clear();
    sif.cancel_all_source = concurrency::cancellation_token_source();
    auto cancel_token = sif.cancel_all_source.get_token();

    concurrency::create_task(

        [cancel_token, inform]() {

            scanDirectory(sif.fileSpecification.path, cancel_token, inform);
            if (cancel_token.is_canceled()) {
                for (auto& sf : SearchableFile::queue)
                    if (sf.status == SearchableFile::Status::Waiting) sf.status = SearchableFile::Status::Canceled;
                PostMessage(inform, WM_APP_SEARCH_CANCELED, 0, 0);
                return;
            }

            std::sort(SearchableFile::queue.begin(), SearchableFile::queue.end(),
                [](const SearchableFile& a, const SearchableFile& b) {return a.size > b.size; });

            PostMessage(inform, WM_APP_UPDATE_COUNT, SearchableFile::queue.size(), 1); // 1 flags scanning completed

            try {
                concurrency::run_with_cancellation_token([]() {
                    concurrency::parallel_for_each(SearchableFile::queue.begin(), SearchableFile::queue.end(), SearchableFile::Search,
                        concurrency::simple_partitioner(1));
                }, cancel_token);
            }
            catch (const concurrency::task_canceled&) {}

            if (cancel_token.is_canceled())
                for (auto& sf : SearchableFile::queue)
                    if (sf.status == SearchableFile::Status::Waiting) sf.status = SearchableFile::Status::Canceled;

            if (const size_t queueSize = SearchableFile::queue.size()) {
                std::vector<size_t> alphaOrder(queueSize);
                for (size_t i = 0; i < queueSize; ++i) alphaOrder[i] = i;
                std::sort(alphaOrder.begin(), alphaOrder.end(),
                    [](size_t a, size_t b) {
                        return StrCmpLogicalW(SearchableFile::queue[a].filePath.data(), SearchableFile::queue[b].filePath.data()) < 0;
                    });
                sif.matchResults.index.clear();
                sif.matchResults.text.clear();
                size_t files   = 0;
                size_t matches = 0;
                size_t textlen = 0;
                for (const auto& sf : SearchableFile::queue) {
                    if (sf.matches_found) {
                        ++files;
                        matches += sf.matches_found;
                        textlen += sf.results.text.length() + 62 + 2 * sf.filePath.length();
                    }
                }
                if (matches) {
                    // Using wide character formatting because it honors the UserLocale more reliably
                    std::string singleLineFindText = utf16to8(std::format(UserLocale, L" {:Ld} match{:s} in {:Ld} of {:Ld} file{:s} ",
                        matches, matches == 1 ? L"" : L"es",
                        files, SearchableFile::queue.size(), SearchableFile::queue.size() == 1 ? L"" : L"s"));
                    singleLineFindText += ucd.regex ? "(Regex" : "(Plain text";
                    if (              ucd.matchCase   ) singleLineFindText += ", match case";
                    if (!ucd.regex && ucd.wholeWord   ) singleLineFindText += ", whole word";
                    if ( ucd.regex && ucd.dotAll      ) singleLineFindText += ", dot all";
                    if ( ucd.regex && ucd.freeSpacing ) singleLineFindText += ", free spacing";
                    singleLineFindText += "): ";
                    for (size_t i = 0; i < sif.findString.length(); ++i) {
                        switch (sif.findString[i]) {
                        case '\t':
                            singleLineFindText += reinterpret_cast<const char*>(u8"\u2B72");
                            break;
                        case '\n':
                            singleLineFindText += reinterpret_cast<const char*>(u8"\u240A");
                            break;
                        case '\r':
                            if (i + 1 < sif.findString.length() && sif.findString[i + 1] == '\n') {
                                singleLineFindText += reinterpret_cast<const char*>(u8"\u21A9");
                                ++i;
                            }
                            else singleLineFindText += reinterpret_cast<const char*>(u8"\u240D");
                            break;
                        default:
                            singleLineFindText += sif.findString[i];
                        }
                    }
                    singleLineFindText += "\r\n";
                    sif.matchResults.index.reserve(matches + files + 1);
                    sif.matchResults.text.reserve(singleLineFindText.length() + textlen + 2);
                    sif.matchResults.index.emplace_back();
                    sif.matchResults.index.back().lineNumber = -2;
                    sif.matchResults.index.back().length = singleLineFindText.length();
                    sif.matchResults.text = singleLineFindText;
                    for (size_t i : alphaOrder) {
                        auto& sf = SearchableFile::queue[i];
                        if (sf.matches_found > 0) {
                            size_t linesMatched = sf.results.index.size();
                            std::string fileHeader = utf16to8(std::format(UserLocale, L"   {:Ld} match{:s} in {:Ld} line{:s}: ",
                                sf.matches_found, sf.matches_found == 1 ? L"" : L"es",
                                linesMatched, linesMatched == 1 ? L"" : L"s")
                                + sf.filePath) + "\r\n";
                            sif.matchResults.index.emplace_back();
                            sif.matchResults.index.back().lineNumber = -1;
                            sif.matchResults.index.back().length = fileHeader.length();
                            sif.matchResults.index.insert(sif.matchResults.index.end(),
                                                          sf.results.index.begin(), sf.results.index.end());
                            sif.matchResults.text += fileHeader;
                            sif.matchResults.text += sf.results.text;
                        }
                    }
                }
            }

            PostMessage(inform, cancel_token.is_canceled() ? WM_APP_SEARCH_CANCELED : WM_APP_SEARCH_COMPLETE, 0, 0);

        }, cancel_token);

}

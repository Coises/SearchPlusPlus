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

#include "SearchInFiles.h"
#include <concrt.h>
#include <ppltasks.h>

void processFileSearchResults(HWND inform, bool shadow);
int scanDirectory(const std::wstring& targetDir, concurrency::cancellation_token token, DWORD* error);


void dispatchSearchTasks(HWND inform) {

    SearchableFile::queue = std::make_shared<std::vector<SearchableFile>>();
    sif.cancel_all_source = concurrency::cancellation_token_source();
    auto cancel_token = sif.cancel_all_source.get_token();
    concurrency::Scheduler* pScheduler = concurrency::Scheduler::Create(concurrency::SchedulerPolicy());

    concurrency::create_task([queue = SearchableFile::queue, cancel_token, inform, pScheduler]() {

        pScheduler->Attach();

        DWORD scanDirectoryError = 0;
        int countAllFiles = scanDirectory(sif.fileSpecification.path, cancel_token, &scanDirectoryError);

        if (!queue->empty() && !cancel_token.is_canceled()) {

            std::sort(queue->begin(), queue->end(), [](const auto& a, const auto& b) { return a.size > b.size; });

            PostMessage(inform, WM_APP_SEARCH_STARTED, 0, 0);

            try {
                concurrency::run_with_cancellation_token([queue]() {
                    concurrency::parallel_for_each(queue->begin(), queue->end(),
                        [](auto& sf) { sf.search(); }, concurrency::simple_partitioner(1));
                    }, cancel_token);
            }
            catch (const concurrency::task_canceled&) {}

            if (cancel_token.is_canceled())
                for (auto& sf : *SearchableFile::queue)
                    if (sf.status == SearchableFile::Status::Waiting) sf.status = SearchableFile::Status::Canceled;

        }

        concurrency::CurrentScheduler::Detach();
        pScheduler->Release();

        if (!cancel_token.is_canceled()) {
            if      (!queue->empty())    processFileSearchResults(inform, false);
            else if (scanDirectoryError) PostMessage(inform, WM_APP_SEARCH_COMPLETE, 2, scanDirectoryError);
            else                         PostMessage(inform, WM_APP_SEARCH_COMPLETE, 0, countAllFiles);
        }

    }, cancel_token);

}

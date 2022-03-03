/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <DownloadController.h>
#include <DownloadWorker.h>
#include <PythonBindings.h>


namespace O3DE::ProjectManager
{
    DownloadWorker::DownloadWorker()
        : QObject()
    {
    }

    void DownloadWorker::StartDownload()
    {
        auto gemDownloadProgress = [=](int bytesDownloaded, int totalBytes)
        {
            emit UpdateProgress(bytesDownloaded, totalBytes);
        };
        AZ::Outcome<void, AZStd::pair<AZStd::string, AZStd::string>> gemInfoResult =
            PythonBindingsInterface::Get()->DownloadGem(m_gemName, gemDownloadProgress, /*force*/true);

        if (gemInfoResult.IsSuccess())
        {
            emit Done("", "");
        }
        else
        {
            emit Done(gemInfoResult.GetError().first.c_str(), gemInfoResult.GetError().second.c_str());
        }
    }

    void DownloadWorker::SetGemToDownload(const QString& gemName, bool downloadNow)
    {
        m_gemName = gemName;
        if (downloadNow)
        {
            StartDownload();
        }
    }

} // namespace O3DE::ProjectManager

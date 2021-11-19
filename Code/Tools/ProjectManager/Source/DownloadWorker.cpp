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
        auto gemDownloadProgress = [=](int downloadProgress)
        {
            m_downloadProgress = downloadProgress;
            emit UpdateProgress(downloadProgress);
        };
        AZ::Outcome<void, AZStd::string> gemInfoResult = PythonBindingsInterface::Get()->DownloadGem(m_gemName, gemDownloadProgress);
        if (gemInfoResult.IsSuccess())
        {
            emit Done("");
        }
        else
        {
            emit Done(tr("Gem download failed"));
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

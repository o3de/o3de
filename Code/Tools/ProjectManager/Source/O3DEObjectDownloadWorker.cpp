/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <O3DEObjectDownloadController.h>
#include <O3DEObjectDownloadWorker.h>
#include <PythonBindings.h>

#include <QMessageBox>
#include <QDesktopServices>
#include <QUrl>


namespace O3DE::ProjectManager
{
    O3DEObjectDownloadWorker::O3DEObjectDownloadWorker()
        : QObject()
    {
    }

    void O3DEObjectDownloadWorker::StartDownload()
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

    void O3DEObjectDownloadWorker::SetGemToDownload(const QString& gemName, bool downloadNow)
    {
        m_gemName = gemName;
        if (downloadNow)
        {
            StartDownload();
        }
    }

} // namespace O3DE::ProjectManager

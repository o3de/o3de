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
        auto objectDownloadProgress = [=](int bytesDownloaded, int totalBytes)
        {
            emit UpdateProgress(bytesDownloaded, totalBytes);
        };
        AZ::Outcome<void, AZStd::pair<AZStd::string, AZStd::string>> objectInfoResult;
        if (m_downloadType == DownloadController::DownloadObjectType::Gem)
        {
            objectInfoResult = PythonBindingsInterface::Get()->DownloadGem(m_objectName, objectDownloadProgress, /*force*/ true);
        }
        else if (m_downloadType == DownloadController::DownloadObjectType::Project)
        {
            objectInfoResult = PythonBindingsInterface::Get()->DownloadProject(m_objectName, objectDownloadProgress, /*force*/ true);
        }

        if (objectInfoResult.IsSuccess())
        {
            emit Done("", "");
        }
        else
        {
            emit Done(objectInfoResult.GetError().first.c_str(), objectInfoResult.GetError().second.c_str());
        }
    }

    void DownloadWorker::SetGemToDownload(const QString& gemName, bool downloadNow)
    {
        m_objectName = gemName;
        m_downloadType = DownloadController::DownloadObjectType::Gem;
        if (downloadNow)
        {
            StartDownload();
        }
    }

    void DownloadWorker::SetProjectToDownload(const QString& projectName, bool downloadNow)
    {
        m_objectName = projectName;
        m_downloadType = DownloadController::DownloadObjectType::Project;
        if (downloadNow)
        {
            StartDownload();
        }
    }

    void DownloadWorker::SetTemplateToDownload(const QString& templateName, bool downloadNow)
    {
        m_objectName = templateName;
        m_downloadType = DownloadController::DownloadObjectType::Template;
        if (downloadNow)
        {
            StartDownload();
        }
    }

} // namespace O3DE::ProjectManager

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
        IPythonBindings::DetailedOutcome objectInfoResult;
        if (m_downloadType == DownloadController::DownloadObjectType::Gem)
        {
            objectInfoResult = PythonBindingsInterface::Get()->DownloadGem(m_objectName, m_destinationPath , objectDownloadProgress, /*force*/ true);
        }
        else if (m_downloadType == DownloadController::DownloadObjectType::Project)
        {
            objectInfoResult = PythonBindingsInterface::Get()->DownloadProject(m_objectName, m_destinationPath, objectDownloadProgress, /*force*/ true);
        }
        else if (m_downloadType == DownloadController::DownloadObjectType::Template)
        {
            objectInfoResult = PythonBindingsInterface::Get()->DownloadTemplate(m_objectName, m_destinationPath, objectDownloadProgress, /*force*/ true);
        }
        else
        {
            AZ_Error("DownloadWorker", false, "Unknown download object type %d", m_downloadType);
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

    void DownloadWorker::SetObjectToDownload(const QString& objectName, const QString& destinationPath, DownloadController::DownloadObjectType objectType, bool downloadNow)
    {
        m_objectName = objectName;
        m_destinationPath = destinationPath;
        m_downloadType = objectType;
        if (downloadNow)
        {
            StartDownload();
        }
    }

} // namespace O3DE::ProjectManager

/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/
#include "UiCanvasEditor_precompiled.h"

#include "EditorCommon.h"

#include <Util/PathUtil.h>

#include <QStandardPaths>
#include <QMessageBox>

namespace
{
    QString GetEngineRootDir()
    {
        static bool hasBeenInit = false;
        static QString fullDir;
        if (!hasBeenInit)
        {
            hasBeenInit = true;

            QDir appPath(qApp->applicationDirPath());
            while (!appPath.isRoot())
            {
                if (QFile::exists(appPath.filePath("engineroot.txt")))
                {
                    fullDir = appPath.absolutePath();
                    break;
                }

                if (!appPath.cdUp())
                {
                    break;
                }
            }
        }
        return fullDir;
    }
} // anonymous namespace.

namespace FileHelpers
{
    QString GetAbsoluteDir(const char* subDir)
    {
        QString p = QString::fromLocal8Bit(Path::GetEditingGameDataFolder().c_str()) + "/" + subDir;

        return p;
    }

    QString GetAbsoluteGameDir()
    {
        QString p = QString::fromLocal8Bit(Path::GetEditingGameDataFolder().c_str()) + "/";

        return p;
    }

    QString GetRelativePathFromEngineRoot(const QString& fullPath)
    {
        QString rootPath = GetEngineRootDir() + "/";

        QString result = fullPath;
        if (result.startsWith(rootPath, Qt::CaseInsensitive))
        {
            result = result.remove(0, rootPath.length());
        }
        return result;
    }

    void AppendExtensionIfNotPresent(QString& filename,
        const char* extension)
    {
        QFileInfo fileInfo(filename);
        if (0 != fileInfo.suffix().compare(extension, Qt::CaseInsensitive))
        {
            filename += QString(".") + QString(extension);
        }
    }

    bool FilenameHasExtension(QString& filename,
        const char* extension)
    {
        QFileInfo fileInfo(filename);
        return 0 == fileInfo.suffix().compare(extension, Qt::CaseInsensitive);
    }

    QString GetAppDataPath()
    {
        QStringList appDataDirs(QStandardPaths::standardLocations(QStandardPaths::DataLocation));
        return appDataDirs.empty() ? QString() : appDataDirs.first();
    }

    SourceControlResult SourceControlAddOrEdit(const char* fullPath, QWidget* parent)
    {
        if (!AzToolsFramework::SourceControlCommandBus::FindFirstHandler())
        {
            // No source control provider is present.
            return SourceControlResult::kSourceControlResult_NoSourceControl;
        }

        if (!GetIEditor()->IsSourceControlConnected())
        {
            // Not connected to source control provider.
            return SourceControlResult::kSourceControlResult_NotConnected;
        }

        bool done = false;
        SourceControlResult result = SourceControlResult::kSourceControlResult_Ok;

        using SCCommandBus = AzToolsFramework::SourceControlCommandBus;
        SCCommandBus::Broadcast(&SCCommandBus::Events::RequestEdit, fullPath, true,
            [&done, &result]([[maybe_unused]] bool success, const AzToolsFramework::SourceControlFileInfo& info)
            {
                if (info.m_status == AzToolsFramework::SourceControlStatus::SCS_ProviderIsDown)
                {
                    result = SourceControlResult::kSourceControlResult_SourceControlDown;
                }
                else if (info.m_status == AzToolsFramework::SourceControlStatus::SCS_ProviderError)
                {
                    result = SourceControlResult::kSourceControlResult_SourceControlError;
                }

                done = true;
            });

        // Block until the source control operation is complete.
        while (!done)
        {
            AZ::TickBus::ExecuteQueuedEvents();
            AZStd::this_thread::sleep_for(AZStd::chrono::milliseconds(100));
        }

        if (result == FileHelpers::SourceControlResult::kSourceControlResult_SourceControlDown)
        {
            QMessageBox(QMessageBox::Critical,
                "Error",
                "Source control is down",
                QMessageBox::Ok, parent).exec();
        }
        else if (result == FileHelpers::SourceControlResult::kSourceControlResult_SourceControlError)
        {
            QMessageBox(QMessageBox::Critical,
                "Error",
                "Source control system error. Is your session still valid?",
                QMessageBox::Ok, parent).exec();
        }

        return result;
    }
}   // namespace FileHelpers

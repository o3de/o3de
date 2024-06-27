/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <AzCore/IO/Path/Path.h>
#include <AzCore/Settings/SettingsRegistryMergeUtils.h>

#include "EditorCommon.h"

#include <Util/PathUtil.h>

#include <QStandardPaths>
#include <QMessageBox>

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
        AZ::IO::FixedMaxPath engineRootPath;
        if (auto settingsRegistry = AZ::SettingsRegistry::Get(); settingsRegistry != nullptr)
        {
            settingsRegistry->Get(engineRootPath.Native(), AZ::SettingsRegistryMergeUtils::FilePathKey_EngineRootFolder);
        }

        return AZ::IO::PathView(fullPath.toUtf8().constData()).LexicallyProximate(engineRootPath).c_str();
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

        AzToolsFramework::SourceControlState sourceControlState = AzToolsFramework::SourceControlState::Disabled;
        AzToolsFramework::SourceControlConnectionRequestBus::BroadcastResult(sourceControlState, &AzToolsFramework::SourceControlConnectionRequests::GetSourceControlState);
        if (sourceControlState != AzToolsFramework::SourceControlState::Active)
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

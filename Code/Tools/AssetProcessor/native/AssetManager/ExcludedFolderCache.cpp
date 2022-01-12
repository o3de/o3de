/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <QDirIterator>
#include <AzCore/Settings/SettingsRegistryMergeUtils.h>
#include <native/AssetManager/ExcludedFolderCache.h>
#include <utilities/assetUtils.h>
#include <utilities/PlatformConfiguration.h>
#include <AzCore/IO/Path/Path.h>

namespace AssetProcessor
{
    ExcludedFolderCache::ExcludedFolderCache(const PlatformConfiguration* platformConfig) : m_platformConfig(platformConfig)
    {
        AZ::Interface<ExcludedFolderCacheInterface>::Register(this);
    }

    ExcludedFolderCache::~ExcludedFolderCache()
    {
        AZ::Interface<ExcludedFolderCacheInterface>::Unregister(this);
    }

    const AZStd::unordered_set<AZStd::string>& ExcludedFolderCache::GetExcludedFolders()
    {
        if (!m_builtCache)
        {
            for (int i = 0; i < m_platformConfig->GetScanFolderCount(); ++i)
            {
                const auto& scanFolderInfo = m_platformConfig->GetScanFolderAt(i);
                QDir rooted(scanFolderInfo.ScanPath());
                QString absolutePath = rooted.absolutePath();
                AZStd::stack<QString> dirs;
                dirs.push(absolutePath);
                
                while (!dirs.empty())
                {
                    absolutePath = dirs.top();
                    dirs.pop();

                    // Scan only folders, do not recurse so we have the chance to ignore a subfolder before going deeper
                    QDirIterator dirIterator(absolutePath, QDir::Dirs | QDir::NoSymLinks | QDir::NoDotAndDotDot);

                    // Loop all the folders in this directory
                    while (dirIterator.hasNext())
                    {
                        dirIterator.next();
                        QString pathMatch = rooted.absoluteFilePath(dirIterator.filePath());

                        if (m_platformConfig->IsFileExcluded(pathMatch))
                        {
                            // Add the folder to the list and do not proceed any deeper
                            m_excludedFolders.emplace(pathMatch.toUtf8().constData());
                        }
                        else if (scanFolderInfo.RecurseSubFolders())
                        {
                            // Folder is not excluded and recurse is enabled, add to the list of folders to check
                            dirs.push(pathMatch);
                        }
                    }
                }
            }

            // Add the cache to the list as well
            AZStd::string projectCacheRootValue;
            AZ::SettingsRegistry::Get()->Get(projectCacheRootValue, AZ::SettingsRegistryMergeUtils::FilePathKey_CacheProjectRootFolder);
            projectCacheRootValue = AssetUtilities::NormalizeFilePath(projectCacheRootValue.c_str()).toUtf8().constData();
            m_excludedFolders.emplace(projectCacheRootValue);

            // Register to be notified about deletes so we can remove old ignored folders
            auto fileStateCache = AZ::Interface<IFileStateRequests>::Get();

            if (fileStateCache)
            {
                m_handler = AZ::Event<FileStateInfo>::Handler([this](FileStateInfo fileInfo)
                {
                    if (fileInfo.m_isDirectory)
                    {
                        AZStd::scoped_lock lock(m_pendingNewFolderMutex);

                        m_pendingDeletes.emplace(fileInfo.m_absolutePath.toUtf8().constData());
                    }
                });

                fileStateCache->RegisterForDeleteEvent(m_handler);
            }
            else
            {
                AZ_Error("ExcludedFolderCache", false, "Failed to find IFileStateRequests interface");
            }

            m_builtCache = true;
        }

        // Incorporate any pending folders
        AZStd::unordered_set<AZStd::string> pendingAdds;
        AZStd::unordered_set<AZStd::string> pendingDeletes;
        
        {
            AZStd::scoped_lock lock(m_pendingNewFolderMutex);
            pendingAdds.swap(m_pendingNewFolders);
            pendingDeletes.swap(m_pendingDeletes);
        }

        if (!pendingAdds.empty())
        {
            m_excludedFolders.insert(pendingAdds.begin(), pendingAdds.end());
        }

        if (!pendingDeletes.empty())
        {
            for (const auto& pendingDelete : pendingDeletes)
            {
                m_excludedFolders.erase(pendingDelete);
            }
        }

        return m_excludedFolders;
    }

    void ExcludedFolderCache::FileAdded(QString path)
    {
        QString relativePath, scanFolderPath;
        
        if (!m_platformConfig->ConvertToRelativePath(path, relativePath, scanFolderPath))
        {
            AZ_Error("ExcludedFolderCache", false, "Failed to get relative path for newly added file %s", path.toUtf8().constData());
            return;
        }

        AZ::IO::Path azPath(relativePath.toUtf8().constData());
        AZ::IO::Path absolutePath(scanFolderPath.toUtf8().constData());
        
        for (const auto& pathPart : azPath)
        {
            absolutePath /= pathPart;

            QString normalized = AssetUtilities::NormalizeFilePath(absolutePath.c_str());

            if (m_platformConfig->IsFileExcluded(normalized))
            {
                // Add the folder to a pending list, since this callback runs on another thread
                AZStd::scoped_lock lock(m_pendingNewFolderMutex);

                m_pendingNewFolders.emplace(normalized.toUtf8().constData());
                break;
            }
        }
    }
}

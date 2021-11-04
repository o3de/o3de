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

namespace AssetProcessor
{
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

            m_builtCache = true;
        }

        return m_excludedFolders;
    }
}

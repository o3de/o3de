/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AssetManager/ExcludedFolderCacheInterface.h>
#include <AssetManager/FileStateCache.h>

namespace AssetProcessor
{
    class PlatformConfiguration;

    struct ExcludedFolderCache : ExcludedFolderCacheInterface
    {
        explicit ExcludedFolderCache(const PlatformConfiguration* platformConfig);
        ~ExcludedFolderCache() override;

        // Gets a set of absolute paths to folder which have been excluded according to the platform configuration rules
        // Note - not thread safe
        const AZStd::unordered_set<AZStd::string>& GetExcludedFolders() override;

        void FileAdded(QString path) override;

        //! Initialize the cache from a known list of excluded folders, so that it does not have to do a scan
        //! for itself.  Destroys the input.
        void InitializeFromKnownSet(AZStd::unordered_set<AZStd::string>&& excludedFolders);

    private:
        bool m_builtCache = false;
        const PlatformConfiguration* m_platformConfig{};
        AZStd::unordered_set<AZStd::string> m_excludedFolders;

        AZStd::recursive_mutex m_pendingNewFolderMutex;
        AZStd::unordered_set<AZStd::string> m_pendingNewFolders; // Newly ignored folders waiting to be added to m_excludedFolders
        AZStd::unordered_set<AZStd::string> m_pendingDeletes;
        AZ::Event<FileStateInfo>::Handler m_handler;
    };
}

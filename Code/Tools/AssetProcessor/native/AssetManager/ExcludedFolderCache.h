/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

namespace AssetProcessor
{
    struct ExcludedFolderCache
    {
        ExcludedFolderCache(const PlatformConfiguration* platformConfig) : m_platformConfig(platformConfig) {}

        // Gets a set of absolute paths to folder which have been excluded according to the platform configuration rules
        // Note - not thread safe
        const AZStd::unordered_set<AZStd::string>& GetExcludedFolders();

    private:
        bool m_builtCache = false;
        const PlatformConfiguration* m_platformConfig{};
        AZStd::unordered_set<AZStd::string> m_excludedFolders;
    };
}
